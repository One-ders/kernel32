/* $TSOS: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)lcd_drv.c
 */
#include "sys.h"
#include "irq.h"
#include "jz4750_lcd.h"
#include "jz4750_tve.h"

#include "jz4750_clocks.h"
#include "clocks.h"

#include <devices.h>
#include <fb.h>

#include <string.h>
#include <malloc.h>

#if 0
#ifndef __lcd_special_pin_init
#define __lcd_special_pin_init()
#endif
#ifndef __lcd_special_on
#define __lcd_special_on()
#endif
#ifndef __lcd_special_off
#define __lcd_special_off()
#endif


struct lcd_cfb_info {
	struct fb_info		fb;
	struct display_switch	*dispsw;
	int			currcon;
	int			func_use_count;
	struct {
		unsigned short red,green,blue;
	} palette[NR_PALETTE];
        int b_lcd_display;
        int b_lcd_pwm;
        int backlight_level;
};

static struct lcd_cfb_info	*jzlcd_info;

#define CFG_MODE_18B		0x00000080

#define CFG_VSYNC_SHIFT		8
#define CFG_VSYNC_MASK		0x00000100
#define CFG_VSYNC_N		(1<<CFG_VSYNC_SHIFT)
#define CFG_HSYNC_SHIFT		11
#define CFG_HSYNC_MASK		0x00000800
#define CFG_HSYNC_N		(1<<CFG_HSYNC_SHIFT)

struct jzfb_info {
	unsigned int cfg;
	unsigned int w;
	unsigned int h;
	unsigned int bpp;
	unsigned int fclk;
	unsigned int hsw;
	unsigned int vsw;
	unsigned int elw;
	unsigned int blw;
	unsigned int efw;
	unsigned int bfw;
};

// pavo have SAMSUNG_LTP400WQF02
static struct jzfb_info jzfb = {
	CFG_MODE_18B|CFG_HSYNC_N|CFG_VSYNC_N,
	480,
	272,
	18,
	60,
	41,
	10,
	2,
	2,
	2,
	2
};

static struct lcd_desc *lcd_desc_base;
static struct lcd_desc *lcd_palette_desc;
static struct lcd_desc *lcd_frame_desc0;
static struct lcd_desc *lcd_frame_desc1;

static volatile unsigned char *lcd_palette;
static volatile unsigned char *lcd_frame[CONFIG_JZLCD_FRAMEBUFFER_MAX];

struct jz_lcd_buffer_addrs_t jz_lcd_buffer_addrs;

static struct driver *pindrv;
static struct device_handle *pin_dh;


#define GPIO_PWM	123
#define PWM_CHN		4
#define PWM_FULL	101

#define __lcd_set_backlight_level(n) \
  do { \
	__gpio_as_output(32*3+27); \
	__gpio_set_pin(32*3+27); \
   } while (0)

#define __lcd_close_backlight() \
do { \
	__gpio_as_output(GPIO_PWM); \
	__gpio_clear_pin(GPIO_PWM); \
  } while (0)

#define __lcd_display_pin_init() \
   do { \
	__gpio_as_output(GPIO_DISP_OFF_N); \
	__pmc_start_tcu(); \
	__lcd_special_pin_init(); \
   } while (0)

#define __lcd_display_on() \
do { \
        __gpio_set_pin(GPIO_DISP_OFF_N); \
        __lcd_special_on(); \
        __lcd_set_backlight_level(80); \
} while (0)

#define __lcd_display_off() \
do { \
        __lcd_special_off(); \
        __lcd_close_backlight(); \
        __gpio_clear_pin(GPIO_DISP_OFF_N); \
} while (0)

static int lcd_set_ena() {
	LCDC->ctrl|=LCDC_CTRL_ENA;
	return 0;
}

static int lcd_clr_ena() {
	LCDC->ctrl&=~LCDC_CTRL_ENA;
	return 0;
}

static int lcd_enable_ofu_intr() {
	LCDC->ctrl|=LCDC_CTRL_OFUM;
	return 0;
}

static int lcd_disable_ofu_intr() {
	LCDC->ctrl&=~LCDC_CTRL_OFUM;
	return 0;
}

static int jzfb_setcolreg(unsigned int regno, unsigned int red,
			  unsigned int green, unsigned int blue,
			  unsigned int transp, struct fb_info *info) {

	struct lcd_cfb_info *cfb =(struct lcd_cfb_info *)info;
	unsigned short *ptr, ctmp;

	if(regno>=NR_PALETTE) return 1;

	cfb->palette[regno].red		= red;
	cfb->palette[regno].green	= green;
	cfb->palette[regno].blue	= blue;

	if (cfb->fb.var.bits_per_pixel <= 16) {
		red	>>= 8;
		green	>>= 8;
		blue	>>= 8;

		red	&= 0xff;
		green	&= 0xff;
		blue	&= 0xff;
	}

	switch (cfb->fb.var.bits_per_pixel) {
		case 1:
		case 2:
		case 4:
		case 8:
			if (((jzfb.cfg&MODE_MASK)==MODE_STN_MONO_SINGLE) ||
				((jzfb.cfg&MODE_MASK)==MODE_STN_MONO_DUAL)) {
				ctmp=(77L*red + 150L*green + 29L*blue)>>8;
				ctmp=((ctmp>>3)<<11) | ((ctmp>>2)<<5) |
					(ctmp>>3);
			} else {
				/* RGB 565 */
				if (((red>>3)==0) && ((red>>2)!=0))
					red=1<<3;
				if (((blue>>3)==0) && ((blue>>2)!=0))
					blue=1<<3;
				ctmp=((red>>3)<<11) |
					((green>>2)<<5) |
					(blue >> 3);
			}

			ptr = (unsigned short *)lcd_palette;
			ptr = (unsigned short *)(((unsigned int)ptr)|0xa0000000);
			ptr[regno] = ctmp;

			break;
		case 15:
			if (regno<16) {
				((unsigned int *)cfb->fb.pseudo_palette)[regno] =
					((red >> 3) << 10) |
					((green >> 3) << 5) |
					(blue >> 3);
			}
			break;
		case 16:
			if (regno<16) {
				((unsigned int *)cfb->fb.pseudo_palette)[regno] =
					((red >> 3) << 11) |
					((green >> 2) << 5) |
					(blue >> 3);
			}
			break;
		case 18:
		case 24:
		case 32:
			if (regno<16) {
				((unsigned int *)cfb->fb.pseudo_palette)[regno] =
					(red << 16) |
					(green << 8) |
					(blue << 0);
			}
			break;
	}
	return 0;
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg) {
	int ret = 0;
	void *argp = (void *)arg;

	switch (cmd) {
		case FBIOSETBACKLIGHT:
			__lcd_set_backlight_level(arg); /* We support 8 levels here. */
			break;
		case FBIODISPON:
			__lcd_display_on();
			break;
		case FBIODISPOFF:
			__lcd_display_off();
			break;
//		case FBIOPRINT_REGS:
//			print_regs();
//			break;
		case FBIOGETBUFADDRS:
			if ( copy_to_user(argp, &jz_lcd_buffer_addrs,
				sizeof(struct jz_lcd_buffer_addrs_t)) )
				return -EFAULT;
			break;
		default:
			sys_printf("Warn: Command(%x) not support\n", cmd);
			break;
	}
	return ret;
}

static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned long start;
	unsigned long off;
	unsigned int len;

	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = cfb->fb.fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + cfb->fb.fix.smem_len);
	start &= PAGE_MASK;

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

#if 0
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                               vma->vm_end - vma->vm_start,
                               vma->vm_page_prot)) {
                return -EAGAIN;
        }
#endif
        return 0;
}



static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info) {
	sys_printf("jzfb_check_var\n");
	return 0;
}

static int jzfb_set_par(struct fb_info *info) {
	sys_printf("jzfb_set_par\n");
	return 0;
}

static int jzfb_blank(int blank_mode, struct fb_info *info) {
	switch(blank_mode) {
		case FB_BLANK_UNBLANK:
			lcd_set_ena();
			__lcd_display_on();
			break;
		case FB_BLANK_NORMAL:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_POWERDOWN:
			break;
		default:
			break;
	}
	return 0;
}

static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info) {
	struct lcd_cfb_info *cfb=(struct lcd_cfb_info *)info;
	int dy;

	if (!var || !cfb) return -EINVAL;

	if (var->xoffset - cfb->fb.var.xoffset) {
		return -EINVAL;
	}

	dy=var->yoffset-cfb->fb.var.yoffset;
	if (dy) {
		lcd_frame_desc0->databuf += (cfb->fb.fix.line_length * dy);
	}
	return 0;
}

static struct fb_ops jzfb_ops = {
	.fb_setcolreg           = jzfb_setcolreg,
	.fb_check_var           = jzfb_check_var,
	.fb_set_par             = jzfb_set_par,
	.fb_blank               = jzfb_blank,
	.fb_pan_display         = jzfb_pan_display,
	.fb_fillrect            = cfb_fillrect,
	.fb_copyarea            = cfb_copyarea,
	.fb_imageblit           = cfb_imageblit,
	.fb_mmap                = jzfb_mmap,
	.fb_ioctl               = jzfb_ioctl,
};

static int jzfb_set_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
        //struct display *display;
	int chgvar = 0;

	var->height                 = jzfb.h ;
	var->width                  = jzfb.w ;
	var->bits_per_pixel         = jzfb.bpp;

	var->vmode                  = FB_VMODE_NONINTERLACED;
	var->activate               = cfb->fb.var.activate;
	var->xres                   = var->width;
	var->yres                   = var->height;
	var->xres_virtual           = var->width;
	var->yres_virtual           = var->height;
	var->xoffset                = 0;
	var->yoffset                = 0;
	var->pixclock               = 0;
	var->left_margin            = 0;
	var->right_margin           = 0;
	var->upper_margin           = 0;
	var->lower_margin           = 0;
	var->hsync_len              = 0;
	var->vsync_len              = 0;
	var->sync                   = 0;
	var->activate              &= ~FB_ACTIVATE_TEST;

	/*
	 * CONUPDATE and SMOOTH_XPAN are equal.  However,
         * SMOOTH_XPAN is only used internally by fbcon.
         */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = cfb->fb.var.xoffset;
		var->yoffset = cfb->fb.var.yoffset;
	}

	if (var->activate & FB_ACTIVATE_TEST)
		return 0;

	if ((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NOW)
		return -EINVAL;

	if (cfb->fb.var.xres != var->xres)
		chgvar = 1;
	if (cfb->fb.var.yres != var->yres)
		chgvar = 1;
	if (cfb->fb.var.xres_virtual != var->xres_virtual)
		chgvar = 1;
	if (cfb->fb.var.yres_virtual != var->yres_virtual)
		chgvar = 1;
	if (cfb->fb.var.bits_per_pixel != var->bits_per_pixel)
		chgvar = 1;

	//display = fb_display + con;

	var->red.msb_right      = 0;
        var->green.msb_right    = 0;
        var->blue.msb_right     = 0;

        switch(var->bits_per_pixel){
        case 1: /* Mono */
                cfb->fb.fix.visual      = FB_VISUAL_MONO01;
                cfb->fb.fix.line_length = (var->xres * var->bits_per_pixel) / 8;
                break;
        case 2: /* Mono */
                var->red.offset         = 0;
                var->red.length         = 2;
                var->green.offset       = 0;
                var->green.length       = 2;
                var->blue.offset        = 0;
                var->blue.length        = 2;

                cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
                cfb->fb.fix.line_length = (var->xres * var->bits_per_pixel) / 8;
                break;
        case 4: /* PSEUDOCOLOUR*/
                var->red.offset         = 0;
                var->red.length         = 4;
                var->green.offset       = 0;
                var->green.length       = 4;
                var->blue.offset        = 0;
                var->blue.length        = 4;

                cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
                cfb->fb.fix.line_length = var->xres / 2;
                break;
        case 8: /* PSEUDOCOLOUR, 256 */
                var->red.offset         = 0;
                var->red.length         = 8;
                var->green.offset       = 0;
                var->green.length       = 8;
                var->blue.offset        = 0;
                var->blue.length        = 8;

		cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
                cfb->fb.fix.line_length = var->xres ;
                break;
        case 15: /* DIRECTCOLOUR, 32k */
                var->bits_per_pixel     = 15;
                var->red.offset         = 10;
                var->red.length         = 5;
                var->green.offset       = 5;
                var->green.length       = 5;
                var->blue.offset        = 0;
                var->blue.length        = 5;

                cfb->fb.fix.visual      = FB_VISUAL_DIRECTCOLOR;
                cfb->fb.fix.line_length = var->xres_virtual * 2;
                break;
        case 16: /* DIRECTCOLOUR, 64k */
                var->bits_per_pixel     = 16;
                var->red.offset         = 11;
                var->red.length         = 5;
                var->green.offset       = 5;
                var->green.length       = 6;
                var->blue.offset        = 0;
                var->blue.length        = 5;

                cfb->fb.fix.visual      = FB_VISUAL_TRUECOLOR;
                cfb->fb.fix.line_length = var->xres_virtual * 2;
                break;
        case 18:
        case 24:
        case 32:
                /* DIRECTCOLOUR, 256 */
//              var->bits_per_pixel     = 18;
		var->bits_per_pixel     = 32;

                var->red.offset         = 16;
                var->red.length         = 8;
                var->green.offset       = 8;
                var->green.length       = 8;
                var->blue.offset        = 0;
                var->blue.length        = 8;
                var->transp.offset      = 24;
                var->transp.length      = 8;

                cfb->fb.fix.visual      = FB_VISUAL_TRUECOLOR;
                cfb->fb.fix.line_length = var->xres_virtual * 4;
                break;

        default: /* in theory this should never happen */
		sys_printf("%s: don't support for %dbpp\n",
                       cfb->fb.fix.id, var->bits_per_pixel);
                break;
        }

        cfb->fb.var = *var;
        cfb->fb.var.activate &= ~FB_ACTIVATE_ALL;

	/*
	 * Update the old var.  The fbcon drivers still use this.
	 * Once they are using cfb->fb.var, this can be dropped.
	 *                                      --rmk
	 */
	//display->var = cfb->fb.var;
	/*
	 * If we are setting all the virtual consoles, also set the
	 * defaults used to create new consoles.
	 */
	fb_set_cmap(&cfb->fb.cmap, &cfb->fb);

	return 0;
}


static struct lcd_cfb_info *fb_alloc_fb_info(void) {
	struct lcd_cfb_info *cfb;

