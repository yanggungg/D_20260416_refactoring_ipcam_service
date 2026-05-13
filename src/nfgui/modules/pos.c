/*
 * pos.c
 *  - POS(ATM) monitor module
 *  - dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Sep 4, 2014
 *
 */

#include "pos.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "var.h"
#include "support/color.h"

#include "nf_api_eventlog.h"
#include "nf_api_pos_eventlog.h"


#define MAX_PAGE                    (65)
#define MAX_ROW                     (64)

#define MAX_HIGHLIGHT_CNT           (4)
#define MAX_EXCLUDE_CNT             (4)

#define POS_LOCK()		g_mutex_lock(ipos.mtx)
#define POS_UNLOCK()	g_mutex_unlock(ipos.mtx)



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _PL_TBL {
    int             cnt;
    NF_LOG_DATA     *plog;
} PL_TBL;

typedef struct _PD_TBL {
    int             cnt;
    POS_DATA_T      *pdata;
} PD_TBL;

typedef struct _HIGHLIGHT_CONF {
    int             use;
    int             color[MAX_HIGHLIGHT_CNT];          // highlight color index
    int             text[MAX_HIGHLIGHT_CNT][32];    
} HIGHLIGHT_CONF;

typedef struct _EXCLUDE_CONF {
    int             use;
    int             text[MAX_EXCLUDE_CNT][32];    
} EXCLUDE_CONF;

typedef struct _PPROP {
    int             enable;
    BITMASK         mode;           // 0 : none, 1 : live, 2 : playback, 3 : live & playback 
    int             align;          // 0 : left, 1 : right
    int             font_type;      // 0 : small, 1 : medium, 2 : large
    int             font_color;     // normal color index
    int             dwell_time;     // 0 : until next, ect : x sec
    int             scroll_type;    // 0 : clear, 1 : roll up
    HIGHLIGHT_CONF  highlight;
    EXCLUDE_CONF    exclude;    
} PPROP;

typedef struct _PPAGE {
    int             init;
    BITMASK         chmask;
    GList           *loglist;
} PPAGE;

typedef struct _POS_T {
    PPROP          prop;
    PPAGE          page[MAX_PAGE];
    GMutex		   *mtx;
} POS_T;





////////////////////////////////////////////////////////////
//
// private variable
//

static POS_T ipos;
static guint64 g_put_livelogid = 0;
static guint64 g_put_playbacklogid = 0;
static guint64 g_put_testid = 0;
        

////////////////////////////////////////////////////////////
//
// private functions
//

static gint _prvRgbToColorIdx(guint rgb_col)
{
    gint idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF);

    switch(rgb_col)
    {
        case 0xffffff: idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF); break;
        case 0x808080: idx = COLOR_PRG_IDX(UX_COLOR_808080); break;
        case 0xffff00: idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case 0x0000ff: idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case 0x00ff00: idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case 0xff0000: idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;        
        default: break;
    }

    return idx;
}

static int _load_property()
{
    PosOsdData posdb;

    DAL_get_pososd_data(&posdb);

    ipos.prop.enable = 1;
    ipos.prop.mode = posdb.disp_mode;
    ipos.prop.align = posdb.position;
    ipos.prop.font_type = posdb.font;
    ipos.prop.font_color = _prvRgbToColorIdx(posdb.normal_color);
    ipos.prop.dwell_time = posdb.duration;   
    ipos.prop.scroll_type = posdb.scroll;

    ipos.prop.highlight.use = posdb.highlight;
    ipos.prop.highlight.color[0] = _prvRgbToColorIdx(posdb.highlight_color[0]);
    strcpy(ipos.prop.highlight.text[0], posdb.highlight_text[0]);    
    ipos.prop.highlight.color[1] = _prvRgbToColorIdx(posdb.highlight_color[1]);
    strcpy(ipos.prop.highlight.text[1], posdb.highlight_text[1]);    
    ipos.prop.highlight.color[2] = _prvRgbToColorIdx(posdb.highlight_color[2]);
    strcpy(ipos.prop.highlight.text[2], posdb.highlight_text[2]);    
    ipos.prop.highlight.color[3] = _prvRgbToColorIdx(posdb.highlight_color[3]);
    strcpy(ipos.prop.highlight.text[3], posdb.highlight_text[3]);    

    ipos.prop.exclude.use = posdb.exclude;
    strcpy(ipos.prop.exclude.text[0], posdb.exclude_text[0]);    
    strcpy(ipos.prop.exclude.text[1], posdb.exclude_text[1]);    
    strcpy(ipos.prop.exclude.text[2], posdb.exclude_text[2]);    
    strcpy(ipos.prop.exclude.text[3], posdb.exclude_text[3]);    
    
    return 0;
}

