#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mycore/sys.h"
#include "mycore/io.h"
#include <mycore/fb.h>

static struct fb_fix_screeninfo finfo;
static struct fb_var_screeninfo vinfo;
static unsigned char *fbp;

static int clearScreen() {
	int x=0,y=0;
	int location=0;
        x = 0; y = 0;
	for (y = 0; y < 272; y++) {
		for (x = 0; x < 480; x++) {
			location = 
			(x+0) * (vinfo.bits_per_pixel/8) +
			(y+0) * finfo.line_length;

			fbp[location]=0;
			fbp[location+1]=0;
			fbp[location+2]=0;
			fbp[location+3]=0;
                }
        }

	return 0;
}

int main(int argc, char **argv) {
	int opt;
	int i=0;
	int cs=0;
	char *fname;

	int fb_fd;
	int fd;
	unsigned int screensize;

	setlinebuf(stdout);
#if 0
	printf("in main, got %d args, address of argv is %x\n", argc, argv);
	
	while(i<argc) {
		printf("%d: %s\n", i, argv[i]);
		i++;
	}
#endif

	while((opt=getopt(argc,argv, "c"))!=-1) {
		switch(opt) {
		case 'c':
			cs=1;
			break;
		default:
			fflush(stdout);
			printf("bad opt %x\n",opt);
			exit(-1);
		}
	}

	if ((argc-optind)>1) {
		printf("usage: fb [-c] [fname]\n");
		fflush(stdout);
		exit(-1);
	}

	printf("argc is %d, optind is %d\n", argc,optind);
	fname=argv[optind];

	if (fname) {
		printf("filename is %s\n",fname);
	}

	fb_fd=io_open("fb0");

	if (fb_fd<0) {
		printf("could not open fb\n");
		exit(1);
	}

	if (io_control(fb_fd, FBIOGET_FSCREENINFO, &finfo,sizeof(finfo)) < 0) {
		printf("could not get fscreeninfo\n");
		exit(1);
	}

	if (io_control(fb_fd,FBIOGET_VSCREENINFO, &vinfo,sizeof(vinfo)) < 0) {
		printf("could not get vscreeninfo\n");
		exit(1);
	}

	printf("xoffset = %d, yoffset = %d\n", vinfo.xoffset, vinfo.yoffset);

	screensize = vinfo.xres_virtual * vinfo.yres_virtual * (vinfo.bits_per_pixel / 8);

	fbp=(char *)io_mmap(0,screensize, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);

	if (!fbp) {
		printf("could not get vscreeninfo\n");
		exit(1);
	}


	if (cs) {
		clearScreen();
	}

	if (fname) {
		int offs=0;
		fd=open(fname,O_RDONLY);
		if (fd<0) {
			printf("could not open file: %s\n", fname);
			goto out;
		}

		while(1) {
			int rc=read(fd,fbp+offs,4096);
//			printf("read %d bytes\n", rc);
			if (rc!=4096) {
				break;
			}
			offs+=4096;
		}
	}

out:
	io_munmap(fbp,screensize);
	return 0;
}
