/*
 *	Generic parts
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br.c,v 1.1.1.1 2006-01-16 08:12:14 jeff Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/if_bridge.h>
#include <linux/inetdevice.h>
#include <asm/uaccess.h>
#include "br_private.h"

#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
#include "../atm/lec.h"
#endif

#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>

#define DEV_NAME_LENGTH 10
#define IPADDR_LENGTH 15

#define MAX_VPN_POLICY	8
#define BRTRIGGER_WRITE_LENGTH	120

typedef struct vpn_trigger_s {
	unsigned char on;
	__u32   ss;
	__u32   se;
	__u32   ds;
	__u32   de;
	char name[13];
} vpn_trigger_t;

static vpn_trigger_t  vpn_trigger[MAX_VPN_POLICY];
static int vpn_total_triggers = 0;
//static unsigned long vpn_server_itself = 0;
static struct proc_dir_entry *br_trigger = NULL;
//DECLARE_MUTEX(vpn_write_sema);
DECLARE_MUTEX_LOCKED(vpn_read_sema);

static int br_trigger_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data){
	char *p = page;
	int len;
	int i;

	down_interruptible(&vpn_read_sema);
	for(i=0;i<vpn_total_triggers;i++)
		if(vpn_trigger[i].on==2)
			break;
	p += sprintf(p, "%s", vpn_trigger[i].name);
	//vpn_trigger[i].on=0;
	//up(&vpn_write_sema);

	len = (p - page) - off; 
	if (len < 0) 
		len = 0; 
	*eof = (len <= count) ? 1 : 0; 
	*start = page + off; 
	return len;
}

static int br_trigger_write_proc(struct file *file, const char *buffer, unsigned long count, void *data){
	int len;
	char value[BRTRIGGER_WRITE_LENGTH+1];
	struct br_filter *new_filter, *last_filter;
	vpn_trigger_t new;
	int i;
	int offset=0;
	
	len = count > BRTRIGGER_WRITE_LENGTH ? BRTRIGGER_WRITE_LENGTH : count;
	if ( copy_from_user(value, buffer, len) ) 
		return -EFAULT;
	value[len] = '\0';

	if(len<2) {
		vpn_total_triggers=0;
		memset(vpn_trigger, 0, sizeof(vpn_trigger_t)*MAX_VPN_POLICY);
	} else if(len<16){

		if(value[0]=='\1')  /* IPsec SA establish */
			offset=1;
#if 0
		else if(value[0]=='\2') { /* setup vpn_server_itself */
			sscanf(value+1,"%u", &vpn_server_itself);
			return len;
		} else
#endif
		else
			offset=0;

		for(i=0;i<vpn_total_triggers;i++) {
			if(memcmp(vpn_trigger[i].name, value+offset, len-1-offset)==0){
				if(offset) /* IPsec SA establish */
					vpn_trigger[i].on=0;
				else
					vpn_trigger[i].on=1;
			}
		}
				
	} else {
		sscanf(value,"%u %u %u %u %s", &new.ss, &new.se, &new.ds, &new.de, new.name);
		new.on=1;
		memcpy(&vpn_trigger[vpn_total_triggers++], &new, sizeof(vpn_trigger_t));
	}
	return len;
}

/* 2005.03.31 add for Any subnet */
static unsigned long my_br_ifa_address=0;

void set_br_ifa_address(unsigned long addr) {
	my_br_ifa_address=addr;
}

