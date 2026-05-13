
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_dvabx_component.h"
#include "vw_vkeyboard.h"

#include "vw_event_email_edit.h"
#include "vw_event_audio_edit.h"
#include "ix_mem.h"

#include "nf_record.h"

#include <dirent.h>

enum {
	EMAIL_ADD_COMPLETE = 0,
	EMAIL_ADD_FULL,
	EMAIL_ADD_DUPLICATION
};


static gchar (*g_address)[EMAIL_STRING_LENGTH];
static guint g_addrCnt = 0;

static gchar g_file[FILE_COUNT][FILE_STRING_LENGTH];
static gint g_fileCnt;
static gint g_org_fileCnt;
static guint g_evntCnt = 0;
static guint g_file_idx_of_evt[EVENT_COUNT] = {0};
static guint g_org_file_idx_of_evt[EVENT_COUNT] = {0};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_addrObj;
static NFOBJECT *g_testObj;
static NFOBJECT *g_addObj;
static NFOBJECT *g_lboxObj;
static NFOBJECT *g_combo[FILE_COUNT];
static NFOBJECT *g_filecombo;

DVA_COMPONENT_DATA_T *g_component_data;

static char *remove_ext(char* mystr) {
    char *retstr;
    char *lastdot;
    if (mystr == NULL)
         return NULL;
    if ((retstr = malloc (strlen (mystr) + 1)) == NULL)
        return NULL;
    strcpy (retstr, mystr);
    lastdot = strrchr (retstr, '.');
    if (lastdot != NULL)
        *lastdot = '\0';
    return retstr;
}

static void list_dir(gchar (*file)[FILE_STRING_LENGTH])
{
    struct dirent *entry;
    DIR *dir = opendir("/opt/");
	char *ext;
	gint i = 0;

	if(g_fileCnt) g_fileCnt = 0;
	memset(g_file, 0x00, sizeof(g_file));

    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
		if((ext=strrchr(entry->d_name, '.')) == NULL)	
		{
			continue;
		}
		if(!strcmp(ext, ".wav")) 
		{
			strcpy(g_file[i], entry->d_name);
			i++;
		}
    }

	if(g_fileCnt) g_fileCnt = 0;

	for(i=0; i<FILE_COUNT; i++) {
		if(strlen(g_file[i]))
		{
			g_fileCnt++;
		}
	}
	g_org_fileCnt = g_fileCnt;

    closedir(dir);
}
static gint _get_focused_eventidx(gchar *event)
{
	guint event_idx;

	if(!strcmp(event, "INTRUSION"))	event_idx = 0;
	else if(!strcmp(event, "ENTER")) event_idx = 1;
	else if(!strcmp(event, "EXIT")) event_idx = 2;
	else if(!strcmp(event, "REMOVED")) event_idx = 3;
	else if(!strcmp(event, "LOITERING")) event_idx = 4;
	else if(!strcmp(event, "STOPPED")) event_idx = 5;
	else if(!strcmp(event, "FORWARD DIRECTION")) event_idx = 6;
	else if(!strcmp(event, "REVERSE DIRECTION")) event_idx = 7;
	else return -1;

	return event_idx;
}

static gint _get_focused_event(gchar event[EVENT_STRING_LENGTH])
{
    if (nfui_nfobject_is_shown(g_lboxObj) == FALSE) return -1;
	if (nfui_listbox_get_focus_text((NFLISTBOX*)g_lboxObj, 0) == NULL) return -1;

    strcpy(event, nfui_listbox_get_focus_text((NFLISTBOX*)g_lboxObj, 0));
    return 0;
}

