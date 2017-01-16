/* Shared library add-on to iptables to add http string matching support. 
 * 
 * Copyright (C) Oliver.Hao <oliver_hao@sdc.sercomm.com>
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
#include "ipt_http_string.h"

//#define HTTP_STRING_DEBUG
/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"HTTP STRING match v%s options:\n"
"--string [!] string             Match a string in a packet\n",
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

/* Final check; must have specified --string. */
static void
final_check(unsigned int flags)
{
	if (!flags)
		exit_error(PARAMETER_PROBLEM,
			   "STRING match: You must specify `--string'");
}

int get_http_match_condition(char *file, struct ipt_http_string_info *http_match_info)
{
	int fp;
	
	memset(http_match_info,0,sizeof(struct ipt_http_string_info));
	if((fp = open(file,O_RDONLY)) < 0)
	{
		printf("open %s fail\n",file);
		return -1;
	}

	if(read(fp,http_match_info,sizeof(struct ipt_http_string_info)) != sizeof(struct ipt_http_string_info))
	{
		printf("read %s fail\n",file);
		return -1;
	}
	close(fp);
	return 1;
}

#ifdef HTTP_STRING_DEBUG
void dump_http_string(struct ipt_http_string_info *http_match_info)
{
	int i;

	printf("http string list:\n");
	printf("	url string list:\n");
	for( i = 0; i < MAX_URL_NUM; i++)
		printf("		len=%d,str=%s\n",http_match_info->url_length[i],http_match_info->url_string[i]);
	printf("	key word string list:\n");
	for( i = 0; i < MAX_KEY_WORD_NUM; i++)
		printf("		len=%d,str=%s\n",http_match_info->key_length[i],http_match_info->key_word[i]);
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
	struct ipt_http_string_info *http_match_info = (struct ipt_http_string_info *)(*match)->data;

	switch (c) {
	case 'f':
		check_inverse(optarg, &invert, &optind, 0);
		if(get_http_match_condition(argv[optind-1], http_match_info) < 0)
			return -1;
		if (invert)
			http_match_info->invert = 1;
		*flags = 1;
		break;

	default:
		return 0;
	}
#ifdef HTTP_STRING_DEBUG
	dump_http_string(http_match_info);
#endif
	return 1;
}


/* Prints out the matchinfo. */
static void
print(const struct ipt_ip *ip,
      const struct ipt_entry_match *match,
      int numeric)
{
	printf("HTTP STRING match ");
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void
save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
{
	printf("Can not Support save ");
}

static
struct iptables_match http_string
= {
    .name          = "http_string",
    .version       = IPTABLES_VERSION,
    .size          = IPT_ALIGN(sizeof(struct ipt_http_string_info)),
    .userspacesize = IPT_ALIGN(sizeof(struct ipt_http_string_info)),
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
	register_match(&http_string);
}
