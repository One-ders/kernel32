
#include <sys.h>
#include <io.h>
#include <fb.h>
#include <malloc.h>

#define FBPIXMAPSIZE    (1024 * 8)

struct fb_data {
	struct fb_info *fi;
};

#define FB_MAX 1
static int num_registered_fb;
static struct fb_data fb_data[FB_MAX];

struct my_userdata {
	DEVICE_HANDLE();
	struct fb_data *fb_data;
	unsigned int ppos;
};

struct my_userdata my_userdata[4];

DEVICE_ROOT(root);

static struct device_handle *fb_open(void *instance,
					DRV_CBH cb, void *dum) {

//	struct device_handle *dh;
	struct my_userdata *ud;
	struct fb_data *fb_data=(struct fb_data *)instance;
	struct fb_info  *fi=fb_data->fi;
	int rc=0;

	ud=(struct my_userdata *)driver_user_get_udata(root);
	if (!ud) return 0;

        ud->fb_data=fb_data;
	ud->ppos=0;


	if (fi->fbops->fb_open) {
		rc=fi->fbops->fb_open(fi,1);
	}

	if (rc) {
		driver_user_put_udata(root,DEVICE_H(ud));
		return 0;
	}

	sys_printf("fb_open: OK\n");

	return DEVICE_H(ud);
}

static int fb_close(struct device_handle *dh) {
	return 0;
}

static int fb_control(struct device_handle *dh,
			int cmd, void *arg, int arg_len) {

	struct my_userdata *ud=DEVICE_UDATA(struct my_userdata,dh);
	struct fb_info *fi=ud->fb_data->fi;

	switch (cmd) {
		case WR_CHAR: {
			unsigned long p=ud->ppos;
			unsigned int *buffer;
			unsigned int *src;
			unsigned int *dst;
			int c,i,cnt=0,err=0;
			unsigned long total_size;

			if (fi->fbops->fb_write) {
				int rc=fi->fbops->fb_write(fi,arg,arg_len,&ud->ppos);
				if (rc<0) return rc;
				ud->ppos+=rc;
				return rc;
			}
			total_size=fi->screen_size;
			if (total_size == 0) {
				total_size=fi->fix.smem_len;
			}

			if (p>total_size) return -EFBIG;
			if (arg_len>total_size) {
				err=-EFBIG;
				arg_len=total_size;
			}

			if (arg_len+p>total_size) {
				if (!err) {
					err=-ENOSPC;
				}
				arg_len=total_size-p;
			}
			buffer=get_page();
			if (!buffer) return -ENOMEM;
			dst=(unsigned int *)(fi->screen_base+p);

			if (fi->fbops->fb_sync) {
				fi->fbops->fb_sync(fi);
			}

			while(arg_len) {
				c=(arg_len>PAGE_SIZE)?PAGE_SIZE:arg_len;
				src=buffer;
				memcpy(src,arg,c);
				for(i=c>>2;i--;) {
					fb_writel(*src++,dst++);
				}
				if (c&3) {
					unsigned char *src8=(unsigned char *)src;
					unsigned char *dst8=(unsigned char *)dst;

					for(i=c&3;i--;) {
						fb_writeb(*src8++,dst8++);
					}
					dst=(unsigned int *)dst8;
				}
				ud->ppos+=c;
				arg+=c;
				cnt+=c;
				arg_len-=c;
			}

			put_page(buffer);
			return (cnt)?cnt:err;
		}
		case FBIOGET_VSCREENINFO:
			memcpy(arg, &fi->var,sizeof(fi->var));
			return 0;
		case FBIOGET_FSCREENINFO:
			return memcpy(arg, &fi->fix,sizeof(fi->fix));
		case IO_MMAP:
			return (((unsigned long int)fi->screen_base)&~0xa0000000)+((unsigned long)arg);
		default:
			if (!fi->fbops->fb_ioctl) return -EINVAL;
			return fi->fbops->fb_ioctl(fi, cmd, arg);
	}
	return 0;
}

static int fb_init (void *inst) {
	sys_printf("fb_init: for %x\n",inst);
	driver_user_data_init(&root,
			DEVICE_H(my_userdata),
			(sizeof(my_userdata)/sizeof(struct my_userdata)));
	return 0;
}


static int fb_start (void *inst) {
	sys_printf("fb_start: for %x\n",inst);
	return 0;
}


static struct driver_ops fb_ops = {
	fb_open,
	fb_close,
	fb_control,
	fb_init,
	fb_start,
};


static struct driver fb0 = {
	"fb0",
	&fb_data[0],
	&fb_ops,
};

int register_framebuffer(struct fb_info *fb_info) {
	int i;
	struct fb_event event;
	struct fb_videomode mode;

	if (num_registered_fb == FB_MAX)
		return -ENXIO;
	num_registered_fb++;
	for (i = 0 ; i < FB_MAX; i++)
		if (!fb_data[i].fi)
			break;

	fb_info->node = i;
	fb_info->dev =  &fb0;

	driver_publish(&fb0);

	if (fb_info->pixmap.addr == NULL) {
                fb_info->pixmap.addr = malloc(FBPIXMAPSIZE);
                if (fb_info->pixmap.addr) {
                        fb_info->pixmap.size = FBPIXMAPSIZE;
                        fb_info->pixmap.buf_align = 1;
                        fb_info->pixmap.scan_align = 1;
                        fb_info->pixmap.access_align = 32;
                        fb_info->pixmap.flags = FB_PIXMAP_DEFAULT;
                }
        }
        fb_info->pixmap.offset = 0;

        if (!fb_info->pixmap.blit_x)
                fb_info->pixmap.blit_x = ~(unsigned int)0;

        if (!fb_info->pixmap.blit_y)
                fb_info->pixmap.blit_y = ~(unsigned int)0;

        if (LIST_EMPTY(&fb_info->modelist))
                LIST_INIT(&fb_info->modelist);

        fb_var_to_videomode(&mode, &fb_info->var);
        fb_add_videomode(&mode, &fb_info->modelist);
	fb_data[i].fi = fb_info;

	return 0;
}


