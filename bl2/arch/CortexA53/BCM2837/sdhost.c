#include "sys.h"
#include "devices.h"
#include "gpio.h"

#include "sdhost.h"

#include "mmc_cmd.h"
#include "mmc_host.h"

#define BIT(a)	(1<<a)
#define HZ	100

// GPIO pins used for SDHOST.
#define GPIO_DAT3  53
#define GPIO_DAT2  52
#define GPIO_DAT1  51
#define GPIO_DAT0  50
#define GPIO_CMD   49
#define GPIO_CLK   48

#define SDCMD  0x00 /* Command to SD card              - 16 R/W */
#define SDARG  0x04 /* Argument to SD card             - 32 R/W */
#define SDTOUT 0x08 /* Start value for timeout counter - 32 R/W */
#define SDCDIV 0x0c /* Start value for clock divider   - 11 R/W */
#define SDRSP0 0x10 /* SD card response (31:0)         - 32 R   */
#define SDRSP1 0x14 /* SD card response (63:32)        - 32 R   */
#define SDRSP2 0x18 /* SD card response (95:64)        - 32 R   */
#define SDRSP3 0x1c /* SD card response (127:96)       - 32 R   */
#define SDHSTS 0x20 /* SD host status                  - 11 R/W */
#define SDVDD  0x30 /* SD card power control           -  1 R/W */
#define SDEDM  0x34 /* Emergency Debug Mode            - 13 R/W */
#define SDHCFG 0x38 /* Host configuration              -  2 R/W */
#define SDHBCT 0x3c /* Host byte count (debug)         - 32 R/W */
#define SDDATA 0x40 /* Data to/from SD card            - 32 R/W */
#define SDHBLC 0x50 /* Host block count (SDIO/SDHC)    -  9 R/W */

#define SDCMD_NEW_FLAG                  0x8000
#define SDCMD_FAIL_FLAG                 0x4000
#define SDCMD_BUSYWAIT                  0x800
#define SDCMD_NO_RESPONSE               0x400
#define SDCMD_LONG_RESPONSE             0x200
#define SDCMD_WRITE_CMD                 0x80
#define SDCMD_READ_CMD                  0x40
#define SDCMD_CMD_MASK                  0x3f

#define SDCDIV_MAX_CDIV                 0x7ff

#define SDHSTS_BUSY_IRPT                0x400
#define SDHSTS_BLOCK_IRPT               0x200
#define SDHSTS_SDIO_IRPT                0x100
#define SDHSTS_REW_TIME_OUT             0x80
#define SDHSTS_CMD_TIME_OUT             0x40
#define SDHSTS_CRC16_ERROR              0x20
#define SDHSTS_CRC7_ERROR               0x10
#define SDHSTS_FIFO_ERROR               0x08
/* Reserved */
/* Reserved */
#define SDHSTS_DATA_FLAG                0x01

#define SDHSTS_TRANSFER_ERROR_MASK      (SDHSTS_CRC7_ERROR | \
                                         SDHSTS_CRC16_ERROR | \
                                         SDHSTS_REW_TIME_OUT | \
                                         SDHSTS_FIFO_ERROR)

#define SDHSTS_ERROR_MASK               (SDHSTS_CMD_TIME_OUT | \
                                         SDHSTS_TRANSFER_ERROR_MASK)

#define SDHCFG_BUSY_IRPT_EN     BIT(10)
#define SDHCFG_BLOCK_IRPT_EN    BIT(8)
#define SDHCFG_SDIO_IRPT_EN     BIT(5)
#define SDHCFG_DATA_IRPT_EN     BIT(4)
#define SDHCFG_SLOW_CARD        BIT(3)
#define SDHCFG_WIDE_EXT_BUS     BIT(2)
#define SDHCFG_WIDE_INT_BUS     BIT(1)
#define SDHCFG_REL_CMD_LINE     BIT(0)

#define SDVDD_POWER_OFF         0
#define SDVDD_POWER_ON          1

#define SDEDM_FORCE_DATA_MODE   BIT(19)
#define SDEDM_CLOCK_PULSE       BIT(20)
#define SDEDM_BYPASS            BIT(21)

#define SDEDM_WRITE_THRESHOLD_SHIFT     9
#define SDEDM_READ_THRESHOLD_SHIFT      14
#define SDEDM_THRESHOLD_MASK            0x1f

