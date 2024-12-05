
#define POWER_MODE_SLOW_CLK     0       // bit 0 = 0
#define POWER_MODE_FAST_CLK     1       // bit 0 = 1
#define POWER_MODE_NO_WAIT      0       // bit 1&2 = 0
#define POWER_MODE_WAIT_WFI     2       // bit 1&2 = 2
#define POWER_MODE_WAIT_WFE     4       // bit 1&2 = 4
#define POWER_MODE_DEEP_SLEEP   8       // needs external event/irq to wakeup

struct system_params {
        char *sys_console_dev;
        char *user_console_dev;
        volatile unsigned short int power_mode;
};

extern struct system_params system_params;

