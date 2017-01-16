/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2003,2004 USAGI/WIDE Project <http://www.linux-ipv6.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 23 Apr 2001: Harald Welte <laforge@gnumonks.org>
 *	- new API and handling of conntrack/nat helpers
 *	- now capable of multiple expectations for one master
 * 16 Jul 2002: Harald Welte <laforge@gnumonks.org>
 *	- add usage/reference counts to ip_conntrack_expect
 *	- export ip_conntrack[_expect]_{find_get,put} functions
 * 16 Dec 2003: Yasuyuki Kozakai @USAGI <yasuyuki.kozakai@toshiba.co.jp>
 *	- generalize L3 protocol denendent part.
 * 23 Mar 2004: Yasuyuki Kozakai @USAGI <yasuyuki.kozakai@toshiba.co.jp>
 *	- add support various size of conntrack structures.
 * 26 Jan 2006: Harald Welte <laforge@netfilter.org>
 * 	- restructure nf_conn (introduce nf_conn_help)
 * 	- redesign 'features' how they were originally intended
 * 26 Feb 2006: Pablo Neira Ayuso <pablo@eurodev.net>
 * 	- add support for L3 protocol module load on demand.
 *
 * Derived from net/ipv4/netfilter/ip_conntrack_core.c
 */

#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <net/ip.h>
#include <net/route.h>
#include <linux/tcp.h>
#include <linux/stddef.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/err.h>
#include <linux/percpu.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/mm.h>
#if defined(CONFIG_MIPS_BRCM)
#include <linux/blog.h>
#endif

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_tuple.h>

#define NF_CONNTRACK_VERSION	"0.5.0"

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

DEFINE_RWLOCK(nf_conntrack_lock);
EXPORT_SYMBOL_GPL(nf_conntrack_lock);

/* nf_conntrack_standalone needs this */
atomic_t nf_conntrack_count = ATOMIC_INIT(0);
EXPORT_SYMBOL_GPL(nf_conntrack_count);

void (*nf_conntrack_destroyed)(struct nf_conn *conntrack);
EXPORT_SYMBOL_GPL(nf_conntrack_destroyed);

unsigned int nf_conntrack_htable_size __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_htable_size);

int nf_conntrack_max __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_max);

struct list_head *nf_conntrack_hash __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_hash);

struct nf_conn nf_conntrack_untracked __read_mostly;
EXPORT_SYMBOL_GPL(nf_conntrack_untracked);

unsigned int nf_ct_log_invalid __read_mostly;
LIST_HEAD(unconfirmed);
static int nf_conntrack_vmalloc __read_mostly;

static unsigned int nf_conntrack_next_id;

DEFINE_PER_CPU(struct ip_conntrack_stat, nf_conntrack_stat);
EXPORT_PER_CPU_SYMBOL(nf_conntrack_stat);

/*
 * This scheme offers various size of "struct nf_conn" dependent on
 * features(helper, nat, ...)
 */

#define NF_CT_FEATURES_NAMELEN	256
static struct {
	/* name of slab cache. printed in /proc/slabinfo */
	char *name;

	/* size of slab cache */
	size_t size;

	/* slab cache pointer */
	struct kmem_cache *cachep;

	/* allocated slab cache + modules which uses this slab cache */
	int use;

} nf_ct_cache[NF_CT_F_NUM];

/* protect members of nf_ct_cache except of "use" */
DEFINE_RWLOCK(nf_ct_cache_lock);

/* This avoids calling kmem_cache_create() with same name simultaneously */
static DEFINE_MUTEX(nf_ct_cache_mutex);

static int nf_conntrack_hash_rnd_initted;
static unsigned int nf_conntrack_hash_rnd;

#if defined(CONFIG_MIPS_BRCM)
void (*dynahelper_ref)(struct module * m) = NULL;
EXPORT_SYMBOL_GPL(dynahelper_ref);
void (*dynahelper_unref)(struct module * m) = NULL;
EXPORT_SYMBOL_GPL(dynahelper_unref);

/* bugfix for lost connection */
LIST_HEAD(safe_list);
#endif

#if defined(CONFIG_SERCOMM_CODE)
int nf_conntrack_count_max_record =0;

#define HASH_PORT_MAX	40
struct port_n{
	struct list_head list;					// list 
	unsigned int port;						// port number
};

struct list_head g_reserve_port_list[HASH_PORT_MAX];

void reserve_port_list_init( void )
{
	int idx;

	for( idx =0; idx < HASH_PORT_MAX; idx ++) {
		INIT_LIST_HEAD(&g_reserve_port_list[idx]);
	}
}

int reserve_port_hash(unsigned int port )
{
	return (port % HASH_PORT_MAX);
}

/*

	return 
		1: is a reserve_port
		0: not a reserve_port
*/
int reserve_port_search( unsigned int port )
{
	struct list_head *hash,*lh;
	struct port_n *port_list;
	
	hash = &g_reserve_port_list[ reserve_port_hash(port ) ];

	for (lh = hash->next; lh != hash; lh = lh->next) 
	{
		port_list = list_entry(lh,struct port_n,list);	

		if( port_list->port == port)
			return 1;
	}

	return 0;
}

void reserve_port_add( unsigned int port )
{
	struct list_head *hash;
	struct port_n* new_port = NULL;
	int idx;

	idx =  reserve_port_hash(port ) ;
	if( reserve_port_search (port ) == 0 ){	// can't find this port
		hash = &g_reserve_port_list[idx];		

		new_port = kmalloc(sizeof(struct port_n),GFP_ATOMIC);
		if (NULL != new_port){
			INIT_LIST_HEAD(&new_port->list);
			new_port->port = port;
			list_add(&new_port->list,hash);
		}
		else{
			printk("%s() %s:%d, kmalloc fails\n", __FUNCTION__, __FILE__, __LINE__);
		}
	}
}

void _reserve_port_clean_idx( int idx)
{
	
	struct list_head *ptr;
	struct port_n *entry;
	
	list_for_each(ptr, &g_reserve_port_list[idx]) 
	{
		entry = list_entry(ptr, struct port_n, list);
		list_del_init(&entry->list);
		kfree(entry);
	}
	
	INIT_LIST_HEAD(&g_reserve_port_list[idx]);
}

void reserve_port_clean( void )
{
	int idx;

	for( idx=0; idx < HASH_PORT_MAX ; idx++)
	{
		_reserve_port_clean_idx( idx);
	}
}


// Allen, reserve ct for 1~1024 port
int ct_reserve = 620; 					// reserve conntracks for 1 ~ 1024 port
static atomic_t ct_not_reserve_count = ATOMIC_INIT(0);  // ct counter for not port 1~1024
int ct_not_reserve_max =0;  // ct counter max

int is_reserve( int portnum)
{
	if( portnum <= 1024 )
		return 1;

	return reserve_port_search( portnum);
}

// restrict to conntrack count of each host
#define HOST_MAX 256	// 0 ~ ( HOST_MAX -1 )

struct nf_conntrack_host_table{
	atomic_t count;
//	int count_old;
	atomic_t count_tcp;
	atomic_t count_udp;
	
	u_int32_t src_addr;
	u_int32_t dst_addr;
	
};
struct nf_conntrack_host_table nf_conntrack_host[HOST_MAX];


static inline u_int32_t hash_conntrack_per_host( u_int32_t addr)
{
	unsigned char * paddr =(unsigned char *) &addr;
//	return  ( paddr[0] + paddr[1] + paddr[2] + paddr[3] ) % HOST_MAX;
	return  paddr[3];
}
#endif

static u_int32_t __hash_conntrack(const struct nf_conntrack_tuple *tuple,
				  unsigned int size, unsigned int rnd)
{
	unsigned int a, b;
	a = jhash((void *)tuple->src.u3.all, sizeof(tuple->src.u3.all),
		  ((tuple->src.l3num) << 16) | tuple->dst.protonum);
	b = jhash((void *)tuple->dst.u3.all, sizeof(tuple->dst.u3.all),
			(tuple->src.u.all << 16) | tuple->dst.u.all);

	return jhash_2words(a, b, rnd) % size;
}

static inline u_int32_t hash_conntrack(const struct nf_conntrack_tuple *tuple)
{
	return __hash_conntrack(tuple, nf_conntrack_htable_size,
				nf_conntrack_hash_rnd);
}

