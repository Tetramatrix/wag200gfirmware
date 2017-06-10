/* Description : To develop IGMP - Multicast Aware Bridge
 *
 * Date : 21-Oct-2004
 *
 *
 */


/* File could be included depending on CONFIG_BRIDGED_MULTICAST
 *
 */

/* #ifdef CONFIG_BRIDGED_MULTICAST */


#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/if_bridge.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include "br_private.h"
#include "br_mfd.h"

/*TXTL_IGMP01 25-oct-2004*/
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/igmp.h>	
#include <net/checksum.h>

#define IGMP_SIZE (sizeof(struct igmphdr)+sizeof(struct iphdr))
#define INPUT	1
#define FORWARD 0

#ifndef TI_SLOW_PATH
extern void br_flood(struct net_bridge *br,struct sk_buff *skb,int clone);
extern int should_deliver(struct net_bridge_port *p, struct sk_buff *skb);	
extern void br_send_frame_error(struct sk_buff *skb);
extern void br_send_frame(struct net_bridge_port *to, struct sk_buff *skb);
#else
extern void br_flood_deliver(struct net_bridge *br, struct sk_buff *skb, int clone);
extern void __br_deliver(struct net_bridge_port *to, struct sk_buff *skb);
#endif

extern void br_selective_flood(struct net_bridge *br, struct net_bridge_mfd_entry *mfd, struct sk_buff *skb);


static __inline__ int br_addr_hash(__u32 *addr);
static __inline__ void __mfd_hash_link(struct net_bridge *br,struct net_bridge_mfd_entry *ent,int hash);
static __inline__ void __mfd_hash_unlink(struct net_bridge_mfd_entry *ent);
void br_other_querier_present_timer_expired(unsigned long __data);
void br_general_query_timer_expired(unsigned long __data);
static int br_send_igmp_query(struct net_bridge *br,__u32 group,int max_resp_time);
int  br_enable_proxy_mode(struct net_bridge *br);
int br_disable_proxy_mode(struct net_bridge *br);
void br_mfd_init(struct net_bridge *br);
void br_mfd_close(struct net_bridge *br);
void br_membership_timer_expired(unsigned long __data);
static __inline__ void copy_mfd(struct __mfd_entry *ent, struct net_bridge_mfd_entry *f);
void br_mfd_cleanup(struct net_bridge *br);
void br_mfd_delete_all_ports(struct net_bridge_mfd_entry *f);
int br_mfd_delete_v1_ports(struct net_bridge *br);
void br_mfd_delete_port(struct net_bridge *br, struct net_bridge_port *p);
void br_mfd_delete_port_from_group(struct net_bridge *br, struct net_bridge_port *p,__u32 group);
void br_leave_report_received(struct net_bridge *br, struct net_bridge_port *p,__u32 group);
int br_get_mc_status(struct net_bridge *br, unsigned char *_buf);
int br_mfd_get_entries(struct net_bridge *br, unsigned char *_buf);
struct net_bridge_mfd_entry *br_mfd_get(struct net_bridge *br, __u32 addr);
static __inline__ int __mfd_insert_port(struct net_bridge *br,struct net_bridge_mfd_entry *mfd,
					struct net_bridge_port *source,__u8 type, struct sk_buff *skb);
void br_mfd_insert(struct net_bridge *br,struct net_bridge_port *source, __u32 *addr,__u8 type,struct sk_buff *skb);
__u8 br_handle_mc(struct net_bridge *br, struct sk_buff *skb,int mode);
void br_dont_forward_report(struct net_bridge *br,__u32 group, int exclude_if); 
void br_mc_learn_and_flood(struct net_bridge *br, struct net_bridge_port *source, __u8 type,__u32 *group,struct sk_buff *skb);

__u8 br_input_handle_mc(struct net_bridge *br,struct sk_buff *skb);
__u8 br_forward_handle_mc(struct net_bridge *br,struct sk_buff *skb);

static __inline__ int br_addr_hash(__u32 *addr)
{
	unsigned long x;
	
	x = (__u8)(*addr);
	x = (x << 2) ^ (__u8)(*(addr));
	x = (x << 2) ^ (__u8)(*(addr));
	x = (x << 2) ^ (__u8)(*(addr));
	

	x ^= x >> 8;

	return x & (BR_HASH_SIZE - 1);
}

static __inline__ void __mfd_hash_link(struct net_bridge *br,
				   struct net_bridge_mfd_entry *ent,
				   int hash)
{
	ent->next_hash = br->mfd_hash[hash];
	if (ent->next_hash != NULL)
		ent->next_hash->pprev_hash = &ent->next_hash;
	br->mfd_hash[hash] = ent;
	ent->pprev_hash = &br->mfd_hash[hash];
}

static __inline__ void __mfd_hash_unlink(struct net_bridge_mfd_entry *ent)
{
	*(ent->pprev_hash) = ent->next_hash;
	if (ent->next_hash != NULL)
		ent->next_hash->pprev_hash = ent->pprev_hash;
	ent->next_hash = NULL;
	ent->pprev_hash = NULL;
}

#ifdef CONFIG_BRIDGE_CONF_MODE
void br_other_querier_present_timer_expired(unsigned long __data)
{
	struct net_bridge *br = (struct net_bridge *)__data;
	struct timer_list *timer = &br->general_query_timer;
	
	/* Since, query has not been received from the other router acting as Querier for a time period of Other Querier
	 * Present timer i.e for 300 sec, bridge should assume the role of Querier 
	 */

	DEBUGP(KERN_INFO "Bridge %s : Other Querier Present Timer Expired ; Switching to Querier\n", br->dev.name);

	br->isQuerier = IGMP_QUERIER;
	
	br_send_igmp_query(br, 0, DEFAULT_QUERY_RESPONSE_INTERVAL*10 );

	init_timer(timer);
	timer->data =(unsigned long)br;
	timer->function = br_general_query_timer_expired;
	timer->expires = jiffies + DEFAULT_QUERY_INTERVAL * HZ;
	add_timer(timer);
	
}
	
void br_general_query_timer_expired(unsigned long __data)
{
	struct net_bridge *br = (struct net_bridge *)__data;
	struct timer_list *timer = &br->general_query_timer;
	
	DEBUGP(KERN_INFO "Bridge %s : General Query Timer Expired\n", br->dev.name);
	br_send_igmp_query(br,0,DEFAULT_QUERY_RESPONSE_INTERVAL*10);

	timer->expires = jiffies + DEFAULT_QUERY_INTERVAL * HZ;
	add_timer(timer);		
  
}


