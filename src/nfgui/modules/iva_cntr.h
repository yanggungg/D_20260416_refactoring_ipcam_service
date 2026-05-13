/*
 * iva_cntr.h
 * 	- ai analytics counter module
 *	- dependencies :
 *	
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Mar 14, 2019
 *
 */

#ifndef __IVA_CNTR_H
#define __IVA_CNTR_H



////////////////////////////////////////////////////////////
//
// public data type 
//

typedef struct _IVACR_T {
	guint chmask;
} IVACR_T;

typedef struct _CRVAL_T {
	time_t ttime;
	int cnt;
} CRVAL_T;

typedef struct _IVACR_DATA_T {
	CRVAL_T hour[24];
	CRVAL_T day[7];
} IVACR_DATA_T;


////////////////////////////////////////////////////////////
//
// public interfaces
//

int iva_cntr_init();
int iva_cntr_reset();
int iva_cntr_put_ai_dvabx_event(NF_NOTIFY_INFO *pnotify);
int iva_cntr_put_ai_builtin_event(NF_NOTIFY_INFO *pnotify);
int iva_cntr_put_classic_va_event(NF_NOTIFY_INFO *pnotify);

IVACR_T *iva_cntr_create();
int iva_cntr_destroy(IVACR_T *icr);
int iva_cntr_set_filter_ch(IVACR_T *icr, int ch, int onoff);
int iva_cntr_set_filter_chmask(IVACR_T *icr, unsigned int chmask);
int iva_cntr_get_counter_data(IVACR_T *icr, IVACR_DATA_T *data);

#endif 