int nf_conntrack_register_cache(u_int32_t features, const char *name,
				size_t size)
{
	int ret = 0;
	char *cache_name;
	struct kmem_cache *cachep;

	DEBUGP("nf_conntrack_register_cache: features=0x%x, name=%s, size=%d\n",
	       features, name, size);

	if (features < NF_CT_F_BASIC || features >= NF_CT_F_NUM) {
		DEBUGP("nf_conntrack_register_cache: invalid features.: 0x%x\n",
			features);
		return -EINVAL;
	}

	mutex_lock(&nf_ct_cache_mutex);

	write_lock_bh(&nf_ct_cache_lock);
	/* e.g: multiple helpers are loaded */
	if (nf_ct_cache[features].use > 0) {
		DEBUGP("nf_conntrack_register_cache: already resisterd.\n");
		if ((!strncmp(nf_ct_cache[features].name, name,
			      NF_CT_FEATURES_NAMELEN))
		    && nf_ct_cache[features].size == size) {
			DEBUGP("nf_conntrack_register_cache: reusing.\n");
			nf_ct_cache[features].use++;
			ret = 0;
		} else
			ret = -EBUSY;

		write_unlock_bh(&nf_ct_cache_lock);
		mutex_unlock(&nf_ct_cache_mutex);
		return ret;
	}
	write_unlock_bh(&nf_ct_cache_lock);

	/*
	 * The memory space for name of slab cache must be alive until
	 * cache is destroyed.
	 */
	cache_name = kmalloc(sizeof(char)*NF_CT_FEATURES_NAMELEN, GFP_ATOMIC);
	if (cache_name == NULL) {
		DEBUGP("nf_conntrack_register_cache: can't alloc cache_name\n");
		ret = -ENOMEM;
		goto out_up_mutex;
	}

	if (strlcpy(cache_name, name, NF_CT_FEATURES_NAMELEN)
						>= NF_CT_FEATURES_NAMELEN) {
		printk("nf_conntrack_register_cache: name too long\n");
		ret = -EINVAL;
		goto out_free_name;
	}

	cachep = kmem_cache_create(cache_name, size, 0, 0,
				   NULL, NULL);
	if (!cachep) {
		printk("nf_conntrack_register_cache: Can't create slab cache "
		       "for the features = 0x%x\n", features);
		ret = -ENOMEM;
		goto out_free_name;
	}

	write_lock_bh(&nf_ct_cache_lock);
	nf_ct_cache[features].use = 1;
	nf_ct_cache[features].size = size;
	nf_ct_cache[features].cachep = cachep;
	nf_ct_cache[features].name = cache_name;
	write_unlock_bh(&nf_ct_cache_lock);

	goto out_up_mutex;

out_free_name:
	kfree(cache_name);
out_up_mutex:
	mutex_unlock(&nf_ct_cache_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(nf_conntrack_register_cache);

/* FIXME: In the current, only nf_conntrack_cleanup() can call this function. */
void nf_conntrack_unregister_cache(u_int32_t features)
{
	struct kmem_cache *cachep;
	char *name;

	/*
	 * This assures that kmem_cache_create() isn't called before destroying
	 * slab cache.
	 */
	DEBUGP("nf_conntrack_unregister_cache: 0x%04x\n", features);
	mutex_lock(&nf_ct_cache_mutex);

	write_lock_bh(&nf_ct_cache_lock);
	if (--nf_ct_cache[features].use > 0) {
		write_unlock_bh(&nf_ct_cache_lock);
		mutex_unlock(&nf_ct_cache_mutex);
		return;
	}
	cachep = nf_ct_cache[features].cachep;
	name = nf_ct_cache[features].name;
	nf_ct_cache[features].cachep = NULL;
	nf_ct_cache[features].name = NULL;
	nf_ct_cache[features].size = 0;
	write_unlock_bh(&nf_ct_cache_lock);

	synchronize_net();

	kmem_cache_destroy(cachep);
	kfree(name);

	mutex_unlock(&nf_ct_cache_mutex);
}
EXPORT_SYMBOL_GPL(nf_conntrack_unregister_cache);

int
nf_ct_get_tuple(const struct sk_buff *skb,
		unsigned int nhoff,
		unsigned int dataoff,
		u_int16_t l3num,
		u_int8_t protonum,
		struct nf_conntrack_tuple *tuple,
		const struct nf_conntrack_l3proto *l3proto,
		const struct nf_conntrack_l4proto *l4proto)
{
	NF_CT_TUPLE_U_BLANK(tuple);

	tuple->src.l3num = l3num;
	if (l3proto->pkt_to_tuple(skb, nhoff, tuple) == 0)
		return 0;

	tuple->dst.protonum = protonum;
	tuple->dst.dir = IP_CT_DIR_ORIGINAL;

	return l4proto->pkt_to_tuple(skb, dataoff, tuple);
}
EXPORT_SYMBOL_GPL(nf_ct_get_tuple);

int
nf_ct_invert_tuple(struct nf_conntrack_tuple *inverse,
		   const struct nf_conntrack_tuple *orig,
		   const struct nf_conntrack_l3proto *l3proto,
		   const struct nf_conntrack_l4proto *l4proto)
{
	NF_CT_TUPLE_U_BLANK(inverse);

	inverse->src.l3num = orig->src.l3num;
	if (l3proto->invert_tuple(inverse, orig) == 0)
		return 0;

	inverse->dst.dir = !orig->dst.dir;

	inverse->dst.protonum = orig->dst.protonum;
	return l4proto->invert_tuple(inverse, orig);
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuple);

static void
clean_from_lists(struct nf_conn *ct)
{
	DEBUGP("clean_from_lists(%p)\n", ct);
	list_del(&ct->tuplehash[IP_CT_DIR_ORIGINAL].list);
	list_del(&ct->tuplehash[IP_CT_DIR_REPLY].list);

	/* Destroy all pending expectations */
	nf_ct_remove_expectations(ct);
}

static void destroy_conntrack(struct nf_conntrack *nfct)
{
	struct nf_conn *ct = (struct nf_conn *)nfct;
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	typeof(nf_conntrack_destroyed) destroyed;
#if defined(CONFIG_MIPS_BRCM)
	struct nf_conn_help *help = nfct_help(ct);
#endif
#if defined(CONFIG_SERCOMM_CODE)
	u_int32_t idx = hash_conntrack_per_host( ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip );
	u_int16_t proto_num = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum;
#endif

#if defined(CONFIG_MIPS_BRCM)
#if defined(CONFIG_BLOG)
    DEBUGP("destroy_conntrack(%p) blog keys[0x%08x,0x%08x]\n",
           ct, ct->blog_key[IP_CT_DIR_ORIGINAL], ct->blog_key[IP_CT_DIR_REPLY]);

    clear_bit(IPS_BLOG_BIT, &ct->status);   /* Disable further blogging */
    if ( (ct->blog_key[IP_CT_DIR_ORIGINAL] != 0)
         || (ct->blog_key[IP_CT_DIR_REPLY] != 0) ) 
        blog_stop(NULL, ct);      /* Conntrack going away, stop associated flows */
#endif
	if (nf_ct_is_confirmed(ct) && help && help->helper && dynahelper_unref)
		dynahelper_unref(help->helper->me);
#else
    DEBUGP("destroy_conntrack(%p)\n", ct);
#endif
	
	NF_CT_ASSERT(atomic_read(&nfct->use) == 0);
	NF_CT_ASSERT(!timer_pending(&ct->timeout));
#if defined(CONFIG_SERCOMM_CODE)
	if( ct->reserve.mask != IPS_RESERVE ){
		atomic_dec(&ct_not_reserve_count);
	}
#endif

	nf_conntrack_event(IPCT_DESTROY, ct);
	set_bit(IPS_DYING_BIT, &ct->status);

	/* To make sure we don't get any weird locking issues here:
	 * destroy_conntrack() MUST NOT be called with a write lock
	 * to nf_conntrack_lock!!! -HW */
	rcu_read_lock();
	l3proto = __nf_ct_l3proto_find(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num);
	if (l3proto && l3proto->destroy)
		l3proto->destroy(ct);

	l4proto = __nf_ct_l4proto_find(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.l3num,
				       ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.protonum);
	if (l4proto && l4proto->destroy)
		l4proto->destroy(ct);

	destroyed = rcu_dereference(nf_conntrack_destroyed);
	if (destroyed)
		destroyed(ct);

	rcu_read_unlock();

	write_lock_bh(&nf_conntrack_lock);
	/* Expectations will have been removed in clean_from_lists,
	 * except TFTP can create an expectation on the first packet,
	 * before connection is in the list, so we need to clean here,
	 * too. */
	nf_ct_remove_expectations(ct);

	#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7) || defined(CONFIG_NETFILTER_XT_MATCH_LAYER7_MODULE)
	if(ct->layer7.app_proto)
		kfree(ct->layer7.app_proto);
	if(ct->layer7.app_data)
	kfree(ct->layer7.app_data);
	#endif


	/* We overload first tuple to link into unconfirmed list. */
	if (!nf_ct_is_confirmed(ct)) {
		BUG_ON(list_empty(&ct->tuplehash[IP_CT_DIR_ORIGINAL].list));
		list_del(&ct->tuplehash[IP_CT_DIR_ORIGINAL].list);
	}

	NF_CT_STAT_INC(delete);
#if defined(CONFIG_SERCOMM_CODE)
	#if defined(CONFIG_IP_NF_MATCH_LAYER7) || defined(CONFIG_IP_NF_MATCH_LAYER7_MODULE)
		/* This ought to get free'd somewhere.  How about here? */
		if(ct->layer7.app_proto) /* this is sufficient, right? */
			kfree(ct->layer7.app_proto);
		if(ct->layer7.app_data)
			kfree(ct->layer7.app_data);
	#endif
#endif

	write_unlock_bh(&nf_conntrack_lock);

	if (ct->master)
		nf_ct_put(ct->master);

	DEBUGP("destroy_conntrack: returning ct=%p to slab\n", ct);
	nf_conntrack_free(ct);
#if defined(CONFIG_SERCOMM_CODE)
	atomic_dec(&nf_conntrack_host[idx].count);
	if ( proto_num == IPPROTO_TCP)
		atomic_dec(&nf_conntrack_host[idx].count_tcp);
	else if ( proto_num == IPPROTO_UDP)
		atomic_dec(&nf_conntrack_host[idx].count_udp);
#endif

}

static void death_by_timeout(unsigned long ul_conntrack)
{
	struct nf_conn *ct = (void *)ul_conntrack;
	struct nf_conn_help *help = nfct_help(ct);

	if (help && help->helper && help->helper->destroy)
		help->helper->destroy(ct);

	write_lock_bh(&nf_conntrack_lock);
	/* Inside lock so preempt is disabled on module removal path.
	 * Otherwise we can get spurious warnings. */
	NF_CT_STAT_INC(delete_list);
	clean_from_lists(ct);
	write_unlock_bh(&nf_conntrack_lock);
	nf_ct_put(ct);
}

struct nf_conntrack_tuple_hash *
__nf_conntrack_find(const struct nf_conntrack_tuple *tuple,
		    const struct nf_conn *ignored_conntrack)
{
	struct nf_conntrack_tuple_hash *h;
	unsigned int hash = hash_conntrack(tuple);

	list_for_each_entry(h, &nf_conntrack_hash[hash], list) {
		if (nf_ct_tuplehash_to_ctrack(h) != ignored_conntrack &&
		    nf_ct_tuple_equal(tuple, &h->tuple)) {
			NF_CT_STAT_INC(found);
			return h;
		}
		NF_CT_STAT_INC(searched);
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_find);

/* Find a connection corresponding to a tuple. */
struct nf_conntrack_tuple_hash *
nf_conntrack_find_get(const struct nf_conntrack_tuple *tuple,
		      const struct nf_conn *ignored_conntrack)
{
	struct nf_conntrack_tuple_hash *h;

	read_lock_bh(&nf_conntrack_lock);
	h = __nf_conntrack_find(tuple, ignored_conntrack);
	if (h)
		atomic_inc(&nf_ct_tuplehash_to_ctrack(h)->ct_general.use);
	read_unlock_bh(&nf_conntrack_lock);

	return h;
}
EXPORT_SYMBOL_GPL(nf_conntrack_find_get);

static void __nf_conntrack_hash_insert(struct nf_conn *ct,
				       unsigned int hash,
				       unsigned int repl_hash)
{
	ct->id = ++nf_conntrack_next_id;
	list_add(&ct->tuplehash[IP_CT_DIR_ORIGINAL].list,
		 &nf_conntrack_hash[hash]);
	list_add(&ct->tuplehash[IP_CT_DIR_REPLY].list,
		 &nf_conntrack_hash[repl_hash]);
}

void nf_conntrack_hash_insert(struct nf_conn *ct)
{
	unsigned int hash, repl_hash;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	write_lock_bh(&nf_conntrack_lock);
	__nf_conntrack_hash_insert(ct, hash, repl_hash);
	write_unlock_bh(&nf_conntrack_lock);
}
EXPORT_SYMBOL_GPL(nf_conntrack_hash_insert);

/* Confirm a connection given skb; places it in hash table */
int
__nf_conntrack_confirm(struct sk_buff **pskb)
{
	unsigned int hash, repl_hash;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;
	struct nf_conn_help *help;
	enum ip_conntrack_info ctinfo;

	ct = nf_ct_get(*pskb, &ctinfo);

	/* ipt_REJECT uses nf_conntrack_attach to attach related
	   ICMP/TCP RST packets in other direction.  Actual packet
	   which created connection will be IP_CT_NEW or for an
	   expected connection, IP_CT_RELATED. */
	if (CTINFO2DIR(ctinfo) != IP_CT_DIR_ORIGINAL)
		return NF_ACCEPT;

	hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	repl_hash = hash_conntrack(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

	/* We're not in hash table, and we refuse to set up related
	   connections for unconfirmed conns.  But packet copies and
	   REJECT will give spurious warnings here. */
	/* NF_CT_ASSERT(atomic_read(&ct->ct_general.use) == 1); */

	/* No external references means noone else could have
	   confirmed us. */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));
	DEBUGP("Confirming conntrack %p\n", ct);

	write_lock_bh(&nf_conntrack_lock);

	/* See if there's one in the list already, including reverse:
	   NAT could have grabbed it without realizing, since we're
	   not in the hash.  If there is, we lost race. */
	list_for_each_entry(h, &nf_conntrack_hash[hash], list)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
				      &h->tuple))
			goto out;
	list_for_each_entry(h, &nf_conntrack_hash[repl_hash], list)
		if (nf_ct_tuple_equal(&ct->tuplehash[IP_CT_DIR_REPLY].tuple,
				      &h->tuple))
			goto out;

	/* Remove from unconfirmed list */
	list_del(&ct->tuplehash[IP_CT_DIR_ORIGINAL].list);

	__nf_conntrack_hash_insert(ct, hash, repl_hash);
	/* Timer relative to confirmation time, not original
	   setting time, otherwise we'd get timer wrap in
	   weird delay cases. */
	ct->timeout.expires += jiffies;
	add_timer(&ct->timeout);
	atomic_inc(&ct->ct_general.use);
	set_bit(IPS_CONFIRMED_BIT, &ct->status);
	NF_CT_STAT_INC(insert);
	write_unlock_bh(&nf_conntrack_lock);
	help = nfct_help(ct);
#if defined(CONFIG_MIPS_BRCM)
	if (help && help->helper) {
		if (dynahelper_ref)
			dynahelper_ref(help->helper->me);
		nf_conntrack_event_cache(IPCT_HELPER, *pskb);
	}
#endif /* CONFIG_MIPS_BRCM */
#ifdef CONFIG_NF_NAT_NEEDED
	if (test_bit(IPS_SRC_NAT_DONE_BIT, &ct->status) ||
	    test_bit(IPS_DST_NAT_DONE_BIT, &ct->status))
		nf_conntrack_event_cache(IPCT_NATINFO, *pskb);
#endif
	nf_conntrack_event_cache(master_ct(ct) ?
				 IPCT_RELATED : IPCT_NEW, *pskb);
	return NF_ACCEPT;

out:
	NF_CT_STAT_INC(insert_failed);
	write_unlock_bh(&nf_conntrack_lock);
	return NF_DROP;
}
EXPORT_SYMBOL_GPL(__nf_conntrack_confirm);

