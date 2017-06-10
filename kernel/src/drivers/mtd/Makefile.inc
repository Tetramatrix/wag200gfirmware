#
# Out of tree drivers/mtd/Makefile.inc
#
# $Id: Makefile.inc,v 1.4 2004/07/14 01:43:43 jwboyer Exp $ 

# Use the default include if we are compiling out of tree

ifndef CONFIG_MTD
include $(mtd)/defconfig
EXTRA_CFLAGS := -g -DMTD_OUT_OF_TREE -Werror
endif

# This must be first in the include path, so it goes in $(CC) rather
# then $(EXTRA_CFLAGS)

CC += -I$(mtd)/../../include

ifdef SIMPLEMAP
EXTRA_CFLAGS += -DSIMPLEMAP
endif

include $(obj)/Makefile.common