static int br_send_igmp_query(struct net_bridge *br,__u32 group,int max_resp_time)
{
	struct sk_buff *skb;
	struct ethhdr *eh;
	struct iphdr *iph;
	struct igmphdr *igmph;
	struct rtable *rt;
	struct net_device *dev;
	u32  dst;
	u32  addr;


	if (group == 0)
	{
		DEBUGP("Bridge %s sending IGMP General Query\n", br->dev.name);
		dst = IGMP_ALL_HOSTS;
	}
	else
	{
		DEBUGP("Bridge %s sending IGMP Specific Query for %x\n", br->dev.name, group);
 		dst = group;
	}

	br_dont_forward_report(br, group, -1);
	
	dev = &br->dev;
	
	if (ip_route_output(&rt, dst, 0, 0, dev->ifindex))
		return -1;
	if (rt->rt_src == 0) {
		ip_rt_put(rt);
		return -1;
	}
   
	skb = alloc_skb(IGMP_SIZE + dev->hard_header_len + 15, GFP_ATOMIC);
	if(skb == NULL)
	{
		ip_rt_put(rt);
		return -1;	
	}
	
	skb->dst = &rt->u.dst;

	skb_reserve(skb, (dev->hard_header_len + 15)& ~15);
	
	skb->mac.ethernet = eh = (struct ethhdr *)skb_put(skb, sizeof(struct ethhdr));
	
	addr = ntohl(dst);

	eh->h_dest[0] = 0x01;
	eh->h_dest[1] = 0x00;
	eh->h_dest[2] = 0x5E;
	eh->h_dest[5] = addr&0xFF;
	addr >>= 8;
	eh->h_dest[4] = addr&0xFF;
	addr >>= 8; 
	eh->h_dest[3] = addr&0x7F;

	memcpy(eh->h_source, dev->dev_addr, dev->addr_len);

	eh->h_proto = htons(ETH_P_IP);
 
	skb->nh.iph = iph = (struct iphdr *)skb_put(skb,sizeof(struct iphdr));
	
	iph->version  = 4;
	iph->ihl      = (sizeof(struct iphdr))>>2;
	iph->tos      = 0;
	iph->frag_off = __constant_htons(IP_DF);
	iph->ttl      = 1;
	iph->daddr    = dst;
	iph->saddr    = rt->rt_src;
	iph->protocol = IPPROTO_IGMP;
	iph->tot_len  = htons(IGMP_SIZE);
	ip_select_ident(iph, &rt->u.dst, NULL);

	ip_send_check(iph);

	igmph = (struct igmphdr *)skb_put(skb, sizeof(struct igmphdr));
	igmph->type=IGMP_MEMBERSHIP_QUERY;
	igmph->code=max_resp_time;
	igmph->csum=0;
	igmph->group=group;
	igmph->csum=ip_compute_csum((void *)igmph, sizeof(struct igmphdr));

	br_flood(br,skb,0);	
	
	return 0;

}

int  br_enable_proxy_mode(struct net_bridge *br)
{

	/* If qeneral query timer present, delete that */
	struct timer_list *timer;
	if(br->isQuerier == IGMP_QUERIER)
	{
		timer= &br->general_query_timer;
		del_timer(timer);
	}
	else if (br->isQuerier == IGMP_NON_QUERIER)	/* If other query timer present, delete that */
	{
		timer = &br->other_querier_present_timer;
		del_timer(timer);
	}

	/* Querier / Non-querier is Not Applicable for Bridge. IGMP Proxy takes care of Querier / Non-Querier */
	br->isQuerier = IGMP_QUERIER_NA; 

	DEBUGP(KERN_INFO "Bridge %s Conf Mode : MULTICAST_BRIDGE_WITH_IGMP_PROXY enabled\n", br->dev.name);
	br->conf_mode = MULTICAST_BRIDGE_WITH_IGMP_PROXY;
	return 0;

}

int br_disable_proxy_mode(struct net_bridge *br)
{ 
	/* add the qeneral query timer */
	struct timer_list *timer = &br->general_query_timer;
	
	DEBUGP(KERN_INFO "Bridge %s Conf Mode : MULTICAST_BRIDGE_ONLY enabled\n", br->dev.name);

	br->conf_mode = MULTICAST_BRIDGE_ONLY;
	br->isQuerier = IGMP_QUERIER; /* Multicast Bridge begins as a Querier */

	br_send_igmp_query(br, 0, DEFAULT_QUERY_RESPONSE_INTERVAL*10);

	init_timer(timer);
	timer->data = (unsigned long) br;
	timer->function = br_general_query_timer_expired;
	timer->expires = jiffies + DEFAULT_QUERY_INTERVAL* HZ;
	add_timer(timer);
	
	return 0;
}
#endif /* #ifdef CONFIG_BRIDGE_CONF_MODE */

void br_mfd_init(struct net_bridge *br)
{
	struct timer_list *timer;

	br->mc_forwarding = MULTICAST_FORWARDING_ENABLED; /* Default value could be set as per client's requirements */
#ifdef CONFIG_BRIDGE_CONF_MODE
	if(br->mc_forwarding == MULTICAST_FORWARDING_ENABLED)
	{	
		br->conf_mode = MULTICAST_BRIDGE_ONLY;
		br->isQuerier = IGMP_QUERIER; /* Bridge begins as a Querier */
		br_send_igmp_query(br,0,DEFAULT_QUERY_RESPONSE_INTERVAL * 10); 

		/* Although, sending Query is not effective now, since bridge interfaces are not added while bridge is 
	         * added, Querier Timer is started. Bridge interfaces are added subsequently and dynamically.
		 * When Querier timer expires, General Query is flooded on all bridged interfaces - this is effective
		 */

		timer = &br->general_query_timer;
		init_timer(timer);
		timer->data = (unsigned long) br;
		timer->function = br_general_query_timer_expired;
		timer->expires = jiffies + DEFAULT_QUERY_INTERVAL* HZ;
		add_timer(timer);
	}
#endif
	
}


void br_mfd_close(struct net_bridge *br)
{
	struct timer_list *timer;
#ifdef CONFIG_BRIDGE_CONF_MODE
	if(br->conf_mode == MULTICAST_BRIDGE_ONLY)
	{

		if (br->isQuerier == IGMP_QUERIER)
		{
			timer = &br->general_query_timer;
			del_timer(timer);
		}
		else if (br->isQuerier == IGMP_NON_QUERIER)
		{
			timer = &br->other_querier_present_timer;
			del_timer(timer);
		}	
	}
	else
	{
		/* No Query timers are running*/
	}
#endif
	
}

