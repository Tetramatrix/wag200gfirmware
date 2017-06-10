/*
<:copyright-gpl
 Copyright 2002 Broadcom Corp. All Rights Reserved.

 This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
:>
*/
/******************************************************************************
//
//  Filename:       ip_conntrack_proto_esp.c
//  Author:         Pavan Kumar
//  Creation Date:  05/27/04
//
//  Description:
//      Implements the ESP ALG connectiontracking.
//
*****************************************************************************/
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/netfilter_ipv4/ip_conntrack_protocol.h>
#include <linux/netfilter_ipv4/ip_nat_esp.h>

#define ESP_TIMEOUT (30*HZ)
#define ESP_STREAM_TIMEOUT (180*HZ)

#define ESP_UNREPLIEDDNS_TIMEOUT (1*HZ)

#define IPSEC_FREE     0
#define IPSEC_INUSE    1
#define MAX_PORTS      64
#define TEMP_SPI_START 1500

enum{
    Esp_Pkt_To_Tuple_0,
    Esp_New_1,
    Esp_Packet_2,
    Esp_Packet_3
};//use for which_func_call

struct _esp_table {
        u_int32_t l_spi;
        u_int32_t r_spi;
        u_int32_t l_ip;
        u_int32_t r_ip;
        u_int32_t timeout;
        u_int16_t tspi;
        int       inuse;
        u_int32_t seq;
};

static struct _esp_table esp_table[MAX_PORTS];

#if 0
#define DEBUGP(format, args...) printk(__FILE__ ":" __FUNCTION__ ": " \
				       format, ## args)
#else
#define DEBUGP(format, args...)
#endif

static u_int16_t cur_spi = 0;

/*
 * Allocate a free IPSEC table entry.
 */
struct _esp_table *alloc_esp_entry ( void )
{
	int idx = 0;
	struct _esp_table *esp_entry = esp_table;

	for ( ; idx < MAX_PORTS; idx++ ) {
		if ( esp_entry->inuse == IPSEC_FREE ) {
			esp_entry->tspi  = cur_spi = TEMP_SPI_START + idx;
			esp_entry->inuse = IPSEC_INUSE;
			DEBUGP ("%s:%s New esp_entry at %p"
				   " tspi %u cspi %u\n", __FILE__, __FUNCTION__,
				   esp_entry, esp_entry->tspi, cur_spi );
			return esp_entry;
		}
		esp_entry++;
	}
	return NULL;
}

/*
 * Search an ESP table entry by the Security Parameter Identifier (SPI).
 */