int br_trigger_hook(struct sk_buff *skb) {
	int i;
	unsigned long saddr, daddr;
	struct in_device *in_dev;
#if 1   //defined (CONFIG_MIPS_AVALANCHE_FAST_BRIDGE)
	/* todo: Endien Issue ? */
        daddr = (skb->data[16]<<24) | (skb->data[17]<<16) | (skb->data[18]<<8) | (skb->data[19]);
	if(daddr==my_br_ifa_address)
		return 0;
        saddr = (skb->data[12]<<24) | (skb->data[13]<<16) | (skb->data[14]<<8) | (skb->data[15]);
#else 	/* CONFIG_MIPS_AVALANCHE_FAST_BRIDGE */
        daddr=ntohl(skb->nh.iph->daddr);
	if(daddr==my_br_ifa_address)
		return 0;
        saddr=ntohl(skb->nh.iph->saddr);
#endif 	/* CONFIG_MIPS_AVALANCHE_FAST_BRIDGE */

	for(i=0;i<vpn_total_triggers;i++){
		if( (vpn_trigger[i].on!=0) &&
		    (saddr <= vpn_trigger[i].se) &&
		    (saddr >= vpn_trigger[i].ss) &&
		    (daddr <= vpn_trigger[i].de) &&
		    (daddr >= vpn_trigger[i].ds)){
			if(vpn_trigger[i].on==1) {
				vpn_trigger[i].on=2;
				up(&vpn_read_sema);
				return 1;
			} else if(vpn_trigger[i].on==2){
				return 1;
			} 
		}
	}
	return 0;
}
/********************************************************/


struct br_filter {
	char dev_name[DEV_NAME_LENGTH+1];
	//char valid_ip[IPADDR_LENGTH+1];
	__u32   saddr;
	struct net_device  *dev; 
	struct br_filter *next;
	struct br_filter *prev;
};

static struct br_filter *br_filter_root=NULL;
static struct proc_dir_entry *br_filter = NULL;

int br_filter_pass(struct net_bridge_port *p, struct sk_buff *skb){
	struct br_filter *fp;
	unsigned char ip[4];

	if(p==NULL || skb==NULL || br_filter_root==NULL || (skb->protocol!=htons(ETH_P_IP)))
		return 1;
	fp=br_filter_root;
	do {
		if((fp->dev==p->dev)||(strcmp(p->dev->name, fp->dev_name)==0)){
			fp->dev=p->dev;
#ifdef BRFILTER_DEBUG
		        ip[0]=fp->saddr>>24;
		        ip[1]=(fp->saddr&0x00ffffff)>>16; 
			ip[2]=(fp->saddr&0x0000ffff)>>8; 
			ip[3]=(fp->saddr&0x000000ff); 
			printk("fp->saddr=%hd.%hd.%hd.%hd ", ip[0],ip[1],ip[2],ip[3]);
		        ip[0]=skb->nh.iph->saddr>>24;
		        ip[1]=(skb->nh.iph->saddr&0x00ffffff)>>16; 
			ip[2]=(skb->nh.iph->saddr&0x0000ffff)>>8; 
			ip[3]=(skb->nh.iph->saddr&0x000000ff); 
			printk("skb->nh.iph->saddr=%hd.%hd.%hd.%hd\n", ip[0],ip[1],ip[2],ip[3]);
#endif
			if(fp->saddr==skb->nh.iph->saddr)
				return 1;
			else
				return 0;
		}
		fp=fp->next;
	} while(fp!=NULL);
	return 1;
}

static int br_filter_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data){
	char *p = page;
	int len;
	struct br_filter *filter;
	int i;
	
	filter=br_filter_root;
	if(filter!=NULL) {
		do {
			p += sprintf(p, "br_filter:%s %x\n", filter->dev_name, filter->saddr); 
			filter=filter->next;
		} while(filter!=NULL);
	}

	p += sprintf(p, "\n-- BR_TRIGGER_ENTRIES --\n");
	for(i=0;i<MAX_VPN_POLICY;i++){
		p += sprintf(p, "vpn_trigger[%d]: %d %x %x %x %x %s\n", \
			     i, vpn_trigger[i].on, vpn_trigger[i].ss, \
			     vpn_trigger[i].se, vpn_trigger[i].ds, \
			     vpn_trigger[i].de, vpn_trigger[i].name);
	}

	len = (p - page) - off; 
	if (len < 0) 
		len = 0; 
	*eof = (len <= count) ? 1 : 0; 
	*start = page + off; 
	return len;
}

