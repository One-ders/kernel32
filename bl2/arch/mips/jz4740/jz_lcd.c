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
#include "io.h"
#include "sys.h"
#include "irq.h"
#include "jz_lcd.h"

#include "clocks.h"

#include <devices.h>
#include <fb.h>

#include <string.h>
#include <malloc.h>

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
	int	rc=0;

	__gpio_as_lcd_18bit();

// CONFIG_SOC_JZ4740
#if 0
// but special modes are for SHARP,CASIO,SAMSUN monochromes
	switch(jzfb.cfg&LCDC_CFG_MODE_MASK) {
		case	LCDC_CFG_MODE_SPECIAL_TFT1:
		case	LCDC_CFG_MODE_SPECIAL_TFT2:
		case	LCDC_CFG_MODE_SPECIAL_TFT3:
			gpio_as_lcd_special();
			break;
		default:;
	}
#endif

	__lcd_display_pin_init();

	cfb=fb_alloc_fb_info();
	if (!cfb) goto out_err;

	rc=fb_map_smem(cfb);
	if (rc) goto out_err;

	jzfb_set_var(&cfb->fb.var, -1, &cfb->fb);

	lcd_descriptor_init();

	rc=lcd_hw_init();
	if (rc) goto out_err;

	if (jzfb.bpp<=8) {
		LCDC->DA0=virt_to_phys(lcd_palette_desc);
	} else {
		LCDC->DA0=virt_to_phys(lcd_frame_desc0);
	}

	if (((jzfb.cfg&LCDC_CFG_MODE_MASK)==LCDC_CFG_MODE_D_C_STN) ||
		((jzfb.cfg&LCDC_CFG_MODE_MASK)==LCDC_CFG_MODE_D_M_STN)) {
		LCDC->DA1=virt_to_phys(lcd_frame_desc1);
	}

	lcd_set_ena();

	install_irq_handler(LCD_IRQ, lcd_interrupt_handler,0);

	lcd_enable_ofu_intr();

	rc=register_framebuffer(&cfb->fb);
	if (rc<0) {
		sys_printf("jz_lcd_init: register framebuffer err %d\n",rc);
		goto out_err;
	}

	sys_printf("fb%d: %s frame buffer device, using %dK of video memory\n",
		cfb->fb.node, cfb->fb.fix.id, cfb->fb.fix.smem_len>>10);

	__lcd_display_on();

	return 0;

out_err:
	fb_unmap_smem(cfb);
	fb_free_fb_info(cfb);

	return rc;
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
