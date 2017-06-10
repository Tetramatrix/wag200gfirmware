
#ifndef __PSP_USB_HOST_CONFIG_H__

#include<linux/config.h>
#define  __PSP_USB_HOST_CONFIG_H__

/* Configuration structure definitions */
#ifdef CONFIG_USB_PSP_HOST11_VBUS

#elif defined(CONFIG_USB_PSP_HOST11_VLYNQ )
/* encapsulate all parameters required for configuring V2U Host
 * in this structure */
typedef struct
{
	unsigned int baseaddress;
	unsigned int irq;       /* vlynq bit posn interr is mapped to */
	unsigned int reset_base;/* reset register address on V2U*/
        unsigned int reset_bit;/* Host Reset bit posn */
        unsigned int clock_base;/* address for clock control reg on V2U*/
	unsigned int clock_val;/* value to be programmed in clock reg */
	unsigned int endian_reg;/*address for Endianess reg*/
	unsigned int endian_val;/* value to be programmed for endianess*/
}VLYNQ_CFG_HDR;


extern VLYNQ_CFG_HDR * usb_host_get_config(void);
#else
#error "USB_PSP_HOST11 Bus type  not defined- define VBUS/VLYNQ"
#endif


#endif
