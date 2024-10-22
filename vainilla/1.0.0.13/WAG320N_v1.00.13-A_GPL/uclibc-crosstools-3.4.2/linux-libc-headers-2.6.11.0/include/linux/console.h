/*
 *  linux/include/linux/console.h
 *
 *  Copyright (C) 1993        Hamish Macdonald
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Changed:
 * 10-Mar-94: Arno Griffioen: Conversion for vt100 emulator port from PC LINUX
 */

#ifndef _LINUX_CONSOLE_H_
#define _LINUX_CONSOLE_H_ 1

#include <linux/types.h>

struct vc_data;
struct console_font_op;
struct console_font;
struct module;

/*
 * this is what the terminal answers to a ESC-Z or csi0c query.
 */
#define VT100ID "\033[?1;2c"
#define VT102ID "\033[?6c"

struct consw {
	struct module *owner;
	const char *(*con_startup)(void);
	void	(*con_init)(struct vc_data *, int);
	void	(*con_deinit)(struct vc_data *);
	void	(*con_clear)(struct vc_data *, int, int, int, int);
	void	(*con_putc)(struct vc_data *, int, int, int);
	void	(*con_putcs)(struct vc_data *, const unsigned short *, int, int, int);
	void	(*con_cursor)(struct vc_data *, int);
	int	(*con_scroll)(struct vc_data *, int, int, int, int);
	void	(*con_bmove)(struct vc_data *, int, int, int, int, int, int);
	int	(*con_switch)(struct vc_data *);
	int	(*con_blank)(struct vc_data *, int, int);
	int	(*con_font_set)(struct vc_data *, struct console_font *, unsigned);
	int	(*con_font_get)(struct vc_data *, struct console_font *);
	int	(*con_font_default)(struct vc_data *, struct console_font *, char *);
	int	(*con_font_copy)(struct vc_data *, int);
	int	(*con_resize)(struct vc_data *, unsigned int, unsigned int);
	int	(*con_set_palette)(struct vc_data *, unsigned char *);
	int	(*con_scrolldelta)(struct vc_data *, int);
	int	(*con_set_origin)(struct vc_data *);
	void	(*con_save_screen)(struct vc_data *);
	__u8	(*con_build_attr)(struct vc_data *, __u8, __u8, __u8, __u8, __u8);
	void	(*con_invert_region)(struct vc_data *, __u16 *, int);
	__u16    *(*con_screen_pos)(struct vc_data *, int);
	unsigned long (*con_getxy)(struct vc_data *, unsigned long, int *, int *);
};

extern const struct consw *conswitchp;

extern const struct consw dummy_con;	/* dummy console buffer */
extern const struct consw fb_con;	/* frame buffer based console */
extern const struct consw vga_con;	/* VGA text console */
extern const struct consw newport_con;	/* SGI Newport console  */
extern const struct consw prom_con;	/* SPARC PROM console */

int take_over_console(const struct consw *sw, int first, int last, int deflt);
void give_up_console(const struct consw *sw);

/* scroll */
#define SM_UP       (1)
#define SM_DOWN     (2)

/* cursor */
#define CM_DRAW     (1)
#define CM_ERASE    (2)
#define CM_MOVE     (3)

/*
 *	The interface for a console, or any other device that
 *	wants to capture console messages (printer driver?)
 */

#define CON_PRINTBUFFER	(1)
#define CON_CONSDEV	(2) /* Last on the command line */
#define CON_ENABLED	(4)

struct console
{
	char	name[8];
	void	(*write)(struct console *, const char *, unsigned);
	int	(*read)(struct console *, char *, unsigned);
	struct tty_driver *(*device)(struct console *, int *);
	void	(*unblank)(void);
	int	(*setup)(struct console *, char *);
	short	flags;
	short	index;
	int	cflag;
	void	*data;
	struct	 console *next;
};

extern int add_preferred_console(char *name, int idx, char *options);
extern void register_console(struct console *);
extern int unregister_console(struct console *);
extern struct console *console_drivers;
extern void acquire_console_sem(void);
extern int try_acquire_console_sem(void);
extern void release_console_sem(void);
extern void console_conditional_schedule(void);
extern void console_unblank(void);
extern struct tty_driver *console_device(int *);
extern void console_stop(struct console *);
extern void console_start(struct console *);
extern int is_console_locked(void);

/* Some debug stub to catch some of the obvious races in the VT code */
#if 1
#define WARN_CONSOLE_UNLOCKED()	WARN_ON(!is_console_locked() && !oops_in_progress)
#else
#define WARN_CONSOLE_UNLOCKED()
#endif

/* VESA Blanking Levels */
#define VESA_NO_BLANKING        0
#define VESA_VSYNC_SUSPEND      1
#define VESA_HSYNC_SUSPEND      2
#define VESA_POWERDOWN          3

#endif /* _LINUX_CONSOLE_H */