static int _find_pltbl_start_index(POSX_T *posx, PL_TBL *tbl)
{
	int i, index = 0;
    
	for (i = 0; i < tbl->cnt; i++) 
	{	
        if (tbl->plog[i].log_id == posx->slogid)
        {
            index = i;
            break;
        }
	}

    if (i == tbl->cnt) 
    {
        index = tbl->cnt - 1;
        posx->spos = 0;
    }

	return index;
}

static int _is_include_timestamp(time_t from, time_t to, time_t log_time)
{
    if ((log_time <= from) || (log_time > to)) return 0;

    return 1;
}

static int _proc_dwelltime(POSX_T *posx, PD_TBL *pdtbl, int tbl_row, time_t curtime)
{
    time_t from, to;
    GTimeVal log_time;    
    int i;

    if (ipos.prop.dwell_time == 0) return 0;

    from = curtime - ipos.prop.dwell_time;   
    to = curtime;

    for (i = 0; i < tbl_row; i++)
    {
        GUINT64_TO_GTIMEVAL(pdtbl->pdata[i].timestamp, log_time);
        if (!_is_include_timestamp(from, to, log_time.tv_sec))
        {
            memset(pdtbl->pdata[i].text, 0x00, sizeof(pdtbl->pdata[i].text));
        }
    }

    return 0;
}

static int _is_highlight_row(char *text, int *color)
{
    int i, str_len;
    
    for (i = 0; i < MAX_HIGHLIGHT_CNT; i++)
    {
		if (strlen(ipos.prop.highlight.text[i]) == 0) continue;
		
		if (strcasestr(text, ipos.prop.highlight.text[i])) 
        {
            *color = ipos.prop.highlight.color[i];
		    return 1;
        }
    }

    return 0;
}

static int _proc_highlight(POSX_T *posx, PD_TBL *pdtbl, int tbl_row)
{
    int i, color;

    if (!ipos.prop.highlight.use) return 0;

    for (i = 0; i < tbl_row; i++)
    {
        if (_is_highlight_row(pdtbl->pdata[i].text, &color))
        {
            pdtbl->pdata[i].font_color = color;
        }		
    }

    return 0;
}

static int _exclude_text(char *text)
{
	int i, j, str_len;
	char *p;

	for (i = 0; i < MAX_EXCLUDE_CNT; i++) 
	{
		if (strlen(ipos.prop.exclude.text[i]) == 0) continue;
		
		if (p = strcasestr(text, ipos.prop.exclude.text[i])) 
		{		
            str_len = strlen(ipos.prop.exclude.text[i]);
		
			for (j = 0; j < strlen(text); j++) 
			{
				if ((j < p-text) || (j >= p-text+str_len)) text[j] = '*';
			}
		}
	}
	
    return 0;
}

static int _proc_exclude(POSX_T *posx, PD_TBL *pdtbl, int tbl_row)
{
    int i;

    if (!ipos.prop.exclude.use) return 0;

    for (i = 0; i < tbl_row; i++)
    {
        _exclude_text(pdtbl->pdata[i].text);
    }

    return 0;
}

