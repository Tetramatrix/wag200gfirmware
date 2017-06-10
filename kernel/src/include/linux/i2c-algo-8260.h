/* ------------------------------------------------------------------------- */
/* i2c-algo-8260.h i2c driver algorithms for MPC8260 CPM		     */
/* ------------------------------------------------------------------------- */

/* $Id: i2c-algo-8260.h,v 1.1.1.1 2006-01-16 08:12:45 jeff Exp $ */

#ifndef I2C_ALGO_8260_H
#define I2C_ALGO_8260_H 1

#include <linux/i2c.h>

struct i2c_algo_8260_data {
	uint dp_addr;
	int reloc;
	volatile i2c8260_t *i2c;
	volatile iic_t	*iip;
	volatile cpm8260_t *cp;

	u_char	temp[513];
};

#endif /* I2C_ALGO_8260_H */
