

#include <types.h>
#include <mm.h>
#include <io.h>
#include <string.h>
#include <fb.h>
#include "fb_draw.h"

#if BITS_PER_LONG == 32
#  define FB_WRITEL fb_writel
#  define FB_READL  fb_readl
#else
#  define FB_WRITEL fb_writeq
#  define FB_READL  fb_readq
#endif


static void
bitfill_aligned(unsigned long *dst, int dst_idx, unsigned long pat,
                unsigned n, int bits, unsigned int bswapmask) {
	unsigned long first, last;

	if (!n) return;

	first = fb_shifted_pixels_mask_long(dst_idx, bswapmask);
	last = ~fb_shifted_pixels_mask_long((dst_idx+n) % bits, bswapmask);

	if (dst_idx+n <= bits) {
		// Single word
		if (last) first &= last;
		FB_WRITEL(comp(pat, FB_READL(dst), first), dst);
	} else {
		// Multiple destination words

		// Leading bits
		if (first!= ~0UL) {
			FB_WRITEL(comp(pat, FB_READL(dst), first), dst);
			dst++;
			n -= bits - dst_idx;
		}

		// Main chunk
		n /= bits;
		while (n >= 8) {
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        FB_WRITEL(pat, dst++);
                        n -= 8;
                }
                while (n--) FB_WRITEL(pat, dst++);

		// Trailing bits
		if (last)
			FB_WRITEL(comp(pat, FB_READL(dst), last), dst);
	}
}

static void
bitfill_unaligned(unsigned long  *dst, int dst_idx, unsigned long pat,
                        int left, int right, unsigned n, int bits) {
	unsigned long first, last;

	if (!n) return;

	first = FB_SHIFT_HIGH(~0UL, dst_idx);
	last = ~(FB_SHIFT_HIGH(~0UL, (dst_idx+n) % bits));

	if (dst_idx+n <= bits) {
		// Single word
		if (last) first &= last;
		FB_WRITEL(comp(pat, FB_READL(dst), first), dst);
        } else {
		// Multiple destination words
		// Leading bits
                if (first) {
			FB_WRITEL(comp(pat, FB_READL(dst), first), dst);
			dst++;
			pat = pat << left | pat >> right;
			n -= bits - dst_idx;
		}

		// Main chunk
		n /= bits;
		while (n >= 4) {
			FB_WRITEL(pat, dst++);
			pat = pat << left | pat >> right;
			FB_WRITEL(pat, dst++);
			pat = pat << left | pat >> right;
			FB_WRITEL(pat, dst++);
			pat = pat << left | pat >> right;
			FB_WRITEL(pat, dst++);
			pat = pat << left | pat >> right;
			n -= 4;
		}
		while (n--) {
			FB_WRITEL(pat, dst++);
			pat = pat << left | pat >> right;
		}

		// Trailing bits
		if (last)
			FB_WRITEL(comp(pat, FB_READL(dst), first), dst);
	}
}

static void
bitfill_aligned_rev(unsigned long *dst, int dst_idx, unsigned long pat,
                unsigned n, int bits, unsigned int bswapmask) {
	unsigned long val = pat, dat;
	unsigned long first, last;

	if (!n) return;

	first = fb_shifted_pixels_mask_long(dst_idx, bswapmask);
	last = ~fb_shifted_pixels_mask_long((dst_idx+n) % bits, bswapmask);

	if (dst_idx+n <= bits) {
		// Single word
		if (last) first &= last;
		dat = FB_READL(dst);
		FB_WRITEL(comp(dat ^ val, dat, first), dst);
	} else {
		// Multiple destination words
		// Leading bits
		if (first!=0UL) {
			dat = FB_READL(dst);
			FB_WRITEL(comp(dat ^ val, dat, first), dst);
			dst++;
			n -= bits - dst_idx;
		}

		// Main chunk
		n /= bits;
		while (n >= 8) {
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
			n -= 8;
		}
		while (n--) {
			FB_WRITEL(FB_READL(dst) ^ val, dst);
			dst++;
		}

		// Trailing bits
		if (last) {
			dat = FB_READL(dst);
			FB_WRITEL(comp(dat ^ val, dat, last), dst);
		}
	}
}

