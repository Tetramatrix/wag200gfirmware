CC=mipsel-uclibc-gcc
all:			brctl brctld
	

brctl:			brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			$(CC) -Wall -Os -o brctl brctl.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a

brctld:			brctld.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a
			$(CC) -Wall -Os -o brctld brctld.o brctl_cmd.o brctl_disp.o ../libbridge/libbridge.a

clean:
			rm -f *.o
			rm -f brctl
			rm -f brctld
			rm -f core

brctl.o:		brctl.c brctl.h
			$(CC) -I../libbridge -Wall -c -Os -o brctl.o brctl.c

brctld.o:		brctld.c brctl.h
			$(CC) -I../libbridge -Wall -c -Os -o brctld.o brctld.c

brctl_cmd.o:		brctl_cmd.c brctl.h
			$(CC) -I../libbridge -Wall -c -Os -o brctl_cmd.o brctl_cmd.c

brctl_disp.o:		brctl_disp.c brctl.h
			$(CC) -I../libbridge -Wall -c -Os -o brctl_disp.o brctl_disp.c
