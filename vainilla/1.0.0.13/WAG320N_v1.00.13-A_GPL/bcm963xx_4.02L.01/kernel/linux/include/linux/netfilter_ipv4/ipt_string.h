#ifndef _IPT_STRING_H
#define _IPT_STRING_H

#include <linux/netfilter/xt_string.h>

#define IPT_STRING_MAX_PATTERN_SIZE XT_STRING_MAX_PATTERN_SIZE
#define IPT_STRING_MAX_ALGO_NAME_SIZE XT_STRING_MAX_ALGO_NAME_SIZE
#define ipt_string_info xt_string_info


#define IPT_STRING_HAYSTACK_THRESH	100
#define IPT_STRING_NEEDLE_THRESH	20
#define BM_MAX_NLEN 256
#define BM_MAX_HLEN 1024

typedef char *(*proc_ipt_search) (char *, char *, int, int);

#endif /* _IPT_STRING_H */
