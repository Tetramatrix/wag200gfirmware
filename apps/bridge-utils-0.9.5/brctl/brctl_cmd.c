/*
 * Copyright (C) 2000 Lennert Buytenhek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <asm/param.h>
#include "libbridge.h"
#include "brctl.h"

int br_cmd_addbr(struct bridge *br, char *brname, char *arg1, char *arg2, char *arg3)
{
	int err;

	if ((err = br_add_bridge(brname)) == 0)
		return 0;

	switch (err) {
	case EEXIST:
		fprintf(stderr,	"device %s already exists; can't create "
			"bridge with the same name\n", brname);
		break;

	default:
		perror("br_add_bridge");
		break;
	}
  return 1;
}

int br_cmd_delbr(struct bridge *br, char *brname, char *arg1, char *arg2, char *arg3)
{
	int err;

	if ((err = br_del_bridge(brname)) == 0)
		return 0;

	switch (err) {
	case ENXIO:
		fprintf(stderr, "bridge %s doesn't exist; can't delete it\n",
			brname);
		break;

	case EBUSY:
		fprintf(stderr, "bridge %s is still up; can't delete it\n",
			brname);
		break;

	default:
		perror("br_del_bridge");
		break;
	}
  return 1;
}

int br_cmd_addif(struct bridge *br, char *ifname, char *arg1, char *arg2, char *arg3)
{
	int err;
	int ifindex;

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		return 1;
	}

	if ((err = br_add_interface(br, ifindex)) == 0)
		return 0;

	switch (err) {
	case EBUSY:
		fprintf(stderr,	"device %s is already a member of a bridge; "
			"can't enslave it to bridge %s.\n", ifname,
			br->ifname);
		break;

	case ELOOP:
		fprintf(stderr, "device %s is a bridge device itself; "
			"can't enslave a bridge device to a bridge device.\n",
			ifname);
		break;

	default:
		perror("br_add_interface");
		break;
	}
  return 1;
}

int br_cmd_delif(struct bridge *br, char *ifname, char *arg1, char *arg2, char *arg3)
{
	int err;
	int ifindex;

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "interface %s does not exist!\n", ifname);
		return 1;
	}

	if ((err = br_del_interface(br, ifindex)) == 0)
		return 0;

	switch (err) {
	case EINVAL:
		fprintf(stderr, "device %s is not a slave of %s\n",
			ifname, br->ifname);
		break;

	default:
		perror("br_del_interface");
		break;
	}
  return 1;
}

int br_cmd_setageing(struct bridge *br, char *time, char *arg1, char *arg2, char *arg3)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_ageing_time(br, &tv);
  return 0;
}

int br_cmd_setbridgeprio(struct bridge *br, char *_prio, char *arg1, char *arg2, char *arg3)
{
	int prio;

	sscanf(_prio, "%i", &prio);
	br_set_bridge_priority(br, prio);
  return 0;
}

int br_cmd_setfd(struct bridge *br, char *time, char *arg1, char *arg2, char *arg3)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_forward_delay(br, &tv);
  return 0;
}

int br_cmd_setgcint(struct bridge *br, char *time, char *arg1, char *arg2, char *arg3)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_gc_interval(br, &tv);
  return 0;
}

int br_cmd_sethello(struct bridge *br, char *time, char *arg1, char *arg2, char *arg3)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_hello_time(br, &tv);
  return 0;
}

int br_cmd_setmaxage(struct bridge *br, char *time, char *arg1, char *arg2, char *arg3)
{
	double secs;
	struct timeval tv;

	sscanf(time, "%lf", &secs);
	tv.tv_sec = secs;
	tv.tv_usec = 1000000 * (secs - tv.tv_sec);
	br_set_bridge_max_age(br, &tv);
  return 0;
}

int br_cmd_setpathcost(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	int cost;
	struct port *p;

	if ((p = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return 1;
	}

	sscanf(arg1, "%i", &cost);
	br_set_path_cost(p, cost);
  return 0;
}

int br_cmd_setportprio(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	int cost;
	struct port *p;

	if ((p = br_find_port(br, arg0)) == NULL) {
		fprintf(stderr, "can't find port %s in bridge %s\n", arg0, br->ifname);
		return 1;
	}

	sscanf(arg1, "%i", &cost);
	br_set_port_priority(p, cost);
  return 0;
}

int br_cmd_stp(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	int stp;

	stp = 0;
	if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
		stp = 1;

	br_set_stp_state(br, stp);
  return 0;
}

int br_cmd_showstp(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	br_dump_info(br);
  return 0;
}

int br_cmd_show(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	printf("bridge name\tbridge id\t\tSTP enabled\tinterfaces\n");
	br = bridge_list;
	while (br != NULL) {
		printf("%s\t\t", br->ifname);
		br_dump_bridge_id((unsigned char *)&br->info.bridge_id);
		printf("\t%s\t\t", br->info.stp_enabled?"yes":"no");
		br_dump_interface_list(br);

		br = br->next;
	}
  return 0;
}

static int compare_fdbs(const void *_f0, const void *_f1)
{
	const struct fdb_entry *f0 = _f0;
	const struct fdb_entry *f1 = _f1;

#if 0
	if (f0->port_no < f1->port_no)
		return -1;

	if (f0->port_no > f1->port_no)
		return 1;
#endif

	return memcmp(f0->mac_addr, f1->mac_addr, 6);
}

void __dump_fdb_entry(struct fdb_entry *f)
{
	printf("%3i\t", f->port_no);
	printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t",
	       f->mac_addr[0], f->mac_addr[1], f->mac_addr[2],
	       f->mac_addr[3], f->mac_addr[4], f->mac_addr[5]);
	printf("%s\t\t", f->is_local?"yes":"no");
	br_show_timer(&f->ageing_timer_value);
	printf("\n");
}

int br_cmd_showmacs(struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
	struct fdb_entry fdb[1024];
	int offset;

	printf("port no\tmac addr\t\tis local?\tageing timer\n");

	offset = 0;
	while (1) {
		int i;
		int num;

		num = br_read_fdb(br, fdb, offset, 1024);
		if (!num)
			break;

		qsort(fdb, num, sizeof(struct fdb_entry), compare_fdbs);

		for (i=0;i<num;i++)
			__dump_fdb_entry(fdb+i);

		offset += num;
	}
  return 0;
}

int br_cmd_showfilter (struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
  struct filter_entry entry[20];
  int num, i;


  printf("\n\t\t\tBridge Filter Table\n\n");
  printf("Access Type\tDst MAC Addr\t\tSrc MAC Addr\t\tProtocol\n");


  num = br_show_filter(br, entry, 20);
  if (!num)
    return 0;

  for (i = 0; i < num; i++) {

      printf("\n%s\t\t%.2x-%.2x-%.2x-%.2x-%.2x-%.2x\t%.2x-%.2x-%.2x-%.2x-%.2x-%.2x\t0x%x\nFrames Matched: %d\n", entry[i].access_type? "Allow" : "Deny",entry[i].dst_mac_addr[0], entry[i].dst_mac_addr[1], entry[i].dst_mac_addr[2], entry[i].dst_mac_addr[3], entry[i].dst_mac_addr[4], entry[i].dst_mac_addr[5], entry[i].src_mac_addr[0], entry[i].src_mac_addr[1], entry[i].src_mac_addr[2], entry[i].src_mac_addr[3], entry[i].src_mac_addr[4], entry[i].src_mac_addr[5], entry[i].proto, entry[i].frames_matched);

  }


  return 0;
}

int br_cmd_setfilter (struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
  int filter_active;

  filter_active = 0;
  if (!strcmp(arg0, "on") || !strcmp(arg0, "yes") || !strcmp(arg0, "1"))
    filter_active = 1;

  br_set_filter_state(br, filter_active);

  return 0;
}

int br_cmd_addfilter (struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
  struct filter_entry *br_filter;
  char *ch;
  int i = 0, value;
  unsigned char filter_type = 0;


  /* Determine Type of Bridge Filtering */
  filter_type = (unsigned char) atoi(arg3);
  if (!((filter_type == 0) || (filter_type == 1))) {
    printf("Invalid Access Type Entry\n");
    return -1;
  }
    
  br_filter->access_type = (unsigned char)filter_type;


  /* Verify MAC Address Entered */
  if ((ch = strtok(arg0, "-" )) != NULL) {

      if (scan_number (&ch, &value, 0) == 0)
	br_filter->dst_mac_addr[i++] = value;
      else {
	printf("Invalid Destination MAC Address Entry\n");
	return -1;
      }

      while ((ch = strtok(NULL, "-")) != NULL)
	{
	  if (scan_number (&ch, &value, 0) == 0)
	    br_filter->dst_mac_addr[i++] = value;
	  else {
	    printf("Invalid Destination MAC Address Entry\n");
	    return -1;
	  }
	}
  }

  i = 0;
  if ((ch = strtok(arg1, "-" )) != NULL) {

      if (scan_number (&ch, &value, 0) == 0)
	br_filter->src_mac_addr[i++] = value;
      else {
	printf("Invalid Source MAC Address Entry\n");
	return -1;
      }

      while ((ch = strtok(NULL, "-")) != NULL)
	{
	  if (scan_number (&ch, &value, 0) == 0)
	    br_filter->src_mac_addr[i++] = value;
	  else {
	    printf("Invalid Source MAC Address Entry\n");
	    return -1;
	  }
	}
  }


  i = 0;
  if ((ch = strtok(arg2, "x" )) != NULL) {
      while ((ch = strtok(NULL, "x")) != NULL)
	{
	  if (scan_number (&ch, &value, 1) == 0)
	      br_filter->proto = value;
	  else {
	    printf("Invalid Protocol Entry\n");
	    return -1;
	  }
	}
  }

  return br_set_filter_entry(br, br_filter);
}

