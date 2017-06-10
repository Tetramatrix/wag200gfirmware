/*TXTL_IGMP01 20/10/2004 bridge multicast list*/
#ifndef BR_MFD_H
#define BR_MFD_H

#ifdef CONFIG_BRIDGED_MULTICAST

#define MULTICAST_FORWARDING_DISABLED 0
#define MULTICAST_FORWARDING_ENABLED 1

#define IGMP_MEMBERSHIP_QUERY           0x11
#define IGMP_V1_MEMBERSHIP_REPORT       0x12
#define IGMP_V2_MEMBERSHIP_REPORT       0x16
#define IGMP_V3_MEMBERSHIP_REPORT       0x22
#define IGMP_LEAVE_GROUP                0x17

#define IGMP_MULTICAST_DATA		1
#define NON_IP_MULTICAST		-1
#define NON_IGMP_MULTICAST		-2	
#define MFD_PARSE_ERROR			-3	
#define MFD_MEMORY_ERROR		-4

#define DEFAULT_QUERY_INTERVAL 		125
#define DEFAULT_QUERY_RESPONSE_INTERVAL 10
#define DEFAULT_SPECIFIC_QUERY_RESPONSE_INTERVAL 1
#define DEFAULT_GROUP_MEMBERSHIP_TIMER ((2*125)+(DEFAULT_QUERY_RESPONSE_INTERVAL)*10)
#define DEFAULT_OTHER_QUERIER_PRESENT	((2*125)+(DEFAULT_QUERY_RESPONSE_INTERVAL * 10) / 2)
#define DEFAULT_LEAVE_TIMER	3

#define MULTICAST_BRIDGE_ONLY                0
#define MULTICAST_BRIDGE_WITH_IGMP_PROXY     1

#define IGMP_QUERIER_NA			-1
#define IGMP_QUERIER				1
#define IGMP_NON_QUERIER			0


struct port_list
{
	struct port_list 	*next;
	struct net_bridge_mfd_entry *mfd;
	struct net_bridge_port	*dst;
	unsigned short		vinfo:1; /* 0 or 1*/

	unsigned short		dont_forward_report:1; /* 0 or 1*/

	struct timer_list       mcgrp_ageing_timer;
};

struct net_bridge_mfd_entry
{
	struct net_bridge_mfd_entry 	*next_hash;
	struct netbridge_mfd_entry 	**pprev_hash;
	atomic_t			port_count;
	__u32				addr;
	struct port_list		*plist;		
};


extern void br_mfd_init(struct net_bridge *br);
extern void br_mfd_close(struct net_bridge *br);

extern void br_mfd_delete_port(struct net_bridge *br, struct net_bridge_port *p);
extern struct net_bridge_mfd_entry *br_mfd_get(struct net_bridge *br,__u32  addr);
extern void br_mfd_insert(struct net_bridge *br,struct net_bridge_port *source,__u32 *addr,__u8 type,struct sk_buff *skb);
extern __u8 br_mc_parse(struct net_bridge *br,struct sk_buff *skb,int mode);
extern void br_dont_forward_report(struct net_bridge *br, __u32 group,int exclude_if);
extern void br_mc_learn_and_flood(struct net_bridge *br, struct net_bridge_port *source, __u8 type,__u32 *group,struct sk_buff *skb);

extern int br_mfd_get_entries(struct net_bridge *br,unsigned char *_buf);
extern int br_get_mc_status(struct net_bridge *br, unsigned char *_buf);
extern int br_mfd_delete_v1_ports(struct net_bridge *br);
extern int br_enable_proxy_mode(struct net_bridge *br);
extern int br_disable_proxy_mode(struct net_bridge *br);
#endif /*CONFIG_BRIDGE_MULTICAST*/

//#endif
#endif


