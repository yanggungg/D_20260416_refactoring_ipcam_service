/*
 * scm_led.c
 * 	- front led services
 *	- dependency :
 *
 * Written by JungKyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Aug 5, 2011
 *
 */

#include "scm.h"
#include "iux_afx.h"
#include "nf_api_ipcam.h"
#include "scm_internal.h"
#include "ix_mem.h"

#include "nf_util_device.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"SCM_LED"

#define POWER_LED               37
#define	REC_LED					38
#define	NET_LED					39
#define ALARM_LED       		40

static gint power_led_status = 0;
static gint rec_led_status = 0;
static gint net_led_status = 0;
static gint alarm_led_status = 0;




////////////////////////////////////////////////////////////
//
// private functions
//

static int _turn_on_rec_led()
{
	if (rec_led_status)	return -1;

	DMSG(1, "record led turn on\n");
	nf_dev_keypad_led_on(REC_LED);
	rec_led_status = 1;
	return 0;
}

static int _turn_off_rec_led()
{
	if (!rec_led_status)	return -1;

	DMSG(1, "record led turn off\n");
	nf_dev_keypad_led_off(REC_LED);
	rec_led_status = 0;
	return 0;
}

static int _turn_on_net_led()
{
	if (net_led_status)	return -1;

	DMSG(1, "network led turn on\n");
	nf_dev_keypad_led_on(NET_LED);
	net_led_status = 1;
	return 0;
}

static int _turn_off_net_led()
{
	if (!net_led_status)	return -1;

	DMSG(1, "network led turn off\n");
	nf_dev_keypad_led_off(NET_LED);
	net_led_status = 0;
	return 0;
}

static int _turn_on_alarm_led()
{
	if (alarm_led_status)	return -1;

	DMSG(1, "alarm led turn on\n");
	nf_dev_keypad_led_on(ALARM_LED);
	alarm_led_status = 1;	
	return 0;
}

static int _turn_off_alarm_led()
{
	if (!alarm_led_status)	return -1;

	DMSG(1, "alarm led turn off\n");
	nf_dev_keypad_led_off(ALARM_LED);
	alarm_led_status = 0;		
	return 0;
}

static int _turn_on_power_led()
{
	if (power_led_status)	return -1;

	DMSG(1, "alarm led turn on\n");
	nf_dev_keypad_led_on(POWER_LED);
	power_led_status = 1;	
	return 0;
}

static int _turn_off_power_led()
{
	if (!power_led_status)	return -1;

	DMSG(1, "alarm led turn off\n");
	nf_dev_keypad_led_off(POWER_LED);
	power_led_status = 0;		
	return 0;
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_update_record_led(NF_NOTIFY_INFO *pnotify)
{
	int ch, rec_check = 0;
	int max_ch = var_get_ch_count();
	
	if(pnotify) {
		for (ch = 0; ch < max_ch; ch++)
		{
			if ((pnotify->c.chmap[ch] != ' ') && (pnotify->c.chmap[ch] != 'p'))
				rec_check++;	
		}

		if (rec_check)
			_turn_on_rec_led();
		else
			_turn_off_rec_led();
	}

	return 0;
}

int _scm_update_network_led(NF_NOTIFY_INFO *pnotify)
{
	if(pnotify) {
		if (pnotify->d.params[0]) 
			_turn_on_net_led();
		else 
			_turn_off_net_led();		
	}

	return 0;
}


int _scm_update_alarm_led(NF_NOTIFY_INFO *pnotify)
{
	if(pnotify) {
		if (pnotify->d.params[0] || pnotify->d.params[1]) 
			_turn_on_alarm_led();
		else 
			_turn_off_alarm_led();		
	}

	return 0;
}




////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_turn_on_led_all()
{
	_turn_on_rec_led();
	_turn_on_alarm_led();
	_turn_on_net_led();
	return 0;
}

int scm_turn_off_led_all()
{
	_turn_off_rec_led();
	_turn_off_alarm_led();
	_turn_off_net_led();
	return 0;
}

int scm_turn_on_led_all_qc()
{
    nf_dev_keypad_led_all_on();
    return 0;
}

int scm_turn_off_led_all_qc()
{
    nf_dev_keypad_led_all_off();
    return 0;
}

