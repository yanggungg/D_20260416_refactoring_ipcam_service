
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"
#include "objects/nfcombobox.h"

#include "vw_text_box_open.h"
#include "vw_pos_setting_popup.h"
#include "ix_mem.h"

#include "pos.h"
#include "nf_pos.h"


#define POPUP_SIZE_WID		    (1100)
#define POPUP_SIZE_HEI          (840)

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_serial_port;
static NFOBJECT *g_baud_obj;
static NFOBJECT *g_databit_obj;
static NFOBJECT *g_parity_obj;
static NFOBJECT *g_stopbit_obj;
static NFOBJECT *g_trans_start_obj[6];
static NFOBJECT *g_trans_end_obj[6];
static NFOBJECT *g_eof_line_obj[6];
static NFOBJECT *g_ignore_str_obj[6];
static NFOBJECT *g_protocol;

static PosDevData *g_org_posdev_data;
static PosDevData g_posdev_data[GUI_CHANNEL_CNT];

static gint g_cur_ch = 0;
static guint g_pos_test_tmr = 0;
static POSX_T *g_posx = 0;


static gint _is_conflict_port(gint ch)
{
    gint i;

    if (strcmp(g_posdev_data[ch].port, "NONE") == 0) return 0;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (i == ch) continue;
        if (strcmp(g_posdev_data[i].port, "NONE") == 0) continue;

        if (strcmp(g_posdev_data[i].port, g_posdev_data[ch].port) == 0) return 1;
    }

    return 0;
}

static gint _display_conflict_port()
{
    gint i, conflict;

    conflict = _is_conflict_port(g_cur_ch);
    
    if (conflict) nfui_nflabel_set_pango_font((NFLABEL*)g_serial_port, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_FF0000));
    else nfui_nflabel_set_pango_font((NFLABEL*)g_serial_port, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(231));

    return 0;
}

static gint _update_protocol_list(NFOBJECT *obj)
{
    gint i, cnt;
    gchar **protocol;
    
    cnt = nf_pos_get_protocol_cnt();
    if (!cnt) return -1;
    
    protocol = imalloc(sizeof(gchar*) * cnt);

    for (i = 0; i < cnt; i++)
    {
        protocol[i] = imalloc(sizeof(gchar)*MAX_POS_PROTO_STR_LENGTH);
    }

    nf_pos_get_protocol_list(protocol);

    for (i = 0; i < cnt; i++)
    {
        nfui_combobox_append_data((NFCOMBOBOX*)obj, protocol[i]);
        ifree(protocol[i]);
    }

    ifree(protocol);
    
    return 0;
}

static gint _update_charset_list(NFOBJECT *obj)
{
    gint i, cnt;
    gchar **charset;
    
    cnt = nf_pos_get_charset_cnt();    
    if (!cnt) return -1;

    charset = imalloc(sizeof(gchar*) * cnt);

    for (i = 0; i < cnt; i++)
    {
        charset[i] = imalloc(sizeof(gchar)*MAX_POS_CHARSET_STR_LENGTH);
    }

    nf_pos_get_charset_list(charset);

    for (i = 0; i < cnt; i++)
    {
        nfui_combobox_append_data((NFCOMBOBOX*)obj, charset[i]);
        ifree(charset[i]);
    }

    ifree(charset);
    
    return 0;
}

static gint _update_capable_serial_info(gchar *str)
{
    NF_POS_SERIAL_OPTION option;

    memset(&option, 0x00, sizeof(NF_POS_SERIAL_OPTION));
    nf_pos_get_serial_option(str, &option);    

    if (option.baudrate) nfui_nfobject_enable(g_baud_obj);
    else nfui_nfobject_disable(g_baud_obj);
    
    if (option.databit) nfui_nfobject_enable(g_databit_obj);
    else nfui_nfobject_disable(g_databit_obj);

    if (option.parity) nfui_nfobject_enable(g_parity_obj);
    else nfui_nfobject_disable(g_parity_obj);

    if (option.stopbit) nfui_nfobject_enable(g_stopbit_obj);
    else nfui_nfobject_disable(g_stopbit_obj);
    
    return 0;
}

