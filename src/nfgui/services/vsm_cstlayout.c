/*
 * vsm_cstlayout.c
 *        - dependency :
 *
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Apr 9, 2017
 *
 */

#include "nf_afx.h"

#include "../support/color.h"
#include "../support/util.h"
#include "modules/ssm.h"

#include "cmm.h"
#include "scm.h"

#include "ix_mem.h"

#include "vsm.h"
#include "vsm_internal.h"



////////////////////////////////////////////////////////////
//
// private variable
//

static DispCstlayout_t g_disp_cstlayout = {0, };
static guint g_delay_fulsh_tmr = 0;


////////////////////////////////////////////////////////////
//
// private interfaces
//

static VSM_ID_E _substitute_win_id_for_latter(gchar latter)
{
    gchar c_arr[33] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"};
    gint i;
    VSM_ID_E win_id = -1;

    for (i = 0; i < strlen(c_arr); i++)
    {
        if (latter == c_arr[i]) {
            win_id = i;
	    break;
        }
    }
    
    return win_id;
}

static gchar _substitute_latter_for_win_id(VSM_ID_E win_id)
{
    gchar c_arr[32] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"};
    
    return c_arr[win_id];
}

static gint _load_element_data(DispCstlayout_t *cstlayout)
{
	gint i = 0, j;
	gchar tmp[STRING_SIZE_32+1];
	GValue ret_value = {0,};

	g_message("%s, %d", __FUNCTION__, __LINE__);

// VSM_DIV1
	if (nf_sysdb_get_key0("disp.cstlayout.div1.DCNT", &ret_value, NULL))
	{
		cstlayout->div1_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div1_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div1.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div1_items[i].div = VSM_DIV1;
					cstlayout->div1_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}

// VSM_DIV4
	if (nf_sysdb_get_key0("disp.cstlayout.div4.DCNT", &ret_value, NULL))
	{
		cstlayout->div4_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div4_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div4.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div4_items[i].div = VSM_DIV4;
					cstlayout->div4_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}

// VSM_DIV9
	if (nf_sysdb_get_key0("disp.cstlayout.div9.DCNT", &ret_value, NULL))
	{
		cstlayout->div9_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div9_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div9.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div9_items[i].div = VSM_DIV9;
					cstlayout->div9_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}

// VSM_DIV16
	if (nf_sysdb_get_key0("disp.cstlayout.div16.DCNT", &ret_value, NULL))
	{
		cstlayout->div16_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div16_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div16.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div16_items[i].div = VSM_DIV16;
					cstlayout->div16_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}

// VSM_DIV36
#if 0
	if (nf_sysdb_get_key0("disp.cstlayout.div32.DCNT", &ret_value, NULL))
	{
		cstlayout->div36_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div36_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div16.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 16; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'P') {
					cstlayout->div36_items[i].div = VSM_DIV36;
					cstlayout->div36_items[i].conf[j] = tmp[j]-'A';
				}
			}
		}
	}
#else
    cstlayout->div36_item_cnt = 1;
    cstlayout->div36_items[0].div = VSM_DIV36;
    for (i = 0; i < 32; i++) {
        cstlayout->div36_items[0].conf[i] = i;
    }
    
#endif
// VSM_DIV6
	if (nf_sysdb_get_key0("disp.cstlayout.div6.DCNT", &ret_value, NULL))
	{
		cstlayout->div6_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div6_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div6.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div6_items[i].div = VSM_DIV6;
					cstlayout->div6_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}	