#define SDEDM_FSM_MASK          0xf
#define SDEDM_FSM_IDENTMODE     0x0
#define SDEDM_FSM_DATAMODE      0x1
#define SDEDM_FSM_READDATA      0x2
#define SDEDM_FSM_WRITEDATA     0x3
#define SDEDM_FSM_READWAIT      0x4
#define SDEDM_FSM_READCRC       0x5
#define SDEDM_FSM_WRITECRC      0x6
#define SDEDM_FSM_WRITEWAIT1    0x7
#define SDEDM_FSM_POWERDOWN     0x8
#define SDEDM_FSM_POWERUP       0x9
#define SDEDM_FSM_WRITESTART1   0xa
#define SDEDM_FSM_WRITESTART2   0xb
#define SDEDM_FSM_GENPULSES     0xc
#define SDEDM_FSM_WRITEWAIT2    0xd
#define SDEDM_FSM_STARTPOWDOWN  0xf

#define SDDATA_FIFO_WORDS       16

#define FIFO_READ_THRESHOLD     4
#define FIFO_WRITE_THRESHOLD    4
#define SDDATA_FIFO_PIO_BURST   8

#define PIO_THRESHOLD   1  /* Maximum block count for PIO (0 = always DMA) */

extern int wait_cycles(unsigned long int t);

int is_power_of_2(unsigned int val) {
	unsigned int vdec=val-1;
	if ((vdec&val)==0) return 1;
	return 0;
}


#define SB_ATOMIC	1
#define SB_READ		2
#define SB_WRITE	4

struct scatter_buf {
	unsigned int		length;
	unsigned int		consumed;
	void			*addr;
	struct scatter_buf	*next;
};

struct host {
	struct host_command	*cmd;
	struct host_req		*mrq;
	struct host_data	*data;
	struct host_host	*mmc;
	unsigned int		clock;
	unsigned int		max_clk;
	unsigned int		hcfg;
	unsigned int		cdiv;
	unsigned long int	ns_per_fifo_word;
	unsigned int		data_complete;
	unsigned int		blocks;
	int			use_busy;
	int			use_sbc;
	int			irq_block;
	int			irq_busy;
	int			irq_data;
	struct scatter_buf	scatter_buf;
};


// Standard driver user management
struct userdata {
	struct device_handle dh;
	void	*cb_data;
	DRV_CBH	cb_handler;
	int	ev_flags;
	int	busy;
	struct  host host;
};

#define MAX_USERS 1
static struct userdata ud[MAX_USERS];

static struct userdata *get_userdata(void) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (!ud[i].busy) {
			ud[i].busy=1;
			return &ud[i];
		}
	}
	return 0;
}

static void put_userdata(struct userdata *uud) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (&ud[i]==uud) {
			uud->busy=0;
			return;
		}
	}
}

static void wakeup_users(unsigned int ev) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		unsigned int rev;
		if (ud[i].busy&&
			ud[i].cb_handler &&
			(rev=ud[i].ev_flags&ev)) {
			ud[i].ev_flags&=~ev;
			ud[i].cb_handler(&ud[i].dh,rev,ud[i].cb_data);
		}
	}
}
static void sdhost_finish_command(struct host *host);

struct SDHOST *sdhost=SDHOST_BASE;

static unsigned int min(unsigned int a, unsigned int b) {
	if (a<b) return a;
	return b;
}

static void sb_start(struct scatter_buf *sb, void *data, unsigned int len, unsigned int flags) {
	sb->addr=data;
	sb->length=len;
}

static struct scatter_buf *sb_next(struct scatter_buf *sb_in) {
	return sb_in->next;
}

static void sb_stop(struct scatter_buf *sb_in) {

}

static void sdhost_dumpcmd(struct host *host, struct host_command *cmd,
                            const char *label) {

	if (!cmd) {
		return;
	}

	sys_printf("%c%s op %d arg 0x%x flags 0x%x - resp %08x %08x %08x %08x, err %d\n",
                (cmd == host->cmd) ? '>' : ' ',
                label, cmd->opcode, cmd->arg, cmd->flags,
                cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3],
                cmd->error);
}