static gint _get_hexString_from_db(NFOBJECT *obj[], gchar *str)
{
    gint i = 0;
    gchar tmp[256];
    gchar *p;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

    if (!strlen(str)) return -1;

    strcpy(tmp, str);    
    p = strtok_r(tmp, ",", &pnext);
    pbuf = pnext;
    nfui_nflabel_set_text((NFLABEL*)obj[i++], p);

    while(p = strtok_r(pbuf, ",", &pnext)) {       
        nfui_nflabel_set_text((NFLABEL*)obj[i++], p);
        pbuf = pnext;
    }

    return 0;
}

static gint _get_db_from_hexString(NFOBJECT *obj[], gchar *str)
{
    gint i;

    for (i = 0; i < 6; i++)
    {   
        if (strcmp(nfui_nflabel_get_text((NFLABEL*)obj[i]), "-") != 0)
        {       
            if (strlen(str)) strcat(str, ",");        
            strcat(str, nfui_nflabel_get_text((NFLABEL*)obj[i]));
        }
    }

    return 0;
}

static gint _get_hexVal_from_hexString(gint *hex_val, gchar *str)
{
    gint i = 0;
    gchar tmp[256];
    gchar *p;
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

    for (i = 0; i < 8; i++)
    {
        hex_val[i] = -1;
    }
   
    if (strcmp(str, "-") != 0)
    {
        i = 0;
        
        strcpy(tmp, str);
        p = strtok_r(tmp, "|", &pnext);
        pbuf = pnext;
        sscanf(p, "0x%02X", &hex_val[i++]);

        while(p = strtok_r(pbuf, "|", &pnext)) {       
            sscanf(p, "0x%02X", &hex_val[i++]);
            pbuf = pnext;
        }
    }

    return 0;
}

static gint _get_hexString_from_hexVal(gint *hex_val, gchar *str)
{
    gint i;
    gchar tmp[8];

    for (i = 0; i < 8; i++)
    {   
        if (hex_val[i] != -1) 
        {
            memset(tmp, 0x00, sizeof(tmp));
            g_sprintf(tmp, "0x%02X", hex_val[i]);
        
            if (strlen(str)) strcat(str, "|");        
            strcat(str, tmp);
        }
    }

    if (!strlen(str)) strcpy(str, "-");

    return 0;
}

static gboolean _send_pos_text_test(gpointer data)
{
    gint i, send_cnt = 16;
    gchar *strBuf;

    if (!g_posx) return TRUE;

    posx_get_pos_table(g_posx, send_cnt);
    if (g_posx->count < send_cnt) send_cnt = g_posx->count;
    
    for (i = 0; i < send_cnt; i++)
    {
        strBuf = imalloc(strlen(g_posx->data[i].text)+1);
        strcpy(strBuf, g_posx->data[i].text);
        evt_send_to_local(INFY_TEXTBOX_MODIFY_LINE, i, 1, strBuf);
    }

    return TRUE;
}

static void _destory_posx()
{
    posx_clear_test_log();

    if (g_posx) posx_destroy(g_posx);
    g_posx = 0;
}

static void _pos_text_callback(char *text)
{
//    g_message("%s, %d, text:%s", __FUNCTION__, __LINE__, text);

    if (g_posx) posx_put_test_log(g_cur_ch, text);
}

static gint _start_pos_test()
{
    NF_POS_DB_T test_data;

    memset(&test_data, 0x00, sizeof(NF_POS_DB_T));
    test_data.act = 1;
    test_data.type = g_posdev_data[g_cur_ch].type;
    strcpy(test_data.port_alias, g_posdev_data[g_cur_ch].port);
    test_data.protocol = g_posdev_data[g_cur_ch].protocol;
    test_data.char_set = g_posdev_data[g_cur_ch].char_set;
    test_data.baud = g_posdev_data[g_cur_ch].baud;
    test_data.parity = g_posdev_data[g_cur_ch].parity;
    test_data.databit = g_posdev_data[g_cur_ch].databit;
    test_data.stopbit = g_posdev_data[g_cur_ch].stopbit;    
    strcpy(test_data.transact_start, g_posdev_data[g_cur_ch].transact_start);
    strcpy(test_data.transact_end, g_posdev_data[g_cur_ch].transact_end);
    strcpy(test_data.endofline, g_posdev_data[g_cur_ch].endofline);
    strcpy(test_data.ignore, g_posdev_data[g_cur_ch].ignore);
    
    if (!nf_pos_test_start(g_cur_ch, &test_data, _pos_text_callback)) return 0; 

    g_posx = posx_create(32);
    g_pos_test_tmr = g_timeout_add_full(G_PRIORITY_DEFAULT, 400, _send_pos_text_test, 0, _destory_posx);
    
    return 1;
}

