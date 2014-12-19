
#include <sys.h>
#include <string.h>
#include <procps.h>

static struct device_handle dh;

struct userdata {
	struct device_handle dh;
	char 	name[32];
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
		}
	}
}

static int get_procs(struct dent *dents, int count) {
	struct task *p=troot;
	int i=0;

	while(p) {
		i++;
		if (i<=count) {
			strcpy(dents->name,p->name);
			dents++;
		} else {
			return 0;
		}
		p=p->next2;
	}
	return i;
}

static int get_procdata(char *name, struct procdata *pd, int size) {
	struct task *p=troot;

	while(p) {
		if (strcmp(p->name,name)==0) {
			strcpy(pd->name,name);
			pd->id=p->id;
			pd->addr=(unsigned long int)p;
			pd->sp=(unsigned long int)p->sp;
			pd->state=p->state;
			pd->prio_flags=p->prio_flags;
			pd->active_tics=p->active_tics;
			if (p->state!=1) {
				pd->pc=get_stacked_pc(p);
			} else pd->pc=0;
			return sizeof(struct procdata);
		}
		p=p->next2;
	}
	return -1;
}

static struct device_handle *procps_open(void *instance, DRV_CBH callback, void *userdata) {
	return &dh;
}

static int procps_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int procps_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	switch(cmd) {
		case RD_CHAR: {
			struct userdata *ud=(struct userdata *)dh;
			return get_procdata(ud->name,arg1,arg2);
		}
		case READDIR: {
			struct dent *dents=(struct dent *)arg1;
			return get_procs(dents,arg2);
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

static int procps_start(void *inst) {
	return 0;
}

static int procps_init(void *inst) {
	return 0;
}

static struct driver_ops procps_drv_ops = {
	procps_open,
	procps_close,
	procps_control,
	procps_init,
	procps_start,
};

static struct driver procps_drv = {
	"procps",
	0,
	&procps_drv_ops,
};

void init_procps_drv(void) {
	driver_publish(&procps_drv);
}

INIT_FUNC(init_procps_drv);