static void sdhost_dumpregs(struct host *host) {
        struct host_req *mrq = host->mrq;

        if (mrq) {
                sdhost_dumpcmd(host, mrq->sbc, "sbc");
                sdhost_dumpcmd(host, mrq->cmd, "cmd");
                if (mrq->data) {
                        sys_printf("data blocks %x blksz %x - err %d\n",
                                mrq->data->blocks,
                                mrq->data->blksz,
                                mrq->data->error);
                }
                sdhost_dumpcmd(host, mrq->stop, "stop");
        }

        sys_printf("=========== REGISTER DUMP ===========\n");
        sys_printf("SDCMD  0x%08x\n", sdhost->cmd);
        sys_printf("SDARG  0x%08x\n", sdhost->arg);
        sys_printf("SDTOUT 0x%08x\n", sdhost->tout);
        sys_printf("SDCDIV 0x%08x\n", sdhost->cdiv);
        sys_printf("SDRSP0 0x%08x\n", sdhost->resp[0]);
        sys_printf("SDRSP1 0x%08x\n", sdhost->resp[1]);
        sys_printf("SDRSP2 0x%08x\n", sdhost->resp[2]);
        sys_printf("SDRSP3 0x%08x\n", sdhost->resp[3]);
        sys_printf("SDHSTS 0x%08x\n", sdhost->hstat);
        sys_printf("SDVDD  0x%08x\n", sdhost->vdd);
        sys_printf("SDEDM  0x%08x\n", sdhost->edm);
        sys_printf("SDHCFG 0x%08x\n", sdhost->hcfg);
        sys_printf("SDHBCT 0x%08x\n", sdhost->hbct);
        sys_printf("SDHBLC 0x%08x\n", sdhost->hblc);
        sys_printf("===========================================\n");
}

static void sdhost_reset_internal(struct host *host) {
	unsigned int tmp;

	sdhost->vdd=SDVDD_POWER_OFF;
	sdhost->cmd=0;
	sdhost->arg=0;
	sdhost->tout=0xf00000;
	sdhost->cdiv=0;
	sdhost->hstat=0x7f9;
	sdhost->hcfg=0;
	sdhost->hbct=0;
	sdhost->hblc=0;

	tmp=sdhost->edm;
	tmp&=~((SDEDM_THRESHOLD_MASK << SDEDM_READ_THRESHOLD_SHIFT) |
		(SDEDM_THRESHOLD_MASK << SDEDM_WRITE_THRESHOLD_SHIFT));

	tmp|=(FIFO_READ_THRESHOLD << SDEDM_READ_THRESHOLD_SHIFT) |
		(FIFO_WRITE_THRESHOLD << SDEDM_WRITE_THRESHOLD_SHIFT);

	sdhost->edm=tmp;
	sys_sleep(20);
	host->clock=0;
	sdhost->hcfg=host->hcfg;
	sdhost->cdiv=host->cdiv;
}


static void sdhost_reset(struct host *host) {

	sdhost_reset_internal(host);
}

static void sdhost_wait_transfer_complete(struct host *host) {
	int timediff;
	unsigned int alternate_idle;

	alternate_idle=(host->mrq->data->flags&MMC_DATA_READ) ?
		SDEDM_FSM_READWAIT: SDEDM_FSM_WRITESTART1;

	timediff=0;

	while(1) {
		unsigned int edm,fsm;

		edm=sdhost->edm;
		fsm=edm&SDEDM_FSM_MASK;

		if ((fsm==SDEDM_FSM_IDENTMODE) ||
			(fsm == SDEDM_FSM_DATAMODE)) {
			break;
		}

		if (fsm==alternate_idle) {
			sdhost->edm=edm|SDEDM_FORCE_DATA_MODE;
			break;
		}

		timediff++;
		if (timediff==100000) {
			sys_printf("wait_transfer_complete, still waiting after %d retries\n", timediff);
			sdhost_dumpregs(host);
			host->mrq->data->error=-ETIMEDOUT;
			return;
		}
// resched ??
	}
}

static void sdhost_transfer_block_pio(struct host *host, int is_read) {
	unsigned long blksize;
	unsigned long wait_max;

	blksize=host->data->blksz;

	wait_max=tq_tic+(500/10);

	while(blksize) {
		int copy_words;
		unsigned int hsts=0;
		unsigned long len;
		unsigned int *buf;

		if (!sb_next(&host->scatter_buf)) {
			host->data->error=-EINVAL;
			break;
		}

		len=min(host->scatter_buf.length, blksize);
		if (len%4) {
			host->data->error=-EINVAL;
			break;
		}

		blksize-=len;
		host->scatter_buf.consumed=len;

		buf=(unsigned int *)host->scatter_buf.addr;

		copy_words=len/4;

		while(copy_words) {
			int burst_words, words;
			unsigned int edm;

			burst_words=min(SDDATA_FIFO_PIO_BURST, copy_words);
			edm=sdhost->edm;
			if (is_read) {
				words=((edm>>4)&0x1f);
			} else {
				words=SDDATA_FIFO_WORDS-((edm>>4)&0x1f);
			}

			if (words<burst_words) {
				int fsm_state=(edm&SDEDM_FSM_MASK);

				if ((is_read&&
				     (fsm_state!=SDEDM_FSM_READDATA&&
				      fsm_state!=SDEDM_FSM_READWAIT&&
				      fsm_state!=SDEDM_FSM_READCRC)) ||
				    (!is_read &&
				     (fsm_state!=SDEDM_FSM_WRITEDATA&&
				      fsm_state!=SDEDM_FSM_WRITESTART1&&
				      fsm_state!=SDEDM_FSM_WRITESTART2))) {
					hsts=sdhost->hstat;
					sys_printf("fsm %x, hsts %08x\n",
						fsm_state, hsts);
					if (hsts&SDHSTS_ERROR_MASK) {
						break;
					}
				}

				if ((tq_tic-wait_max)>0) {
					sys_printf("PIO %s timeout - EDM %08x\n",
					is_read?"read":"write", edm);
					hsts=SDHSTS_REW_TIME_OUT;
					break;
				}

				sys_udelay(((burst_words-words)*
						host->ns_per_fifo_word)/1000);
				continue;
			} else if (words>copy_words) {
				words=copy_words;
			}

			copy_words-=words;
			while(words) {
				if (is_read) {
					*(buf++)=sdhost->data;
				} else {
					sdhost->data=*(buf++);
				}
			}
		}
		if (hsts&SDHSTS_ERROR_MASK) {
			break;
		}
	}
	sb_stop(&host->scatter_buf);
}