/* Returns true if a connection correspondings to the tuple (required
   for NAT). */
int
nf_conntrack_tuple_taken(const struct nf_conntrack_tuple *tuple,
			 const struct nf_conn *ignored_conntrack)
{
	struct nf_conntrack_tuple_hash *h;

	read_lock_bh(&nf_conntrack_lock);
	h = __nf_conntrack_find(tuple, ignored_conntrack);
	read_unlock_bh(&nf_conntrack_lock);

	return h != NULL;
}
EXPORT_SYMBOL_GPL(nf_conntrack_tuple_taken);

/* There's a small race here where we may free a just-assured
   connection.  Too bad: we're in trouble anyway. */
static int early_drop(struct list_head *chain)
{
	/* Traverse backwards: gives us oldest, which is roughly LRU */
	struct nf_conntrack_tuple_hash *h=NULL;
	struct nf_conn *ct = NULL, *tmp=NULL;
	int dropped = 0;

	read_lock_bh(&nf_conntrack_lock);
	list_for_each_entry_reverse(h, chain, list) {
		tmp = nf_ct_tuplehash_to_ctrack(h);
		if (!test_bit(IPS_ASSURED_BIT, &tmp->status)) {
			ct = tmp;
			atomic_inc(&ct->ct_general.use);
			break;
		}
	}
	read_unlock_bh(&nf_conntrack_lock);

	if (!ct)
		return dropped;

	if (del_timer(&ct->timeout)) {
		death_by_timeout((unsigned long)ct);
		dropped = 1;
		NF_CT_STAT_INC_ATOMIC(early_drop);
	}
	nf_ct_put(ct);
	return dropped;
}

#if defined(CONFIG_SERCOMM_CODE)
#define	DROP_UNREPLIED		1
#define	DROP_FORCE			2
#define	DROP_NOT_RESERVED	3

static int drop_packet_with_flag(struct list_head *chain, int drop_flag)
{
	/* Traverse backwards: gives us oldest, which is roughly LRU */
	struct nf_conntrack_tuple_hash *h=NULL;
	int dropped = 0;
	struct nf_conn *ct = NULL, *tmp=NULL;

	read_lock_bh(&nf_conntrack_lock);
	list_for_each_entry_reverse(h, chain, list) {
		tmp = nf_ct_tuplehash_to_ctrack(h);
		//printk("%s: %d\n", __FUNCTION__, __LINE__);
		if(drop_flag==DROP_UNREPLIED){
			if (tmp->reserve.mask!=IPS_RESERVE && !test_bit(IPS_ASSURED_BIT, &tmp->status)) {
				ct = tmp;
				atomic_inc(&ct->ct_general.use);
				break;
			}			
		}
		else if(drop_flag==DROP_FORCE){
			ct = tmp;
			atomic_inc(&ct->ct_general.use);
			break;
		}
		else if(drop_flag==DROP_NOT_RESERVED){
			//printk("tmp->reserve.mask: %x\n", tmp->reserve.mask);
			//printk("IPS_RESERVE: %x\n", IPS_RESERVE);
			if (tmp->reserve.mask!=IPS_RESERVE) {
				ct = tmp;
				atomic_inc(&ct->ct_general.use);
				break;
			}			
		}		
	}	
	read_unlock_bh(&nf_conntrack_lock);
	if (!ct){
		return dropped;
	}
	if (del_timer(&ct->timeout)) {
		//printk("death_by_timeout: %d\n", drop_flag);
		death_by_timeout((unsigned long)ct);
		dropped = 1;
		NF_CT_STAT_INC_ATOMIC(early_drop);
	}
	nf_ct_put(ct);
	return dropped;
}
#endif

