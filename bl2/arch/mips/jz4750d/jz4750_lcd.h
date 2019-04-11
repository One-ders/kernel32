
#define NR_PALETTE		256
#define PALETTE_SIZE    (NR_PALETTE*2)


/* use new descriptor(8 words) */
struct jz4750_lcd_dma_desc {
	unsigned int next_desc;         /* LCDDAx */
	unsigned int databuf;           /* LCDSAx */
	unsigned int frame_id;          /* LCDFIDx */ 
	unsigned int cmd;               /* LCDCMDx */
	unsigned int offsize;           /* Stride Offsize(in word) */
	unsigned int page_width;        /* Stride Pagewidth(in word) */
	unsigned int cmd_num;           /* Command Number(for SLCD) */
	unsigned int desc_size;         /* Foreground Size */
};

struct jz4750lcd_panel_t {
	unsigned int cfg;       /* panel mode and pin usage etc. */
	unsigned int slcd_cfg;  /* Smart lcd configurations */
	unsigned int ctrl;      /* lcd controll register */
	unsigned int w;         /* Panel Width(in pixel) */
	unsigned int h;         /* Panel Height(in line) */
	unsigned int fclk;      /* frame clk */
	unsigned int hsw;       /* hsync width, in pclk */
	unsigned int vsw;       /* vsync width, in line count */
	unsigned int elw;       /* end of line, in pclk */
	unsigned int blw;       /* begin of line, in pclk */
	unsigned int efw;       /* end of frame, in line count */
	unsigned int bfw;       /* begin of frame, in line count */
};

struct jz4750lcd_fg_t {
	int bpp;        /* foreground bpp */
	int x;          /* foreground start position x */
	int y;          /* foreground start position y */
	int w;          /* foreground width */
	int h;          /* foreground height */
};

struct jz4750lcd_osd_t {
	unsigned int osd_cfg;   /* OSDEN, ALHPAEN, F0EN, F1EN, etc */
	unsigned int osd_ctrl;  /* IPUEN, OSDBPP, etc */
	unsigned int rgb_ctrl;  /* RGB Dummy, RGB sequence, RGB to YUV */
	unsigned int bgcolor;   /* background color(RGB888) */
	unsigned int colorkey0; /* foreground0's Colorkey enable, Colorkey value */
	unsigned int colorkey1; /* foreground1's Colorkey enable, Colorkey value */
	unsigned int alpha;     /* ALPHAEN, alpha value */
	unsigned int ipu_restart; /* IPU Restart enable, ipu restart interval time */

#define FG_NOCHANGE             0x0000
#define FG0_CHANGE_SIZE         0x0001
#define FG0_CHANGE_POSITION     0x0002
#define FG1_CHANGE_SIZE         0x0010
#define FG1_CHANGE_POSITION     0x0020
#define FG_CHANGE_ALL           ( FG0_CHANGE_SIZE | FG0_CHANGE_POSITION | \
                                  FG1_CHANGE_SIZE | FG1_CHANGE_POSITION )
	int fg_change;
	struct jz4750lcd_fg_t fg0;      /* foreground 0 */
	struct jz4750lcd_fg_t fg1;      /* foreground 1 */
};


struct jz4750lcd_info {
	struct jz4750lcd_panel_t panel;
	struct jz4750lcd_osd_t osd;
};

/* Jz LCDFB supported I/O controls. */
#define FBIOSETBACKLIGHT        0x4688 /* set back light level */
#define FBIODISPON              0x4689 /* display on */
#define FBIODISPOFF             0x468a /* display off */
#define FBIORESET               0x468b /* lcd reset */
#define FBIOPRINT_REG           0x468c /* print lcd registers(debug) */
#define FBIOROTATE              0x46a0 /* rotated fb */
#define FBIOGETBUFADDRS         0x46a1 /* get buffers addresses */
#define FBIO_GET_MODE           0x46a2 /* get lcd info */
#define FBIO_SET_MODE           0x46a3 /* set osd mode */
#define FBIO_DEEP_SET_MODE      0x46a4 /* set panel and osd mode */
#define FBIO_MODE_SWITCH        0x46a5 /* switch mode between LCD and TVE */
#define FBIO_GET_TVE_MODE       0x46a6 /* get tve info */
#define FBIO_SET_TVE_MODE       0x46a7 /* set tve mode */