static void sdhost_transfer_pio(struct host *host) {
	unsigned int sdhsts;
	int is_read;

	is_read=(host->data->flags&MMC_DATA_READ)!=0;
	sdhost_transfer_block_pio(host,is_read);

	sdhsts=sdhost->hstat;

	if (sdhsts&(SDHSTS_CRC16_ERROR|
			SDHSTS_CRC7_ERROR|
			SDHSTS_FIFO_ERROR)) {
		sys_printf("%s transfer error - HSTS %08x\n",
			is_read?"read":"write",sdhsts);
		host->data->error=-EILSEQ;
	} else if ((sdhsts&(SDHSTS_CMD_TIME_OUT|
			SDHSTS_REW_TIME_OUT))) {
		sys_printf("%s timeout error - HSTS %08x\n",
			is_read?"read":"write",sdhsts);
		host->data->error=-ETIMEDOUT;
	}
}

static void sdhost_set_transfer_irqs(struct host *host) {
	unsigned int all_irqs=SDHCFG_DATA_IRPT_EN|SDHCFG_BLOCK_IRPT_EN|
		SDHCFG_BUSY_IRPT_EN;

	host->hcfg=(host->hcfg&~all_irqs)|
		SDHCFG_DATA_IRPT_EN|
		SDHCFG_BUSY_IRPT_EN;

	sdhost->hcfg=host->hcfg;
}

static void sdhost_prepare_data(struct host *host,
				struct host_command *cmd) {
	struct host_data *data=cmd->data;

	if (host->data) {
		sys_printf("sdhost_prepare_data: host data busy\n");
	}

	host->data=data;
	if (!data) return;

	host->data_complete=0;
	host->data->bytes_xfered=0;

//	if (!host->dma_desc) {
	{
		int flags=SB_ATOMIC;

		if (data->flags&MMC_DATA_READ) {
			flags|=SB_READ;
		} else {
			flags|=SB_WRITE;
		}

		sb_start(&host->scatter_buf, data->sb, data->sb_len,flags);
		host->blocks=data->blocks;
	}

	sdhost_set_transfer_irqs(host);

	sdhost->hbct=data->blksz;
	sdhost->hblc=data->blocks;
}

static unsigned int sdhost_read_wait_sdcmd(struct host *host,
						unsigned int max_ms) {
	unsigned int value;
	int ret=0;
	int i=0;

	while(!((value=sdhost->cmd)&SDCMD_NEW_FLAG)) {
		sys_udelay(1);
		if (i>=10) {
			ret=-ETIMEDOUT;
			break;
		}
		i++;
	}

//	ret=readl_poll_timeout(&sdhost->cmd, value,
//			!(value&SDCMD_NEW_FLAG),1,10);

	if (ret==-ETIMEDOUT) {
//		ret=readl_poll_timeout(&sdhost->cmd, value,
//				!(value&SDCMD_NEW_FLAG),
//				10, max_ms*1000);
		i=0;
		while(!((value=sdhost->cmd)&SDCMD_NEW_FLAG)) {
			sys_udelay(10);
			if (i>=(max_ms*1000)) {
				ret=-ETIMEDOUT;
				break;
			}
			i++;
		}
	}

	if (ret==-ETIMEDOUT) {
		sys_printf("%s timout (%dms)\n", __func__, max_ms);
	}

	return value;
}