void br_membership_timer_expired(unsigned long __data)
{
	struct port_list *pnode = (struct port_list *)__data;
	struct net_bridge *br = pnode->dst->br;
	struct net_bridge_mfd_entry *mfd = pnode->mfd;

	DEBUGP("Group membership timer expired for %s port for mcgrp %04x\n", pnode->dst->dev->name, mfd->addr);

	//delete the port from the mfd
	br_mfd_delete_port_from_group(br, pnode->dst, mfd->addr);

}

static __inline__ void copy_mfd(struct __mfd_entry *ent, struct net_bridge_mfd_entry *f)
{
	struct port_list *pnode;
	int port_count=0;
	

	memset(ent, 0, sizeof(struct __mfd_entry));
	ent->grp_addr = f->addr;

	pnode = f->plist;

	while(pnode != NULL)
	{
		ent->port_list[port_count] = pnode->dst->dev->ifindex;
		ent->vinfo_list[port_count] = pnode->vinfo;
		port_count++;
		pnode = pnode->next;
	}
		
	ent->no_ports = port_count;
}

void br_mfd_cleanup(struct net_bridge *br)
{
	int i;

	DEBUGP("br_mfd_cleanup:\n");

	write_lock_bh(&br->mfd_hash_lock);
	for (i=0;i<BR_HASH_SIZE;i++) {
		struct net_bridge_mfd_entry *f;

		f = br->mfd_hash[i];
		while (f != NULL) {
			struct net_bridge_mfd_entry *g;

			g = f->next_hash;
			
			br_mfd_delete_all_ports(f);	
			__mfd_hash_unlink(f);
			kfree(f);
		
			f = g;
		}
	}
	write_unlock_bh(&br->mfd_hash_lock);
}

void br_mfd_delete_all_ports(struct net_bridge_mfd_entry *f)
{
	
	struct port_list *pnode = f->plist;
	struct port_list *tnode;

	DEBUGP("br_mfd_delete_all_ports:\n");
	
	while(pnode != NULL)
	{
		tnode = pnode;
		pnode = pnode->next;
		kfree(tnode);
	}

	atomic_set(&f->port_count, 0);
	f->plist = NULL;

}

int br_mfd_delete_v1_ports(struct net_bridge *br)
{
	int i;
   struct port_list *tnode = NULL, *prev_pnode = NULL;

	DEBUGP("br_mfd_delete_v1_ports:\n");
                                                                                                                            
   write_lock_bh(&br->mfd_hash_lock);
   for (i = 0; i < BR_HASH_SIZE; i++) 
   {
   struct net_bridge_mfd_entry *f;                                    
	f = br->mfd_hash[i];
   while (f != NULL) 
	{
        struct net_bridge_mfd_entry *g;
        struct port_list *pnode;                         
        g = f->next_hash;         
	pnode = f->plist;
	while(pnode != NULL)
	{				
	if(pnode->vinfo & 1)
            {
            	/*remove the port*/
              tnode = pnode;
              pnode = pnode->next;

				  if (prev_pnode != NULL)
						prev_pnode->next = pnode;
				  else
						f->plist = pnode;
		
              kfree(tnode);
         		/*if it is last port to be removed */
		        if(atomic_dec_and_test(&f->port_count))
       		  {
            	 __mfd_hash_unlink(f);
             	kfree(f);
				   break;	
				  }         		
				}
				else
				{
					prev_pnode = pnode;
					pnode = pnode->next;
				}
          }
         f = g;
      }
   }
   write_unlock_bh(&br->mfd_hash_lock);
	return 0;                                                                
}

void br_mfd_delete_port(struct net_bridge *br, struct net_bridge_port *p)
{
	int i;
	struct port_list *tnode = NULL, *prev_pnode = NULL;
	DEBUGP("br_mfd_delete_port:\n");

	write_lock_bh(&br->mfd_hash_lock);
	for (i=0; i < BR_HASH_SIZE; i++) 
	{
		struct net_bridge_mfd_entry *f;

		f = br->mfd_hash[i];
		while (f != NULL) 
		{
			struct net_bridge_mfd_entry *g;
			struct port_list *pnode;

			g = f->next_hash;
			pnode = f->plist;
			while(pnode != NULL)
			{
				if (pnode->dst == p)
				{
					/*remove the port*/
					tnode = pnode;
					pnode = pnode->next;

					if (prev_pnode != NULL)
						prev_pnode->next = pnode;
					else
						f->plist = pnode;

					kfree(tnode);
					/*if it is last port to be removed */
					if(atomic_dec_and_test(&f->port_count))
					{
						__mfd_hash_unlink(f);
						kfree(f);
					}
					break;
									
				}
				else
				{
					prev_pnode = pnode;
					pnode = pnode->next;
				}	
			}			
			f = g;
		}
	}
	write_unlock_bh(&br->mfd_hash_lock);
}


void br_mfd_delete_port_from_group(struct net_bridge *br, struct net_bridge_port *p,__u32 group)
{
	int i;
	struct port_list *tnode = NULL, *prev_pnode = NULL;

	DEBUGP("br_mfd_delete_port_from_group: %04x\n", group);

	write_lock_bh(&br->mfd_hash_lock);
	for (i=0; i<BR_HASH_SIZE; i++) 
	{
		struct net_bridge_mfd_entry *f;
		f = br->mfd_hash[i];
		while (f != NULL) 
		{
			struct net_bridge_mfd_entry *g;
			struct port_list *pnode;
			g = f->next_hash;			
			if(f->addr == group)
			{
				DEBUGP("br_mfd_delete_port_from_group: mfd found for %04x\n", group);
				pnode = f->plist;
				while(pnode != NULL)
				{
					if(pnode->dst == p)
					{
						/*remove the port*/
						
						DEBUGP("br_mfd_delete_port_from_group : port is %s\n", pnode->dst->dev->name);
						tnode = pnode;
						pnode = pnode->next;

						if (prev_pnode != NULL)
							prev_pnode->next = pnode;
						else
							f->plist = pnode;

						kfree(tnode);
						/*if it is last port to be removed */
						if(atomic_dec_and_test(&f->port_count))
						{
							__mfd_hash_unlink(f);
							kfree(f);
						}
						break;
					}
					else
					{
						prev_pnode = pnode;
						pnode = pnode->next;
					}	
				}		
				break;
			}
			f = g;
		}
	}
	write_unlock_bh(&br->mfd_hash_lock);
}