static gint _open_text_box()
{
    TEXT_BOX_INFO box_info;
    TEXT_BOX_CONTROL box_control;

    memset(&box_info, 0x00, sizeof(TEXT_BOX_INFO));
    memset(&box_control, 0x00, sizeof(TEXT_BOX_CONTROL));

    box_info.win_w = 800;
    box_info.win_h = 800;    
    box_info.win_x = (1920-box_info.win_w)/2;
    box_info.win_y = (1080-box_info.win_h)/2;
    box_info.type = TB_TYPE_LABEL;
    
    box_control.modify = INFY_TEXTBOX_MODIFY_LINE;
    
    VW_TextBox_Open(g_curwnd, "POS TEST", &box_info, &box_control, 0);

    return 1;
}

static gint _exit_pos_test()
{
    nf_pos_test_end();

    if (g_pos_test_tmr) g_source_remove(g_pos_test_tmr);
    g_pos_test_tmr = 0;
    
    return 1;
}

static gint _set_obj_by_protocol(gint expose)
{
    gchar *protocol;
    gint i;

    protocol = nfui_combobox_get_value((NFCOMBOBOX*)g_protocol);

    if (strcmp(protocol, "EPSON") == 0)
    {
        for (i = 0; i < 6; i++)
        {
            nfui_nfobject_disable(g_trans_start_obj[i]);
            nfui_nfobject_disable(g_trans_end_obj[i]);
            nfui_nfobject_disable(g_eof_line_obj[i]);
            nfui_nfobject_disable(g_ignore_str_obj[i]);
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            nfui_nfobject_enable(g_trans_start_obj[i]);
            nfui_nfobject_enable(g_trans_end_obj[i]);
            nfui_nfobject_enable(g_eof_line_obj[i]);
            nfui_nfobject_enable(g_ignore_str_obj[i]);
        }
    }

    if (expose)
    {
        for (i = 0; i < 6; i++)
        {
            nfui_signal_emit(g_trans_start_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_trans_end_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_eof_line_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ignore_str_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    return 0;
}

static gboolean post_serial_port_auto_select_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;


	}

	return FALSE;
}

static gboolean post_serial_port_edit_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gchar org_port[128];
        gchar new_port[128];
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(org_port, 0x00, sizeof(org_port));
        memset(new_port, 0x00, sizeof(new_port));

        strcpy(org_port, nfui_nflabel_get_text((NFLABEL*)g_serial_port));
        strcpy(new_port, org_port);
        vw_edit_pos_serial_port_open(g_curwnd, new_port);

        if (strcmp(org_port, new_port) == 0) return FALSE;

        strcpy(g_posdev_data[g_cur_ch].port, new_port);
        _display_conflict_port();
        
        nfui_nflabel_set_text((NFLABEL*)g_serial_port, new_port);
        nfui_signal_emit(g_serial_port, GDK_EXPOSE, TRUE);

        _update_capable_serial_info(new_port);
        nfui_signal_emit(g_baud_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_databit_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_parity_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_stopbit_obj, GDK_EXPOSE, TRUE);        
	}

	return FALSE;
}

static gboolean post_serial_protocol_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        g_posdev_data[g_cur_ch].protocol = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        _set_obj_by_protocol(1);
    }

    return FALSE;
}

static gboolean post_serial_charset_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        g_posdev_data[g_cur_ch].char_set = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_serial_baudrate_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gchar *baudrate;

        baudrate = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        g_posdev_data[g_cur_ch].baud = atoi(baudrate);
    }

    return FALSE;
}