	cfb=get_page();
	if (!cfb) return 0;

	jzlcd_info=cfb;

	memset(cfb,0,sizeof(struct lcd_cfb_info));

	cfb->currcon=-1;

	strcpy(cfb->fb.fix.id,"jz-lcd");
	cfb->fb.fix.type	= FB_TYPE_PACKED_PIXELS;
	cfb->fb.fix.type_aux	= 0;
	cfb->fb.fix.xpanstep	= 1;
	cfb->fb.fix.ypanstep	= 1;
	cfb->fb.fix.ywrapstep	= 0;
	cfb->fb.fix.accel	= FB_ACCEL_NONE;

	cfb->fb.var.nonstd	= 0;
	cfb->fb.var.activate	= FB_ACTIVATE_NOW;
	cfb->fb.var.height	= -1;
	cfb->fb.var.width	= -1;
	cfb->fb.var.accel_flags	= FB_ACCELF_TEXT;

	cfb->fb.fbops		= &jzfb_ops;
	cfb->fb.flags		= FBINFO_FLAG_DEFAULT;

	cfb->fb.pseudo_palette	= (void *)(cfb+1);

	switch (jzfb.bpp) {
		case 1:
			fb_alloc_cmap(&cfb->fb.cmap,4,0);
			break;
		case 2:
			fb_alloc_cmap(&cfb->fb.cmap,8,0);
			break;
		case 4:
			fb_alloc_cmap(&cfb->fb.cmap,32,0);
			break;
		case 8:
		default:
			fb_alloc_cmap(&cfb->fb.cmap,256,0);
			break;
	}

	return cfb;
}


/*
 * Map screen memory
 */
static int fb_map_smem(struct lcd_cfb_info *cfb) {
	struct page *map = NULL;
	unsigned char *tmp;
	unsigned int page_shift, needroom, t;

	if (jzfb.bpp == 18 || jzfb.bpp == 24) {
		t = 32;
	} else {
		t = jzfb.bpp;
	}

	needroom = ((jzfb.w * t + 7) >> 3) * jzfb.h;
	for (page_shift = 0; page_shift < 12; page_shift++) {
		if ((PAGE_SIZE << page_shift) >= needroom) {
			break;
		}
	}

        /* lcd_palette room:
         * 0 -- 512: lcd palette
         * 1024 -- [1024+16*3]: lcd descripters
         * [1024+16*3] -- 4096: reserved
         */
        lcd_palette = (unsigned char *)get_page();
	if ((!lcd_palette)) {
		return -ENOMEM;
	}

	memset((void *)lcd_palette, 0, PAGE_SIZE);
//	map = virt_to_page(lcd_palette);
 //       set_bit(PG_reserved, &map->flags);
	lcd_desc_base  = (struct lcd_desc *)(lcd_palette + 1024);

	jz_lcd_buffer_addrs.fb_num = CONFIG_JZLCD_FRAMEBUFFER_MAX;
	/* alloc frame buffer space */
	for ( t = 0; t < CONFIG_JZLCD_FRAMEBUFFER_MAX; t++ ) {
		lcd_frame[t] = (unsigned char *)get_pages(page_shift);
		if ((!lcd_frame[t])) {
			sys_printf("no mem for fb[%d]\n", t);
			return -ENOMEM;
		}
		memset((void *)lcd_frame[t], 0, PAGE_SIZE << page_shift);
		for (tmp=(unsigned char *)lcd_frame[t];
			tmp < lcd_frame[t] + (PAGE_SIZE << page_shift);
			tmp += PAGE_SIZE) {
//			map = virt_to_page(tmp);
//			set_bit(PG_reserved, &map->flags);
		}
		jz_lcd_buffer_addrs.fb_phys_addr[t] = virt_to_phys((void *)lcd_frame[t]);
	}

	cfb->fb.fix.smem_start = virt_to_phys((void *)lcd_frame[0]);
	cfb->fb.fix.smem_len = (PAGE_SIZE << page_shift);
	// Move address to uncached space
	cfb->fb.screen_base =
	(unsigned char *)(((unsigned int)lcd_frame[0] & 0x1fffffff) | 0xa0000000);
	if (!cfb->fb.screen_base) {
		sys_printf("%s: unable to map screen memory\n", cfb->fb.fix.id);
		return -ENOMEM;
	}

	return 0;
}

static void fb_free_fb_info(struct lcd_cfb_info *cfb) {
	if (cfb) {
		fb_alloc_cmap(&cfb->fb.cmap, 0, 0);
		free(cfb);
	}
}

static void fb_unmap_smem(struct lcd_cfb_info *cfb) {
	struct page * map = NULL;
	unsigned char *tmp;
	unsigned int page_shift, needroom, t;

	if (jzfb.bpp == 18 || jzfb.bpp == 24)
		t = 32;
	else
		t = jzfb.bpp;

	needroom = ((jzfb.w * t + 7) >> 3) * jzfb.h;
	for (page_shift = 0; page_shift < 12; page_shift++)
		if ((PAGE_SIZE << page_shift) >= needroom)
			break;

	if (cfb && cfb->fb.screen_base) {
//		iounmap(cfb->fb.screen_base);
		cfb->fb.screen_base = NULL;
//		release_mem_region(cfb->fb.fix.smem_start,
//		cfb->fb.fix.smem_len);
	}

	if (lcd_palette) {
//		map = virt_to_page(lcd_palette);
//		clear_bit(PG_reserved, &map->flags);
//		free_pages((int)lcd_palette, 0);
	}

	for ( t=0; t < CONFIG_JZLCD_FRAMEBUFFER_MAX; t++ ) {
		if (lcd_frame[t]) {
			for (tmp=(unsigned char *)lcd_frame[t];
				tmp < lcd_frame[t] + (PAGE_SIZE << page_shift);
				tmp += PAGE_SIZE) {
//				map = virt_to_page(tmp);
//				clear_bit(PG_reserved, &map->flags);
			}
//			free_pages((int)lcd_frame[t], page_shift);
		}
	}
}

static void lcd_descriptor_init(void) {
	int i;
	unsigned int pal_size;
	unsigned int frm_size, ln_size;
	unsigned char dual_panel = 0;

	i = jzfb.bpp;
        if (i == 18 || i == 24)
                i = 32;

        frm_size = (jzfb.w*jzfb.h*i)>>3;
        ln_size = (jzfb.w*i)>>3;

        if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
            ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL)) {
                dual_panel = 1;
                frm_size >>= 1;
        }

        frm_size = frm_size / 4;
        ln_size = ln_size / 4;

        switch (jzfb.bpp) {
        case 1:
                pal_size = 4;
                break;
        case 2:
                pal_size = 8;
                break;
        case 4:
                pal_size = 32;
                break;
        case 8:
        default:
                pal_size = 512;
        }

        pal_size /= 4;

	lcd_frame_desc0  = lcd_desc_base + 0;
        lcd_frame_desc1  = lcd_desc_base + 1;
        lcd_palette_desc = lcd_desc_base + 2;

        jz_lcd_buffer_addrs.lcd_desc_phys_addr = (unsigned int)virt_to_phys(lcd_frame_desc0);

        /* Palette Descriptor */
        lcd_palette_desc->next_desc = (int)virt_to_phys(lcd_frame_desc0);
        lcd_palette_desc->databuf = (int)virt_to_phys((void *)lcd_palette);
        lcd_palette_desc->frame_id = (unsigned int)0xdeadbeaf;
        lcd_palette_desc->cmd = pal_size|LCDC_CMD_PAL; /* Palette Descriptor */

        /* Frame Descriptor 0 */
        if (jzfb.bpp <= 8)
                lcd_frame_desc0->next_desc = (int)virt_to_phys(lcd_palette_desc);
        else
                lcd_frame_desc0->next_desc = (int)virt_to_phys(lcd_frame_desc0);
        lcd_frame_desc0->databuf = virt_to_phys((void *)lcd_frame[0]);
        lcd_frame_desc0->frame_id = (unsigned int)0xbeafbeaf;
        lcd_frame_desc0->cmd = LCDC_CMD_SOFINT | LCDC_CMD_EOFINT | frm_size;
//        dma_cache_wback((unsigned int)(lcd_palette_desc),0x10);
//        dma_cache_wback((unsigned int)(lcd_frame_desc0),0x10);

        if (!(dual_panel))
                return;

        /* Frame Descriptor 1 */
        lcd_frame_desc1->next_desc = (int)virt_to_phys(lcd_frame_desc1);
        lcd_frame_desc1->databuf = virt_to_phys((void *)(lcd_frame[0] + frm_size * 4));
        lcd_frame_desc1->frame_id = (unsigned int)0xdeaddead;
        lcd_frame_desc1->cmd = LCDC_CMD_SOFINT | LCDC_CMD_EOFINT | frm_size;
//        dma_cache_wback((unsigned int)(lcd_frame_desc1),0x10);
}

