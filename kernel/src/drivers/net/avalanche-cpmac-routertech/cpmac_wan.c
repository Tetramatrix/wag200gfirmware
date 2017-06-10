/**************************************************************************
 * FILE PURPOSE	:  	Marvell WAN Port Implementation.
 **************************************************************************
 * FILE NAME	:   cpmac_wan.c
 *
 * DESCRIPTION	:
 *  Creats a WAN device.
 *
 *	CALL-INs:
 *
 *	CALL-OUTs:
 *
 *	User-Configurable Items:
 *
 *	(C) Copyright 2004, Texas Instruments, Inc.
 *************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/init.h>
#include <asm/mips-boards/prom.h>
#include <linux/proc_fs.h>
#include "cpmac_wan.h"

#include <asm/uaccess.h>

MODULE_AUTHOR("Maintainer: Jo");
MODULE_DESCRIPTION("CPMAC WAN Driver");

int wanDbgLevel = WAN_LOG_FATAL;
unsigned long	  my_skb_alloc = 0;
struct proc_dir_entry *wan_config = NULL;
struct proc_dir_entry *wan_stats = NULL;
struct net_device *global_wan_dev_array[1];

static int ewan_read_stats(char* buf, char **start, off_t offset, int count,int *eof, void *data);
static int ewan_write_stats (struct file *fp, const char * buf, unsigned long count, void * data);
static int wan_config_read(char* buf,char **start,off_t offset,int count,int *eof,void *data);
static int wan_config_write(struct file *file, const char* buffer,unsigned long count,void *data);
static int wan_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *wan_get_stats(struct net_device *dev);
static void str2eaddr(unsigned char *ea, unsigned char *str);
static int	wanlogMsg (int level,char *fmt,int arg1,int arg2,int arg3,int arg4,int arg5);

/**************************************************************************
 * FUNCTION NAME : str2hexnum
 **************************************************************************
 * DESCRIPTION   :
 *
 * RETURNS		 :
***************************************************************************/
static unsigned char str2hexnum(unsigned char c)
{
	if(c >= '0' && c <= '9')
    	return c - '0';
	if(c >= 'a' && c <= 'f')
    	return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
    	return c - 'A' + 10;
	return 0;
}

/**************************************************************************
 * FUNCTION NAME : str2eaddr
 **************************************************************************
 * DESCRIPTION   :
 *
 * RETURNS		 :
***************************************************************************/
static void str2eaddr(unsigned char *ea, unsigned char *str)
{
    int i;
    unsigned char num;
    for(i = 0; i < 6; i++) 
    {
        if((*str == '.') || (*str == ':'))
            str++;
        num = str2hexnum(*str++) << 4;
        num |= (str2hexnum(*str++));
        ea[i] = num;
    }
}

/**************************************************************************
 * FUNCTION NAME : wanlogMsg
 **************************************************************************
 * DESCRIPTION   :
 * 	Logs messages from the WAN module. 
 ***************************************************************************/
static int	wanlogMsg (int level,char *fmt,int arg1,int arg2,int arg3,int arg4,int arg5)
{
	/* Check if the message needs to be logged ? */	
	if (level > wanDbgLevel)
		return 0;

	/* YES. The Message DEBUG level is greater than the module debug level. */
	printk (fmt, arg1, arg2, arg3, arg4, arg5, 0);

	return 0;
}

/**************************************************************************
 * FUNCTION NAME : ewan_read_stats
 **************************************************************************
 * DESCRIPTION   :
 *   Read the ewan0 port statistics
 ***************************************************************************/
static int ewan_read_stats(char* buf, char **start, off_t offset, int count,
                              int *eof, void *data)
{
	struct net_device 			*p_dev = global_wan_dev_array[0];
	CPMAC_WAN_PRIVATE_INFO_T 	*ewan_priv;
    int                			len   = 0;
    int                limit = count - 80;


    if(!p_dev)
    	goto proc_error;

	ewan_priv = p_dev->priv;

    /* Transmit stats */
    if(len<=limit)
	    len+= sprintf(buf+len, "\nWAN Statistics\n");
    if(len<=limit)
    	len+= sprintf(buf+len, " Transmit Stats\n");
	if(len<=limit)
    	len+= sprintf(buf+len, "   Tx Valid Bytes Sent        :%lu\n", ewan_priv->net_dev_stats.tx_bytes);
    if(len<=limit)
        len+= sprintf(buf+len, "   Good Tx Frames             :%lu\n", ewan_priv->net_dev_stats.tx_packets);
    if(len<=limit)
        len+= sprintf(buf+len, "   Tx Error Frames            :%lu\n", ewan_priv->net_dev_stats.tx_errors);
    if(len<=limit)
        len+= sprintf(buf+len, "   Tx Dropped Frames          :%lu\n", ewan_priv->net_dev_stats.tx_dropped);
    if(len<=limit)
        len+= sprintf(buf+len, "\n");


     /* Receive Stats */
	if(len<=limit)
    	len+= sprintf(buf+len, " Receive Stats\n");
    if(len<=limit)
    	len+= sprintf(buf+len, "   Rx Valid Bytes Received    :%lu\n", ewan_priv->net_dev_stats.rx_bytes);
	if(len<=limit)
    	len+= sprintf(buf+len, "   Good Rx Frames             :%lu\n", ewan_priv->net_dev_stats.rx_packets);
	if(len<=limit)
    	len+= sprintf(buf+len, "   Rx Errors                  :%lu\n", ewan_priv->net_dev_stats.rx_errors);
    return len;

 proc_error:
    *eof=1;
    return len;
}

