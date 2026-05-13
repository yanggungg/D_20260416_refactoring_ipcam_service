/*
 * vw_dvabx_prop_internal.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#ifndef	__DVABX_PROP_INTERNAL_H
#define __DVABX_PROP_INTERNAL_H

#define DVABX_ROCL_FIXED_W	    (525)
#define DVABX_ROCL_FIXED_H	    (756 + 60)

#define DVABX_BTN_W		        (145)
#define DVABX_BTN2_W		    (245)
#define DVABX_BTN_H	            (40)

#define DVABX_WIDTH_GAP	        (10)
#define DVABX_BTN_GAP		    (10)

#define DVABX_BTN1_H		    (DVABX_BTN_H + DVABX_BTN_GAP)
#define DVABX_BTN2_H		    (DVABX_BTN_H*2 + DVABX_BTN_GAP + 15)

#define DVABX_BTN_X		        (DVABX_ROCL_FIXED_W - (DVABX_BTN_W + DVABX_WIDTH_GAP))
#define DVABX_BTN1_X		    (DVABX_ROCL_FIXED_W - (DVABX_BTN_W + DVABX_WIDTH_GAP)*2)
#define DVABX_BTN1_Y		    (DVABX_ROCL_FIXED_H - (DVABX_BTN_H + DVABX_BTN_GAP))

#define DVABX_BTN2_X		    (DVABX_ROCL_FIXED_W - DVABX_BTN2_W - DVABX_WIDTH_GAP)
#define DVABX_BTN2_Y		    (DVABX_ROCL_FIXED_H - (DVABX_BTN_H*2 + DVABX_BTN_GAP*2))

#define DVABX_ADD_BTN_X		    (DVABX_ROCL_FIXED_W - (DVABX_BTN_W + DVABX_WIDTH_GAP)*2)
#define DVABX_OK_BTN_X			(DVABX_ROCL_FIXED_H - (DVABX_BTN_W*2))


////////////////////////////////////////////////////////////
//
// public interfaces
//
gint _dvabx_rcol_page(NFOBJECT *parent);
void _dvabx_rocl_show(NFOBJECT *parent);
void _dvabx_rocl_page_hide(NFOBJECT *parent);

gint _dvabx_rule_page(NFOBJECT *parent);
void _dvabx_rule_show(NFOBJECT *parent);
void _dvabx_rule_hide(NFOBJECT *parent);

gint _dvabx_rule_ready(NFOBJECT *parent);
void _dvabx_rule_ready_show(NFOBJECT *parent);
void _dvabx_rule_ready_hide(NFOBJECT *parent);

gint _dvabx_rule_line(NFOBJECT *parent);
void _dvabx_rule_line_show(NFOBJECT *parent);
void _dvabx_rule_line_hide(NFOBJECT *parent);

gint _dvabx_rule_area(NFOBJECT *parent);
void _dvabx_rule_area_show(NFOBJECT *parent);
void _dvabx_rule_area_hide(NFOBJECT *parent);

gint _dvabx_rule_counter(NFOBJECT *parent);
void _dvabx_rule_counter_show(NFOBJECT *parent);
void _dvabx_rule_counter_hide(NFOBJECT *parent);

gint _dvabx_calibration_page(NFOBJECT *parent);
void _dvabx_calibration_show(NFOBJECT *parent);
void _dvabx_calibration_hide(NFOBJECT *parent);

gint _dvabx_calibration_result(NFOBJECT *parent);
void _dvabx_calibration_result_show(NFOBJECT *parent);
void _dvabx_calibration_result_hide(NFOBJECT *parent);

gint _dvabx_option_page(NFOBJECT *parent);
void _dvabx_option_show(NFOBJECT *parent);
void _dvabx_option_hide(NFOBJECT *parent);

gint _dvabx_live_log_page(NFOBJECT *parent);
void _dvabx_live_log_show(NFOBJECT *parent);
void _dvabx_live_log_hide(NFOBJECT *parent);


#endif
