#include <linux/config.h>
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/psp_trace.h>
#include <asm/uaccess.h>

#define TRACE_NUM_ENTRIES 256  // always keep this variable as power of 2

#define TRACE_MAX_EVENT_NAME_LENGTH 30

typedef struct {
      unsigned long event_id;/* trace entry identification */
      unsigned long ts; /* Timestamp */
	  int param;
} trace_entry_t;
 
static int g_position = 0;

typedef struct {
	  int init_done;
	  int enable;
} trace_control_t;
 
typedef struct {
      char event_name[TRACE_MAX_EVENT_NAME_LENGTH];
	  int event_state;
	  int group;
} trace_evenr_record_in_lut_t;
 
trace_control_t trace_control = {FALSE,TRUE};
 
trace_entry_t   *trace_entry[TRACE_NUM_ENTRIES];
 
trace_evenr_record_in_lut_t trace_trace_lut[] =
        {
/* 0  */ 	{"tn7atm_sar_irq <-",1,TRACER_ATM_DRIVER_GROUP},
/* 1  */ 	{"tn7atm_sar_irq ->",1,TRACER_ATM_DRIVER_GROUP},
/* 2  */ 	{"tn7sar_handle_interrupt <-",0,TRACER_ATM_DRIVER_GROUP},
/* 3  */ 	{"tn7sar_handle_interrupt ->",0,TRACER_ATM_DRIVER_GROUP},
/* 4  */ 	{"DeviceInt <-",0,TRACER_ATM_DRIVER_GROUP},
/* 5  */ 	{"DeviceInt ->",0,TRACER_ATM_DRIVER_GROUP},
/* 6  */ 	{"cpsar_RxInt <-",1,TRACER_ATM_DRIVER_GROUP},
/* 7  */ 	{"cpsar_RxInt ->",1,TRACER_ATM_DRIVER_GROUP},
/* 8  */ 	{"cpsar_TxInt <-",1,TRACER_ATM_DRIVER_GROUP},
/* 9  */ 	{"cpsar_TxInt ->",1,TRACER_ATM_DRIVER_GROUP},
/* 10 */ 	{"tn7sar_receive <-",0,TRACER_ATM_DRIVER_GROUP},
/* 11 */ 	{"tn7sar_receive ->",0,TRACER_ATM_DRIVER_GROUP},
/* 12 */ 	{"tn7atm_receive <-",1,TRACER_ATM_DRIVER_GROUP},
/* 13 */ 	{"tn7atm_receive ->",1,TRACER_ATM_DRIVER_GROUP},
/* 14 */ 	{"push <-",1,TRACER_ATM_DRIVER_GROUP},
/* 15 */ 	{"push ->",1,TRACER_ATM_DRIVER_GROUP},
/* 16 */ 	{"halRxReturn <-",0,TRACER_ATM_DRIVER_GROUP},
/* 17 */ 	{"halRxReturn ->",0,TRACER_ATM_DRIVER_GROUP},
/* 18 */ 	{"tn7sar_malloc_rxbuffer <-",0,TRACER_ATM_DRIVER_GROUP},
/* 19 */ 	{"tn7sar_malloc_rxbuffer ->",0,TRACER_ATM_DRIVER_GROUP},
/* 20 */ 	{"tn7atm_send_complete <-",1,TRACER_ATM_DRIVER_GROUP},
/* 21 */ 	{"tn7atm_send_complete ->",1,TRACER_ATM_DRIVER_GROUP},
/* 22 */ 	{"pop <-",1,TRACER_ATM_DRIVER_GROUP},
/* 23 */ 	{"pop ->",1,TRACER_ATM_DRIVER_GROUP},
/* 24 */ 	{"tn7atm_send <-",1,TRACER_ATM_DRIVER_GROUP},
/* 25 */ 	{"tn7atm_send ->",1,TRACER_ATM_DRIVER_GROUP},
/* 26 */ 	{"tn7atm_queue_packet_to_sar <-",0,TRACER_ATM_DRIVER_GROUP},
/* 27 */ 	{"tn7atm_queue_packet_to_sar ->",0,TRACER_ATM_DRIVER_GROUP},
/* 28 */ 	{"tn7sar_send_packet <-",0,TRACER_ATM_DRIVER_GROUP},
/* 29 */ 	{"tn7sar_send_packet ->",0,TRACER_ATM_DRIVER_GROUP},
/* 30 */ 	{"halSend <-",0,TRACER_ATM_DRIVER_GROUP},
/* 31 */ 	{"halSend ->",0,TRACER_ATM_DRIVER_GROUP},

/* 32 */ 	{"cpmac_dev_tx <-",1,TRACER_ETH_DRIVER_GROUP},
/* 33 */ 	{"cpmac_dev_tx ->",1,TRACER_ETH_DRIVER_GROUP},
/* 34 */ 	{"cpmac_halSend <-",0,TRACER_ETH_DRIVER_GROUP},
/* 35 */ 	{"cpmac_halSend ->",0,TRACER_ETH_DRIVER_GROUP},
/* 36 */ 	{"cpmac_TxInt <-",1,TRACER_ETH_DRIVER_GROUP},
/* 37 */ 	{"cpmac_TxInt ->",1,TRACER_ETH_DRIVER_GROUP},
/* 38 */ 	{"cpmac_hal_send_complete <-",1,TRACER_ETH_DRIVER_GROUP},
/* 39 */ 	{"cpmac_hal_send_complete ->",1,TRACER_ETH_DRIVER_GROUP},
/* 40 */ 	{"cpmac_hal_isr <-",1,TRACER_ETH_DRIVER_GROUP},
/* 41 */ 	{"cpmac_hal_isr ->",1,TRACER_ETH_DRIVER_GROUP},
/* 42 */ 	{"cpmac_handle_tasklet <-",1,TRACER_ETH_DRIVER_GROUP},
/* 43 */ 	{"cpmac_handle_tasklet ->",1,TRACER_ETH_DRIVER_GROUP},
/* 44 */ 	{"cpmac_RxInt <-",0,TRACER_ETH_DRIVER_GROUP},
/* 45 */ 	{"cpmac_RxInt ->",0,TRACER_ETH_DRIVER_GROUP},
/* 46 */ 	{"cpmac_halRxReturn <-",0,TRACER_ETH_DRIVER_GROUP},
/* 47 */ 	{"cpmac_halRxReturn ->",0,TRACER_ETH_DRIVER_GROUP},
/* 48 */ 	{"cpmac_halIsr <-",0,TRACER_ETH_DRIVER_GROUP},
/* 49 */ 	{"cpmac_halIsr ->",0,TRACER_ETH_DRIVER_GROUP},
/* 50 */ 	{"cpmac_hal_receive <-",1,TRACER_ETH_DRIVER_GROUP},
/* 51 */ 	{"cpmac_hal_receive ->",1,TRACER_ETH_DRIVER_GROUP},
/* 52 */ 	{"netif_rx <-",1,TRACER_ETH_DRIVER_GROUP},
/* 53 */ 	{"netif_rx ->",1,TRACER_ETH_DRIVER_GROUP},
/* 54 */ 	{"cpmac_hal_malloc_buffer <-",0,TRACER_ETH_DRIVER_GROUP},
/* 55 */ 	{"cpmac_hal_malloc_buffer ->",0,TRACER_ETH_DRIVER_GROUP},
/* 56 */ 	{"atm_led <-",0,TRACER_ETH_DRIVER_GROUP},
/* 57 */ 	{"atm_led ->",0,TRACER_ETH_DRIVER_GROUP},
/* 58 */ 	{"cpmac_dev_alloc_skb <-",0,TRACER_ETH_DRIVER_GROUP},
/* 59 */ 	{"cpmac_dev_alloc_skb ->",0,TRACER_ETH_DRIVER_GROUP},
/* 60 */ 	{"atm_dev_alloc_skb <-",0,TRACER_ETH_DRIVER_GROUP},
/* 61 */ 	{"atm_dev_alloc_skb ->",0,TRACER_ETH_DRIVER_GROUP},


/* 62 */ 	{"ip_rcv <-",1,TRACER_IP_STACK_GROUP},
/* 63 */ 	{"ip_rcv ->",1,TRACER_IP_STACK_GROUP},
/* 64 */ 	{"ip_rcv_finish <-",1,TRACER_IP_STACK_GROUP},
/* 65 */ 	{"ip_rcv_finish ->",1,TRACER_IP_STACK_GROUP},
/* 66 */ 	{"ip_forward <-",1,TRACER_IP_STACK_GROUP},
/* 67 */ 	{"ip_forward ->",1,TRACER_IP_STACK_GROUP},
/* 68 */ 	{"ip_finish_output <-",1,TRACER_IP_STACK_GROUP},
/* 69 */ 	{"ip_finish_output ->",1,TRACER_IP_STACK_GROUP},
/* 70 */ 	{"ip_finish_output2 <-",1,TRACER_IP_STACK_GROUP},
/* 71 */ 	{"ip_finish_output2 ->",1,TRACER_IP_STACK_GROUP},

/* 72 */ 	{"USB_sendTx <-",1,TRACER_USB_DRIVER_GROUP},
/* 73 */ 	{"USB_sendTx ->",1,TRACER_USB_DRIVER_GROUP},
/* 74 */ 	{"rndis_usb_poll <-",1,TRACER_USB_DRIVER_GROUP},
/* 75 */ 	{"rndis_usb_poll ->",1,TRACER_USB_DRIVER_GROUP},
/* 76 */ 	{"USBEndReceiveData <-",1,TRACER_USB_DRIVER_GROUP},
/* 77 */ 	{"USBEndReceiveData ->",1,TRACER_USB_DRIVER_GROUP},
/* 78 */ 	{"rndis_receive_bulk <-",1,TRACER_USB_DRIVER_GROUP},
/* 79 */ 	{"rndis_receive_bulk ->",1,TRACER_USB_DRIVER_GROUP},
/* 80 */ 	{"rndis_usb_send_packet <-",1,TRACER_USB_DRIVER_GROUP},
/* 81 */ 	{"rndis_usb_send_packet ->",1,TRACER_USB_DRIVER_GROUP},
/* 82 */ 	{"rndis_send_complete <-",1,TRACER_USB_DRIVER_GROUP},
/* 83 */ 	{"rndis_send_complete ->",1,TRACER_USB_DRIVER_GROUP},
/* 84 */ 	{"usb_bulk_tx_start <-",1,TRACER_USB_DRIVER_GROUP},
/* 85 */ 	{"usb_bulk_tx_start ->",1,TRACER_USB_DRIVER_GROUP},
/* 86 */ 	{"usb_tasklet <-",1,TRACER_USB_DRIVER_GROUP},
/* 87 */ 	{"usb_tasklet ->",1,TRACER_USB_DRIVER_GROUP},
/* 88 */ 	{"hal_usb_isr <-",1,TRACER_USB_DRIVER_GROUP},
/* 89 */ 	{"hal_usb_isr ->",1,TRACER_USB_DRIVER_GROUP},

};