struct _esp_table *search_esp_entry_by_spi ( const struct esphdr *esph,
					     u_int32_t daddr, u_int32_t saddr, int which_func_call )
{
	int idx = 0;
	struct _esp_table *esp_entry = esp_table;
//	unsigned int l_ip;
//	unsigned int r_ip;
	unsigned int dest_addr=ntohl(daddr);
	unsigned int src_addr=ntohl(saddr);
	
	
	for ( ; idx < MAX_PORTS; idx++, esp_entry++ ) {
		if ( esp_entry->inuse == IPSEC_FREE ) {
			continue;
		}

//	l_ip=esp_entry->l_ip;
//	r_ip=esp_entry->r_ip;
		
	DEBUGP ("------------------------------------------------------------\n");	
	DEBUGP (" which_func_call =%d  idx=%d  spi=%x   l_spi=%x    r_spi=%x \n"
	        ,which_func_call ,idx, ntohl(esph->spi), esp_entry->l_spi, esp_entry->r_spi);
	DEBUGP (" l_ip=%u.%u.%u.%u   r_ip=%u.%u.%u.%u   daddr=%u.%u.%u.%u  saddr=%u.%u.%u.%u\n"
		   ,NIPQUAD(esp_entry->l_ip), NIPQUAD(esp_entry->r_ip), NIPQUAD(dest_addr), NIPQUAD(src_addr));
	DEBUGP (" esp_entry->seq=%u   esph->seq=%u\n"
		   ,esp_entry->seq, ntohl(esph->seq));
	DEBUGP ("------------------------------------------------------------\n");	
	
		/* If we have seen traffic both ways */
		if ( esp_entry->l_spi != 0 && esp_entry->r_spi != 0 ) {
			if ( esp_entry->l_spi == ntohl(esph->spi) ||
			     esp_entry->r_spi == ntohl(esph->spi) ) {
				return esp_entry;
			}
			continue;
		}
		/* If we have seen traffic only one way */
		if ( esp_entry->l_spi == 0 || esp_entry->r_spi == 0 ) {
			/* We have traffic from local */
			if ( esp_entry->l_spi ) {
				if ( ntohl(esph->spi) == esp_entry->l_spi ) {
					DEBUGP ("%s:%s One Way Traffic From Local Entry %p\n",
						__FILE__, __FUNCTION__, esp_entry);
					return esp_entry;
				}
				if( esp_entry->r_ip == src_addr){
				    if(which_func_call == Esp_Pkt_To_Tuple_0){
				        /* This must be the first packet from remote */
				        esp_entry->r_spi = ntohl(esph->spi);
				        esp_entry->r_ip = ntohl(daddr);
				    }
				    return esp_entry;
			    }
			    continue;
			/* We have seen traffic only from remote */
			} else if ( esp_entry->r_spi ) {
				if ( ntohl(esph->spi) == esp_entry->r_spi ) {
					DEBUGP ("%s:%s One Way Traffic From Remote Entry %p\n",
						__FILE__, __FUNCTION__, esp_entry);
					return esp_entry;
				}
				    if( esp_entry->r_ip == dest_addr){
				    if(which_func_call == Esp_Pkt_To_Tuple_0){
				        /* This must be the first packet from local */
				        esp_entry->l_spi = ntohl(esph->spi);
				    }
				    return esp_entry;
			    }
			    continue;
			}
		}
	}
	return NULL;
}

static int esp_pkt_to_tuple(const void *datah, size_t datalen,
			    struct ip_conntrack_tuple *tuple)
{
	const struct esphdr *esph = datah;
	struct _esp_table *esp_entry;

	if ( (esp_entry = search_esp_entry_by_spi ( esph, tuple->dst.ip, tuple->src.ip, Esp_Pkt_To_Tuple_0 ) ) == NULL ) {
		esp_entry = alloc_esp_entry();
		if ( esp_entry == NULL ) {
			return 0;
		}
		esp_entry->l_spi = ntohl(esph->spi);
		esp_entry->l_ip  = ntohl(tuple->src.ip);
		esp_entry->r_ip = ntohl(tuple->dst.ip);
        esp_entry->seq   = ntohl(esph->seq);
	}
	DEBUGP ("%s:%s tspi %u cspi %u spi 0x%x seq 0x%x"
		   " sip %u.%u.%u.%u dip %u.%u.%u.%u\n", __FILE__,
		   __FUNCTION__, esp_entry->tspi, cur_spi,
		   ntohl(esph->spi), ntohl(esph->seq),
		   NIPQUAD(tuple->src.ip), NIPQUAD(tuple->dst.ip) );
	tuple->dst.u.esp.spi = esp_entry->tspi;
	tuple->src.u.esp.spi = esp_entry->tspi;
	return 1;
}

static int esp_invert_tuple(struct ip_conntrack_tuple *tuple,
			    const struct ip_conntrack_tuple *orig)
{
	DEBUGP ("%s:%s cspi %u dspi %u sspi %u"
		   " sip %u.%u.%u.%u dip %u.%u.%u.%u\n",
		   __FILE__, __FUNCTION__, cur_spi, orig->dst.u.esp.spi,
		   orig->src.u.esp.spi, NIPQUAD(tuple->src.ip),
		   NIPQUAD(tuple->dst.ip) );
	tuple->src.u.esp.spi = orig->dst.u.esp.spi;
	tuple->dst.u.esp.spi = orig->src.u.esp.spi;
	return 1;
}

/* Print out the per-protocol part of the tuple. */
static unsigned int esp_print_tuple(char *buffer,
				    const struct ip_conntrack_tuple *tuple)
{
	return sprintf(buffer, "sport=%u dport=%u ",
		       ntohs(tuple->src.u.esp.spi), ntohs(tuple->dst.u.esp.spi));
}

