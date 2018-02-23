

#define IWDG_KR_KICK	0xaaaa
#define IWDG_KR_START	0xcccc
#define IWDG_KR_UNLOCK	0x5555



struct IWDG {
	unsigned int KR;    // 0x00
	unsigned int PR;    // 0x04
	unsigned int RLR;   // 0x08
	unsigned int SR;	 // 0x0c
};