/*
 * LCD panel specific definition
 */
/* AUO */
#if defined(CONFIG_JZ4750_LCD_AUO_A043FL01V2)
#if defined(CONFIG_JZ4750_APUS) /* board pavo */
	#define SPEN            (32*3+29)       /*LCD_CS*/
	#define SPCK            (32*3+26)       /*LCD_SCL*/
	#define SPDA            (32*3+27)       /*LCD_SDA*/
	#define LCD_RET         (32*4+25)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4750_FUWA) || defined(CONFIG_JZ4750D_FUWA1) /* board fpga */
	#define SPEN            (32*3+29)       /*LCD_CS*/
	#define SPCK            (32*3+26)       /*LCD_SCL*/
	#define SPDA            (32*3+27)       /*LCD_SDA*/
	#define LCD_RET         (32*5+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4750L_F4750L) /* board pavo */
	#define SPEN            (32*3+30)       /*LCD_CS*/
	#define SPCK            (32*3+26)       /*LCD_SCL*/
	#define SPDA            (32*3+27)       /*LCD_SDA*/
	#define LCD_RET         (32*5+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4750D_CETUS) || defined(CONFIG_JZ4750D_KMP510) /* board pavo */
	#define SPEN            (32*5+13)       /*LCD_CS*/
	#define SPCK            (32*5+10)       /*LCD_SCL*/
	#define SPDA            (32*5+11)       /*LCD_SDA*/
	#define LCD_RET         (32*4+18)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/Jzlcd.h, please define SPI pins on your board."
#endif


#define __spi_write_reg(reg, val)               \
	do {                                    \
		unsigned char no;               \
		unsigned short value;           \
		unsigned char a=0;              \
		unsigned char b=0;              \
		__gpio_as_output(SPEN); /* use SPDA */  \
		__gpio_as_output(SPCK); /* use SPCK */  \
		__gpio_as_output(SPDA); /* use SPDA */  \
		a=reg;                          \
		b=val;                          \
		__gpio_set_pin(SPEN);           \
		__gpio_clear_pin(SPCK);         \
		udelay(50);                     \
		__gpio_clear_pin(SPDA);         \
		__gpio_clear_pin(SPEN);         \
		udelay(50);                     \
		value=((a<<8)|(b&0xFF));        \
		for(no=0;no<16;no++)            \
		{                               \
			if((value&0x8000)==0x8000){           \
				__gpio_set_pin(SPDA);}        \
			else{                                 \
				__gpio_clear_pin(SPDA); }     \
			udelay(50);                     \
			__gpio_set_pin(SPCK);           \
			value=(value<<1);               \
			udelay(50);                     \
			__gpio_clear_pin(SPCK);         \
		}                                       \
		__gpio_set_pin(SPEN);                   \
		udelay(400);                            \
	} while (0)

#define __spi_read_reg(reg,val)                 \
	do{                                     \
		unsigned char no;               \
		unsigned short value;                   \
		__gpio_as_output(SPEN); /* use SPDA */  \
		__gpio_as_output(SPCK); /* use SPCK */  \
		__gpio_as_output(SPDA); /* use SPDA */  \
		value = ((reg << 0) | (1 << 7));        \
		val = 0;                                \
		__gpio_as_output(SPDA);                 \
		__gpio_set_pin(SPEN);                   \
		__gpio_clear_pin(SPCK);                 \
		udelay(50);                             \
		__gpio_clear_pin(SPDA);                 \
		__gpio_clear_pin(SPEN);                 \
		udelay(50);                             \
		for (no = 0; no < 16; no++ ) {          \
			udelay(50);                     \
			if(no < 8)                      \
			{                                               \
				if (value & 0x80) /* send data */       \
					__gpio_set_pin(SPDA);           \
				else                                    \
					__gpio_clear_pin(SPDA);         \
				udelay(50);                             \
				__gpio_set_pin(SPCK);                   \
				value = (value << 1);                   \
				udelay(50);                             \
				__gpio_clear_pin(SPCK);                 \
				if(no == 7)                             \
					__gpio_as_input(SPDA);          \
			}                                               \
			else                                            \
			{                                               \
				udelay(100);                            \
				__gpio_set_pin(SPCK);                   \
				udelay(50);                             \
				val = (val << 1);                       \
				val |= __gpio_get_pin(SPDA);            \
				__gpio_clear_pin(SPCK);                 \
			}                                               \
		}                                                       \
		__gpio_as_output(SPDA);                                 \
		__gpio_set_pin(SPEN);                                   \
		udelay(400);                                            \
	} while(0)