int trace_proc_print(char* page, char **start, off_t off, int count,int *eof, void *data);
int trace_ctl_read(char* page, char **start, off_t off, int count,int *eof, void *data);
int trace_ctl_write (struct file *fp, const char * buf, unsigned long count, void * data);
int trace_event_list_print(char* page, char **start, off_t off, int count,int *eof, void *data);
void trace_proc_print_2_scr(void);

/* Proc entries */
static struct proc_dir_entry *trace_ctl = NULL;

int __init trace_init(void)
{
	int i;
	
	if(trace_control.init_done == TRUE) 
	{
			printk("!!!!!!! Trace is already up !!!!!!!!!\n");
			return -1;
	}

	for(i=0; i<TRACE_NUM_ENTRIES; i++)
	{
#ifdef PAL
		if( PAL_osMemAlloc(0,sizeof(trace_entry_t),0,&(trace_entry[i])) == PAL_SOK )
			PAL_osMemSet(trace_entry[i], 0, sizeof(trace_entry_t));
#else
		if( (trace_entry[i] = kmalloc(sizeof(trace_entry_t), GFP_KERNEL)) != NULL )
			memset(trace_entry[i], 0, sizeof(trace_entry_t));
#endif
		else
		{
			printk("!!!!!!! OUT of memory !!!!!!!!!\n");
			trace_exit();
			return -1;
		}	
	}
	trace_control.init_done = TRUE;

	create_proc_read_entry("avalanche/trace",0,NULL,trace_proc_print,NULL);
	create_proc_read_entry("avalanche/trace_events",0,NULL,trace_event_list_print,NULL);

    trace_ctl = create_proc_entry("avalanche/trace_ctl", 0644, NULL);
    if(trace_ctl)
    {
        trace_ctl->read_proc  = trace_ctl_read;
        trace_ctl->write_proc = trace_ctl_write;
    }


	return 0;
}
 
