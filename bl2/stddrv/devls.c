
#include <sys.h>
#include <string.h>
#include <devls.h>
#include <frame.h>

static struct device_handle dh;

struct userdata {
	struct	device_handle dh;
	char 	name[32];
	int	busy;
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
		}
	}
}

static int get_devs(struct dent *dents, int count) {
	struct driver *d=drv_root;
	int i=0;

	while(d) {
		i++;
		if (i<=count) {
			strcpy(dents->name,d->name);
			dents++;
		} else {
			return 0;
		}
		d=d->next;
	}
	return i;
}

static int get_devdata(char *name, struct devdata *dd, int size) {
	struct driver *d=drv_root;
	struct task *t=troot;
	int i=0;

	while(d) {
		if (strcmp(d->name,name)==0) {
			strcpy(dd->name,name);
			while(t) {
				struct user_fd *ofd=t->fd_list;
				while(ofd) {
					if (ofd->driver==d) {
						dd->pid[i]=t->id;
						i++;
						if (i>=16) return -1;
					}
					ofd=ofd->next;
				}
				t=t->next2;
			}
		}
		d=d->next;
	}
	dd->numofpids=i;
	return sizeof(*dd);
}

static struct device_handle *devls_open(void *instance, DRV_CBH callback, void *userdata) {
	return &dh;
}

static int devls_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int devls_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	switch(cmd) {
		case RD_CHAR: {
			struct userdata *ud=(struct userdata *)dh;
			return get_devdata(ud->name,arg1,arg2);
		}
		case READDIR: {
			struct dent *dents=(struct dent *)arg1;
			return get_devs(dents,arg2);
		}
		case DYNOPEN: {
			struct dyn_open_args *doa=(struct dyn_open_args *)arg1;
			struct userdata *ud=get_userdata();
			if (!ud) return -1;
			strcpy(ud->name,doa->name);
			doa->dh=(struct device_handle *)ud;
			return 0;
		}
		default: {
			return -1;
		}
	}
	return -1;
}

static int devls_start(void *inst) {
	return 0;
}

static int devls_init(void *inst) {
	return 0;
}

static struct driver_ops devls_drv_ops = {
	devls_open,
	devls_close,
	devls_control,
	devls_init,
	devls_start,
};

static struct driver devls_drv = {
	"dev",
	0,
	&devls_drv_ops,
};

void init_devls_drv(void) {
	driver_publish(&devls_drv);
}

INIT_FUNC(init_devls_drv);