static struct nf_conn *
__nf_conntrack_alloc(const struct nf_conntrack_tuple *orig,
		     const struct nf_conntrack_tuple *repl,
		     const struct nf_conntrack_l3proto *l3proto,
		     u_int32_t features)
{
	struct nf_conn *conntrack = NULL;
	struct nf_conntrack_helper *helper;
	unsigned int hash = 0;
#if defined(CONFIG_SERCOMM_CODE)
	u_int32_t host_hash=0;
	static unsigned int drop_next;
	int count_all=0;
	u_int16_t proto_num=0;	// protocol number
	u_int16_t port_num=0;	// TCP or UDP port number	
#endif
		
	if (unlikely(!nf_conntrack_hash_rnd_initted)) {
		get_random_bytes(&nf_conntrack_hash_rnd, 4);
		nf_conntrack_hash_rnd_initted = 1;
	}

	/* We don't want any race condition at early drop stage */
	hash = hash_conntrack(orig);
#if defined(CONFIG_SERCOMM_CODE)
	proto_num = orig->dst.protonum;
	switch ( proto_num ) {
		case IPPROTO_TCP:
			port_num =ntohs( orig->dst.u.tcp.port);
			break;
		case IPPROTO_UDP:
			port_num =ntohs( orig->dst.u.udp.port);		
			break;
		default:	// other protocol -> add to reserve area
			port_num =1;
	}
	if(is_reserve(port_num)==0){//Not Reserverd Port
    	switch ( proto_num ) {
	    	case IPPROTO_TCP:
	    		port_num =ntohs( orig->src.u.tcp.port);
	    		break;
	    	case IPPROTO_UDP:
	    		port_num =ntohs( orig->src.u.udp.port);
	    		break;
	    	default:	// other protocol -> add to reserve area
	    		port_num =1;
    	}
    }

	host_hash =hash_conntrack_per_host( orig->src.u3.ip);
	//printk("port_num: %d\n", port_num);
	//printk("host_hash: %d\n", host_hash);
	//printk("nf_conntrack_count: %u\n", atomic_read(&nf_conntrack_count));
	//printk("nf_conntrack_max: %u\n", nf_conntrack_max);	
	//Total Connections >= Maximum Number
	if (nf_conntrack_max && atomic_read(&nf_conntrack_count) >= nf_conntrack_max) {
		/* Try dropping from random chain, or else from the
		   chain about to put into (in case they're trying to
		   bomb one hash chain). */
        if(is_reserve( port_num) == 0){//Not Reserved Port
			//printk("%s: It seems BT session, drop it.\n", __FUNCTION__);
            if (net_ratelimit())
				printk(KERN_WARNING "nf_conntrack: table full, dropping packet. Line: %d\n", __LINE__);
	        return ERR_PTR(-ENOMEM);
	    }

	    //Connections of Not Reserved Ports > 70%
		if(atomic_read(&ct_not_reserve_count)>( nf_conntrack_max*7/10 )) {
			unsigned int next = (drop_next++)%nf_conntrack_htable_size;
			
			//printk("%s: %d\n", __FUNCTION__, __LINE__);
	    	//printk("ct_not_reserve_count: %u\n", ct_not_reserve_count);
			//Try to drop one session of non-reserved port and without IPS_ASSURED_BIT.
			if (!drop_packet_with_flag(&nf_conntrack_hash[next], DROP_UNREPLIED) && !drop_packet_with_flag(&nf_conntrack_hash[hash], DROP_UNREPLIED)) {
				int exit_idx=next;
				
				//printk("%s: %d\n", __FUNCTION__, __LINE__);
				//printk("next: %d\n", next);
				//Try to drop one session of non-reserved port.
				while (!drop_packet_with_flag(&nf_conntrack_hash[next], DROP_NOT_RESERVED)){
					next = (drop_next++)%nf_conntrack_htable_size;
					if(exit_idx==next){
						if (net_ratelimit())
							printk(KERN_WARNING "nf_conntrack: table full, dropping packet. Line: %d\n", __LINE__);
							return ERR_PTR(-ENOMEM);
					}
				}
			}
			//printk("%s: %d\n", __FUNCTION__, __LINE__);
		}
		else {		
			unsigned int next = (drop_next++)%nf_conntrack_htable_size;
				
			//Try to drop one session without IPS_ASSURED_BIT.
			//printk("%s: %d\n", __FUNCTION__, __LINE__);
			//printk("ct_not_reserve_count: %u\n", ct_not_reserve_count);
			//printk("next: %d\n", next);
			//printk("hash: %d\n", hash);
			if (!early_drop(&nf_conntrack_hash[next]) && !early_drop(&nf_conntrack_hash[hash])) {
				int exit_idx=next;
					
				//printk("%s: %d\n", __FUNCTION__, __LINE__);
				while (!drop_packet_with_flag(&nf_conntrack_hash[next], DROP_FORCE)){
					next = (drop_next++)%nf_conntrack_htable_size;
					if(exit_idx == next){
						if (net_ratelimit())
							printk(KERN_WARNING "nf_conntrack: table full, dropping packet. Line: %d\n", __LINE__);
						return ERR_PTR(-ENOMEM);
					}
				}
			}
		}
		//printk("%s: %d\n", __FUNCTION__, __LINE__);
	}
#else
	if (nf_conntrack_max && atomic_read(&nf_conntrack_count) > nf_conntrack_max) {
		/* Try dropping from this hash chain. */
		if (!early_drop(&nf_conntrack_hash[hash])) {
			if (net_ratelimit())
				printk(KERN_WARNING "nf_conntrack: table full, dropping packet. Line: %d\n", __LINE__);
			return ERR_PTR(-ENOMEM);
		}
	}
#endif

	/*  find features needed by this conntrack. */
	features |= l3proto->get_features(orig);

	/* FIXME: protect helper list per RCU */
	read_lock_bh(&nf_conntrack_lock);
	helper = __nf_ct_helper_find(repl);
	/* NAT might want to assign a helper later */
	if (helper || features & NF_CT_F_NAT)
		features |= NF_CT_F_HELP;
	read_unlock_bh(&nf_conntrack_lock);

	DEBUGP("nf_conntrack_alloc: features=0x%x\n", features);

	read_lock_bh(&nf_ct_cache_lock);

	if (unlikely(!nf_ct_cache[features].use)) {
		DEBUGP("nf_conntrack_alloc: not supported features = 0x%x\n",
			features);
		goto out;
	}

	conntrack = kmem_cache_alloc(nf_ct_cache[features].cachep, GFP_ATOMIC);
	if (conntrack == NULL) {
		DEBUGP("nf_conntrack_alloc: Can't alloc conntrack from cache\n");
		goto out;
	}

	memset(conntrack, 0, nf_ct_cache[features].size);
	conntrack->features = features;
	atomic_set(&conntrack->ct_general.use, 1);
	conntrack->ct_general.destroy = destroy_conntrack;
#if defined(CONFIG_MIPS_BRCM) && defined(CONFIG_BLOG)
    DEBUGP("nf_conntrack_alloc: ct<%p> BLOGible\n", conntrack );
    set_bit(IPS_BLOG_BIT, &conntrack->status);  /* Enable conntrack blogging */
#endif
	conntrack->tuplehash[IP_CT_DIR_ORIGINAL].tuple = *orig;
	conntrack->tuplehash[IP_CT_DIR_REPLY].tuple = *repl;
	/* Don't set timer yet: wait for confirmation */
	init_timer(&conntrack->timeout);
	conntrack->timeout.data = (unsigned long)conntrack;
	conntrack->timeout.function = death_by_timeout;
#if defined(CONFIG_MIPS_BRCM)
	/* bugfix for lost connections */
	list_add(&conntrack->safe_list, &safe_list);
#endif
	atomic_inc(&nf_conntrack_count);
#if defined(CONFIG_SERCOMM_CODE)
	if ( is_reserve( port_num) == 0 ){	// Not a reserveD port
		int count=0;
		
		atomic_inc(&ct_not_reserve_count);
		count = atomic_read(&ct_not_reserve_count);
		if ( count > ct_not_reserve_max)
			ct_not_reserve_max = count;
	}
	else
		conntrack->reserve.mask = IPS_RESERVE;

	if ( proto_num == IPPROTO_TCP)
		atomic_inc(&nf_conntrack_host[host_hash].count_tcp);
	else if ( proto_num == IPPROTO_UDP)
		atomic_inc(&nf_conntrack_host[host_hash].count_udp);
	
	nf_conntrack_host[host_hash ].src_addr= orig->src.u3.ip;
	nf_conntrack_host[host_hash ].dst_addr= orig->dst.u3.ip;
	count_all = atomic_read(&nf_conntrack_count);
	if( count_all > nf_conntrack_count_max_record )
		nf_conntrack_count_max_record = count_all;
#endif

	read_unlock_bh(&nf_ct_cache_lock);

	return conntrack;
out:
	read_unlock_bh(&nf_ct_cache_lock);
	//atomic_dec(&nf_conntrack_count);
	return conntrack;
}

struct nf_conn *nf_conntrack_alloc(const struct nf_conntrack_tuple *orig,
				   const struct nf_conntrack_tuple *repl)
{
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conn *ct;

	rcu_read_lock();
	l3proto = __nf_ct_l3proto_find(orig->src.l3num);
	ct = __nf_conntrack_alloc(orig, repl, l3proto, 0);
	rcu_read_unlock();

	return ct;
}
EXPORT_SYMBOL_GPL(nf_conntrack_alloc);

