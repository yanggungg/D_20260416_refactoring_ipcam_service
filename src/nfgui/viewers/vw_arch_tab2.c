#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfbutton.h"

#include "../viewers/objects/cw_calendar.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_archiving.h"
#include "vw_arch_tab2.h"
#include "vw_arch_export.h"
#include "vw_snapshot.h"

#include "../services/scm.h"
#include "vw_internal.h"
#include <math.h>
#include "dtf.h"

#define MAX_ROW							(19)

// LIST LABEL
#define ART2_LIST_LABEL_H				(38)
#define ART2_LIST_LABEL_H_INTERVAL		(42)
#define ART2_LIST_START_X				(137)
#define ART2_LIST_START_Y				(0)

// PREV, NEXT BUTTON
#define ART2_PAGE_BTN_SIZE				(60)

#define	ART2_PREV_BTN_X					(781)
#define	ART2_PREV_BTN_Y					(ART2_LIST_START_Y + ART2_LIST_LABEL_H_INTERVAL*(MAX_ROW+1) + 10)

#define ART2_PAGE_LABEL_W				(120)
#define ART2_PAGE_LABEL_H				(40)
#define ART2_PAGE_LABEL_X		(ART2_PREV_BTN_X + ART2_PAGE_BTN_SIZE)
#define ART2_PAGE_LABEL_Y				(ART2_PREV_BTN_Y)

#define	ART2_NEXT_BTN_X			(ART2_PAGE_LABEL_X + ART2_PAGE_LABEL_W)
#define	ART2_NEXT_BTN_Y					(ART2_PAGE_LABEL_Y)

// DELETE BTN
#define ART2_DELETE_BTN_SIZE			(200)
#define ART2_DELETE_BTN_X				(ART2_NEXT_BTN_X + ART2_PAGE_BTN_SIZE + 406)
#define ART2_DELETE_BTN_Y				(ART2_NEXT_BTN_Y)


#define NORMAL_OUTLINE_COLOR		0
#define FOCUS_OUTLINE_COLOR			146
#define SELECT_OUTLINE_COLOR		147

enum {
	ART2_KIND_AVI,
	ART2_KIND_SNAP,
	ART2_KIND_PRESERVE,

	ART2_KIND_NUM
};

enum {
	STATUS_NORMAL,
	STATUS_OVERWRITING,

	STATUS_NUM
};



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *titlelbl_obj[5];			// TITLE
static NFOBJECT *listlbl_obj[MAX_ROW][5];	// LIST
static NFOBJECT *delbtns_obj[MAX_ROW];		// DELETE ITEM BUTTONS
static NFOBJECT *pagelbl_obj;				// PAGE NUMBER
static NFOBJECT *kindcombo_obj;
static NFOBJECT *playback_obj;
static NFOBJECT *export_obj;
static NFOBJECT *del_export_obj;

static NFOBJECT *g_content_fixed;

static NF_ARCH_AVI_INFO 	*art2_avi_info;
static NF_ARCH_SNAP_INFO 	*art2_snap_info;
static NF_DISK_PRESERVE_INFO *art2_preserve_info;

static gint focused_row = -1;
static gint selected_row = -1;

static gint cur_page = 0;	// 1 ~ n
static gint total_page = 0;

static gint avi_count = 0;
static guint16 art2_avi_id[MAX_ROW];
static guint64 art2_avi_chmask[MAX_ROW];

static gint snap_count = 0;
static gint bak_snap_count = 0;
static guint16 art2_snap_id[MAX_ROW];

static gint preserve_cnt = 0;
static guint16 art2_preserve_id[MAX_ROW];


