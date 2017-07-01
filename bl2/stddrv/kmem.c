
#include <sys.h>
#include <string.h>
#include <kmem.h>

struct userdata {
	struct device_handle dh;
	unsigned long int fpos;
	int busy;
};

static struct userdata ud[16];

static struct userdata *get_userdata(void) {
	int i;
	for(i=0;i<16;i++) {
		if (!ud[i].busy) {
			ud[i].busy=1;
			return &ud[i];
		}
	}
	return 0;
}

static void put_userdata(struct userdata *uud) {
	int i;
	for(i=0;i<16;i++) {
		if (&ud[i]==uud) {
			uud->busy=0;
			return;
		}
	}
}

static struct device_handle *kmem_open(void *instance, DRV_CBH callback, void *userdata) {
	struct userdata *ud=get_userdata();
	if (!ud) return 0;
	ud->fpos=0;
	
	return (struct device_handle *)ud;
}

static int kmem_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int kmem_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	struct userdata *udh=(struct userdata *)dh;
	switch(cmd) {
		case RD_CHAR: {
			memcpy(arg1,(void *)udh->fpos,arg2);
			udh->fpos+=arg2;
			return arg2;
		}
		case WR_CHAR: {
			sys_printf("kmem wr: %08x <- %08x\n",
					udh->fpos, *(unsigned long int *)arg1);
			memcpy((void *)udh->fpos,arg1,arg2);
			udh->fpos+=arg2;
			return arg2;
		}
		case IO_LSEEK: {
			switch(arg2) {
				case 0:
					udh->fpos=(unsigned long int)arg1;
					break;
				case 1:
					udh->fpos+=(unsigned long int)arg1;
					break;
				default:
					return -1;
			}
			return udh->fpos;
		}
		default: {
			return -1;
		}
	}
	return -1;
}

static int kmem_start(void *inst) {
	return 0;
}

static int kmem_init(void *inst) {
	return 0;
}

static struct driver_ops kmem_drv_ops = {
	kmem_open,
	kmem_close,
	kmem_control,
	kmem_init,
	kmem_start,
};

static struct driver kmem_drv = {
	"kmem",
	0,
	&kmem_drv_ops,
};

void init_kmem_drv(void) {
	driver_publish(&kmem_drv);
}

INIT_FUNC(init_kmem_drv);
