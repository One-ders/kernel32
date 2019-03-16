
struct UART {
       union {
                volatile unsigned char urbr;
                volatile unsigned char uthr;
                volatile unsigned char udllr;
                volatile unsigned int  fill1;
        };
        union {
                volatile unsigned char uier;
                volatile unsigned char udlhr;
                volatile unsigned int  fill2;
        };
        union {
                volatile unsigned char uiir;
                volatile unsigned char ufcr;
                volatile unsigned int  fill3;
        };
        union {
                volatile unsigned char ulcr;
                volatile unsigned int  fill4;
        };
        union {
                volatile unsigned char umcr;
                volatile unsigned int  fill5;
        };
        union {
                volatile unsigned char ulsr;
                volatile unsigned int  fill6;
        };
        union {
                volatile unsigned char umsr;
                volatile unsigned int  fill7;
        };
        union {
                volatile unsigned char uspr;
                volatile unsigned int  fill8;
        };
        union {
                volatile unsigned char isr;
                volatile unsigned int  fill9;
        };
};