static int lcd_hw_init(void) {
	unsigned int val = 0;
	unsigned int pclk;
	unsigned int stnH;
	int ret = 0;
	int pll_div;
        /* Setting Control register */
	switch (jzfb.bpp) {
	case 1:
		val |= LCDC_CTRL_BPP_1;
		break;
	case 2:
		val |= LCDC_CTRL_BPP_2;
		break;
	case 4:
		val |= LCDC_CTRL_BPP_4;
		break;
	case 8:
		val |= LCDC_CTRL_BPP_8;
		break;
	case 15:
		val |= LCDC_CTRL_RGB555;
	case 16:
		val |= LCDC_CTRL_BPP_16;
		break;
	case 17 ... 32:
		val |= LCDC_CTRL_BPP_18_24;      /* target is 4bytes/pixel */
		break;
	default:
		sys_printf("The BPP %d is not supported\n", jzfb.bpp);
		val |= LCDC_CTRL_BPP_16;
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.bpp) {
		case 1:
		case 2:
			val |= LCDC_CTRL_FRC_2;
			break;
		case 4:
			val |= LCDC_CTRL_FRC_4;
			break;
		case 8:
		default:
			val |= LCDC_CTRL_FRC_16;
			break;
		}
		break;
	}

	val |= LCDC_CTRL_BST_16;         /* Burst Length is 16WORD=64Byte */

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.cfg & STN_DAT_PINMASK) {
#define align2(n) (n)=((((n)+1)>>1)<<1)
#define align4(n) (n)=((((n)+3)>>2)<<2)
#define align8(n) (n)=((((n)+7)>>3)<<3)
		case STN_DAT_PIN1:
			/* Do not adjust the hori-param value. */
			break;
		case STN_DAT_PIN2:
			align2(jzfb.hsw);
			align2(jzfb.elw);
			align2(jzfb.blw);
			break;
		case STN_DAT_PIN4:
			align4(jzfb.hsw);
			align4(jzfb.elw);
			align4(jzfb.blw);
			break;
		case STN_DAT_PIN8:
			align8(jzfb.hsw);
			align8(jzfb.elw);
			align8(jzfb.blw);
			break;
		}
		break;
	}

	val |=  LCDC_CTRL_OFUP;         /* Output FIFO underrun protection */
	LCDC->ctrl = val;

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		if (((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL) ||
			((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL)) {
			stnH = jzfb.h >> 1;
                } else {
			stnH = jzfb.h;
		}

                LCDC->vsync = (0 << 16) | jzfb.vsw;
                LCDC->hsync = ((jzfb.blw+jzfb.w) << 16) | (jzfb.blw+jzfb.w+jzfb.hsw);

		/* Screen setting */
		LCDC->vat = ((jzfb.blw + jzfb.w + jzfb.hsw + jzfb.elw) << 16) |
				(stnH + jzfb.vsw + jzfb.bfw + jzfb.efw);
		LCDC->dah = (jzfb.blw << 16) | (jzfb.blw + jzfb.w);
                LCDC->dav = (0 << 16) | (stnH);

		/* AC BIAs signal */
		LCDC->ps = (0 << 16) | (stnH+jzfb.vsw+jzfb.efw+jzfb.bfw);

		break;

	case MODE_TFT_GEN:
	case MODE_TFT_SHARP:
	case MODE_TFT_CASIO:
	case MODE_TFT_SAMSUNG:
	case MODE_8BIT_SERIAL_TFT:
	case MODE_TFT_18BIT:
		LCDC->vsync = (0 << 16) | jzfb.vsw;
#if defined(CONFIG_JZLCD_INNOLUX_AT080TN42)
		LCDC->dav = (0 << 16) | ( jzfb.h );
#else
		LCDC->dav = ((jzfb.vsw + jzfb.bfw) << 16) |
				(jzfb.vsw + jzfb.bfw + jzfb.h);
#endif /*#if defined(CONFIG_JZLCD_INNOLUX_AT080TN42)*/
		LCDC->vat =(((jzfb.blw+jzfb.w+jzfb.elw+jzfb.hsw))<<16) |
			 (jzfb.vsw + jzfb.bfw + jzfb.h + jzfb.efw);
		LCDC->hsync = (0 << 16) | jzfb.hsw;
		LCDC->dah = ((jzfb.hsw + jzfb.blw) << 16) |
				(jzfb.hsw + jzfb.blw + jzfb.w);
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
        case MODE_TFT_SAMSUNG: {
		unsigned int total, tp_s, tp_e, ckv_s, ckv_e;
		unsigned int rev_s, rev_e, inv_s, inv_e;
		total = jzfb.blw + jzfb.w + jzfb.elw + jzfb.hsw;
		tp_s = jzfb.blw + jzfb.w + 1;
		tp_e = tp_s + 1;
		ckv_s = tp_s - clocks_get_pixclk()/(1000000000/4100);
		ckv_e = tp_s + total;
		rev_s = tp_s - 11;      /* -11.5 clk */
		rev_e = rev_s + total;
		inv_s = tp_s;
		inv_e = inv_s + total;
		LCDC->cls = (tp_s << 16) | tp_e;
		LCDC->ps = (ckv_s << 16) | ckv_e;
		LCDC->spl = (rev_s << 16) | rev_e;
		LCDC->rev = (inv_s << 16) | inv_e;
		jzfb.cfg |= STFT_REVHI | STFT_SPLHI;
		break;
	}
	case MODE_TFT_SHARP: {
		unsigned int total, cls_s, cls_e, ps_s, ps_e;
		unsigned int spl_s, spl_e, rev_s, rev_e;
		total = jzfb.blw + jzfb.w + jzfb.elw + jzfb.hsw;
#if !defined(CONFIG_JZLCD_INNOLUX_AT080TN42)
		spl_s = 1;
		spl_e = spl_s + 1;
		cls_s = 0;
		cls_e = total - 60;     /* > 4us (pclk = 80ns) */
		ps_s = cls_s;
		ps_e = cls_e;
		rev_s = total - 40;     /* > 3us (pclk = 80ns) */
		rev_e = rev_s + total;
		jzfb.cfg |= STFT_PSHI;
#else           /*#if defined(CONFIG_JZLCD_INNOLUX_AT080TN42)*/
		spl_s = total - 5; /* LD */
		spl_e = total - 3;
		cls_s = 32;     /* CKV */
		cls_e = 145;
		ps_s  = 0;      /* OEV */
		ps_e  = 45;
		rev_s = 0;      /* POL */
		rev_e = 0;
#endif          /*#if defined(CONFIG_JZLCD_INNOLUX_AT080TN42)*/
		LCDC->spl = (spl_s << 16) | spl_e;
		LCDC->cls = (cls_s << 16) | cls_e;
		LCDC->ps = (ps_s << 16) | ps_e;
		LCDC->rev = (rev_s << 16) | rev_e;
		break;
	}
	case MODE_TFT_CASIO:
		break;
	} // end switch

	/* Configure the LCD panel */
	LCDC->cfg = jzfb.cfg;

	/* Timing setting */
	__pmc_stop_lcd();

	val = jzfb.fclk; /* frame clk */

	if ( (jzfb.cfg & MODE_MASK) != MODE_8BIT_SERIAL_TFT) {
		pclk = val * (jzfb.w + jzfb.hsw + jzfb.elw + jzfb.blw) *
			(jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */
	} else {
		/* serial mode: Hsync period = 3*Width_Pixel */
		pclk = val * (jzfb.w*3 + jzfb.hsw + jzfb.elw + jzfb.blw) *
			(jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */
	}
//      pclk = val * (jzfb.w + jzfb.hsw + jzfb.elw + jzfb.blw) *
//              (jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */
	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
		((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
		pclk = (pclk * 3);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
		((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
		((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
		((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk = pclk >> ((jzfb.cfg & STN_DAT_PINMASK) >> 4);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
		((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk >>= 1;

	pll_div = ( CGU->cpccr & CGU_CPCCR_PCS ); /* clock source,0:pllout/2 1: pllout */
	pll_div = pll_div ? 1 : 2 ;
	val = ( cgu_get_pllout()/pll_div ) / pclk;
	val--;
	if ( val > 0x1ff ) {
		sys_printf("CPM_LPCDR too large, set it to 0x1ff\n");
		val = 0x1ff;
	}
	__cgu_set_pixdiv(val);

	val = pclk * 3 ;        /* LCDClock > 2.5*Pixclock */
	val = cgu_get_pllout() / val;

	__cgu_set_ldiv( val );
	CGU->cpccr |= CGU_CPCCR_CE ; /* update divide */

	clocks_set_pixclk(cgu_get_pixclk());
	clocks_set_lcdclk(cgu_get_lcdclk());
	sys_printf("LCDC: PixClock:%d LcdClock:%d\n",
		clocks_get_pixclk(), clocks_get_lcdclk());

	__pmc_start_lcd();
	sys_udelay(1000);
	return ret;
}

#endif
/////////////////

#undef DEBUG

#ifdef DEBUG
#define dprintk(x...)   printk(x)
#define print_dbg(f, arg...) printk("dbg::" __FILE__ ",LINE(%d): " f "\n", __LINE__, ## arg)
#else
#define dprintk(x...)
#define print_dbg(f, arg...) do {} while (0)
#endif

#define print_err(f, arg...) printk(KERN_ERR DRIVER_NAME ": " f "\n", ## arg)
#define print_warn(f, arg...) printk(KERN_WARNING DRIVER_NAME ": " f "\n", ## arg)
#define print_info(f, arg...) printk(KERN_INFO DRIVER_NAME ": " f "\n", ## arg)

struct lcd_cfb_info {
        struct fb_info          fb;
        struct display_switch   *dispsw;
        signed int              currcon;
        int                     func_use_count;

        struct {
                u16 red, green, blue;
        } palette[NR_PALETTE];
#ifdef CONFIG_PM
        struct pm_dev           *pm;
#endif
        int b_lcd_display;
        int b_lcd_pwm;
        int backlight_level;
};


static struct lcd_cfb_info *jz4750fb_info;
static struct jz4750_lcd_dma_desc *dma_desc_base;
static struct jz4750_lcd_dma_desc *dma0_desc_palette, *dma0_desc0, *dma0_desc1, *dma1_desc0, *dma1_desc1;
#define DMA_DESC_NUM            6

static unsigned char *lcd_palette;
static unsigned char *lcd_frame0;
static unsigned char *lcd_frame1;

static struct jz4750_lcd_dma_desc *dma0_desc_cmd0, *dma0_desc_cmd;
static unsigned char *lcd_cmdbuf ;

static void jz4750fb_set_mode( struct jz4750lcd_info * lcd_info );
static void jz4750fb_deep_set_mode( struct jz4750lcd_info * lcd_info );

static int jz4750fb_set_backlight_level(int n);
static int jz4750fb_lcd_display_on(void);
static int jz4750fb_lcd_display_off(void);

struct jz4750lcd_info jz4750_lcd_panel = {
#if defined(CONFIG_JZ4750_LCD_SAMSUNG_LTP400WQF02)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
                LCD_CFG_MODE_TFT_18BIT |        /* output 18bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP,    /* Vsync polarity: leading edge is falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
                480, 272, 60, 41, 10, 2, 2, 2, 2,
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 480, 272}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 720, 573}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750_LCD_AUO_A043FL01V2)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
                LCD_CFG_MODE_TFT_24BIT |        /* output 18bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP,    /* Vsync polarity: leading edge is falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
                480, 272, 60, 41, 10, 8, 4, 4, 2,
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_F1EN | /* enable Foreground1 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {16, 0, 0, 480, 272}, /* bpp, x, y, w, h */
                 .fg1 = {16, 0, 0, 720, 573}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750_LCD_TOPPOLY_TD043MGEB1)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
                LCD_CFG_MODE_TFT_24BIT |        /* output 18bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP,    /* Vsync polarity: leading edge is falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
                800, 480, 60, 1, 1, 40, 215, 10, 34,
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_F1EN | /* enable Foreground1 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0xff, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 800, 480}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 800, 480}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750_LCD_TRULY_TFT_GG1P0319LTSW_W)
        .panel = {
                 .cfg = LCD_CFG_LCDPIN_SLCD | /* Underrun recover*/
                 LCD_CFG_NEWDES | /* 8words descriptor */
                 LCD_CFG_MODE_SLCD, /* TFT Smart LCD panel */
                 .slcd_cfg = SLCD_CFG_DWIDTH_16BIT | SLCD_CFG_CWIDTH_16BIT | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING | SLCD_CFG_TYPE_PARALLEL,
                 .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,       /* 16words burst, enable out FIFO underrun irq */
                 240, 320, 60, 0, 0, 0, 0, 0, 0,
         },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_F1EN | /* enable Foreground0 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 240, 320}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 240, 320}, /* bpp, x, y, w, h */
         },

#elif defined(CONFIG_JZ4750_LCD_FOXCONN_PT035TN01)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
//              LCD_CFG_MODE_TFT_18BIT |        /* output 18bpp */
                LCD_CFG_MODE_TFT_24BIT |        /* output 24bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP |   /* Vsync polarity: leading edge is falling edge */
                LCD_CFG_PCP,    /* Pix-CLK polarity: data translations at falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
                320, 240, 80, 1, 1, 10, 50, 10, 13
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_F1EN |        /* enable Foreground1 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750_LCD_INNOLUX_PT035TN01_SERIAL)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_SERIAL_TFT | /* Serial TFT panel */
                LCD_CFG_MODE_TFT_18BIT |        /* output 18bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP |   /* Vsync polarity: leading edge is falling edge */
                LCD_CFG_PCP,    /* Pix-CLK polarity: data translations at falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
                320, 240, 60, 1, 1, 10, 50, 10, 13
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750_SLCD_KGM701A3_TFT_SPFD5420A)
        .panel = {
//               .cfg = LCD_CFG_LCDPIN_SLCD | LCD_CFG_RECOVER | /* Underrun recover*/ 
                 .cfg = LCD_CFG_LCDPIN_SLCD | /* Underrun recover*/
//               LCD_CFG_DITHER | /* dither */
                 LCD_CFG_NEWDES | /* 8words descriptor */
                 LCD_CFG_MODE_SLCD, /* TFT Smart LCD panel */
                 .slcd_cfg = SLCD_CFG_DWIDTH_18BIT | SLCD_CFG_CWIDTH_18BIT | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING | SLCD_CFG_TYPE_PARALLEL,
                 .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,       /* 16words burst, enable out FIFO underrun irq */
                 400, 240, 60, 0, 0, 0, 0, 0, 0,
         },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_ALPHAMD | /* alpha blending mode */
//               LCD_OSDC_F1EN | /* enable Foreground1 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
//               .fg0 = {32, 0, 0, 400, 240}, /* bpp, x, y, w, h */
                 .fg0 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 400, 240}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750D_VGA_DISPLAY)
        .panel = {
                .cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER |/* Underrun recover */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
                LCD_CFG_MODE_TFT_24BIT |        /* output 18bpp */
                LCD_CFG_HSP |   /* Hsync polarity: active low */
                LCD_CFG_VSP,    /* Vsync polarity: leading edge is falling edge */
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst, enable out FIFO underrun irq */
//              800, 600, 60, 128, 4, 40, 88, 0, 23
                640, 480, 54, 96, 2, 16, 48, 10, 33
//              1280, 720, 50, 152, 15, 22, 200, 14, 1
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
//               LCD_OSDC_F1EN | /* enable Foreground1 */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80001000, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32, 0, 0, 640, 480}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 640, 480}, /* bpp, x, y, w, h */
         },
#elif defined(CONFIG_JZ4750D_KMP510_LCD)
        .panel = {
                .cfg =  LCD_CFG_MODE_TFT_18BIT |
                        LCD_CFG_VSP | LCD_CFG_DEP |
                        LCD_CFG_PCP | LCD_CFG_HSP |
                        LCD_CFG_RECOVER | LCD_CFG_NEWDES,
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_BPP_18_24 |
                        LCD_CTRL_OFUM |
                        LCD_CTRL_BST_8,
                320, 240, 60, 2, 1, 78, 6, 20, 1
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
                                LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 5,         /* disable ipu,  */
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x00001000, /* ipu restart */
                 .fg_change = 0,
                 .fg0 = {32, 0, 0, 320, 240}, /* bpp, x, y, w, h */
                 .fg1 = {32, 0, 0, 720, 573}, /* bpp, x, y, w, h */
         },

#else
#error "Select LCD panel first!!!"
#endif
};

struct jz4750lcd_info jz4750_info_tve = {
        .panel = {
                .cfg = LCD_CFG_TVEN | /* output to tve */
                LCD_CFG_NEWDES | /* 8words descriptor */
                LCD_CFG_RECOVER | /* underrun protect */
                LCD_CFG_MODE_INTER_CCIR656, /* Interlace CCIR656 mode */
                .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,        /* 16words burst */
                TVE_WIDTH_PAL, TVE_HEIGHT_PAL, TVE_FREQ_PAL, 0, 0, 0, 0, 0, 0,
        },
        .osd = {
                 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//               LCD_OSDC_ALPHAEN | /* enable alpha */
                 LCD_OSDC_F0EN, /* enable Foreground0 */
                 .osd_ctrl = 0,         /* disable ipu,  */
                 .rgb_ctrl = LCD_RGBC_YCC, /* enable RGB => YUV */
                 .bgcolor = 0x00000000, /* set background color Black */
                 .colorkey0 = 0, /* disable colorkey */
                 .colorkey1 = 0, /* disable colorkey */
                 .alpha = 0xA0, /* alpha value */
                 .ipu_restart = 0x80000100, /* ipu restart */
                 .fg_change = FG_CHANGE_ALL, /* change all initially */
                 .fg0 = {32,},  /*  */
                 .fg0 = {32,},
        },
};



struct jz4750lcd_info *jz4750_lcd_info = &jz4750_lcd_panel; /* default output to lcd panel */

#ifdef  DEBUG
static void print_lcdc_registers(void)  /* debug */
{
        /* LCD Controller Resgisters */
        printk("REG_LCD_CFG:\t0x%08x\n", REG_LCD_CFG);
        printk("REG_LCD_CTRL:\t0x%08x\n", REG_LCD_CTRL);
        printk("REG_LCD_STATE:\t0x%08x\n", REG_LCD_STATE);
        printk("REG_LCD_OSDC:\t0x%08x\n", REG_LCD_OSDC);
        printk("REG_LCD_OSDCTRL:\t0x%08x\n", REG_LCD_OSDCTRL);
        printk("REG_LCD_OSDS:\t0x%08x\n", REG_LCD_OSDS);
        printk("REG_LCD_BGC:\t0x%08x\n", REG_LCD_BGC);
        printk("REG_LCD_KEK0:\t0x%08x\n", REG_LCD_KEY0);
        printk("REG_LCD_KEY1:\t0x%08x\n", REG_LCD_KEY1);
        printk("REG_LCD_ALPHA:\t0x%08x\n", REG_LCD_ALPHA);
        printk("REG_LCD_IPUR:\t0x%08x\n", REG_LCD_IPUR);
        printk("REG_LCD_VAT:\t0x%08x\n", REG_LCD_VAT);
        printk("REG_LCD_DAH:\t0x%08x\n", REG_LCD_DAH);
        printk("REG_LCD_DAV:\t0x%08x\n", REG_LCD_DAV);
        printk("REG_LCD_XYP0:\t0x%08x\n", REG_LCD_XYP0);
        printk("REG_LCD_XYP1:\t0x%08x\n", REG_LCD_XYP1);
        printk("REG_LCD_SIZE0:\t0x%08x\n", REG_LCD_SIZE0);
        printk("REG_LCD_SIZE1:\t0x%08x\n", REG_LCD_SIZE1);
        printk("REG_LCD_RGBC\t0x%08x\n", REG_LCD_RGBC);
        printk("REG_LCD_VSYNC:\t0x%08x\n", REG_LCD_VSYNC);
        printk("REG_LCD_HSYNC:\t0x%08x\n", REG_LCD_HSYNC);
        printk("REG_LCD_PS:\t0x%08x\n", REG_LCD_PS);
        printk("REG_LCD_CLS:\t0x%08x\n", REG_LCD_CLS);
        printk("REG_LCD_SPL:\t0x%08x\n", REG_LCD_SPL);
        printk("REG_LCD_REV:\t0x%08x\n", REG_LCD_REV);
        printk("REG_LCD_IID:\t0x%08x\n", REG_LCD_IID);
        printk("REG_LCD_DA0:\t0x%08x\n", REG_LCD_DA0);
        printk("REG_LCD_SA0:\t0x%08x\n", REG_LCD_SA0);
        printk("REG_LCD_FID0:\t0x%08x\n", REG_LCD_FID0);
        printk("REG_LCD_CMD0:\t0x%08x\n", REG_LCD_CMD0);
        printk("REG_LCD_OFFS0:\t0x%08x\n", REG_LCD_OFFS0);
        printk("REG_LCD_PW0:\t0x%08x\n", REG_LCD_PW0);
        printk("REG_LCD_CNUM0:\t0x%08x\n", REG_LCD_CNUM0);
        printk("REG_LCD_DESSIZE0:\t0x%08x\n", REG_LCD_DESSIZE0);
        printk("REG_LCD_DA1:\t0x%08x\n", REG_LCD_DA1);
        printk("REG_LCD_SA1:\t0x%08x\n", REG_LCD_SA1);
        printk("REG_LCD_FID1:\t0x%08x\n", REG_LCD_FID1);
        printk("REG_LCD_CMD1:\t0x%08x\n", REG_LCD_CMD1);
        printk("REG_LCD_OFFS1:\t0x%08x\n", REG_LCD_OFFS1);
        printk("REG_LCD_PW1:\t0x%08x\n", REG_LCD_PW1);
        printk("REG_LCD_CNUM1:\t0x%08x\n", REG_LCD_CNUM1);
        printk("REG_LCD_DESSIZE1:\t0x%08x\n", REG_LCD_DESSIZE1);
        printk("==================================\n");
        printk("REG_LCD_VSYNC:\t%d:%d\n", REG_LCD_VSYNC>>16, REG_LCD_VSYNC&0xfff);
        printk("REG_LCD_HSYNC:\t%d:%d\n", REG_LCD_HSYNC>>16, REG_LCD_HSYNC&0xfff);
        printk("REG_LCD_VAT:\t%d:%d\n", REG_LCD_VAT>>16, REG_LCD_VAT&0xfff);
        printk("REG_LCD_DAH:\t%d:%d\n", REG_LCD_DAH>>16, REG_LCD_DAH&0xfff);
        printk("REG_LCD_DAV:\t%d:%d\n", REG_LCD_DAV>>16, REG_LCD_DAV&0xfff);
        printk("==================================\n");

        /* Smart LCD Controller Resgisters */
        printk("REG_SLCD_CFG:\t0x%08x\n", REG_SLCD_CFG);
        printk("REG_SLCD_CTRL:\t0x%08x\n", REG_SLCD_CTRL);
        printk("REG_SLCD_STATE:\t0x%08x\n", REG_SLCD_STATE);
        printk("==================================\n");

        /* TVE Controller Resgisters */
        printk("REG_TVE_CTRL:\t0x%08x\n", REG_TVE_CTRL);
        printk("REG_TVE_FRCFG:\t0x%08x\n", REG_TVE_FRCFG);
        printk("REG_TVE_SLCFG1:\t0x%08x\n", REG_TVE_SLCFG1);
        printk("REG_TVE_SLCFG2:\t0x%08x\n", REG_TVE_SLCFG2);
        printk("REG_TVE_SLCFG3:\t0x%08x\n", REG_TVE_SLCFG3);
        printk("REG_TVE_LTCFG1:\t0x%08x\n", REG_TVE_LTCFG1);
        printk("REG_TVE_LTCFG2:\t0x%08x\n", REG_TVE_LTCFG2);
        printk("REG_TVE_CFREQ:\t0x%08x\n", REG_TVE_CFREQ);
        printk("REG_TVE_CPHASE:\t0x%08x\n", REG_TVE_CPHASE);
        printk("REG_TVE_CBCRCFG:\t0x%08x\n", REG_TVE_CBCRCFG);
        printk("REG_TVE_WSSCR:\t0x%08x\n", REG_TVE_WSSCR);
        printk("REG_TVE_WSSCFG1:\t0x%08x\n", REG_TVE_WSSCFG1);
        printk("REG_TVE_WSSCFG2:\t0x%08x\n", REG_TVE_WSSCFG2);
        printk("REG_TVE_WSSCFG3:\t0x%08x\n", REG_TVE_WSSCFG3);

        printk("==================================\n");

        if ( 1 ) {
                unsigned int * pii = (unsigned int *)dma_desc_base;
                int i, j;
                for (j=0;j< DMA_DESC_NUM ; j++) {
                        printk("dma_desc%d(0x%08x):\n", j, (unsigned int)pii);
                        for (i =0; i<8; i++ ) {
                                printk("\t\t0x%08x\n", *pii++);
                        }
                }
        }
}
#else
#define print_lcdc_registers()
#endif

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
        chan &= 0xffff;
        chan >>= 16 - bf->length;
        return chan << bf->offset;
}

static int jz4750fb_setcolreg(u_int regno, u_int red, 
				u_int green, u_int blue,
				u_int transp, struct fb_info *info) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned short *ptr, ctmp;

//      print_dbg("regno:%d,RGBt:(%d,%d,%d,%d)\t", regno, red, green, blue, transp);
	if (regno >= NR_PALETTE) return 1;

	cfb->palette[regno].red         = red ;
	cfb->palette[regno].green       = green;
	cfb->palette[regno].blue        = blue;

	if (cfb->fb.var.bits_per_pixel <= 16) {
		red     >>= 8;
		green   >>= 8;
		blue    >>= 8;

		red     &= 0xff;
		green   &= 0xff;
		blue    &= 0xff;
	}

	switch (cfb->fb.var.bits_per_pixel) {
		case 1:
		case 2:
		case 4:
		case 8:
			if (((jz4750_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) ==
			 LCD_CFG_MODE_SINGLE_MSTN ) ||
			((jz4750_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) == 
			LCD_CFG_MODE_DUAL_MSTN )) {
				ctmp = (77L * red + 150L * green + 29L * blue) >> 8;
				ctmp = ((ctmp >> 3) << 11) | ((ctmp >> 2) << 5)|
					(ctmp >> 3);
			} else {
				/* RGB 565 */
				if (((red >> 3) == 0) && ((red >> 2) != 0))
					red = 1 << 3;
				if (((blue >> 3) == 0) && ((blue >> 2) != 0))
					blue = 1 << 3;
				ctmp = ((red >> 3) << 11) | 
					((green >> 2) << 5) | 
					(blue >> 3);
			}

			ptr = (unsigned short *)lcd_palette;
			ptr = (unsigned short *)(((u32)ptr)|0xa0000000);
			ptr[regno] = ctmp;

			break;

		case 15:
			if (regno < 16) {
				((u32 *)cfb->fb.pseudo_palette)[regno] =
					((red >> 3) << 10) |
					((green >> 3) << 5) |
					(blue >> 3);
			}
			break;
		case 16:
			if (regno < 16) {
				((u32 *)cfb->fb.pseudo_palette)[regno] =
					((red >> 3) << 11) |
					((green >> 2) << 5) |
					(blue >> 3);
			}
			break;
		case 17 ... 32:
			if (regno < 16) {
				((u32 *)cfb->fb.pseudo_palette)[regno] =
					(red << 16) |
					(green << 8) |
					(blue << 0);

/*				if (regno < 16) {
					unsigned val;
					val  = chan_to_field(red, &cfb->fb.var.red);
					val |= chan_to_field(green, &cfb->fb.var.green);
					val |= chan_to_field(blue, &cfb->fb.var.blue);
					((u32 *)cfb->fb.pseudo_palette)[regno] = val;
				}
*/

			}
			break;
        }
        return 0;
}

/* 
 * switch to tve mode from lcd mode
 * mode:
 *      PANEL_MODE_TVE_PAL: switch to TVE_PAL mode
 *      PANEL_MODE_TVE_NTSC: switch to TVE_NTSC mode
 */
static void jz4750lcd_info_switch_to_TVE(int mode) {
	struct jz4750lcd_info *info;
	struct jz4750lcd_osd_t *osd_lcd;
	int x, y, w, h;

	info = jz4750_lcd_info = &jz4750_info_tve;
	osd_lcd = &jz4750_lcd_panel.osd;

	switch ( mode ) {
		case PANEL_MODE_TVE_PAL:
			/* TVE PAL enable extra halfline signal */
			info->panel.cfg |= LCD_CFG_TVEPEH; 
			info->panel.w = TVE_WIDTH_PAL;
			info->panel.h = TVE_HEIGHT_PAL;
			info->panel.fclk = TVE_FREQ_PAL;

			w = ( osd_lcd->fg0.w < TVE_WIDTH_PAL )? 
				osd_lcd->fg0.w:TVE_WIDTH_PAL;
			h = ( osd_lcd->fg0.h < TVE_HEIGHT_PAL )?
				osd_lcd->fg0.h:TVE_HEIGHT_PAL;
//			x = ((TVE_WIDTH_PAL - w) >> 2) << 1;
//			y = ((TVE_HEIGHT_PAL - h) >> 2) << 1;
			x = 0;
			y = 0;
			info->osd.fg0.bpp = osd_lcd->fg0.bpp;
			info->osd.fg0.x = x;
			info->osd.fg0.y = y;
			info->osd.fg0.w = w;
			info->osd.fg0.h = h;

			w = ( osd_lcd->fg1.w < TVE_WIDTH_PAL )? 
				osd_lcd->fg1.w:TVE_WIDTH_PAL;
			h = ( osd_lcd->fg1.h < TVE_HEIGHT_PAL )?
				osd_lcd->fg1.h:TVE_HEIGHT_PAL;
//			x = ((TVE_WIDTH_PAL-w) >> 2) << 1;
//			y = ((TVE_HEIGHT_PAL-h) >> 2) << 1;
			x = 0;
			y = 0;
			info->osd.fg1.bpp = 32; /* use RGB888 in TVE mode*/
			info->osd.fg1.x = x;
			info->osd.fg1.y = y;
			info->osd.fg1.w = w;
			info->osd.fg1.h = h;
			break;
		case PANEL_MODE_TVE_NTSC:
			/* TVE NTSC disable extra halfline signal */
			info->panel.cfg &= ~LCD_CFG_TVEPEH; 
			info->panel.w = TVE_WIDTH_NTSC;
			info->panel.h = TVE_HEIGHT_NTSC;
			info->panel.fclk = TVE_FREQ_NTSC;

			w = ( osd_lcd->fg0.w < TVE_WIDTH_NTSC )? 
				osd_lcd->fg0.w:TVE_WIDTH_NTSC;
			h = ( osd_lcd->fg0.h < TVE_HEIGHT_NTSC)?
				osd_lcd->fg0.h:TVE_HEIGHT_NTSC;
			x = ((TVE_WIDTH_NTSC - w) >> 2) << 1;
			y = ((TVE_HEIGHT_NTSC - h) >> 2) << 1;
//			x = 0;
//			y = 0;
			info->osd.fg0.bpp = osd_lcd->fg0.bpp;
			info->osd.fg0.x = x;
			info->osd.fg0.y = y;
			info->osd.fg0.w = w;
			info->osd.fg0.h = h;

			w = ( osd_lcd->fg1.w < TVE_WIDTH_NTSC )? 
				osd_lcd->fg1.w:TVE_WIDTH_NTSC;
			h = ( osd_lcd->fg1.h < TVE_HEIGHT_NTSC)?
				osd_lcd->fg1.h:TVE_HEIGHT_NTSC;
			x = ((TVE_WIDTH_NTSC - w) >> 2) << 1;
			y = ((TVE_HEIGHT_NTSC - h) >> 2) << 1;
			info->osd.fg1.bpp = 32; /* use RGB888 int TVE mode */
			info->osd.fg1.x = x;
			info->osd.fg1.y = y;
			info->osd.fg1.w = w;
			info->osd.fg1.h = h;
			break;
		default:
			sys_printf("%s, %s: Unknown tve mode\n", __FILE__, __FUNCTION__);
	}
}

static int jz4750fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg) {
	int ret = 0;

	void *argp = (void *)arg;

//	struct jz4750lcd_info *lcd_info = jz4750_lcd_info;


	switch (cmd) {
		case FBIOSETBACKLIGHT:
			jz4750fb_set_backlight_level(arg);
			break;
		case FBIODISPON:
			REG_LCD_STATE = 0; /* clear lcdc status */
			__lcd_slcd_special_on();
			__lcd_clr_dis();
			__lcd_set_ena(); /* enable lcdc */

			jz4750fb_lcd_display_on();
			break;

		case FBIODISPOFF:
			jz4750fb_lcd_display_off();

			if ((jz4750_lcd_info->panel.cfg&LCD_CFG_LCDPIN_SLCD)||
				(jz4750_lcd_info->panel.cfg&LCD_CFG_TVEN )) {
				__lcd_clr_ena(); /* Smart lcd and TVE mode only support quick disable */
			} else {
				int cnt;
				/* when CPU main freq is 336MHz,wait for 16ms */
				cnt = 336000 * 16;
				__lcd_set_dis(); /* regular disable */
				while(!__lcd_disable_done() && cnt) {
					cnt--;
				}
				if (cnt == 0) {
					sys_printf("LCD disable timeout! REG_LCD_STATE=0x%08xx\n",REG_LCD_STATE);
				}
				REG_LCD_STATE &= ~LCD_STATE_LDD;
			}
			break;
		case FBIOPRINT_REG:
			print_lcdc_registers();
			break;
		case FBIO_GET_MODE:
			print_dbg("fbio get mode\n");
			if (copy_to_user(argp, jz4750_lcd_info, sizeof(struct jz4750lcd_info))) {
				return -EFAULT;
			}
			break;
		case FBIO_SET_MODE:
			print_dbg("fbio set mode\n");
			if (copy_from_user(jz4750_lcd_info, argp, sizeof(struct jz4750lcd_info))) {
				return -EFAULT;
			}
			/* set mode */
			jz4750fb_set_mode(jz4750_lcd_info);
			break;
		case FBIO_DEEP_SET_MODE:
			print_dbg("fbio deep set mode\n");
			if (copy_from_user(jz4750_lcd_info, argp, sizeof(struct jz4750lcd_info))) {
				return -EFAULT;
			}
			jz4750fb_deep_set_mode(jz4750_lcd_info);
			break;
#ifdef CONFIG_FB_JZ4750_TVE
		case FBIO_MODE_SWITCH:
			print_dbg("lcd mode switch between tve and lcd, arg=%lu\n", arg);
			switch ( arg ) {
				/* switch to TVE_PAL mode */
				case PANEL_MODE_TVE_PAL:
				/* switch to TVE_NTSC mode */
				case PANEL_MODE_TVE_NTSC:
					jz4750lcd_info_switch_to_TVE(arg);
					jz4750tve_init(arg); /* tve controller init */
					sys_udelay(100);
					jz4750tve_enable_tve();
					/* turn off lcd backlight */
					jz4750fb_lcd_display_off();
					break;
				case PANEL_MODE_LCD_PANEL:      /* switch to LCD mode */
				default :
					/* turn off TVE, turn off DACn... */
					jz4750tve_disable_tve();
					jz4750_lcd_info = &jz4750_lcd_panel;
					/* turn on lcd backlight */
					jz4750fb_lcd_display_on();
					break;
			}
			jz4750fb_deep_set_mode(jz4750_lcd_info);
			break;
		case FBIO_GET_TVE_MODE:
			print_dbg("fbio get TVE mode\n");
			if (copy_to_user(argp, jz4750_tve_info, sizeof(struct jz4750tve_info))) {
				return -EFAULT;
			}
			break;
		case FBIO_SET_TVE_MODE:
			print_dbg("fbio set TVE mode\n");
			if (copy_from_user(jz4750_tve_info, argp, sizeof(struct jz4750tve_info))) {
				return -EFAULT;
			}
			/* set tve mode */
			jz4750tve_set_tve_mode(jz4750_tve_info);
			break;
#endif
		default:
			dprintk("%s, unknown command(0x%x)", __FILE__, cmd);
			break;
	}

	return ret;
}

/* Use mmap /dev/fb can only get a non-cacheable Virtual Address. */
static int jz4750fb_mmap(struct fb_info *info, struct vm_area_struct *vma) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned long start;
	unsigned long off;
	u32 len;
	dprintk("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);

	off = vma->vm_pgoff << PAGE_SHIFT;
	//fb->fb_get_fix(&fix, PROC_CONSOLE(info), info);

	/* frame buffer memory */
	start = cfb->fb.fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + cfb->fb.fix.smem_len);
	start &= PAGE_MASK;

	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

#if 0
        vma->vm_pgoff = off >> PAGE_SHIFT;
        vma->vm_flags |= VM_IO;
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);        /* Uncacheable */

#if 1
        pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
        pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED;               /* Uncacheable */
//      pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NONCOHERENT;   /* Write-Back */
#endif

        if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                               vma->vm_end - vma->vm_start,
                               vma->vm_page_prot)) {
                return -EAGAIN;
        }
