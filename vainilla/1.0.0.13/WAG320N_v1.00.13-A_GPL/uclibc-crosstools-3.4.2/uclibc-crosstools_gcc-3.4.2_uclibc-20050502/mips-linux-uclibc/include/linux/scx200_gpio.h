
__u32 scx200_gpio_configure(int index, __u32 set, __u32 clear);
void scx200_gpio_dump(unsigned index);

extern unsigned scx200_gpio_base;
extern spinlock_t scx200_gpio_lock;
extern long scx200_gpio_shadow[2];

#define scx200_gpio_present() (scx200_gpio_base!=0)

/* Definitions to make sure I do the same thing in all functions */
#define __SCx200_GPIO_BANK unsigned bank = index>>5
#define __SCx200_GPIO_IOADDR unsigned short ioaddr = scx200_gpio_base+0x10*bank
#define __SCx200_GPIO_SHADOW long *shadow = scx200_gpio_shadow+bank
#define __SCx200_GPIO_INDEX index &= 31

#define __SCx200_GPIO_OUT __asm__ __volatile__("outsl":"=mS" (shadow):"d" (ioaddr), "0" (shadow))

/* returns the value of the GPIO pin */

static inline int scx200_gpio_get(int index) {
	__SCx200_GPIO_BANK;
	__SCx200_GPIO_IOADDR + 0x04;
	__SCx200_GPIO_INDEX;
		
	return (inl(ioaddr) & (1<<index)) ? 1 : 0;
}

/* return the value driven on the GPIO signal (the value that will be
   driven if the GPIO is configured as an output, it might not be the
   state of the GPIO right now if the GPIO is configured as an input) */

static inline int scx200_gpio_current(int index) {
        __SCx200_GPIO_BANK;
	__SCx200_GPIO_INDEX;
		
	return (scx200_gpio_shadow[bank] & (1<<index)) ? 1 : 0;
}

/* drive the GPIO signal high */

static inline void scx200_gpio_set_high(int index) {
	__SCx200_GPIO_BANK;
	__SCx200_GPIO_IOADDR;
	__SCx200_GPIO_SHADOW;
	__SCx200_GPIO_INDEX;
	set_bit(index, shadow);
	__SCx200_GPIO_OUT;
}

/* drive the GPIO signal low */

static inline void scx200_gpio_set_low(int index) {
	__SCx200_GPIO_BANK;
	__SCx200_GPIO_IOADDR;
	__SCx200_GPIO_SHADOW;
	__SCx200_GPIO_INDEX;
	clear_bit(index, shadow);
	__SCx200_GPIO_OUT;
}

/* drive the GPIO signal to state */

static inline void scx200_gpio_set(int index, int state) {
	__SCx200_GPIO_BANK;
	__SCx200_GPIO_IOADDR;
	__SCx200_GPIO_SHADOW;
	__SCx200_GPIO_INDEX;
	if (state)
		set_bit(index, shadow);
	else
		clear_bit(index, shadow);
	__SCx200_GPIO_OUT;
}

/* toggle the GPIO signal */
static inline void scx200_gpio_change(int index) {
	__SCx200_GPIO_BANK;
	__SCx200_GPIO_IOADDR;
	__SCx200_GPIO_SHADOW;
	__SCx200_GPIO_INDEX;
	change_bit(index, shadow);
	__SCx200_GPIO_OUT;
}

#undef __SCx200_GPIO_BANK
#undef __SCx200_GPIO_IOADDR
#undef __SCx200_GPIO_SHADOW
#undef __SCx200_GPIO_INDEX
#undef __SCx200_GPIO_OUT

/*
    Local variables:
        compile-command: "make -C ../.. bzImage modules"
        c-basic-offset: 8
    End:
*/