static gboolean delete_file(guint index)
{
	if(index + 1 == FILE_COUNT) {
		memset(g_file[index], 0x00, FILE_STRING_LENGTH);
		g_fileCnt -= 1;
	}
	else if (index + 1 < FILE_COUNT) {
    	g_memmove(g_file[index], g_file[index + 1], ((FILE_COUNT - index - 1) * FILE_STRING_LENGTH));
    	memset(g_file[g_fileCnt - 1], 0x00, FILE_STRING_LENGTH);

    	g_fileCnt -= 1;
	}
	else {
		memset(g_file[index], 0x00, FILE_STRING_LENGTH);
		return FALSE;
	}
	g_org_fileCnt = g_fileCnt;

	return TRUE;
}

static gboolean post_test_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		int ch_mask = 0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if (strcmp("NONE", nfui_combobox_get_value(g_filecombo)))
		{
			ch_mask |= (1 << g_component_data->preview.ch);
//ksi_test
			#if defined(ENABLE_AI_ALARM_AUDIO)
				nf_audio_ai_alarm_test(ch_mask, nfui_combobox_get_value(g_filecombo));
			#elif defined(ENABLE_AI_ALARM_AUDIO_DSP)
				nf_rec_audio_alarm_test(ch_mask, nfui_combobox_get_value(g_filecombo));
			#elif  defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
				nf_HI_audio_alarm_test(ch_mask, nfui_combobox_get_value(g_filecombo));
			#endif			// #if defined(ENABLE_AI_ALARM_AUDIO_DSP)
			// 	nf_rec_audio_alarm_test(ch_mask, nfui_combobox_get_value(g_filecombo));
			// #elif  defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
			// 	nf_HI_audio_alarm_test(ch_mask, nfui_combobox_get_value(g_filecombo));
			// #endif
		}
	}
	return FALSE;
}

static gboolean post_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		int i;
		guint event_idx;
		gchar *event;
		gchar strBuf[256];

		if (g_fileCnt < FILE_COUNT)
			VW_System_AudioUpload_Open(g_curwnd, g_file);
		else
		{
			snprintf(strBuf, sizeof(strBuf)-1, lookup_string("The maximum number of files that can be uploaded is %d. Please delete the file to upload another file."), FILE_COUNT);
			nftool_mbox(g_curwnd, "WARNING", strBuf, NFTOOL_MB_OK);
		}

		if(g_fileCnt) g_fileCnt = 0;

		for(i=0; i<FILE_COUNT; i++) {
			if(strlen(g_file[i]))
				g_fileCnt++;
		}
	}

	return FALSE;
}