static void sdhost_finish_request(struct host *host) {
	struct host_req *mrq;

//	tdrv->ops->control(tdrv_dh,HR_TIMER_CANCEL,0,0);

	mrq=host->mrq;

	host->mrq=0;
	host->cmd=0;
	host->data=0;

//	request_done(host,mrq);
	wakeup_users(EV_STATE);
}

static int sdhost_send_command(struct host *host, struct host_command *cmd) {
	unsigned int sdcmd, sdhsts;
	unsigned long timeout;
	unsigned long int usec;

	if (host->cmd) {
		sys_printf("sdhost_send_command: host->cmd not free\n");
	}

	sdcmd=sdhost_read_wait_sdcmd(host,100);
	if (sdcmd&SDCMD_NEW_FLAG) {
		sys_printf("previous cmd never completed.\n");
		sdhost_dumpregs(host);
		cmd->error=-EILSEQ;
		sdhost_finish_request(host);
		return -1;
	}

#if 0
	if (!cmd->data&&cmd->busy_timeout>9000) {
		timeout=DIV_ROUND_UP(cmd->busy_timeout,1000)*HZ+HZ;
	} else {
		timeout=10*HZ;
	}
#endif

//	usec=timeout*1000000;
//	tdrv->ops->control(tdrv_dh, HR_TIMER_SET, &usec, sizeof(usec));

	host->cmd=cmd;

	sdhsts=sdhost->hstat;
	if (sdhsts&SDHSTS_ERROR_MASK) {
		sdhost->hstat=sdhsts;
	}

	if ((cmd->flags&MMC_RSP_136)&&(cmd->flags&MMC_RSP_BUSY)) {
		sys_printf("sdhost: unsupported response type!\n");
		cmd->error=-EINVAL;
		sdhost_finish_request(host);
		return -1;
	}

	sdhost_prepare_data(host,cmd);

	sdhost->arg=cmd->arg;

	sdcmd=cmd->opcode&SDCMD_CMD_MASK;

	host->use_busy=0;
	if (!(cmd->flags&MMC_RSP_PRESENT)) {
		sdcmd|=SDCMD_NO_RESPONSE;
	} else {
		if (cmd->flags&MMC_RSP_136) {
			sdcmd|=SDCMD_LONG_RESPONSE;
		}
		if (cmd->flags&MMC_RSP_BUSY) {
			sdcmd|=SDCMD_BUSYWAIT;
			host->use_busy=1;
		}
	}

	if (cmd->data) {
		if (cmd->data->flags&MMC_DATA_WRITE) {
			sdcmd|=SDCMD_WRITE_CMD;
		}
		if (cmd->data->flags&MMC_DATA_READ) {
			sdcmd|=SDCMD_READ_CMD;
		}
	}

	sdhost->cmd=sdcmd|SDCMD_NEW_FLAG;

	return 0;
}

static void sdhost_transfer_complete(struct host *host) {
	struct host_data *data;

	if (!host->data_complete) {
		sys_printf("sdhost_transfer_complete: called when not complete\n");
	}
	data=host->data;
	host->data=0;

	/* send CMD12 if -
	 *   open-ended multiblock transfer (no CMD23)
	 *   error in multiblock transfer
	 */
	if (host->mrq->stop && (data->error || !host->use_sbc)) {
		if (sdhost_send_command(host,host->mrq->stop)) {
			if (!host->use_busy) {
				sdhost_finish_command(host);
			}
		}
	} else {
		sdhost_wait_transfer_complete(host);
		sdhost_finish_request(host);
	}
}

static void sdhost_finish_data(struct host *host) {
	struct host_data *data;

	data=host->data;

	host->hcfg&=~(SDHCFG_DATA_IRPT_EN|SDHCFG_BLOCK_IRPT_EN);
	sdhost->hcfg=host->hcfg;

	data->bytes_xfered=data->error?0:(data->blksz*data->blocks);
	host->data_complete=1;

	if (host->cmd) {
		sys_printf("Finished early - HSTS %08x\n",
				sdhost->hstat);
	} else {
		sdhost_transfer_complete(host);
	}
}

