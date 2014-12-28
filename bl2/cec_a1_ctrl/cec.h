/* $CecA1GW: cec.h, v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)cec.h
 */

#if DEBUG
#define DPRINTF(a...) {if (cec_debug) printf(a);}
#define DUMP_DATA(a,b,c,d) cec_dump_data(a,b,c,d)
int cec_dump_data(int itf, char *pretext, unsigned char *buf, int len);
#else
#define DPRINTF(a...)
#define DUMP_DATA(a,b,c,d)
#endif

extern int cec_debug;

int cec_init_cec(void);
int cec_set_rec(int enable);
int cec_send(int itf, unsigned char *b, int len);
//int cec_set_ackmask(int ackmask);

int wakeup_usb_dev(void);


#define  CEC_DEVICE_TYPE_TV                0
#define  CEC_DEVICE_TYPE_RECORDING_DEVICE  1
#define  CEC_DEVICE_TYPE_RESERVED          2
#define  CEC_DEVICE_TYPE_TUNER             3
#define  CEC_DEVICE_TYPE_PLAYBACK_DEVICE   4
#define  CEC_DEVICE_TYPE_AUDIO_SYSTEM      5

#define  CECDEVICE_UNKNOWN           -1 //not a valid logical address
#define  CECDEVICE_TV                0
#define  CECDEVICE_RECORDINGDEVICE1  1
#define  CECDEVICE_RECORDINGDEVICE2  2
#define  CECDEVICE_TUNER1            3
#define  CECDEVICE_PLAYBACKDEVICE1   4
#define  CECDEVICE_AUDIOSYSTEM       5
#define  CECDEVICE_TUNER2            6
#define  CECDEVICE_TUNER3            7
#define  CECDEVICE_PLAYBACKDEVICE2   8
#define  CECDEVICE_RECORDINGDEVICE3  9
#define  CECDEVICE_TUNER4            10
#define  CECDEVICE_PLAYBACKDEVICE3   11
#define  CECDEVICE_RESERVED1         12
#define  CECDEVICE_RESERVED2         13
#define  CECDEVICE_FREEUSE           14
#define  CECDEVICE_UNREGISTERED      15
#define  CECDEVICE_BROADCAST         15



