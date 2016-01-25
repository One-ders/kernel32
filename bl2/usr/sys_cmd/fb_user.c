
#include <string.h>
#include "sys.h"
#include "io.h"
#include <fb.h>

int fb_test(void *dum) {
	int fb_fd;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	unsigned int screensize;
	char *fbp;
	int x=0,y=0;
	int location=0;
	
	fb_fd=io_open("fb0");

	if (fb_fd<0) {
		printf("could not open fb\n");
		while(1) {
			sleep(100);
		}
		return 0;
	}

	printf("fb opened fd=%d\n",fb_fd);

	if (io_control(fb_fd, FBIOGET_FSCREENINFO, &finfo,sizeof(finfo)) < 0) {
		printf("could not get fscreeninfo\n");
		while(1) {
			sleep(100);
		}
	}

	printf("fix_info: id=%s\n", finfo.id);
        printf("fix_info: smem_start=%x\n",finfo.smem_start);
        printf("fix_info: smem_len=%d\n",finfo.smem_len);
        printf("fix_info: type=%x\n",finfo.type);
        printf("fix_info: line_length=%d\n",finfo.line_length);
        printf("fix_info: mmio_start=%x\n",finfo.mmio_start);
        printf("fix_info: mmio_len=%d\n",finfo.mmio_len);

	if (io_control(fb_fd,FBIOGET_VSCREENINFO, &vinfo,sizeof(vinfo)) < 0) {
		printf("could not get vscreeninfo\n");
		while(1) {
			sleep(100);
		}
	}

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	screensize = vinfo.xres_virtual * vinfo.yres_virtual * (vinfo.bits_per_pixel / 8);

	x = 0; y = 0;
	for (y = 0; y < 272; y++) {
		for (x = 0; x < 480; x++) {
			char out[4];
			location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
				(y+vinfo.yoffset) * finfo.line_length;

			printf("output location: %d\n", location);
			out[0]=100;
			io_write(fb_fd,out, 1);
			out[0]=15+(x-0)/2;
			io_write(fb_fd,out, 1);
			out[0]=200-(y-0)/5;
			io_write(fb_fd,out, 1);
			out[0]=0;
			io_write(fb_fd,out, 1);
		}
	}

	while(1) {
		sleep(100);
	}
	return 0;
}