// VSM_DIV8
	if (nf_sysdb_get_key0("disp.cstlayout.div8.DCNT", &ret_value, NULL))
	{
		cstlayout->div8_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for (i = 0; i < cstlayout->div8_item_cnt; i++)
	{
		if (nf_sysdb_get_key1("disp.cstlayout.div8.D%d.ch", i, &ret_value, NULL))
		{
			memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
			g_stpcpy(tmp, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			for (j = 0; j < 32; j++)
			{
				if (!tmp[j]) break;

				if (tmp[j] >= 'A' && tmp[j] <= 'f') {
					cstlayout->div8_items[i].div = VSM_DIV8;
					cstlayout->div8_items[i].conf[j] = _substitute_win_id_for_latter(tmp[j]);
				}
			}
		}
	}	

	g_message("%s, %d", __FUNCTION__, __LINE__);

	return 0;
}

static gint _save_element_data(DispCstlayout_t *cstlayout)
{
	gint i = 0, j;
	gchar tmp[STRING_SIZE_32+1];
	GValue set_value = {0,};

	g_message("%s, %d", __FUNCTION__, __LINE__);

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

// VSM_DIV1
	for (i = 0; i < cstlayout->div1_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div1_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div1_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] =_substitute_latter_for_win_id(cstlayout->div1_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div1.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}

// VSM_DIV4
	for (i = 0; i < cstlayout->div4_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div4_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div4_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] = _substitute_latter_for_win_id(cstlayout->div4_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div4.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}

// VSM_DIV9
	for (i = 0; i < cstlayout->div9_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div9_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div9_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] = _substitute_latter_for_win_id(cstlayout->div9_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div9.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}	

// VSM_DIV16
	for (i = 0; i < cstlayout->div16_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div16_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div16_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] = _substitute_latter_for_win_id(cstlayout->div16_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div16.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}		

// VSM_DIV6
	for (i = 0; i < cstlayout->div6_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div6_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div6_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] = _substitute_latter_for_win_id(cstlayout->div6_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div6.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}		

// VSM_DIV8
	for (i = 0; i < cstlayout->div8_item_cnt; i++)
	{
		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
		for (j = 0; j < 32; j++)
		{
			if (cstlayout->div8_items[i].conf[j] >= VSM_WIN_ID1 && cstlayout->div8_items[i].conf[j] < VSM_WIN_ID36)
			{
				tmp[j] = _substitute_latter_for_win_id(cstlayout->div8_items[i].conf[j]);
			}
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, tmp);
		nf_sysdb_set_key1("disp.cstlayout.div8.D%d.ch", i, &set_value, NULL);
		g_value_unset(&set_value);
	}				

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	g_message("%s, %d", __FUNCTION__, __LINE__);

	return 0;
}

static gboolean _delay_flush_cstlayout(gpointer data)
{
	DispCstlayout_t tmp_cstlayout;

	g_message("%s, %d", __FUNCTION__, __LINE__);

	memset(&tmp_cstlayout, 0x00, sizeof(DispCstlayout_t));
	_load_element_data(&tmp_cstlayout);

	if (memcmp(&g_disp_cstlayout, &tmp_cstlayout, sizeof(DispCstlayout_t)) != 0) {
		_save_element_data(&g_disp_cstlayout);
		nf_sysdb_save("disp");
//		scm_put_log(CHANGE_DISP_LAYOUT, 0, 0);
	}

	g_delay_fulsh_tmr = 0;
	return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _vsm_init_cstlayout()
{
	g_message("%s, %d", __FUNCTION__, __LINE__);

	memset(&g_disp_cstlayout, 0x00, sizeof(DispCstlayout_t));
	_load_element_data(&g_disp_cstlayout);
	return 0;
}

gint _vsm_save_cstlayout()
{
	g_message("%s, %d", __FUNCTION__, __LINE__);

	if (g_delay_fulsh_tmr) {
		g_source_remove(g_delay_fulsh_tmr);
		g_delay_fulsh_tmr = 0;
	}

	g_delay_fulsh_tmr = g_timeout_add(60000, _delay_flush_cstlayout, 0);
	return 0;
}

gint _vsm_get_cstlayout_itemcnt(VSM_DIV_E dtype)
{
	gint item_cnt = 0;

	if (dtype == VSM_DIV1) item_cnt = g_disp_cstlayout.div1_item_cnt;
	if (dtype == VSM_DIV4) item_cnt = g_disp_cstlayout.div4_item_cnt;
	if (dtype == VSM_DIV9) item_cnt = g_disp_cstlayout.div9_item_cnt;
	if (dtype == VSM_DIV16) item_cnt = g_disp_cstlayout.div16_item_cnt;
	if (dtype == VSM_DIV36) item_cnt = g_disp_cstlayout.div36_item_cnt;
	if (dtype == VSM_DIV6) item_cnt = g_disp_cstlayout.div6_item_cnt;
	if (dtype == VSM_DIV8) item_cnt = g_disp_cstlayout.div8_item_cnt;

	g_message("%s, %d, cnt:%d", __FUNCTION__, __LINE__, item_cnt);

	return item_cnt;
}

DispClayoutElement_t *_vsm_get_cstlayout_element(VSM_DIV_E dtype, gint cst_idx)
{
	DispClayoutElement_t *elm;

	g_message("%s, %d, idx:%d, cnt:%d", __FUNCTION__, __LINE__, cst_idx, _vsm_get_cstlayout_itemcnt(dtype));

	if (cst_idx >= _vsm_get_cstlayout_itemcnt(dtype)) iassert(0);

	if (dtype == VSM_DIV1) elm = &g_disp_cstlayout.div1_items[cst_idx];
	if (dtype == VSM_DIV4) elm = &g_disp_cstlayout.div4_items[cst_idx];
	if (dtype == VSM_DIV9) elm = &g_disp_cstlayout.div9_items[cst_idx];
	if (dtype == VSM_DIV16) elm = &g_disp_cstlayout.div16_items[cst_idx];
	if (dtype == VSM_DIV36) elm = &g_disp_cstlayout.div36_items[cst_idx];
	if (dtype == VSM_DIV6) elm = &g_disp_cstlayout.div6_items[cst_idx];
	if (dtype == VSM_DIV8) elm = &g_disp_cstlayout.div8_items[cst_idx];

	g_message("%s, %d, elm:%p", __FUNCTION__, __LINE__, elm);
	g_message("%s, %d, elm_div:%d", __FUNCTION__, __LINE__, elm->div);

	return elm;
}

gint _vsm_set_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx)
{
	if (cst_idx >= _vsm_get_cstlayout_itemcnt(dtype)) cst_idx = 0;
	g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, cst_idx);

	return cst_idx;
}