#define __lcd_special_pin_init()                \
	do {                                            \
		__gpio_as_output(SPEN); /* use SPDA */  \
		__gpio_as_output(SPCK); /* use SPCK */  \
		__gpio_as_output(SPDA); /* use SPDA */  \
		__gpio_as_output(LCD_RET);              \
		udelay(50);                             \
		__gpio_clear_pin(LCD_RET);              \
		udelay(100);                            \
		__gpio_set_pin(LCD_RET);                \
	} while (0)

#define __lcd_special_on()                           \
	do {                                         \
		udelay(50);                          \
		__gpio_clear_pin(LCD_RET);           \
		udelay(100);                         \
		__gpio_set_pin(LCD_RET);             \
} while (0)

#define __lcd_special_off() \
	do { \
		__gpio_clear_pin(LCD_RET);              \
	} while (0)

#endif  /* CONFIG_JZLCD_AUO_A030FL01_V1 */

/* TRULY_TFTG320240DTSW */
#if defined(CONFIG_JZ4750_LCD_TRULY_TFTG320240DTSW_16BIT) || \
	defined(CONFIG_JZ4750_LCD_TRULY_TFTG320240DTSW_18BIT)

#if defined(CONFIG_JZ4750_FUWA)
#define LCD_RESET_PIN   (32*3+25)// LCD_REV, GPD25
#else
#error "Define LCD_RESET_PIN on your board"
#endif

#define __lcd_special_on() \
do { \
	__gpio_as_output(32*3+30);\
	__gpio_clear_pin(32*3+30);\
	__gpio_as_output(LCD_RESET_PIN); \
	__gpio_set_pin(LCD_RESET_PIN); \
	udelay(100); \
	__gpio_clear_pin(LCD_RESET_PIN); \
	udelay(100); \
	__gpio_set_pin(LCD_RESET_PIN); \
} while (0)

#endif /* CONFIG_JZ4750_LCD_TRULY_TFTG320240DTSW */

#if defined(CONFIG_JZ4750_LCD_TOPPOLY_TD025THEA7_RGB_DELTA) || defined(CONFIG_JZ4750_LCD_TOPPOLY_TD025THEA7_RGB_DUMMY)

#if defined(CONFIG_JZ4750_LCD_TOPPOLY_TD025THEA7_RGB_DELTA)
#define PANEL_MODE 0x02         /* RGB Delta */
#elif defined(CONFIG_JZ4750_LCD_TOPPOLY_TD025THEA7_RGB_DUMMY)
#define PANEL_MODE 0x00         /* RGB Dummy */
#endif

#if defined(CONFIG_JZ4750_FUWA) /* board FuWa */
        #define SPEN    (32*3+16)       //LCD_D16 - GPD16
        #define SPCK    (32*3+17)       //LCD_D17 - GPD17
        #define SPDA    (32*3+21)       //LCD_DE  - GPD21
        #define LCD_RET (32*3+25)       //LCD_REV - GPD25  //use for lcd reset
#else
#error "please define SPI pins on your board."
#endif

#define __spi_write_reg1(reg, val) \
	do { \
		unsigned char no;\
		unsigned short value;\
		unsigned char a=0;\
		unsigned char b=0;\
		a=reg;\
		b=val;\
		__gpio_set_pin(SPEN);\
		udelay(100);\
		__gpio_clear_pin(SPCK);\
		__gpio_clear_pin(SPDA);\
		__gpio_clear_pin(SPEN);\
		udelay(25);\
		value=((a<<8)|(b&0xFF));\
		for(no=0;no<16;no++)\
		{\
			__gpio_clear_pin(SPCK);\
			if((value&0x8000)==0x8000)\
				__gpio_set_pin(SPDA);\
			else\
				__gpio_clear_pin(SPDA);\
			udelay(25);\
			__gpio_set_pin(SPCK);\
			value=(value<<1); \
			udelay(25);\
		}\
		__gpio_clear_pin(SPCK);\
		__gpio_set_pin(SPEN);\
		udelay(100);\
	} while (0)

