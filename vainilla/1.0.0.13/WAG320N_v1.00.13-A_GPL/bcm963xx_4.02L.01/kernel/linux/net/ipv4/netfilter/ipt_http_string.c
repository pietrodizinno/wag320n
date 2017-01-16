
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
 * 		
 *  This software based on ipt_string.c by Emmanuel Roger <winfield@freegates.be>
 *
 *  Copyright (C) 2006-2009, SerComm Corporation
 *  All Rights Reserved.  
 * 		
 */

#include <linux/smp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include "ipt_http_string.h"
#define smp_num_cpus				1

#define IPT_STRING_HAYSTACK_THRESH	100
#define IPT_STRING_NEEDLE_THRESH	20
//#define HTTP_STRING_DEBUG 

#define BM_MAX_NLEN 256
#define BM_MAX_HLEN 1024

struct string_per_cpu {
	int *skip;
	int *shift;
	int *len;
};


//#define HTTP_STRING_DEBUG

static struct string_per_cpu *bm_http_string_data=NULL;

#ifdef HTTP_STRING_DEBUG
#define between(x, start, end) ((x)>=(start) && (x)<=(end))
void print_packet(unsigned char *data, int len)
{
    int i,j;

    printk("packet length: %d%s:\n", len, len>100?"(only show the first 100 bytes)":"");
    if(len > 100) {
        len = 100;
    }
    for(i=0;len;) {
        if(len >=16 ) {
            for(j=0;j<16;j++) {
                printk("%02x ", data[i++]);
            }
            printk("| ");

            i -= 16;
            for(j=0;j<16;j++) {
                if( between(data[i], 0x21, 0x7e) ) {
                    printk("%c", data[i++]);
                }
                else {
                    printk(".");
                    i++;
                }
            }
            printk("\n");

            len -= 16;
        }
        else {
            /* last line */

            for(j=0; j<len; j++) {
                printk("%02x ", data[i++]);
           }
            for(;j<16;j++) {
                printk("   ");
            }
            printk("| ");

            i -= len;
            for(j=0;j<len;j++) {
                if( between(data[i], 0x21, 0x7e) ) {
                    printk("%c", data[i++]);
                }
                else {
                    printk(".");
                    i++;
                }
            }
            for(;j<16;j++) {
                printk(" ");
            }
            printk("\n");

            len = 0;
        }
    }
    return;

}
#endif

/* Search string in line which include "HOST" - Writer by Jeff Sun - May.23.2005 - */
static char *search_host_linear (char *needle, char *haystack, int needle_len, int haystack_len)
{
	char *k = haystack + (haystack_len-needle_len);
	char *t = haystack;
	char hoststr[7];

	strcpy(hoststr,"HOST: ");

	while ( t++ < k ) {
		if(*t=='\r' && *(t+1)=='\n' && *(t+2)=='\r' && *(t+3)=='\n')
			return NULL;

		if(strnicmp(t, hoststr, 6) == 0){
			t=t+6;
//			for(;*t!='\r' && *t!='\n';t++)
			if ( strnicmp(t, needle, needle_len) == 0 ) return t;

			return NULL;
		}
	}

	return NULL;
}

/* Boyer Moore Sublinear string search - VERY FAST */
static char *search_sublinear (char *needle, char *haystack, int needle_len, int haystack_len)
{
	int M1, right_end, sk, sh;
	int ended, j, i;

	int *skip, *shift, *len;

	/* use data suitable for this CPU */
	shift=bm_http_string_data[smp_processor_id()].shift;
	skip=bm_http_string_data[smp_processor_id()].skip;
	len=bm_http_string_data[smp_processor_id()].len;

	/* Setup skip/shift tables */
	M1 = right_end = needle_len-1;
	for (i = 0; i < BM_MAX_HLEN; i++) skip[i] = needle_len;
	for (i = 0; needle[i]; i++) skip[(int)needle[i]] = M1 - i;

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

	/* Do the search*/
	while (right_end < haystack_len)
	{
		for (i = 0; i < needle_len && haystack[right_end - i] == needle[M1 - i]; i++);
		if (i == needle_len) {
			return haystack+(right_end - M1);
		}

		sk = skip[(int)haystack[right_end - i]];
		sh = shift[i];
		right_end = max(right_end - i + sk, right_end + sh);
	}

	return NULL;
}

/* Linear string search based on memcmp() */
static char *search_linear (char *needle, char *haystack, int needle_len, int haystack_len)
{
	char *k = haystack + (haystack_len-needle_len);
	char *t = haystack;

	while ( t++ < k ) {
		/* Ron */
#if 0
		if ( memcmp(t, needle, needle_len) == 0 ) return t;
#else
		if ( strnicmp(t, needle, needle_len) == 0 ) return t;

#endif
	}

	return NULL;
}