static void _change_outLine_list(gint row, gint color)
{
	static GdkGC *gc;
	static GdkDrawable *drawable;
	gint i, obj_x, obj_y, obj_w, obj_h;
	gint outline_w=0, outline_h=0;

	drawable = nfui_nfobject_get_window((NFOBJECT*)listlbl_obj[row][0]);
	gc = nfui_nfobject_get_gc(listlbl_obj[row][0]);

	gdk_gc_set_line_attributes(gc,
			2,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	for (i = 0; i < 5; i++)
	{
		if (nfui_nfobject_is_shown(listlbl_obj[row][i]))
		{
			nfui_nfobject_get_size(listlbl_obj[row][i], &obj_w, &obj_h);
			outline_w += (obj_w + 2);
		}
	}

	outline_h = obj_h+2;

	nfui_nfobject_get_offset(listlbl_obj[row][0], &obj_x, &obj_y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
	gdk_draw_rectangle (drawable, gc, FALSE, obj_x-1, obj_y-1, outline_w, outline_h);

	nfui_nfobject_gc_unref(gc);
}

static gint _draw_selectLine(gint row)
{
	if (selected_row == row)
		return -1;

		if (selected_row != -1)
			_change_outLine_list(selected_row, NORMAL_OUTLINE_COLOR);

		_change_outLine_list(row, SELECT_OUTLINE_COLOR);
		selected_row = row;

	return 0;
}

static gint _erase_selectLine()
{
	if (selected_row == -1)
		return -1;

	_change_outLine_list(selected_row, NORMAL_OUTLINE_COLOR);
	selected_row = -1;

	return 0;
}

static gint _draw_focusLine(gint row)
{
	if (focused_row == row)
		return -1;

// leave old focus row
	if ((selected_row != -1) && (selected_row == focused_row))
		_change_outLine_list(selected_row, SELECT_OUTLINE_COLOR);
	else if (focused_row != -1)
		_change_outLine_list(focused_row, NORMAL_OUTLINE_COLOR);

// enter new focus row
	_change_outLine_list(row, FOCUS_OUTLINE_COLOR);
	focused_row = row;

	return 0;
}

static gint _erase_focusLine()
{
	if (focused_row == -1)
		return -1;

	if (focused_row == selected_row)
		_change_outLine_list(focused_row, SELECT_OUTLINE_COLOR);
	else
		_change_outLine_list(focused_row, NORMAL_OUTLINE_COLOR);

	focused_row = -1;

	return 0;
}

static gint _find_index_matched_listOBJ(NFOBJECT *obj)
{
	gint i;

	for(i = 0; i < MAX_ROW; i++)
	{
		if ((listlbl_obj[i][0] == obj)
			|| (listlbl_obj[i][1] == obj)
			|| (listlbl_obj[i][2] == obj)
			|| (listlbl_obj[i][3] == obj)
			|| (listlbl_obj[i][4] == obj))
		{
			break;
		}
	}

	if (i == MAX_ROW)
	{
		g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
		g_assert(0);
	}

	return i;
}

static gint _find_column_matched_listOBJ(NFOBJECT *obj, gint row)
{
	gint i;

	for(i = 0; i < 5; i++)
	{
		if (listlbl_obj[row][i] == obj)
			break;
	}

	if (i == 5)
	{
		g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
		g_assert(0);
	}

	return i;
}

static gint _update_page_label(gint current_page, gint total_page)
{
	gchar page_buf[40];

	sprintf(page_buf, "%d/%d", current_page, total_page);
	nfui_nflabel_set_text((NFLABEL*)pagelbl_obj, page_buf);
	nfui_signal_emit(pagelbl_obj, GDK_EXPOSE, TRUE);

	return 0;
}

static void _change_list_form(gint type)
{
	gint i, j;

	if (type == ART2_KIND_AVI)
	{
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[0], "NAME");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[1], "START");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[2], "END");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[3], "SIZE");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[4], "SET BY");

		nfui_nfobject_set_size(titlelbl_obj[0], 318, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[1], 329, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[2], 329, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[3], 160, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[4], 394, ART2_LIST_LABEL_H);

		nfui_nfobject_show(titlelbl_obj[0]);
		nfui_nfobject_show(titlelbl_obj[1]);
		nfui_nfobject_show(titlelbl_obj[2]);
		nfui_nfobject_show(titlelbl_obj[3]);
		nfui_nfobject_show(titlelbl_obj[4]);

		for (i = 0; i < MAX_ROW; i++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][0], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][1], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][2], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][3], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][4], "");

			nfui_nfobject_set_size(listlbl_obj[i][0], 318, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][1], 329, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][2], 329, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][3], 160, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][4], 394, ART2_LIST_LABEL_H);

			nfui_nfobject_show(listlbl_obj[i][0]);
			nfui_nfobject_show(listlbl_obj[i][1]);
			nfui_nfobject_show(listlbl_obj[i][2]);
			nfui_nfobject_show(listlbl_obj[i][3]);
			nfui_nfobject_show(listlbl_obj[i][4]);

			nfui_nfobject_disable(delbtns_obj[i]);
		}

		nfui_signal_emit(g_content_fixed, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(playback_obj);
		nfui_signal_emit(playback_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(export_obj);
		nfui_signal_emit(export_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(del_export_obj);
		nfui_signal_emit(del_export_obj, GDK_EXPOSE, TRUE);
	}
	else if(type == ART2_KIND_SNAP)
	{
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[0], "NAME");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[1], "START");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[2], "TIME");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[3], "SIZE");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[4], "SET BY");

		nfui_nfobject_set_size(titlelbl_obj[0], 649, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[1], 0, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[2], 329, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[3], 160, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[4], 394, ART2_LIST_LABEL_H);

		nfui_nfobject_show(titlelbl_obj[0]);
		nfui_nfobject_hide(titlelbl_obj[1]);
		nfui_nfobject_show(titlelbl_obj[2]);
		nfui_nfobject_show(titlelbl_obj[3]);
		nfui_nfobject_show(titlelbl_obj[4]);

		for (i = 0; i < MAX_ROW; i++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][0], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][1], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][2], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][3], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][4], "");

			nfui_nfobject_set_size(listlbl_obj[i][0], 649, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][1], 0, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][2], 329, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][3], 160, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][4], 394, ART2_LIST_LABEL_H);

			nfui_nfobject_show(listlbl_obj[i][0]);
			nfui_nfobject_hide(listlbl_obj[i][1]);
			nfui_nfobject_show(listlbl_obj[i][2]);
			nfui_nfobject_show(listlbl_obj[i][3]);
			nfui_nfobject_show(listlbl_obj[i][4]);

			nfui_nfobject_disable(delbtns_obj[i]);
		}

		nfui_signal_emit(g_content_fixed, GDK_EXPOSE, TRUE);

		nfui_nfobject_disable(playback_obj);
		nfui_signal_emit(playback_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(export_obj);
		nfui_signal_emit(export_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(del_export_obj);
		nfui_signal_emit(del_export_obj, GDK_EXPOSE, TRUE);
	}
	else if(type == ART2_KIND_PRESERVE)
	{
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[0], "NO.");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[1], "START");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[2], "END");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[3], "SIZE");
		nfui_nflabel_set_text((NFLABEL*)titlelbl_obj[4], "STATUS");

		nfui_nfobject_set_size(titlelbl_obj[0], 318, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[1], 329, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[2], 329, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[3], 160, ART2_LIST_LABEL_H);
		nfui_nfobject_set_size(titlelbl_obj[4], 394, ART2_LIST_LABEL_H);

		nfui_nfobject_show(titlelbl_obj[0]);
		nfui_nfobject_show(titlelbl_obj[1]);
		nfui_nfobject_show(titlelbl_obj[2]);
		nfui_nfobject_show(titlelbl_obj[3]);
		nfui_nfobject_show(titlelbl_obj[4]);

		for (i = 0; i < MAX_ROW; i++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][0], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][1], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][2], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][3], "");
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i][4], "");

			nfui_nfobject_set_size(listlbl_obj[i][0], 318, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][1], 329, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][2], 329, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][3], 160, ART2_LIST_LABEL_H);
			nfui_nfobject_set_size(listlbl_obj[i][4], 394, ART2_LIST_LABEL_H);

			nfui_nfobject_show(listlbl_obj[i][0]);
			nfui_nfobject_show(listlbl_obj[i][1]);
			nfui_nfobject_show(listlbl_obj[i][2]);
			nfui_nfobject_show(listlbl_obj[i][3]);
			nfui_nfobject_show(listlbl_obj[i][4]);

			nfui_nfobject_disable(delbtns_obj[i]);
		}

		nfui_signal_emit(g_content_fixed, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(playback_obj);
		nfui_signal_emit(playback_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_disable(export_obj);
		nfui_signal_emit(export_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_disable(del_export_obj);
		nfui_signal_emit(del_export_obj, GDK_EXPOSE, TRUE);
	}
}

static void arch_tab2_avi_load_page(gint current_page, gint total_page)
{
	gchar buf[5][100];
	gchar ampm[2][10];
	gint i, j, k;
	time_t t_start, t_end;
	GTimeVal tvtemp;
	int size_cnt = 0;
	float total_size = 0;
	char buf2[32];
	int start_row;

	memset(&tvtemp, 0x00, sizeof(GTimeVal));

	if (art2_avi_info) scm_free_avi_list(art2_avi_info);
	art2_avi_info = scm_new_avi_list(&avi_count);
	if (!art2_avi_info && avi_count > 0)
	{
		avi_count = 0;
	}

	if (current_page == 0) start_row = 0;
	else start_row = (current_page-1) * MAX_ROW;

	memset(art2_avi_id, 0xFF, sizeof(art2_avi_id));

	for(i = start_row, j=0 ; i < (start_row + MAX_ROW) ; i++, j++)
	{
		if(i >= avi_count) {
			for(k = 0 ; k < 5 ; k++) {
				memset(buf[k], 0x00, 100);
			}
			nfui_nfobject_disable(delbtns_obj[j]);
		}
		else {

			memset(buf, 0x00, sizeof(buf));
			memset(ampm, 0x00, sizeof(ampm));

			art2_avi_id[j] = art2_avi_info[i].arch_id;
			art2_avi_chmask[j] = art2_avi_info[i].channel_mask;

			g_sprintf(buf[0], "%s", art2_avi_info[i].tag);
			GUINT64_TO_GTIMEVAL(art2_avi_info[i].time_beg, tvtemp);
			t_start = tvtemp.tv_sec;
			GUINT64_TO_GTIMEVAL(art2_avi_info[i].time_end, tvtemp);
			t_end = tvtemp.tv_sec;

			dtf_get_local_datetime(t_start, buf[1]);
			dtf_get_local_datetime(t_end, buf[2]);

			total_size = (float)art2_avi_info[i].total_size;

			size_cnt = 0;
			while((guint)total_size > 1024)
			{
				++size_cnt;
				total_size/=1024;
			}

			if(size_cnt == 0) g_sprintf(buf2, "%.1f KB", total_size);
			else if(size_cnt == 1) g_sprintf(buf2, "%.1f MB", total_size);
			else if(size_cnt == 2) g_sprintf(buf2, "%.1f GB", total_size);
			else if(size_cnt == 3) g_sprintf(buf2, "%.1f TB", total_size);
			else g_sprintf(buf2, "ERROR");

			sprintf(buf[3], "%s", buf2);
			sprintf(buf[4], "%s", art2_avi_info[i].user);

			nfui_nfobject_enable(delbtns_obj[j]);
		}

		for(k = 0; k < 5; k++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[j][k], buf[k]);
			nfui_signal_emit(listlbl_obj[j][k], GDK_EXPOSE, TRUE);
		}

		nfui_signal_emit(delbtns_obj[j], GDK_EXPOSE, FALSE);
	}

	if (art2_avi_info) scm_free_avi_list(art2_avi_info);
	art2_avi_info = 0;

}

