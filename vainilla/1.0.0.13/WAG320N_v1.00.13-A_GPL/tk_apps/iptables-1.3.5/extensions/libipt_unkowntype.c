/* Shared library add-on to iptables to add string matching support. 
 * 
 * Copyright (C) 2000 Emmanuel Roger  <winfield@freegates.be>
 *
 * ChangeLog
 *     27.01.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 *             Changed --tos to --string in save(). Also
 *             updated to work with slightly modified
 *             ipt_unkowntype_info.
 */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#include <iptables.h>
#include <linux/netfilter_ipv4/ipt_unkowntype.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"UNKOWNTYPE match v%s options:\n"
"--unkowntype [!] string         Match packets with unkown protocol\n",
IPTABLES_VERSION);

	fputc('\n', stdout);
}

static struct option opts[] = {
	{ "unkowntype", 1, 0, '1' },
	{0}
};

/* Initialize the match. */
static void
init(struct ipt_entry_match *m, unsigned int *nfcache)
{
	*nfcache |= NFC_UNKNOWN;
}

static void
parse_string(const unsigned char *s, struct ipt_unkowntype_info *info)
{	
	int i=0, slen;

	slen = strlen(s);

	if (slen == 0) {
		exit_error(PARAMETER_PROBLEM,
			"UNKOWNTYPE must contain at least one char");
	}

	while (i < slen) {
		if (s[i] > '9' || s[i] < '0') {
			exit_error(PARAMETER_PROBLEM,
				"UNKOWNTYPE include error char");
		} 
		i++;
	}
	info->unkownprotocol = atoi(s);

}

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ipt_entry *entry,
      unsigned int *nfcache,
      struct ipt_entry_match **match)
{
	struct ipt_unkowntype_info *stringinfo = (struct ipt_unkowntype_info *)(*match)->data;

	switch (c) {
	case '1':
		check_inverse(optarg, &invert, &optind, 0);
		parse_string(argv[optind-1], stringinfo);
		if (invert)
			stringinfo->invert = 1;
		*flags = 1;
		break;

	default:
		return 0;
	}
	return 1;
}

static void
print_string(char string[], int invert, int numeric)
{

	if (invert)
		fputc('!', stdout);
	printf("%s ",string);
}

/* Final check; must have specified --string. */
static void
final_check(unsigned int flags)
{
	if (!flags)
		exit_error(PARAMETER_PROBLEM,
			   "UNKOWNTYPE match: You must specify `--unkowntype'");
}

/* Prints out the matchinfo. */
static void
print(const struct ipt_ip *ip,
      const struct ipt_entry_match *match,
      int numeric)
{
	printf("unkowntype match ");
	print_string(((struct ipt_unkowntype_info *)match->data)->unkownprotocol?"all":"none",
		  ((struct ipt_unkowntype_info *)match->data)->invert, numeric);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
static void
save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
{
	printf("--unkowntype");
	print_string(((struct ipt_unkowntype_info *)match->data)->unkownprotocol?"all":"none",
		  ((struct ipt_unkowntype_info *)match->data)->invert, 0);
}

static
struct iptables_match unkowntype
= {
    .name			= "unkowntype",
    .version		= IPTABLES_VERSION,
    .size			= IPT_ALIGN(sizeof(struct ipt_unkowntype_info)),
    .userspacesize	= IPT_ALIGN(sizeof(struct ipt_unkowntype_info)),
    .help			= &help,
    .init			= &init,
    .parse			= &parse,
    .final_check	= &final_check,
    .print			= &print,
    .save			= &save,
    .extra_opts		= opts
};

void _init(void)
{
	register_match(&unkowntype);
}