static gboolean post_serial_parity_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        g_posdev_data[g_cur_ch].parity = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_serial_databit_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gchar *databit;

        databit = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        g_posdev_data[g_cur_ch].databit = atoi(databit);
    }

    return FALSE;
}

static gboolean post_serial_stopbit_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gchar *stopbit;

        stopbit = nfui_combobox_get_value((NFCOMBOBOX*)obj);
        g_posdev_data[g_cur_ch].stopbit = atoi(stopbit);
    }

    return FALSE;
}

static gboolean post_serial_transact_start_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
        gint hex_val[8];
        gchar *strBuf;
        gchar tmp[64];

        strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
        _get_hexVal_from_hexString(hex_val, strBuf);
        
        vw_vkey_hex_open(g_curwnd, 300, 300, hex_val, 8);

        memset(tmp, 0x00, sizeof(tmp));
        _get_hexString_from_hexVal(hex_val, tmp);
        nfui_nflabel_set_text((NFLABEL*)obj, tmp);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(g_posdev_data[g_cur_ch].transact_start, 0x00, sizeof(g_posdev_data[g_cur_ch].transact_start));
        _get_db_from_hexString(g_trans_start_obj, g_posdev_data[g_cur_ch].transact_start);
	}
	
	return FALSE;
}

static gboolean post_serial_transact_end_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
        gint hex_val[8];
        gchar *strBuf;
        gchar tmp[64];

        strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
        _get_hexVal_from_hexString(hex_val, strBuf);
        
        vw_vkey_hex_open(g_curwnd, 300, 300, hex_val, 8);

        memset(tmp, 0x00, sizeof(tmp));
        _get_hexString_from_hexVal(hex_val, tmp);
        nfui_nflabel_set_text((NFLABEL*)obj, tmp);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(g_posdev_data[g_cur_ch].transact_end, 0x00, sizeof(g_posdev_data[g_cur_ch].transact_end));
        _get_db_from_hexString(g_trans_end_obj, g_posdev_data[g_cur_ch].transact_end);        
	}
	
	return FALSE;
}

static gboolean post_serial_eof_line_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
        gint hex_val[8];
        gchar *strBuf;
        gchar tmp[64];

        strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
        _get_hexVal_from_hexString(hex_val, strBuf);
        
        vw_vkey_hex_open(g_curwnd, 300, 300, hex_val, 8);

        memset(tmp, 0x00, sizeof(tmp));
        _get_hexString_from_hexVal(hex_val, tmp);
        nfui_nflabel_set_text((NFLABEL*)obj, tmp);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(g_posdev_data[g_cur_ch].endofline, 0x00, sizeof(g_posdev_data[g_cur_ch].endofline));
        _get_db_from_hexString(g_eof_line_obj, g_posdev_data[g_cur_ch].endofline);        
	}

	return FALSE;
}

static gboolean post_serial_ignore_str_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_2BUTTON_PRESS)
	{
        gint hex_val[8];
        gchar *strBuf;
        gchar tmp[64];

        strBuf = nfui_nflabel_get_text((NFLABEL*)obj);
        _get_hexVal_from_hexString(hex_val, strBuf);
        
        vw_vkey_hex_open(g_curwnd, 300, 300, hex_val, 8);

        memset(tmp, 0x00, sizeof(tmp));
        _get_hexString_from_hexVal(hex_val, tmp);
        nfui_nflabel_set_text((NFLABEL*)obj, tmp);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(g_posdev_data[g_cur_ch].ignore, 0x00, sizeof(g_posdev_data[g_cur_ch].ignore));
        _get_db_from_hexString(g_ignore_str_obj, g_posdev_data[g_cur_ch].ignore);                
	}
	
	return FALSE;
}

