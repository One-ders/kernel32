#include <sys.h>
#include "yaffs_trace.h"
#include "yaffs_guts.h"
#include "yaffs_osglue.h"

#include <nand.h>


static int yaffs_errno;

unsigned yaffs_trace_mask =

        YAFFS_TRACE_SCAN |
        YAFFS_TRACE_GC |
        YAFFS_TRACE_ERASE |
        YAFFS_TRACE_ERROR |
        YAFFS_TRACE_TRACING |
        YAFFS_TRACE_ALLOCATE |
        YAFFS_TRACE_BAD_BLOCKS |
        YAFFS_TRACE_VERIFY |
        0;




void yaffsfs_SetError(int err) {
	yaffs_errno=err;
}

int yaffsfs_GetLastError(void) {
	return yaffs_errno;
}

void yaffsfs_Lock(void) {
}

void yaffsfs_Unlock(void) {
}

unsigned int yaffsfs_CurrentTime(void) {
	return 0;
}

void yaffs_bug_fn(const char *file_name, int line_no) {
	sys_printf("yaffs buf in %s:%d\n", file_name, line_no);
}

int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request) {
	sys_printf("yaffsfs check mem region  %x:%d\n", addr, size);
	return 0;
}

static inline char	*med3(char *, char *, char *,
    int (*)(const void *, const void *));
static inline void	 swapfunc(char *, char *, size_t, int);