static void arch_tab2_snap_load_page(gint current_page, gint total_page)
{
	gchar buf[5][100];
	gchar ampm[2][10];
	gint i, j, k;
	time_t t_start, t_end;
	GTimeVal tvtemp;
	int size_cnt = 0;
	float total_size = 0;
	char buf2[32];
	int start_row;

	memset(&tvtemp, 0x00, sizeof(GTimeVal));

	if (art2_snap_info) scm_free_snap_list(art2_snap_info, bak_snap_count);
	art2_snap_info = scm_new_snap_list(&snap_count);
	bak_snap_count = snap_count;
	if (!art2_snap_info && snap_count > 0)
	{
		snap_count = 0;
		bak_snap_count = 0;
	}

	if (current_page == 0) start_row = 0;
	else start_row = (current_page-1) * MAX_ROW;

	memset(art2_snap_id, 0xFF, sizeof(art2_snap_id));

	for(i = start_row, j=0 ; i < (start_row + MAX_ROW) ; i++, j++)
	{
		if(i >= snap_count) {
			for(k = 0 ; k < 5 ; k++) {
				memset(buf[k], 0x00, 100);
			}
			nfui_nfobject_disable(delbtns_obj[j]);
		}
		else {

			memset(buf, 0x00, sizeof(buf));
			memset(ampm, 0x00, sizeof(ampm));

			art2_snap_id[j] = art2_snap_info[i].arch_id;

			g_sprintf(buf[0], "%s", art2_snap_info[i].tag);
			GUINT64_TO_GTIMEVAL(art2_snap_info[i].time_image, tvtemp);
			t_start = tvtemp.tv_sec;
			GUINT64_TO_GTIMEVAL(art2_snap_info[i].time_image, tvtemp);
			t_end = tvtemp.tv_sec;

			dtf_get_local_datetime(t_start, buf[1]);
			dtf_get_local_datetime(t_end, buf[2]);

			total_size = (float)art2_snap_info[i].total_size;

			size_cnt = 0;
			while((guint)total_size > 1024)
			{
				++size_cnt;
				total_size/=1024;
			}

			if(size_cnt == 0) g_sprintf(buf2, "%.1f BYTE", total_size);
			else if(size_cnt == 1) g_sprintf(buf2, "%.1f KB", total_size);
			else if(size_cnt == 2) g_sprintf(buf2, "%.1f MB", total_size);
			else if(size_cnt == 3) g_sprintf(buf2, "%.1f GB", total_size);
			else if(size_cnt == 4) g_sprintf(buf2, "%.1f TB", total_size);
			else g_sprintf(buf2, "ERROR");

			sprintf(buf[3], "%s", buf2);
			sprintf(buf[4], "%s", art2_snap_info[i].user);

			nfui_nfobject_enable(delbtns_obj[j]);
		}

		for(k = 0; k < 5; k++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[j][k], buf[k]);
			nfui_signal_emit(listlbl_obj[j][k], GDK_EXPOSE, TRUE);
		}

		nfui_signal_emit(delbtns_obj[j], GDK_EXPOSE, FALSE);
	}

	if (art2_snap_info) scm_free_snap_list(art2_snap_info, bak_snap_count);
	art2_snap_info = 0;
	bak_snap_count = snap_count;
}

