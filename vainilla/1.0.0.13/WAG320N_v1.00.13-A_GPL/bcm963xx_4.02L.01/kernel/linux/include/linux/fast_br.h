#ifndef _LINUX_FAST_BR_H
#define _LINUX_FAST_BR_H

typedef int (*fast_br_handle_frame_hook_t)(struct sk_buff *skb);
typedef void (*fast_br_register_t)(struct net_device *);
typedef void (*fast_br_unregister_t)(struct net_device *);

fast_br_handle_frame_hook_t fast_br_handle_frame_hook=NULL;
fast_br_register_t fast_br_register=NULL;
fast_br_unregister_t fast_br_unregister=NULL;

EXPORT_SYMBOL(fast_br_handle_frame_hook);
EXPORT_SYMBOL(fast_br_register);
EXPORT_SYMBOL(fast_br_unregister);

#endif