#define min(a, b)	(a) < (b) ? a : b

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) { 		\
	size_t i = (n) / sizeof (TYPE); 		\
	TYPE *pi = (TYPE *)(void *)(parmi); 		\
	TYPE *pj = (TYPE *)(void *)(parmj); 		\
	do { 						\
		TYPE	t = *pi;			\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void
swapfunc(char *a, char *b, size_t n, int swaptype)
{

	if (swaptype <= 1) 
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)						\
	if (swaptype == 0) {					\
		long t = *(long *)(void *)(a);			\
		*(long *)(void *)(a) = *(long *)(void *)(b);	\
		*(long *)(void *)(b) = t;			\
	} else							\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) if ((n) > 0) swapfunc((a), (b), (size_t)(n), swaptype)

static inline char *
med3(char *a, char *b, char *c,
    int (*cmp)(const void *, const void *))
{

	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

void
qsort(void *a, size_t n, size_t es,
    int (*cmp)(const void *, const void *))
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	size_t d, r, s;
	int swaptype, cmp_result;

//	_DIAGASSERT(a != NULL || n == 0 || es == 0);
//	_DIAGASSERT(cmp != NULL);

loop:	SWAPINIT(a, es);
	if (n < 7) {
		for (pm = (char *) a + es; pm < (char *) a + n * es; pm += es)
			for (pl = pm; pl > (char *) a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = (char *) a + (n / 2) * es;
	if (n > 7) {
		pl = (char *) a;
		pn = (char *) a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap(a, pm);
	pa = pb = (char *) a + es;

	pc = pd = (char *) a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (cmp_result = cmp(pb, a)) <= 0) {
			if (cmp_result == 0) {
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (cmp_result = cmp(pc, a)) >= 0) {
			if (cmp_result == 0) {
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		pb += es;
		pc -= es;
	}

	pn = (char *) a + n * es;
	r = min(pa - (char *) a, pb - pa);
	vecswap(a, pb - r, r);
	r = min((size_t)(pd - pc), pn - pd - es);
	vecswap(pb, pn - r, r);
	/*
	 * To save stack space we sort the smaller side of the partition first
	 * using recursion and eliminate tail recursion for the larger side.
	 */
	r = pb - pa;
	s = pd - pc;
	if (r < s) {
		/* Recurse for 1st side, iterate for 2nd side. */
		if (s > es) {
			if (r > es)
				qsort(a, r / es, es, cmp);
			a = pn - s;
			n = s / es;
			goto loop;
		}
	} else {
		/* Recurse for 2nd side, iterate for 1st side. */
		if (r > es) {
			if (s > es)
				qsort(pn - s, s / es, es, cmp);
			n = r / es;
			goto loop;
		}
	}
}

//================================================================
//
//  Nand blk dev interface
//
//================================================================
//
//

static int yaffs_nand_write(struct yaffs_dev *dev, int nand_chunk,
                                   const unsigned char *data, 
					int data_len,
                                   const unsigned char *oob, 
					int oob_len) {

	sys_printf("yaffs_nand_write\n");
	return 0;
}

static int yaffs_nand_read(struct yaffs_dev *dev, int nand_chunk,
                                   unsigned char *data, 
					int data_len,
                                   unsigned char *oob, 
					int oob_len,
                                   enum yaffs_ecc_result *ecc_result_out) {

	sys_printf("yaffs_nand_read\n");
	return 0;
}

static int yaffs_nand_erase(struct yaffs_dev *dev, int block_no) {
	sys_printf("yaffs_nand_erase\n");
	return 0;
}

static int yaffs_nand_mark_bad(struct yaffs_dev *dev, int block_no) {
	sys_printf("yaffs_nand_mark_bad\n");
	return 0;
}

static int yaffs_nand_check_bad(struct yaffs_dev *dev, int block_no) {
	sys_printf("yaffs_nand_check_bad\n");
	return 0;
}

static int yaffs_nand_init(struct yaffs_dev *dev) {
	sys_printf("yaffs_nand_init\n");
	return 0;
}

static int yaffs_nand_deinit(struct yaffs_dev *dev) {
	sys_printf("yaffs_nand_deinit\n");
	return 0;
}


struct nand_context {
	void *bub;
	unsigned char *buffer;
};

static struct yaffs_dev ydev;
static struct nand_context ctxt;
static unsigned char buffer[64];

int mount_nand(char *nand_dev_name) {
	struct driver *nand_dev=driver_lookup(nand_dev_name);
	struct device_handle *dh=0;
	struct nand_config nand_cfg;
	struct yaffs_param *param;
	struct yaffs_driver *ydrv=&ydev.drv;

	memset(&ydev,0,sizeof(ydev));

	if (!nand_dev) return -1;
	dh=nand_dev->ops->open(0, NULL, 0);
	if (!dh) return -1;

	if (nand_dev->ops->control(dh,NAND_GET_CONFIG,&nand_cfg,sizeof(nand_cfg))<0) {
		nand_dev->ops->close(dh);
		return -1;
	}

	param=&ydev.param;

	param->name="bajs";
	param->total_bytes_per_chunk=nand_cfg.page_size*nand_cfg.pages_per_block;
	param->chunks_per_block=nand_cfg.pages_per_block;
	param->n_reserved_blocks=0;
	param->start_block=0;
	param->end_block=nand_cfg.n_blocks-1;
	param->is_yaffs2=1;
	param->use_nand_ecc=1;
	param->n_caches=10;
	param->stored_endian=2;


	ydrv->drv_write_chunk_fn=yaffs_nand_write;
	ydrv->drv_read_chunk_fn =yaffs_nand_read;
	ydrv->drv_erase_fn      =yaffs_nand_erase;
	ydrv->drv_mark_bad_fn	=yaffs_nand_mark_bad;
	ydrv->drv_check_bad_fn	=yaffs_nand_check_bad;
	ydrv->drv_initialise_fn	=yaffs_nand_init;
	ydrv->drv_deinitialise_fn=yaffs_nand_deinit;

	ctxt.bub = 0;
	ctxt.buffer=buffer;
	ydev.driver_context=(void *)&ctxt;
	
	yaffs_add_device(&ydev);

	return 0;
}

static struct device_handle *yaffsfs_open(void *inst, DRV_CBH cb, void *dum) {
	return 0;
}

static int yaffsfs_close(struct device_handle *hd) {
	return 0;
}

static int yaffsfs_control(struct device_handle *dh, int cmd, void *arg, int arg_len) {
	return -1;
}

static int yaffsfs_init(void *inst) {
	return 0;
}

static int yaffsfs_start(void *inst) {
	return 0;
}

static struct driver_ops yaffsfs_ops = {
	yaffsfs_open,
	yaffsfs_close,
	yaffsfs_control,
	yaffsfs_init,
	yaffsfs_start,
};

static struct driver yaffsfs_drv = {
	"yaffsfs",
	0,
	&yaffsfs_ops,
};

void init_yaffsfs(void) {
	driver_publish(&yaffsfs_drv);
}

INIT_FUNC(init_yaffsfs);