#endif
	return 0;
}

/* checks var and eventually tweaks it to something supported,
 * DO NOT MODIFY PAR */
static int jz4750fb_check_var(struct fb_var_screeninfo *var, 
				struct fb_info *info) {
	sys_printf("jz4750fb_check_var, not implement\n");
	return 0;
}


/* 
 * set the video mode according to info->var
 */
static int jz4750fb_set_par(struct fb_info *info) {
	sys_printf("jz4750fb_set_par, not implemented\n");
	return 0;
}

/*
 * (Un)Blank the display.
 * Fix me: should we use VESA value?
 */
static int jz4750fb_blank(int blank_mode, struct fb_info *info) {
	dprintk("jz4750 fb_blank %d %p", blank_mode, info);
	switch (blank_mode) {
		case FB_BLANK_UNBLANK:
			//case FB_BLANK_NORMAL:
			/* Turn on panel */
			__lcd_set_ena();
			jz4750fb_lcd_display_on();
			break;
		case FB_BLANK_NORMAL:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_POWERDOWN:
#if 0
			/* Turn off panel */
			__lcd_display_off();
			__lcd_set_dis();
#endif
			break;
		default:
			break;
	}
	return 0;
}


/* 
 * pan display
 */
static int jz4750fb_pan_display(struct fb_var_screeninfo *var, 
				struct fb_info *info) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	int dy;

	if (!var || !cfb) {
		return -EINVAL;
	}

	if (var->xoffset - cfb->fb.var.xoffset) {
		/* No support for X panning for now! */
		return -EINVAL;
	}

	dy = var->yoffset;
	print_dbg("var.yoffset: %d", dy);
	if (dy) {
		dma0_desc0->databuf = 
			(unsigned int)virt_to_phys((void *)lcd_frame0 + 
			(cfb->fb.fix.line_length * dy));
//		dma_cache_wback((unsigned int)(dma0_desc0), 
//				sizeof(struct jz4750_lcd_dma_desc));

	} else {
		dma0_desc0->databuf = 
			(unsigned int)virt_to_phys((void *)lcd_frame0);
//		dma_cache_wback((unsigned int)(dma0_desc0), 
//				sizeof(struct jz4750_lcd_dma_desc));
	}

	return 0;
}