/**************************************************************************
 * FUNCTION NAME : ewan_write_stats
 **************************************************************************
 * DESCRIPTION   :
 *   Clear the ewan0 port statistics
 ***************************************************************************/
static int ewan_write_stats (struct file *fp, const char * buf, unsigned long count, void * data)
{
	struct net_device 			*p_dev = global_wan_dev_array[0];
	char local_buf[31];
    int  ret_val = 0;
	CPMAC_WAN_PRIVATE_INFO_T 	*ewan_priv;

	ewan_priv = p_dev->priv;

	if(count > 30)
	{
		printk("Error : Buffer Overflow\n");
		printk("Use echo 0 > /proc/avalanche/ewan_stats to reset the statistics\n");
		return -EFAULT;
	}

	copy_from_user(local_buf,buf,count);
	local_buf[count-1]='\0'; /* Ignoring last \n char */
        ret_val = count;
	
	if(strcmp("0",local_buf)==0)
	{
    
            /* Valid command */
	    	printk("Resetting statistics for ewan0 interface.\n");

		    memset(&ewan_priv->net_dev_stats, 0, sizeof(struct net_device_stats));

	}
	else
	{
		printk("Error: Unknown operation on ewan0 statistics\n");
		printk("Use echo 0 > /proc/avalanche/ewan_stats to reset the statistics\n");
		return -EFAULT;
	}
	
	return ret_val;
}


/**************************************************************************
 * FUNCTION NAME : wan_config_read
 **************************************************************************
 * DESCRIPTION   :
 *   Read the wan_config proc file
 ***************************************************************************/
static int wan_config_read(char* buf, char **start, off_t offset, 
		                   int count, int *eof, void *data) 
{
	int 						len;

	len = sprintf(buf, "My skb Alloc count :%lu\n", my_skb_alloc);
	return len;	
}

static int wan_config_write(struct file *file, const char* buffer, unsigned long count, void *data)
{
    wanlogMsg (WAN_LOG_DEBUG, "Write proc.\n",0,0,0,0,0);
	return 0;	
}

/**************************************************************************
 * FUNCTION NAME : wan_dev_open
 **************************************************************************
 * DESCRIPTION   :
 *   Registered callback routine with the NET layer to open the network 
 *   device.
 ***************************************************************************/
