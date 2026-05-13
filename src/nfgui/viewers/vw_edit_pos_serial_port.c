
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
#include "objects/nfcombobox.h"

#include "nf_pos.h" 

#include "ix_mem.h"
#include "vw_edit_pos_serial_port.h"


#define WIN_SIZE_WID				(350)
#define WIN_SIZE_HEI				(200)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_port_obj;

static gchar *g_select_port;

static gint g_pos_count;
static NF_POS_SERIAL_INFO g_pos_info[NF_POS_SERIAL_MAX];


static gint _get_pos_serial_port()
{
	memset(g_pos_info, 0x00, sizeof(NF_POS_SERIAL_INFO) * NF_POS_SERIAL_MAX);
    nf_pos_get_serial_info(g_pos_info, &g_pos_count);
    return 0;
}

static gint _update_pos_serial_port()
{
    gint i;

    nfui_combobox_remove_all((NFCOMBOBOX*)g_port_obj);

    for (i = 0; i < g_pos_count; i++)
    {
        nfui_combobox_append_data((NFCOMBOBOX*)g_port_obj, &g_pos_info[i]);

        if (strcmp(&g_pos_info[i], g_select_port) == 0)
        {
            nfui_combobox_set_data_no_expose((NFCOMBOBOX*)g_port_obj, g_select_port);
        }
    }        

    return 0;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        strcpy(g_select_port, nfui_combobox_get_value((NFCOMBOBOX*)g_port_obj));
	
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
	if (evt->type == INFY_DETECT_POSDEV_NOTIFY)
    {
        _get_pos_serial_port();
        _update_pos_serial_port();
        nfui_signal_emit(g_port_obj, GDK_EXPOSE, TRUE);
    }
	else if (evt->type == GDK_DELETE)
	{
        uxm_unreg_imsg_event(obj, INFY_DETECT_POSDEV_NOTIFY);
	
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_edit_pos_serial_port_open(NFWINDOW *parent, gchar *select_port)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	   
	gint pos_x, pos_y;	
    gint i, count;


    g_select_port = select_port;
    

	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, (1920-WIN_SIZE_WID)/2, (1080-WIN_SIZE_HEI)/2, WIN_SIZE_WID, WIN_SIZE_HEI, "EDIT PORT", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_set_size(obj, WIN_SIZE_WID-40, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 80);
    g_port_obj = obj;

    _get_pos_serial_port();
    _update_pos_serial_port();
    
	obj = nftool_normal_button_create_type1("OK", 140);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_SIZE_WID/2-145, WIN_SIZE_HEI-60);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 140);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, WIN_SIZE_WID/2+5, WIN_SIZE_HEI-60);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);

    uxm_reg_imsg_event(main_wnd, INFY_DETECT_POSDEV_NOTIFY);
	    
	gtk_main();
	
	return 0;
}