#define  CEC_OPCODE_ACTIVE_SOURCE                 0x82
#define  CEC_OPCODE_IMAGE_VIEW_ON                 0x04
#define  CEC_OPCODE_TEXT_VIEW_ON                  0x0D
#define  CEC_OPCODE_INACTIVE_SOURCE               0x9D
#define  CEC_OPCODE_REQUEST_ACTIVE_SOURCE         0x85
#define  CEC_OPCODE_ROUTING_CHANGE                0x80
#define  CEC_OPCODE_ROUTING_INFORMATION           0x81
#define  CEC_OPCODE_SET_STREAM_PATH               0x86
#define  CEC_OPCODE_STANDBY                       0x36
#define  CEC_OPCODE_RECORD_OFF                    0x0B
#define  CEC_OPCODE_RECORD_ON                     0x09
#define  CEC_OPCODE_RECORD_STATUS                 0x0A
#define  CEC_OPCODE_RECORD_TV_SCREEN              0x0F
#define  CEC_OPCODE_CLEAR_ANALOGUE_TIMER          0x33
#define  CEC_OPCODE_CLEAR_DIGITAL_TIMER           0x99
#define  CEC_OPCODE_CLEAR_EXTERNAL_TIMER          0xA1
#define  CEC_OPCODE_SET_ANALOGUE_TIMER            0x34
#define  CEC_OPCODE_SET_DIGITAL_TIMER             0x97
#define  CEC_OPCODE_SET_EXTERNAL_TIMER            0xA2
#define  CEC_OPCODE_SET_TIMER_PROGRAM_TITLE       0x67
#define  CEC_OPCODE_TIMER_CLEARED_STATUS          0x43
#define  CEC_OPCODE_TIMER_STATUS                  0x35
#define  CEC_OPCODE_CEC_VERSION                   0x9E
#define  CEC_OPCODE_GET_CEC_VERSION               0x9F
#define  CEC_OPCODE_GIVE_PHYSICAL_ADDRESS         0x83
#define  CEC_OPCODE_GET_MENU_LANGUAGE             0x91
#define  CEC_OPCODE_REPORT_PHYSICAL_ADDRESS       0x84
#define  CEC_OPCODE_SET_MENU_LANGUAGE             0x32
#define  CEC_OPCODE_DECK_CONTROL                  0x42
#define  CEC_OPCODE_DECK_STATUS                   0x1B
#define  CEC_OPCODE_GIVE_DECK_STATUS              0x1A
#define  CEC_OPCODE_PLAY                          0x41
#define  CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS      0x08
#define  CEC_OPCODE_SELECT_ANALOGUE_SERVICE       0x92
#define  CEC_OPCODE_SELECT_DIGITAL_SERVICE        0x93
#define  CEC_OPCODE_TUNER_DEVICE_STATUS           0x07
#define  CEC_OPCODE_TUNER_STEP_DECREMENT          0x06
#define  CEC_OPCODE_TUNER_STEP_INCREMENT          0x05
#define  CEC_OPCODE_DEVICE_VENDOR_ID              0x87
#define  CEC_OPCODE_GIVE_DEVICE_VENDOR_ID         0x8C
#define  CEC_OPCODE_VENDOR_COMMAND                0x89
#define  CEC_OPCODE_VENDOR_COMMAND_WITH_ID        0xA0
#define  CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN     0x8A
#define  CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP       0x8B
#define  CEC_OPCODE_SET_OSD_STRING                0x64
#define  CEC_OPCODE_GIVE_OSD_NAME                 0x46
#define  CEC_OPCODE_SET_OSD_NAME                  0x47
#define  CEC_OPCODE_MENU_REQUEST                  0x8D
#define  CEC_OPCODE_MENU_STATUS                   0x8E
#define  CEC_OPCODE_USER_CONTROL_PRESSED          0x44
#define  CEC_OPCODE_USER_CONTROL_RELEASE          0x45
#define  CEC_OPCODE_GIVE_DEVICE_POWER_STATUS      0x8F
#define  CEC_OPCODE_REPORT_POWER_STATUS           0x90
#define  CEC_OPCODE_FEATURE_ABORT                 0x00
#define  CEC_OPCODE_ABORT                         0xFF
#define  CEC_OPCODE_GIVE_AUDIO_STATUS             0x71
#define  CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS 0x7D
#define  CEC_OPCODE_REPORT_AUDIO_STATUS           0x7A
#define  CEC_OPCODE_SET_SYSTEM_AUDIO_MODE         0x72
#define  CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST     0x70
#define  CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS      0x7E
#define  CEC_OPCODE_SET_AUDIO_RATE                0x9A

enum CecVendorId {
    Toshiba      = 0x000039,
    Samsung      = 0x0000F0,
    Denon        = 0x0005CD,
    Marantz      = 0x000678,
    Loewe        = 0x000982,
    Onkyo        = 0x0009B0,
    Medion       = 0x000CB8,
    Toshiba2     = 0x000CE7,
    PulseEight   = 0x001582,
    Akai         = 0x0020C7,
    AOC          = 0x002467,
    Panasonic    = 0x008045,
    Philips      = 0x00903E,
    Daewoo       = 0x009053,
    Yamaha       = 0x00A0DE,
    Grundig      = 0x00D0D5,
    Pioneer      = 0x00E036,
    LG           = 0x00E091,
    Sharp        = 0x08001F,
    Sony         = 0x080046,
    Broadcom     = 0x18C086,
    Vizio        = 0x6B746D,
    Benq         = 0x8065E9,
    HarmanKardon = 0x9C645E,
    Unknown      = 0
};




typedef int (*CEC_CALLBACK)(unsigned char *buf, int len);

#define CEC_BUS 1
#define A1_LINK 2
#define USB_BUS 3

int cec_attach(int itf, unsigned int la, CEC_CALLBACK);
int cec_detach(unsigned int la);
int cec_send_physical_address(int itf, unsigned int addr, unsigned short paddr, int function);
int cec_send_osd_name(int itf, unsigned int addr,unsigned char *idbuf);
int cec_send_system_audio_mode_status(int itf, unsigned int addr, int on);
int cec_send_system_audio_mode_set(int itf, unsigned int addr, int on);
int cec_send_vendor_id(int itf, unsigned int addr,unsigned int vendor_id);
int cec_send_power_status(int itf, unsigned int addr, int on);
int cec_send_active_source(int itf, unsigned int cec_addr,unsigned short paddr);
int cec_send_standby(int itf, unsigned int addr);
int cec_send_image_view_on(int itf, unsigned int addr);
int cec_send_cec_version(int itf, unsigned int addr, unsigned char ver);
int cec_send_abort(int itf, unsigned int addr,unsigned char opcode, unsigned char reason);