int br_cmd_delfilter (struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
  struct filter_entry *br_filter;
  char *ch;
  int i = 0, value;
  unsigned char filter_type = 0;


  /* Determine Type of Bridge Filtering */
  filter_type = (unsigned char) atoi(arg3);
  if (!((filter_type == 0) || (filter_type == 1))) {
    printf("Invalid Access Type Entry\n");
    return -1;
  }
    
  br_filter->access_type = (unsigned char)filter_type;


  /* Verify MAC Address Entered */
  if ((ch = strtok(arg0, "-" )) != NULL) {

      if (scan_number (&ch, &value, 0) == 0)
	br_filter->dst_mac_addr[i++] = value;
      else {
	printf("Invalid Destination MAC Address Entry\n");
	return -1;
      }

      while ((ch = strtok(NULL, "-")) != NULL)
	{
	  if (scan_number (&ch, &value, 0) == 0)
	    br_filter->dst_mac_addr[i++] = value;
	  else {
	    printf("Invalid Destination MAC Address Entry\n");
	    return -1;
	  }
	}
  }

  i = 0;
  if ((ch = strtok(arg1, "-" )) != NULL) {

      if (scan_number (&ch, &value, 0) == 0)
	br_filter->src_mac_addr[i++] = value;
      else {
	printf("Invalid Source MAC Address Entry\n");
	return -1;
      }

      while ((ch = strtok(NULL, "-")) != NULL)
	{
	  if (scan_number (&ch, &value, 0) == 0)
	    br_filter->src_mac_addr[i++] = value;
	  else {
	    printf("Invalid Source MAC Address Entry\n");
	    return -1;
	  }
	}
  }


  i = 0;
  if ((ch = strtok(arg2, "x" )) != NULL) {
      while ((ch = strtok(NULL, "x")) != NULL)
	{
	  if (scan_number (&ch, &value, 1) == 0)
	      br_filter->proto = value;
	  else {
	    printf("Invalid Protocol Entry\n");
	    return -1;
	  }
	}
  }

  return br_del_filter_entry(br, br_filter);
}