static void arch_tab2_preserve_data_load_page(gint current_page, gint total_page)
{
	gchar buf[5][100];
	gchar ampm[2][10];
	gint i, j, k;
	guint status;
	time_t t_start, t_end;
	GTimeVal tvtemp;
	int size_cnt = 0;
	float total_size = 0;
	char buf2[32];
	int start_row;

	memset(&tvtemp, 0x00, sizeof(GTimeVal));

	if (art2_preserve_info) scm_free_preserve_list(art2_preserve_info);
	art2_preserve_info = scm_new_preserve_list(&preserve_cnt);
	if (!art2_preserve_info && preserve_cnt > 0)
	{
		preserve_cnt = 0;
	}

	if (current_page == 0) start_row = 0;
	else start_row = (current_page-1) * MAX_ROW;

	memset(art2_preserve_id, 0xFF, sizeof(art2_preserve_id));

	for(i = start_row, j=0 ; i < (start_row + MAX_ROW) ; i++, j++)
	{
		if(i >= preserve_cnt || (art2_preserve_info[i].start_timestamp == 0 && art2_preserve_info[i].end_timestamp == 0)) {
			for(k = 0 ; k < 5 ; k++) {
				memset(buf[k], 0x00, 100);
			}
			nfui_nfobject_disable(delbtns_obj[j]);
		}
		else {

			memset(buf, 0x00, sizeof(buf));
			memset(ampm, 0x00, sizeof(ampm));

			art2_preserve_id[j] = art2_preserve_info[i].no;

			g_sprintf(buf[0], "%d", art2_preserve_info[i].no+1);

			tvtemp.tv_sec = art2_preserve_info[i].start_timestamp;
			t_start = tvtemp.tv_sec;

			tvtemp.tv_sec = art2_preserve_info[i].end_timestamp;
			t_end = tvtemp.tv_sec;

			dtf_get_local_datetime(t_start, buf[1]);
			dtf_get_local_datetime(t_end, buf[2]);

			total_size = (float)art2_preserve_info[i].mb_cnt * 128;

			size_cnt = 0;
			while((guint)total_size > 1024)
			{
				++size_cnt;
				total_size/=1024;
			}

			if(size_cnt == 0) g_sprintf(buf2, "%.1f MB", total_size);
			else if(size_cnt == 1) g_sprintf(buf2, "%.1f GB", total_size);
			else if(size_cnt == 2) g_sprintf(buf2, "%.1f TB", total_size);
			else g_sprintf(buf2, "ERROR");

			sprintf(buf[3], "%s", buf2);

			status = art2_preserve_info[i].status;
			if(status == STATUS_NORMAL)
				sprintf(buf[4], "%s", "NORMAL");
			else if(status == STATUS_OVERWRITING)
				sprintf(buf[4], "%s", "OVERWRITING");
			else
				sprintf(buf[4], "%s", "UNKNOWN");

			nfui_nfobject_disable(delbtns_obj[j]);
		}

		for(k = 0; k < 5; k++)
		{
			nfui_nflabel_set_text((NFLABEL*)listlbl_obj[j][k], buf[k]);
			nfui_signal_emit(listlbl_obj[j][k], GDK_EXPOSE, TRUE);
		}

		nfui_signal_emit(delbtns_obj[j], GDK_EXPOSE, FALSE);
	}

	if (art2_preserve_info) scm_free_preserve_list(art2_preserve_info);
	art2_preserve_info = 0;
}