void trace_exit(void) {
   int i;
  
  for(i=0; i<TRACE_NUM_ENTRIES; i++) 
    if(trace_entry[i])
#ifdef PAL
    	PAL_osMemFree(0, trace_entry[i], 0);
#else 
		kfree(trace_entry[i]);
#endif
	trace_control.init_done = FALSE;
 
}
 
void trace_event(int event_id,int param)
{   
	int pos,tmp;
	unsigned long flags;

	if(trace_control.enable == FALSE)
		return;

	if(trace_trace_lut[event_id].event_state == 0)
		return;

	TRACE_GET_TIMESTAMP(tmp);

	save_and_cli(flags);

	pos = g_position % TRACE_NUM_ENTRIES;
	g_position++;
	
	restore_flags(flags);

	trace_entry[pos]->ts = tmp;
	trace_entry[pos]->event_id= event_id;
	trace_entry[pos]->param= param;
}
 
void trace_act_disable(void) {
	trace_control.enable = FALSE;
}
void trace_enable(void) {
	trace_control.enable = TRUE;
}

const char end_string[] =   "\n------------------ END OF TRACE BUFFER ------------------------\n\n"  ;

int trace_proc_print(char* page, char **start, off_t off, int count,int *eof, void *data)
{
	signed int len = 0,size ;
	static int new_count;
	static int cur_pos,done;

	if (off == 0) 
	{
		trace_control.enable = FALSE;
		len += sprintf(page+len, "\n------------------ TRACE BUFFER, pos = %d  --------------------\n\n", g_position); 
		new_count=0;
		cur_pos = g_position < TRACE_NUM_ENTRIES ? 0 : g_position % TRACE_NUM_ENTRIES;
		done = 0;
	}

	size = g_position < TRACE_NUM_ENTRIES ? g_position : TRACE_NUM_ENTRIES;

 	
 	if ((new_count >= size) && (done == 1)) 
	{
		g_position = 0;
		trace_control.enable = TRUE;

		*eof = 1;
		return 0;
	}

	while( (len < ((signed int)(count - 80 - sizeof(end_string))))  && ( new_count < size) )
//	while( (len < ((count - 80 - sizeof(end_string))))  && ( new_count < size) )
	{
		len += sprintf(page+len, "%s %lu %x\n", 
						trace_trace_lut[trace_entry[cur_pos]->event_id].event_name,
						trace_entry[cur_pos]->ts,
						trace_entry[cur_pos]->param);
		cur_pos = ( cur_pos + 1) % TRACE_NUM_ENTRIES;
		new_count++;
	}
	
	if( new_count >= size && (len < (count - 80 - sizeof(end_string))) )
	{
		len += sprintf(page+len, end_string); 
		done=1;
	}

	*eof = 0;
	*start = page;

	return len;
}