void nf_conntrack_free(struct nf_conn *conntrack)
{
	u_int32_t features = conntrack->features;
#if defined(CONFIG_MIPS_BRCM)
	/* bugfix for lost connections */
	list_del(&conntrack->safe_list);
#endif
	NF_CT_ASSERT(features >= NF_CT_F_BASIC && features < NF_CT_F_NUM);
	DEBUGP("nf_conntrack_free: features = 0x%x, conntrack=%p\n", features,
	       conntrack);
	kmem_cache_free(nf_ct_cache[features].cachep, conntrack);
	atomic_dec(&nf_conntrack_count);
}
EXPORT_SYMBOL_GPL(nf_conntrack_free);

/* Allocate a new conntrack: we return -ENOMEM if classification
   failed due to stress.  Otherwise it really is unclassifiable. */
static struct nf_conntrack_tuple_hash *
init_conntrack(const struct nf_conntrack_tuple *tuple,
	       struct nf_conntrack_l3proto *l3proto,
	       struct nf_conntrack_l4proto *l4proto,
	       struct sk_buff *skb,
	       unsigned int dataoff)
{
	struct nf_conn *conntrack;
	struct nf_conntrack_tuple repl_tuple;
	struct nf_conntrack_expect *exp;
	u_int32_t features = 0;
	
	if (!nf_ct_invert_tuple(&repl_tuple, tuple, l3proto, l4proto)) {
		DEBUGP("Can't invert tuple.\n");
		return NULL;
	}

	read_lock_bh(&nf_conntrack_lock);
	exp = __nf_conntrack_expect_find(tuple);
	if (exp && exp->helper)
		features = NF_CT_F_HELP;
	read_unlock_bh(&nf_conntrack_lock);

	conntrack = __nf_conntrack_alloc(tuple, &repl_tuple, l3proto, features);
	if (conntrack == NULL || IS_ERR(conntrack)) {
		DEBUGP("Can't allocate conntrack.\n");
		return (struct nf_conntrack_tuple_hash *)conntrack;
	}

	if (!l4proto->new(conntrack, skb, dataoff)) {
		nf_conntrack_free(conntrack);
		DEBUGP("init conntrack: can't track with proto module\n");
		return NULL;
	}

	write_lock_bh(&nf_conntrack_lock);
	exp = find_expectation(tuple);

	if (exp) {
		DEBUGP("conntrack: expectation arrives ct=%p exp=%p\n",
			conntrack, exp);
		/* Welcome, Mr. Bond.  We've been expecting you... */
		__set_bit(IPS_EXPECTED_BIT, &conntrack->status);
		conntrack->master = exp->master;
		if (exp->helper)
			nfct_help(conntrack)->helper = exp->helper;
#ifdef CONFIG_NF_CONNTRACK_MARK
		conntrack->mark = exp->master->mark;
#endif
#ifdef CONFIG_NF_CONNTRACK_SECMARK
		conntrack->secmark = exp->master->secmark;
#endif
		nf_conntrack_get(&conntrack->master->ct_general);
		NF_CT_STAT_INC(expect_new);
	} else {
		struct nf_conn_help *help = nfct_help(conntrack);

		if (help)
			help->helper = __nf_ct_helper_find(&repl_tuple);
		NF_CT_STAT_INC(new);
	}

	/* Overload tuple linked list to put us in unconfirmed list. */
	list_add(&conntrack->tuplehash[IP_CT_DIR_ORIGINAL].list, &unconfirmed);
	write_unlock_bh(&nf_conntrack_lock);

	if (exp) {
		if (exp->expectfn)
			exp->expectfn(conntrack, exp);
		nf_conntrack_expect_put(exp);
	}

	return &conntrack->tuplehash[IP_CT_DIR_ORIGINAL];
}

#ifdef YAHOO_ODM
static __inline__ u16 tcp_v4_check(struct tcphdr *th, int len,
		unsigned long saddr, unsigned long daddr,
		unsigned long base)
{
	return csum_tcpudp_magic(saddr,daddr,len,IPPROTO_TCP,base);
}

static inline int ip_route_output(struct rtable **rp,u32 daddr, u32 saddr, u32 tos, int oif)
{
	struct flowi fl = { .nl_u = { .ip4_u =
		{ .daddr = daddr,
			.saddr = saddr,
			.tos = tos } } };

	return ip_route_output_key(rp, &fl);
}

static void ip_direct_send(struct sk_buff *skb)
{
	struct dst_entry *dst = skb->dst;
	struct hh_cache *hh = dst->hh;

	if (hh) {
		int hh_alen;

		read_lock_bh(&hh->hh_lock);
		hh_alen = HH_DATA_ALIGN(hh->hh_len);
		memcpy(skb->data - hh_alen, hh->hh_data, hh_alen);
		read_unlock_bh(&hh->hh_lock);
		skb_push(skb, hh->hh_len);
		hh->hh_output(skb);
	} else if (dst->neighbour)
	{
		dst->neighbour->output(skb);
	}
	else {
		//printk(KERN_DEBUG "khm in MIRROR\n");
		kfree_skb(skb);
	}
}

#endif

/* On success, returns conntrack ptr, sets skb->nfct and ctinfo */
static inline struct nf_conn *
resolve_normal_ct(struct sk_buff *skb,
		  unsigned int dataoff,
		  u_int16_t l3num,
		  u_int8_t protonum,
		  struct nf_conntrack_l3proto *l3proto,
		  struct nf_conntrack_l4proto *l4proto,
		  int *set_reply,
		  enum ip_conntrack_info *ctinfo)
{
	struct nf_conntrack_tuple tuple;
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;

	if (!nf_ct_get_tuple(skb, (unsigned int)(skb->nh.raw - skb->data),
			     dataoff, l3num, protonum, &tuple, l3proto,
			     l4proto)) {
		DEBUGP("resolve_normal_ct: skb<%p> Can't get tuple\n", skb);
		return NULL;
	}

	/* look for tuple match */
	h = nf_conntrack_find_get(&tuple, NULL);
	if (!h) {
		h = init_conntrack(&tuple, l3proto, l4proto, skb, dataoff);
		if (!h)
			return NULL;
		if (IS_ERR(h))
			return (void *)h;
	}
	ct = nf_ct_tuplehash_to_ctrack(h);

	/* It exists; we have (non-exclusive) reference. */
	if (NF_CT_DIRECTION(h) == IP_CT_DIR_REPLY) {
		*ctinfo = IP_CT_ESTABLISHED + IP_CT_IS_REPLY;
		/* Please set reply bit if this packet OK */
		*set_reply = 1;
	} else {
		/* Once we've had two way comms, always ESTABLISHED. */
		if (test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
			DEBUGP("nf_conntrack_in: normal packet<%p> for ct<%p>\n", skb, ct);
			*ctinfo = IP_CT_ESTABLISHED;
		} else if (test_bit(IPS_EXPECTED_BIT, &ct->status)) {
			DEBUGP("nf_conntrack_in: related packet<%p> for ct<%p>\n", skb, ct);
			*ctinfo = IP_CT_RELATED;
		} else {
			DEBUGP("nf_conntrack_in: new packet<%p> for ct<%p>\n", skb, ct);
			*ctinfo = IP_CT_NEW;
		}
		*set_reply = 0;
	}
	skb->nfct = &ct->ct_general;
	skb->nfctinfo = *ctinfo;

#if defined(CONFIG_MIPS_BRCM) && defined(CONFIG_BLOG)
    {
        struct nf_conn_help * help = nfct_help(ct);

        if ( (help != (struct nf_conn_help *)NULL) &&
             (help->helper != (struct nf_conntrack_helper *)NULL) )
        {
            DEBUGP("nf_conntrack_in: skb<%p> ct<%p> helper<%s> found\n",
                   skb, ct, help->helper->name);
            clear_bit(IPS_BLOG_BIT, &ct->status);
        }
        if (test_bit(IPS_BLOG_BIT, &ct->status))    /* OK to blog ? */
        {
            DEBUGP("nf_conntrack_in: skb<%p> blog<%p> ct<%p>\n",
                    skb, skb->blog_p, ct );
            blog_nfct(skb, ct);                     /* Blog conntrack */
        }
        else
        {
            DEBUGP("nf_conntrack_in: skb<%p> ct<%p> NOT BLOGible<%p>\n",
                    skb, ct, skb->blog_p );
            blog_free(skb);                         /* No blogging */
        }
    }
#endif

	return ct;
}