static int _play()
{
	gchar buf[100];
	GTimeVal begTime;
	GTimeVal endTime;
	gint strlen;
	gint kind_index;
	unsigned int sel_mask = 0;

	memset(&begTime, 0x00, sizeof(begTime));
	memset(&endTime, 0x00, sizeof(endTime));
	memset(buf, 0x00, sizeof(buf));

	strlen = nfui_nflabel_get_strlen(listlbl_obj[selected_row][0]);

	if (strlen == 0) return -1;

	kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

	if (kind_index == ART2_KIND_AVI) {
		NF_ARCH_AVI_INFO tmp_art2_avi_info;

		gint selected_id = art2_avi_id[selected_row];
		scm_get_avi_reserved_data(selected_id, 1, &tmp_art2_avi_info);

		begTime.tv_sec = tmp_art2_avi_info.time_beg / 1000000000;

		//SKSHIN, ugie's module cannot support discontinuous channel playing.
		//sel_mask = tmp_art2_avi_info.channel_mask & 0xffff;
		//
		sel_mask = var_get_ch_mask();
		vw_archiving_start_playback(OPEN_BY_ARCH_DF);
		vsm_playback_start( sel_mask,
							begTime,
							PLAYBACK_NORMAL);
	}
	else if(kind_index == ART2_KIND_SNAP) {

	}
	else if(kind_index == ART2_KIND_PRESERVE) {
		NF_DISK_PRESERVE_INFO tmp_art2_preserve_info;

		gint selected_id = art2_preserve_id[selected_row];

		scm_get_preserve_reserved_data(selected_id, 1, &tmp_art2_preserve_info);
		begTime.tv_sec = tmp_art2_preserve_info.start_timestamp;
		endTime.tv_sec = tmp_art2_preserve_info.end_timestamp;

		sel_mask = var_get_ch_mask();

		//nf_disk_preserve_pb_mode(1, tmp_art2_preserve_info.sess_id);
		vw_preserve_play_timelimit(begTime.tv_sec, endTime.tv_sec);
		vw_archiving_start_playback(OPEN_BY_PRESERVE_PLAY);
		vsm_preserve_playback_start( sel_mask,
							         begTime,
							         PLAYBACK_NORMAL,
									 tmp_art2_preserve_info.sess_id);
	}

	return 0;
}

static int _make_avi_burn_info(guint16 arch_id, guchar dev_id, BURN_INFO *burn_info)
{
	NF_ARCH_AVI_INFO tmp_info;

	scm_get_avi_reserved_data(arch_id, 1, &tmp_info);

	burn_info->type = NF_ARCH_TYPE_AVI;
	burn_info->arch_id = arch_id;
	burn_info->dev_id = dev_id;
	strcpy(burn_info->tag, tmp_info.tag);
	if (strlen(tmp_info.memo)) strcpy(burn_info->memo, tmp_info.memo);
	else strcpy(burn_info->memo, " ");
	return 0;
}

static int _make_snap_info(guint16 arch_id, SNAPSHOT_INFO_T *info)
{
	NF_ARCH_SNAP_INFO snap_info;

	scm_get_snap_reserved_data(arch_id, 1, &snap_info);

	info->ch = (gint)snap_info.channel;
	info->time = snap_info.time_image / 1000000000;;
	info->size = (gint)snap_info.total_size;

/*
	info->width = ntohs(*((short *)((char *)snap_info.pimage + 567)));
	info->height = ntohs(*((short *)((char *)snap_info.pimage + 565)));
*/
	if(!nf_jpeg_get_snapshot_size(snap_info.pimage, snap_info.total_size, &info->width, &info->height))
	{
		info->width = ntohs(*((short *)((char *)snap_info.pimage + 567)));
		info->height = ntohs(*((short *)((char *)snap_info.pimage + 565)));
	}

	info->buffer = (void *)snap_info.pimage;

	return 0;
}

static int _free_snap_info(SNAPSHOT_INFO_T *info)
{
	free(info->buffer);	// to free the memory allocated by arch_manager
	return 0;
}

static int _delete_avi_page(int page)
{
	int i;
	for (i = 0; i < MAX_ROW; ++i) {
		if (art2_avi_id[i] != 0xFFFF)
			scm_delete_avi(art2_avi_id[i]);
	}

	return 0;
}

static int _delete_snap_page(int page)
{
	int i;
	for (i = 0; i < MAX_ROW; ++i) {
		if (art2_snap_id[i] != 0xFFFF)
			scm_delete_snap(art2_snap_id[i]);
	}

	return 0;
}

static gboolean arch_tab2_post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			if (selected_row != -1)
				_change_outLine_list(selected_row, SELECT_OUTLINE_COLOR);
		}
		break;

		case GDK_DELETE: break;

		default : break;
	}

	return FALSE;

}