/* use default function cfb_fillrect, cfb_copyarea, cfb_imageblit */
static struct fb_ops jz4750fb_ops = {
//.owner                  = THIS_MODULE,
	.fb_setcolreg	= jz4750fb_setcolreg,
	.fb_check_var	= jz4750fb_check_var,
	.fb_set_par	= jz4750fb_set_par,
	.fb_blank	= jz4750fb_blank,
	.fb_pan_display	= jz4750fb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_mmap	= jz4750fb_mmap,
	.fb_ioctl	= jz4750fb_ioctl,
};

static int jz4750fb_set_var(struct fb_var_screeninfo *var, int con,
                        struct fb_info *info) {
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	struct jz4750lcd_info *lcd_info = jz4750_lcd_info;
	int chgvar = 0;

	var->height                 = lcd_info->osd.fg0.h;      /* tve mode */
	var->width                  = lcd_info->osd.fg0.w;
	var->bits_per_pixel         = lcd_info->osd.fg0.bpp;

	var->vmode                  = FB_VMODE_NONINTERLACED;
	var->activate               = cfb->fb.var.activate;
	var->xres                   = var->width;
	var->yres                   = var->height;
	var->xres_virtual           = var->width;
	var->yres_virtual           = var->height;
	var->xoffset                = 0;
	var->yoffset                = 0;
	var->pixclock               = 0;
	var->left_margin            = 0;
	var->right_margin           = 0;
	var->upper_margin           = 0;
	var->lower_margin           = 0;
	var->hsync_len              = 0;
	var->vsync_len              = 0;
	var->sync                   = 0;
	var->activate              &= ~FB_ACTIVATE_TEST;

	/*
	 * CONUPDATE and SMOOTH_XPAN are equal.  However,
	 * SMOOTH_XPAN is only used internally by fbcon.
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = cfb->fb.var.xoffset;
		var->yoffset = cfb->fb.var.yoffset;
	}

	if (var->activate & FB_ACTIVATE_TEST)
		return 0;

	if ((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NOW)
		return -EINVAL;

	if (cfb->fb.var.xres != var->xres)
		chgvar = 1;
	if (cfb->fb.var.yres != var->yres)
		chgvar = 1;
	if (cfb->fb.var.xres_virtual != var->xres_virtual)
		chgvar = 1;
	if (cfb->fb.var.yres_virtual != var->yres_virtual)
		chgvar = 1;
	if (cfb->fb.var.bits_per_pixel != var->bits_per_pixel)
		chgvar = 1;

        //display = fb_display + con;

	var->red.msb_right      = 0;
	var->green.msb_right    = 0;
	var->blue.msb_right     = 0;

	switch(var->bits_per_pixel){
		case 1: /* Mono */
			cfb->fb.fix.visual      = FB_VISUAL_MONO01;
			cfb->fb.fix.line_length = (var->xres * var->bits_per_pixel) / 8;
			break;
		case 2: /* Mono */
			var->red.offset         = 0;
			var->red.length         = 2;
			var->green.offset       = 0;
			var->green.length       = 2;
			var->blue.offset        = 0;
			var->blue.length        = 2;

			cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
			cfb->fb.fix.line_length = (var->xres * var->bits_per_pixel) / 8;
			break;
		case 4: /* PSEUDOCOLOUR*/
			var->red.offset         = 0;
			var->red.length         = 4;
			var->green.offset       = 0;
			var->green.length       = 4;
			var->blue.offset        = 0;
			var->blue.length        = 4;

			cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
			cfb->fb.fix.line_length = var->xres / 2;
			break;
		case 8: /* PSEUDOCOLOUR, 256 */
			var->red.offset         = 0;
			var->red.length         = 8;
			var->green.offset       = 0;
			var->green.length       = 8;
			var->blue.offset        = 0;
			var->blue.length        = 8;

			cfb->fb.fix.visual      = FB_VISUAL_PSEUDOCOLOR;
			cfb->fb.fix.line_length = var->xres ;
			break;
		case 15: /* DIRECTCOLOUR, 32k */
			var->bits_per_pixel     = 15;
			var->red.offset         = 10;
			var->red.length         = 5;
			var->green.offset       = 5;
			var->green.length       = 5;
			var->blue.offset        = 0;
			var->blue.length        = 5;

			cfb->fb.fix.visual      = FB_VISUAL_DIRECTCOLOR;
			cfb->fb.fix.line_length = var->xres_virtual * 2;
			break;
		case 16: /* DIRECTCOLOUR, 64k */
			var->bits_per_pixel     = 16;
			var->red.offset         = 11;
			var->red.length         = 5;
			var->green.offset       = 5;
			var->green.length       = 6;
			var->blue.offset        = 0;
			var->blue.length        = 5;

			cfb->fb.fix.visual      = FB_VISUAL_TRUECOLOR;
			cfb->fb.fix.line_length = var->xres_virtual * 2;
			break;
		case 17 ... 32:
			/* DIRECTCOLOUR, 256 */
			var->bits_per_pixel     = 32;

			var->red.offset         = 16;
			var->red.length         = 8;
			var->green.offset       = 8;
			var->green.length       = 8;
			var->blue.offset        = 0;
			var->blue.length        = 8;
			var->transp.offset      = 24;
			var->transp.length      = 8;

			cfb->fb.fix.visual      = FB_VISUAL_TRUECOLOR;
			cfb->fb.fix.line_length = var->xres_virtual * 4;
			break;
		default: /* in theory this should never happen */
			sys_printf("%s: don't support for %dbpp\n",
				cfb->fb.fix.id, var->bits_per_pixel);
			break;
	}

	cfb->fb.var = *var;
	cfb->fb.var.activate &= ~FB_ACTIVATE_ALL;

	/*
	 * Update the old var.  The fbcon drivers still use this.
	 * Once they are using cfb->fb.var, this can be dropped.
	 *                                      --rmk
	 */
	//display->var = cfb->fb.var;
	/*
	 * If we are setting all the virtual consoles, also set the
	 * defaults used to create new consoles.
	 */
	fb_set_cmap(&cfb->fb.cmap, &cfb->fb);

	dprintk("jz4750fb_set_var: after fb_set_cmap...\n");

	return 0;
}

static struct lcd_cfb_info *jz4750fb_alloc_fb_info(void) {
	struct lcd_cfb_info *cfb;
	cfb=get_page();

	if (!cfb) {
		return NULL;
	}

	memset(cfb, 0, sizeof(struct lcd_cfb_info) );

	cfb->currcon		= -1;
	cfb->backlight_level	= LCD_DEFAULT_BACKLIGHT;

	strcpy(cfb->fb.fix.id, "jz-lcd");
	cfb->fb.fix.type        = FB_TYPE_PACKED_PIXELS;
	cfb->fb.fix.type_aux    = 0;
	cfb->fb.fix.xpanstep    = 1;
	cfb->fb.fix.ypanstep    = 1;
	cfb->fb.fix.ywrapstep   = 0;
	cfb->fb.fix.accel       = FB_ACCEL_NONE;

	cfb->fb.var.nonstd      = 0;
	cfb->fb.var.activate    = FB_ACTIVATE_NOW;
	cfb->fb.var.height      = -1;
	cfb->fb.var.width       = -1;
	cfb->fb.var.accel_flags = FB_ACCELF_TEXT;

	cfb->fb.fbops           = &jz4750fb_ops;
	cfb->fb.flags           = FBINFO_FLAG_DEFAULT;

	cfb->fb.pseudo_palette  = (void *)(cfb + 1);

	switch (jz4750_lcd_info->osd.fg0.bpp) {
		case 1:
			fb_alloc_cmap(&cfb->fb.cmap, 4, 0);
			break;
		case 2:
			fb_alloc_cmap(&cfb->fb.cmap, 8, 0);
			break;
		case 4:
			fb_alloc_cmap(&cfb->fb.cmap, 32, 0);
			break;
		case 8:
		default:
			fb_alloc_cmap(&cfb->fb.cmap, 256, 0);
			break;
	}

        dprintk("fb_alloc_cmap,fb.cmap.len:%d....\n", cfb->fb.cmap.len);

	return cfb;
}

/*
 * Map screen memory
 */