unsigned int
nf_conntrack_in(int pf, unsigned int hooknum, struct sk_buff **pskb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_conntrack_l3proto *l3proto;
	struct nf_conntrack_l4proto *l4proto;
	unsigned int dataoff;
	u_int8_t protonum;
	int set_reply = 0;
	int ret;

	/* Previously seen (loopback or untracked)?  Ignore. */
	if ((*pskb)->nfct) {
		NF_CT_STAT_INC_ATOMIC(ignore);
		return NF_ACCEPT;
	}

	/* rcu_read_lock()ed by nf_hook_slow */
	l3proto = __nf_ct_l3proto_find((u_int16_t)pf);

	if ((ret = l3proto->prepare(pskb, hooknum, &dataoff, &protonum)) <= 0) {
		DEBUGP("not prepared to track yet or error occured\n");
		return -ret;
	}

	l4proto = __nf_ct_l4proto_find((u_int16_t)pf, protonum);

	/* It may be an special packet, error, unclean...
	 * inverse of the return code tells to the netfilter
	 * core what to do with the packet. */
	if (l4proto->error != NULL &&
	    (ret = l4proto->error(*pskb, dataoff, &ctinfo, pf, hooknum)) <= 0) {
		NF_CT_STAT_INC_ATOMIC(error);
		NF_CT_STAT_INC_ATOMIC(invalid);
		return -ret;
	}

	ct = resolve_normal_ct(*pskb, dataoff, pf, protonum, l3proto, l4proto,
			       &set_reply, &ctinfo);
	if (!ct) {
		/* Not valid part of a connection */
		NF_CT_STAT_INC_ATOMIC(invalid);
		return NF_ACCEPT;
	}

	if (IS_ERR(ct)) {
		/* Too stressed to deal. */
		NF_CT_STAT_INC_ATOMIC(drop);
		return NF_DROP;
	}
	
#ifdef YAHOO_ODM
	if(ct->yahoo_flag)
	{
		struct sk_buff *nskb;
		struct tcphdr *tcph;
		struct sk_buff *ackskb;
		__u16 tmp;
		__u32 t;
		__u32 offset;
		__u32 tmp_ip;
		int send = 0;
		struct rtable *rt;
		struct net_device *dev;
		int blen;
		nskb = *pskb;

		tcph = (struct tcphdr *)((u_int32_t*)nskb->nh.iph + nskb->nh.iph->ihl);

		if (CTINFO2DIR(ctinfo) == IP_CT_DIR_ORIGINAL )
		{
			if(tcph->fin && tcph->ack )
			{
				ackskb = skb_copy(nskb, GFP_ATOMIC);
				if (!ackskb)
				{
					printk("<0>[ ERROR ]IP_CT_DIR_ORIGINAL MM ERROR\n");
					return NF_DROP;
				}
				tcph = (struct tcphdr *)((u_int32_t*)ackskb->nh.iph + ackskb->nh.iph->ihl);
				tmp_ip = ackskb->nh.iph->saddr;
				ackskb->nh.iph->saddr = ackskb->nh.iph->daddr;
				ackskb->nh.iph->daddr = tmp_ip;

				tmp = tcph->source;
				tcph->source = tcph->dest;
				tcph->dest = tmp;

				t = tcph->seq;
				tcph->seq = tcph->ack_seq;
				tcph->ack_seq = htonl(ntohl(t) + 1);

				blen = ackskb->len - ackskb->nh.iph->ihl*4;

				tcph->check = 0;
				tcph->check = tcp_v4_check(tcph, blen, ackskb->nh.iph->saddr, ackskb->nh.iph->daddr,
						csum_partial((char *)tcph, blen, 0));

				/* Adjust IP checksum */
				ackskb->nh.iph->check = 0;
				ackskb->nh.iph->check = ip_fast_csum((unsigned char *)ackskb->nh.iph,
						ackskb->nh.iph->ihl);
				if (ip_route_output(&rt, ackskb->nh.iph->daddr,0,
							RT_TOS(ackskb->nh.iph->tos) | RTO_CONN,
							0) != 0)

				{
					printk("<0>[ ERROR ] ip_route_output ERROR \n");
					return NF_DROP;
				}
				dst_release(ackskb->dst);
				ackskb->dst = &rt->u.dst;
				ip_direct_send(ackskb);
				if( ct->yahoo_flag & 0x04 )
				{
					ct->yahoo_flag = 0;
				}
				else
				{
					ct->yahoo_flag |= 0x02;
				}
				return NF_DROP;
			}
			else
				if(tcph->ack)
				{
					return NF_DROP;
					/*
					   if(ct->ack_seq == 0)
					   {
					   printk("<0>[ WARNNING ]IP_CT_DIR_ORIGINAL  someone send it, DROP it\n");
					   return NF_DROP;
					   }
					   else
					   {
					   tcph->ack_seq = ct->ack_seq;
					   ct->ack_seq == 0;
					   }
					   */
				}
		}
		else
		{
			ackskb = skb_copy(nskb, GFP_ATOMIC);
			if (!ackskb)
			{
				return NF_DROP;
			}
			tcph = (struct tcphdr *)((u_int32_t*)ackskb->nh.iph + ackskb->nh.iph->ihl);

			if(tcph->syn)
			{
				kfree_skb(nskb);
				return NF_DROP;
			}

			tmp_ip = ackskb->nh.iph->saddr;
			ackskb->nh.iph->saddr = ackskb->nh.iph->daddr;
			ackskb->nh.iph->daddr = tmp_ip;

			tmp = tcph->source;
			tcph->source = tcph->dest;
			tcph->dest = tmp;

			offset = ntohs(nskb->nh.iph->tot_len) - tcph->doff*4 - nskb->nh.iph->ihl*4;

			if(tcph->fin && tcph->ack )
			{
				if( ct->yahoo_flag & 0x02 )
				{
					ct->yahoo_flag = 0;
				}
				else
				{
					ct->yahoo_flag |= 0x04;
				}
				t = tcph->seq;
				tcph->seq = tcph->ack_seq;
				tcph->ack_seq = htonl(ntohl(t) + 1);
				send = 1;
			}
			else
				if(tcph->ack)
				{
					if(offset > 0)
					{
						t = tcph->seq;
						tcph->rst = 0;
						tcph->psh = 0;
						tcph->urg = 0;
						tcph->ece = 0;
						tcph->cwr = 0;
						tcph->syn = 0;
						tcph->fin = 0;

						tcph->seq = tcph->ack_seq;
						tcph->ack_seq = htonl(ntohl(t) + offset);
						skb_trim(ackskb, ackskb->nh.iph->ihl*4 + tcph->doff*4);
						ackskb->nh.iph->tot_len = htons(ackskb->nh.iph->ihl*4 + tcph->doff*4);
						send = 1;
					}
				}
			if(send == 1)
			{
				blen = ackskb->len - ackskb->nh.iph->ihl*4;

				tcph->check = 0;
				tcph->check = tcp_v4_check(tcph, blen, ackskb->nh.iph->saddr, ackskb->nh.iph->daddr,
						csum_partial((char *)tcph, blen, 0));

				/* Adjust IP checksum */
				ackskb->nh.iph->check = 0;
				ackskb->nh.iph->check = ip_fast_csum((unsigned char *)ackskb->nh.iph,
						ackskb->nh.iph->ihl);
				if (ip_route_output(&rt, ackskb->nh.iph->daddr,0,
							RT_TOS(ackskb->nh.iph->tos) | RTO_CONN,
							0) != 0)

				{
					printk("<0>[ ERROR ] ip_route_output ERROR \n");
					return NF_DROP;
				}
				dst_release(ackskb->dst);
				ackskb->dst = &rt->u.dst;
				/*
				   if((dev = dev_get_by_name("nas0")) == NULL)
				   {
				   printk("<0>[ ERROR ] dev_get_by_name ERROR \n");
				   return NF_DROP;
				   }
				   nskb->dev = dev;
				   */
				ip_direct_send(ackskb);

				ct->ack_seq = 0;
			}
			else
			{
				kfree_skb(ackskb);
			}

			return NF_DROP;
		}
		blen = nskb->len - nskb->nh.iph->ihl*4;
		tcph->check = 0;
		tcph->check = tcp_v4_check(tcph, blen, nskb->nh.iph->saddr, nskb->nh.iph->daddr,
				csum_partial((char *)tcph, blen, 0));

		/* Adjust IP checksum */
		nskb->nh.iph->check = 0;
		nskb->nh.iph->check = ip_fast_csum((unsigned char *)nskb->nh.iph,
				nskb->nh.iph->ihl);

	}
skip:
#endif

	NF_CT_ASSERT((*pskb)->nfct);

	ret = l4proto->packet(ct, *pskb, dataoff, ctinfo, pf, hooknum);
	if (ret < 0) {
		/* Invalid: inverse of the return code tells
		 * the netfilter core what to do */
		DEBUGP("nf_conntrack_in: ct<%p> Can't track with proto module\n", ct);
		nf_conntrack_put((*pskb)->nfct);
		(*pskb)->nfct = NULL;
		NF_CT_STAT_INC_ATOMIC(invalid);
		return -ret;
	}

	if (set_reply && !test_and_set_bit(IPS_SEEN_REPLY_BIT, &ct->status))
		nf_conntrack_event_cache(IPCT_STATUS, *pskb);

	return ret;
}
EXPORT_SYMBOL_GPL(nf_conntrack_in);

