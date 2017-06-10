#include <asm/addrspace.h>
#define GPIO_BASE       (KSEG1ADDR(0x08610900))
/*------------------------------------------*/
/* GPIO I/F                                 */
/*------------------------------------------*/

#define GPIO_DATA_INPUT      (*(volatile unsigned int *)(GPIO_BASE + 0x00000000))
#define GPIO_DATA_OUTPUT     (*(volatile unsigned int *)(GPIO_BASE + 0x00000004))
#define GPIO_DATA_DIR        (*(volatile unsigned int *)(GPIO_BASE + 0x00000008)) /* 0=out 1=in */
#define GPIO_DATA_ENABLE     (*(volatile unsigned int *)(GPIO_BASE + 0x0000000C)) /* 1=GPIO */