static void
bitfill_unaligned_rev(unsigned long *dst, int dst_idx, unsigned long pat,
                        int left, int right, unsigned n, int bits) {
	unsigned long first, last, dat;

	if (!n) return;

	first = FB_SHIFT_HIGH(~0UL, dst_idx);
	last = ~(FB_SHIFT_HIGH(~0UL, (dst_idx+n) % bits));

	if (dst_idx+n <= bits) {
		// Single word
		if (last) first &= last;
		dat = FB_READL(dst);
		FB_WRITEL(comp(dat ^ pat, dat, first), dst);
	} else {
		// Multiple destination words
		// Leading bits
		if (first != 0UL) {
			dat = FB_READL(dst);
			FB_WRITEL(comp(dat ^ pat, dat, first), dst);
			dst++;
			pat = pat << left | pat >> right;
			n -= bits - dst_idx;
		}

		// Main chunk
		n /= bits;
		while (n >= 4) {
			FB_WRITEL(FB_READL(dst) ^ pat, dst);
			dst++;
			pat = pat << left | pat >> right;
			FB_WRITEL(FB_READL(dst) ^ pat, dst);
			dst++;
			pat = pat << left | pat >> right;
			FB_WRITEL(FB_READL(dst) ^ pat, dst);
			dst++;
			pat = pat << left | pat >> right;
			FB_WRITEL(FB_READL(dst) ^ pat, dst);
			dst++;
			pat = pat << left | pat >> right;
			n -= 4;
		}

		while (n--) {
			FB_WRITEL(FB_READL(dst) ^ pat, dst);
			dst++;
			pat = pat << left | pat >> right;
		}

		// Trailing bits
		if (last) {
			dat = FB_READL(dst);
			FB_WRITEL(comp(dat ^ pat, dat, last), dst);
		}
	}
}

void cfb_fillrect(struct fb_info *p, const struct fb_fillrect *rect) {
	unsigned long pat, fg;
	unsigned long width = rect->width, height = rect->height;
	int bits = BITS_PER_LONG, bytes = bits >> 3;
	unsigned int bpp = p->var.bits_per_pixel;
	unsigned long *dst;
	int dst_idx, left;

	if (p->state != FBINFO_STATE_RUNNING) return;

	if (p->fix.visual == FB_VISUAL_TRUECOLOR ||
		p->fix.visual == FB_VISUAL_DIRECTCOLOR )
		fg = ((unsigned int *) (p->pseudo_palette))[rect->color];
	else
		fg = rect->color;

	pat = pixel_to_pat( bpp, fg);

	dst = (unsigned long *)((unsigned long)p->screen_base & ~(bytes-1));
        dst_idx = ((unsigned long)p->screen_base & (bytes - 1))*8;
        dst_idx += rect->dy*p->fix.line_length*8+rect->dx*bpp;
        /* FIXME For now we support 1-32 bpp only */
	left = bits % bpp;
	if (p->fbops->fb_sync) p->fbops->fb_sync(p);
	if (!left) {
		unsigned int bswapmask = fb_compute_bswapmask(p);
		void (*fill_op32)(unsigned long *dst, int dst_idx,
					unsigned long pat, unsigned n, int bits,
					unsigned int bswapmask) = NULL;

		switch (rect->rop) {
		case ROP_XOR:
			fill_op32 = bitfill_aligned_rev;
			break;
		case ROP_COPY:
			fill_op32 = bitfill_aligned;
			break;
		default:
			sys_printf("cfb_fillrect(): unknown rop, defaulting to ROP_COPY\n");
			fill_op32 = bitfill_aligned;
			break;
		}
		while (height--) {
			dst += dst_idx >> (ffs(bits) - 1);
			dst_idx &= (bits - 1);
			fill_op32(dst, dst_idx, pat, width*bpp, bits, bswapmask);
			dst_idx += p->fix.line_length*8;
		}
	} else {
		int right;
		int r;
		int rot = (left-dst_idx) % bpp;
		void (*fill_op)(unsigned long *dst, int dst_idx,
			unsigned long pat, int left, int right,
			unsigned n, int bits) = NULL;

		/* rotate pattern to correct start position */
		pat = pat << rot | pat >> (bpp-rot);

		right = bpp-left;
		switch (rect->rop) {
		case ROP_XOR:
			fill_op = bitfill_unaligned_rev;
			break;
		case ROP_COPY:
			fill_op = bitfill_unaligned;
			break;
		default:
			sys_printf("cfb_fillrect(): unknown rop, defaulting to ROP_COPY\n");
			fill_op = bitfill_unaligned;
			break;
		}

		while (height--) {
			dst += dst_idx >> (ffs(bits) - 1);
			dst_idx &= (bits - 1);
			fill_op(dst, dst_idx, pat, left, right,
				width*bpp, bits);
			r = (p->fix.line_length*8) % bpp;
			pat = pat << (bpp-r) | pat >> r;
			dst_idx += p->fix.line_length*8;
		}
	}
}