static gboolean post_lbox_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_LISTBOX_CHANGED)
	{
		gint i;
		guint event_idx;
		gchar *event;
		
		event = nfui_listbox_get_text_of_list((NFLISTBOX*)g_lboxObj, nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj)), 0);
		if (event == NULL)
		{
			nfui_nfobject_disable(g_filecombo);
			nfui_nfobject_disable(g_testObj);
			nfui_nfobject_disable(g_addObj);
			nfui_signal_emit(g_filecombo, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_addObj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
		if ((event_idx = _get_focused_eventidx(event)) == -1)
		{
			nfui_nfobject_disable(g_filecombo);
			nfui_nfobject_disable(g_testObj);
			nfui_nfobject_disable(g_addObj);
			nfui_signal_emit(g_filecombo, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_addObj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
		
		if(event_idx >= 0) {
			if (event_idx < EVENT_COUNT)
			{
				nfui_nfobject_enable(g_filecombo);

				nfui_combobox_set_index_no_expose(g_filecombo, g_org_file_idx_of_evt[event_idx]);

				if (!strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)), "NONE"))	nfui_nfobject_disable(g_testObj);
				else	nfui_nfobject_enable(g_testObj);
				nfui_nfobject_enable(g_addObj);
			}
			else
			{
				nfui_nfobject_disable(g_filecombo);
				nfui_nfobject_disable(g_testObj);
				nfui_combobox_set_index_no_expose(g_filecombo,0);
			}
			nfui_signal_emit(g_filecombo, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_addObj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		int idx;
		int f_idx;
		guint event_idx = 0;
		gchar *event = NULL;

		if (!strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)), "NONE"))	nfui_nfobject_disable(g_testObj);
		else	nfui_nfobject_enable(g_testObj);
		nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);

		event = nfui_listbox_get_text_of_list((NFLISTBOX*)g_lboxObj, nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj)), 0);
		if (event == NULL)	return FALSE;
		if ((event_idx = _get_focused_eventidx(event)) == -1)	return FALSE;
		
		g_file_idx_of_evt[event_idx] = nfui_combobox_get_cur_index(g_filecombo);
		memcpy(g_org_file_idx_of_evt, g_file_idx_of_evt, sizeof(g_org_file_idx_of_evt));
	}
	else if(evt->type == GDK_EXPOSE) 
	{

        if (g_org_fileCnt != g_fileCnt)
        {
			int i;
			int event_idx;
			gchar *event = NULL;

			g_org_fileCnt = g_fileCnt;
			nfui_combobox_remove_all(g_filecombo);
			nfui_combobox_append_data(g_filecombo, "NONE");
			for (i=0; i<g_fileCnt; i++)
				nfui_combobox_append_data(g_filecombo, g_file[i]);

			nfui_combobox_set_index_no_expose(g_filecombo, g_fileCnt);

			if (!strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)), "NONE"))	nfui_nfobject_disable(g_testObj);
			else	nfui_nfobject_enable(g_testObj);

			event = nfui_listbox_get_text_of_list((NFLISTBOX*)g_lboxObj, nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj)), 0);
			if (event == NULL)	return FALSE;
			if ((event_idx = _get_focused_eventidx(event)) == -1)	return FALSE;

			g_file_idx_of_evt[event_idx] = nfui_combobox_get_cur_index(g_filecombo);
			memcpy(g_org_file_idx_of_evt, g_file_idx_of_evt, sizeof(g_org_file_idx_of_evt));

			nfui_signal_emit(g_filecombo, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);
        }
	}

	return FALSE;
}

static gboolean post_close_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;
		int i;
		int count = 0;
		guint event_idx = 0;;
		gchar *event = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		memset(g_component_data->area.event_audio, 0x00, sizeof(g_component_data->area.event_audio));
		memset(g_component_data->line.event_audio, 0x00, sizeof(g_component_data->line.event_audio));

		for (i=0; i<EVENT_COUNT; i++)
		{
			event = nfui_listbox_get_text_of_list((NFLISTBOX*)g_lboxObj, i, 0);
			if (event == NULL)
				continue;

			if ((event_idx = _get_focused_eventidx(event)) == -1)
				continue;

			nfui_combobox_set_index_no_expose(g_filecombo, g_file_idx_of_evt[event_idx]);

			if (!strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)), "NONE"))
			{
				snprintf(g_component_data->area.event_audio[event_idx], sizeof(g_component_data->area.event_audio[event_idx])-1, "");
				snprintf(g_component_data->line.event_audio[event_idx], sizeof(g_component_data->line.event_audio[event_idx])-1, "");
			}
			else
			{
				snprintf(g_component_data->area.event_audio[event_idx], sizeof(g_component_data->area.event_audio[event_idx])-1, "%s", nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)));
				snprintf(g_component_data->line.event_audio[event_idx], sizeof(g_component_data->line.event_audio[event_idx])-1, "%s", nfui_combobox_get_value(NF_COMBOBOX(g_filecombo)));
			}
		}

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}
	return FALSE;
}

