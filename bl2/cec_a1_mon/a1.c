/* $CecA1GW: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)a1.c
 */
#include <string.h>
#include "sys.h"
#include "io.h"
#include "a1_drv.h"
#include "a1.h"
#include "cec.h"

#include "asynchio.h"

static int fd_a1;
int a1_send(unsigned char *buf, int len);

#define SOURCE_TUNER 0x00
#define SOURCE_PHONO 0x01
#define SOURCE_CD    0x02
/* Undefined		0x03 */
#define SOURCE_MDTAPE 0x04
/* Undefined 		0x05 */
/* Undefined 		0x06 */
/* Undefined 		0x07 */
/* Undefined 		0x08 */
/* Undefined 		0x09 */
/* Undefined 		0x0a */
/* Undefined 		0x0b */
/* Undefined 		0x0c */
/* Undefined 		0x0d */
/* Undefined 		0x0e */
/* Undefined 		0x0f */
#define SOURCE_VIDEO1	0x10
#define SOURCE_VIDEO2	0x11
#define SOURCE_VIDEO3	0x12
/* Undefined 		0x13 */
/* Undefined 		0x14 */
/* Set TVLD		0x15 */
#define SOURCE_TVLD	0x16
/* Undefined 		0x17 */
/* Undefined 		0x18 */
#define SOURCE_DVD	0x19


#define POWER_STAT_BIT	1
#define MUTE_STAT_BIT	2



static int handle_a1_data(int fd, int ev, void *dum) {
	int rc;
	unsigned char a1_rbuf[24];
	rc=io_read(fd,(char *)a1_rbuf,sizeof(a1_rbuf));
	if (rc<0) {
		printf("got error a1 read %d\n", rc);
		return 0;
	}

	if (!(a1_rbuf[0]&0x8)) {
		DPRINTF("received a1 data, not for me!\n");
		DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
	}
	a1_rbuf[0]&=~0x8;
	if (a1_rbuf[0]==0xC0) {
		DPRINTF("a1: msg from amp: ");
		switch (a1_rbuf[1]) {
			case 0x06:
				DPRINTF("Mute\n");
				break;
			case 0x07:
				DPRINTF("Unmute\n");
				break;
			case 0x0c:
				DPRINTF("5.1 on\n");
				break;
			case 0x0d:
				DPRINTF("5.1 off\n");
				break;
			case 0x0e:
				DPRINTF("Unavailable\n");
				break;
			case 0x0f:
				DPRINTF("Error\n");
				break;
			case 0x2E:
				DPRINTF("Power on\n");
				break;
			case 0x2F:
				DPRINTF("Power off\n");
				break;
			case 0x43:
				DPRINTF("Input stat\n");
				break;
			case 0x70:
				DPRINTF("Status source AA(%02x), AV(%02x) CC(%02x)\n",
					a1_rbuf[2], a1_rbuf[3], a1_rbuf[4]);
				break;
			case 0x6A:
				DPRINTF("Device Name %s\n", &a1_rbuf[2]);
				break;
			case 0x71:
				DPRINTF("Status 2nd aud\n");
				break;
			case 0xE1:
				DPRINTF("Status\n");
				break;
			default:
				DPRINTF("Uknown (%x)\n", a1_rbuf[1]);
				DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
				break;
		}
	} else if (a1_rbuf[0]==0xC1) {
		DPRINTF("a1: msg from tuner: ");
		switch (a1_rbuf[1]) {
			case 0x40:
				DPRINTF("Station Status\n");
				break;
			case 0x70:
				DPRINTF("Tuner Status\n");
				break;
			default:
				DPRINTF("Unknown (%x)\n", a1_rbuf[1]);
				DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
				break;
		}
	} else if (a1_rbuf[0]==0xC2) {
		DPRINTF("a1: msg from uknown dev(%x)\n",0xc2);
	} else if (a1_rbuf[0]==0xC3) {
		DPRINTF("a1: msg from surround: ");
		switch(a1_rbuf[1]) {
			case 0x40:
				DPRINTF("Status R vol\n");
				break;
			case 0x50:
				DPRINTF("Status F vol\n");
				break;
			case 0x70:
				DPRINTF("Status Surround\n");
				break;
			default:
				DPRINTF("Unknown (%x)\n",a1_rbuf[1]);
				DUMP_DATA(A1_LINK, "got a1 data", a1_rbuf,rc);
				break;
		}
	} else {
		DPRINTF("a1: got msg from unknown originator (%x)\n", a1_rbuf[0]);
	}
	return 0;
}

int a1_send(unsigned char *buf, int len) {
	int rc;
	io_control(fd_a1,F_SETFL,(void *)0,0);
	rc=io_write(fd_a1,(char *)buf,len);
	io_control(fd_a1,F_SETFL,(void *)O_NONBLOCK,0);
	return rc;
}


int a1_init_a1(void) {
	fd_a1=io_open(A1_DRV0);
	io_control(fd_a1,F_SETFL,(void *)O_NONBLOCK,0);
	register_event(fd_a1,EV_READ,handle_a1_data,0);
	return fd_a1;
}