static int wan_dev_open (struct net_device *dev)
{
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : wan_dev_stop
 **************************************************************************
 * DESCRIPTION   :
 *   Registered callback routine with the NET layer to stop the network 
 *   device.
 ***************************************************************************/
static int wan_dev_stop (struct net_device *dev)
{
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : wan_dev_init
 **************************************************************************
 * DESCRIPTION   :
 *  Registered callback routine with the NET layer for device initialization
 ***************************************************************************/
static int wan_dev_init(struct net_device *p_dev)
{
    CPMAC_WAN_PRIVATE_INFO_T *ptr_wan_priv 	= p_dev->priv;
    char *mac_string        				= NULL;
	int i;

    mac_string=prom_getenv("mac_ewan");

    if(!mac_string)
    {
        mac_string="00.a0.e0.32.06.02";
        printk("Error getting mac from the boot loader enviroment for %s\n", p_dev->name);
        printk("Using default mac address: %s\n", mac_string);
        printk("Use the boot loader command:\n");
		printk("    setenv mac_ewan xx.xx.xx.xx.xx.xx\n");
        printk("to set mac address\n");
    }

    str2eaddr(ptr_wan_priv->mac_addr, mac_string);

    for (i=0; i <= ETH_ALEN; i++)
    {
        /* This sets the hardware address */
		p_dev->dev_addr[i] = ptr_wan_priv->mac_addr[i]; 
    }

    /* Fill in the fields. */
	p_dev->open               = wan_dev_open;
	p_dev->stop               = wan_dev_stop;
	p_dev->get_stats          = wan_get_stats;
	p_dev->hard_start_xmit    = wan_xmit;

    wanlogMsg (WAN_LOG_DEBUG, "DEBUG: WAN Device Initialization completed.\n",0,0,0,0,0);
	return 0;
}

/**************************************************************************
 * FUNCTION NAME : wan_xmit
 **************************************************************************
 * DESCRIPTION   :
 *  Registered callback routine with the NET layer to transmit a packet.
 ***************************************************************************/
static int wan_xmit(struct sk_buff *skb, struct net_device *dev)
{   
	wanlogMsg (WAN_LOG_DEBUG, "Tx packet on WAN port.\n",0,0,0,0,0);
	cpmac_dev_tx(skb, dev);
	return 0;
}

/**************************************************************************
 * FUNCTION NAME : wan_get_stats
 **************************************************************************
 * DESCRIPTION   :
 *  Registered callback routine with the NET layer to retreive statistics.
 ***************************************************************************/
static struct net_device_stats *wan_get_stats(struct net_device *dev)
{
    CPMAC_WAN_PRIVATE_INFO_T* ptr_wan_dev_priv = (CPMAC_WAN_PRIVATE_INFO_T *)dev->priv;
	return &ptr_wan_dev_priv->net_dev_stats;
}

/**************************************************************************
 * FUNCTION NAME : wan_dev_initialize
 **************************************************************************
 * DESCRIPTION   :
 *  This is the standard API function that needs to be called to create a 
 *  WAN Bridge Device. The 'dev_name' is passed as a parameter to the 
 *  function the function assumes that all necessary validations have been 
 *  done before.  
 *
 * RETURNS       :
 *      Pointer to the WAN Bridge network device    - Success.
 *      NULL                                        - Error.
 ***************************************************************************/
static int __init wan_dev_initialize(void)
{
	int                 err;
    struct net_device*  p_dev;
    size_t		        dev_size;

    dev_size = sizeof(struct net_device) + sizeof(CPMAC_WAN_PRIVATE_INFO_T);

    /* Allocate memory for the NET device. */
    if((p_dev = (struct net_device *) kmalloc(dev_size, GFP_KERNEL)) == NULL)
	{
	    wanlogMsg (WAN_LOG_FATAL, "Error: Unable to allocate memory for the NET device.\n",0,0,0,0,0);
        return -1;
    }
    
	memset ((void *)p_dev, 0, dev_size);

    /* Initialize the private structure. */
	p_dev->priv = (CPMAC_WAN_PRIVATE_INFO_T*)(((char *) p_dev) + sizeof(struct net_device));

	dev_alloc_name(p_dev, "ewan0"); /* allocating device name */

    ether_setup(p_dev);  /* Remove this LATER */

    /* Register the Intialization and Uninitialization functions. */
	p_dev->init    = wan_dev_init;

    /* Register the netdevice. */ 
	err = register_netdev(p_dev);
	if (err < 0)
	{
	    wanlogMsg (WAN_LOG_FATAL, "Error: Could not register NET device ewan0.\n",0,0,0,0,0);
        kfree(p_dev);
		return -1;
	}

	global_wan_dev_array[0] = p_dev;

	/* Proc entry for statistics */
    wan_stats = create_proc_entry("avalanche/ewan_stats", 0644, NULL);
    if(wan_stats)
    {
            wan_stats->read_proc  = ewan_read_stats;
            wan_stats->write_proc = ewan_write_stats;
    }


	/* Create read/write proc entry */
	wan_config = create_proc_entry("avalanche/wan_config", 0, NULL);
	if (wan_config) 
	{
			wan_config->data = NULL;
			wan_config->read_proc = wan_config_read;
			wan_config->write_proc = wan_config_write;
			wan_config->owner = THIS_MODULE;
	}
	else
	{
	    wanlogMsg (WAN_LOG_FATAL, "Error: Could not create proc entry.\n",0,0,0,0,0);
		return -1;
	}

    wanlogMsg (WAN_LOG_DEBUG, "Installed ewan0 NET device.\n",0,0,0,0,0);
	return 0;
}

/**************************************************************************
 * FUNCTION NAME : wan_dev_uninitialize
 **************************************************************************
 * DESCRIPTION   :
 *  The API Function provided to uninitialize a network device. 
 ***************************************************************************/
void wan_dev_uninitialize(void)
{
    struct net_device *p_dev;

	p_dev = global_wan_dev_array[0];

	/* Remove proc entry */
	remove_proc_entry("avalanche/wan_config", NULL);

    /* Unregister the network device. */
    unregister_netdev(p_dev);

    /* Clean up the memory allocated for the NET Device.*/ 
    kfree (p_dev);
    return;
}

module_init(wan_dev_initialize);
module_exit(wan_dev_uninitialize);