int nf_ct_invert_tuplepr(struct nf_conntrack_tuple *inverse,
			 const struct nf_conntrack_tuple *orig)
{
	int ret;

	rcu_read_lock();
	ret = nf_ct_invert_tuple(inverse, orig,
				 __nf_ct_l3proto_find(orig->src.l3num),
				 __nf_ct_l4proto_find(orig->src.l3num,
						      orig->dst.protonum));
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL_GPL(nf_ct_invert_tuplepr);

/* Alter reply tuple (maybe alter helper).  This is for NAT, and is
   implicitly racy: see __nf_conntrack_confirm */
void nf_conntrack_alter_reply(struct nf_conn *ct,
			      const struct nf_conntrack_tuple *newreply)
{
	struct nf_conn_help *help = nfct_help(ct);

	write_lock_bh(&nf_conntrack_lock);
	/* Should be unconfirmed, so not in hash table yet */
	NF_CT_ASSERT(!nf_ct_is_confirmed(ct));

	DEBUGP("Altering reply tuple of %p to ", ct);
	NF_CT_DUMP_TUPLE(newreply);

	ct->tuplehash[IP_CT_DIR_REPLY].tuple = *newreply;
	if (!ct->master && help && help->expecting == 0)
		help->helper = __nf_ct_helper_find(newreply);
	write_unlock_bh(&nf_conntrack_lock);
}
EXPORT_SYMBOL_GPL(nf_conntrack_alter_reply);

/* Refresh conntrack for this many jiffies and do accounting if do_acct is 1 */
void __nf_ct_refresh_acct(struct nf_conn *ct,
			  enum ip_conntrack_info ctinfo,
			  const struct sk_buff *skb,
			  unsigned long extra_jiffies,
			  int do_acct)
{
	int event = 0;

	NF_CT_ASSERT(ct->timeout.data == (unsigned long)ct);
	NF_CT_ASSERT(skb);

	write_lock_bh(&nf_conntrack_lock);

	/* Only update if this is not a fixed timeout */
	if (test_bit(IPS_FIXED_TIMEOUT_BIT, &ct->status)) {
		write_unlock_bh(&nf_conntrack_lock);
		return;
	}

	/* If not in hash table, timer will not be active yet */
	if (!nf_ct_is_confirmed(ct)) {
		ct->timeout.expires = extra_jiffies;
		event = IPCT_REFRESH;
	} else {
		unsigned long newtime = jiffies + extra_jiffies;

		/* Only update the timeout if the new timeout is at least
		   HZ jiffies from the old timeout. Need del_timer for race
		   avoidance (may already be dying). */
		if (newtime - ct->timeout.expires >= HZ
		    && del_timer(&ct->timeout)) {
			ct->timeout.expires = newtime;
			add_timer(&ct->timeout);
			event = IPCT_REFRESH;
		}
	}

#ifdef CONFIG_NF_CT_ACCT
	if (do_acct) {
		ct->counters[CTINFO2DIR(ctinfo)].packets++;
		ct->counters[CTINFO2DIR(ctinfo)].bytes +=
			skb->len - (unsigned int)(skb->nh.raw - skb->data);

		if ((ct->counters[CTINFO2DIR(ctinfo)].packets & 0x80000000)
		    || (ct->counters[CTINFO2DIR(ctinfo)].bytes & 0x80000000))
			event |= IPCT_COUNTER_FILLING;
	}
#endif

	write_unlock_bh(&nf_conntrack_lock);

	/* must be unlocked when calling event cache */
	if (event)
		nf_conntrack_event_cache(event, skb);
}
EXPORT_SYMBOL_GPL(__nf_ct_refresh_acct);

#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_conntrack.h>
#include <linux/mutex.h>


/* Generic function for tcp/udp/sctp/dccp and alike. This needs to be
 * in ip_conntrack_core, since we don't want the protocols to autoload
 * or depend on ctnetlink */
int nf_ct_port_tuple_to_nfattr(struct sk_buff *skb,
			       const struct nf_conntrack_tuple *tuple)
{
	NFA_PUT(skb, CTA_PROTO_SRC_PORT, sizeof(u_int16_t),
		&tuple->src.u.tcp.port);
	NFA_PUT(skb, CTA_PROTO_DST_PORT, sizeof(u_int16_t),
		&tuple->dst.u.tcp.port);
	return 0;

nfattr_failure:
	return -1;
}
EXPORT_SYMBOL_GPL(nf_ct_port_tuple_to_nfattr);

static const size_t cta_min_proto[CTA_PROTO_MAX] = {
	[CTA_PROTO_SRC_PORT-1]  = sizeof(u_int16_t),
	[CTA_PROTO_DST_PORT-1]  = sizeof(u_int16_t)
};

int nf_ct_port_nfattr_to_tuple(struct nfattr *tb[],
			       struct nf_conntrack_tuple *t)
{
	if (!tb[CTA_PROTO_SRC_PORT-1] || !tb[CTA_PROTO_DST_PORT-1])
		return -EINVAL;

	if (nfattr_bad_size(tb, CTA_PROTO_MAX, cta_min_proto))
		return -EINVAL;

	t->src.u.tcp.port = *(__be16 *)NFA_DATA(tb[CTA_PROTO_SRC_PORT-1]);
	t->dst.u.tcp.port = *(__be16 *)NFA_DATA(tb[CTA_PROTO_DST_PORT-1]);

	return 0;
}
EXPORT_SYMBOL_GPL(nf_ct_port_nfattr_to_tuple);
#endif

/* Used by ipt_REJECT and ip6t_REJECT. */
void __nf_conntrack_attach(struct sk_buff *nskb, struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;

	/* This ICMP is in reverse direction to the packet which caused it */
	ct = nf_ct_get(skb, &ctinfo);
	if (CTINFO2DIR(ctinfo) == IP_CT_DIR_ORIGINAL)
		ctinfo = IP_CT_RELATED + IP_CT_IS_REPLY;
	else
		ctinfo = IP_CT_RELATED;

	/* Attach to new skbuff, and increment count */
	nskb->nfct = &ct->ct_general;
	nskb->nfctinfo = ctinfo;
	nf_conntrack_get(nskb->nfct);
}
EXPORT_SYMBOL_GPL(__nf_conntrack_attach);

static inline int
do_iter(const struct nf_conntrack_tuple_hash *i,
	int (*iter)(struct nf_conn *i, void *data),
	void *data)
{
	return iter(nf_ct_tuplehash_to_ctrack(i), data);
}

/* Bring out ya dead! */
static struct nf_conn *
get_next_corpse(int (*iter)(struct nf_conn *i, void *data),
		void *data, unsigned int *bucket)
{
	struct nf_conntrack_tuple_hash *h;
	struct nf_conn *ct;

	write_lock_bh(&nf_conntrack_lock);
	for (; *bucket < nf_conntrack_htable_size; (*bucket)++) {
		list_for_each_entry(h, &nf_conntrack_hash[*bucket], list) {
			ct = nf_ct_tuplehash_to_ctrack(h);
			if (iter(ct, data))
				goto found;
		}
	}
	list_for_each_entry(h, &unconfirmed, list) {
		ct = nf_ct_tuplehash_to_ctrack(h);
		if (iter(ct, data))
			set_bit(IPS_DYING_BIT, &ct->status);
	}
	write_unlock_bh(&nf_conntrack_lock);
	return NULL;
found:
	atomic_inc(&ct->ct_general.use);
	write_unlock_bh(&nf_conntrack_lock);
	return ct;
}

void
nf_ct_iterate_cleanup(int (*iter)(struct nf_conn *i, void *data), void *data)
{
	struct nf_conn *ct;
	unsigned int bucket = 0;

	while ((ct = get_next_corpse(iter, data, &bucket)) != NULL) {
		/* Time to push up daises... */
		if (del_timer(&ct->timeout))
			death_by_timeout((unsigned long)ct);
		/* ... else the timer will get him soon. */

		nf_ct_put(ct);
	}
}
EXPORT_SYMBOL_GPL(nf_ct_iterate_cleanup);

static int kill_all(struct nf_conn *i, void *data)
{
	return 1;
}

static void free_conntrack_hash(struct list_head *hash, int vmalloced, int size)
{
	if (vmalloced)
		vfree(hash);
	else
		free_pages((unsigned long)hash,
			   get_order(sizeof(struct list_head) * size));
}

void nf_conntrack_flush(void)
{
	nf_ct_iterate_cleanup(kill_all, NULL);
}
EXPORT_SYMBOL_GPL(nf_conntrack_flush);

/* Mishearing the voices in his head, our hero wonders how he's
   supposed to kill the mall. */
void nf_conntrack_cleanup(void)
{
	int i;

	rcu_assign_pointer(ip_ct_attach, NULL);

	/* This makes sure all current packets have passed through
	   netfilter framework.  Roll on, two-stage module
	   delete... */
	synchronize_net();

	nf_ct_event_cache_flush();
 i_see_dead_people:
	nf_conntrack_flush();
#if defined(CONFIG_MIPS_BRCM)
	/* bugfix for lost connections */
	if (atomic_read(&nf_conntrack_count) != 0) {
		unsigned int bucket = 0;
		struct nf_conn *ct;
		if ((ct = get_next_corpse(kill_all, NULL, &bucket)) != NULL) {
			nf_ct_put(ct);
		} else {
			printk("found %d lost ct\n",
				atomic_read(&nf_conntrack_count));
			while(!list_empty(&safe_list)) {
				ct = container_of(safe_list.next,
					struct nf_conn, safe_list);
				nf_conntrack_free(ct);
			}
			atomic_set(&nf_conntrack_count, 0);
		}
	}
#endif
	if (atomic_read(&nf_conntrack_count) != 0) {
		schedule();
		goto i_see_dead_people;
	}
	/* wait until all references to nf_conntrack_untracked are dropped */
	while (atomic_read(&nf_conntrack_untracked.ct_general.use) > 1)
		schedule();

	for (i = 0; i < NF_CT_F_NUM; i++) {
		if (nf_ct_cache[i].use == 0)
			continue;

		NF_CT_ASSERT(nf_ct_cache[i].use == 1);
		nf_ct_cache[i].use = 1;
		nf_conntrack_unregister_cache(i);
	}
	kmem_cache_destroy(nf_conntrack_expect_cachep);
	free_conntrack_hash(nf_conntrack_hash, nf_conntrack_vmalloc,
			    nf_conntrack_htable_size);

	nf_conntrack_l4proto_unregister(&nf_conntrack_l4proto_generic);

	/* free l3proto protocol tables */
	for (i = 0; i < PF_MAX; i++)
		if (nf_ct_protos[i]) {
			kfree(nf_ct_protos[i]);
			nf_ct_protos[i] = NULL;
		}
#if defined(CONFIG_SERCOMM_CODE)
	remove_proc_entry("net/ct_stat", NULL /* parent dir */);
	reserve_port_clean();
#endif
}

static struct list_head *alloc_hashtable(int size, int *vmalloced)
{
	struct list_head *hash;
	unsigned int i;

	*vmalloced = 0;
	hash = (void*)__get_free_pages(GFP_KERNEL,
				       get_order(sizeof(struct list_head)
						 * size));
	if (!hash) {
		*vmalloced = 1;
		printk(KERN_WARNING "nf_conntrack: falling back to vmalloc.\n");
		hash = vmalloc(sizeof(struct list_head) * size);
	}

	if (hash)
		for (i = 0; i < size; i++)
			INIT_LIST_HEAD(&hash[i]);

	return hash;
}

int set_hashsize(const char *val, struct kernel_param *kp)
{
	int i, bucket, hashsize, vmalloced;
	int old_vmalloced, old_size;
	int rnd;
	struct list_head *hash, *old_hash;
	struct nf_conntrack_tuple_hash *h;

	/* On boot, we can set this without any fancy locking. */
	if (!nf_conntrack_htable_size)
		return param_set_uint(val, kp);

	hashsize = simple_strtol(val, NULL, 0);
	if (!hashsize)
		return -EINVAL;

	hash = alloc_hashtable(hashsize, &vmalloced);
	if (!hash)
		return -ENOMEM;

	/* We have to rehahs for the new table anyway, so we also can
	 * use a newrandom seed */
	get_random_bytes(&rnd, 4);

	write_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < nf_conntrack_htable_size; i++) {
		while (!list_empty(&nf_conntrack_hash[i])) {
			h = list_entry(nf_conntrack_hash[i].next,
				       struct nf_conntrack_tuple_hash, list);
			list_del(&h->list);
			bucket = __hash_conntrack(&h->tuple, hashsize, rnd);
			list_add_tail(&h->list, &hash[bucket]);
		}
	}
	old_size = nf_conntrack_htable_size;
	old_vmalloced = nf_conntrack_vmalloc;
	old_hash = nf_conntrack_hash;

	nf_conntrack_htable_size = hashsize;
	nf_conntrack_vmalloc = vmalloced;
	nf_conntrack_hash = hash;
	nf_conntrack_hash_rnd = rnd;
	write_unlock_bh(&nf_conntrack_lock);

	free_conntrack_hash(old_hash, old_vmalloced, old_size);
	return 0;
}

