
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

int dump_tlb(void);
int flush_tlb(void);

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
		case KMEM_GET_MEMINFO: {
			struct meminfo *meminfo=(struct meminfo *)arg1;
			meminfo->totalRam=SDRAM_SIZE;
			meminfo->freePages=count_free_pages();
			meminfo->sharedPages=count_shared_pages();
			return 0;
		}
		case KMEM_GET_FIRST_PSMAP: {
			struct ps_memmap *ps_memmap=(struct ps_memmap *)arg1;
			struct task *t=lookup_task_for_name(ps_memmap->ps_name);
			int i;
			ps_memmap->sh_data=0;
			if (!t) return -1;
			unsigned long int *pgd=(unsigned long int *)t->asp->pgd;
			for(i=0;i<1024;i++) {
				unsigned long int *pt;
				if (pt=(unsigned long int *)pgd[i]) {
					int j;
					for(j=0;j<1024;j++) {
						unsigned long int pte;
						if (pte=pt[j]) {
							unsigned int vaddr;
							vaddr=(i<<22)|(j<<12);
							ps_memmap->vaddr=vaddr;
							if (pte&0x80000000) {
								int pte_flags=pte&0x3f;
								int shared_ref;
								ps_memmap->sh_data=(pte&0x7fffffff)>>6;
								shared_ref=get_pfn_sh(ps_memmap->sh_data);
								ps_memmap->sh_data|=((shared_ref&0x3f)<<24);
								ps_memmap->pte=(shared_ref&~0x3f) | pte_flags;
							} else {
								ps_memmap->pte=pte;
							}
							return 0;
						}
					}
				}
			}
			return -1;
		}
		case KMEM_GET_NEXT_PSMAP: {
			struct ps_memmap *ps_memmap=(struct ps_memmap *)arg1;
			struct task *t=lookup_task_for_name(ps_memmap->ps_name);
			int i;
			int start_pgd_index=ps_memmap->vaddr>>22;
			int start_pt_index=((ps_memmap->vaddr>>12)&0x3ff)+1;
			ps_memmap->sh_data=0;
			if (!t) return -1;
			unsigned long int *pgd=(unsigned long int *)t->asp->pgd;
			for(i=start_pgd_index;i<1024;i++) {
				unsigned long int *pt;
				if (pt=(unsigned long int *)pgd[i]) {
					int j;
					for(j=start_pt_index;j<1024;j++) {
						unsigned long int pte;
						if (pte=pt[j]) {
							unsigned int vaddr;
							vaddr=(i<<22)|(j<<12);
							ps_memmap->vaddr=vaddr;
							if (pte&0x80000000) {
								int pte_flags=pte&0x3f;
								int shared_ref;
								ps_memmap->sh_data=(pte&0x7fffffff)>>6;
								shared_ref=get_pfn_sh(ps_memmap->sh_data);
								ps_memmap->sh_data|=((shared_ref&0x3f)<<24);
								ps_memmap->pte=(shared_ref&~0x3f) | pte_flags;
							} else {
								ps_memmap->pte=pte;
							}
							return 0;
						}
					}
					start_pt_index=0;
				}
			}
			return -1;
		}
		case KMEM_DUMP_TLB: {
			dump_tlb();
			return 0;
		}
		case KMEM_FLUSH_TLB: {
			flush_tlb();
			return 0;
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
