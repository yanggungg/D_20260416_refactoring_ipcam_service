/*
 * vw_vca_rev_prop.h
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
 *
 */



////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _analysis_classic_prop_init_page(NFOBJECT *parent);
gint _analysis_classic_prop_show_page(gint ch, AiAnalysisActData *analysis_data);
gint _analysis_classic_prop_hide_page(gint ch);

gint _analysis_classic_prop_load_changed_data(gint ch);
gint _analysis_classic_prop_is_changed_data();

gint _analysis_classic_prop_save_data();
gint _analysis_classic_prop_cancel_data(gint ch, gint expose);