typedef char *(*proc_ipt_search) (char *, char *, int, int);
static char *http_head_str[] = {"GET","HEAD","POST",NULL};

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const struct xt_match *match,
      const void *matchinfo,
      int offset,
      unsigned int protoff,
      int *hotdrop)
{
	const struct ipt_http_string_info *info = matchinfo;
	struct iphdr *ip = skb->nh.iph;
	int hlen;
	char  *haystack;
	int find = 0;
	int need_search_url = 0;
	int i;
	proc_ipt_search search = search_linear;

#ifdef HTTP_STRING_DEBUG
	printk("<0>""call obj https string\n");
#endif
	if ( !ip ) return 0;

	/* get lenghts, and validate them */
	hlen=ntohs(ip->tot_len)-(ip->ihl*4);
	haystack=(char *)ip+(ip->ihl*4);
#ifdef HTTP_STRING_DEBUG
	printk("<0>""hlen=%d,haystack=%x\n",hlen,haystack);
	print_packet(haystack, 100);
#endif
	i = -1;
	while(http_head_str[++i] != NULL)
	{
		if(search_linear(http_head_str[i], haystack, strlen(http_head_str[i]), hlen) != NULL)
		{
#ifdef HTTP_STRING_DEBUG
			printk("<0>""this packet is %s\n",http_head_str[i]);
#endif
			need_search_url = 1;
			break;
		}
	}

	if(need_search_url == 1)
	{
		for( i = 0; i < MAX_URL_NUM; i++)
		{
			if(info->url_length[i] != 0)
			{
#ifdef HTTP_STRING_DEBUG
					printk("<0>""begin to find url %s\n",info->url_string[i]);
#endif
				if(info->url_length[i] > hlen)
					continue;
				if(search_host_linear((char *)info->url_string[i], haystack, info->url_length[i], hlen) != NULL)
				{
					find = 1;
#ifdef HTTP_STRING_DEBUG
					printk("<0>""find url %s\n",info->url_string[i]);
#endif
					goto find_match;
				}
			}
		}
	}
#ifdef HTTP_STRING_DEBUG
	printk("<0>""can not find url,so begin to find key word\n");
#endif
	for( i = 0; i < MAX_KEY_WORD_NUM; i++)
	{
		if(info->key_length[i] != 0)
		{
#ifdef HTTP_STRING_DEBUG
			printk("<0>""begin to find key word %s\n",info->key_word[i]);
#endif
				/* The sublinear search comes in to its own
				 * on the larger packets */
			if ( (hlen>IPT_STRING_HAYSTACK_THRESH) &&
	  		(info->key_length[i]>IPT_STRING_NEEDLE_THRESH) ) {
			if ( hlen < BM_MAX_HLEN ) {
				search=search_sublinear;
			}else{
				if (net_ratelimit());
				}
			}
			if(info->key_length[i] > hlen)
				continue;
			if(search((char *)info->key_word[i], haystack, info->key_length[i], hlen) != NULL)
			{
				find = 1;
#ifdef HTTP_STRING_DEBUG
					printk("<0>""find keyword %s\n",info->key_word[i]);
#endif
				goto find_match;
			}
		}
	}
#ifdef HTTP_STRING_DEBUG
	printk("<0>""can not find match\n");
#endif
find_match :
    return (find ^ info->invert);
}

static int
checkentry(const char *tablename,
           const void *ip,
		   const struct xt_match *match,
           void *matchinfo,
           unsigned int hook_mask)
{

	/*
       if (matchsize != IPT_ALIGN(sizeof(struct ipt_http_string_info)))
               return 0;
			   */

       return 1;
}

static void string_freeup_data(void)
{
	int c;

	if ( bm_http_string_data ) {
		for(c=0; c<smp_num_cpus; c++) {
			if ( bm_http_string_data[c].shift ) kfree(bm_http_string_data[c].shift);
			if ( bm_http_string_data[c].skip ) kfree(bm_http_string_data[c].skip);
			if ( bm_http_string_data[c].len ) kfree(bm_http_string_data[c].len);
		}
		kfree(bm_http_string_data);
	}
}

static struct ipt_match http_string_match =
{
    .name 	= "http_string",
    .family	= AF_INET,
    .match 	= match,
    .matchsize	= sizeof(struct ipt_http_string_info),
    .checkentry = checkentry,
    .me 	= THIS_MODULE
};

static int __init init(void)
{
	int c;
	size_t tlen;
	size_t alen;

	tlen=sizeof(struct string_per_cpu)*smp_num_cpus;
	alen=sizeof(int)*BM_MAX_HLEN;

	/* allocate array of structures */
	if ( !(bm_http_string_data=kmalloc(tlen,GFP_KERNEL)) ) {
		return 0;
	}

	memset(bm_http_string_data, 0, tlen);

	/* allocate our skip/shift tables */
	for(c=0; c<smp_num_cpus; c++) {
		if ( !(bm_http_string_data[c].shift=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_http_string_data[c].skip=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_http_string_data[c].len=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
	}

	return xt_register_match(&http_string_match);

alloc_fail:
	string_freeup_data();
	return 0;
}

static void __exit fini(void)
{
	xt_unregister_match(&http_string_match);
	string_freeup_data();
}

module_init(init);
module_exit(fini);
