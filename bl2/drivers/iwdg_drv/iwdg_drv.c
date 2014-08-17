/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)iwdg_drv.c
 */
#include "stm32/stm32f407.h"
#include "stm32/devices.h"
#include "sys.h"
#include "iwdg_drv.h"

static struct device_handle my_dh;

/*************************  Independent Watch dog driver ***************************/

static int iwdg_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	IWDG->KR=IWDG_KR_KICK;
        return 0;
}

static int iwdg_close(struct device_handle *dh) {
        return 0;
}

static struct device_handle *iwdg_open(void *instance, DRV_CBH cb_handler, void *dum) {

	IWDG->KR=IWDG_KR_START;
        return &my_dh;
}


static int iwdg_init(void *inst) {
	return 0;
};

static int iwdg_start(void *inst) {
	return 0;
}

static struct driver_ops iwdg_drv_ops = {
        iwdg_open,
        iwdg_close,
        iwdg_control,
	iwdg_init,
	iwdg_start,
};

static struct driver iwdg_drv = {
	IWDG_DRV,
	0,
	&iwdg_drv_ops,
};

void init_iwdg_drv(void) {
	driver_publish(&iwdg_drv);
}

INIT_FUNC(init_iwdg_drv);
