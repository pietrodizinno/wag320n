/*
 * Hardware specific definitions for SL-C7xx series of PDAs
 *
 * Copyright (c) 2004-2005 Richard Purdie
 *
 * Based on Sharp's 2.4 kernel patches
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __ASM_ARCH_CORGI_H
#define __ASM_ARCH_CORGI_H  1


/*
 * Corgi (Non Standard) GPIO Definitions
 */
#define CORGI_GPIO_KEY_INT			(0)	/* Keyboard Interrupt */
#define CORGI_GPIO_AC_IN			(1) /* Charger Detection */
#define CORGI_GPIO_WAKEUP			(3) /* System wakeup notification? */
#define CORGI_GPIO_AK_INT			(4)	/* Headphone Jack Control Interrupt */
#define CORGI_GPIO_TP_INT			(5)	/* Touch Panel Interrupt */
#define CORGI_GPIO_nSD_WP			(7) /* SD Write Protect? */
#define CORGI_GPIO_nSD_DETECT		(9) /* MMC/SD Card Detect */
#define CORGI_GPIO_nSD_INT			(10) /* SD Interrupt for SDIO? */
#define CORGI_GPIO_MAIN_BAT_LOW		(11) /* Main Battery Low Notification */
#define CORGI_GPIO_BAT_COVER		(11) /* Battery Cover Detect */
#define CORGI_GPIO_LED_ORANGE		(13) /* Orange LED Control */
#define CORGI_GPIO_CF_CD			(14) /* Compact Flash Card Detect */
#define CORGI_GPIO_CHRG_FULL		(16) /* Charging Complete Notification */
#define CORGI_GPIO_CF_IRQ			(17) /* Compact Flash Interrupt */
#define CORGI_GPIO_LCDCON_CS		(19) /* LCD Control Chip Select */
#define CORGI_GPIO_MAX1111_CS		(20) /* MAX1111 Chip Select */
#define CORGI_GPIO_ADC_TEMP_ON		(21) /* Select battery voltage or temperature */
#define CORGI_GPIO_IR_ON			(22) /* Enable IR Transciever */
#define CORGI_GPIO_ADS7846_CS		(24) /* ADS7846 Chip Select */
#define CORGI_GPIO_SD_PWR			(33) /* MMC/SD Power */
#define CORGI_GPIO_CHRG_ON			(38) /* Enable battery Charging */
#define CORGI_GPIO_DISCHARGE_ON		(42) /* Enable battery Discharge */
#define CORGI_GPIO_CHRG_UKN			(43) /* Unknown Charging (Bypass Control?) */
#define CORGI_GPIO_HSYNC			(44) /* LCD HSync Pulse */
#define CORGI_GPIO_USB_PULLUP		(45) /* USB show presence to host */


/*
 * Corgi Keyboard Definitions
 */
#define CORGI_KEY_STROBE_NUM		(12)
#define CORGI_KEY_SENSE_NUM			(8)
#define CORGI_GPIO_ALL_STROBE_BIT	(0x00003ffc)
#define CORGI_GPIO_HIGH_SENSE_BIT	(0xfc000000)
#define CORGI_GPIO_HIGH_SENSE_RSHIFT	(26)
#define CORGI_GPIO_LOW_SENSE_BIT	(0x00000003)
#define CORGI_GPIO_LOW_SENSE_LSHIFT	(6)
#define CORGI_GPIO_STROBE_BIT(a)	GPIO_bit(66+(a))
#define CORGI_GPIO_SENSE_BIT(a)		GPIO_bit(58+(a))
#define CORGI_GAFR_ALL_STROBE_BIT	(0x0ffffff0)
#define CORGI_GAFR_HIGH_SENSE_BIT	(0xfff00000)
#define CORGI_GAFR_LOW_SENSE_BIT	(0x0000000f)
#define CORGI_GPIO_KEY_SENSE(a)		(58+(a))
#define CORGI_GPIO_KEY_STROBE(a)	(66+(a))


/*
 * Corgi Interrupts
 */
