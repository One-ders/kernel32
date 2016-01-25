
#define CONFIG_JZLCD_FRAMEBUFFER_MAX 1
#define NR_PALETTE		256

struct lcd_desc {
        unsigned int next_desc; /* LCDDAx */
        unsigned int databuf;   /* LCDSAx */
        unsigned int frame_id;  /* LCDFIDx */ 
        unsigned int cmd;       /* LCDCMDx */
};

#define MODE_MASK               0x0f
#define MODE_TFT_GEN            0x00
#define MODE_TFT_SHARP          0x01
#define MODE_TFT_CASIO          0x02
#define MODE_TFT_SAMSUNG        0x03
#define MODE_CCIR656_NONINT     0x04
#define MODE_CCIR656_INT        0x05
#define MODE_STN_COLOR_SINGLE   0x08
#define MODE_STN_MONO_SINGLE    0x09
#define MODE_STN_COLOR_DUAL     0x0a
#define MODE_STN_MONO_DUAL      0x0b
#define MODE_8BIT_SERIAL_TFT    0x0c

#define MODE_TFT_18BIT          (1<<7)

#define STN_DAT_PIN1    (0x00 << 4)
#define STN_DAT_PIN2    (0x01 << 4)
#define STN_DAT_PIN4    (0x02 << 4)
#define STN_DAT_PIN8    (0x03 << 4)
#define STN_DAT_PINMASK STN_DAT_PIN8

#define STFT_PSHI       (1 << 15)
#define STFT_CLSHI      (1 << 14)
#define STFT_SPLHI      (1 << 13)
#define STFT_REVHI      (1 << 12)

#define DE_P            (0 << 9)
#define DE_N            (1 << 9)

#define PCLK_P          (0 << 10)
#define PCLK_N          (1 << 10)

#define HSYNC_P         (0 << 11)
#define HSYNC_N         (1 << 11)

#define VSYNC_P         (0 << 8)
#define VSYNC_N         (1 << 8)

#define DATA_NORMAL     (0 << 17)
#define DATA_INVERSE    (1 << 17)

/* Jz LCDFB supported I/O controls. */
#define FBIOSETBACKLIGHT        0x4688
#define FBIODISPON              0x4689
#define FBIODISPOFF             0x468a
#define FBIORESET               0x468b
#define FBIOPRINT_REGS          0x468c
#define FBIOGETBUFADDRS         0x468d

struct jz_lcd_buffer_addrs_t {
	int fb_num;
	unsigned int lcd_desc_phys_addr;
	unsigned int fb_phys_addr[CONFIG_JZLCD_FRAMEBUFFER_MAX];
};


