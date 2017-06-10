/**************************************************************************
 * FILE PURPOSE	:  	WAN Bridge Structures
 **************************************************************************
 * FILE NAME	:   wb.h
 *
 * DESCRIPTION	:
 * 	Stuctures and definations used by the WAN Bridge and WAN Devices.
 *
 *	(C) Copyright 2003, Texas Instruments, Inc.
 *************************************************************************/

#ifndef __CPMAC_WAN_H__
#define __CPMAC_WAN_H__

#include <linux/netdevice.h>

/******************************* DEFINITIONS *****************************/

/* Log Levels for various messages in the system. */
#define WAN_LOG_DEBUG                10
#define WAN_LOG_FATAL                1

/******************************* EXTERN FUNCTION *************************/
extern int  cpmac_dev_tx( struct sk_buff *skb, struct net_device *p_dev);

/******************************* LOCAL DECLARATIONS **********************/
typedef struct
{
    void                         *owner;
    unsigned int                 link_speed;
    unsigned int                 link_mode;
    unsigned int                 loop_back;
    /*CPMAC_DEVICE_MIB_T           *device_mib;
    CPMAC_DRV_STATS_T            *stats;*/
    unsigned int                 flags;
    char                         mac_addr[6];
    struct net_device_stats      net_dev_stats; 
    void*                        led_handle;
} CPMAC_WAN_PRIVATE_INFO_T;

extern struct net_device *global_wan_dev_array[1];
extern unsigned long	 my_skb_alloc;

#endif  /* __CPMAC_WAN_H__ */