static int jz4750fb_map_smem(struct lcd_cfb_info *cfb) {
	unsigned long page;
	unsigned int page_shift, needroom, needroom1, bpp, w, h;

	bpp = jz4750_lcd_info->osd.fg0.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;
#ifndef CONFIG_FB_JZ4750_TVE
	w = jz4750_lcd_info->osd.fg0.w;
	h = jz4750_lcd_info->osd.fg0.h;
#else
	w = ( jz4750_lcd_info->osd.fg0.w > TVE_WIDTH_PAL )?jz4750_lcd_info->osd.fg0.w:TVE_WIDTH_PAL;
	h = ( jz4750_lcd_info->osd.fg0.h > TVE_HEIGHT_PAL )?jz4750_lcd_info->osd.fg0.h:TVE_HEIGHT_PAL;
#endif
	needroom1 = needroom = ((w * bpp + 7) >> 3) * h;
#if defined(CONFIG_FB_JZ4750_LCD_USE_2LAYER_FRAMEBUFFER)
	bpp = jz4750_lcd_info->osd.fg1.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;

#ifndef CONFIG_FB_JZ4750_TVE
	w = jz4750_lcd_info->osd.fg1.w;
	h = jz4750_lcd_info->osd.fg1.h;
#else
	w = ( jz4750_lcd_info->osd.fg1.w > TVE_WIDTH_PAL )?jz4750_lcd_info->osd.fg1.w:TVE_WIDTH_PAL;
	h = ( jz4750_lcd_info->osd.fg1.h > TVE_HEIGHT_PAL )?jz4750_lcd_info->osd.fg1.h:TVE_HEIGHT_PAL;
#endif
	needroom += ((w * bpp + 7) >> 3) * h;
#endif // two layer

	for (page_shift = 0; page_shift < 12; page_shift++)
		if ((PAGE_SIZE << page_shift) >= needroom)
			break;
	lcd_palette = (unsigned char *)get_page();
	lcd_frame0 = (unsigned char *)get_pages(page_shift);

	if ((!lcd_palette) || (!lcd_frame0))
		return -ENOMEM;

	memset((void *)lcd_palette, 0, PAGE_SIZE);
	memset((void *)lcd_frame0, 0, PAGE_SIZE << page_shift);

	dma_desc_base = (struct jz4750_lcd_dma_desc *)
		((void*)lcd_palette + ((PALETTE_SIZE+3)/4)*4);

#if defined(CONFIG_FB_JZ4750_SLCD)

	lcd_cmdbuf = (unsigned char *)__get_free_pages(GFP_KERNEL, 0);
	memset((void *)lcd_cmdbuf, 0, PAGE_SIZE);

	{       int data, i, *ptr;
		ptr = (unsigned int *)lcd_cmdbuf;
		data = WR_GRAM_CMD;
		data = ((data & 0xff) << 1) | ((data & 0xff00) << 2);
		for(i = 0; i < 3; i++){
			ptr[i] = data;
		}
	}
#endif

#if defined(CONFIG_FB_JZ4750_LCD_USE_2LAYER_FRAMEBUFFER)
	lcd_frame1 = lcd_frame0 + needroom1;
#endif

	/*
	 * Set page reserved so that mmap will work. This is necessary
	 * since we'll be remapping normal memory.
	 */
	page = (unsigned long)lcd_palette;
//	SetPageReserved(virt_to_page((void*)page));

	for (page = (unsigned long)lcd_frame0;
		page < PAGE_ALIGN((unsigned long)lcd_frame0 + (PAGE_SIZE<<page_shift));
		page += PAGE_SIZE) {
//		SetPageReserved(virt_to_page((void*)page));
	}

	cfb->fb.fix.smem_start = virt_to_phys((void *)lcd_frame0);
	cfb->fb.fix.smem_len = (PAGE_SIZE << page_shift); /* page_shift/2 ??? */
	cfb->fb.screen_base =
	(unsigned char *)(((unsigned int)lcd_frame0&0x1fffffff) | 0xa0000000);

	if (!cfb->fb.screen_base) {
		sys_printf("jz4750fb, %s: unable to map screen memory\n", cfb->fb.fix.id);
		return -ENOMEM;
	}

	return 0;
}

static void jz4750fb_free_fb_info(struct lcd_cfb_info *cfb) {
	if (cfb) {
		fb_alloc_cmap(&cfb->fb.cmap, 0, 0);
		free(cfb);
	}
}

static void jz4750fb_unmap_smem(struct lcd_cfb_info *cfb) {
	struct page * map = NULL;
	unsigned char *tmp;
	unsigned int page_shift, needroom, bpp, w, h;

	bpp = jz4750_lcd_info->osd.fg0.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;
	w = jz4750_lcd_info->osd.fg0.w;
	h = jz4750_lcd_info->osd.fg0.h;
	needroom = ((w * bpp + 7) >> 3) * h;
#if defined(CONFIG_FB_JZ4750_LCD_USE_2LAYER_FRAMEBUFFER)
	bpp = jz4750_lcd_info->osd.fg1.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;
	w = jz4750_lcd_info->osd.fg1.w;
	h = jz4750_lcd_info->osd.fg1.h;
	needroom += ((w * bpp + 7) >> 3) * h;
#endif

	for (page_shift = 0; page_shift < 12; page_shift++)
		if ((PAGE_SIZE << page_shift) >= needroom)
			break;

	if (cfb && cfb->fb.screen_base) {
//		iounmap(cfb->fb.screen_base);
		cfb->fb.screen_base = NULL;
//		release_mem_region(cfb->fb.fix.smem_start,
//					cfb->fb.fix.smem_len);
	}

	if (lcd_palette) {
//		map = virt_to_page(lcd_palette);
//		clear_bit(PG_reserved, &map->flags);
//		free_pages((int)lcd_palette, 0);
	}

	if (lcd_frame0) {
		for (tmp=(unsigned char *)lcd_frame0;
			tmp < lcd_frame0 + (PAGE_SIZE << page_shift);
			tmp += PAGE_SIZE) {
//			map = virt_to_page(tmp);
//			clear_bit(PG_reserved, &map->flags);
		}
//		free_pages((int)lcd_frame0, page_shift);
	}
}


/* initial dma descriptors */
static void jz4750fb_descriptor_init( struct jz4750lcd_info * lcd_info ) {
	unsigned int pal_size;

	switch ( lcd_info->osd.fg0.bpp ) {
		case 1:
			pal_size = 4;
			break;
		case 2:
			pal_size = 8;
			break;
		case 4:
			pal_size = 32;
			break;
		case 8:
		default:
			pal_size = 512;
	}

	pal_size /= 4;

	dma0_desc_palette       = dma_desc_base + 0;
	dma0_desc0              = dma_desc_base + 1;
	dma0_desc1              = dma_desc_base + 2;
	dma0_desc_cmd0          = dma_desc_base + 3; /* use only once */
	dma0_desc_cmd           = dma_desc_base + 4;
	dma1_desc0              = dma_desc_base + 5;
	dma1_desc1              = dma_desc_base + 6;

	/*
	 * Normal TFT panel's DMA Chan0: 
	 *      TO LCD Panel:   
	 *              no palette:     dma0_desc0 <<==>> dma0_desc0 
	 *              palette :       dma0_desc_palette <<==>> dma0_desc0
	 *      TO TV Encoder:
	 *              no palette:     dma0_desc0 <<==>> dma0_desc1
	 *              palette:        dma0_desc_palette --> dma0_desc0 
	 *                    --> dma0_desc1 --> dma0_desc_palette --> ...
	 *                              
	 * SMART LCD TFT panel(dma0_desc_cmd)'s DMA Chan0: 
	 *      TO LCD Panel:
	 *              no palette:     dma0_desc_cmd <<==>> dma0_desc0 
	 *              palette :       dma0_desc_palette --> dma0_desc_cmd
	 *                     --> dma0_desc0 --> dma0_desc_palette --> ...
	 *      TO TV Encoder:
	 *              no palette:     dma0_desc_cmd --> dma0_desc0 
	 *                     --> dma0_desc1 --> dma0_desc_cmd --> ...
	 *              palette:        dma0_desc_palette --> dma0_desc_cmd 
	 *                              --> dma0_desc0 --> dma0_desc1 
	 *                              --> dma0_desc_palette --> ...
	 * DMA Chan1:
	 *      TO LCD Panel:
	 *              dma1_desc0 <<==>> dma1_desc0 
	 *      TO TV Encoder:
	 *              dma1_desc0 <<==>> dma1_desc1
	 */

#if defined(CONFIG_FB_JZ4750_SLCD)
	/* First CMD descriptors, use only once, cmd_num isn't 0 */
	dma0_desc_cmd0->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
	dma0_desc_cmd0->databuf=(unsigned int)virt_to_phys((void *)lcd_cmdbuf);
	dma0_desc_cmd0->frame_id  = (unsigned int)0x0da0cad0; /* dma0's cmd0 */
	dma0_desc_cmd0->cmd       = LCD_CMD_CMD | 3; /* command */
	dma0_desc_cmd0->offsize   = 0;
	dma0_desc_cmd0->page_width= 0;
	dma0_desc_cmd0->cmd_num   = 3;

	/* Dummy Command Descriptor, cmd_num is 0 */
	dma0_desc_cmd->next_desc  = (unsigned int)virt_to_phys(dma0_desc0);
	dma0_desc_cmd->databuf    = 0;
	dma0_desc_cmd->frame_id   = (unsigned int)0x0da000cd; /* dma0's cmd0 */
	dma0_desc_cmd->cmd        = LCD_CMD_CMD | 0; /* dummy command */
	dma0_desc_cmd->cmd_num    = 0;
	dma0_desc_cmd->offsize    = 0;
	dma0_desc_cmd->page_width = 0;

	/* Palette Descriptor */
	dma0_desc_palette->next_desc=(unsigned int)virt_to_phys(dma0_desc_cmd0);
#else
	/* Palette Descriptor */
	dma0_desc_palette->next_desc=(unsigned int)virt_to_phys(dma0_desc0);
#endif
	dma0_desc_palette->databuf=(unsigned int)virt_to_phys((void *)lcd_palette);
	dma0_desc_palette->frame_id = (unsigned int)0xaaaaaaaa;
	dma0_desc_palette->cmd = LCD_CMD_PAL|pal_size; /* Palette Descriptor */

	/* DMA0 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc1);
	} else {                   /* Normal TFT LCD */
#if defined(CONFIG_FB_JZ4750_SLCD)
		dma0_desc0->next_desc=(unsigned int)virt_to_phys(dma0_desc_cmd);
#else
		dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
#endif
	}

	dma0_desc0->databuf = virt_to_phys((void *)lcd_frame0);
	dma0_desc0->frame_id = (unsigned int)0x0000da00; /* DMA0'0 */

	/* DMA0 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */

		/* load palette only once at setup */
		if (lcd_info->osd.fg0.bpp <= 8) {
			dma0_desc1->next_desc = (unsigned int)
					virt_to_phys(dma0_desc_palette);
		} else {
#if defined(CONFIG_FB_JZ4750_SLCD)  /* for smatlcd */
			dma0_desc1->next_desc = (unsigned int)
						virt_to_phys(dma0_desc_cmd);
#else
			dma0_desc1->next_desc = (unsigned int)
						virt_to_phys(dma0_desc0);
#endif
		}
		dma0_desc1->frame_id = (unsigned int)0x0000da01; /* DMA0'1 */
	}

	if (lcd_info->osd.fg0.bpp <= 8) { /* load palette only once at setup */
		REG_LCD_DA0 = virt_to_phys(dma0_desc_palette);
	} else {
#if defined(CONFIG_FB_JZ4750_SLCD)  /* for smartlcd */
		REG_LCD_DA0 = virt_to_phys(dma0_desc_cmd0); //smart lcd
#else
		REG_LCD_DA0 = virt_to_phys(dma0_desc0); //tft
#endif
	}

	/* DMA1 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma1_desc0->next_desc = (unsigned int)virt_to_phys(dma1_desc1);
	} else {                   /* Normal TFT LCD */
		dma1_desc0->next_desc = (unsigned int)virt_to_phys(dma1_desc0);
	}

	dma1_desc0->databuf = virt_to_phys((void *)lcd_frame1);
	dma1_desc0->frame_id = (unsigned int)0x0000da10; /* DMA1'0 */

	/* DMA1 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma1_desc1->next_desc = (unsigned int)virt_to_phys(dma1_desc0);
		dma1_desc1->frame_id = (unsigned int)0x0000da11; /* DMA1'1 */
	}

	REG_LCD_DA1 = virt_to_phys(dma1_desc0); /* set Dma-chan1's Descripter Addrress */
//	dma_cache_wback_inv((unsigned int)(dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4750_lcd_dma_desc));

#if 0
	/* Palette Descriptor */
	if ( lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ) 
//		dma0_desc_palette->next_desc = (unsigned int)virt_to_phys(dma0_desc_cmd);
		dma0_desc_palette->next_desc = (unsigned int)virt_to_phys(dma0_desc_cmd1);
	else
		dma0_desc_palette->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
	dma0_desc_palette->databuf = (unsigned int)virt_to_phys((void *)lcd_palette);
	dma0_desc_palette->frame_id = (unsigned int)0xaaaaaaaa;
	dma0_desc_palette->cmd  = LCD_CMD_PAL | pal_size; /* Palette Descriptor */

	/* Dummy Command Descriptor, cmd_num is 0 */
	dma0_desc_cmd->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
	dma0_desc_cmd->databuf  = (unsigned int)virt_to_phys((void *)lcd_cmdbuf);
	dma0_desc_cmd->frame_id = (unsigned int)0x0da0cad0; /* dma0's cmd0 */
	dma0_desc_cmd->cmd      = LCD_CMD_CMD | 3; /* dummy command */
	dma0_desc_cmd->offsize  = 0; /* dummy command */
	dma0_desc_cmd->page_width = 0; /* dummy command */
	dma0_desc_cmd->cmd_num  = 3;

//---------------------------------
	dma0_desc_cmd1->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
	dma0_desc_cmd1->databuf         = 0; 
	dma0_desc_cmd1->frame_id = (unsigned int)0x0da0cad1; /* dma0's cmd0 */
	dma0_desc_cmd1->cmd     = LCD_CMD_CMD | 0; /* dummy command */
	dma0_desc_cmd1->cmd_num         = 0;
	dma0_desc_cmd1->offsize         = 0; /* dummy command */
	dma0_desc_cmd1->page_width = 0; /* dummy command */