static gboolean post_copy_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        VW_Copy_Setting_posdev(g_curwnd, g_cur_ch, g_posdev_data);

        _display_conflict_port();
        
        nfui_nflabel_set_text((NFLABEL*)g_serial_port, g_posdev_data[g_cur_ch].port);
        nfui_signal_emit(g_serial_port, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_test_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gint is_start;
        guint ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        ret = nftool_mbox(g_curwnd, "NOTICE", "POS logs for all channels will not be recorded during testing.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;

        is_start = _start_pos_test();
        
        if (!is_start) 
        {
            nftool_mbox(g_curwnd, "WARNING", "POS TEST FAIL", NFTOOL_MB_OK);
            return FALSE;
        }

        _open_text_box();   // gtk_main block
        _exit_pos_test();
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (memcmp(g_org_posdev_data, g_posdev_data, sizeof(PosDevData) * GUI_CHANNEL_CNT) != 0)
        {
            g_memmove(g_org_posdev_data, g_posdev_data, sizeof(PosDevData) * GUI_CHANNEL_CNT);
        }

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_pos_setting_popup_open(NFWINDOW *parent, gint cur_ch, PosDevData *posdev)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *serial_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	gint size_w, size_h;
	gint radio_size_w, radio_size_h;
    gint pos_x, pos_y;
	gint i;

	guint width[2];
    gchar buf[32];

	const gchar *strDev[] = {"SERIAL PORT", "NETWORK"};
	

    g_cur_ch = cur_ch;

    g_org_posdev_data = posdev;
    
	memset(g_posdev_data, 0x00, sizeof(PosDevData) * GUI_CHANNEL_CNT);
    g_memmove(g_posdev_data, g_org_posdev_data, sizeof(PosDevData) * GUI_CHANNEL_CNT);
    

	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "POS SETTING", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = 20;
    pos_y = 64;

	nfui_get_pixbuf_size(radio_img[0], &radio_size_w, &radio_size_h);

	for (i = 0; i < 1/*2*/; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)radio_size_w, (guint)radio_size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(40-radio_size_h)/2);

		if (i == 0) 
		{
		    slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }
		else 
		{
		    nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		    nfui_nfobject_disable(obj);
        }

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDev[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 160, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+radio_size_w+8, pos_y);		

		pos_x += 240;
    }