static gboolean pre_art2_listlbl_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint row, col;
	NFOBJECT* cur_focus;

	if (obj->kfocus == NFOBJECT_FOCUS)
	{
		if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
		{
			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			row = _find_index_matched_listOBJ(obj);
			col = _find_column_matched_listOBJ(obj, row);

			if ((kpid == KEYPAD_LEFT) && (col != 0))
			{
				cur_focus = nfui_get_cur_focus(obj);
				if (!cur_focus) return FALSE;
				nfui_set_key_focus(cur_focus, FALSE);
				nfui_set_key_focus(listlbl_obj[row][0], TRUE);
				nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
				nfui_signal_emit(listlbl_obj[row][0], GDK_EXPOSE, TRUE);
			}
			else if ((kpid == KEYPAD_RIGHT) && (col != 4))
			{
				cur_focus = nfui_get_cur_focus(obj);
				if (!cur_focus) return FALSE;
				nfui_set_key_focus(cur_focus, FALSE);
				nfui_set_key_focus(listlbl_obj[row][4], TRUE);
				nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
				nfui_signal_emit(listlbl_obj[row][4], GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_art2_listlbl_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint result;

	if (evt->type == GDK_EXPOSE)
	{
		i = _find_index_matched_listOBJ(obj);
		if (obj->kfocus == NFOBJECT_FOCUS)
			_draw_focusLine(i);
		else
			_erase_focusLine(i);
	}

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		i = _find_index_matched_listOBJ(obj);
		result = _draw_selectLine(i);

		if ((evt->type == GDK_2BUTTON_PRESS) || ((kpid == KEYPAD_ENTER) && (result == -1)))
		{
		    if (!ssm_check_access_auth(USR_AUTH_SEARCH)) return FALSE;
		    
			_play();
		}
	}
	else if(evt->type == GDK_ENTER_NOTIFY)
	{
		NFOBJECT* cur_focus;

		cur_focus = nfui_get_cur_focus(obj);
		if (!cur_focus) return FALSE;
		nfui_set_key_focus(cur_focus, FALSE);
		nfui_set_key_focus(obj, TRUE);
		nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}


static gboolean art2_delbtns_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type return_value;
	gint i, j, sel_del_item = -1;
	gint return_delitem_result;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint kind_index;
		kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

		for(i = 0; i < MAX_ROW; i++)
		{
			if(delbtns_obj[i] == obj)
			{
				if(kind_index == ART2_KIND_AVI) sel_del_item = art2_avi_id[i];
				if(kind_index == ART2_KIND_SNAP) sel_del_item = art2_snap_id[i];
				break;
			}
		}

		if(sel_del_item > -1)
		{
			return_value = nftool_mbox(g_curwnd, "CONFIRM", "The item you have selected will be released.\nDo you want to continue?",
						 NFTOOL_MB_OKCANCEL);

			if(return_value == NFTOOL_MB_OK)
			{
				if(kind_index == ART2_KIND_AVI)
				{
					return_delitem_result = scm_delete_avi(sel_del_item);
					if(return_delitem_result == 0)
					{
						avi_count = scm_get_avi_list_count();

						if(avi_count > 0)
						{
							total_page = (gint)ceil((gdouble)avi_count / MAX_ROW);
							cur_page = 1;
						}

						arch_tab2_avi_load_page(cur_page, total_page);
					}
				}
				else if(kind_index == ART2_KIND_SNAP)
				{
					return_delitem_result = scm_delete_snap(sel_del_item);
					if(return_delitem_result == 0)
					{
						snap_count = scm_get_snap_list_count();

						if(snap_count > 0)
						{
							total_page = (gint)ceil((gdouble)snap_count / MAX_ROW);
							cur_page = 1;
						}

						arch_tab2_snap_load_page(cur_page, total_page);
					}
				}

				_update_page_label(cur_page, total_page);
			}
			else
			{
				return FALSE;
			}

		}
	}

	return FALSE;

}

static gboolean art2_prev_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t start, end;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint kind_index;

		kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

		if(kind_index == ART2_KIND_AVI)
		{
			if(cur_page > 1)
			{
				cur_page--;

				avi_count = scm_get_avi_list_count();

				if(avi_count > 0)
				{
					_erase_selectLine();
					arch_tab2_avi_load_page(cur_page, total_page);
				}
			}

		}
		else if(kind_index == ART2_KIND_SNAP)
		{
			if(cur_page > 1)
			{
				cur_page--;
				snap_count = scm_get_snap_list_count();

				if(snap_count > 0)
				{
					_erase_selectLine();
					arch_tab2_snap_load_page(cur_page, total_page);
				}
			}
		}
		else if(kind_index == ART2_KIND_PRESERVE)
		{
			if(cur_page > 1)
			{
				cur_page--;
				preserve_cnt = scm_get_preserve_list_count();

				if(preserve_cnt > 0)
				{
					_erase_selectLine();
					arch_tab2_preserve_data_load_page(cur_page, total_page);
				}
			}
		}

		_update_page_label(cur_page, total_page);
	}

	return FALSE;
}

static gboolean art2_next_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t start, end;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(cur_page < total_page)
		{
			gint kind_index;

			kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

			if(kind_index == ART2_KIND_AVI)
			{
				cur_page++;

				avi_count = scm_get_avi_list_count();
				if(avi_count > 0)
				{
					_erase_selectLine();
					arch_tab2_avi_load_page(cur_page, total_page);
				}
			}
			else if(kind_index == ART2_KIND_SNAP)
			{
				cur_page++;
				snap_count = scm_get_snap_list_count();

				if(snap_count > 0)
				{
					_erase_selectLine();
					arch_tab2_snap_load_page(cur_page, total_page);
				}
			}
			else if(kind_index == ART2_KIND_PRESERVE)
			{
				cur_page++;
				preserve_cnt = scm_get_preserve_list_count();

				if(preserve_cnt > 0)
				{
					_erase_selectLine();
					arch_tab2_preserve_data_load_page(cur_page, total_page);
				}
			}

			_update_page_label(cur_page, total_page);
		}
	}

	return FALSE;
}

static gboolean
art2_export_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint kind_index;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (selected_row != -1)
		{
			gint strlen;

			strlen = nfui_nflabel_get_strlen(listlbl_obj[selected_row][0]);

			if (strlen)
			{
				kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

				if(kind_index == ART2_KIND_AVI)
				{
					BURN_INFO burn_info;
					guint video_mask = (guint)art2_avi_chmask[selected_row] & 0xffffffff;

                                        memset(&burn_info, 0x00, sizeof(BURN_INFO));
                    if (ssm_get_covert_mask() & video_mask)
                    {
                        nftool_mbox(g_curwnd, "WARNING", "You may not export data to an external device\nwhen covert channel settings are applied.", NFTOOL_MB_OK);
                        return FALSE;
                    }

					_make_avi_burn_info(art2_avi_id[selected_row], 0, &burn_info);
					VW_ArchExport_Open(g_curwnd, &burn_info);
				}
				else
				{
					SNAPSHOT_INFO_T info;

					_make_snap_info(art2_snap_id[selected_row], &info);

                    if (ssm_get_covert_mask() & (1 << info.ch))
                    {
                        nftool_mbox(g_curwnd, "WARNING", "You may not export data to an external device\nwhen covert channel settings are applied.", NFTOOL_MB_OK);
                        return FALSE;
                    }

					VW_Snapshot_Open(g_curwnd, &info, SS_MODE_BURN);
					_free_snap_info(&info);
				}
			}
		}
	}

	return FALSE;
}