#define CORGI_IRQ_GPIO_KEY_INT		IRQ_GPIO(0)
#define CORGI_IRQ_GPIO_AC_IN		IRQ_GPIO(1)
#define CORGI_IRQ_GPIO_WAKEUP		IRQ_GPIO(3)
#define CORGI_IRQ_GPIO_AK_INT		IRQ_GPIO(4)
#define CORGI_IRQ_GPIO_TP_INT		IRQ_GPIO(5)
#define CORGI_IRQ_GPIO_nSD_DETECT	IRQ_GPIO(9)
#define CORGI_IRQ_GPIO_nSD_INT		IRQ_GPIO(10)
#define CORGI_IRQ_GPIO_MAIN_BAT_LOW	IRQ_GPIO(11)
#define CORGI_IRQ_GPIO_CF_CD		IRQ_GPIO(14)
#define CORGI_IRQ_GPIO_CHRG_FULL	IRQ_GPIO(16)	/* Battery fully charged */
#define CORGI_IRQ_GPIO_CF_IRQ		IRQ_GPIO(17)
#define CORGI_IRQ_GPIO_KEY_SENSE(a)	IRQ_GPIO(58+(a))	/* Keyboard Sense lines */


/*
 * Corgi SCOOP GPIOs and Config
 */
#define CORGI_SCP_LED_GREEN		SCOOP_GPCR_PA11
#define CORGI_SCP_SWA			SCOOP_GPCR_PA12  /* Hinge Switch A */
#define CORGI_SCP_SWB			SCOOP_GPCR_PA13  /* Hinge Switch B */
#define CORGI_SCP_MUTE_L		SCOOP_GPCR_PA14
#define CORGI_SCP_MUTE_R		SCOOP_GPCR_PA15
#define CORGI_SCP_AKIN_PULLUP	SCOOP_GPCR_PA16
#define CORGI_SCP_APM_ON		SCOOP_GPCR_PA17
#define CORGI_SCP_BACKLIGHT_CONT	SCOOP_GPCR_PA18
#define CORGI_SCP_MIC_BIAS		SCOOP_GPCR_PA19

#define CORGI_SCOOP_IO_DIR	( CORGI_SCP_LED_GREEN | CORGI_SCP_MUTE_L | CORGI_SCP_MUTE_R | \
			CORGI_SCP_AKIN_PULLUP | CORGI_SCP_APM_ON | CORGI_SCP_BACKLIGHT_CONT | \
			CORGI_SCP_MIC_BIAS )
#define CORGI_SCOOP_IO_OUT	( CORGI_SCP_MUTE_L | CORGI_SCP_MUTE_R )


/*
 * Corgi Parameter Area Definitions
 */
#define FLASH_MEM_BASE	0xa0000a00
#define FLASH_MAGIC_CHG(a,b,c,d) ( ( d << 24 ) | ( c << 16 )  | ( b << 8 ) | a )

#define FLASH_COMADJ_MAJIC	FLASH_MAGIC_CHG('C','M','A','D')
#define	FLASH_COMADJ_MAGIC_ADR	0x00
#define	FLASH_COMADJ_DATA_ADR	0x04

#define FLASH_PHAD_MAJIC	FLASH_MAGIC_CHG('P','H','A','D')
#define	FLASH_PHAD_MAGIC_ADR	0x38
#define	FLASH_PHAD_DATA_ADR	0x3C

struct sharpsl_flash_param_info {
  unsigned int comadj_keyword;
  unsigned int comadj;

  unsigned int uuid_keyword;
  unsigned char uuid[16];

  unsigned int touch_keyword;
  unsigned int touch1;
  unsigned int touch2;
  unsigned int touch3;
  unsigned int touch4;

  unsigned int adadj_keyword;
  unsigned int adadj;

  unsigned int phad_keyword;
  unsigned int phadadj;
};


/*
 * External Functions
 */
extern unsigned long corgi_ssp_ads7846_putget(unsigned long);
extern unsigned long corgi_ssp_ads7846_get(void);
extern void corgi_ssp_ads7846_put(ulong data);
extern void corgi_ssp_ads7846_lock(void);
extern void corgi_ssp_ads7846_unlock(void);
extern void corgi_ssp_lcdtg_send (__u8 adrs, __u8 data);
extern void corgi_ssp_blduty_set(int duty);
extern int corgi_ssp_max1111_get(ulong data);

#endif /* __ASM_ARCH_CORGI_H  */