void trace_proc_print_2_scr(void)
{
int i;

	trace_control.enable = FALSE;

	printk("\n------------------ TRACE BUFFER, pos = %d  --------------------\n\n", g_position); 


	if( g_position >= TRACE_NUM_ENTRIES )
	{
		for(i= g_position % TRACE_NUM_ENTRIES ; i < TRACE_NUM_ENTRIES; i++)
		{
				printk("%d %s %x %lu\n",i, 
								trace_trace_lut[trace_entry[i]->event_id].event_name,trace_entry[i]->param,trace_entry[i]->ts);
		}
	}
			
	for(i = 0; i < ( g_position % TRACE_NUM_ENTRIES ); i++)
	{
			printk("%d %s %x %lu\n",i, 
							trace_trace_lut[trace_entry[i]->event_id].event_name,trace_entry[i]->param,trace_entry[i]->ts);
	}
	
	printk("\n------------------------ END OF TRACE BUFFER -------------------------\n\n"); 

	trace_control.enable = TRUE;

	return;
}

int trace_ctl_read(char* page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;

	if(trace_control.enable == TRUE)
       len+= sprintf(page+len, "\n Trace enable\n");
	else
       len+= sprintf(page+len, "\n Trace disable\n");

	return(len);
		
}
int trace_ctl_write (struct file *fp, const char * buf, unsigned long count, void * data)
{
    char local_buf[31];
    int  ret_val = 0;

    if(count > 30)
    {
        printk("Error : Buffer Overflow\n");
        printk("Use \"echo 0 > trace_ctl\" to disable the trace\n");
        printk("Use \"echo 1 > trace_ctl\" to enable the trace\n");
        return -EFAULT;
    }

    copy_from_user(local_buf,buf,count);
    local_buf[count-1]='\0'; /* Ignoring last \n char */
    ret_val = count;
    
	printk(local_buf);

    if(strcmp("0",local_buf)==0)
    {
		printk("\n Trace disable\n");
		trace_act_disable();
	}
    if(strcmp("1",local_buf)==0)
    {
		printk("\n Trace enable\n");
		trace_enable();
	}

    return ret_val;
}
int trace_event_list_print(char* page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
	static int i=0;
	int size;
	
	size= sizeof(trace_trace_lut)/sizeof(trace_trace_lut[0]);

	if (off == 0) 
	{
		len+=sprintf(page+len,"Available events for tracing %d\n",size);
		i=0;
	}

 	if (i >= size) 
	{
		*eof = 1;
		return 0;
	}

	while( (len < (count - 80)) && (i < size) )
	{
		len += sprintf(page+len,"id=%d name:%s state %d\n",i,trace_trace_lut[i].event_name,trace_trace_lut[i].event_state);
		i++;
	}

	
	*eof = 0;
	*start = page;
	return len;
}
 
module_init(trace_init);
EXPORT_SYMBOL_NOVERS(trace_event);
//EXPORT_SYMBOL(trace_event);