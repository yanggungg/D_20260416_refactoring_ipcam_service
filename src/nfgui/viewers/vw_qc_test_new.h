/*
 * vw_qc_test_new.h
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 12, 2012
 *
 */

#ifndef _VW_QC_TEST_NEW_H_
#define _VW_QC_TEST_NEW_H_

typedef void (*QC_TEST_CB_FUNC) (gpointer data);

gint vw_qc_test_create_new(NFWINDOW *parent);
gint vw_qc_test_show_new();
gint vw_qc_test_hide_new();
gboolean vw_qc_test_is_shown_new();

gint vw_qc_test_connect_func_new(gint keyIdx, gchar *label, QC_TEST_CB_FUNC cb_func, gpointer cb_param);
gint vw_qc_test_info_add_line_new(gchar *str, gint len);
gint vw_qc_test_info_update_new();
gint vw_qc_test_info_erase_new();

gint vw_get_qc_result(gchar result[][16], gint tab);

#endif


