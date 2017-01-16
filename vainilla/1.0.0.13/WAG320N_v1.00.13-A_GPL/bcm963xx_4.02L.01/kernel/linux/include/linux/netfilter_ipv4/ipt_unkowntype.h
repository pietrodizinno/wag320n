#ifndef _IPT_UNKOWNTYPE_P_H
#define _IPT_UNKOWNTYPE_P_H

/* *** PERFORMANCE TWEAK ***
 * Packet size and search string threshold,
 * above which sublinear searches is used. */
#define IPT_STRING_HAYSTACK_THRESH	100
#define IPT_STRING_NEEDLE_THRESH	20

#define BM_MAX_NLEN 256
#define BM_MAX_HLEN 1024

typedef char *(*proc_ipt_search) (char *, char *, int, int);

struct ipt_unkowntype_info {
    int unkownprotocol;
    u_int16_t invert;
};

#endif /* _IPT_UNKOWNTYPE_P_H */
