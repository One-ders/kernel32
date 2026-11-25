
#include <stdint.h>

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  1
#define FDT_END_NODE    2
#define FDT_PROP        3
#define FDT_NOP         4
#define FDT_END         9

unsigned long dtb_phys_addr = 0;
unsigned long dtb_totalsize = 0;
unsigned long initrd_phys_start = 0;
unsigned long initrd_phys_end = 0;
unsigned long phys_mem_size = 0;

static inline uint32_t fdt32(uint32_t v) {
    return __builtin_bswap32(v);
}

static inline uint64_t fdt64(uint64_t v) {
    return __builtin_bswap64(v);
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

void parse_dtb(unsigned long va_start) {
    if (!dtb_phys_addr) return;

    uint8_t *base = (uint8_t *)(va_start + dtb_phys_addr);
    uint32_t *hdr = (uint32_t *)base;

    if (fdt32(hdr[0]) != FDT_MAGIC) return;

    dtb_totalsize = fdt32(hdr[1]);
    uint32_t *p = (uint32_t *)(base + fdt32(hdr[2]));
    const char *strs = (const char *)(base + fdt32(hdr[3]));

    int in_memory = 0;

    while (1) {
        uint32_t token = fdt32(*p++);
        if (token == FDT_BEGIN_NODE) {
            const char *name = (const char *)p;
            in_memory = (name[0] == 'm' && name[1] == 'e' && name[2] == 'm' &&
                         name[3] == 'o' && name[4] == 'r' && name[5] == 'y' &&
                         (name[6] == '\0' || name[6] == '@'));
            while (*(const char *)p) p = (uint32_t *)((uintptr_t)p + 1);
            p = (uint32_t *)(((uintptr_t)p + 4) & ~3UL);
        } else if (token == FDT_END_NODE) {
            in_memory = 0;
        } else if (token == FDT_PROP) {
            uint32_t len = fdt32(*p++);
            const char *pname = strs + fdt32(*p++);
            uint8_t *val = (uint8_t *)p;

            if (streq(pname, "linux,initrd-start")) {
                initrd_phys_start = (len == 8) ? fdt64(*(uint64_t *)val) : fdt32(*(uint32_t *)val);
            } else if (streq(pname, "linux,initrd-end")) {
                initrd_phys_end = (len == 8) ? fdt64(*(uint64_t *)val) : fdt32(*(uint32_t *)val);
            } else if (in_memory && streq(pname, "reg")) {
                if (len == 16)
                    phys_mem_size = fdt64(*(uint64_t *)(val + 8));
                else if (len == 8)
                    phys_mem_size = fdt32(*(uint32_t *)(val + 4));
            }
            p = (uint32_t *)(((uintptr_t)val + len + 3) & ~3UL);
        } else if (token == FDT_NOP) {
            continue;
        } else if (token == FDT_END) {
            break;
        }
    }
}