//-----------------------------------
	/* DMA0 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc1);
        } else {                   /* Normal TFT LCD */
		if (lcd_info->osd.fg0.bpp <= 8) { /* load palette only once at setup?? */
//			dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc_palette); //tft
			dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc_cmd); // smart lcd
		} else if ( lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ) {
			dma0_desc0->next_desc = (unsigned int)
					virt_to_phys(dma0_desc_cmd1);
//			dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc_cmd);
		} else {
			dma0_desc0->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
		}
	}

	dma0_desc0->databuf = virt_to_phys((void *)lcd_frame0);
	dma0_desc0->frame_id = (unsigned int)0x0000da00; /* DMA0'0 */

	/* DMA0 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		if (lcd_info->osd.fg0.bpp <= 8) { /* load palette only once at setup?? */
			dma0_desc1->next_desc = (unsigned int)virt_to_phys(dma0_desc_palette);
                
                } else if ( lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ) {
			dma0_desc1->next_desc = (unsigned int)virt_to_phys(dma0_desc_cmd);
		} else {
			dma0_desc1->next_desc = (unsigned int)virt_to_phys(dma0_desc0);
		}
		dma0_desc1->frame_id = (unsigned int)0x0000da01; /* DMA0'1 */
	}

	/* DMA1 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma1_desc0->next_desc = (unsigned int)virt_to_phys(dma1_desc1);
	} else {                   /* Normal TFT LCD */
		dma1_desc0->next_desc = (unsigned int)virt_to_phys(dma1_desc0);
	}

	dma1_desc0->databuf = virt_to_phys((void *)lcd_frame1);
	dma1_desc0->frame_id = (unsigned int)0x0000da10; /* DMA1'0 */

	/* DMA1 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		dma1_desc1->next_desc = (unsigned int)virt_to_phys(dma1_desc0);
		dma1_desc1->frame_id = (unsigned int)0x0000da11; /* DMA1'1 */
	}

	if (lcd_info->osd.fg0.bpp <= 8) { /*load palette only once at setup??*/
		REG_LCD_DA0 = virt_to_phys(dma0_desc_palette);
	} else {
//		REG_LCD_DA0 = virt_to_phys(dma0_desc_cmd); //smart lcd
		REG_LCD_DA0 = virt_to_phys(dma0_desc0); //tft
	}
	REG_LCD_DA1 = virt_to_phys(dma1_desc0); /* set Dma-chan1's Descripter Addrress */
	dma_cache_wback_inv((unsigned int)(dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4750_lcd_dma_desc));
#endif
}

static void jz4750fb_set_panel_mode( struct jz4750lcd_info * lcd_info ) {
	struct jz4750lcd_panel_t *panel = &lcd_info->panel;
#ifdef CONFIG_JZ4750D_VGA_DISPLAY
	REG_TVE_CTRL |= TVE_CTRL_DAPD;
	REG_TVE_CTRL &= ~( TVE_CTRL_DAPD1 | TVE_CTRL_DAPD2 | TVE_CTRL_DAPD3);
#endif
	/* set bpp */
	lcd_info->panel.ctrl &= ~LCD_CTRL_BPP_MASK;
	if ( lcd_info->osd.fg0.bpp == 1 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_1;
	} else if ( lcd_info->osd.fg0.bpp == 2 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_2;
	} else if ( lcd_info->osd.fg0.bpp == 4 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_4;
	} else if ( lcd_info->osd.fg0.bpp == 8 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_8;
	} else if ( lcd_info->osd.fg0.bpp == 15 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_16 | LCD_CTRL_RGB555;
	} else if ( lcd_info->osd.fg0.bpp == 16 ) {
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_16 | LCD_CTRL_RGB565;
	} else if (lcd_info->osd.fg0.bpp>16&&lcd_info->osd.fg0.bpp<32+1) {
		lcd_info->osd.fg0.bpp = 32;
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_18_24;
	} else {
		sys_printf("The BPP %d is not supported\n", 
					lcd_info->osd.fg0.bpp);
		lcd_info->osd.fg0.bpp = 32;
		lcd_info->panel.ctrl |= LCD_CTRL_BPP_18_24;
	}

	lcd_info->panel.cfg |= LCD_CFG_NEWDES; /*use 8words descriptor always */

	REG_LCD_CTRL = lcd_info->panel.ctrl; /* LCDC Controll Register */
	REG_LCD_CFG = lcd_info->panel.cfg; /* LCDC Configure Register */
	REG_SLCD_CFG = lcd_info->panel.slcd_cfg;/*Smart LCD Configure Register*/

	if (lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD) {/*enable Smart LCD DMA*/
		REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;
	}

	switch ( lcd_info->panel.cfg & LCD_CFG_MODE_MASK ) {
		case LCD_CFG_MODE_GENERIC_TFT:
		case LCD_CFG_MODE_INTER_CCIR656:
		case LCD_CFG_MODE_NONINTER_CCIR656:
		case LCD_CFG_MODE_SLCD:
		default:
                /* only support TFT16 TFT32, 
  			not support STN and Special TFT by now(10-06-2008)*/
			REG_LCD_VAT = 
				(((panel->blw + panel->w + panel->elw + panel->hsw)) << 16) | (panel->vsw + panel->bfw + panel->h + panel->efw);
			REG_LCD_DAH = 
				((panel->hsw + panel->blw) << 16) | (panel->hsw + panel->blw + panel->w);
			REG_LCD_DAV = 
				((panel->vsw + panel->bfw) << 16) | (panel->vsw + panel->bfw + panel->h);
			REG_LCD_HSYNC = (0 << 16) | panel->hsw;
			REG_LCD_VSYNC = (0 << 16) | panel->vsw;
			break;
	}
}

static void jz4750fb_set_osd_mode( struct jz4750lcd_info * lcd_info) {
	dprintk("%s, %d\n", __FILE__, __LINE__ );
	lcd_info->osd.osd_ctrl &= ~(LCD_OSDCTRL_OSDBPP_MASK);
	if ( lcd_info->osd.fg1.bpp == 15 ) {
		lcd_info->osd.osd_ctrl |= 
			LCD_OSDCTRL_OSDBPP_15_16|LCD_OSDCTRL_RGB555;
        } else if ( lcd_info->osd.fg1.bpp == 16 ) {
		lcd_info->osd.osd_ctrl |= 
			LCD_OSDCTRL_OSDBPP_15_16|LCD_OSDCTRL_RGB565;
	} else {
		lcd_info->osd.fg1.bpp = 32;
		lcd_info->osd.osd_ctrl |= LCD_OSDCTRL_OSDBPP_18_24;
	}

	REG_LCD_OSDC    = lcd_info->osd.osd_cfg; /* F0, F1, alpha, */

	REG_LCD_OSDCTRL = lcd_info->osd.osd_ctrl; /* IPUEN, bpp */
	REG_LCD_RGBC    = lcd_info->osd.rgb_ctrl;
	REG_LCD_BGC     = lcd_info->osd.bgcolor;
	REG_LCD_KEY0    = lcd_info->osd.colorkey0;
	REG_LCD_KEY1    = lcd_info->osd.colorkey1;
	REG_LCD_ALPHA   = lcd_info->osd.alpha;
	REG_LCD_IPUR    = lcd_info->osd.ipu_restart;
}


static void jz4750fb_foreground_resize( struct jz4750lcd_info * lcd_info) {
	int fg0_line_size, fg0_frm_size, fg1_line_size, fg1_frm_size;
	/* 
	 * NOTE: 
	 * Foreground change sequence: 
	 *      1. Change Position Registers -> LCD_OSDCTL.Change;
	 *      2. LCD_OSDCTRL.Change -> descripter->Size
	 * Foreground, only one of the following can be change at one time:
	 *      1. F0 size;
	 *      2. F0 position
	 *      3. F1 size
	 *      4. F1 position
	 */

	/* 
	 * The rules of f0, f1's position:
	 *      f0.x + f0.w <= panel.w;
	 *      f0.y + f0.h <= panel.h;
	 *
	 * When output is LCD panel, fg.y and fg.h can be odd number or even number.
	 * When output is TVE, as the TVE has odd frame and even frame,
	 * to simplified operation, fg.y and fg.h should be even number always.
	 *
	 */

	/* Foreground 0  */
	if ( lcd_info->osd.fg0.x >= lcd_info->panel.w ) {
		lcd_info->osd.fg0.x = lcd_info->panel.w;
	}
	if ( lcd_info->osd.fg0.y >= lcd_info->panel.h ) {
		lcd_info->osd.fg0.y = lcd_info->panel.h;
	}
	if ( lcd_info->osd.fg0.x + lcd_info->osd.fg0.w > lcd_info->panel.w ) {
		lcd_info->osd.fg0.w = lcd_info->panel.w - lcd_info->osd.fg0.x;
	}
	if ( lcd_info->osd.fg0.y + lcd_info->osd.fg0.h > lcd_info->panel.h ) {
		lcd_info->osd.fg0.h = lcd_info->panel.h - lcd_info->osd.fg0.y;
	}

#if 0
	/* Foreground 1 */
	/* Case TVE ??? TVE 720x573 or 720x480*/
	if ( lcd_info->osd.fg1.x >= lcd_info->panel.w ) 
		lcd_info->osd.fg1.x = lcd_info->panel.w;
	if ( lcd_info->osd.fg1.y >= lcd_info->panel.h ) 
		lcd_info->osd.fg1.y = lcd_info->panel.h;
	if ( lcd_info->osd.fg1.x + lcd_info->osd.fg1.w > lcd_info->panel.w ) 
		lcd_info->osd.fg1.w = lcd_info->panel.w - lcd_info->osd.fg1.x;
	if ( lcd_info->osd.fg1.y + lcd_info->osd.fg1.h > lcd_info->panel.h ) 
		lcd_info->osd.fg1.h = lcd_info->panel.h - lcd_info->osd.fg1.y;
#endif
//      fg0_line_size = lcd_info->osd.fg0.w*((lcd_info->osd.fg0.bpp+7)/8);
	fg0_line_size = (lcd_info->osd.fg0.w*(lcd_info->osd.fg0.bpp)/8);
	fg0_line_size = ((fg0_line_size+3)>>2)<<2; /* word aligned */
	fg0_frm_size = fg0_line_size * lcd_info->osd.fg0.h;

	fg1_line_size = lcd_info->osd.fg1.w*((lcd_info->osd.fg1.bpp+7)/8);
	fg1_line_size = ((fg1_line_size+3)>>2)<<2; /* word aligned */
	fg1_frm_size = fg1_line_size * lcd_info->osd.fg1.h;

	if ( lcd_info->osd.fg_change ) {
		if ( lcd_info->osd.fg_change & FG0_CHANGE_POSITION ) { /* F1 change position */
			REG_LCD_XYP0 = lcd_info->osd.fg0.y << 16 | lcd_info->osd.fg0.x;
		}
		if ( lcd_info->osd.fg_change & FG1_CHANGE_POSITION ) { /* F1 change position */
			REG_LCD_XYP1 = lcd_info->osd.fg1.y << 16 | lcd_info->osd.fg1.x;
		}

		/* set change */
		if ( !(lcd_info->osd.osd_ctrl & LCD_OSDCTRL_IPU) &&
			(lcd_info->osd.fg_change != FG_CHANGE_ALL) ) {
			REG_LCD_OSDCTRL |= LCD_OSDCTRL_CHANGES;
		}

                /* wait change ready??? */
//              while ( REG_LCD_OSDS & LCD_OSDS_READY ) /* fix in the future, Wolfgang, 06-20-2008 */
		print_dbg("wait LCD_OSDS_READY\n");

		if ( lcd_info->osd.fg_change & FG0_CHANGE_SIZE ) { /* change FG0 size */
			if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* output to TV */
				dma0_desc0->cmd = dma0_desc1->cmd = (fg0_frm_size/4)/2;
				dma0_desc0->offsize = dma0_desc1->offsize
					= fg0_line_size/4;
				dma0_desc0->page_width = dma0_desc1->page_width
					= fg0_line_size/4;
				dma0_desc1->databuf = virt_to_phys((void *)(lcd_frame0 + fg0_line_size));
				REG_LCD_DA0 = virt_to_phys(dma0_desc0); //tft
			} else {
				dma0_desc0->cmd = dma0_desc1->cmd = fg0_frm_size/4;
				dma0_desc0->offsize = dma0_desc1->offsize =0;
				dma0_desc0->page_width = dma0_desc1->page_width = 0;
			}

			dma0_desc0->desc_size = dma0_desc1->desc_size
				= lcd_info->osd.fg0.h << 16 | lcd_info->osd.fg0.w;
			REG_LCD_SIZE0 = (lcd_info->osd.fg0.h<<16)|lcd_info->osd.fg0.w;

		}

		if ( lcd_info->osd.fg_change & FG1_CHANGE_SIZE ) { /* change FG1 size*/
			if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* output to TV */
				dma1_desc0->cmd = dma1_desc1->cmd = (fg1_frm_size/4)/2;
				dma1_desc0->offsize = dma1_desc1->offsize = fg1_line_size/4;
				dma1_desc0->page_width = dma1_desc1->page_width = fg1_line_size/4;
				dma1_desc1->databuf = virt_to_phys((void *)(lcd_frame1 + fg1_line_size));
				REG_LCD_DA1 = virt_to_phys(dma0_desc1); //tft

			} else {
				dma1_desc0->cmd = dma1_desc1->cmd = fg1_frm_size/4;
				dma1_desc0->offsize = dma1_desc1->offsize = 0;
				dma1_desc0->page_width = dma1_desc1->page_width = 0;
			}

			dma1_desc0->desc_size = dma1_desc1->desc_size
				= lcd_info->osd.fg1.h << 16 | lcd_info->osd.fg1.w;
			REG_LCD_SIZE1 = lcd_info->osd.fg1.h << 16|lcd_info->osd.fg1.w;
		}

//		dma_cache_wback((unsigned int)(dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4750_lcd_dma_desc));
		lcd_info->osd.fg_change = FG_NOCHANGE; /* clear change flag */
	}
}