static void sdhost_finish_command(struct host *host) {
	struct host_command *cmd=host->cmd;
	unsigned int sdcmd;

	sdcmd=sdhost_read_wait_sdcmd(host,100);

	/* Check for errors */
	if (sdcmd&SDCMD_NEW_FLAG) {
		sys_printf("command never completed.\n");
		sdhost_dumpregs(host);
		host->cmd->error=-EIO;
		sdhost_finish_request(host);
		return;
	} else if (sdcmd&SDCMD_FAIL_FLAG) {
		unsigned int sdhsts=sdhost->hstat;
		sdhost->hstat=SDHSTS_ERROR_MASK;

		if (!(sdhsts&SDHSTS_CRC7_ERROR)||(host->cmd->opcode!=MMC_SEND_OP_COND)) {
			unsigned int edm, fsm;

			if (sdhsts&SDHSTS_CMD_TIME_OUT) {
				host->cmd->error=-ETIMEDOUT;
			} else {
				sys_printf("unexpected command %d error\n", host->cmd->opcode);
				sdhost_dumpregs(host);
				host->cmd->error=-EILSEQ;
			}
			edm=sdhost->edm;
			fsm=edm&SDEDM_FSM_MASK;
			if (fsm==SDEDM_FSM_READWAIT ||
				fsm==SDEDM_FSM_WRITESTART1) {
				/* Kick the fsm out of its wait */
				sdhost->edm=edm|SDEDM_FORCE_DATA_MODE;
				sdhost_finish_request(host);
				return;
			}
		}
	}

	if (cmd->flags&MMC_RSP_PRESENT) {
		if (cmd->flags&MMC_RSP_136) {
			int i;

			for(i=0;i<4;i++) {
				cmd->resp[3-i]=sdhost->resp[i];
			}
		} else {
			cmd->resp[0]=sdhost->resp[0];
		}
	}

	if (cmd==host->mrq->sbc) {
		host->cmd=0;
		if (sdhost_send_command(host, host->mrq->cmd)) {
			if (!host->use_busy) {
				sdhost_finish_command(host);
			}
		}
	} else if (cmd==host->mrq->stop) {
		sdhost_finish_request(host);
	} else {
		host->cmd=0;
		if (!host->data) {
			sdhost_finish_request(host);
		} else if (host->data_complete) {
			sdhost_transfer_complete(host);
		}
	}
}

static void sdhost_timeout(void *vhost) {
	struct host *host=(struct host *)vhost;

	if (host->mrq) {
		sys_printf("timeout waiting for hardware interrupt.\n");
		sdhost_dumpregs(host);
		sdhost_reset(host);

		if (host->data) {
			host->data->error=-ETIMEDOUT;
			sdhost_finish_data(host);
		} else {
			if (host->cmd) {
				host->cmd->error=-ETIMEDOUT;
			} else {
				host->mrq->cmd->error=-ETIMEDOUT;
			}

			sdhost_finish_request(host);
		}
	}
}

static int sdhost_check_cmd_error(struct host *host, unsigned int intmask) {

	if (!(intmask&SDHSTS_ERROR_MASK)) return 0;
	if (!host->cmd) return 1;

	sys_printf("sdhost_busy_irq: intmask %08x\n",intmask);

	if (intmask&SDHSTS_CRC7_ERROR) {
		host->cmd->error=-EILSEQ;
	} else if (intmask&(SDHSTS_CRC16_ERROR | SDHSTS_FIFO_ERROR)) {
		if (host->mrq->data) {
			host->mrq->data->error=-EILSEQ;
		} else {
			host->cmd->error=-EILSEQ;
		}
	} else if (intmask&SDHSTS_REW_TIME_OUT) {
		if (host->mrq->data) {
			host->mrq->data->error=-ETIMEDOUT;
		} else {
			host->cmd->error=-ETIMEDOUT;
		}
	} else if (intmask&SDHSTS_CMD_TIME_OUT) {
		host->cmd->error=-ETIMEDOUT;
	}
	sdhost_dumpregs(host);
	return 1;
}

static int sdhost_check_data_error(struct host *host, unsigned int intmask) {
	if (!host->data) return 0;
	if (intmask&(SDHSTS_CRC16_ERROR|SDHSTS_FIFO_ERROR)) {
		host->data->error=-EILSEQ;
	}
	if (intmask&SDHSTS_REW_TIME_OUT) {
		host->data->error=-ETIMEDOUT;
	}
	return 0;
}

static int sdhost_busy_irq(struct host *host) {
	if (!host->cmd) {
		sys_printf("sdhost_busy_irq: no cmd\n");
		sdhost_dumpregs(host);
		return 0;
	}

	if (!host->use_busy) {
		sys_printf("sdhost_busy_irq: not busy\n");
		sdhost_dumpregs(host);
		return 0;
	}
	host->use_busy=0;
	sdhost_finish_command(host);
	return 0;
}