gint _vsm_increase_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx)
{
	if (++cst_idx >= _vsm_get_cstlayout_itemcnt(dtype)) cst_idx = 0;
	g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, cst_idx);

	return cst_idx;
}

gint _vsm_decrease_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx)
{
	if (--cst_idx < 0) cst_idx = _vsm_get_cstlayout_itemcnt(dtype) - 1;
	g_message("%s, %d, idx:%d", __FUNCTION__, __LINE__, cst_idx);

	return cst_idx;
}

gint _vsm_get_sfc_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm)
{
	gchar win;

#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

	// g_message("%s, %d, elm:%p", __FUNCTION__, __LINE__, elm);
	// g_message("%s, %d, elm_div:%d", __FUNCTION__, __LINE__, elm->div);	

	for (win=0; win<nr_channel[elm->div]; win++)
	{
		// g_message("%s, %d, win:%d, ch:%d", __FUNCTION__, __LINE__, win, elm->conf[win]);
		psfc->cinfo[elm->conf[win]].win_id = win;
	}

	psfc->div = elm->div;

	return 0;
}

gint _vsm_set_sfc_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm)
{
	gint i;
	gchar win_id = -1;

	//g_message("%s, %d, elm:%p", __FUNCTION__, __LINE__, elm);
	if (!elm) return -1;
	//g_message("%s, %d, psfc_div:%d, elm_div:%d", __FUNCTION__, __LINE__, psfc->div, elm->div);
	if (psfc->div != elm->div) return -1;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		win_id = psfc->cinfo[i].win_id;
		if (win_id != -1) elm->conf[win_id] = i;
	}

	return 0;
}

gint _vsm_is_matched_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm)
{
	gint i;
	gchar win_id = -1;

	//g_message("%s, %d, elm:%p", __FUNCTION__, __LINE__, elm);
	if (!elm) return 0;
	//g_message("%s, %d, psfc_div:%d, elm_div:%d", __FUNCTION__, __LINE__, psfc->div, elm->div);
	if (psfc->div != elm->div) return 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		win_id = psfc->cinfo[i].win_id;
		if (win_id != -1) {
			if (elm->conf[win_id] != i) return 0;
		}
	}

	return 1;
}

gint _vsm_is_channel_check_cstlayout(DispClayoutElement_t *elm, gint check_ch)
{
	CameraData camdata;
	gchar win;
	gint ch;

#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

	//g_message("%s, %d, elm:%p", __FUNCTION__, __LINE__, elm);
	if (!elm) return 0;
	//g_message("%s, %d, elm_div:%d", __FUNCTION__, __LINE__, elm->div);	

	for (win=0; win<nr_channel[elm->div]; win++)
	{
		ch = elm->conf[win];
		if (check_ch == ch) return 1;
	}

	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vsm_is_changed_cstlayout()
{
	DispCstlayout_t tmp_cstlayout;

	g_message("%s, %d", __FUNCTION__, __LINE__);

	memset(&tmp_cstlayout, 0x00, sizeof(DispCstlayout_t));
	_load_element_data(&tmp_cstlayout);

	if (memcmp(&g_disp_cstlayout, &tmp_cstlayout, sizeof(DispCstlayout_t)) == 0) return 0;

	return 1;
}

gint vsm_load_cstlayout()
{
	DispCstlayout_t tmp_cstlayout;

	g_message("%s, %d", __FUNCTION__, __LINE__);

	if (g_delay_fulsh_tmr) {
		g_source_remove(g_delay_fulsh_tmr);
		g_delay_fulsh_tmr = 0;
	}

	memset(&tmp_cstlayout, 0x00, sizeof(DispCstlayout_t));
	_load_element_data(&tmp_cstlayout);

	//if (memcmp(&g_disp_cstlayout, &tmp_cstlayout, sizeof(DispCstlayout_t)) == 0) return 0;

	memcpy(&g_disp_cstlayout, &tmp_cstlayout, sizeof(DispCstlayout_t));
	vsm_change_sfc_cstlayout_load(VSM_DEFAULT_DIV);

	return 0;
}