void br_leave_report_received(struct net_bridge *br, struct net_bridge_port *p,__u32 group)
{
   int i;
   struct port_list *tnode = NULL;
   struct timer_list *timer;	

   DEBUGP("br_leave_report_received: %04x\n", group);

   write_lock_bh(&br->mfd_hash_lock);
   for (i=0; i<BR_HASH_SIZE; i++)
   {
      struct net_bridge_mfd_entry *f;
      f = br->mfd_hash[i];
      while (f != NULL)
      {
         struct net_bridge_mfd_entry *g;
         struct port_list *pnode;
         g = f->next_hash;
         if(f->addr == group)
         {
            DEBUGP("br_leave_report_received: mfd found for %04x\n", group);
            pnode = f->plist;
            while(pnode != NULL)
				{
               if(pnode->dst == p)
               {

                  DEBUGP("br_leave_report_received for port %s and group membership timer updated\n", pnode->dst->dev->name);

#ifdef CONFIG_BRIDGE_CONF_MODE
						/* If configuration mode is Bridge only and Bridge is acting as Querier, then send Specific Query */
						if((br->conf_mode == MULTICAST_BRIDGE_ONLY) && (br->isQuerier == IGMP_QUERIER))
						{
							DEBUGP("Sending specific query for group %x\n",group);
							br_send_igmp_query(br,group,DEFAULT_SPECIFIC_QUERY_RESPONSE_INTERVAL *10);
						}
#endif

               	/* update timer value*/
						timer = &pnode->mcgrp_ageing_timer;

				 		del_timer(timer);
						init_timer(timer);
						timer->data = (unsigned long) pnode;
						timer->function = br_membership_timer_expired;
						timer->expires = jiffies + DEFAULT_LEAVE_TIMER * HZ;
						add_timer(timer);
	
						break;
	            }
               else
               {
                  pnode = pnode->next;
					}
			   }
            break;
         }
         f = g;
      }
   }
   write_unlock_bh(&br->mfd_hash_lock);
}

int br_get_mc_status(struct net_bridge *br, unsigned char *_buf)
{
	struct __mc_status mcs;
	struct __mc_status *temp = (struct __mc_status *)_buf;
	int err;

	memset(&mcs,0,sizeof(struct __mc_status));

	mcs.mc_forwarding = br->mc_forwarding;

#ifdef CONFIG_BRIDGE_CONF_MODE
	if(br->mc_forwarding == MULTICAST_FORWARDING_ENABLED)
	{
		mcs.conf_mode = br->conf_mode;
		mcs.isQuerier = br->isQuerier;
	}
	else
	{
		mcs.conf_mode = 0;
		mcs.isQuerier = 0;
	}
#endif
	err = copy_to_user(temp, &mcs,sizeof(struct __mc_status));
	
	if(err)
		return -EFAULT;
	else
		return 0;
}

int br_mfd_get_entries(struct net_bridge *br, unsigned char *_buf)
{
	int i;
	int num;
	struct __mfd_entry *walk;
	
	num = 0;
	walk = (struct __mfd_entry *)_buf;

	read_lock_bh(&br->mfd_hash_lock);
	for (i=0;i<BR_HASH_SIZE;i++) {
		struct net_bridge_mfd_entry *mfd;

		mfd = br->mfd_hash[i];
		while (mfd != NULL ) {
			struct __mfd_entry ent;
			int err;
			struct net_bridge_mfd_entry *g;
			struct net_bridge_mfd_entry **pp; 

			copy_mfd(&ent, mfd);

			read_unlock_bh(&br->mfd_hash_lock);
			err = copy_to_user(walk, &ent, sizeof(struct __mfd_entry));
			read_lock_bh(&br->mfd_hash_lock);


			g = mfd->next_hash;
			pp = mfd->pprev_hash;

			if (err)
				goto out_fault;

			if (g == NULL && pp == NULL)
				goto out_disappeared;

			num++;
			walk++;

			mfd = g;
		}
	}

 out:
	read_unlock_bh(&br->mfd_hash_lock);
	return num;

 out_disappeared:
	num = -EAGAIN;
	goto out;

 out_fault:
	num = -EFAULT;
	goto out;
}

struct net_bridge_mfd_entry *br_mfd_get(struct net_bridge *br, __u32 addr)
{
	struct net_bridge_mfd_entry *mfd;	
	DEBUGP("br_mfd_get: for addr %x\n", addr);	
	read_lock_bh(&br->mfd_hash_lock);
	mfd = br->mfd_hash[br_addr_hash(&addr)];
	while (mfd != NULL)
	{
		if(mfd->addr == addr)
		{ 	
			DEBUGP("Found mfd entry for addr %04x\n", addr);	
			read_unlock_bh(&br->mfd_hash_lock);
			return mfd;
		}
		mfd = mfd->next_hash;
	}
	read_unlock_bh(&br->mfd_hash_lock);
	return NULL;
}

static __inline__ int __mfd_insert_port(struct net_bridge *br,struct net_bridge_mfd_entry *mfd,
					struct net_bridge_port *source,__u8 type, struct sk_buff *skb)
{
	struct port_list *p;
	struct port_list *new_port;
	struct timer_list *timer;

	DEBUGP("__mfd_insert_port:\n");
	if(mfd->plist != NULL)
	{
	
		for(p = mfd->plist; p != NULL; p=p->next)
		{	
			/*if port already present*/
			if(p->dst == source)
			{
				/* update the version information if it is V1 member*/
				if( p->vinfo == 0) 
				{ 
					if(type == IGMP_V1_MEMBERSHIP_REPORT)
						p->vinfo = 0x01;
				}

				/* update the timer value*/
				timer = &p->mcgrp_ageing_timer;
				del_timer(timer);
				init_timer(timer);
				timer->data =(unsigned long)p;
				timer->function = br_membership_timer_expired;
				timer->expires = jiffies + DEFAULT_GROUP_MEMBERSHIP_TIMER * HZ;
				add_timer( &p->mcgrp_ageing_timer );

				/* Reset the membership report forward flag */
				p->dont_forward_report = 0;

				/* Considering Report Suppression Mechanism, membership report should be selectively flooded.
				 * Refer RFC2236 - Protocol Description section for Report Suppression Mechanism
				 */
				br_selective_flood(br,mfd,skb);

				DEBUGP("Group membership timer updated for %s port for mcgrp %04x\n", p->dst->dev->name, mfd->addr);
				return 0;
			}
		}
	}
	/* add a new port to the list*/
	new_port = kmalloc(sizeof(struct port_list),GFP_ATOMIC);
	if (new_port == NULL) 
	{
		printk(KERN_ERR "In __mfd_insert_port : kmalloc failed for new port\n");
		return 1;
	}