static int sdhost_data_irq(struct host *host, unsigned int intmask) {

	if (!host->data) return 0;

	sdhost_check_data_error(host,intmask);
	if (host->data->error) {
		goto finished;
	}

	if (host->data->flags&MMC_DATA_WRITE) {
		host->hcfg&=~(SDHCFG_DATA_IRPT_EN);
		host->hcfg|=SDHCFG_BLOCK_IRPT_EN;
		sdhost->hcfg=host->hcfg;
		sdhost_transfer_pio(host);
	} else {
		sdhost_transfer_pio(host);
		host->blocks--;
		if ((host->blocks==0)||host->data->error) {
			goto finished;
		}
	}
	return 0;

finished:
	host->hcfg&=~(SDHCFG_DATA_IRPT_EN|SDHCFG_BLOCK_IRPT_EN);
	sdhost->hcfg=host->hcfg;
	return 0;
}

static int sdhost_data_threaded_irq(struct host *host) {
	if (!host->data) return 0;
	if ((host->blocks==0)||host->data->error) {
		sdhost_finish_data(host);
	}
	return 0;
}

static int sdhost_block_irq(struct host *host) {
	if (!host->data) {
		sys_printf("sdhost_block_irq: Warn, no data\n");
		sdhost_dumpregs(host);
		return 0;
	}

//	if (!host->dma_desc) {
	{
		if (!host->blocks) {
			sys_printf("sdhost_block_irq: Warn: no blocks\n") ;
		}
		if (host->data->error||(--host->blocks==0)) {
			sdhost_finish_data(host);
		} else {
			sdhost_transfer_pio(host);
		}
//	} else if (host->data->flags&MMC_DATA_WRITE) {
//		sdhost_finish_data(host);
	}
	return 0;
}

static int sdhost_irq(int irq, void *dev) {
	struct host *host=dev;
	unsigned int intmask;

	intmask=sdhost->hstat;

	sdhost->hstat=SDHSTS_BUSY_IRPT|SDHSTS_BLOCK_IRPT|SDHSTS_SDIO_IRPT|SDHSTS_DATA_FLAG;

	if (intmask&SDHSTS_BLOCK_IRPT) {
		sdhost_check_data_error(host,intmask);
		host->irq_block=1;
		//IRQ_WAKE_THREAD
	}

	if (intmask&SDHSTS_BUSY_IRPT) {
		if (!sdhost_check_cmd_error(host,intmask)) {
			host->irq_busy=1;
			//IRQ_WAKE_THREAD
		} else {
			//IRQ_HANDLE
		}
	}

	if ((intmask&SDHSTS_DATA_FLAG)&&
		(host->hcfg&SDHCFG_DATA_IRPT_EN)) {
		sdhost_data_irq(host,intmask);
		host->irq_data=1;
		//IRQ_WAKE_THREAD
	}
	return 0;
}

static int sdhost_threaded_irq(int irq, void *dev) {
	struct host *host=dev;
	unsigned long flags;
	int block,busy,data;

	block=host->irq_block;
	busy =host->irq_busy;
	data =host->irq_data;

	host->irq_block=0;
	host->irq_busy=0;
	host->irq_data=0;

	if (block) sdhost_block_irq(host);
	if (busy)  sdhost_busy_irq(host);
	if (data)  sdhost_data_threaded_irq(host);

	return 0;
}

static unsigned int sdhost_set_clock(struct host *host,
					unsigned int clock,
					unsigned int caps) {
//	struct mmc_host *mmc=mmc_from_priv(host);
	int div;
	unsigned int actual_clock;

	if (clock<100000) {
		host->cdiv=SDCDIV_MAX_CDIV;
		sdhost->cdiv=host->cdiv;
		return 0;
	}

	div=host->max_clk/clock;
	if (div<2) div=2;
	if ((host->max_clk/div)>clock) div++;
	div-=2;

	if (div>SDCDIV_MAX_CDIV) div=SDCDIV_MAX_CDIV;

	clock=host->max_clk/(div+2);
	actual_clock=clock;

	host->ns_per_fifo_word=(1000000000 / clock) *
		((caps & MMC_CAP_4_BIT_DATA) ? 8 : 32);

	host->cdiv = div;
	sdhost->cdiv=host->cdiv;
	sdhost->tout=actual_clock/2;

	return actual_clock;
}