static int _parse_log_text(NF_POS_LOG_DATA *poslog, char *text)
{
    if (poslog->pos_log_type == NF_POS_LOG_TYPE_ITEM)
    {

    }
    else if (poslog->pos_log_type == NF_POS_LOG_TYPE_TOTAL)
    {

    }
    else if (poslog->pos_log_type == NF_POS_LOG_TYPE_TEXT)
    {
        strcpy(text, poslog->Text.text);
    }
    else if (poslog->pos_log_type == NF_POS_LOG_TYPE_RAW)
    {

    }
	
    return 0;
}

static int _make_pltbl(GList *list, int list_cnt, PL_TBL *pltbl)
{
	GList *plist;
	int i = 0;

    pltbl->cnt = list_cnt;
    pltbl->plog = imalloc(sizeof(NF_LOG_DATA) * list_cnt);

	for (plist = list; plist; plist = g_list_next(plist)) 
	{
        memcpy(&pltbl->plog[i++], plist->data, sizeof(NF_LOG_DATA));
	}
    
    return 0;
}

static int _make_pdtbl(POSX_T *posx, PL_TBL *pltbl, int sidx, PD_TBL *pdtbl, int tbl_row)
{
	int i, j;
	int tbl_idx;

	pdtbl->pdata = imalloc(sizeof(POS_DATA_T) * tbl_row);

    tbl_idx = posx->spos;

	for (i = sidx; i < pltbl->cnt; i++) 
	{    	
        NF_POS_LOG_DATA *poslog = (NF_POS_LOG_DATA*)pltbl->plog[i].text;
    	POS_DATA_T tmp_data;

        memset(&tmp_data, 0x00, sizeof(POS_DATA_T));

        tmp_data.id = poslog->log_id;
        tmp_data.timestamp = poslog->timestamp;    
        tmp_data.font_color = ipos.prop.font_color;
        _parse_log_text(poslog, tmp_data.text);

        if (tbl_idx >= tbl_row)
        {
            tbl_idx = 0;

            if (ipos.prop.scroll_type == 0)
            {
                for (j = 0; j < tbl_row; j++)
                {
                    memset(pdtbl->pdata[j].text, 0x00, sizeof(pdtbl->pdata[j].text));
                }
            }            
        }

        memcpy(&pdtbl->pdata[tbl_idx++], &tmp_data, sizeof(POS_DATA_T));
	}

    return 0;
}

static int _prepare_next_tbl(POSX_T *posx)
{
    int i;

    posx->slogid = posx->data[0].id;
    posx->spos = 0;  

	for (i = 0; i < posx->count; i++) 
    {    
        if (!posx->data[i].id) continue;
    
        if (posx->data[i].id < posx->slogid)
        {
            posx->slogid = posx->data[i].id;
            posx->spos = i;
        }                
    }
   
    return 0;
}

static int _destory_pltbl(PL_TBL *pltbl)
{
    if (pltbl->plog) ifree(pltbl->plog);
    pltbl->cnt = 0;
    return 0;
}

static int _destory_pdtbl(PD_TBL *pdtbl)
{    
    if (pdtbl->pdata) ifree(pdtbl->pdata);
    pdtbl->cnt = 0;
    return 0;
}

static int _get_pos_log_table(POSX_T *posx, int tbl_row, time_t curtime)
{
	GList *list;
	int list_cnt;
	int start_idx;

    PL_TBL pltbl;
    PD_TBL pdtbl;

    list = ipos.page[posx->pageid].loglist;
    if (!list) return -1;
    
    list_cnt = g_list_length(list);    
    if (!list_cnt) return -1;

    memset(&pltbl, 0x00, sizeof(PL_TBL));
    memset(&pdtbl, 0x00, sizeof(PD_TBL));

    _make_pltbl(list, list_cnt, &pltbl);
    start_idx = _find_pltbl_start_index(posx, &pltbl);

    _make_pdtbl(posx, &pltbl, start_idx, &pdtbl, tbl_row);
    _proc_dwelltime(posx, &pdtbl, tbl_row, curtime);
    _proc_highlight(posx, &pdtbl, tbl_row);
    _proc_exclude(posx, &pdtbl, tbl_row);
    
    posx->data = imalloc(sizeof(POS_DATA_T) * tbl_row);
    memcpy(posx->data, pdtbl.pdata, sizeof(POS_DATA_T) * tbl_row);    
    posx->count = tbl_row;

    _prepare_next_tbl(posx);

    _destory_pltbl(&pltbl);
    _destory_pdtbl(&pdtbl);

    return 0;
}

