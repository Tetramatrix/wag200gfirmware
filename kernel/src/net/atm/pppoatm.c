/* net/atm/pppoatm.c - RFC2364 PPP over ATM/AAL5 */

/* Copyright 1999-2000 by Mitchell Blank Jr */
/* Based on clip.c; 1995-1999 by Werner Almesberger, EPFL LRC/ICA */
/* And on ppp_async.c; Copyright 1999 Paul Mackerras */
/* And help from Jens Axboe */

/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * This driver provides the encapsulation and framing for sending
 * and receiving PPP frames in ATM AAL5 PDUs.
 */

//-------------------------------------------------------------------------------------
// Copyright 2004, Texas Instruments Incorporated
//
// This program has been modified from its original operation by Texas Instruments
// to do the following:
// 
// 1) Voice-Data coexistence - Protocol Inspection.
//
// THIS MODIFIED SOFTWARE AND DOCUMENTATION ARE PROVIDED
// "AS IS," AND TEXAS INSTRUMENTS MAKES NO REPRESENTATIONS 
// OR WARRENTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
// TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY 
// PARTICULAR PURPOSE OR THAT THE USE OF THE SOFTWARE OR 
// DOCUMENTATION WILL NOT INFRINGE ANY THIRD PARTY PATENTS, 
// COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. 
// See The GNU General Public License for more details.
//
// These changes are covered under version 2 of the GNU General Public License, 
// dated June 1991.
//-------------------------------------------------------------------------------------

/*
 * One shortcoming of this driver is that it does not comply with
 * section 8 of RFC2364 - we are supposed to detect a change
 * in encapsulation and immediately abort the connection (in order
 * to avoid a black-hole being created if our peer loses state
 * and changes encapsulation unilaterally.  However, since the
 * ppp_generic layer actually does the decapsulation, we need
 * a way of notifying it when we _think_ there might be a problem)
 * There's two cases:
 *   1.	LLC-encapsulation was missing when it was enabled.  In
 *	this case, we should tell the upper layer "tear down
 *	this session if this skb looks ok to you"
 *   2.	LLC-encapsulation was present when it was disabled.  Then
 *	we need to tell the upper layer "this packet may be
 *	ok, but if its in error tear down the session"
 * These hooks are not yet available in ppp_generic
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/ppp_channel.h>
#include <linux/atmppp.h>

/* For PROTOCOL Inspection. */
#include <linux/ip.h>
#include <linux/udp.h>

extern int mod_cong;
extern int gIsVoiceEnabled;