static void jz4750fb_change_clock( struct jz4750lcd_info * lcd_info ) {

#if defined(CONFIG_FPGA)
	REG_LCD_REV = 0x00000004;
	sys_printf("Fuwa test, pixclk divide REG_LCD_REV=0x%08x\n", REG_LCD_REV);
	sys_printf("Fuwa test, pixclk %d\n", JZ_EXTAL/(((REG_LCD_REV&0xFF)+1)*2));
#else
	unsigned int val = 0;
	unsigned int pclk;
	/* Timing setting */
	__cpm_stop_lcd();

	val = lcd_info->panel.fclk; /* frame clk */

	if ( (lcd_info->panel.cfg & LCD_CFG_MODE_MASK) != LCD_CFG_MODE_SERIAL_TFT) {
		pclk = val * (lcd_info->panel.w + lcd_info->panel.hsw + lcd_info->panel.elw + lcd_info->panel.blw) * (lcd_info->panel.h + lcd_info->panel.vsw + lcd_info->panel.efw + lcd_info->panel.bfw); /* Pixclk */
	} else {
		/* serial mode: Hsync period = 3*Width_Pixel */
		pclk = val * (lcd_info->panel.w*3 + lcd_info->panel.hsw + lcd_info->panel.elw + lcd_info->panel.blw) * (lcd_info->panel.h + lcd_info->panel.vsw + lcd_info->panel.efw + lcd_info->panel.bfw); /* Pixclk */
	}

        /********* In TVE mode PCLK = 27MHz ***********/
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) {   /* LCDC output to TVE */
		REG_CPM_LPCDR  |= CPM_LPCDR_LTCS;
		pclk = 27000000;
		val = __cpm_get_pllout2() / pclk; /* pclk */
		val--;
		__cpm_set_pixdiv(val);

		dprintk("REG_CPM_LPCDR = 0x%08x\n", REG_CPM_LPCDR);
#if defined(CONFIG_SOC_JZ4750) /* Jz4750D don't use LCLK */
		val = pclk * 3 ;        /* LCDClock > 2.5*Pixclock */

		val =(__cpm_get_pllout()) / val;
		if ( val > 0x1f ) {
			sys_printf("lcd clock divide is too large, set it to 0x1f\n");
			val = 0x1f;
		}
		__cpm_set_ldiv( val );
#endif
		__cpm_select_pixclk_tve();

		REG_CPM_CPCCR |= CPM_CPCCR_CE ; /* update divide */
	} else {          /* LCDC output to  LCD panel */
		val = __cpm_get_pllout2() / pclk; /* pclk */
		val--;
		dprintk("ratio: val = %d\n", val);
		if ( val > 0x7ff ) {
			sys_printf("pixel clock divid is too large, set it to 0x7ff\n");
			val = 0x7ff;
		}

		__cpm_set_pixdiv(val);

		dprintk("REG_CPM_LPCDR = 0x%08x\n", REG_CPM_LPCDR);
#if defined(CONFIG_SOC_JZ4750) /* Jz4750D don't use LCLK */
		val = pclk * 3 ;        /* LCDClock > 2.5*Pixclock */
		val =__cpm_get_pllout2() / val;
		if ( val > 0x1f ) {
			sys_printf("lcd clock divide is too large, set it to 0x1f\n");
			val = 0x1f;
		}
		__cpm_set_ldiv( val );
#endif

		__cpm_select_pixclk_lcd();

		REG_CPM_CPCCR |= CPM_CPCCR_CE ; /* update divide */

	}

	dprintk("REG_CPM_LPCDR=0x%08x\n", REG_CPM_LPCDR);
	dprintk("REG_CPM_CPCCR=0x%08x\n", REG_CPM_CPCCR);

	jz_clocks.pixclk = __cpm_get_pixclk();
	sys_printf("LCDC: PixClock:%d\n", jz_clocks.pixclk);

#if defined(CONFIG_SOC_JZ4750) /* Jz4750D don't use LCLK */
	jz_clocks.lcdclk = __cpm_get_lcdclk();
	sys_printf("LCDC: LcdClock:%d\n", jz_clocks.lcdclk);
#endif
	__cpm_start_lcd();
	sys_udelay(1000);
        /* 
         * set lcd device clock and lcd pixel clock.
         * what about TVE mode???
         *
         */
#endif

}


/* 
 * jz4750fb_set_mode(), set osd configure, resize foreground
 *
 */
static void jz4750fb_set_mode( struct jz4750lcd_info * lcd_info ) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	jz4750fb_set_osd_mode(lcd_info);
	jz4750fb_foreground_resize(lcd_info);
	jz4750fb_set_var(&cfb->fb.var, -1, &cfb->fb);
}


/*
 * jz4750fb_deep_set_mode,
 *
 */
static void jz4750fb_deep_set_mode( struct jz4750lcd_info * lcd_info ) {
	/* configurate sequence:
	 * 1. disable lcdc.
	 * 2. init frame descriptor.
	 * 3. set panel mode
	 * 4. set osd mode
	 * 5. start lcd clock in CPM
	 * 6. enable lcdc.
	 */

	__lcd_clr_ena();        /* Quick Disable */
	lcd_info->osd.fg_change = FG_CHANGE_ALL; /* change FG0, FG1 size, postion??? */
	jz4750fb_descriptor_init(lcd_info);
	jz4750fb_set_panel_mode(lcd_info);
	jz4750fb_set_mode(lcd_info);
	jz4750fb_change_clock(lcd_info);
	__lcd_set_ena();        /* enable lcdc */
}


#if 0
static int lcd_interrupt_handler(int irq_no, void *dum) {
	unsigned int state;

	state = LCDC->state;

//	sys_printf("lcd irq\n");
	if (state & LCDC_STATE_EOF) {
		LCDC->state = state & ~LCDC_STATE_EOF;
	} else if (state & LCDC_STATE_IFU0) {
		LCDC->state = state & ~LCDC_STATE_IFU0;
	} else if (state & LCDC_STATE_OUF) {
		LCDC->state = state & ~LCDC_STATE_OUF;
	}
	return 0;
}
#endif



static int jz4750fb_interrupt_handler(int irq, void *dev_id) {
	unsigned int state;
	static int irqcnt=0;

	state = REG_LCD_STATE;
	dprintk("In the lcd interrupt handler, state=0x%x\n", state);

	if (state & LCD_STATE_EOF) { /* End of frame */
		REG_LCD_STATE = state & ~LCD_STATE_EOF;
	}

	if (state & LCD_STATE_IFU0) {
		sys_printf("%s, InFiFo0 underrun\n", __FUNCTION__);
		REG_LCD_STATE = state & ~LCD_STATE_IFU0;
	}

	if (state & LCD_STATE_IFU1) {
		sys_printf("%s, InFiFo1 underrun\n", __FUNCTION__);
		REG_LCD_STATE = state & ~LCD_STATE_IFU1;
	}

	if (state & LCD_STATE_OFU) { /* Out fifo underrun */
		REG_LCD_STATE = state & ~LCD_STATE_OFU;
		if ( irqcnt++ > 100 ) {
			__lcd_disable_ofu_intr();
			sys_printf("disable Out FiFo underrun irq.\n");
		}
		sys_printf("%s, Out FiFo underrun.\n", __FUNCTION__);
	}

	return 0;
}


//-------------------------

/* Backlight Control Interface via sysfs 
 * 
 * LCDC:
 * Enabling LCDC when LCD backlight is off will only affects cfb->display.
 *
 * Backlight:
 * Changing the value of LCD backlight when LCDC is off will only affect the cfb->backlight_level.
 *
 * - River. 
 */


static int jz4750fb_lcd_display_off(void) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	__lcd_close_backlight();
	__lcd_display_off();

#ifdef HAVE_LCD_PWM_CONTROL 
	if (cfb->b_lcd_pwm) {
		__lcd_pwm_stop();
		cfb->b_lcd_pwm = 0;
	}
#endif

	cfb->b_lcd_display = 0;

	return 0;
}

static int jz4750fb_lcd_display_on(void) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	__lcd_display_on();

	/* Really restore LCD backlight when LCD backlight is turned on. */
	if (cfb->backlight_level) {
#ifdef HAVE_LCD_PWM_CONTROL
		if (!cfb->b_lcd_pwm) {
			__lcd_pwm_start();
			cfb->b_lcd_pwm = 1;
		}
#endif
		__lcd_set_backlight_level(cfb->backlight_level);
	}

	cfb->b_lcd_display = 1;

	return 0;
}

static int jz4750fb_set_backlight_level(int n) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	if (n) {
		if (n > LCD_MAX_BACKLIGHT)
			n = LCD_MAX_BACKLIGHT;

		if (n < LCD_MIN_BACKLIGHT)
			n = LCD_MIN_BACKLIGHT;

		/* Really change the value of backlight when LCDC is enabled. */
		if (cfb->b_lcd_display) {
			if (n<LCD_MAX_BACKLIGHT) {
#ifdef HAVE_LCD_PWM_CONTROL             
				if (!cfb->b_lcd_pwm) {
					__lcd_pwm_start();
					cfb->b_lcd_pwm = 1;
				}
#endif
				__lcd_set_backlight_level(n);
#if 0
			} else {
				__lcd_set_backlight_max();
#endif
			}
		}
	} else {
		/* Turn off LCD backlight. */
		__lcd_close_backlight();

#ifdef HAVE_LCD_PWM_CONTROL
		if (cfb->b_lcd_pwm) {
			__lcd_pwm_stop();
			cfb->b_lcd_pwm = 0;
		}
#endif
	}

	cfb->backlight_level = n;

	return 0;
}

#if 0
static ssize_t show_bl_level(struct device *device,
                             struct device_attribute *attr, char *buf) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	return snprintf(buf, PAGE_SIZE, "%d\n", cfb->backlight_level);
}

static ssize_t store_bl_level(struct device *device,
                struct device_attribute *attr,
                const char *buf, size_t count) {
	int n;
	char *ep;

	n = simple_strtoul(buf, &ep, 0);
	if (*ep && *ep != '\n')
		return -EINVAL;

	jz4750fb_set_backlight_level(n);

	return count;
}

static struct device_attribute device_attrs[] = {
        __ATTR(backlight_level, S_IRUGO | S_IWUSR, show_bl_level, store_bl_level),
};

static int jz4750fb_device_attr_register(struct fb_info *fb_info) {
	int error = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) { 
		error = device_create_file(fb_info->dev, &device_attrs[i]);

		if (error)
			break;
	}

	if (error) {
		while (--i >= 0)
			device_remove_file(fb_info->dev, &device_attrs[i]);
	}

	return 0;
}

static int jz4750fb_device_attr_unregister(struct fb_info *fb_info) {
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++)
		device_remove_file(fb_info->dev, &device_attrs[i]);

	return 0;
}
#endif
/* End */


/* =============== Driver Init routines ============== */

static void  jz4750fb_drv_cleanup(void) {
	struct lcd_cfb_info *cfb = jz4750fb_info;

	jz4750fb_unmap_smem(cfb);
	jz4750fb_free_fb_info(cfb);

	return;
}

static int jz4750fb_drv_init(void) {
	struct lcd_cfb_info *cfb;
	int err;

	if ( jz4750_lcd_info->osd.fg0.bpp > 16 &&
		jz4750_lcd_info->osd.fg0.bpp < 32 ) {
		jz4750_lcd_info->osd.fg0.bpp = 32;
	}

	switch ( jz4750_lcd_info->osd.fg1.bpp ) {
		case 15:
		case 16:
			break;
		case 17 ... 32:
			jz4750_lcd_info->osd.fg1.bpp = 32;
			break;
		default:
			sys_printf("jz4750fb fg1 not support bpp(%d), force to 32bpp\n",
				jz4750_lcd_info->osd.fg1.bpp);
			jz4750_lcd_info->osd.fg1.bpp = 32;
	}

	cfb = jz4750fb_alloc_fb_info();
	if (!cfb)
		return -ENOMEM;

	jz4750fb_info = cfb;

	err = jz4750fb_map_smem(cfb);
	if (err) {
		jz4750fb_drv_cleanup();
		return err;
	}

	return 0;
}



static int jz4750fb_drv_register(void) {
	struct lcd_cfb_info *cfb = jz4750fb_info;
	int err = 0;

#ifdef CONFIG_PM
/*
 * Note that the console registers this as well, but we want to
 * power down the display prior to sleeping.
 */
	cfb->pm = pm_register(PM_SYS_DEV, PM_SYS_VGA, jzlcd_pm_callback);
	if (cfb->pm)
		cfb->pm->data = cfb;
#endif
	err = register_framebuffer(&cfb->fb);
	if (err < 0) {
		dprintk("jzfb_init(): register framebuffer err.\n");
		return err;
	}

	sys_printf("fb%d: %s frame buffer device, using %dK of video memory\n",
		cfb->fb.node, cfb->fb.fix.id, cfb->fb.fix.smem_len>>10);

//	jz4750fb_device_attr_register(&cfb->fb);

	return err;
}


/* ========== LCDC Controller HW Init routines. =========== */
static void jz4750fb_dev_enable(void) {
	__lcd_set_ena();        /* enalbe LCD Controller */
	jz4750fb_lcd_display_on();

	return;
}
static int jz4750fb_dev_init(void) {
	__lcd_clr_dis();
	__lcd_clr_ena();

	/* gpio init __gpio_as_lcd */
	if (jz4750_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_16BIT) {
		__gpio_as_lcd_16bit();
        } else if (jz4750_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_24BIT) {
		__gpio_as_lcd_24bit();
        } else {
		__gpio_as_lcd_18bit();
	}

	/* In special mode, we only need init special pin, 
	 * as general lcd pin has init in uboot */
#if defined(CONFIG_SOC_JZ4750) || defined(CONFIG_SOC_JZ4750D)
	switch (jz4750_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) {
		case LCD_CFG_MODE_SPECIAL_TFT_1:
		case LCD_CFG_MODE_SPECIAL_TFT_2:
		case LCD_CFG_MODE_SPECIAL_TFT_3:
			__gpio_as_lcd_special();
			break;
		default:
			;
	}
#endif
        /* Configure SLCD module for setting smart lcd control registers */
#if defined(CONFIG_FB_JZ4750_SLCD)
	__lcd_as_smart_lcd();
	__slcd_disable_dma();
	__init_slcd_bus();      /* Note: modify this depend on you lcd */

#endif
	/* init clk */
	jz4750fb_change_clock(jz4750_lcd_info);
	__lcd_display_pin_init();
	__lcd_slcd_special_on();

	install_irq_handler(LCD_IRQ, jz4750fb_interrupt_handler,0);

	return 0;
}



static struct device_handle *jz_lcd_open(void *instance, DRV_CBH cb_h, void *dum) {
	return 0;
}

static int jz_lcd_close(struct device_handle *dh) {
	return 0;
}

static int jz_lcd_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	return 0;
}

static int jz_lcd_init(void *inst) {
	return 0;
}

static int jz_lcd_start(void *inst) {
	struct lcd_cfb_info *cfb;
	int	err=0;

	err=jz4750fb_drv_init();
	if (err) {
		return err;
	}

	err=jz4750fb_dev_init();
	if (err) {
		jz4750fb_drv_cleanup();
		return err;
	}

	jz4750fb_deep_set_mode(jz4750_lcd_info);
	jz4750fb_drv_register();
	
	print_lcdc_registers();

	return 0;
}

static struct driver_ops jz_lcd_drv_ops = {
	jz_lcd_open,
	jz_lcd_close,
	jz_lcd_control,
	jz_lcd_init,
	jz_lcd_start,
};

static struct driver jz_lcd_drv = {
	"jz_lcd",
	0,
	&jz_lcd_drv_ops,
};

void init_jz_lcd_drv(void) {
	driver_publish(&jz_lcd_drv);
}

INIT_FUNC(init_jz_lcd_drv);