int br_cmd_flushfilter (struct bridge *br, char *arg0, char *arg1, char *arg2, char *arg3)
{
  return br_flush_filter(br);
}


static struct command commands[] = {
	{0, 1, "addbr", br_cmd_addbr},
	{1, 1, "addif", br_cmd_addif},
	{0, 1, "delbr", br_cmd_delbr},
	{1, 1, "delif", br_cmd_delif},
	{1, 1, "setageing", br_cmd_setageing},
	{1, 1, "setbridgeprio", br_cmd_setbridgeprio},
	{1, 1, "setfd", br_cmd_setfd},
	{1, 1, "setgcint", br_cmd_setgcint},
	{1, 1, "sethello", br_cmd_sethello},
	{1, 1, "setmaxage", br_cmd_setmaxage},
	{1, 2, "setpathcost", br_cmd_setpathcost},
	{1, 2, "setportprio", br_cmd_setportprio},
	{0, 0, "show", br_cmd_show},
	{1, 0, "showmacs", br_cmd_showmacs},
	{1, 0, "showstp", br_cmd_showstp},
	{1, 1, "stp", br_cmd_stp},
	{1, 1, "setfilter", br_cmd_setfilter},
	{1, 4, "addfilter", br_cmd_addfilter},
	{1, 0, "flushfilter", br_cmd_flushfilter},
	{1, 0, "showfilter", br_cmd_showfilter},
	{1, 4, "delfilter", br_cmd_delfilter},
};