static gboolean post_delete_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
		gint f_idx;
		gchar file_name[128];
		gchar full_name[256];
		gchar raw_full_name[256];
		guint event_idx;
		guint event_value;
		gchar *event;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_message("DELETE => %s", nfui_combobox_get_value(g_filecombo));
		f_idx = nfui_combobox_get_cur_index(g_filecombo);

		if((nfui_combobox_get_value(g_filecombo) == NULL) || (!strcmp("NONE", nfui_combobox_get_value(g_filecombo))) || nfui_nfobject_is_disabled(g_filecombo))
				return FALSE;
		else
		{
			//_get_focused_file(file_name);
			strcpy(file_name, nfui_combobox_get_value(g_filecombo));
			sprintf(full_name, "/opt/%s", file_name);

			//delete_file((guint)f_idx);
			g_memmove(g_file[f_idx-1], g_file[f_idx], ((FILE_COUNT - (f_idx-1) - 1) * FILE_STRING_LENGTH));
			memset(g_file[g_fileCnt - 1], 0x00, FILE_STRING_LENGTH);
			g_fileCnt -= 1;
			g_org_fileCnt = g_fileCnt;

			nfui_combobox_remove_all(g_filecombo);
			nfui_combobox_append_data(g_filecombo, "NONE");
			for (i=0; i<g_fileCnt; i++)
			{
				nfui_combobox_append_data(g_filecombo, g_file[i]);
			}
			nfui_combobox_set_index_no_expose(g_filecombo, 0);
			nfui_signal_emit(g_filecombo, GDK_EXPOSE, TRUE);
			sprintf(raw_full_name, "/opt/%s.raw", remove_ext(file_name));

			if (strcmp(file_name, "NONE"))
			{
				int nResult = remove(full_name);
				nResult = remove(raw_full_name);

				if( nResult == 0 )
				{
					printf( "file delete success" );
				}
				else if( nResult == -1 )
				{
					perror( "file delete fail" );
				}
			}

			nfui_nfobject_disable(g_testObj);
			nfui_signal_emit(g_testObj, GDK_EXPOSE, TRUE);

			event = nfui_listbox_get_text_of_list((NFLISTBOX*)g_lboxObj, nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj)), 0);
			if (event == NULL)	return FALSE;
			if ((event_idx = _get_focused_eventidx(event)) == -1)
			{
				nfui_nfobject_disable(g_filecombo);
				return FALSE;
			}

			event_value = g_file_idx_of_evt[event_idx];
			g_file_idx_of_evt[event_idx] = 0;

			for(i=0; i<EVENT_COUNT; i++)
			{
				if(g_file_idx_of_evt[i] == event_value)
					g_file_idx_of_evt[i] = 0;
				else if(g_file_idx_of_evt[i] > event_value)
					g_file_idx_of_evt[i]--;
			}

			memcpy(g_org_file_idx_of_evt, g_file_idx_of_evt, sizeof(g_org_file_idx_of_evt));
		}
	}
	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	}

	return FALSE;
}

