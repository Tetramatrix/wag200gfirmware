/* Configuration parameters */


#include "psp-usb-host-config.h"

#define USB_HOST_LITTLE_ENDIAN

#ifdef CONFIG_USB_PSP_HOST11_VBUS
#endif

#ifdef CONFIG_USB_PSP_HOST11_VLYNQ
VLYNQ_CFG_HDR usbHostConfigTab[2]=
{
	/* definitions for Host: Vlynq0 V2U:Vlynq0*/
	{
	baseaddress: 0xac010000,/* base of Ohci Control regs */
	irq        : 4,      /* vlynq irq bit mapping */
	reset_base : 0xac000200,/* reset control reg on V2U */
        reset_bit  : 2,         /* Host reset bit posn */
        clock_base : 0xac000208,/* clock control reg on v2u */
	clock_val  : 0x3,       /* value to be programmed */
        endian_reg : 0xac00021c,
	endian_val : 0x0
	}
#if 0	
	,
	/* definitions for Host:Vlynq1 V2u:Vlynq0*/
	{
	baseaddress: 0xac010000,/* base of Ohci Control regs */
	irq        : 36,      /* vlynq irq bit mapping=32+4 */
	reset_base : 0xac000200,/* reset control reg on V2U */
        reset_bit  : 2,         /* Host reset bit posn */
        clock_base : 0xac000208,/* clock control reg on v2u */
	clock_val  : 0x3,       /* value to be programmed */
        endian_reg : 0xac00021c,
	endian_val : 0x0
	}
#endif	
};	


VLYNQ_CFG_HDR * usb_host_get_config()
{
	/* currently only Vlynq0 on Titan SDB supported */
	return ( &usbHostConfigTab[0]);
}


#endif /* end of  ifdef CONFIG_USB_PSP_HOST11_VLYNQ */