static gboolean art2_deletepage_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type return_value;
	if(evt->type == GDK_BUTTON_RELEASE) {

		return_value = nftool_mbox(g_curwnd, "CONFIRM", "All of items in this page will be released.\nDo you want to continue?",
				NFTOOL_MB_OKCANCEL);

		if(return_value == NFTOOL_MB_OK)
		{
			gint kind_index;

			_erase_selectLine();
			kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

			if(kind_index == ART2_KIND_AVI)
			{
				_delete_avi_page(cur_page);
				avi_count = scm_get_avi_list_count();
				if ((cur_page - 1) * MAX_ROW >= avi_count) --cur_page;

				total_page = (gint)ceil((gdouble)avi_count / MAX_ROW);
				arch_tab2_avi_load_page(cur_page, total_page);
			}
			else if(kind_index == ART2_KIND_SNAP)
			{
				_delete_snap_page(cur_page);
				snap_count = scm_get_snap_list_count();
				if ((cur_page - 1) * MAX_ROW >= snap_count) --cur_page;

				total_page = (gint)ceil((gdouble)snap_count / MAX_ROW);
				arch_tab2_snap_load_page(cur_page, total_page);
			}

			_update_page_label(cur_page, total_page);
		}
		else
			return FALSE;
	}

	return FALSE;
}

static gboolean art2_playback_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (!ssm_check_access_auth(USR_AUTH_SEARCH)) return FALSE;

		if (selected_row != -1)
			_play();
	}

	return FALSE;
}

static gboolean art2_close_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		VW_Archiving_Close();

	return FALSE;
}

static gboolean art2_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case NFEVENT_COMBOBOX_CHANGED :
		{
			gint index, kind_index;

			_erase_selectLine();

			kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

			_change_list_form(kind_index);

			cur_page = 0;
			total_page = 0;

			if(kind_index == ART2_KIND_AVI)
			{
				if(avi_count > 0)
				{
					cur_page = 1;
					total_page = (gint)ceil((gdouble)avi_count / MAX_ROW);
					arch_tab2_avi_load_page(cur_page, total_page);
				}
			}
			else if(kind_index == ART2_KIND_SNAP)
			{
				if(snap_count > 0)
				{
					cur_page = 1;
					total_page = (gint)ceil((gdouble)snap_count / MAX_ROW);
					arch_tab2_snap_load_page(cur_page, total_page);
				}
			}
			else if(kind_index == ART2_KIND_PRESERVE)
			{
				if(preserve_cnt > 0)
				{
					cur_page = 1;
					total_page = (gint)ceil((gdouble)preserve_cnt / MAX_ROW);
					arch_tab2_preserve_data_load_page(cur_page, total_page);
				}
			}

			_update_page_label(cur_page, total_page);
		}
		break;
		case GDK_DELETE: break;
		default : break;
	}
	return FALSE;
}