#define __spi_write_reg(reg, val) \
	do {\
		__spi_write_reg1((reg<<2), val); \
		udelay(100); \
	}while(0)
        
#define __lcd_special_pin_init() \
	do { \
		__gpio_as_output(SPEN); /* use SPDA */\
		__gpio_as_output(SPCK); /* use SPCK */\
		__gpio_as_output(SPDA); /* use SPDA */\
		__gpio_as_output(SPDA); /* use reset */\
		__gpio_as_output(LCD_RET); /* use reset */\
		__gpio_set_pin(LCD_RET);\
		mdelay(15);\
		__gpio_clear_pin(LCD_RET);\
		mdelay(15);\
		__gpio_set_pin(LCD_RET);\
	} while (0)

#define __lcd_special_on() \
	do { \
		mdelay(10); \
		__spi_write_reg(0x00, 0x10); \
		__spi_write_reg(0x01, 0xB1); \
		__spi_write_reg(0x00, 0x10); \
		__spi_write_reg(0x01, 0xB1); \
		__spi_write_reg(0x02, PANEL_MODE); /* RGBD MODE */ \
		__spi_write_reg(0x03, 0x01); /* Noninterlace*/  \
		mdelay(10); \
	} while (0)

#define __lcd_special_off() \
	do { \
	} while (0)

#endif  /* CONFIG_JZ4750_LCD_TOPPOLY_TD025THEA7_RGB_DELTA */

#ifndef __lcd_special_pin_init
#define __lcd_special_pin_init()
#endif
#ifndef __lcd_special_on
#define __lcd_special_on()
#endif
#ifndef __lcd_special_off
#define __lcd_special_off()
#endif


/*
 * Platform specific definition
 */
#if defined(CONFIG_JZ4750D_VGA_DISPLAY)
#define __lcd_display_pin_init()
#define __lcd_display_on()
#define __lcd_display_off()

#elif defined(CONFIG_JZ4750_APUS)/* board apus */
#define __lcd_display_pin_init() \
do { \
	__gpio_as_output(GPIO_LCD_VCC_EN_N);     \
	__lcd_special_pin_init();          \
} while (0)

#define __lcd_display_on() \
do { \
	__gpio_clear_pin(GPIO_LCD_VCC_EN_N);    \
	__lcd_special_on();                     \
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_special_off();     \
} while (0)

#elif defined(CONFIG_JZ4750D_KMP510)
#define __lcd_display_pin_init() \
do { \
	__gpio_as_output((2*32)+23);     \
} while (0)

#elif defined(CONFIG_JZ4750D_CETUS)/* board apus */
#define __lcd_display_pin_init() \
do { \
	__gpio_as_output(GPIO_LCD_VCC_EN_N);     \
	__lcd_special_pin_init();          \
} while (0)

#define __lcd_display_on() \
do { \
	__gpio_set_pin(GPIO_LCD_VCC_EN_N);      \
	__lcd_special_on();                     \
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_special_off();     \
} while (0)

#else /* ===================== other boards =================================*/

#define __lcd_display_pin_init() \
do { \
	__lcd_special_pin_init();          \
} while (0)
#define __lcd_display_on() \
do { \
	__lcd_special_on();                     \
	__lcd_set_backlight_level(100);         \
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_close_backlight();           \
	__lcd_special_off();     \
} while (0)
#endif /* APUS */

/*****************************************************************************
 * LCD display pin dummy macros
 *****************************************************************************/

#ifndef __lcd_display_pin_init
#define __lcd_display_pin_init()
#endif
#ifndef __lcd_slcd_special_on
#define __lcd_slcd_special_on()
#endif
#ifndef __lcd_display_on
#define __lcd_display_on()
#endif
#ifndef __lcd_display_off
#define __lcd_display_off()
#endif
#ifndef __lcd_set_backlight_level
#define __lcd_set_backlight_level(n)
#endif
