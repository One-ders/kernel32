
#include <types.h>
#include <page.h>

#define VM_READ         0x00000001      /* currently active flags */
#define VM_WRITE        0x00000002
#define VM_EXEC         0x00000004
#define VM_SHARED       0x00000008

#define VM_EXECUTABLE   0x00001000
#define VM_LOCKED       0x00002000
#define VM_IO           0x00004000      /* Memory mapped I/O or similar */

struct vm_area_struct {
	unsigned long vm_start;
	unsigned long vm_end;
	struct vm_area_struct *prev,*next;
	unsigned long vm_pgoff;
	unsigned long vm_flags;
	unsigned int  vm_page_prot;
};

size_t copy_to_user(void *uptr, const void *src, size_t n);
size_t copy_from_user(void *dst, const void *uptr, size_t n);

struct task;

unsigned long int get_mmap_vaddr(struct task *current, unsigned int size);
int mapmem(struct task *t, unsigned long int vaddr, unsigned long int paddr, unsigned int attr);
int unmapmem(struct task *t, unsigned long int vaddr);

unsigned int virt_to_phys(void *v);