	new_port->next = mfd->plist;
	new_port->mfd = mfd;
	new_port->dst = source;

	if(type == IGMP_V1_MEMBERSHIP_REPORT) /*V1 MEMBERSHIP */
		new_port->vinfo = 0x1;
	else
		new_port->vinfo = 0x0;

	new_port->dont_forward_report = 0; /* Forward Membership report on this interface */

	/* set the ageing timer for the newly created port */

	timer = &new_port->mcgrp_ageing_timer;
	init_timer(timer);
	timer->data = (unsigned long)new_port;
	timer->function = br_membership_timer_expired;
	timer->expires = jiffies + DEFAULT_GROUP_MEMBERSHIP_TIMER * HZ;
	add_timer(timer);
	
	DEBUGP("Group membership timer added for %s port for mcgrp %04x\n", new_port->dst->dev->name, mfd->addr);

	mfd->plist = new_port;
	atomic_inc(&mfd->port_count);

	/* Considering Report Suppression Mechanism, membership report should be selectively flooded.
	 * Refer RFC2236 - Protocol Description section for Report Suppression Mechanism
	 */
	br_selective_flood(br,mfd,skb);

   return 0;
}	


void br_mfd_insert(struct net_bridge *br,
		   struct net_bridge_port *source,
		   __u32 *addr,__u8 type,struct sk_buff *skb)
{
	struct net_bridge_mfd_entry *mfd;
	int hash;

	DEBUGP("br_mfd_insert:\n");

	hash = br_addr_hash(addr);

	DEBUGP("\tHash value:%d\n",hash);

	write_lock_bh(&br->mfd_hash_lock);
	mfd = br->mfd_hash[hash];
	while (mfd != NULL) 
	{
		DEBUGP("\tmfd->addr: %x\n",mfd->addr);
		DEBUGP("\tgroup addr: %x\n",*addr);

		if(mfd->addr == *addr)
		{
			DEBUGP("mfd_group_entry already exists\n");
			__mfd_insert_port(br,mfd,source,type,skb);
			write_unlock_bh(&br->mfd_hash_lock);
			return;
		}

		mfd = mfd->next_hash;
	}

	mfd = kmalloc(sizeof(*mfd), GFP_ATOMIC);
	if (mfd == NULL) {
		write_unlock_bh(&br->mfd_hash_lock);
		printk(KERN_ERR "In br_mfd_insert : kmalloc failed for new mfd entry\n");
		return;
	}
	
	DEBUGP("New mfd entry created\n");

	mfd->addr = *addr;
	mfd->plist = NULL;
	atomic_set(&mfd->port_count, 0);
	__mfd_insert_port(br,mfd,source,type,skb);
	__mfd_hash_link(br, mfd, hash);

	write_unlock_bh(&br->mfd_hash_lock);
}


__u8 br_forward_handle_mc(struct net_bridge *br,struct sk_buff *skb)
{
	int ihl;
	struct ethhdr ethh;
	struct igmphdr igmph;
	struct iphdr iph;
	int i;
	unsigned short protocol;	
	__u8 type;
	__u32 group;
	struct timer_list *timer;
	unsigned char* data_ptr;
	struct in_device *in_dev;
	struct net_bridge_mfd_entry* mfd;
	struct sk_buff *skb2;
	struct port_list* pnode;

	data_ptr = skb->data;
	memcpy(&ethh,(struct ethhdr*)data_ptr,sizeof(struct ethhdr));
	data_ptr += sizeof(struct ethhdr);

	DEBUGP("\tMAC header:\n");
	DEBUGP("\t  MAC Dest Addr :");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_dest[i]);
	}
	DEBUGP("\n\t  MAC Source Addr : ");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_source[i]);
	}
	DEBUGP("\n\t  Protocol Type :%x\n",ethh.h_proto);
	protocol = ethh.h_proto;

	
	/*Determine if the protocol is ip */
	if (ntohs(protocol) == ETH_P_IP)
	{
		memcpy(&iph,(struct iphdr *)data_ptr,sizeof(struct iphdr));
		
#if 0
	   if (ip_fast_csum((u8 *)iph, iph->ihl) != 0)
		{
	      kfree_skb(clone); 
			return MFD_PARSE_ERROR;
		}
                                                                                
    	{
        	__u32 len = ntohs(iph->tot_len);
	      if (clone->len < len || len < (iph->ihl<<2))
			{
        	                kfree_skb(clone);
				return MFD_PARSE_ERROR;
			}
                                                                                
	      /* Our transport medium may have padded the buffer out. Now we know it
   	    * is IP we can trim to the true length of the frame.
        	 * Note this now means skb->len holds ntohs(iph->tot_len).
	       */
         if (clone->len > len) {
            __pskb_trim(clone, len);
           	if (clone->ip_summed == CHECKSUM_HW)
          	clone->ip_summed = CHECKSUM_NONE;
        	}	
      }

#endif		
				
		/*Process the igmp header if present*/
		ihl = iph.ihl*4;
		
		data_ptr += ihl; 
		
		DEBUGP("\tIP HEADER\n");
		DEBUGP("\t  IP source address : %4x\n",iph.saddr);
		DEBUGP("\t  IP destination address : %4x\n",iph.daddr);
		DEBUGP("\t  IP Protocol : %x\n",iph.protocol);

		/* Pull out additionl 8 bytes to save some space in protocols. */	

		if(iph.protocol == IPPROTO_IGMP ) /*IGMP*/
		{
			DEBUGP("\tIGMP Header\n");
			memcpy(&igmph,(struct igmphdr*)data_ptr,sizeof(struct igmphdr));
			group = igmph.group;
			type = igmph.type;
			DEBUGP("\t  IGMP Type :%x, Group : %4x\n", type, group);
#ifdef CONFIG_BRIDGE_CONF_MODE
			if((br->conf_mode == MULTICAST_BRIDGE_ONLY) && (type == IGMP_MEMBERSHIP_QUERY))	
			{
				in_dev = __in_dev_get(&br->dev);
				if(in_dev == NULL)
				{
					br->statistics.tx_dropped++;
					printk(KERN_ERR "br_mc_parse : Query Pkt : __in_dev_get for bridge device %s unsuccessful\n", br->dev.name);
					return MFD_PARSE_ERROR;
				}
				if(br->isQuerier == IGMP_QUERIER)
				{
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->general_query_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{
						/* Remains a Querier */
					}
				}
				else if (br->isQuerier == IGMP_NON_QUERIER)
				{
						
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State: Remain Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->other_querier_present_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{

						/* Change the bridge state to Querier */
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Querier\n", br->dev.name);
						del_timer(&br->other_querier_present_timer);
						br->isQuerier = IGMP_QUERIER;
						br_send_igmp_query(br,0,DEFAULT_QUERY_RESPONSE_INTERVAL*10);
						timer = &br->general_query_timer;
						init_timer(timer);
					   timer->data =(unsigned long)br;
					   timer->function = br_general_query_timer_expired;
					   timer->expires = jiffies + DEFAULT_QUERY_INTERVAL * HZ;
					   add_timer(timer);			  
					}
				}
			}
#endif
			switch(type)
			{
				case IGMP_MEMBERSHIP_QUERY:
						DEBUGP("Flooding IGMP Query packet\n");
						br_dont_forward_report(br,group,-1);
#ifndef TI_SLOW_PATH
						br_flood(br,skb,0);
#else
						br_flood_deliver(br, skb, 0);
#endif
						break;
				case IGMP_V1_MEMBERSHIP_REPORT:
				case IGMP_V2_MEMBERSHIP_REPORT:
				case IGMP_V3_MEMBERSHIP_REPORT:
				case IGMP_LEAVE_GROUP:
				default:	
#ifndef TI_SLOW_PATH
						br_flood(br, skb, 0);
#else
						br_flood_deliver(br, skb, 0);
#endif
						break;
			}
			
			return 0;		
 		
		}
		else
		{
			/* For Multicast Data packet, forward decision is based on
			 * IP Destination Address which is a Multicast Group Address
			 */

			group = iph.daddr;

			/*Multicast Data packets*/
			DEBUGP("Multicast Data packet\n");
			if((mfd = br_mfd_get(br,group)) != NULL)
			{
			 	pnode = mfd->plist;
				if(pnode != NULL)
				{
					for(; pnode->next != NULL; pnode = pnode->next)
					{
						DEBUGP("Sending multicast data on %s\n", pnode->dst->dev->name);						
						skb2 = skb_clone(skb, GFP_ATOMIC);
						if (skb2) 
						{
							/* br_send_frame flow will take care to free skb2 */
#ifndef TI_SLOW_PATH
							br_send_frame(pnode->dst,skb2); 
#else
							br_deliver(pnode->dst, skb2);
#endif
						}
		      		else
						{
							printk(KERN_ERR "In __br_handle_frame : skb_copy failed for multicast data pkt\n");						
							br->statistics.tx_dropped++;
							return MFD_MEMORY_ERROR;
						}
					}
					DEBUGP("Sending multicast data on %s\n", pnode->dst->dev->name);						
#ifndef TI_SLOW_PATH
					br_send_frame(pnode->dst, skb);
#else
					br_deliver(pnode->dst, skb);	
#endif
				}
			}

			if (skb)
				kfree_skb(skb);	

			return 0;
		}
	}
	else
	{
		DEBUGP("Non IP Multicast\n");
#ifndef TI_SLOW_PATH
		br_flood(br, skb, 0);
#else
		br_deliver(br, skb, 0);	
#endif
		return 0;	
	}

	return MFD_PARSE_ERROR;	
}


