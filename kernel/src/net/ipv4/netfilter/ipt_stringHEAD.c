/* Kernel module to match a string into a packet.
 *
 * Copyright (C) 2000 Emmanuel Roger  <winfield@freegates.be>
 * 
 * ChangeLog
 *	19.02.2002: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed SMP re-entrancy problem using per-cpu data areas
 *		for the skip/shift tables.
 *	02.05.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed kernel panic, due to overrunning boyer moore string
 *		tables. Also slightly tweaked heuristic for deciding what
 * 		search algo to use.
 * 	27.01.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 * 		Implemented Boyer Moore Sublinear search algorithm
 * 		alongside the existing linear search based on memcmp().
 * 		Also a quick check to decide which method to use on a per
 * 		packet basis.
 */

#include <linux/smp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_stringHEAD.h>

#define smp_num_cpus 1

struct string_per_cpu {
	int *skip;
	int *shift;
	int *len;
};

struct string_per_cpu *bm_string_data_head=NULL;

/* Boyer Moore Sublinear string search - VERY FAST 
char *search_sublinear (char *needle, char *haystack, int needle_len, int haystack_len) 
{
	int M1, right_end, sk, sh;  
	int ended, j, i;

	int *skip, *shift, *len;
	
	// use data suitable for this CPU 
	shift=bm_string_data_head[smp_processor_id()].shift;
	skip=bm_string_data_head[smp_processor_id()].skip;
	len=bm_string_data_head[smp_processor_id()].len;
	
	// Setup skip/shift tables 
	M1 = right_end = needle_len-1;
	for (i = 0; i < BM_MAX_HLEN; i++) skip[i] = needle_len;  
	for (i = 0; needle[i]; i++) skip[needle[i]] = M1 - i;  

	for (i = 1; i < needle_len; i++) {   
		for (j = 0; j < needle_len && needle[M1 - j] == needle[M1 - i - j]; j++);  
		len[i] = j;  
	}  

	shift[0] = 1;  
	for (i = 1; i < needle_len; i++) shift[i] = needle_len;  
	for (i = M1; i > 0; i--) shift[len[i]] = i;  
	ended = 0;  
	
	for (i = 0; i < needle_len; i++) {  
		if (len[i] == M1 - i) ended = i;  
		if (ended) shift[i] = ended;  
	}  

	// Do the search
	while (right_end < haystack_len)
	{
		for (i = 0; i < needle_len && haystack[right_end - i] == needle[M1 - i]; i++);  
		if (i == needle_len) {
			return haystack+(right_end - M1);
		}
		
		sk = skip[haystack[right_end - i]];  
		sh = shift[i];
		right_end = max(right_end - i + sk, right_end + sh);  
	}

	return NULL;
}  
*/


/* Search string in the beginning of each line in HTTP head - Writer by Jeff Sun - Mar.3.2005 - */
char *search_head_linear (char *needle, char *haystack, int needle_len, int haystack_len) 
{
	char *k = haystack + (haystack_len-needle_len);
	char *t = haystack;

//printk("in stringHEAD\n");
	while ( t++ < k ) {		
		if(*t=='\r' && *(t+1)=='\n' && *(t+2)=='\r' && *(t+3)=='\n')
			return NULL;

check_again:	
		for(;*t!='\r' && t<k;t++) ;
		
		if(t==k)
			return NULL;
		else
		{
			t+=2;
		    
			if ( strnicmp(t, needle, needle_len) == 0 ) {
			    if(*needle!='P'){
                    char tmpc1,tmpc2;

                    if((unsigned long)t&1) t++;
                    tmpc1=*t;
                    tmpc2=*(t+1);
                    *t=*(t+2);
                    *(t+1)=*(t+3);
                    *(t+2)=tmpc1;
                    *(t+3)=tmpc2;
                    if(*needle=='S'){ //is "Set-Cookie"
                        goto check_again;
                    }
                }
				return t;
			}
			else
				continue;
		}
	}

	return NULL;
}


static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,
      const void *hdr,
      u_int16_t datalen,
      int *hotdrop)
{
	const struct ipt_string_info *info = matchinfo;
	struct iphdr *ip = skb->nh.iph;
	int hlen, nlen;
	char *needle, *haystack;
	proc_ipt_search search=search_head_linear;

	if ( !ip ) return 0;

	/* get lenghts, and validate them */
	nlen=info->len;
	hlen=ntohs(ip->tot_len)-(ip->ihl*4);
	if ( nlen > hlen ) return 0;

	needle=(char *)&info->string;
	haystack=(char *)ip+(ip->ihl*4);
	
	/* The sublinear search comes in to its own
	 * on the larger packets */
	if ( (hlen>IPT_STRING_HAYSTACK_THRESH) &&
	  	(nlen>IPT_STRING_NEEDLE_THRESH) ) {
		if ( hlen < BM_MAX_HLEN ) {
			search=search_head_linear;//search=search_sublinear;
		}else{
			if (net_ratelimit());
		}
	}
	
    return ((search(needle, haystack, nlen, hlen)!=NULL) ^ info->invert);
}

static int
checkentry(const char *tablename,
           const struct ipt_ip *ip,
           void *matchinfo,
           unsigned int matchsize,
           unsigned int hook_mask)
{

       if (matchsize != IPT_ALIGN(sizeof(struct ipt_string_info)))
               return 0;

       return 1;
}

static void string_freeup_data(void)
{
	int c;
	
	if ( bm_string_data_head ) {
		for(c=0; c<smp_num_cpus; c++) {
			if ( bm_string_data_head[c].shift ) kfree(bm_string_data_head[c].shift);
			if ( bm_string_data_head[c].skip ) kfree(bm_string_data_head[c].skip);
			if ( bm_string_data_head[c].len ) kfree(bm_string_data_head[c].len);
		}
		kfree(bm_string_data_head);
	}
}

static struct ipt_match stringHEAD_match =
{ 
    .list = { NULL, NULL },
    .name = "stringHEAD",
    .match = &match,
    .checkentry = &checkentry,
    .destroy = NULL,
    .me = THIS_MODULE
};

static int __init init(void)
{
	int c;
	size_t tlen;
	size_t alen;

	tlen=sizeof(struct string_per_cpu)*smp_num_cpus;
	alen=sizeof(int)*BM_MAX_HLEN;
	
	/* allocate array of structures */
	if ( !(bm_string_data_head=kmalloc(tlen,GFP_KERNEL)) ) {
		return 0;
	}
	
	memset(bm_string_data_head, 0, tlen);
	
	/* allocate our skip/shift tables */
	for(c=0; c<smp_num_cpus; c++) {
		if ( !(bm_string_data_head[c].shift=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_string_data_head[c].skip=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_string_data_head[c].len=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
	}
	
	return ipt_register_match(&stringHEAD_match);

alloc_fail:
	string_freeup_data();
	return 0;
}

static void __exit fini(void)
{
	ipt_unregister_match(&stringHEAD_match);
	string_freeup_data();
}

module_init(init);
module_exit(fini);