static void sdhost_request(struct host *host, struct host_req *hrq) {
	unsigned int fsm, edm;

	if (hrq->sbc) hrq->sbc->error=0;
	if (hrq->cmd) hrq->cmd->error=0;
	if (hrq->data) hrq->data->error=0;
	if (hrq->stop) hrq->stop->error=0;

	if (hrq->data && !is_power_of_2(hrq->data->blksz)) {
		sys_printf("sdhost error: unsupported block size (%d bytes)\n",hrq->data->blksz);
		if (hrq->cmd) hrq->cmd->error=-EINVAL;
//		request_done(host,hrq);
		wakeup_users(EV_STATE);
		return;
	}

	if (host->mrq) {
		sys_printf("sdhost_request: warn, overwriting host->mrq\n");
	}
	host->mrq=hrq;

	edm=sdhost->edm;
	fsm=edm&SDEDM_FSM_MASK;

	if ((fsm!=SDEDM_FSM_IDENTMODE) &&
	    (fsm!=SDEDM_FSM_DATAMODE)) {
		sys_printf("sdhost error: prev cmd (%d) not completed (EDM %08x)\n",
			sdhost->cmd&SDCMD_CMD_MASK, edm);
		sdhost_dumpregs(host);

		if (hrq->cmd) hrq->cmd->error=-EILSEQ;

		sdhost_finish_request(host);
		return;
	}

	host->use_sbc=!!hrq->sbc&&host->mrq->data &&
			(host->mrq->data->flags & MMC_DATA_READ);
	if (host->use_sbc) {
		if (sdhost_send_command(host,hrq->sbc)) {
			if (!host->use_busy) sdhost_finish_command(host);
		}
	} else if (hrq->cmd && sdhost_send_command(host,hrq->cmd)) {
		if (!host->use_busy) sdhost_finish_command(host);
	}
}

static void sdhost_set_ios(struct host *host, struct ios_conf *ios_conf) {
	if ((!ios_conf->clock)||(ios_conf->clock!=host->clock)) {
		ios_conf->actual_clock=
			sdhost_set_clock(host,
					ios_conf->clock,
					ios_conf->cap);
		host->clock=ios_conf->clock;
	}

	/*set bus width */
	host->hcfg &= ~SDHCFG_WIDE_EXT_BUS;
	if (ios_conf->bus_width==MMC_BUS_WIDTH_4) {
		host->hcfg|=SDHCFG_WIDE_EXT_BUS;
	}
	host->hcfg|=SDHCFG_WIDE_INT_BUS;

	/* Disable clever clock switching */
	host->hcfg|= SDHCFG_SLOW_CARD;
	sdhost->hcfg=host->hcfg;
}


// support funcs
static void init_gpio() {

	// gpio data0 to data3 --> pin 50-53
	gpio_set_function(GPIO_DAT0, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT1, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT2, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT3, GPIO_PIN_ALT3);
	gpio_set_pull(GPIO_DAT0, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT1, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT2, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT3, GPPUD_PULL_UP);

	// clk & cmd pin 48 and 49
	gpio_set_function(GPIO_CLK, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_CMD, GPIO_PIN_ALT3);

	gpio_set_pull(GPIO_CLK, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_CMD, GPPUD_PULL_UP);
}

// emmc driver <-> os integration

static struct device_handle *sdhost_open(void *driver_instance, DRV_CBH cb_handler, void *dum) {
	struct userdata *ud=get_userdata();
	if (!ud) return 0;
	ud->cb_handler=cb_handler;
	ud->cb_data=dum;
	return &ud->dh;
}

static int sdhost_close(struct device_handle *dh) {
	put_userdata((struct userdata *)dh);
	return 0;
}

static int sdhost_control(struct device_handle *dh,
			int cmd,
			void *arg1,
			int arg2) {
	struct userdata	*u=(struct userdata *)dh;
	struct host *host;
	if (!u) return -1;

	host=&u->host;

	switch(cmd) {
		case CONFIG_IOS: {
			struct ios_conf *ios_conf=
				(struct ios_conf *)arg1;
			if (sizeof(struct ios_conf)!=arg2) {
				return -1;
			}
			sdhost_set_ios(host,ios_conf);
			break;
		}
		case HOST_REQUEST: {
			struct host_req *host_req=
				(struct host_req *)arg1;
			if (sizeof(struct host_req)!=arg2) {
				return -1;
			}
			sdhost_request(host,host_req);
			break;
		}
	}
	return -1;
}

static int sdhost_start(void *inst) {
	return 0;
}

static int init_done=0;
static int sdhost_init(void *inst) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	if (!init_done) {
		init_done=1;
		init_gpio();

		sys_printf("sdhost_init: done\n");
	}

	return 0;
}

static struct driver_ops sdhost_drv_ops = {
	sdhost_open,
	sdhost_close,
	sdhost_control,
	sdhost_init,
	sdhost_start,
};

static struct driver sdhost_drv0 = {
	"sdhost0",
	(void *)0,
	&sdhost_drv_ops,
};

void init_sdhost() {
	driver_publish(&sdhost_drv0);
}

INIT_FUNC(init_sdhost);
