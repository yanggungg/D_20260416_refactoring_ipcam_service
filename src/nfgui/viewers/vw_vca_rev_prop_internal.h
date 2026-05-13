/*
 * vw_vca_rev_prop_internal.h
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
 *
 */

#ifndef	__VCA_REV_PROP_INTERNAL_H
#define __VCA_REV_PROP_INTERNAL_H

#define VCA_REV_ROCL_FIXED_W	(525)
#define VCA_REV_ROCL_FIXED_H	(756 + 60)

#define VCA_REV_BTN_W		    (145)
#define VCA_REV_BTN2_W		    (245)
#define VCA_REV_BTN_H	        (40)

#define VCA_REV_WIDTH_GAP	    (10)
#define VCA_REV_BTN_GAP		    (10)

#define VCA_REV_BTN1_H		    (VCA_REV_BTN_H + VCA_REV_BTN_GAP)
#define VCA_REV_BTN2_H		    (VCA_REV_BTN_H*2 + VCA_REV_BTN_GAP + 15)

#define VCA_REV_BTN_X		    (VCA_REV_ROCL_FIXED_W - (VCA_REV_BTN_W + VCA_REV_WIDTH_GAP))
#define VCA_REV_BTN1_X		    (VCA_REV_ROCL_FIXED_W - (VCA_REV_BTN_W + VCA_REV_WIDTH_GAP)*2)
#define VCA_REV_BTN1_Y		    (VCA_REV_ROCL_FIXED_H - (VCA_REV_BTN_H + VCA_REV_BTN_GAP))

#define VCA_REV_BTN2_X		    (VCA_REV_ROCL_FIXED_W - VCA_REV_BTN2_W - VCA_REV_WIDTH_GAP)
#define VCA_REV_BTN2_Y		    (VCA_REV_ROCL_FIXED_H - (VCA_REV_BTN_H*2 + VCA_REV_BTN_GAP*2))

#define VCA_REV_ADD_BTN_X		(VCA_REV_ROCL_FIXED_W - (VCA_REV_BTN_W + VCA_REV_WIDTH_GAP)*2)
#define VCA_REV_OK_BTN_X		(VCA_REV_ROCL_FIXED_H - (VCA_REV_BTN_W*2))


////////////////////////////////////////////////////////////
//
// public interfaces
//
gint _vca_rev_rcol_page(NFOBJECT *parent);
void _vca_rev_rocl_show(NFOBJECT *parent);
void _vca_rev_rocl_page_hide(NFOBJECT *parent);

gint _vca_rev_rule_page(NFOBJECT *parent);
void _vca_rev_rule_show(NFOBJECT *parent);
void _vca_rev_rule_hide(NFOBJECT *parent);

gint _vca_rev_rule_ready(NFOBJECT *parent);
void _vca_rev_rule_ready_show(NFOBJECT *parent);
void _vca_rev_rule_ready_hide(NFOBJECT *parent);

gint _vca_rev_rule_line(NFOBJECT *parent);
void _vca_rev_rule_line_show(NFOBJECT *parent);
void _vca_rev_rule_line_hide(NFOBJECT *parent);

gint _vca_rev_rule_area(NFOBJECT *parent);
void _vca_rev_rule_area_show(NFOBJECT *parent);
void _vca_rev_rule_area_hide(NFOBJECT *parent);

gint _vca_rev_rule_counter(NFOBJECT *parent);
void _vca_rev_rule_counter_show(NFOBJECT *parent);
void _vca_rev_rule_counter_hide(NFOBJECT *parent);

gint _vca_rev_calibration_page(NFOBJECT *parent);
void _vca_rev_calibration_show(NFOBJECT *parent);
void _vca_rev_calibration_hide(NFOBJECT *parent);

gint _vca_rev_calibration_result(NFOBJECT *parent);
void _vca_rev_calibration_result_show(NFOBJECT *parent);
void _vca_rev_calibration_result_hide(NFOBJECT *parent);

gint _vca_rev_option_page(NFOBJECT *parent);
void _vca_rev_option_show(NFOBJECT *parent);
void _vca_rev_option_hide(NFOBJECT *parent);

gint _vca_rev_live_log_page(NFOBJECT *parent);
void _vca_rev_live_log_show(NFOBJECT *parent);
void _vca_rev_live_log_hide(NFOBJECT *parent);


#endif
