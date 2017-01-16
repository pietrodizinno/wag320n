/* Shared library add-on to iptables to add LOG support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <getopt.h>
#include <iptables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_dns_msq.h>

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"DNSMSQ v%s ",
IPTABLES_VERSION);
}

static struct option opts[] = {
	{ .name = 0 }
};

/* Initialize the target. */
static void
init(struct ipt_entry_target *t, unsigned int *nfcache)
{

}

static void
parse_string(const unsigned char *s, struct ipt_dnsmsq_info *info)
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


}
#define IPT_YHOO_OPT 0x01

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ipt_entry *entry,
      struct ipt_entry_target **target)
{
	return 1;
}

/* Final check; nothing. */
static void final_check(unsigned int flags)
{
}
static void
print_string(char string[], int invert, int numeric)
{

	if (invert)
		fputc('!', stdout);
	printf("%s ",string);
}
/* Prints out the targinfo. */
static void
print(const struct ipt_ip *ip,
      const struct ipt_entry_target *target,
      int numeric)
{
	printf("DNSMSQ");
}

/* Saves the union ipt_targinfo in parsable form to stdout. */
static void
save(const struct ipt_ip *ip, const struct ipt_entry_target *target)
{
}

static
struct iptables_target DNSMSQ
= {
    .name          = "DNSMSQ",
    .version       = IPTABLES_VERSION,
    .size          = IPT_ALIGN(sizeof(struct ipt_dnsmsq_info)),
    .userspacesize = IPT_ALIGN(sizeof(struct ipt_dnsmsq_info)),
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
	register_target(&DNSMSQ);
}
