/* 
  Kernel module to match application layer (OSI layer 7) 
  data in connections.
  
  http://l7-filter.sf.net

  By Matthew Strait and Ethan Sommer, 2003.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
  http://www.gnu.org/licenses/gpl.txt

  Based on ipt_string.c (C) 2000 Emmanuel Roger <winfield@freegates.be>
  and cls_layer7.c (C) 2003 Matthew Strait, Ethan Sommer, Justin Levandoski
*/

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <net/ip.h>
#include <net/tcp.h>

#include "regexp/regexp.c"

#include <linux/netfilter_ipv4/ipt_layer7.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/lockhelp.h>

MODULE_AUTHOR("Matthew Strait <quadong@users.sf.net>, Ethan Sommer <sommere@users.sf.net>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("iptables application layer match module");

#if defined(CONFIG_IP_NF_MATCH_LAYER7_DEBUG)
  #define DPRINTK(format,args...) printk(format,##args)
#else
  #define DPRINTK(format,args...)
#endif

//kylin add
#define CONFIG_IP_NF_MATCH_LAYER7_MAXDATALEN	2048

/* Number of packets whose data we look at.
This can be modified through /proc/net/layer7_numpackets */
static int num_packets = 8;

static struct pattern_cache {
  char * regex_string;
  regexp * pattern;
  struct pattern_cache * next;
} * first_pattern_cache = NULL;

/* I'm new to locking.  Here are my assumptions:

- No one is going to write to /proc/net/layer7_numpackets over and over
  within a short period of time, and if they did, nothing awful would happen.

- This code will never be processing the same packet twice at the same time,
  because iptables rules need to be traversed in order.

- It doesn't matter if two packets from different connections are in here at 
  the same time, because they don't share any data.

- It _does_ matter if two packets from the same connection are here at the same
  time.  In this case, the things we have to protect are the conntracks and
  the list of compiled patterns.
*/
DECLARE_RWLOCK(ct_lock);
DECLARE_LOCK(list_lock);

/* Use instead of regcomp.  As we expect to be seeing the same regexps over and
over again, it make sense to cache the results. */
static regexp * compile_and_cache(char * regex_string, char * protocol) 
{
        struct pattern_cache * node               = first_pattern_cache;
        struct pattern_cache * last_pattern_cache = first_pattern_cache;
	struct pattern_cache * tmp;
        unsigned int len;

        while (node != NULL) {
                if (!strcmp(node->regex_string, regex_string)) 
                        return node->pattern;

                last_pattern_cache = node;/* points at the last non-NULL node */
                node = node->next;
        }

        /* If we reach the end of the list, then we have not yet cached
           the pattern for this regex. Let's do that now. 
	   Be paranoid about running out of memory to avoid list corruption. */
        tmp = kmalloc(sizeof(struct pattern_cache), GFP_ATOMIC);

	if(!tmp) {
		printk(KERN_ERR "layer7: out of memory in compile_and_cache, bailing.\n");
		return NULL;
	}

        tmp->regex_string  = kmalloc(strlen(regex_string) + 1, GFP_ATOMIC);
        tmp->pattern       = kmalloc(sizeof(struct regexp),    GFP_ATOMIC);

	if(!tmp->regex_string || !tmp->pattern) {
		printk(KERN_ERR "layer7: out of memory in compile_and_cache, bailing.\n");
		kfree(tmp->regex_string);
		kfree(tmp->pattern);
		kfree(tmp);
		return NULL;
	}

        tmp->next = NULL;
	/* Ok.  The new node is all ready now. */
	node = tmp;

        if(first_pattern_cache == NULL) /* list is empty */
                first_pattern_cache = node; /* make node the beginning */
        else
                last_pattern_cache->next = node; /* attach node to the end */

        /* copy the string and compile the regex */
        len = strlen(regex_string);
	node->pattern = regcomp(regex_string, &len);
        if ( !node->pattern ) {
                printk(KERN_ERR "layer7: Error compiling regexp \"%s\" (%s)\n", 
			regex_string, protocol);
                /* pattern is now cached as NULL, so we won't try again. */
        }

        strcpy(node->regex_string, regex_string);
        return node->pattern;
}

#ifdef CONFIG_IP_NF_MATCH_LAYER7_DEBUG
/* Converts an unfriendly string into a friendly one by 
replacing unprintables with periods and all whitespace with " ". */
static char * friendly_print(unsigned char * s)
{
	char * f = kmalloc(strlen(s) + 1, GFP_ATOMIC);
	int i;

	if(!f) {
               printk(KERN_ERR "layer7: out of memory in friendly_print, bailing.\n");
               return NULL;
        }

	for(i = 0; i < strlen(s); i++){
		if(isprint(s[i]) && s[i] < 128)	f[i] = s[i];
		else if(isspace(s[i]))		f[i] = ' ';
		else 				f[i] = '.';
	}
	f[i] = '\0';
	return f;
}

static char dec2hex(int i)
{
	switch (i) {
		case 0 ... 9:
			return (char)(i + '0');
			break;
		case 10 ... 15:
			return (char)(i - 10 + 'a');
			break;
		default:
			printk("Problem in dec2hex\n");
			return '\0';
	}
}

static char * hex_print(unsigned char * s)
{
	char * g = kmalloc(strlen(s)*3 + 1, GFP_ATOMIC);
	int i;

	if(!g) {
               printk(KERN_ERR "layer7: out of memory in hex_print, bailing.\n");
               return NULL;
        }

	for(i = 0; i < strlen(s); i++) {
		g[i*3    ] = dec2hex(s[i]/16);
		g[i*3 + 1] = dec2hex(s[i]%16);
		g[i*3 + 2] = ' ';
	}
	g[i*3] = '\0';

	return g;
}
#endif // DEBUG


/* The following functions are here in case we get ported into an environment
(ebtables?) where skb->nh.iph->protocol isn't set. They assume that skb->data
points at the beginning of the IP datagram, which is true for iptables (but in
QoS it points to the beginning of the Ethernet frame). */
#if 0
#define IP_PROTO_OFFSET 9
static int is_tcp_over_ipv4 (const struct sk_buff *skb){return(skb->data[IP_PROTO_OFFSET]==IPPROTO_TCP );}
static int is_udp_over_ipv4 (const struct sk_buff *skb){return(skb->data[IP_PROTO_OFFSET]==IPPROTO_UDP );}
static int is_icmp_over_ipv4(const struct sk_buff *skb){return(skb->data[IP_PROTO_OFFSET]==IPPROTO_ICMP);}
#endif

static int can_handle(const struct sk_buff *skb)
{
	if(!skb->nh.iph) /* not IP */
		return 0;
	if(skb->nh.iph->protocol != IPPROTO_TCP &&
	   skb->nh.iph->protocol != IPPROTO_UDP &&
	   skb->nh.iph->protocol != IPPROTO_ICMP)
		return 0;
	return 1;
}

/* Returns offset the into the skb->data that the application data starts */
static int app_data_offset(const struct sk_buff *skb)
{
	/* In case we are ported somewhere (ebtables?) where skb->nh.iph 
	isn't set, this can be gotten from 4*(skb->data[0] & 0x0f) as well. */
        int ip_hl = 4*skb->nh.iph->ihl;

        if( skb->nh.iph->protocol == IPPROTO_TCP ) {
                /* 12 == offset into TCP header for the header length field. 
		Can't get this with skb->h.th->doff because the tcphdr 
		struct doesn't get set when routing (this is confirmed to be 
		true in Netfilter as well as QoS.) */
                int tcp_hl = 4*(skb->data[ip_hl + 12] >> 4);

                return ip_hl + tcp_hl;
        } else if( skb->nh.iph->protocol == IPPROTO_UDP  ) {
                return ip_hl + 8; /* UDP header is always 8 bytes */
        } else if( skb->nh.iph->protocol == IPPROTO_ICMP ) {
                return ip_hl + 8; /* ICMP header is 8 bytes */
        } else {
                printk(KERN_ERR "layer7: tried to handle unknown protocol!\n");
                return ip_hl + 8; /* something reasonable */
        }
}

/* handles whether there's a match when we aren't appending data anymore */
static int match_no_append(struct ip_conntrack * conntrack, 
			struct ip_conntrack * master_conntrack,
			struct ipt_layer7_info * info)
{
	/* If we're in here, we don't care about the app data anymore */
	WRITE_LOCK(&ct_lock);
	if(master_conntrack->layer7.app_data != NULL) {

               	#ifdef CONFIG_IP_NF_MATCH_LAYER7_DEBUG
               	if(!master_conntrack->layer7.app_proto) {
                       	char * f = friendly_print(master_conntrack->layer7.app_data);
		       	char * g = hex_print(master_conntrack->layer7.app_data);
                       	DPRINTK("\nGave up on the %d length stream: \n%s\n",
	                       master_conntrack->layer7.app_data_len, f);
		       	DPRINTK("\nIn hex: %s\n", g);
                       	kfree(f);
		       	kfree(g);
               	}
               	#endif

		kfree(master_conntrack->layer7.app_data);
		master_conntrack->layer7.app_data = NULL; /* don't free again */
	}
	WRITE_UNLOCK(&ct_lock);

	/* Is top-level master (possibly self) classified? */
	if(master_conntrack->layer7.app_proto) { 
		if(!strcmp(master_conntrack->layer7.app_proto, info->protocol))
		{
		    	/* set own .protocol (for /proc/net/ip_conntrack) */
			WRITE_LOCK(&ct_lock);
		    	if(!conntrack->layer7.app_proto) {
		        	conntrack->layer7.app_proto = kmalloc(strlen(info->protocol), GFP_ATOMIC);
                                if(!conntrack->layer7.app_proto){
                                       printk(KERN_ERR "layer7: out of memory in match_no_append, bailing.\n");
                                       WRITE_UNLOCK(&ct_lock);
                                       return 1;
                                }

		        	strcpy(conntrack->layer7.app_proto, info->protocol);
		    	}
			WRITE_UNLOCK(&ct_lock);
	
		    	return 1;
		} else return 0;
	} else return 0; /* no clasification */
}

/* add the new app data to the conntrack.  Return number of bytes added. */
static int add_data(struct ip_conntrack * master_conntrack, 
			char * app_data, int appdatalen)
{
	int length = 0, i;
	int oldlength = master_conntrack->layer7.app_data_len;

        /* Strip nulls. Make everything lower case (our regex lib doesn't
        do case insensitivity).  Add it to the end of the current data. */
	for(i = 0; i < CONFIG_IP_NF_MATCH_LAYER7_MAXDATALEN-oldlength-1 && 
		   i < appdatalen; i++) {
		if(app_data[i] != '\0') {
			master_conntrack->layer7.app_data[length+oldlength] = 
				/* the kernel version of tolower mungs 'upper ascii' */
				isascii(app_data[i])? tolower(app_data[i]) : app_data[i];
			length++;
		}
	}

	master_conntrack->layer7.app_data[length+oldlength] = '\0';
	master_conntrack->layer7.app_data_len = length + oldlength;

	return length;
}

/* Returns true on match and false otherwise.  */
static int match(/* const */struct sk_buff *skb, const struct net_device *in,
                 const struct net_device *out, const void *matchinfo,
                 int offset,                   int *hotdrop)
{
	struct ipt_layer7_info * info = (struct ipt_layer7_info *)matchinfo;
        enum ip_conntrack_info master_ctinfo, ctinfo;
        struct ip_conntrack *master_conntrack, *conntrack;
	unsigned char * app_data;  
	unsigned int pattern_result, appdatalen;
	regexp * comppattern;

	if(!can_handle(skb)){
		DPRINTK("layer7: This is some protocol I can't handle\n");
		return info->invert;
	}

	LOCK_BH(&list_lock);
	comppattern = compile_and_cache(info->pattern, info->protocol);
	UNLOCK_BH(&list_lock);
	/* the return value gets checked later, when we're ready to use it */

	app_data = skb->data + app_data_offset(skb);
	appdatalen = skb->tail - app_data;

	/* Treat the parent and all its children together as one connection, 
	except for the purpose of setting conntrack->layer7.pattern in the 
	actual connection. This makes /proc/net/ip_conntrack somewhat more 
	satisfying. */
        if(!(conntrack        = ip_conntrack_get((struct sk_buff *)skb, &ctinfo)) ||
           !(master_conntrack = ip_conntrack_get((struct sk_buff *)skb, &master_ctinfo))) {
                DPRINTK("layer7: packet is not from a known connection, giving up.\n");
		return info->invert;
        }
	
        /* Try to get a master conntrack (and its master etc) for FTP, etc. */
        while (master_ct(master_conntrack) != NULL)
                master_conntrack = master_ct(master_conntrack);

	/* skb->cb[0] == seen. Avoid doing things twice if there are two layer7 
	rules. I'm not sure that using cb for this purpose is correct, although 
	it says "put your private variables there" and this seems to qualify.  
	But it doesn't look like it's being used for anything else in the 
	sk_buffs that make it here. I'm open to suggestions for how to be able 
	to write to cb without making the compiler angry.  That I can't figure 
	this out is an argument against this being correct. */
	if(!skb->cb[0]){
		WRITE_LOCK(&ct_lock);
		master_conntrack->layer7.numpackets++;/*starts at 0 via memset*/
		WRITE_UNLOCK(&ct_lock);
	}

	/* On the first packet of a connection, allocate space for app data */
	WRITE_LOCK(&ct_lock);
	if(master_conntrack->layer7.numpackets == 1 && !skb->cb[0]) {
		master_conntrack->layer7.app_data = kmalloc(CONFIG_IP_NF_MATCH_LAYER7_MAXDATALEN, GFP_ATOMIC);
                if(!master_conntrack->layer7.app_data){                                                         
                        printk(KERN_ERR "layer7: out of memory in match, bailing.\n");
                        WRITE_UNLOCK(&ct_lock);
                        return info->invert;
                }

		master_conntrack->layer7.app_data[0] = '\0';
	}
	WRITE_UNLOCK(&ct_lock);

	/* if we've classified it or seen too many packets */
	if(master_conntrack->layer7.numpackets > num_packets || 
	   master_conntrack->layer7.app_proto) {
	
		pattern_result = match_no_append(conntrack, master_conntrack, info);
	
		/* mark the packet seen (probably irrelevant, but consistant) */
		skb->cb[0] = 1;

	        return (pattern_result ^ info->invert);
	}
	
	/* Can end up here, but unallocated, if numpackets is increased during 
	the beginning of a connection */
	if(master_conntrack->layer7.app_data == NULL)
		return (info->invert); /* unmatched */

	if(!skb->cb[0])	{
	 	int newbytes;
		WRITE_LOCK(&ct_lock);
		newbytes = add_data(master_conntrack, app_data, appdatalen);
		WRITE_UNLOCK(&ct_lock);

		if(newbytes == 0) { /* didn't add any data */
			skb->cb[0] = 1;
			/* Didn't match before, not going to match now */
			return info->invert;
		}
	}

	/* If the regexp failed to compile, don't bother running it */
	if(comppattern && regexec(comppattern, master_conntrack->layer7.app_data)) {
		DPRINTK("layer7: regexec positive: %s!\n", info->protocol);
		pattern_result = 1;
	} else pattern_result = 0;

	if(pattern_result) {
		WRITE_LOCK(&ct_lock);
		conntrack->layer7.app_proto = kmalloc(strlen(info->protocol), GFP_ATOMIC);
                if(!conntrack->layer7.app_proto){
                        printk(KERN_ERR "layer7: out of memory in match, bailing.\n");
                        WRITE_UNLOCK(&ct_lock);
                        return (pattern_result ^ info->invert);
                        }
 
		strcpy(conntrack->layer7.app_proto, info->protocol);
		WRITE_UNLOCK(&ct_lock);
	}

	/* mark the packet seen */
	skb->cb[0] = 1;

    	return (pattern_result ^ info->invert);
}

static int checkentry(const char *tablename, const struct ipt_ip *ip,
           void *matchinfo, unsigned int matchsize, unsigned int hook_mask)
{
       if (matchsize != IPT_ALIGN(sizeof(struct ipt_layer7_info))) 
               return 0;
       return 1;
}

static struct ipt_match layer7_match = { 
	.name = "layer7", 
	.match = &match, 
	.checkentry = &checkentry, 
	.me = THIS_MODULE 
};

/* taken from drivers/video/modedb.c */
static int my_atoi(const char *s)
{
    int val = 0;

    for (;; s++) {
        switch (*s) {
            case '0'...'9':
                val = 10*val+(*s-'0');
                break;
            default:
                return val;
        }
    }
}

/* write out num_packets to userland. */
static int layer7_read_proc(char* page, char ** start, off_t off, int count, 
                     int* eof, void * data) 
{
        if(num_packets > 99)
                printk(KERN_ERR "layer7: NOT REACHED. num_packets too big\n");
        
        page[0] = num_packets/10 + '0';
        page[1] = num_packets%10 + '0';
        page[2] = '\n';
        page[3] = '\0';
                
        *eof=1;

        return 3;
}

/* Read in num_packets from userland */
static int layer7_write_proc(struct file* file, const char* buffer, 
                      unsigned long count, void *data) 
{
        char * foo = kmalloc(count, GFP_ATOMIC);

        if(!foo){
                printk(KERN_ERR "layer7: out of memory, bailing.  num_packets unchanged.\n");
                return count;
        }

        /* copy in the data from userland */
        copy_from_user(foo, buffer, count);

        num_packets = my_atoi(foo);
	kfree (foo);

        /* This has an arbitrary limit to make the math easier. I'm lazy. 
	But anyway, 99 is a LOT! If you want more, you're doing it wrong! */
        if(num_packets > 99) {
                printk(KERN_WARNING "layer7: num_packets can't be > 99.\n");
                num_packets = 99;
        } else if(num_packets < 1) {
                printk(KERN_WARNING "layer7: num_packets can't be < 1.\n");
                num_packets = 1;
        }
	
        return count;
}

/* register the proc file */
static void layer7_init_proc(void)
{
        struct proc_dir_entry* entry;

        /* create the file */
        entry = create_proc_entry("layer7_numpackets", 0644, proc_net);

        /* set the callback functions */
	entry->read_proc = layer7_read_proc;
	entry->write_proc = layer7_write_proc;
}

static void layer7_cleanup_proc(void)
{
        remove_proc_entry("layer7_numpackets", proc_net);
}

static int __init init(void)
{
        layer7_init_proc();
	return xt_register_match(&layer7_match);
}

static void __exit fini(void)
{
	layer7_cleanup_proc();
	xt_unregister_match(&layer7_match);
}

module_init(init);
module_exit(fini);
