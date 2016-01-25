
#include <types.h>
#include <sys.h>
#include <fb.h>

#include "fb_draw.h"

#define __LITTLE_ENDIAN

static const unsigned int cfb_tab8[] = {
#if defined(__BIG_ENDIAN)
    0x00000000,0x000000ff,0x0000ff00,0x0000ffff,
    0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff,
    0xff000000,0xff0000ff,0xff00ff00,0xff00ffff,
    0xffff0000,0xffff00ff,0xffffff00,0xffffffff
#elif defined(__LITTLE_ENDIAN)
    0x00000000,0xff000000,0x00ff0000,0xffff0000,
    0x0000ff00,0xff00ff00,0x00ffff00,0xffffff00,
    0x000000ff,0xff0000ff,0x00ff00ff,0xffff00ff,
    0x0000ffff,0xff00ffff,0x00ffffff,0xffffffff
#else
#error FIXME: No endianness??
#endif
};

static const unsigned int cfb_tab16[] = {
#if defined(__BIG_ENDIAN)
    0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff
#elif defined(__LITTLE_ENDIAN)
    0x00000000, 0xffff0000, 0x0000ffff, 0xffffffff
#else
#error FIXME: No endianness??
#endif
};

static const unsigned int cfb_tab32[] = {
        0x00000000, 0xffffffff
};

#define FB_WRITEL fb_writel
#define FB_READL  fb_readl


static inline void color_imageblit(const struct fb_image *image,
                                   struct fb_info *p, unsigned char *dst1,
                                   unsigned int start_index,
                                   unsigned int pitch_index) {
        /* Draw the penguin */
        unsigned int *dst, *dst2;
        unsigned int color = 0, val, shift;
        int i, n, bpp = p->var.bits_per_pixel;
        unsigned int null_bits = 32 - bpp;
        unsigned int *palette = (unsigned int *) p->pseudo_palette;
        const unsigned char *src = image->data;
        unsigned int bswapmask = fb_compute_bswapmask(p);

        dst2 = (unsigned int *) dst1;
        for (i = image->height; i--; ) {
                n = image->width;
                dst = (unsigned int *) dst1;
                shift = 0;
                val = 0;

                if (start_index) {
                        unsigned int start_mask = ~fb_shifted_pixels_mask_u32(start_index, bswapmask);
                        val = FB_READL(dst) & start_mask;
                        shift = start_index;
                }
                while (n--) {
                        if (p->fix.visual == FB_VISUAL_TRUECOLOR ||
                            p->fix.visual == FB_VISUAL_DIRECTCOLOR )
                                color = palette[*src];
                        else
                                color = *src;
                        color <<= FB_LEFT_POS(bpp);
                        val |= FB_SHIFT_HIGH(color, shift ^ bswapmask);
                        if (shift >= null_bits) {
                                FB_WRITEL(val, dst++);

                                val = (shift == null_bits) ? 0 :
                                        FB_SHIFT_LOW(color, 32 - shift);
                        }
                        shift += bpp;
                        shift &= (32 - 1);
                        src++;
		}
                if (shift) {
                        unsigned int end_mask = fb_shifted_pixels_mask_u32(shift, bswapmask);

                        FB_WRITEL((FB_READL(dst) & end_mask) | val, dst);
                }
                dst1 += p->fix.line_length;
                if (pitch_index) {
                        dst2 += p->fix.line_length;
                        dst1 = (unsigned char *)((long )dst2 & ~(sizeof(unsigned int) - 1));

                        start_index += pitch_index;
                        start_index &= 32 - 1;
                }
        }
}