static int _put_live_pos_log(NF_LOG_DATA *log)
{
    NF_POS_LOG_DATA *poslog = (NF_POS_LOG_DATA*)log->text;  
    int i, cnt;

    GTimeVal tv;
    char strBuf[128];
   
    for (i = 0; i < var_get_ch_count(); i++)
    {    
        if (ipos.page[i].chmask & (1 << poslog->chan))
        {
        	GList *list, *plist;
        	NF_LOG_DATA *data = imalloc(sizeof(NF_LOG_DATA));
            NF_POS_LOG_DATA *posData;

        	list = ipos.page[i].loglist;

            if ((list) && (g_list_length(list) >= MAX_ROW))
            {
                plist = g_list_nth(list, 0);
                ifree(plist->data);
                
                list = g_list_delete_link(list, plist);
            }

            memcpy(data, log, sizeof(NF_LOG_DATA));
            data->log_id = g_put_livelogid;
            
            posData = (NF_POS_LOG_DATA*)data->text;
            posData->log_id = g_put_livelogid;

#if 0
            memset(posData->Text.text, 0x00, sizeof(posData->Text.text));
            memset(strBuf, 0x00, sizeof(strBuf));
            GUINT64_TO_GTIMEVAL(poslog->timestamp, tv);
            ifn_get_local_hourmin_text(tv.tv_sec, H24, strBuf);
            g_sprintf(posData->Text.text, "[%llu] [%s]", g_put_livelogid, strBuf);
#endif            
        	ipos.page[i].loglist = g_list_append(list, data);
        }
    }
   
    g_put_livelogid++;

	return 0;
}

static int _put_playback_pos_log(NF_LOG_DATA *log)
{
    int i, cnt;
    NF_POS_LOG_DATA *poslog = (NF_POS_LOG_DATA*)log->text;
   
    for (i = 0; i < var_get_ch_count(); i++)
    {    
        if (ipos.page[32+i].chmask & (1 << poslog->chan))
        {
        	GList *list, *plist;
        	NF_LOG_DATA *data = imalloc(sizeof(NF_LOG_DATA));
            NF_POS_LOG_DATA *posData;

            list = ipos.page[32+i].loglist;

            if ((list) && (g_list_length(list) >= MAX_ROW))
            {
                plist = g_list_nth(list, 0);
                ifree(plist->data);
                
                list = g_list_delete_link(list, plist);
            }

            memcpy(data, log, sizeof(NF_LOG_DATA));
            data->log_id = g_put_playbacklogid;

            posData = (NF_POS_LOG_DATA*)data->text;
            posData->log_id = g_put_playbacklogid;

#if 0
            memset(posData->Text.text, 0x00, sizeof(posData->Text.text));
            g_sprintf(posData->Text.text, "[%llu] [%s]", g_put_playbacklogid, poslog->Text.text);
#endif

            ipos.page[32+i].loglist = g_list_append(list, data);
        }
    }
   
    g_put_playbacklogid++;
   
	return 0;
}

static int _put_test_pos_log(NF_LOG_DATA *log)
{
    NF_POS_LOG_DATA *poslog = (NF_POS_LOG_DATA*)log->text;
   
	GList *list, *plist;
	NF_LOG_DATA *data = imalloc(sizeof(NF_LOG_DATA));
    NF_POS_LOG_DATA *posData;

    list = ipos.page[64].loglist;

    if ((list) && (g_list_length(list) >= MAX_ROW))
    {
        plist = g_list_nth(list, 0);
        ifree(plist->data);
        
        list = g_list_delete_link(list, plist);
    }

    memcpy(data, log, sizeof(NF_LOG_DATA));
    data->log_id = g_put_testid;
    
    posData = (NF_POS_LOG_DATA*)data->text;
    posData->log_id = g_put_testid;

#if 0
    memset(posData->Text.text, 0x00, sizeof(posData->Text.text));
    g_sprintf(posData->Text.text, "[%llu] [%s]", log->log_id, poslog->Text.text);
#endif

    ipos.page[64].loglist = g_list_append(list, data);

    g_put_testid++;
   
	return 0;
}

