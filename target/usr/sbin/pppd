#!/bin/sh

cookie=`ps | grep cookie | grep -v grep | awk 'BEGIN{FS="=";}{print $2;}'`

if [ $cookie = "root" ]
then
	break;
else
	/usr/sbin/cookie&
fi