#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"

#include "modules/ssm.h"

#include "vw_snapshot.h"
#include "vw_arch_export.h"
#include "dtf.h"

#define SNAPSHOT_WIN_SIZE_W         (628)
#define SNAPSHOT_WIN_SIZE_H         (618)
#define SNAPSHOT_WIN_POS_X          ((DISPLAY_ACTIVE_WIDTH-SNAPSHOT_WIN_SIZE_W)/2)
#define SNAPSHOT_WIN_POS_Y          ((DISPLAY_ACTIVE_HEIGHT-SNAPSHOT_WIN_SIZE_H)/2)

#define MIN_SCALE_SIZE(a, b)                   (((a) < (b)) ? (a) : (b))

typedef enum _QUERY_REASON_E {
    EXPORT      = 0,
    RESERVE     = 1,
} QUERY_REASON_E;

static SNAPSHOT_INFO_T *g_snapInfo;
static NFWINDOW *g_curwnd = 0;
static QUERY_REASON_E g_qry_reason = 0;


static void _get_snap_param(NF_ARCH_SNAP_PARAM *param)
{
    gchar titleBuf[STRING_SIZE_CAMTITLE];
    gchar strBuf[STRING_SIZE_64];
    struct tm *stime_info;

    memset(titleBuf, 0x00, sizeof(titleBuf));
    memset(strBuf, 0x00, sizeof(strBuf));

    g_sprintf(titleBuf, "CAM %02d", g_snapInfo->ch+1);

    stime_info = NFLOCALTIME(&g_snapInfo->time);
    strftime(strBuf, sizeof(strBuf), "%Y%m%d%H%M", stime_info);

    g_sprintf(param->tag, "%s_%s", titleBuf, strBuf);
    param->snap_time.tv_sec = g_snapInfo->time;
    param->ch = g_snapInfo->ch;
    param->image_size = g_snapInfo->size;
    param->image = g_snapInfo->buffer;

    ssm_get_cur_id(param->user);
}

static void _query_data()
{
    NF_ARCH_SNAP_PARAM snap_param;
    NF_ARCH_SNAP_INFO snap_info;

    memset(&snap_param, 0x00, sizeof(NF_ARCH_SNAP_PARAM));
    memset(&snap_info, 0x00, sizeof(NF_ARCH_SNAP_INFO));

    _get_snap_param(&snap_param);

    if(scm_start_snapshot_query(&snap_param, &snap_info) < 0) {
        nftool_mbox(g_curwnd, "ERROR", "The operation occured an error.", NFTOOL_MB_OK);
        return FALSE;
    }
}

static void _export_data(guint16 arch_id)
{
    BURN_INFO burn_info;

    memset(&burn_info, 0x00, sizeof(BURN_INFO));

    burn_info.type = NF_ARCH_TYPE_SNAP;
    burn_info.arch_id = arch_id;
    VW_ArchExport_Open(g_curwnd, &burn_info);
}

static void _reserve_data()
{
    int ret;
    ret = scm_reserve_snap_info();
    switch(ret) {
        case -1:
            nftool_mbox(g_curwnd, "ERROR", "Internal error.", NFTOOL_MB_OK);
            break;

        case -11:                                                   // List is full
            nftool_mbox(g_curwnd, "ERROR", "It's unable to reserve\nbecause the archiving list is full.", NFTOOL_MB_OK);
            break;

        case -12:                                                   // queried data is already added
            nftool_mbox(g_curwnd, "ERROR", "Current queried data is\nalready added.", NFTOOL_MB_OK);
            break;

        default:
            break;
    }


    if(ret >= 0)
        nftool_mbox_auto(g_curwnd, 2, "NOTICE", "Data is reserved successfully.");
}

static gboolean post_export_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean use_dl = 0;
	SecurityData secdata;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        DAL_get_use_double_login(&use_dl);

        if (use_dl && !ssm_is_admin())
        {
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
        }
        else
        {
            DAL_get_security_data(&secdata);
            if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
        }
	
        g_qry_reason = EXPORT;
        _query_data();
    }

    return FALSE;
}

