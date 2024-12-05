

#define DR_OE	(1<<11) // Overrun in rx fifo
#define DR_BE	(1<<10) // Break error
#define DR_PE	(1<<9)  // Parity error
#define DR_FE	(1<<8)  // Framing error

#define RSRECR_OE (1<<3) // Overrun error
#define RSRECR_BE (1<<2) // Break error
#define RSRECR_PE (1<<1) // Parity error
#define RSRECR_FE (1<<0) // Framing error

#define FR_RI	(1<<8)	// N/A
#define FR_TXFE	(1<<7)	// Transmit fifo empty
#define FR_RXFF (1<<6)	// Recevie fifo full
#define FR_TXFF (1<<5)	// Transmit fifo full
#define FR_RXFE (1<<4)	// Receive fifo empty
#define FR_BUSY (1<<3)	// Uart is busy
#define FR_DCD	(1<<2)	// N/A
#define FR_DSR	(1<<1)	// N/A
#define FR_CTS  (1<<0)	// ~CTS

#define LCRH_SPS	(1<<7)
// a	bits
// 0	5
// 1	6
// 2	7
// 3	8
#define LCRH_WLEN(a)	(a<<5)
#define LCRH_FEN	(1<<4)	// fifo enable
#define LCRH_STP2	(1<<3)	// two stop bits tx mode
#define LCRH_EPS	(1<<2)	// even parity
#define LCRH_PEN	(1<<1)	// parity enable
#define LCRH_BRK	(1<<0)	// send break

#define CR_CTSEN	(1<<15) // CTS hw flow ctrl
#define CR_RTSEN	(1<<14)	// RTS hw flow ctr
#define CR_RTS		(1<<11)	// ~request to send
#define CR_RXE		(1<<9)	// Receive enable
#define CR_TXE		(1<<8)	// Transmit enable
#define CR_LBE		(1<<7)	// Loopback enable
#define CR_UARTEN	(1<<0)

// a        tx/rx irq at fifo lev
// 0		1/8
// 1		1/4
// 2		1/2
// 3		3/4
// 4		7/8
#define IFLS_RXIFLSEL(a)		(a<<3)
#define IFLS_TXIFLSEL(a)		(a<<0)


#define IMSC_OEIM	(1<<10) // mask overrun error
#define IMSC_BEIM	(1<<9)	// mask break error
#define IMSC_PEIM	(1<<8)	// mask parity error
#define IMSC_FEIM	(1<<7)	// mask framing error
#define IMSC_RTIM	(1<<6)	// mask receive timeout
#define IMSC_TXIM	(1<<5)	// mask tx irq
#define IMSC_RXIM	(1<<4)	// mask rx irq
#define IMSC_CTSMIM	(1<<1)	// mask CTS

#define RIS_OERIS	(1<<10) // Overrun irq stat
#define RIS_BERIS	(1<<9)	// Break error irq stat
#define RIS_PERIS	(1<<8)	// Parity error irq stat
#define RIS_FERIS	(1<<7)	// Framing error irq stat
#define RIS_RTRIS	(1<<6)	// Rx timeout irq stat
#define RIS_TXRIS	(1<<5)	// Tx irq stat
#define RIS_RXRIS	(1<<4)	// Rx irq stat
#define RIS_CTSRMIS	(1<<1)	// CTS irq stat

#define MIS_OEMIS	(1<<10)	// Overrun masked irq stat
#define MIS_BEMIS	(1<<9)	// Break err masked irq stat
#define MIS_PEMIS	(1<<8)	// Parity err masked irq stat
#define MIS_FEMIS	(1<<7)	// Framing error masked irq stat
#define MIS_RTMIS	(1<<6)	// Rx timeout masked irq stat
#define MIS_TXMIS	(1<<5)	// Tx masked irq stat
#define MIS_RXMIS	(1<<4)	// Rx masked irq stat
#define MIS_CTSMMIS	(1<<1)	// CTS masked irq stat

#define ICR_OEIC	(1<<10)	// Overrun irq clear
#define ICR_BEIC	(1<<9)	// Break irq clear
#define ICR_PEIC	(1<<8)	// Parity irq clear
#define ICR_FEIC	(1<<7)	// Framing irq clear
#define ICR_RTIC	(1<<6)	// rx timeout irq clear
#define ICR_TXIC	(1<<5)	// Tx irq clear
#define ICR_RXIC	(1<<4)	// Rx irq clear
#define ICR_CTSMIC	(1<<1)	// CTS irq clear

#define ITCR_ITCR1	(1<<1)	// Test fifo enable
#define ITCR_ITCR0	(1<<0)	// Integration test enable

#define ITIP_ITIP3	(1<<3)	// read cts line bit
#define ITIP_ITIP0	(1<<0)	// read rx data in bit

#define ITOP_ITOP11	(1<<11)	// intra chip output
#define ITOP_ITOP10	(1<<10)	// intra chip output
#define ITOP_ITOP9 	(1<<9)	// intra chip output
#define ITOP_ITOP8 	(1<<8)	// intra chip output
#define ITOP_ITOP7 	(1<<7)	// intra chip output
#define ITOP_ITOP6 	(1<<6)	// intra chip output

#define ITOP_ITOP3 	(1<<3)	// primary output RTS
#define ITOP_ITOP0 	(1<<0)	// primary output TXD

#define TDR_TDR(a)		(a&0x3ff) // read/write fifo when ITCR_ITCR1

struct UART_DEV {
	unsigned int	dr;		// 0x0
	unsigned int	rsrecr;		// 0x4
	unsigned int	pad0[4];	// 0x8-0x14
	unsigned int	fr;		// 0x18
	unsigned int	ilrp;		// 0x20
	unsigned int	ibrd;		// 0x24
	unsigned int 	fbrd;		// 0x28
	unsigned int	lcrh;		// 0x2c
	unsigned int	cr;		// 0x30
	unsigned int	ifls;		// 0x34
	unsigned int	imsc;		// 0x38
	unsigned int	ris;		// 0x3c
	unsigned int	mis;		// 0x40
	unsigned int	icr;		// 0x44
	unsigned int	dmacr;		// 0x48
	unsigned int	pad1[13];	// 0x4c-0x7c
	unsigned int	itcr;		// 0x80
	unsigned int	itip;		// 0x84
	unsigned int	itop;		// 0x88
	unsigned int	tdr;		// 0x8c
};