#if 0
#define DPRINTK(format, args...) \
	printk(KERN_DEBUG "pppoatm: " format, ##args)

#else
#define DPRINTK(format, args...)
#endif
	
#if 0
static void skb_debug(const struct sk_buff *skb)
{
#define NUM2PRINT 50
char buf[NUM2PRINT * 3 + 1];    /* 3 chars per byte */
int i = 0;
for (i = 0; i < skb->len && i < NUM2PRINT; i++) {
    sprintf(buf + i * 3, "%2.2x ", 0xff & skb->data[i]);
    }
printk(KERN_DEBUG "pppoatm: skb: %s\n", buf);
}
#endif


enum pppoatm_encaps {
	e_autodetect = PPPOATM_ENCAPS_AUTODETECT,
	e_vc = PPPOATM_ENCAPS_VC,
	e_llc = PPPOATM_ENCAPS_LLC,
};

struct pppoatm_vcc {
	struct atm_vcc	*atmvcc;	/* VCC descriptor */
	void (*old_push)(struct atm_vcc *, struct sk_buff *);
	void (*old_pop)(struct atm_vcc *, struct sk_buff *);
					/* keep old push/pop for detaching */
	enum pppoatm_encaps encaps;
	int flags;			/* SC_COMP_PROT - compress protocol */
	struct ppp_channel chan;	/* interface to generic ppp layer */
	struct tasklet_struct wakeup_tasklet;
};

/*
 * Header used for LLC Encapsulated PPP (4 bytes) followed by the LCP protocol
 * ID (0xC021) used in autodetection
 */
static const unsigned char pppllc[6] = { 0xFE, 0xFE, 0x03, 0xCF, 0xC0, 0x21 };
#define LLC_LEN		(4)

static inline struct pppoatm_vcc *atmvcc_to_pvcc(const struct atm_vcc *atmvcc)
{
	return (struct pppoatm_vcc *) (atmvcc->user_back);
}

static inline struct pppoatm_vcc *chan_to_pvcc(const struct ppp_channel *chan)
{
	return (struct pppoatm_vcc *) (chan->private);
}

/*
 * We can't do this directly from our _pop handler, since the ppp code
 * doesn't want to be called in interrupt context, so we do it from
 * a tasklet
 */
static void pppoatm_wakeup_sender(unsigned long arg)
{
	ppp_output_wakeup((struct ppp_channel *) arg);
}

/*
 * This gets called every time the ATM card has finished sending our
 * skb.  The ->old_pop will take care up normal atm flow control,
 * but we also need to wake up the device if we blocked it
 */
static void pppoatm_pop(struct atm_vcc *atmvcc, struct sk_buff *skb)
{
	struct pppoatm_vcc *pvcc = atmvcc_to_pvcc(atmvcc);
	pvcc->old_pop(atmvcc, skb);
	/*
	 * We don't really always want to do this since it's
	 * really inefficient - it would be much better if we could
	 * test if we had actually throttled the generic layer.
	 * Unfortunately then there would be a nasty SMP race where
	 * we could clear that flag just as we refuse another packet.
	 * For now we do the safe thing.
	 */
	tasklet_schedule(&pvcc->wakeup_tasklet);
}

/*
 * Unbind from PPP - currently we only do this when closing the socket,
 * but we could put this into an ioctl if need be
 */
static void pppoatm_unassign_vcc(struct atm_vcc *atmvcc)
{
	struct pppoatm_vcc *pvcc;
	pvcc = atmvcc_to_pvcc(atmvcc);
	atmvcc->push = pvcc->old_push;
	atmvcc->pop = pvcc->old_pop;
	tasklet_kill(&pvcc->wakeup_tasklet);
	ppp_unregister_channel(&pvcc->chan);
	atmvcc->user_back = NULL;
	kfree(pvcc);
	/* Gee, I hope we have the big kernel lock here... */
	MOD_DEC_USE_COUNT;
}

/**************************************************************************
 * FUNCTION NAME : pppoa_protocol_inspector
 **************************************************************************
 * DESCRIPTION   :
 *  The function is the protocol inspector which prioritizes the data 
 *  received on the DSL driver. The packet classifer works only on IP data.
 *  Voice typically is a UDP stream with well known ports.
 *
 * RETURNS       :
 *  0   - Drop the packet.
 *  1   - Accept the packet.
 *  
 * NOTES         :
 *  Any changes to this function should also be done if applicable in the 
 *  BR2684 protocol inspection routine defined in the file br2684.c.
 ***************************************************************************/
static int pppoa_protocol_inspector(struct sk_buff *skb)
{        
    /* Protocol inspection and packet prioritization is required only if VOICE
     * is enabled. While running pure data, there is no need to do all this
     * work. */
    if (gIsVoiceEnabled == 1)
    {
        extern void get_sample_stats(int cpu);
        char*               ptr_data; 
        unsigned short      protocol;
        int                 acceptance_level;
        struct iphdr*       ptr_ipheader = NULL;
        struct pppoatm_vcc* pvcc;

        /* PANKAJ TODO: Update the System Level congestion. This function gets 
         * called only from netif_rx. If we have a undirectional flow especially
         * downstream then the DSL driver will drop packets because of the
         * congestion since the protocol inspection is done much before the
         * packets are pushed up the NET layers and since we never again do a
         * netif_rx we are in a deadlock. This is not the correct way to fix
         * the problem. Will need to be investigated further... */
        get_sample_stats (smp_processor_id());

        /* Check the current congestion level. This can be used to define
         * acceptance level for low priority packets. Low priority packets
         * are acceptable till the NET is congested. */
        if (softnet_data[smp_processor_id()].cng_level >= NET_RX_CN_HIGH)
            acceptance_level = 0;
        else
            acceptance_level = 1;

        /* Get the pointer to the VCC handle on which the packet was received. */
        pvcc = atmvcc_to_pvcc(ATM_SKB(skb)->vcc);

        /* Initialize the data pointer. */
        ptr_data = skb->data;

        /* Skip the LLC encapsulation header. */
        if (pvcc->encaps == e_llc)
            ptr_data = ptr_data + LLC_LEN;

        /* Determine the PPP protocol field. */
        protocol = *((unsigned short *)ptr_data);

        /* Control comes here and we need to decide the protocol. */
        if (protocol == 0x2100)
        {
            /* IP Packet. */
            ptr_ipheader = (struct iphdr *)(ptr_data + 2);
        }
        else
        {
            /* Non-IP packet. */
            ptr_ipheader = NULL;
        }

        /* Is the packet IP or NOT ? */
        if (ptr_ipheader != NULL)
        {
            /* Is the packet UDP or not? */
            if (ptr_ipheader->protocol == 17)
            {
                extern int voice_port_match (unsigned int port);
                struct udphdr*  ptr_udpheader = (struct udphdr *) ((char *)ptr_ipheader + ptr_ipheader->ihl*4);

                /* Does the received packet's destination port match the voice ports. */
                if (voice_port_match (ptr_udpheader->dest) == 1)
                {
                    /* Match: Voice Packet. Mark the SKB as high priority and
                     * always accept the packet. */
                    skb->cb[46] = 0x1;
                    return 1;
                }
                else
                {
                    /* No Match: Data Packet. Mark the SKB as low priority and
                     * return the acceptance level as determined by the
                     * congestion. */
                    skb->cb[46] = 0x0;
                    return acceptance_level;
                }
            }
            else
            {
                /* TCP/ICMP packet. Mark these as low priority. */
                skb->cb[46] = 0x0;
                return acceptance_level;
            }
        }
        else
        {
            /* Non IP Packet received. Mark it as high priority, this could be
             * a PPP control packet. We dont want the link to go down because
             * of us dropping the packet. */
            skb->cb[46] = 0x1;
            return 1;
        }
    }
    else
    {
        /* VOICE was NOT enabled. Accept the packet. */
        return 1;
    }
}

/* Called when an AAL5 PDU comes in */
static void pppoatm_push(struct atm_vcc *atmvcc, struct sk_buff *skb)
{
	struct pppoatm_vcc *pvcc = atmvcc_to_pvcc(atmvcc);
        struct sk_buff *clones;
	DPRINTK("pppoatm push\n");
	if (skb == NULL) {			/* VCC was closed */
		DPRINTK("removing ATMPPP VCC %p\n", pvcc);
		pppoatm_unassign_vcc(atmvcc);
		atmvcc->push(atmvcc, NULL);	/* Pass along bad news */
		return;
	}
	atm_return(atmvcc, skb->truesize);
	switch (pvcc->encaps) {
	case e_llc:
	        DPRINTK("LLC: skb data ptr %p\n", skb->data);	
		if (skb->len < LLC_LEN ||
		    memcmp(skb->data, pppllc, LLC_LEN))
			goto error;

        /* There is no need to do alignment correction on the latest DSL driver
         * as the drivers handle the alignment issue. 
         * NOTE: Be careful, this fix has a dependancy on the latest DSL Driver and 
         * PPP Daemon. */
#if 0
		 /* alignment correction: */		 
		 clones = alloc_skb(skb->len + 2, GFP_ATOMIC);
	         if (clones == 0) {
	              printk(KERN_DEBUG "PPPoATM : no mem for cloned skb\n");
		      goto error;
		 }
		 else {
		      skb_reserve(clones,2);  
	              memcpy(skb_put(clones, skb->len), skb->data, skb->len);
                      kfree_skb(skb);
                      skb = clones;
                }
#endif
		skb_pull(skb, LLC_LEN);
		break;
	case e_autodetect:
		if (pvcc->chan.ppp == NULL) {	/* Not bound yet! */
			kfree_skb(skb);
			return;
		}
		if (skb->len >= sizeof(pppllc) &&
		    !memcmp(skb->data, pppllc, sizeof(pppllc))) {
			pvcc->encaps = e_llc;
			skb_pull(skb, LLC_LEN);
			break;
		}
		if (skb->len >= (sizeof(pppllc) - LLC_LEN) &&
		    !memcmp(skb->data, &pppllc[LLC_LEN],
		    sizeof(pppllc) - LLC_LEN)) {
			pvcc->encaps = e_vc;
			pvcc->chan.mtu += LLC_LEN;
			break;
		}
		DPRINTK(" Couldn't autodetect yet "
		    "(skb: %02X %02X %02X %02X %02X %02X)\n",
		    skb->data[0], skb->data[1], skb->data[2],
		    skb->data[3], skb->data[4], skb->data[5]);
		goto error;
	case e_vc:		 
        /* There is no need to do alignment correction on the latest DSL driver
         * as the drivers handle the alignment issue. 
         * NOTE: Be careful, this fix has a dependancy on the latest DSL Driver and 
         * PPP Daemon. */
#if 0
         /* alignment correction */
		 clones = alloc_skb(skb->len + 2, GFP_ATOMIC);
		 if (clones == 0) {
		   printk(KERN_DEBUG "PPPoATM : no mem for cloned skb\n");
		        goto error;
                 }
		 else {
		  skb_reserve(clones,2);
	          memcpy(skb_put(clones, skb->len), skb->data, skb->len);
	          kfree_skb(skb);
	          skb = clones;
	        }						   
#endif
		DPRINTK("VC MUX: skb data ptr %p\n", skb->data);
		break;
	}
	ppp_input(&pvcc->chan, skb);
	return;
    error:
	kfree_skb(skb);
	ppp_input_error(&pvcc->chan, 0);
}

/*
 * Called by the ppp_generic.c to send a packet - returns true if packet
 * was accepted.  If we return false, then it's our job to call
 * ppp_output_wakeup(chan) when we're feeling more up to it.
 * Note that in the ENOMEM case (as opposed to the !atm_may_send case)
 * we should really drop the packet, but the generic layer doesn't
 * support this yet.  We just return 'DROP_PACKET' which we actually define
 * as success, just to be clear what we're really doing.
 */
#define DROP_PACKET 1
static int pppoatm_send(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct pppoatm_vcc *pvcc = chan_to_pvcc(chan);
	ATM_SKB(skb)->vcc = pvcc->atmvcc;
	DPRINTK("pppoatm_send (skb=0x%p, vcc=0x%p)\n",
	    skb, pvcc->atmvcc);
	if (skb->data[0] == '\0' && (pvcc->flags & SC_COMP_PROT))
		(void) skb_pull(skb, 1);
	switch (pvcc->encaps) {		/* LLC encapsulation needed */
	case e_llc:
		if (skb_headroom(skb) < LLC_LEN) {
			struct sk_buff *n;
			n = skb_realloc_headroom(skb, LLC_LEN);
			if (n != NULL &&
			    !atm_may_send(pvcc->atmvcc, n->truesize)) {
				kfree_skb(n);
				goto nospace;
			}
			kfree_skb(skb);
			if ((skb = n) == NULL)
				return DROP_PACKET;
		} else if (!atm_may_send(pvcc->atmvcc, skb->truesize))
			goto nospace;
		memcpy(skb_push(skb, LLC_LEN), pppllc, LLC_LEN);
		break;
	case e_vc:
		if (!atm_may_send(pvcc->atmvcc, skb->truesize))
			goto nospace;
		break;
	case e_autodetect:
		DPRINTK("Trying to send without setting encaps!\n");
		kfree_skb(skb);
		return 1;
	}
	atomic_add(skb->truesize, &ATM_SKB(skb)->vcc->tx_inuse);
	ATM_SKB(skb)->iovcnt = 0;
	ATM_SKB(skb)->atm_options = ATM_SKB(skb)->vcc->atm_options;
	DPRINTK("atm_skb(%p)->vcc(%p)->dev(%p)\n",
	    skb, ATM_SKB(skb)->vcc,
	    ATM_SKB(skb)->vcc->dev);
	return ATM_SKB(skb)->vcc->send(ATM_SKB(skb)->vcc, skb)
	    ? DROP_PACKET : 1;
    nospace:
	/*
	 * We don't have space to send this SKB now, but we might have
	 * already applied SC_COMP_PROT compression, so may need to undo
	 */
	if ((pvcc->flags & SC_COMP_PROT) && skb_headroom(skb) > 0 &&
	    skb->data[-1] == '\0')
		(void) skb_push(skb, 1);
	return 0;
}

/* This handles ioctls sent to the /dev/ppp interface */
static int pppoatm_devppp_ioctl(struct ppp_channel *chan, unsigned int cmd,
	unsigned long arg)
{
	switch (cmd) {
	case PPPIOCGFLAGS:
		return put_user(chan_to_pvcc(chan)->flags, (int *) arg)
		    ? -EFAULT : 0;
	case PPPIOCSFLAGS:
		return get_user(chan_to_pvcc(chan)->flags, (int *) arg)
		    ? -EFAULT : 0;
	}
	return -ENOTTY;
}

static /*const*/ struct ppp_channel_ops pppoatm_ops = {
	start_xmit: pppoatm_send,
	ioctl: pppoatm_devppp_ioctl,
};

static int pppoatm_assign_vcc(struct atm_vcc *atmvcc, unsigned long arg)
{
	struct atm_backend_ppp be;
	struct pppoatm_vcc *pvcc;
	int err;
	/*
	 * Each PPPoATM instance has its own tasklet - this is just a
	 * prototypical one used to initialize them
	 */
	static const DECLARE_TASKLET(tasklet_proto, pppoatm_wakeup_sender, 0);
	if (copy_from_user(&be, (void *) arg, sizeof be))
		return -EFAULT;
	if (be.encaps != PPPOATM_ENCAPS_AUTODETECT &&
	    be.encaps != PPPOATM_ENCAPS_VC && be.encaps != PPPOATM_ENCAPS_LLC)
		return -EINVAL;
	MOD_INC_USE_COUNT;
	pvcc = kmalloc(sizeof(*pvcc), GFP_KERNEL);
	if (pvcc == NULL) {
		MOD_DEC_USE_COUNT;
		return -ENOMEM;
	}
	memset(pvcc, 0, sizeof(*pvcc));
	pvcc->atmvcc = atmvcc;
	pvcc->old_push = atmvcc->push;
	pvcc->old_pop = atmvcc->pop;
	pvcc->encaps = (enum pppoatm_encaps) be.encaps;
	pvcc->chan.private = pvcc;
	pvcc->chan.ops = &pppoatm_ops;
	pvcc->chan.mtu = atmvcc->qos.txtp.max_sdu - PPP_HDRLEN -
	    (be.encaps == e_vc ? 0 : LLC_LEN);
	pvcc->wakeup_tasklet = tasklet_proto;
	pvcc->wakeup_tasklet.data = (unsigned long) &pvcc->chan;
	if ((err = ppp_register_channel(&pvcc->chan)) != 0) {
		kfree(pvcc);
		return err;
	}
	atmvcc->user_back = pvcc;
	atmvcc->push = pppoatm_push;
	atmvcc->pop = pppoatm_pop;
    
    /* PANKAJ: Adding support for PPPoA protocol inspection. */
    atmvcc->protocol_inspector = pppoa_protocol_inspector;

	return 0;
}

/*
 * This handles ioctls actually performed on our vcc - we must return
 * -ENOIOCTLCMD for any unrecognized ioctl
 */
static int pppoatm_ioctl(struct atm_vcc *atmvcc, unsigned int cmd,
	unsigned long arg)
{
	if (cmd != ATM_SETBACKEND && atmvcc->push != pppoatm_push)
		return -ENOIOCTLCMD;
	switch (cmd) {
	case ATM_SETBACKEND: {
		atm_backend_t b;
		if (get_user(b, (atm_backend_t *) arg))
			return -EFAULT;
		if (b != ATM_BACKEND_PPP)
			return -ENOIOCTLCMD;
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		return pppoatm_assign_vcc(atmvcc, arg);
		}
	case PPPIOCGCHAN:
		return put_user(ppp_channel_index(&atmvcc_to_pvcc(atmvcc)->
		    chan), (int *) arg) ? -EFAULT : 0;
	case PPPIOCGUNIT:
		return put_user(ppp_unit_number(&atmvcc_to_pvcc(atmvcc)->
		    chan), (int *) arg) ? -EFAULT : 0;
	}
	return -ENOIOCTLCMD;
}

/* the following avoids some spurious warnings from the compiler */
#define UNUSED __attribute__((unused))

extern int (*pppoatm_ioctl_hook)(struct atm_vcc *, unsigned int, unsigned long);

static int __init UNUSED pppoatm_init(void)
{
	pppoatm_ioctl_hook = pppoatm_ioctl;
	return 0;
}

static void __exit UNUSED pppoatm_exit(void)
{
	pppoatm_ioctl_hook = NULL;
}

module_init(pppoatm_init);
module_exit(pppoatm_exit);

MODULE_AUTHOR("Mitchell Blank Jr <mitch@sfgoth.com>");
MODULE_DESCRIPTION("RFC2364 PPP over ATM/AAL5");