static gboolean _update_list(void *data)
{
	gint kind_index;

	avi_count 	= scm_get_avi_list_count();
	snap_count 	= scm_get_snap_list_count();
	preserve_cnt = scm_get_preserve_list_count();
	kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

	cur_page = 1;

	if(kind_index == ART2_KIND_AVI)
	{
		if(avi_count > 0)
		{
			total_page = (gint)ceil((gdouble)avi_count / MAX_ROW);
			arch_tab2_avi_load_page(cur_page, total_page);
		}
	}
	else if(kind_index == ART2_KIND_SNAP)
	{
		if(snap_count > 0)
		{
			total_page = (gint)ceil((gdouble)snap_count / MAX_ROW);
			arch_tab2_snap_load_page(cur_page, total_page);
		}
	}
	else if(kind_index == ART2_KIND_PRESERVE)
	{
		if(preserve_cnt > 0)
		{
			total_page = (gint)ceil((gdouble)preserve_cnt / MAX_ROW);
			arch_tab2_preserve_data_load_page(cur_page, total_page);
		}
	}

	_update_page_label(cur_page, total_page);

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////
//
//
//

void vw_init_arch_tab2_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;

	gint xpos = ART2_LIST_START_X;
	gint ypos = ART2_LIST_START_Y;
	gint i, j;

	const gchar *listTitlelbl[] = {"NAME", "START", "END", "SIZE", "SET BY"};
	gint list_xpos[5] = {137, 457, 788, 1119, 1281};
	gint list_width[5] = {318, 329, 329, 160, 394};

	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

	guint size_w, size_h;
	GdkPixbuf *delitem_btn_img[NFOBJECT_STATE_COUNT];

	gchar *strReserved[3] =  {"AVI", "SNAPSHOT", "PRESERVE"};
	const gchar *page_btn[] = {"PREV.", "NEXT"};
	const gchar *below_btn[] = {"EXPORT", "PLAYBACK", "CLOSE"};

	nffont_type label_font;

	g_curwnd = nfui_nfobject_get_top(parent);

	art2_avi_info = 0;
	art2_snap_info = 0;
	art2_preserve_info = 0;
	focused_row = -1;
	selected_row = -1;

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	g_content_fixed = content_fixed;

	// RESERVED DATA VERIFY
	/*
	avi_count = scm_get_avi_list_count();
	*/

	for(j = 0; j < 5; j++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(listTitlelbl[j], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		if (j == 1 || j == 2 || j == 4)
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		else if (j == 3)
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
		else
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);

		nfui_nfobject_set_size(obj, list_width[j], ART2_LIST_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], ypos);
		titlelbl_obj[j] = obj;
	}

	if(nftool_cur_language_is_japanese())
		label_font = NFFONT_MEDIUM_THIN;
	else
		label_font = NFFONT_MEDIUM_SEMI;

	ypos += ART2_LIST_LABEL_H_INTERVAL;

	for(i = 0; i < MAX_ROW; i++ )
	{
		for( j = 0; j < 5; j++ )
		{
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(label_font));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
			nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);

			if( j == 1 || j == 2 || j == 4)
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			else if (j == 3)
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 5);
			else
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);

			nfui_nfobject_set_size(obj, list_width[j], ART2_LIST_LABEL_H);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
			nfui_nfobject_support_multi_lang(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, list_xpos[j], ypos);
			nfui_regi_pre_event_callback(obj, pre_art2_listlbl_btn_handler);
			nfui_regi_post_event_callback(obj, post_art2_listlbl_btn_handler);
			listlbl_obj[i][j] = obj;
		}

		ypos += ART2_LIST_LABEL_H_INTERVAL;
	}

	// DELETE ITEM BUTTONS
	delitem_btn_img[0] = nfui_get_image_from_file((IMG_BTN_N_DELETE), NULL);
	delitem_btn_img[1] = nfui_get_image_from_file((IMG_BTN_O_DELETE), NULL);
	delitem_btn_img[2] = nfui_get_image_from_file((IMG_BTN_P_DELETE), NULL);
	delitem_btn_img[3] = nfui_get_image_from_file((IMG_BTN_D_DELETE), NULL);
	nfui_get_pixbuf_size(delitem_btn_img[0], &size_w, &size_h);

	ypos = ART2_LIST_START_Y + 40;

	for(i = 0; i < MAX_ROW; i++ )
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		delbtns_obj[i] = obj;
		nfui_nfbutton_set_image(NF_BUTTON(obj), delitem_btn_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+1538+2 , ypos);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, art2_delbtns_btn_handler);

		ypos += ART2_LIST_LABEL_H_INTERVAL;
	}

	// SEARCH KEYWORD COMBOBOX
	obj = nfui_combobox_new(strReserved, ART2_KIND_NUM, 0);
	kindcombo_obj = obj;
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(obj, 200, 40);
#ifdef _SUPPORT_SNAPSHOT
	nfui_nfobject_show(obj);
#endif
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_LIST_START_X, ART2_PREV_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_combo_event_handler);

	// PREV, NEXT BTN
#if 0
	obj = nftool_normal_button_create_type3(page_btn[0], ART2_PAGE_BTN_SIZE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_PREV_BTN_X, ART2_PREV_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_prev_btn_handler);
#endif
	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_PREV_BTN_X, ART2_PREV_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_prev_btn_handler);


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1/12", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(121));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, ART2_PAGE_LABEL_W, ART2_PAGE_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_PAGE_LABEL_X, ART2_PAGE_LABEL_Y);
	pagelbl_obj = obj;

#if 0
	obj = nftool_normal_button_create_type3(page_btn[1], ART2_PAGE_BTN_SIZE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_NEXT_BTN_X, ART2_NEXT_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_next_btn_handler);
#endif
	nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_NEXT_BTN_X, ART2_NEXT_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_next_btn_handler);


	// DELETE BTN
	obj = nftool_normal_button_create_type3("RELEASE PAGE", ART2_DELETE_BTN_SIZE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, ART2_DELETE_BTN_X, ART2_DELETE_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_deletepage_btn_handler);
	del_export_obj = obj;

	// BELOW BTN
	obj = nftool_normal_button_create_type1(below_btn[0], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R3_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_export_btn_handler);
	export_obj = obj;

	obj = nftool_normal_button_create_type1(below_btn[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_playback_btn_handler);
	playback_obj = obj;

	obj = nftool_normal_button_create_type2(below_btn[2], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art2_close_btn_handler);

	// event handler
	nfui_regi_post_event_callback(content_fixed, arch_tab2_post_fixed_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_timeout_add(10, _update_list, 0);
}


gboolean vw_arch_tab2_out_handler()
{


	return FALSE;
}

gboolean vw_arch_tab2_in_handler()
{
	gint kind_index;

	kind_index = nfui_combobox_get_cur_index(NF_COMBOBOX(kindcombo_obj));

	if(kind_index == ART2_KIND_AVI)
	{
		avi_count = scm_get_avi_list_count();
		total_page = (gint)ceil((gdouble)avi_count / MAX_ROW);
		if (avi_count > 0) cur_page = 1;
		else cur_page = 0;
		arch_tab2_avi_load_page(cur_page, total_page);
	}
	else if(kind_index == ART2_KIND_SNAP)
	{
		snap_count = scm_get_snap_list_count();
		total_page = (gint)ceil((gdouble)snap_count / MAX_ROW);
		if (snap_count > 0) cur_page = 1;
		else cur_page = 0;
		arch_tab2_snap_load_page(cur_page, total_page);
	}
	else if(kind_index == ART2_KIND_PRESERVE)
	{
		preserve_cnt = scm_get_preserve_list_count();
		total_page = (gint)ceil((gdouble)preserve_cnt / MAX_ROW);
		if (preserve_cnt > 0) cur_page = 1;
		else cur_page = 0;
		arch_tab2_preserve_data_load_page(cur_page, total_page);
	}

	_update_page_label(cur_page, total_page);

	return FALSE;
}