static gboolean post_reserve_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean use_dl = 0;
	SecurityData secdata;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        DAL_get_use_double_login(&use_dl);

        if (use_dl && !ssm_is_admin())
        {
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
        }
        else
        {
            DAL_get_security_data(&secdata);
            if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
        }

        g_qry_reason = RESERVE;
        _query_data();
    }

    return FALSE;
}


static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

#if defined(_IPXVE_MODEL_UX)
static void _prevent_transparent(GdkPixbuf *snap)
{
    int width, height, rowstride, i, j;
    guchar *p;

    width = gdk_pixbuf_get_width(snap);
    height = gdk_pixbuf_get_height(snap);
    rowstride = gdk_pixbuf_get_rowstride(snap);
    p = gdk_pixbuf_get_pixels(snap);

    if( (rowstride / width) != 3 )
        return;

    if(!p)
        return;

    for(i=0; i < height; i++){
        for(j=0; j < width; j++){
            if(((p[0] >> 3) == 0x0) && ((p[1] >> 3) == 0x0) && ((p[2] >> 3) == 0x0)){
                p[0] = p[0] | (1 << 3);
                p[1] = p[1] | (1 << 3);
                p[2] = p[2] | (1 << 3);
            }

            p = p + 3;
        }
    }
}
#endif

static gboolean post_snapshot_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
            {
                GInputStream *stream;
                GdkPixbuf *snap_pixbuf = NULL;

                drawable = nfui_nfobject_get_window(obj);
                gc = nfui_nfobject_get_gc(obj);

                nfui_nfobject_get_size(obj, &size_w, &size_h);
                pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
                nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

                // box
                gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(413));
                gdk_draw_rectangle(drawable,
                                gc,
                                FALSE,
                                20,
                                206,
                                588,
                                330);

				#if 0	// Test Code
					{
						FILE *fp = NULL;
						fp = fopen( "/NFDVR/test1.jpg", "wb") ;
						if(fp){
							fwrite(g_snapInfo->buffer, 1, g_snapInfo->size, fp);
							fclose(fp);
						}
					}
				#endif
                stream = g_memory_input_stream_new_from_data(g_snapInfo->buffer, g_snapInfo->size, NULL);
				#if 0	// Test Code
					{
						FILE *fp = NULL;
						fp = fopen( "/NFDVR/test2.jpg", "wb") ;
						if(fp){
							fwrite(g_snapInfo->buffer, 1, g_snapInfo->size, fp);
							fclose(fp);
						}
					}
					{
						GError       *error=NULL;
    	            	snap_pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, 588, 330, FALSE, NULL, error);
						g_printf("%s line%d code(%d) message(%s) \n", __FUNCTION__, __LINE__, error->code, error->message);
					}
				#endif
                gint scaled_w, scaled_h;
                gdouble ratio_w, ratio_h, ratio;
                gint x, y;

                ratio_w = (gdouble)588 / (gdouble)g_snapInfo->width;
                ratio_h = (gdouble)330 / (gdouble)g_snapInfo->height;

                ratio = MIN_SCALE_SIZE(ratio_w, ratio_h);

                scaled_w = (gint)(g_snapInfo->width * ratio);
                scaled_h = (gint)(g_snapInfo->height * ratio);
                if (scaled_w > scaled_h) {
                        scaled_w = 588;
                } else {
                        scaled_h = 330;
                }

                snap_pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, scaled_w, scaled_h, FALSE, NULL, NULL);
                x = 20 + (588 - scaled_w) / 2;
                y = 206 + (330 - scaled_h) / 2;
                gdk_draw_pixbuf(drawable,
                    gc,
                    snap_pixbuf,
                    0, 0,
                    x, y,
                    scaled_w, scaled_h,
                    GDK_RGB_DITHER_NONE,
                    0, 0);

#if defined(_IPXVE_MODEL_UX)
                _prevent_transparent(snap_pixbuf);