struct command *br_command_lookup(char *cmd)
{
	int i;
	int numcommands;

	numcommands = sizeof(commands)/sizeof(commands[0]);

	for (i=0;i<numcommands;i++)
		if (!strcmp(cmd, commands[i].name))
			return &commands[i];

	return NULL;
}

static int scan_number(char **ppString, int *pValue, int type)
{
	int		nCounter;
	int 	nTempValue;
	int 	nBaseValue;
	char 	sNum[5];
	int		index = 0;
	char 	ch = **ppString;

	*pValue = 0;
	while(ch != '\0')
	{
		/* HEX Numbers were expected. Check for valid digits */
		if (( ((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'f')) || 
			  ((ch >= 'A') && (ch <= 'F')) ))
		{
			/* YES a valid digit was entered. Place it in the array. */
			sNum[index++] = ch;
		}
		else
		{
			/* This is NOT valid. */
			return -1;
		}

		/* Skip to the next character and store the previous charcter. */
		*ppString = *ppString + 1;
		ch = **ppString;

		/* Make sure that the length does NOT exceed MAX Permissible length */
		if(type == 0) {
		  if(index >= 3)
		    {
			/* Exceeded MAX Permissible length */
			return -1;
		    }
		}

		if(type == 1) {
		  if(index >= 5)
		    {
			/* Exceeded MAX Permissible length */
			return -1;
		    }
		}
	}

	/* Add the String Delimiter */
	sNum[index] = '\0';

	nBaseValue = 1;
	for(nCounter = index - 1; nCounter >= 0 ; nCounter--)
	{
		switch(sNum[nCounter])
		{
			case 'a':
			case 'A':	nTempValue = 10;
						break;
			case 'b':
			case 'B':	nTempValue = 11;
						break;
			case 'c':
			case 'C':	nTempValue = 12;
						break;
			case 'd':
			case 'D':	nTempValue = 13;
						break;
			case 'e':
			case 'E':	nTempValue = 14;
						break;
			case 'f':
			case 'F':	nTempValue = 15;
						break;
			default:	nTempValue = sNum[nCounter] - '0';
						break;
		}
		*pValue = *pValue + nTempValue * nBaseValue;
		nBaseValue = nBaseValue * 16;
	}
	return 0;
}
