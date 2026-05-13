/*
 * vw_qc_test.h
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 12, 2012
 *
 */

#ifndef _VW_QC_TEST_H_
#define _VW_QC_TEST_H_

typedef void (*QC_TEST_CB_FUNC) (gpointer data);

gint vw_qc_test_create(NFWINDOW *parent);
gint vw_qc_test_show();
gint vw_qc_test_hide();
gboolean vw_qc_test_is_shown();

gint vw_qc_test_connect_func(gint keyIdx, gchar *label, QC_TEST_CB_FUNC cb_func, gpointer cb_param);
gint vw_qc_test_info_add_line(gchar *str, gint len);
gint vw_qc_test_info_update();
gint vw_qc_test_info_erase();

#endif

