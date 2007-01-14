/*
 * This config file is for toshiba Gigabeat F
 */
#define TARGET_TREE /* this target is using the target tree system */

#define TOSHIBA_GIGABEAT_F 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */


#define CONFIG_KEYPAD GIGABEAT_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_S3C2440

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_GIGABEAT /* port controlled PWM */

#define HAVE_LCD_ENABLE

#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          0   /* 0.5 mA */
#define MIN_ACTIVE_BRIGHTNESS_SETTING   16  /* lowest active brightness */
#define MAX_DIM_BRIGHTNESS_SETTING      15  /* highest 'dimness' */
#define MAX_BRIGHTNESS_SETTING          63  /* 32 mA */
#define DEFAULT_BRIGHTNESS_SETTING      39  /* 20 mA */
#define DEFAULT_DIMNESS_SETTING          9  /* 5 mA */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8751

#define BATTERY_CAPACITY_DEFAULT 2000 /* default battery capacity */

#ifndef SIMULATOR

/* The LCD on a Gigabeat is 240x320 - it is portrait */
#define HAVE_PORTRAIT_LCD

/* Define this if you have a Motorola SCF5249 */
#define CONFIG_CPU S3C2440

/* Define this if you want to use coldfire's i2c interface */
#define CONFIG_I2C I2C_S3C2440

/* Type of mobile power - check this out */
#define CONFIG_BATTERY BATT_LIION830    /* could change this later */
#define BATTERY_CAPACITY_MIN 1500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 25         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* ADC[0] is (530) at discharge and 625 at full charge */
#define BATTERY_SCALE_FACTOR 6450

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 16934400

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_GIGABEAT

/* define this if the backlight can be set to a brightness */
#define HAVE_BACKLIGHT_SET_FADING
#define __BACKLIGHT_INIT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_GIGABEAT_STYLE

#define HAVE_HEADPHONE_DETECTION
/* Define this if you have adjustable CPU frequency */
#if 0 /* TODO */
#define HAVE_ADJUSTABLE_CPU_FREQ
#endif

#define BOOTFILE_EXT "gigabeat"
#define BOOTFILE "rockbox." BOOTFILE_EXT


#endif
