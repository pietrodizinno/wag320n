#ifndef __PUSH_BUTTON__
#define __PUSH_BUTTON__

#define PUSH 1
#define RELEASE 0

/*define button id */
#define BUTTON_RESET	1
#define BUTTON_WPS		0

#define NETLINK_PB 23
#define MAX_PAYLOAD 1024 

#define GPIO_BASE       0xfffe0080 //0xfffe0400S
/*------------------------------------------*/
/* GPIO I/F                                 */
/*------------------------------------------*/

//#define GPIO_DATA_INPUT     (*(volatile unsigned int *)(GPIO_BASE + 0x0000000C))
//#define GPIO_DATA_OUTPUT    (*(volatile unsigned int *)(GPIO_BASE + 0x0000000C))
//#define GPIO_DATA_DIR        (*(volatile unsigned int *)(GPIO_BASE + 0x00000008)) /* 0=out 1=in */
//#define GPIO_DATA_ENABLE    (*(volatile unsigned int *)(GPIO_BASE + 0x00000004)) /* 1=GPIO */

#define GPIO_DATA_INPUT      (*(volatile unsigned int *)(GPIO_BASE + 0x0000000C))
#define GPIO_DATA_OUTPUT     (*(volatile unsigned int *)(GPIO_BASE + 0x0000000C))
#define GPIO_DATA_ENABLE     (*(volatile unsigned int *)(GPIO_BASE + 0x00000004))

#define GPIO_DATA_INPUT1	(*(volatile unsigned int *)(GPIO_BASE + 0x00000008))	//GPIO 32~63
#define GPIO_DATA_ENABLE1	(*(volatile unsigned int *)(GPIO_BASE + 0x00000000))	//GPIO 32~63
#define GPIO_DATA_OUTPUT1	(*(volatile unsigned int *)(GPIO_BASE + 0x00000008))	//GPIO 32~63

#define GPIO_WPS_BUTTON    		(1<<2)
#define GPIO_RESET_BUTTON    	(1<<29)

#define REGIO(_dev, _reg) ( ((BF_##_dev##_REGS *)(BF_##_dev))->_reg )

struct pb_data{
	int now_staus;	
};

#endif
