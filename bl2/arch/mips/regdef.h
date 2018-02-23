/*
 * Symbolic register names for 32 bit ABI
 */
#define zero    $0      /* wired zero */
#define AT      $1      /* assembler temp  - uppercase because of ".set at" */
#define v0      $2      /* return value */
#define v1      $3
#define a0      $4      /* argument registers */
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8      /* caller saved */
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16     /* callee saved */
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24     /* caller saved */
#define t9      $25
#define jp      $25     /* PIC jump register */
#define k0      $26     /* kernel scratch */
#define k1      $27
#define gp      $28     /* global pointer */
#define sp      $29     /* stack pointer */
#define fp      $30     /* frame pointer */
#define s8      $30     /* same like fp! */
#define ra      $31     /* return address */

/*
 *  * Coprocessor 0 register names
 *   */
#define CP0_INDEX $0
#define CP0_RANDOM $1
#define CP0_ENTRYLO0 $2
#define CP0_ENTRYLO1 $3
#define CP0_CONF $3
#define CP0_CONTEXT $4
#define CP0_PAGEMASK $5
#define CP0_WIRED $6
#define CP0_INFO $7
#define CP0_BADVADDR $8
#define CP0_COUNT $9
#define CP0_ENTRYHI $10
#define CP0_COMPARE $11
#define CP0_STATUS $12
#define CP0_CAUSE $13
#define CP0_EPC $14
#define CP0_PRID $15
#define CP0_CONFIG $16
#define CP0_LLADDR $17
#define CP0_WATCHLO $18
#define CP0_WATCHHI $19
#define CP0_XCONTEXT $20
#define CP0_FRAMEMASK $21
#define CP0_DIAGNOSTIC $22
#define CP0_PERFORMANCE $25
#define CP0_ECC $26
#define CP0_CACHEERR $27
#define CP0_TAGLO $28
#define CP0_TAGHI $29
#define CP0_ERROREPC $30

#define ST0_IE                  0x00000001
#define ST0_EXL                 0x00000002
#define ST0_ERL                 0x00000004
#define ST0_KSU                 0x00000018
#  define KSU_USER              0x00000010
#  define KSU_SUPERVISOR        0x00000008
#  define KSU_KERNEL            0x00000000
#define ST0_UX                  0x00000020
#define ST0_SX                  0x00000040
#define ST0_KX                  0x00000080
#define ST0_DE                  0x00010000
#define ST0_CE                  0x00020000

#define ST0_CU0			0x10000000

#define CP0_CAUSE_BD		0x80000000
#define CP0_CAUSE_TI		0x40000000
#define CP0_CAUSE_CE		0x10000000
#define CP0_CAUSE_DC		0x08000000
#define CP0_CAUSE_PCI		0x04000000
#define CP0_CAUSE_IV 		0x00800000
#define CP0_CAUSE_IP_SHIFT	8
#define CP0_CAUSE_IP_MASK	0x0000ff00
#define CP0_CAUSE_IP7		0x00008000
#define CP0_CAUSE_IP6		0x00004000
#define CP0_CAUSE_IP5		0x00002000
#define CP0_CAUSE_IP4		0x00001000
#define CP0_CAUSE_IP3		0x00000800
#define CP0_CAUSE_IP2		0x00000400
#define CP0_CAUSE_IP1		0x00000200
#define CP0_CAUSE_IP0		0x00000100
#define CP0_CAUSE_EC_SHIFT	2
#define CP0_CAUSE_EC_MASK 	0x0000007c