__u8 br_input_handle_mc(struct net_bridge *br, struct sk_buff *skb)
{
	int ihl;
	struct ethhdr ethh;
	struct igmphdr igmph;
	struct iphdr iph;
	int i;
	unsigned short protocol;	
	__u8 type;
	__u32 group;
	struct timer_list *timer;
	unsigned char* data_ptr;
	struct in_device *in_dev;
	struct net_bridge_mfd_entry* mfd;
	struct sk_buff *skb2;
	struct port_list* pnode;

	data_ptr = skb->data;
	memcpy(&ethh,(struct ethhdr*)data_ptr,sizeof(struct ethhdr));
	data_ptr += sizeof(struct ethhdr);

	DEBUGP("\tMAC header:\n");
	DEBUGP("\t  MAC Dest Addr :");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_dest[i]);
	}
	DEBUGP("\n\t  MAC Source Addr : ");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_source[i]);
	}
	DEBUGP("\n\t  Protocol Type :%x\n",ethh.h_proto);
	protocol = ethh.h_proto;

	
	/*Determine if the protocol is ip */
	if (ntohs(protocol) == ETH_P_IP)
	{
		memcpy(&iph,(struct iphdr *)data_ptr,sizeof(struct iphdr));
		
#if 0
	   if (ip_fast_csum((u8 *)iph, iph->ihl) != 0)
		{
	      kfree_skb(clone); 
			return MFD_PARSE_ERROR;
		}
                                                                                
    	{
        	__u32 len = ntohs(iph->tot_len);
	      if (clone->len < len || len < (iph->ihl<<2))
			{
        	                kfree_skb(clone);
				return MFD_PARSE_ERROR;
			}
                                                                                
	      /* Our transport medium may have padded the buffer out. Now we know it
   	    * is IP we can trim to the true length of the frame.
        	 * Note this now means skb->len holds ntohs(iph->tot_len).
	       */
         if (clone->len > len) {
            __pskb_trim(clone, len);
           	if (clone->ip_summed == CHECKSUM_HW)
          	clone->ip_summed = CHECKSUM_NONE;
        	}	
      }

#endif		
				
		/*Process the igmp header if present*/
		ihl = iph.ihl*4;
		
		data_ptr += ihl; 
		
		DEBUGP("\tIP HEADER\n");
		DEBUGP("\t  IP source address : %4x\n",iph.saddr);
		DEBUGP("\t  IP destination address : %4x\n",iph.daddr);
		DEBUGP("\t  IP Protocol : %x\n",iph.protocol);

		/* Pull out additionl 8 bytes to save some space in protocols. */	

		if(iph.protocol == IPPROTO_IGMP ) /*IGMP*/
		{
			DEBUGP("\tIGMP Header\n");
			memcpy(&igmph,(struct igmphdr*)data_ptr,sizeof(struct igmphdr));
			group = igmph.group;
			type = igmph.type;
			DEBUGP("\t  IGMP Type :%x, Group : %4x\n", type, group);
#ifdef CONFIG_BRIDGE_CONF_MODE
			if((br->conf_mode == MULTICAST_BRIDGE_ONLY) && (type == IGMP_MEMBERSHIP_QUERY))	
			{
				in_dev = __in_dev_get(&br->dev);
				if(in_dev == NULL)
				{
					printk(KERN_ERR "br_mc_parse : Query Pkt : __in_dev_get for bridge device %s unsuccessful\n", br->dev.name);
					br->statistics.tx_dropped++;
					return MFD_PARSE_ERROR;
				}
				if(br->isQuerier == IGMP_QUERIER)
				{
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->general_query_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{
						/* Remains a Querier */
					}
				}
				else if (br->isQuerier == IGMP_NON_QUERIER)
				{
						
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State: Remain Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->other_querier_present_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{

						/* Change the bridge state to Querier */
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Querier\n", br->dev.name);
						del_timer(&br->other_querier_present_timer);
						br->isQuerier = IGMP_QUERIER;
						br_send_igmp_query(br,0,DEFAULT_QUERY_RESPONSE_INTERVAL*10);
						timer = &br->general_query_timer;
						init_timer(timer);
					   timer->data =(unsigned long)br;
					   timer->function = br_general_query_timer_expired;
					   timer->expires = jiffies + DEFAULT_QUERY_INTERVAL * HZ;
					   add_timer(timer);			  
					}
				}
			}
#endif
			switch(type)
			{
				case IGMP_V1_MEMBERSHIP_REPORT:
				case IGMP_V2_MEMBERSHIP_REPORT:
				case IGMP_V3_MEMBERSHIP_REPORT:
				case IGMP_LEAVE_GROUP:
							DEBUGP("Learning IGMP Membership Report / Leave packet\n");
							br_mc_learn_and_flood(br, skb->dev->br_port, type, &group, skb);
							break;
				case IGMP_MEMBERSHIP_QUERY:
						DEBUGP("Flooding IGMP Query packet\n");
						br_dont_forward_report(br, group, skb->dev->ifindex);
						br_flood(br, skb, 1);
						break;
				default:	
						br_flood(br, skb, 1);
						break;
			}
			
			return 0;		
 		
		}
		else
		{
			/* For Multicast Data packet, forward decision is based on
			 * IP Destination Address which is a Multicast Group Address
			 */

			group = iph.daddr;

			/*Multicast Data packets*/
			DEBUGP("Multicast Data packet\n");
			if((mfd = br_mfd_get(br,group)) != NULL)
			{
					for(pnode = mfd->plist; pnode!= NULL; pnode = pnode->next)
					{
						DEBUGP("Sending multicast data on %s\n", pnode->dst->dev->name);						
						skb2 = skb_clone(skb, GFP_ATOMIC);
						if (skb2) 
						{
							/* br_send_frame flow will take care to free skb2 */
							br_send_frame(pnode->dst,skb2); 
						}
		      		else
						{
							printk(KERN_ERR "In __br_handle_frame : skb_copy failed for multicast data pkt\n");						
							br->statistics.tx_dropped++;
							return MFD_MEMORY_ERROR;
						}
					}
			}	

			return 0;
		}
	
	}
	else
	{
		DEBUGP("Non IP Multicast\n");
		br_flood(br,skb,1);
		return 0;	
	}

	return MFD_PARSE_ERROR;	
}