static inline void slow_imageblit(const struct fb_image *image, 
				struct fb_info *p,
				unsigned char *dst1, unsigned int fgcolor,
				unsigned int bgcolor,
				unsigned int start_index,
				unsigned int pitch_index) {
        unsigned int shift, color = 0, bpp = p->var.bits_per_pixel;
        unsigned int *dst, *dst2;
        unsigned int val, pitch = p->fix.line_length;
        unsigned int null_bits = 32 - bpp;
        unsigned int spitch = (image->width+7)/8;
        const unsigned char *src = image->data, *s;
        unsigned int i, j, l;
        unsigned int bswapmask = fb_compute_bswapmask(p);

        dst2 = (unsigned int *) dst1;
        fgcolor <<= FB_LEFT_POS(bpp);
        bgcolor <<= FB_LEFT_POS(bpp);

        for (i = image->height; i--; ) {
                shift = val = 0;
                l = 8;
                j = image->width;
                dst = (unsigned int *) dst1;
                s = src;

                /* write leading bits */
                if (start_index) {
                        unsigned int start_mask = ~fb_shifted_pixels_mask_u32(start_index, bswapmask);
                        val = FB_READL(dst) & start_mask;
                        shift = start_index;
                }

                while (j--) {
			l--;
                        color = (*s & (1 << l)) ? fgcolor : bgcolor;
                        val |= FB_SHIFT_HIGH(color, shift ^ bswapmask);

                        /* Did the bitshift spill bits to the next long? */
                        if (shift >= null_bits) {
                                FB_WRITEL(val, dst++);
                                val = (shift == null_bits) ? 0 :
                                        FB_SHIFT_LOW(color,32 - shift);
                        }
                        shift += bpp;
                        shift &= (32 - 1);
                        if (!l) { l = 8; s++; };
                }

                /* write trailing bits */
                if (shift) {
                        unsigned int end_mask = fb_shifted_pixels_mask_u32(shift, bswapmask);

                        FB_WRITEL((FB_READL(dst) & end_mask) | val, dst);
                }

                dst1 += pitch;
                src += spitch;
                if (pitch_index) {
                        dst2 += pitch;
                        dst1 = (unsigned char *)((long)dst2 & ~(sizeof(unsigned int) - 1));
                        start_index += pitch_index;
                        start_index &= 32 - 1;
                }

        }
}

static inline void fast_imageblit(const struct fb_image *image, 
				struct fb_info *p,
				unsigned char *dst1, unsigned int fgcolor,
				unsigned int bgcolor)
{
        unsigned int fgx = fgcolor, bgx = bgcolor, bpp = p->var.bits_per_pixel;
        unsigned int ppw = 32/bpp, spitch = (image->width + 7)/8;
        unsigned int bit_mask, end_mask, eorx, shift;
        const char *s = image->data, *src;
        unsigned int *dst;
        const unsigned int *tab = NULL;
        int i, j, k;

        switch (bpp) {
        case 8:
                tab = cfb_tab8;
                break;
        case 16:
                tab = cfb_tab16;
                break;
        case 32:
        default:
                tab = cfb_tab32;
                break;
        }

        for (i = ppw-1; i--; ) {
                fgx <<= bpp;
                bgx <<= bpp;
                fgx |= fgcolor;
                bgx |= bgcolor;
        }

        bit_mask = (1 << ppw) - 1;
        eorx = fgx ^ bgx;
        k = image->width/ppw;

        for (i = image->height; i--; ) {
                dst = (unsigned int *) dst1, shift = 8; src = s;

                for (j = k; j--; ) {
			shift -= ppw;
                        end_mask = tab[(*src >> shift) & bit_mask];
                        FB_WRITEL((end_mask & eorx)^bgx, dst++);
                        if (!shift) { shift = 8; src++; }       
                }
                dst1 += p->fix.line_length;
                s += spitch;
        }
}

void cfb_imageblit(struct fb_info *p, const struct fb_image *image) {
        unsigned int fgcolor, bgcolor, start_index, bitstart, pitch_index = 0;
        unsigned int bpl = sizeof(unsigned int), bpp = p->var.bits_per_pixel;
        unsigned int width = image->width;
        unsigned int dx = image->dx, dy = image->dy;
        unsigned char *dst1;

        if (p->state != FBINFO_STATE_RUNNING)
                return;

        bitstart = (dy * p->fix.line_length * 8) + (dx * bpp);
        start_index = bitstart & (32 - 1);
        pitch_index = (p->fix.line_length & (bpl - 1)) * 8;

        bitstart /= 8;
        bitstart &= ~(bpl - 1);
        dst1 = p->screen_base + bitstart;

        if (p->fbops->fb_sync)
                p->fbops->fb_sync(p);

        if (image->depth == 1) {
                if (p->fix.visual == FB_VISUAL_TRUECOLOR ||
                    p->fix.visual == FB_VISUAL_DIRECTCOLOR) {
                        fgcolor = ((unsigned int *)(p->pseudo_palette))[image->fg_color];
                        bgcolor = ((unsigned int *)(p->pseudo_palette))[image->bg_color];
                } else {
                        fgcolor = image->fg_color;
                        bgcolor = image->bg_color;
                }

                if (32 % bpp == 0 && !start_index && !pitch_index &&
                    ((width & (32/bpp-1)) == 0) &&
                    bpp >= 8 && bpp <= 32)
                        fast_imageblit(image, p, dst1, fgcolor, bgcolor);
                else
                        slow_imageblit(image, p, dst1, fgcolor, bgcolor,
                                        start_index, pitch_index);
        } else
                color_imageblit(image, p, dst1, start_index, pitch_index);
}

