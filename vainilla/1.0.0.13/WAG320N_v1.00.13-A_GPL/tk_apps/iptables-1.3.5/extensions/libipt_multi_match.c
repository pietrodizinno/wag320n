/* Shared library add-on to iptables to add multi match support. 
 * 
 * Copyright (C) 2007 Oliver.Hao  <oliver_hao@sdc.sercomm.com>
 */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iptables.h>
#include "ipt_multi_match.h"

//#define MULTI_MATCH_DEBUG
/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"multi_match v%s options:\n"
"--file [!] file             Match multi condition in a packet\n",
IPTABLES_VERSION);

	fputc('\n', stdout);
}

static struct option opts[] = {
	{ "file", 1, 0, 'f' },
	{0}
};

/* Initialize the match. */
static void
init(struct ipt_entry_match *m, unsigned int *nfcache)
{
	*nfcache |= NFC_UNKNOWN;
}

int get_multi_match_condition(char *file, struct ipt_multi_match_info *multi_match_info)
{
	int fp;
	
	memset(multi_match_info,0,sizeof(struct ipt_multi_match_info));
	if((fp = open(file,O_RDONLY)) < 0)
	{
		printf("open %s fail\n",file);
		return -1;
	}

	if(read(fp,multi_match_info,sizeof(struct ipt_multi_match_info)) != sizeof(struct ipt_multi_match_info))
	{
		printf("read %s fail\n",file);
		return -1;
	}
	close(fp);
	return 1;
}

#ifdef MULTI_MATCH_DEBUG
void dump_multi_match(struct ipt_multi_match_info *multi_match_info)
{
	int i = 0;
	unsigned char *c;
	
	printf("dump multi match:\n");
	printf("	mac list:\n");
	for( i = 0; i < MAX_MAC_NUM; i++)
	{
		printf("		%02x%02x%02x%02x%02x%02x\n",multi_match_info->MAC[i][0],
			multi_match_info->MAC[i][1],multi_match_info->MAC[i][2],
			multi_match_info->MAC[i][3],multi_match_info->MAC[i][4],
			multi_match_info->MAC[i][5]);
	}

	printf("	ip list:\n");
	for( i = 0; i < MAX_IP_NUM; i++)
	{
		c = (unsigned char *)(&(multi_match_info->ip_addr[i]));
		printf("		%d.%d.%d.%d\n",c[0],c[1],c[2],c[3]);
	}

	printf("	ip_range list:\n");
	for( i = 0; i < MAX_IP_RANGE_NUM; i++)
	{
		c = (unsigned char *)(&(multi_match_info->ip_range[i][0]));
		printf("		%d.%d.%d.%d-",c[0],c[1],c[2],c[3]);
		c = (unsigned char *)(&(multi_match_info->ip_range[i][1]));
		printf("%d.%d.%d.%d\n",c[0],c[1],c[2],c[3]);
	}
}
#endif

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ipt_entry *entry,
      unsigned int *nfcache,
      struct ipt_entry_match **match)
{
	struct ipt_multi_match_info *multi_match_info = (struct ipt_multi_match_info *)(*match)->data;

	switch (c) {
	case 'f':
		check_inverse(optarg, &invert, &optind, 0);
		if(get_multi_match_condition(argv[optind-1], multi_match_info) < 0)
			return -1;
		if (invert)
			multi_match_info->invert = 1;
		*flags = 1;
		break;

	default:
		return 0;
	}
#ifdef MULTI_MATCH_DEBUG
	dump_multi_match(multi_match_info);
#endif
	return 1;
}

/* Final check; must have specified --string. */
static void
final_check(unsigned int flags)
{
	if (!flags)
		exit_error(PARAMETER_PROBLEM,
			   "STRING match: You must specify `--string'");
}

/* Prints out the matchinfo. */
static void
print(const struct ipt_ip *ip,
      const struct ipt_entry_match *match,
      int numeric)
{
	printf("multi match ");
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void
save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
{
	printf("can not save ");
}

static
struct iptables_match multi_match
= { 
    .name          = "multi_match",
    .version       = IPTABLES_VERSION,
    .size          = IPT_ALIGN(sizeof(struct ipt_multi_match_info)),
    .userspacesize = IPT_ALIGN(sizeof(struct ipt_multi_match_info)),
    .help          = &help,
    .init          = &init,
    .parse         = &parse,
    .final_check   = &final_check,
    .print         = &print,
    .save          = &save,
    .extra_opts    = opts
};

void _init(void)
{
	register_match(&multi_match);
}