__u8 br_handle_mc(struct net_bridge *br,struct sk_buff *skb,int mode)
{
	int ihl;
	struct ethhdr ethh;
	struct igmphdr igmph;
	struct iphdr iph;
	int i,ifindex;
	unsigned char* data_ptr;
	struct net_bridge_mfd_entry* mfd;
	struct sk_buff *skb2;
	struct port_list* pnode;
	__u8 type;
	__u32 group;
#ifdef CONFIG_BRIDGE_CONF_MODE
	struct timer_list *timer;
	struct in_device *in_dev;
#endif
	
	data_ptr = skb->data;
	memcpy(&ethh,(struct ethhdr*)data_ptr,sizeof(struct ethhdr));
	data_ptr += sizeof(struct ethhdr);

	DEBUGP("\tMAC header:\n");
	DEBUGP("\t  MAC Dest Addr :");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_dest[i]);
	}
	DEBUGP("\n\t  MAC Source Addr : ");
	for(i = 0; i < 6; i++)
	{	
		DEBUGP("%x ",ethh.h_source[i]);
	}
	DEBUGP("\n\t  Protocol Type :%x\n",ethh.h_proto);
	
	/*Determine if the protocol is ip */
	if (ntohs(ethh.h_proto) == ETH_P_IP)
	{
		memcpy(&iph,(struct iphdr *)data_ptr,sizeof(struct iphdr));
		
#if 0
	   	if (ip_fast_csum((u8 *)iph, iph.ihl) != 0)
			{
				return MFD_PARSE_ERROR;
			}
        	__u32 len = ntohs(iph.tot_len);
	      
			if (skb->len < len || len < (iph.ihl<<2))
			{
				return MFD_PARSE_ERROR;
			}

#endif		
				
		/*Process the igmp header if present*/
		ihl = iph.ihl*4;
		
		data_ptr += ihl; 
		
		DEBUGP("\tIP HEADER\n");
		DEBUGP("\t  IP source address : %4x\n",iph.saddr);
		DEBUGP("\t  IP destination address : %4x\n",iph.daddr);
		DEBUGP("\t  IP Protocol : %x\n",iph.protocol);

		if(iph.protocol == IPPROTO_IGMP ) /*IGMP*/
		{
			DEBUGP("\tIGMP Header\n");
			memcpy(&igmph,(struct igmphdr*)data_ptr,sizeof(struct igmphdr));
			type = igmph.type;
			group = igmph.group;
			DEBUGP("\t  IGMP Type :%x, Group : %4x\n",type, group);
#ifdef CONFIG_BRIDGE_CONF_MODE
			if((br->conf_mode == MULTICAST_BRIDGE_ONLY) && (type == IGMP_MEMBERSHIP_QUERY))	
			{
				in_dev = __in_dev_get(&br->dev);
				if(in_dev == NULL)
				{
					printk(KERN_ERR "br_mc_parse : Query Pkt : __in_dev_get for bridge device %s unsuccessful\n", br->dev.name);
					br->statistics.tx_dropped++;
					return MFD_PARSE_ERROR;
				}
				if(br->isQuerier == IGMP_QUERIER)
				{
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->general_query_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{
						/* Remains a Querier */
					}
				}
				else if (br->isQuerier == IGMP_NON_QUERIER)
				{
						
					if(iph.saddr < in_dev->ifa_list->ifa_address)
					{
						
						DEBUGP(KERN_INFO "Bridge %s Querier State: Remain Non Querier\n", br->dev.name);	
						br->isQuerier = IGMP_NON_QUERIER;
						del_timer(&br->other_querier_present_timer);
						timer = &br->other_querier_present_timer;
						init_timer(timer);
						timer->data =(unsigned long)br;
						timer->function = br_other_querier_present_timer_expired;
						timer->expires = jiffies + DEFAULT_OTHER_QUERIER_PRESENT * HZ;
						add_timer(timer);			
					}
					else
					{
						/* Change the bridge state to Querier */
						DEBUGP(KERN_INFO "Bridge %s Querier State : Switching to Querier\n", br->dev.name);
						del_timer(&br->other_querier_present_timer);
						br->isQuerier = IGMP_QUERIER;
						br_send_igmp_query(br,0,DEFAULT_QUERY_RESPONSE_INTERVAL*10);
						timer = &br->general_query_timer;
						init_timer(timer);
					   timer->data =(unsigned long)br;
					   timer->function = br_general_query_timer_expired;
					   timer->expires = jiffies + DEFAULT_QUERY_INTERVAL * HZ;
					   add_timer(timer);			  
					}
				}
			}
#endif
			switch(type)
			{
				/* Frequency of IGMP control packet is relatively less. In control path, checking for INPUT / FORWARD
				 * is ok when compared to space occupied in having two different functions for input & forward
				 */
				case IGMP_V1_MEMBERSHIP_REPORT:
				case IGMP_V2_MEMBERSHIP_REPORT:
				case IGMP_V3_MEMBERSHIP_REPORT:
				case IGMP_LEAVE_GROUP:
						if(mode == INPUT)
						{
							DEBUGP("Learning IGMP Membership Report / Leave packet\n");
							br_mc_learn_and_flood(br,skb->dev->br_port,type,&group,skb);
							kfree_skb(skb);			
						}
						else
						{
#ifndef TI_SLOW_PATH
							br_flood(br, skb, 0);
#else
							br_flood_deliver(br, skb, 0);
#endif
						}
						break;
				case IGMP_MEMBERSHIP_QUERY:
						DEBUGP("Flooding IGMP Query packet\n");
						ifindex = (mode == INPUT)? skb->dev->ifindex: -1;
						br_dont_forward_report(br,group,ifindex);
#ifndef TI_SLOW_PATH
						br_flood(br,skb,0);
#else
						br_flood_deliver(br, skb, 0);
#endif
						break;
				default:	

#ifndef TI_SLOW_PATH
						br_flood(br,skb, 0);
#else
						br_flood_deliver(br, skb, 0);
#endif
						break;
			}
			return 0;		
 		
		}
		else
		{
			/* For Multicast Data packet, forward decision is based on
			 * IP Destination Address which is a Multicast Group Address
			 */
			group = iph.daddr;
			/*Multicast Data packets*/
			DEBUGP("Multicast Data packet\n");
			if((mfd = br_mfd_get(br,group)) != NULL)
			{
			 	pnode = mfd->plist;
				if(pnode != NULL)
				{
					for(; pnode->next != NULL; pnode = pnode->next)
					{
						DEBUGP("Sending multicast data on %s\n", pnode->dst->dev->name);						
						skb2 = skb_clone(skb, GFP_ATOMIC);
						if (skb2) 
						{
							/* br_send_frame flow will take care to free skb2 */
#ifndef TI_SLOW_PATH
							br_send_frame(pnode->dst, skb2); 
#else
							br_deliver(pnode->dst, skb2);
#endif
						}
		      		else
						{
							printk(KERN_ERR "In __br_handle_frame : skb_copy failed for multicast data pkt\n");						
							br->statistics.tx_dropped++;
							return MFD_MEMORY_ERROR;
						}
					}
					DEBUGP("Sending multicast data on %s\n", pnode->dst->dev->name);						
#ifndef TI_SLOW_PATH
					br_send_frame(pnode->dst, skb);
#else
					br_deliver(pnode->dst, skb);
#endif
					return 0;
				}
			}
	
			DEBUGP("No mfd entry for %x group addr\n",group);
			if (skb)
				kfree_skb(skb);
			return 0;
		}
	}
	else
	{
		DEBUGP("Non IP Multicast\n");
#ifndef TI_SLOW_PATH
		br_flood(br, skb, 0);
#else
		br_deliver(br, skb, 0);	
#endif
		return 0;	
	}

	return MFD_PARSE_ERROR;	
}