static gboolean post_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void VW_EvtNoti_Audio_Edit(NFWINDOW *parent, DVA_COMPONENT_DATA_T *component_data)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *lbox;
	NFOBJECT *obj;
	NFOBJECT *delete_btn;

	gint scl_w, scl_h;
	guint pos_x, pos_y;
	guint col_w = 428;
	guint i = 0, j = 0;
	gchar strBuf[128];
	gchar (*file)[FILE_STRING_LENGTH];
	gchar *pStr;

	// init data
	g_component_data = component_data;
	memset(g_file_idx_of_evt, 0x00, sizeof(g_file_idx_of_evt));
	memset(g_org_file_idx_of_evt, 0x00, sizeof(g_org_file_idx_of_evt));
	list_dir(g_file);
	
	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 660, 196, 592, 598);
	g_curwnd = win;
	nfui_nfwindow_set_title(win, "SYSTEM SETUP - EVENT - EVENT NOTI - EMAIL");
	nfui_regi_post_event_callback(win, post_win_event_cb);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 592, 598);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SELECT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);
	pos_x = 26;
	pos_y = 64;

	memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("EVENT"));
    strcat(strBuf, " : ");

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	pos_x += (10 + 96);

	nfui_get_image_size(IMG_N_SCROLL_UP, &scl_w, &scl_h);
    col_w -= scl_w;

	g_lboxObj = nfui_listbox_new(1, &col_w, 40);
	nfui_listbox_set_skin_type(NF_LISTBOX(g_lboxObj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(g_lboxObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_listbox_support_multi_lang(NF_LISTBOX(g_lboxObj), FALSE);
	nfui_nfobject_set_size(g_lboxObj, 428, 340);
	nfui_nfobject_use_focus(g_lboxObj, NFOBJECT_FOCUS_ON);
	if (g_component_data->area.intrusion) 
	{
		pStr = "INTRUSION";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->area.enter)
	{
		pStr = "ENTER";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->area.exit) 
	{
		pStr = "EXIT";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->area.removed) 
	{
		pStr = "REMOVED";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->area.loitering) 
	{
		pStr = "LOITERING";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->area.stopped) 
	{
		pStr = "STOPPED";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->line.forward) 
	{
		pStr = "FORWARD DIRECTION";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	if (g_component_data->line.reverse) 
	{
		pStr = "REVERSE DIRECTION";
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
		g_evntCnt++;
	}
	nfui_nfobject_show(g_lboxObj);
	nfui_regi_post_event_callback(g_lboxObj, post_lbox_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, g_lboxObj, pos_x, pos_y);

	pos_x = 26;
	pos_y += (340 + 20);

	memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s", lookup_string("AUDIO"));
    strcat(strBuf, " : ");

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	/* add */
	pos_x = 26;
	pos_y += 50;

	g_filecombo = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)g_filecombo, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)g_filecombo, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(g_filecombo);
	nfui_nfobject_set_size(g_filecombo, 270, 40);
    nfui_nffixed_put((NFFIXED*)fixed, g_filecombo, pos_x, pos_y);
	nfui_nfobject_disable(g_filecombo);

	nfui_combobox_append_data(g_filecombo, "NONE");
	for (i=0; i<g_fileCnt; i++)
	{
		nfui_combobox_append_data(g_filecombo, g_file[i]);
	}

	for (i=0; i<EVENT_COUNT; i++)
	{
		for (j=0; j<g_fileCnt; j++)
		{
			nfui_combobox_set_index_no_expose(g_filecombo, j+1);
			if(!strcmp(g_component_data->area.event_audio[i], nfui_combobox_get_value(g_filecombo)))
				g_file_idx_of_evt[i] = j+1;
			if(!strcmp(g_component_data->line.event_audio[i], nfui_combobox_get_value(g_filecombo)))
				g_file_idx_of_evt[i] = j+1;
		}
}
	memcpy(g_org_file_idx_of_evt, g_file_idx_of_evt, sizeof(g_file_idx_of_evt));
	nfui_regi_post_event_callback(g_filecombo, post_combo_event_handler);

	pos_x += (270 + 20);
	g_testObj = nftool_normal_button_create_type3("TEST", 120);
	//nfui_nfobject_set_size(obj, 140, 40);
	nfui_regi_post_event_callback(g_testObj, post_test_button_event_handler);
	nfui_nfobject_show(g_testObj);
	nfui_nffixed_put((NFFIXED*)fixed, g_testObj, pos_x, pos_y);
	nfui_nfobject_disable(g_testObj);

	pos_x += (120 + 10);
	/* add */
	g_addObj = nftool_normal_button_create_type3("LOAD", 120);
	nfui_regi_post_event_callback(g_addObj, post_add_button_event_handler);
	nfui_nfobject_set_size(obj, 140, 40);
	nfui_nfobject_show(g_addObj);
	nfui_nfobject_disable(g_addObj);

	nfui_nffixed_put((NFFIXED*)fixed, g_addObj, pos_x, pos_y);
	pos_x = 126;
	pos_y += (56);

	/* button */
	obj = nftool_normal_button_create_type1("DELETE", 174);
	nfui_regi_post_event_callback(obj, post_delete_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	delete_btn = obj;
	pos_x += (174 + 6);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(delete_btn, TRUE);

	nfui_page_open(PGID_EVT_EMAIL_EDIT_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_EVT_EMAIL_EDIT_POPUP, win);
}