// SERIAL FIXED

    pos_x = 20;
    pos_y += 56;

	serial_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(serial_fixed, POPUP_SIZE_WID-40, POPUP_SIZE_HEI-pos_y-56);
	nfui_nfobject_modify_bg(serial_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(serial_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, serial_fixed, pos_x, pos_y);

    pos_x = 0;
    pos_y = 0;

	obj = nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 260, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(posdev[cur_ch].port, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put(serial_fixed, obj, pos_x+262, pos_y);    
    g_serial_port = obj;

    _display_conflict_port();

	obj = nftool_normal_button_create_popup_type1("EDIT", 200);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+516, pos_y);
	nfui_regi_post_event_callback(obj, post_serial_port_edit_event_handler);

	obj = nftool_normal_button_create_popup_type1("AUTO-SELECT", 200);
//	nfui_nfobject_show(obj);
    nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+720, pos_y);
	nfui_regi_post_event_callback(obj, post_serial_port_auto_select_event_handler);

    pos_y += 42;

	obj = nfui_nflabel_new_with_pango_font("* help message", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 900, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

	nfui_nflabel_set_text((NFLABEL *) obj, "* To add a port USB to Serial Device connection is required." );

    pos_y += 56;

	width[0] = 260;
	width[1] = 220;    

	ntb = nfui_nftable_new(2, 3, 2, 1, width, 40);	
	nfui_nfobject_show(ntb);
	nfui_nffixed_put(serial_fixed, ntb, pos_x, pos_y);

	obj = nfui_nflabel_new_with_pango_font("PROTOCOL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
	nfui_regi_post_event_callback(obj, post_serial_protocol_event_handler);
    g_protocol = obj;
    
	_update_protocol_list(obj);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdev[cur_ch].protocol);

	obj = nfui_nflabel_new_with_pango_font("BAUDRATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 1);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 1);
	nfui_regi_post_event_callback(obj, post_serial_baudrate_event_handler);
    g_baud_obj = obj;
	
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "2400");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "4800");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "9600");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "19200");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "38400");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "57600");    
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "115200");

    memset(buf, 0x00, sizeof(buf));
    g_sprintf(buf, "%d", posdev[cur_ch].baud);
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, buf);
    
	obj = nfui_nflabel_new_with_pango_font("DATA BIT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 2);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 2);
	nfui_regi_post_event_callback(obj, post_serial_databit_event_handler);
    g_databit_obj = obj;
    
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "5");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "6");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "7");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "8");

    memset(buf, 0x00, sizeof(buf));
    g_sprintf(buf, "%d", posdev[cur_ch].databit);
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, buf);

	ntb = nfui_nftable_new(2, 3, 2, 1, width, 40);	
	nfui_nfobject_show(ntb);
	nfui_nffixed_put(serial_fixed, ntb, pos_x+550, pos_y);

	obj = nfui_nflabel_new_with_pango_font("CHARACTER SET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
	nfui_regi_post_event_callback(obj, post_serial_charset_event_handler);
	
	_update_charset_list(obj);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdev[cur_ch].char_set);
	
	obj = nfui_nflabel_new_with_pango_font("PARITY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 1);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 1);
	nfui_regi_post_event_callback(obj, post_serial_parity_event_handler);
    g_parity_obj = obj;

    nfui_combobox_append_data((NFCOMBOBOX*)obj, "NONE");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "ODD");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "EVEN");    
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdev[cur_ch].parity);

	obj = nfui_nflabel_new_with_pango_font("STOP BIT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 2);

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);		
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 2);
	nfui_regi_post_event_callback(obj, post_serial_stopbit_event_handler);
    g_stopbit_obj = obj;
    
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "1");
    nfui_combobox_append_data((NFCOMBOBOX*)obj, "2");

    memset(buf, 0x00, sizeof(buf));
    g_sprintf(buf, "%d", posdev[cur_ch].stopbit);
    nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, buf);

    _update_capable_serial_info(posdev[cur_ch].port);
    
    pos_y += 140;

	obj = nfui_nflabel_new_with_pango_font("TRANSACTION START", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 260, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_transact_start_event_handler);
    	g_trans_start_obj[i] = obj;
    }

    pos_y += 41;

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_transact_start_event_handler);
    	g_trans_start_obj[3+i] = obj;
    }

    _get_hexString_from_db(g_trans_start_obj, posdev[cur_ch].transact_start);

    pos_y += 44;

	obj = nfui_nflabel_new_with_pango_font("TRANSACTION END", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 260, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_transact_end_event_handler);
    	g_trans_end_obj[i] = obj;    	
    }

    pos_y += 41;

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_transact_end_event_handler);
    	g_trans_end_obj[3+i] = obj;    	
    }

    _get_hexString_from_db(g_trans_end_obj, posdev[cur_ch].transact_end);

    pos_y += 44;

	obj = nfui_nflabel_new_with_pango_font("LINE DELIMITER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 260, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_eof_line_event_handler);
    	g_eof_line_obj[i] = obj;    	
    }

    pos_y += 41;

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_eof_line_event_handler);
    	g_eof_line_obj[3+i] = obj;    	
    }

    _get_hexString_from_db(g_eof_line_obj, posdev[cur_ch].endofline);

    pos_y += 44;

	obj = nfui_nflabel_new_with_pango_font("IGNORE STRING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 260, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(serial_fixed, obj, pos_x, pos_y);

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_ignore_str_event_handler);
    	g_ignore_str_obj[i] = obj;    	
    }

    pos_y += 41;

    for (i = 0; i < 3; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 264, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, pos_x+262+(i*266), pos_y);
    	nfui_regi_post_event_callback(obj, post_serial_ignore_str_event_handler);
    	g_ignore_str_obj[3+i] = obj;    	
    }

    _get_hexString_from_db(g_ignore_str_obj, posdev[cur_ch].ignore);

    pos_y += 50;

	obj = nftool_normal_button_create_popup_type1("HELP", 140);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)serial_fixed, obj, serial_fixed->width-140, pos_y);

// MAIN FIXED BUTTONS

	obj = nftool_normal_button_create_type1("COPY SETTINGS TO", 300);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_copy_button_event_handler);

	obj = nftool_normal_button_create_type1("TEST", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-20-174-184*2, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_test_button_event_handler);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-20-174-184, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-20-174, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_POS_SETTING_POPUP, main_wnd, nfui_get_last_user());

	_set_obj_by_protocol(0);

	gtk_main();

	nfui_page_close(PGID_POS_SETTING_POPUP, main_wnd);
	
	return 0;
}