#define BRFILTER_WRITE_LENGTH	120
#define BRFILTER_CMD_ADD	'1'
#define BRFILTER_CMD_RESET	'2'
static int br_filter_write_proc(struct file *file, const char *buffer, unsigned long count, void *data){
	int len;
	char value[BRFILTER_WRITE_LENGTH+1];
	int cmd;
	struct br_filter *new_filter, *last_filter;
	
	len = count > BRFILTER_WRITE_LENGTH ? BRFILTER_WRITE_LENGTH : count;
	if ( copy_from_user(value, buffer, len) ) 
		return -EFAULT;
	value[len] = '\0';
	/* find last_filter */
	last_filter=br_filter_root;
	if(last_filter!=NULL)
		while(last_filter->next!=NULL)
			last_filter=last_filter->next;
	switch (value[0]) {
		case BRFILTER_CMD_ADD:
			new_filter=kmalloc(sizeof(struct br_filter), GFP_KERNEL);
			memset(new_filter, 0, sizeof(struct br_filter));
			sscanf(value, "1:%u:%s", &new_filter->saddr, new_filter->dev_name);
#ifdef BRFILTER_DEBUG
			printk("[%s][%x]\n", new_filter->dev_name, new_filter->saddr);
#endif
			if(last_filter==NULL)
				br_filter_root=new_filter;
			else {
				new_filter->prev=last_filter;
				last_filter->next=new_filter;
			}
			break;
		case BRFILTER_CMD_RESET:
			if(last_filter!=NULL)
				do{
					new_filter=last_filter->prev;
					kfree(last_filter);
					last_filter=new_filter;
				} while(last_filter!=NULL);
				br_filter_root=NULL;
			break;
		default:
			printk("Unknown Command!\n");
	}
#ifdef BRFILTER_DEBUG
	printk("br_filter_write_proc:%s\n", value);
#endif
	return len;
}

void br_dec_use_count()
{
	MOD_DEC_USE_COUNT;
}

void br_inc_use_count()
{
	MOD_INC_USE_COUNT;
}

static int __init br_init(void)
{
	printk(KERN_INFO "NET4: Ethernet Bridge 008 for NET4.0\n");

#ifdef CONFIG_BRIDGE_NF
	if (br_netfilter_init())
		return 1;
#endif

#ifdef CONFIG_BRIDGE_NF
	if (br_netfilter_init())
		return 1;
#endif

	br_handle_frame_hook = br_handle_frame;
#ifdef CONFIG_INET
	br_ioctl_hook = br_ioctl_deviceless_stub;
#endif
#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
	br_fdb_get_hook = br_fdb_get;
	br_fdb_put_hook = br_fdb_put;
#endif
	register_netdevice_notifier(&br_device_notifier);

	br_filter = create_proc_entry("br_filter", 0600, &proc_root);
	br_filter->read_proc = br_filter_read_proc;
	br_filter->write_proc = br_filter_write_proc;

	memset(vpn_trigger, 0, sizeof(vpn_trigger_t)*MAX_VPN_POLICY);
	br_trigger = create_proc_entry("br_trigger", 0600, &proc_root);
	br_trigger->read_proc = br_trigger_read_proc;
	br_trigger->write_proc = br_trigger_write_proc;

	return 0;
}

static void __br_clear_frame_hook(void)
{
	br_handle_frame_hook = NULL;
}

static void __br_clear_ioctl_hook(void)
{
#ifdef CONFIG_INET
	br_ioctl_hook = NULL;
#endif	
}

static void __exit br_deinit(void)
{
#ifdef CONFIG_BRIDGE_NF
	br_netfilter_fini();
#endif
	unregister_netdevice_notifier(&br_device_notifier);
	br_call_ioctl_atomic(__br_clear_ioctl_hook);
	net_call_rx_atomic(__br_clear_frame_hook);
#if defined(CONFIG_ATM_LANE) || defined(CONFIG_ATM_LANE_MODULE)
	br_fdb_get_hook = NULL;
	br_fdb_put_hook = NULL;
#endif
	remove_proc_entry("br_filter", &proc_root);
}

EXPORT_NO_SYMBOLS;

module_init(br_init)
module_exit(br_deinit)
MODULE_LICENSE("GPL");
