/*
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: if_bridge.h,v 1.1.1.1 2006-01-16 08:12:45 jeff Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifndef _LINUX_IF_BRIDGE_H
#define _LINUX_IF_BRIDGE_H

#include <linux/types.h>
#include <linux/config.h>

#define CONFIG_BRIDGED_MULTICAST

#ifdef CONFIG_BRIDGED_MULTICAST
/* #define CONFIG_BRIDGE_CONF_MODE */
#define MAX_MCGRP_MEMBERS 256
#endif

#define BRCTL_VERSION 1

#define BRCTL_GET_VERSION 0
#define BRCTL_GET_BRIDGES 1
#define BRCTL_ADD_BRIDGE 2
#define BRCTL_DEL_BRIDGE 3
#define BRCTL_ADD_IF 4
#define BRCTL_DEL_IF 5
#define BRCTL_GET_BRIDGE_INFO 6
#define BRCTL_GET_PORT_LIST 7
#define BRCTL_SET_BRIDGE_FORWARD_DELAY 8
#define BRCTL_SET_BRIDGE_HELLO_TIME 9
#define BRCTL_SET_BRIDGE_MAX_AGE 10
#define BRCTL_SET_AGEING_TIME 11
#define BRCTL_SET_GC_INTERVAL 12
#define BRCTL_GET_PORT_INFO 13
#define BRCTL_SET_BRIDGE_STP_STATE 14
#define BRCTL_SET_BRIDGE_PRIORITY 15
#define BRCTL_SET_PORT_PRIORITY 16
#define BRCTL_SET_PATH_COST 17
#define BRCTL_GET_FDB_ENTRIES 18
#define BRCTL_SET_FILTER_ENTRY 19
#define BRCTL_FLUSH_FILTER_ENTERIES 20
#define BRCTL_SHOW_FILTER_ENTERIES 21
#define BRCTL_SET_FILTER_STATE 22
#define BRCTL_DELETE_FILTER_ENTRY 23

/* frank,040202 */
#ifdef ASUS_IGMP_SNOOPING
#define BRCTL_SET_BRIDGE_IGMPSNOOP_STATE 51
#define BRCTL_SHOW_BRIDGE_IGMPSNOOP 52
#endif
/* frank,040503 */
#ifdef ASUS_MARVELL_VLAN
#define VLAN_READ_REG 0
#define VLAN_WRITE_REG 1
#define VLAN_GET_TRAILER 2
#define VLAN_SET_TRAILER 3
#define VLAN_GET_GROUP 4
#define VLAN_SET_GROUP 5
#endif

#define BR_STATE_DISABLED 0
#define BR_STATE_LISTENING 1
#define BR_STATE_LEARNING 2

#ifdef TI_SLOW_PATH
#define BR_STATE_FORWARDING 3
#define BR_STATE_BLOCKING 4
#else
#define BR_STATE_FORWARDING     4
#define BR_STATE_BLOCKING       8
#endif

#define BRCTL_DELETE_MFD_V1_PORTS 24

#ifdef CONFIG_BRIDGED_MULTICAST
#define BRCTL_MFD_DISPLAY 25
#define BRCTL_GET_MC_STATUS 26

#ifdef CONFIG_BRIDGE_CONF_MODE
#define BRCTL_ENABLE_PROXY_MODE 27
#define BRCTL_DISABLE_PROXY_MODE 28
#endif
#endif

#define BR_STATE_DISABLED       0
#define BR_STATE_LISTENING      1
#define BR_STATE_LEARNING       2

#ifdef TI_SLOW_PATH
#define BR_STATE_FORWARDING     3
#define BR_STATE_BLOCKING       4
#else
#define BR_STATE_FORWARDING     4
#define BR_STATE_BLOCKING       8
#endif

struct __bridge_info
{
	__u64 designated_root;
	__u64 bridge_id;
	__u32 root_path_cost;
	__u32 max_age;
	__u32 hello_time;
	__u32 forward_delay;
	__u32 bridge_max_age;
	__u32 bridge_hello_time;
	__u32 bridge_forward_delay;
	__u8 topology_change;
	__u8 topology_change_detected;
	__u8 root_port;
	__u8 stp_enabled;
	__u32 ageing_time;
	__u32 gc_interval;
	__u32 hello_timer_value;
	__u32 tcn_timer_value;
	__u32 topology_change_timer_value;
	__u32 gc_timer_value;
	__u8 br_filter_active;
};

struct __port_info
{
	__u64 designated_root;
	__u64 designated_bridge;
	__u16 port_id;
	__u16 designated_port;
	__u32 path_cost;
	__u32 designated_cost;
	__u8 state;
	__u8 top_change_ack;
	__u8 config_pending;
	__u8 unused0;
	__u32 message_age_timer_value;
	__u32 forward_delay_timer_value;
	__u32 hold_timer_value;
};

struct __fdb_entry
{
	__u8 mac_addr[6];
	__u8 port_no;
	__u8 is_local;
	__u32 ageing_timer_value;
	__u32 unused;
};

/* frank,040202 */
#ifdef ASUS_IGMP_SNOOPING
struct __mdb_entry
{
	__u32 ip_addr;
	__u32 ageing_timer_value;
	__u32 port_no;
};
#endif

#ifdef CONFIG_BRIDGED_MULTICAST
/* TXTL-IGMP01 06 Dec 2004 
__mc_status: structure to indicate to the user the multicast states of the bridge
__mfd_entry: structure to indicate to the user the multicast learnig at the bridge
*/

struct __mc_status
{
	__u8 mc_forwarding;
#ifdef CONFIG_BRIDGE_CONF_MODE
	__u8 conf_mode;
	__u8 isQuerier;
#endif
	
};

struct __mfd_entry
{
	__u32 grp_addr;
	__u8 no_ports;
	__u8 port_list[MAX_MCGRP_MEMBERS];
	__u8 vinfo_list[MAX_MCGRP_MEMBERS];	
};

#endif
struct __br_filter_entry
{
        char sport[10];
        char dport[10];
	__u8 src_mac_addr[6];
	__u8 dst_mac_addr[6];
	__u16 proto;
	__u8 access_type;
	__u32 frames_matched;
};

#ifdef __KERNEL__

#include <linux/netdevice.h>

struct net_bridge;
struct net_bridge_port;

extern int (*br_ioctl_hook)(unsigned long arg);
extern void (*br_handle_frame_hook)(struct sk_buff *skb);

#endif

#endif