/* Print out the private part of the conntrack. */
static unsigned int esp_print_conntrack(char *buffer,
					const struct ip_conntrack *conntrack)
{
	return 0;
}

/* Returns verdict for packet, and may modify conntracktype */
static int esp_packet(struct ip_conntrack *conntrack,
		      struct iphdr *iph, size_t len,
		      enum ip_conntrack_info conntrackinfo)
{
	const struct esphdr *esph = (void *)iph + iph->ihl*4;
	struct _esp_table *esp_entry;

	DEBUGP ("%s:%s ctinfo %d status %d spi 0x%x seq 0x%x\n",
		   __FILE__, __FUNCTION__, conntrackinfo, conntrack->status,
		   (unsigned long)(ntohl(esph->spi)), (unsigned long)(ntohl(esph->seq) ));
	/*
	 * This should not happen. We get into this routine only if there is
	 * an existing stream.
	 */
	if (conntrackinfo == IP_CT_NEW ) {
		if ( (esp_entry = search_esp_entry_by_spi ( esph,iph->daddr, iph->saddr, Esp_Packet_2 ) ) == NULL ) {
			esp_entry = alloc_esp_entry ();
                        if ( esp_entry == NULL ) {
                               	/* All entries are currently in use */
                               	return NF_DROP;
                        }
                        esp_entry->l_spi = ntohl(esph->spi);
                        esp_entry->l_ip  = ntohl(iph->saddr);
						esp_entry->seq   = ntohl(esph->seq);
                        esp_entry->r_spi = 0;
		}
	}
	/* If we've seen traffic both ways, this is some kind of UDP
	   stream.  Extend timeout. */
	 if (conntrack->status & IPS_SEEN_REPLY) {
		ip_ct_refresh(conntrack, ESP_STREAM_TIMEOUT);
		/* Also, more likely to be important, and not a probe */
		set_bit(IPS_ASSURED_BIT, &conntrack->status);
	} else {
		ip_ct_refresh(conntrack, ESP_TIMEOUT);
    	}
	esp_entry = search_esp_entry_by_spi ( esph, iph->daddr, iph->saddr, Esp_Packet_3 );
	if ( esp_entry != NULL ) {
		DEBUGP ("%s:%s can modify this %u.%u.%u.%u"
			   " with %u.%u.%u.%u\n",
			   __FILE__, __FUNCTION__,
		 	   NIPQUAD(conntrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip),
			   NIPQUAD(esp_entry->l_ip) );
	}

	return NF_ACCEPT;
}

/* Called when a new connection for this protocol found. */
static int esp_new(struct ip_conntrack *conntrack,
			     struct iphdr *iph, size_t len)
{
	const struct esphdr *esph = (void *)iph + iph->ihl*4;
	struct _esp_table *esp_entry;
	DEBUGP ("%s:%s spi 0x%x seq 0x%x sip %u.%u.%u.%u"
		   " daddr %u.%u.%u.%u\n", __FILE__, __FUNCTION__,
		   ntohl(esph->spi), ntohl(esph->seq), NIPQUAD(iph->saddr),
		   NIPQUAD(iph->daddr) );
	if ( (esp_entry = search_esp_entry_by_spi ( esph, iph->daddr, iph->saddr, Esp_New_1) ) == NULL ) {
		/*
		 * Check if this is the same LAN client creating another session.
		 * If this is true, then the LAN IP address will be the same with
		 * a new SPI value. This would indicate that the entire transaction
		 * using the previous value of SPI is now not required.
		 */
		esp_entry = alloc_esp_entry ();
		if ( esp_entry == NULL ) {
			/* All entries are currently in use */
			return NF_DROP;
		}
		esp_entry->l_spi = ntohl(esph->spi);
		esp_entry->l_ip  = ntohl(iph->saddr);
		esp_entry->seq   = ntohl(esph->seq);
		esp_entry->r_spi = 0;
	}
	return 1;
}

struct ip_conntrack_protocol ip_conntrack_protocol_esp
= { { NULL, NULL }, IPPROTO_ESP, "esp",
    esp_pkt_to_tuple, esp_invert_tuple, esp_print_tuple, esp_print_conntrack,
    esp_packet, esp_new, NULL };
