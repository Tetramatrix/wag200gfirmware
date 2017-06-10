/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|         Copyright (c) 1998-2004 Texas Instruments Incorporated           |**
**|                        ALL RIGHTS RESERVED                               |**
**|                                                                          |**
**| Permission is hereby granted to licensees of Texas Instruments           |**
**| Incorporated (TI) products to use this computer program for the sole     |**
**| purpose of implementing a licensee product based on TI products.         |**
**| No other rights to reproduce, use, or disseminate this computer          |**
**| program, whether in part or in whole, are granted.                       |**
**|                                                                          |**
**| TI makes no representation or warranties with respect to the             |**
**| performance of this computer program, and specifically disclaims         |**
**| any responsibility for any damages, special or consequential,            |**
**| connected with the use of this program.                                  |**
**|                                                                          |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

#ifndef _SANGAM_BOARDS_H
#define _SANGAM_BOARDS_H




// Let us define board specific information here. 


#if defined(CONFIG_MIPS_AR7DB) || defined(AR7DB)
#define AFECLK_FREQ                                 35328000
#define REFCLK_FREQ                                 25000000
#define OSC3_FREQ                                   24000000
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0x80000000
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               0x55555555  
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0x80000000
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          0
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         0
#endif


#if defined(CONFIG_MIPS_AR7RD) || defined(AR7RD) || defined (AR7L0)
#define AFECLK_FREQ                                 35328000
#define REFCLK_FREQ                                 25000000
#define OSC3_FREQ                                   24000000
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0x80000000
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               0x2
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0x80000000
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          0
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         0
#endif


#if defined(CONFIG_MIPS_AR7WI) || defined(AR7Wi)
#define AFECLK_FREQ                                 35328000
#define REFCLK_FREQ                                 25000000
#define OSC3_FREQ                                   24000000
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0x80000000
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               0x2
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0x80000000
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          0
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         0
#define VLYNQ0_RESET_GPIO_NUM                       18
#define AVALANCHE_NUM_VLYNQ_HOPS_PER_ROOT           1
#endif


#if defined(CONFIG_MIPS_AR7V) || defined(AR7V)
#define AFECLK_FREQ                                 35328000
#define REFCLK_FREQ                                 25000000
#define OSC3_FREQ                                   24000000
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0x80000000
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               0x2
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0x80000000
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          0
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         0
#define VLYNQ0_RESET_GPIO_NUM                       18
#define AVALANCHE_NUM_VLYNQ_HOPS_PER_ROOT           1
#endif

#if defined(CONFIG_MIPS_AR7VWI) || defined(AR7VWi) || defined(CONFIG_MIPS_AR7VW) || defined(AR7VW) \
 || defined(CONFIG_MIPS_AR7WRD) || defined(AR7WRD)
#define AFECLK_FREQ                                 35328000
#define REFCLK_FREQ                                 25000000
#define OSC3_FREQ                                   24000000
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0x80000000
/* For OHIO use always CPMAC0                     */
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               (IS_OHIO_CHIP() ? AVALANCHE_LOW_CPMAC_PHY_MASK : 0x00010000)
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0x80000000
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          (IS_OHIO_CHIP() ?  1 : 0)
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         1
#define VLYNQ0_RESET_GPIO_NUM                       18
#define AVALANCHE_NUM_VLYNQ_HOPS_PER_ROOT           1
#endif


#if defined CONFIG_MIPS_SEAD2
#define AVALANCHE_LOW_CPMAC_PHY_MASK                0xAAAAAAAA
#define AVALANCHE_HIGH_CPMAC_PHY_MASK               0x55555555
#define AVALANCHE_LOW_CPMAC_MDIX_MASK               0
#define AVALANCHE_LOW_CPMAC_HAS_EXT_SWITCH          0
#define AVALANCHE_HIGH_CPMAC_HAS_EXT_SWITCH         0
#include <asm/mips-boards/sead.h>
#endif

#endif