void br_dont_forward_report(struct net_bridge *br, __u32 group,int exclude_if)
{
	struct net_bridge_mfd_entry *mfd;
	struct port_list *pnode;
	int i = 0;
		
	DEBUGP("br_dont_forward_report called with group addr: %x\n", group);	

	write_lock_bh(&br->mfd_hash_lock);
	if (group == 0)
	{
		for(i=0; i < BR_HASH_SIZE; i++)
		{
			mfd = br->mfd_hash[i];
			while (mfd != NULL)
			{
				for(pnode = mfd->plist; pnode!=NULL; pnode = pnode->next)
				{
					if(pnode->dst->dev->ifindex != exclude_if)
					{
						pnode->dont_forward_report = 1;
						DEBUGP("Dont forward set on port: %s\n", pnode->dst->dev->name);
					}
				}
				mfd = mfd->next_hash;
			}
		}
	}
	else 
	{
		/* Find mfd for this group */
		mfd = br->mfd_hash[br_addr_hash(&group)];
		while (mfd != NULL)
		{
			if(mfd->addr == group)
			{
				for(pnode = mfd->plist; pnode!=NULL; pnode = pnode->next)
				{
					if(pnode->dst->dev->ifindex != exclude_if)
					{
						pnode->dont_forward_report = 1;
						DEBUGP("Dont forward set on port: %s\n", pnode->dst->dev->name);
					}
				}
				break;
			}
			mfd = mfd->next_hash;
		}
	}
	write_unlock_bh(&br->mfd_hash_lock);
	return;
	
}
	
void br_mc_learn_and_flood(struct net_bridge *br, struct net_bridge_port *source, __u8 type,__u32 *group,struct sk_buff *skb)
{

	DEBUGP("br_mc_learn_and_flood:\n");

	if(type == IGMP_V1_MEMBERSHIP_REPORT)
	{
		/* V1 Membership reports */
		br_mfd_insert(br,source,group,type,skb);
	}
	else if(type == IGMP_V2_MEMBERSHIP_REPORT)
	{
		/* V2 Membership reports */
		br_mfd_insert(br,source,group,type,skb);
	}
	else if(type == IGMP_V3_MEMBERSHIP_REPORT)
	{
		/* V3 Membership reports */
		br_mfd_insert(br,source,group,type,skb);
	}
	else if(type == IGMP_LEAVE_GROUP)
	{
		/* Leave Group */
		br_leave_report_received(br, source, *group);
		br_flood(br, skb, 1);
	}
	return;
}
 

/* #endif for #define CONFIG_BRIDGED_MULTICAST */

/* #endif */

