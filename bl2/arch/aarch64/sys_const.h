#include <page.h>

#define VA_START                        0xffff000000000000

#define PHYS_MEMORY_SIZE                0x40000000UL

#if 0
#define PAGE_MASK                       0xfffffffffffff000
#define PAGE_SHIFT                      12
#define TABLE_SHIFT                     9
#define SECTION_SHIFT                   (PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE                       (1 << PAGE_SHIFT)
#define SECTION_SIZE                    (1 << SECTION_SHIFT)
#endif

#define LOW_MEMORY                      (2 * SECTION_SIZE)
#define HIGH_MEMORY                     DEVICE_BASE

#define PAGING_MEMORY                   (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES                    (PAGING_MEMORY/PAGE_SIZE)
#define PTRS_PER_TABLE                  (1 << TABLE_SHIFT)

#define PGD_SHIFT                       PAGE_SHIFT + 3*TABLE_SHIFT
#define PUD_SHIFT                       PAGE_SHIFT + 2*TABLE_SHIFT
#define PMD_SHIFT                       PAGE_SHIFT + TABLE_SHIFT

#define PG_DIR_SIZE                     (3 * PAGE_SIZE)

#define KV2P(a) (((unsigned long int)a)&0x7fffffffL)

#define PT_TABLE        0x3UL
#define PT_PAGE         0x3UL
#define PT_NG           (1ULL << 11)
#define PT_AF           (1UL << 10)          /* Access Flag (must be 1 or hardware faults) */
#define PT_SH_INNER     (3UL << 8)           /* Inner Shareable (required for SMP/caches) */
#define PT_ATTR_NORMAL_NC  (1UL << 2)        /* MAIR index 1 = Normal WB non Cacheable RAM */
#define PT_ATTR_NORMAL  (2UL << 2)           /* MAIR index 2 = Normal WB Cacheable RAM */

/* Access Permissions (AP[2:1] at bits [7:6]) */
#define PT_AP_RW_EL1    (0UL << 6)           /* EL1 RW, EL0 No Access */
#define PT_AP_RW_EL0    (1UL << 6)           /* EL1 RW, EL0 RW */
#define PT_AP_RO_EL1    (2UL << 6)           /* EL1 RO, EL0 No Access */
#define PT_AP_RO_EL0    (3UL << 6)           /* EL1 RO, EL0 RO */

/* Execute Never bits */
#define PT_PXN          (1UL << 53)          /* Privileged Execute Never (always set on user pages!) */
#define PT_UXN          (1UL << 54)          /* Unprivileged (EL0) Execute Never */

/* Convenience combinations for user segments */
#define USER_PAGE_RW    (PT_PAGE | PT_AF | PT_SH_INNER | PT_ATTR_NORMAL | PT_AP_RW_EL0 | PT_PXN | PT_UXN)
#define USER_PAGE_RWX    (PT_PAGE | PT_AF | PT_ATTR_NORMAL | PT_AP_RW_EL0 | PT_PXN | PT_NG)
#define USER_PAGE_RX    (PT_PAGE | PT_AF | PT_SH_INNER | PT_ATTR_NORMAL | PT_AP_RO_EL0 | PT_PXN)
#define USER_PAGE_RO    (PT_PAGE | PT_AF | PT_SH_INNER | PT_ATTR_NORMAL | PT_AP_RO_EL0 | PT_PXN | PT_UXN)


#define MM_TYPE_PAGE_TABLE              0x3
#define MM_TYPE_PAGE                    0x3
#define MM_TYPE_BLOCK                   0x1
#define MM_ACCESS                       (0x1 << 10)
#define MM_ACCESS_PERMISSION            (0x01 << 6)

/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]
 *                      n       MAIR
 *   DEVICE_nGnRnE      000     00000000
 *   NORMAL_NC          001     01000100
 *   NORMAL             010     11111111
 */
#define MT_DEVICE_nGnRnE                0x0
#define MT_NORMAL_NC                    0x1
#define MT_NORMAL			0x2
#define MT_DEVICE_nGnRnE_FLAGS          0x00
#define MT_NORMAL_NC_FLAGS              0x44
#define MT_NORMAL_FLAGS			0xff
#define MAIR_VALUE                      ((MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | \
                                         (MT_NORMAL_NC_FLAGS     << (8 * MT_NORMAL_NC))     | \
                                         (MT_NORMAL_FLAGS        << (8 * MT_NORMAL)))

#define MMU_FLAGS                       (MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS)
#define MMU_DEVICE_FLAGS                (MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)
#define MMU_PTE_FLAGS                   (MM_TYPE_PAGE | (MT_NORMAL_NC << 2) | MM_ACCESS | MM_ACCESS_PERMISSION)



#define TCR_T0SZ                        (64 - 48)
#define TCR_T1SZ                        ((64 - 48) << 16)
#define TCR_TG0_4K                      (0 << 14)
#define TCR_TG1_4K                      (2 << 30)
#define TCR_VALUE                       (TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)

// ***************************************
// SCTLR_EL1, System Control Register (EL1), Page 2025 of AArch64-Reference-Manual.
// ***************************************

#define SCTLR_RESERVED             (3 << 28) | (3 << 22) | (1 << 20) | (1 < < 11)
#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)
#define SCTLR_I_CACHE_DISABLED          (0 << 12)
#define SCTLR_D_CACHE_DISABLED          (0 << 2)
#define SCTLR_MMU_DISABLED              (0 << 0)
#define SCTLR_MMU_ENABLED               (1 << 0)
#define SCTLR_D_CACHE_ENABLED           (1 << 2)
#define SCTLR_I_CACHE_ENABLED		(1 << 12)

#define SCTLR_VALUE_MMU_DISABLED	(SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// ***************************************
// ESR_EL1, Exception Syndrome Register (EL1). Page 1899 of AArch64-Reference-Manual.
// ***************************************

#define ESR_ELx_EC_SHIFT                26
#define ESR_ELx_EC_SVC64                0x15
#define ESR_ELx_EC_DABT_LOW             0x24


#define DEVICE_BASE             0x3F000000
#define PBASE                   DEVICE_BASE

#define S_FRAME_SIZE			288 	// size of all saved registers

#define SYNC_INVALID_EL1t               0
#define IRQ_INVALID_EL1t                1
#define FIQ_INVALID_EL1t                2
#define ERROR_INVALID_EL1t              3

#define SYNC_INVALID_EL1h               4
#define FIQ_INVALID_EL1h                5
#define ERROR_INVALID_EL1h              6

#define FIQ_INVALID_EL0_64              7
#define ERROR_INVALID_EL0_64            8

#define SYNC_INVALID_EL0_32             9
#define IRQ_INVALID_EL0_32              10
#define FIQ_INVALID_EL0_32              11
#define ERROR_INVALID_EL0_32            12

#define SYNC_ERROR                      13
#define SYSCALL_ERROR                   14
#define DATA_ABORT_ERROR                15