static int _clear_test_pos_log()
{
	GList *list, *plist;

    list = ipos.page[64].loglist;
	
	for (plist = list; plist; plist = g_list_next(plist)) 
	{
        ifree(plist->data);	
		list = g_list_delete_link(list, plist);
	}
	
    return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int posx_init()
{
    int i;
    
    memset(&ipos, 0x00, sizeof(POS_T));    

    // live page
    for (i = 0; i < var_get_ch_count(); i++)
    {
        ipos.page[i].init = 1;
        ipos.page[i].chmask = (1 << i);
    }

    // playback page
    for (i = 0; i < var_get_ch_count(); i++)
    {
        ipos.page[32+i].init = 1;
        ipos.page[32+i].chmask = (1 << i);
    }

    // test page
    ipos.page[64].chmask = 0xffff;

    _load_property();    

	ipos.mtx = g_mutex_new();
    
    return 0;
}

int posx_reload_property()
{
    _load_property();
    return 0;
}

int posx_put_live_testlog(int ch, int index, char *str)
{
	NF_LOG_DATA log_data;
    NF_POS_LOG_DATA *poslog;  

    GTimeVal tv;
    
    gettimeofday(&tv, NULL);

    log_data.type = LT_SYSTEM_POS;
    log_data.timestamp = GTIMEVAL_TO_GUINT64(tv);

    poslog = imalloc(sizeof(NF_POS_LOG_DATA));
    poslog->chan = ch;
    poslog->pos_log_type = 2;
    poslog->timestamp = GTIMEVAL_TO_GUINT64(tv);
    
    g_sprintf(poslog->Text.text, "##### [%04d] @@@@ POS DISPLAY TEST --- [%s]", index, str);
    
    memcpy(log_data.text, poslog, sizeof(NF_POS_LOG_DATA));

    POS_LOCK();
    _put_live_pos_log(&log_data);
    POS_UNLOCK();
    
    ifree(poslog);
    return 0;
}

int posx_put_live_log(NF_LOG_DATA *log)
{
    static time_t prev_time = 0;
    int i;

    if (!log) return -1;
    if (log->type != LT_SYSTEM_POS) return -1;

    POS_LOCK();

    _put_live_pos_log(log);

    if (abs(prev_time-time(0)) > 2)
    {
        g_message("%s, %d, time : %d, list count", __FUNCTION__, __LINE__, time(0));

        for (i = 0; i < var_get_ch_count(); i++)
        {
            if (ipos.page[i].loglist == 0) printf("[%2d-00]", i);
            else printf("[%2d-%2d]", i, g_list_length(ipos.page[i].loglist));
        }

        printf("\n");

        prev_time = time(0);
    }
    
    POS_UNLOCK();    
    
    return 0;
}

int posx_put_playback_log(NF_LOG_DATA *log)
{
    static time_t prev_time = 0;
    int i;

    if (!log) return -1;
    if (log->type != LT_SYSTEM_POS) return -1;

    POS_LOCK();
    
    _put_playback_pos_log(log);

    if (abs(prev_time-time(0)) > 2)
    {       
        g_message("%s, %d, time : %d, list count", __FUNCTION__, __LINE__, time(0));    

        for (i = 0; i < var_get_ch_count(); i++)
        {
            if (ipos.page[32+i].loglist == 0) printf("[%2d-00]", i);
            else printf("[%2d-%2d]", i, g_list_length(ipos.page[32+i].loglist));
        }

        printf("\n");

        prev_time = time(0);
    }
    
    POS_UNLOCK();    
    
    return 0;
}

int posx_put_test_log(int ch, char *str)
{
	NF_LOG_DATA log_data;
    NF_POS_LOG_DATA *poslog;  

    GTimeVal tv;
    
    gettimeofday(&tv, NULL);

    log_data.type = LT_SYSTEM_POS;
    log_data.timestamp = GTIMEVAL_TO_GUINT64(tv);

    poslog = imalloc(sizeof(NF_POS_LOG_DATA));
    poslog->chan = ch;
    poslog->pos_log_type = 2;
    poslog->timestamp = GTIMEVAL_TO_GUINT64(tv);
    
    g_sprintf(poslog->Text.text, "%s", str);    
    memcpy(log_data.text, poslog, sizeof(NF_POS_LOG_DATA));

    POS_LOCK();
    _put_test_pos_log(&log_data);
    POS_UNLOCK();
    
    ifree(poslog);
    return 0;
}

int posx_clear_test_log()
{
	GList *list;

    list = ipos.page[64].loglist;
    if (!list) return -1;

    _clear_test_pos_log();
    ipos.page[64].loglist = 0;
    
    return 0;
}

POSX_T *posx_create(int pageid)
{
	POSX_T *posx;

	posx = imalloc(sizeof(POSX_T));
	memset(posx, 0x00, sizeof(POSX_T));
	posx->pageid = pageid;
	posx->slogid = 0;
	posx->count = 0;

	return posx;
}

int posx_destroy(POSX_T *posx)
{
    if (!posx) return -1;

	if (posx->data) ifree(posx->data);
	ifree(posx);

	return 0;
}

int posx_get_live_display_onoff()
{
    int onoff = 0;

    if (!ipos.prop.enable) return 0;
    if (ipos.prop.mode == 1 || ipos.prop.mode == 3) onoff = 1;

    return onoff;
}

int posx_get_playback_display_onoff()
{
    int onoff = 0;

    if (!ipos.prop.enable) return 0;
    if (ipos.prop.mode == 2 || ipos.prop.mode == 3) onoff = 1;

    return onoff;
}

int posx_get_pos_conf(POS_CONF_T *conf)
{
    conf->align = ipos.prop.align;
    conf->font_type = ipos.prop.font_type;
    return 0;
}

int posx_get_pos_table(POSX_T *posx, int tbl_row)
{
	GList *list, *plist;
    static time_t prev_time = 0;
    int i;

    list = ipos.page[posx->pageid].loglist;

    if (!list) return -1;
    if (!g_list_length(list)) return -1;

    POS_LOCK();
    
    _get_pos_log_table(posx, tbl_row, time(0));    

    if (abs(prev_time-time(0)) > 2)
    {
        g_message("%s, %d, time : %d, list count", __FUNCTION__, __LINE__, time(0)); 

        for (i = 0; i < var_get_ch_count(); i++)
        {
            if (ipos.page[i].loglist == 0) printf("[%2d-00]", i);
            else printf("[%2d-%2d]", i, g_list_length(ipos.page[i].loglist));
        }

        printf("\n");

        prev_time = time(0);
    }
    
    POS_UNLOCK();
    
    return 0;
}

int posx_get_pos_table_with_time(POSX_T *posx, int tbl_row, time_t gettime)
{
	GList *list, *plist;
    static time_t prev_time = 0;
    int i;

    list = ipos.page[posx->pageid].loglist;

    if (!list) return -1;
    if (!g_list_length(list)) return -1;

    POS_LOCK();
    
    _get_pos_log_table(posx, tbl_row, gettime);    

    if (abs(prev_time-time(0)) > 2)
    {
        g_message("%s, %d, time : %d, list count", __FUNCTION__, __LINE__, time(0));  

        for (i = 0; i < var_get_ch_count(); i++)
        {
            if (ipos.page[32+i].loglist == 0) printf("[%2d-00]", i);
            else printf("[%2d-%2d]", i, g_list_length(ipos.page[32+i].loglist));
        }

        printf("\n");

        prev_time = time(0);
    }
    
    POS_UNLOCK();
    
    return 0;
}

