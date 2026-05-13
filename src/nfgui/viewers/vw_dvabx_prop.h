/*
 * vw_dvabx_prop.h
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */



////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _analysis_dvabx_prop_init_page(NFOBJECT *parent);
gint _analysis_dvabx_prop_show_page(gint ch, AiAnalysisActData *analysis_data);
gint _post_analysis_dvabx_prop_show_page();
gint _analysis_dvabx_prop_hide_page(gint ch);

gint _analysis_dvabx_prop_load_changed_data(gint ch);
gint _analysis_dvabx_prop_is_changed_data();

gint _analysis_dvabx_prop_save_data();
gint _analysis_dvabx_prop_cancel_data(gint ch, gint expose);