module_param_call(hashsize, set_hashsize, param_get_uint,
		  &nf_conntrack_htable_size, 0600);
#if defined(CONFIG_SERCOMM_CODE)
int proc_dump_nf_conntrack_host(char *buf, char **start, off_t offset,
int count, int *eof, void *data)
{
	int i, len = 0, cnt;
	int limit = count - 80; /* Don't print more than this */

	if ( offset >= HOST_MAX )
	{
		*eof = 1;
		return len;
	}
		
	if( offset == 0)
	{
		len += sprintf(buf+len,"ct count info:\n" );
		len += sprintf(buf+len,"  total = %d\n", atomic_read(&nf_conntrack_count) );
		len += sprintf(buf+len,"  max_record = %d\n", nf_conntrack_count_max_record );
		len += sprintf(buf+len,"  not reserve count = %d\n", atomic_read(&ct_not_reserve_count) );
		len += sprintf(buf+len,"  not reserve max = %d\n", ct_not_reserve_max);

		len += sprintf(buf+len,"\n");
	}

	
	for (i = 0; i < HOST_MAX  && len <limit ; i++)
	{
		if ( i < offset )
		{
			continue;
		}
	
		cnt = atomic_read(&nf_conntrack_host[ i].count);

		if(( cnt > 0 ) )
			len += sprintf(buf + len, "%03d    s=%u.%u.%u.%u    \td=%u.%u.%u.%u    \tct=%d \ttcp=%d \tudp=%d\n",
				i, 
				NIPQUAD(nf_conntrack_host[i].src_addr) , 
				NIPQUAD(nf_conntrack_host[i].dst_addr) , 
				cnt,
				atomic_read(&nf_conntrack_host[ i].count_tcp),
				atomic_read(&nf_conntrack_host[ i].count_udp));

		*start  = *start +1;
	}

	if( i == HOST_MAX)
		*eof = 1;
	return len;
}
#endif

int __init nf_conntrack_init(void)
{
	unsigned int i;
	int ret;

	/* Idea from tcp.c: use 1/16384 of memory.  On i386: 32MB
	 * machine has 256 buckets.  >= 1GB machines have 8192 buckets. */
	if (!nf_conntrack_htable_size) {
		nf_conntrack_htable_size
			= (((num_physpages << PAGE_SHIFT) / 16384)
			   / sizeof(struct list_head));
		if (num_physpages > (1024 * 1024 * 1024 / PAGE_SIZE))
			nf_conntrack_htable_size = 8192;
		if (nf_conntrack_htable_size < 16)
			nf_conntrack_htable_size = 16;
	}
	nf_conntrack_max = 8 * nf_conntrack_htable_size;

	printk("nf_conntrack version %s (%u buckets, %d max)\n",
	       NF_CONNTRACK_VERSION, nf_conntrack_htable_size,
	       nf_conntrack_max);

	nf_conntrack_hash = alloc_hashtable(nf_conntrack_htable_size,
					    &nf_conntrack_vmalloc);
	if (!nf_conntrack_hash) {
		printk(KERN_ERR "Unable to create nf_conntrack_hash\n");
		goto err_out;
	}

	ret = nf_conntrack_register_cache(NF_CT_F_BASIC, "nf_conntrack:basic",
					  sizeof(struct nf_conn));
	if (ret < 0) {
		printk(KERN_ERR "Unable to create nf_conn slab cache\n");
		goto err_free_hash;
	}

	nf_conntrack_expect_cachep = kmem_cache_create("nf_conntrack_expect",
					sizeof(struct nf_conntrack_expect),
					0, 0, NULL, NULL);
	if (!nf_conntrack_expect_cachep) {
		printk(KERN_ERR "Unable to create nf_expect slab cache\n");
		goto err_free_conntrack_slab;
	}

	ret = nf_conntrack_l4proto_register(&nf_conntrack_l4proto_generic);
	if (ret < 0)
		goto out_free_expect_slab;

	/* Don't NEED lock here, but good form anyway. */
	write_lock_bh(&nf_conntrack_lock);
	for (i = 0; i < AF_MAX; i++)
		nf_ct_l3protos[i] = &nf_conntrack_l3proto_generic;
	write_unlock_bh(&nf_conntrack_lock);

	/* For use by REJECT target */
	rcu_assign_pointer(ip_ct_attach, __nf_conntrack_attach);
#if defined(CONFIG_SERCOMM_CODE)
	// initialize nf_conntrack_host
	for (i = 0 ; i < HOST_MAX ; i++) {
		atomic_set(  & nf_conntrack_host[i].count, 0);
	}

	create_proc_read_entry("net/ct_stat", 0 /* default mode */, NULL /* parent dir */, 
 	   	proc_dump_nf_conntrack_host, NULL /* client data */);	

/*

You can refer 
	http://practicallynetworked.com/sharing/app_port_list.htm
for more information

PORT	Application

1701 	L2TP
1723	PPTP
1863	MSN
3128	MSN ?
4500	L2TP
5000	yahoo messenger chat
5001	yahoo messenger chat
5055	yahoo messenger phone
5190	MSN, ALM, ... 
5060	SIP
6891 ~ 
6901 	MSN
*/
	reserve_port_list_init();
	reserve_port_add( 1701);
	reserve_port_add( 1723);
	reserve_port_add( 1863);
	reserve_port_add( 3128);
	reserve_port_add( 4500);
	reserve_port_add( 5000);
	reserve_port_add( 5001);
	reserve_port_add( 5055);
	reserve_port_add( 5190);
	reserve_port_add( 5060);
	reserve_port_add( 6877);	// my online game ^_^
	reserve_port_add( 6891);
	reserve_port_add( 6892);
	reserve_port_add( 6893);
	reserve_port_add( 6894);
	reserve_port_add( 6895);
	reserve_port_add( 6896);
	reserve_port_add( 6897);
	reserve_port_add( 6898);
	reserve_port_add( 6899);
	reserve_port_add( 6900);
	reserve_port_add( 6901);
	// Add for WarCraft III
	reserve_port_add( 6112);
	reserve_port_add( 6113);
	reserve_port_add( 6114);
	reserve_port_add( 6115);
	reserve_port_add( 6116);
	reserve_port_add( 6117);
	reserve_port_add( 6118);
	reserve_port_add( 6119);
	// Add for upnp port
	reserve_port_add( 49151);
	reserve_port_add( 49152);
	reserve_port_add( 49153);
	reserve_port_add( 49154);
	reserve_port_add( 49155);
	reserve_port_add( 49156);
	reserve_port_add( 49157);
	reserve_port_add( 49158);
	reserve_port_add( 49159);
#endif
	/* Set up fake conntrack:
	    - to never be deleted, not in any hashes */
	atomic_set(&nf_conntrack_untracked.ct_general.use, 1);
	/*  - and look it like as a confirmed connection */
	set_bit(IPS_CONFIRMED_BIT, &nf_conntrack_untracked.status);

#if defined(CONFIG_MIPS_BRCM) && defined(CONFIG_BLOG)
    blog_refresh_fn = (blog_refresh_t)__nf_ct_refresh_acct;
#endif

	return ret;

out_free_expect_slab:
	kmem_cache_destroy(nf_conntrack_expect_cachep);
err_free_conntrack_slab:
	nf_conntrack_unregister_cache(NF_CT_F_BASIC);
err_free_hash:
	free_conntrack_hash(nf_conntrack_hash, nf_conntrack_vmalloc, nf_conntrack_htable_size);
err_out:
	return -ENOMEM;
}
