#!/bin/sh

NVRAM=/usr/sbin/nvram

# some NVRAM defaults you can play with:

$NVRAM set usemtd5=ON             # (automount of minix filesystem on)
$NVRAM comit

uplink=`more /proc/avalanche/avsar_modem_stats | grep "US Connection Rate" | grep -v grep | awk '{print $4}'`
let uplink=80*$uplink/100
$NVRAM set rateup=$uplink
$NVRAM comit

# packets per sec (default: 20)
$NVRAM set dstlimit=100
$NVRAM comit

# Quantum is calculated as rate (in bytes) / r2q. r2q is by default 10 and can 
# be overruled if you create an htb qdisc.
# The default value is fair enough. Because I'm curious I'm trying to override 
# the quantum value for each class, which is possible.
# The quantum value should never be less then the MTU (1500)
# (http://www.docum.org/docum.org/faq/cache/31.html)
$NVRAM set quantum=6000
$NVRAM comit

# Calculation for htablesize
# Idea: Use 1/16384 of the total physical space
# Todo: replace num_physpages with active page, because this is
#       real amount of memory avaible.
# Max: 8192
# Min: 16
# This machine: (4096 num_physpages): 128
#
#size = (((num_physpages << PAGE_SHIFT) / 16384)
#		/ sizeof(struct list_head));
#if (num_physpages > (1024 * 1024 * 1024 / PAGE_SIZE))
#	size = 8192;
#if (size < 16)
#	size = 16;
#
$NVRAM set dstlimithtablesize=32
$NVRAM comit

# default 5
$NVRAM set dstlimitburst=2
$NVRAM comit

# default: 2000
# 5000 seems too me a good value for this machine
$NVRAM set dstlimitexpire=5000
$NVRAM comit

# default: 10000
# 5000 seems too me a good value for this machine
$NVRAM set dstlimitinterval=5000
$NVRAM comit

# transfer some of the torrent traffic to a higher qdics...
# (as far as I understand after an initial burst of x packets 
# x packets are send of per second with the higher qdisc until the burst 
# get refilled again) 
$NVRAM set torrentlimit=80
$NVRAM comit

$NVRAM set torrentburst=2
$NVRAM comit

# set StaticBuffers (skb buffer, default was 128, which is was to low)
$NVRAM set StaticBuffer=512
$NVRAM comit

# set StaticMode (either use StaticMode only or both StaticMode + Hybrid,
# Hybrid means dynamically allocation of memory is allowed.
# (It seems to me, that there was a bug in the original code switch)
$NVRAM set StaticMode=0
$NVRAM comit

# copy userdefined firewall to /mnt/mtd5 from where it starts and can be edited.
# You are encouraged to write your own scripts.

cp /usr/sbin/fw-scripts/finetune        /mnt/mtd5
cp /usr/sbin/fw-scripts/bad		/mnt/mtd5
cp /usr/sbin/fw-scripts/dstlimit	/mnt/mtd5
cp /usr/sbin/fw-scripts/htb             /mnt/mtd5
cp /usr/sbin/fw-scripts/tc		/mnt/mtd5
cp /usr/sbin/fw-scripts/rc.local	/mnt/mtd5
cp /usr/sbin/fw-scripts/ping		/mnt/mtd5
cp /usr/sbin/fw-scripts/whitelist	/mnt/mtd5
cp /usr/sbin/fw-scripts/monitor         /mnt/mtd5

# unmount filesystem (to persist files after reboot)
umount /mnt/mtd5

# and mount filesystem again
mount -t minix /dev/mtdblock/5 /mnt/mtd5

# finally let us enable our user defined firewall 
# and override the original Linksys qos-firewall!
# set to 0 if you don't want to. It's still beta status!
$NVRAM set beta=1
$NVRAM comit