#endif
                g_object_unref(snap_pixbuf);
                g_object_unref(stream);

                nfui_nfobject_gc_unref(gc);
            }
            break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static gboolean post_snapshot_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		uxm_unreg_imsg_event(obj, INFY_SNAP_QUERY_SUCCESS);
		gtk_main_quit();
	}
	else if (evt->type == INFY_SNAP_QUERY_SUCCESS)
	{
		CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
		NF_ARCH_SNAP_INFO *snap_info = (NF_ARCH_SNAP_INFO*)pmsg->data;

		switch (g_qry_reason) {
			case 0:
				_export_data(snap_info->arch_id);
				scm_end_snap_query();
				break;
			case 1:
				_reserve_data();
				scm_end_snap_query();
				break;
		}
	}

	return FALSE;
}

gboolean VW_Snapshot_Open(NFWINDOW *parent, SNAPSHOT_INFO_T *info, SNAPSHOT_MODE_E mode)
{
    NFOBJECT *stWin;
    NFOBJECT *stFixed;
    NFOBJECT *obj;
    NFOBJECT *export_btn;

    gchar strBuf[STRING_SIZE_64];

    g_snapInfo = info;


    /* window */
    stWin = (NFOBJECT*)nfui_nfwindow_new(parent, SNAPSHOT_WIN_POS_X, SNAPSHOT_WIN_POS_Y, SNAPSHOT_WIN_SIZE_W, SNAPSHOT_WIN_SIZE_H);
    g_curwnd = stWin;
    nfui_regi_post_event_callback(stWin, post_snapshot_win_event_handler);


    /* fixed */
    stFixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(stFixed, SNAPSHOT_WIN_SIZE_W, SNAPSHOT_WIN_SIZE_H);
    nfui_regi_post_event_callback(stFixed, post_snapshot_fixed_event_cb);
    nfui_nfobject_show(stFixed);


    /* title */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SNAPSHOT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 620, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 4, 4);


    /* camera title */
    memset(strBuf, 0x00, sizeof(strBuf));
    //DAL_get_camera_title(strBuf, info->ch);
    var_get_camtitle(strBuf, info->ch);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 60);

    /* timestamp */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMESTAMP : ", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 130, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
    nfui_nfobject_use_tooltip(obj, FALSE);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 100);

    dtf_get_local_datetime(info->time, strBuf);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 160, 100);


    /* size */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SIZE : ", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 60, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
    nfui_nfobject_use_tooltip(obj, FALSE);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 140);


    memset(strBuf, 0x00, sizeof(strBuf));
    if(g_snapInfo->width < 0 || g_snapInfo->height < 0)
        g_sprintf(strBuf, "%s", "N/A");
    else
        g_sprintf(strBuf, "%d X %d", g_snapInfo->width, g_snapInfo->height);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 80, 140);


    /* preview */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 180);



    /* button */
    obj = nftool_normal_button_create_popup_type2("EXPORT", 192);
    nfui_regi_post_event_callback(obj, post_export_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 550);
    export_btn = obj;

    if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
        nfui_nfobject_disable(obj);

    obj = nftool_normal_button_create_popup_type2("RESERVE", 192);
    nfui_regi_post_event_callback(obj, post_reserve_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 218, 550);

    if ((!ssm_check_access_auth(USR_AUTH_ARCHIVE)) || (mode == SS_MODE_BURN))
        nfui_nfobject_disable(obj);

    obj = nftool_normal_button_create_popup_type2("CLOSE", 192);
    nfui_regi_post_event_callback(obj, post_close_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 416, 550);

    nfui_nfwindow_add((NFWINDOW*)stWin, stFixed);
    nfui_run_main_event_handler(stWin);
    nfui_nfobject_show(stWin);

    nfui_make_key_hierarchy((NFWINDOW*)stWin);
    nfui_set_key_focus(export_btn, TRUE);

    uxm_reg_imsg_event(stWin, INFY_SNAP_QUERY_SUCCESS);
	uxm_monitor_on_imsg_event(stWin, INFY_SNAP_QUERY_SUCCESS);

    nfui_page_open(PGID_SNAPSHOT_POPUP, stWin, nfui_get_last_user());

    gtk_main();

    nfui_page_close(PGID_SNAPSHOT_POPUP, stWin);

    return TRUE;
}
