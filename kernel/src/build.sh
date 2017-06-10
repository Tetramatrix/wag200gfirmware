#!/bin/sh
CPMAC=./drivers/net/avalanche_cpmac/cpmac.c
LED_WEAPPER=./drivers/char/avalanche_led/led_wrapper.c

ln -sf config.normal .config
cp -f led_wrapper.c.mb101 $LED_WEAPPER
#cp -f cpmac.c.mb101 $CPMAC
#rm ./drivers/char/avalanche_led/*.o
#rm ./drivers/net/avalanche_cpmac/*.o

case "$1" in
	806v2)
		cp -f led_wrapper.c.806 $LED_WEAPPER
		ln -sf config.vpn .config
		;;
	806)
		cp -f led_wrapper.c.806 $LED_WEAPPER
		;;
	mb101)
		ln -sf config.internal .config
		;;
	v3)
#		ln -sf config.marvell.vpn.V3 .config
		ln -sf config.marvell.vpn.V3.fromvoice .config	
		cp -f led_wrapper.c.v3 $LED_WEAPPER
		;;
	vpn)
		ln -sf config.marvell.vpn .config
#		cp -f led_wrapper.c.voice $LED_WEAPPER
		;;
	mb102)
		ln -sf config.internal .config
		cp -f led_wrapper.c.mb102 $LED_WEAPPER
		#cp -f cpmac.c.mb102 $CPMAC
		;;
	805)
		ln -sf config.internal.voice .config
		cp -f led_wrapper.c.voice $LED_WEAPPER
		#cp -f cpmac.c.voice $CPMAC
		;;
	809)
		ln -sf config.marvell.voice .config
		cp -f led_wrapper.c.voice $LED_WEAPPER
		#cp -f cpmac.c.voice $CPMAC
		;;

esac

make oldconfig
