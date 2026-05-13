#include "vw_delete.h"

#include "../support/color.h"
#include "objects/nftab.h"
#include "vw_delete_tab1.h"
#include "objects/nfthumbnail.h"
#include "smt.h"

#include "vw_menu.h"

// [ Private Member Method and Function ]

typedef struct _DEL_T {
	LIVESTART_T		*lst;
	NFOBJECT 		*wnd;
} DEL_T;

static NFWINDOW *g_curwnd = 0;
static DEL_T idel;


static void _init_del_main(NFOBJECT *nftab)
{
    gint pos;

    pos = mcf.del.menu_pos[SYS_SUB7_OPERATION_DELETE];		
	if (pos != -1) {
	    vw_init_del_tab1_page(((NFTAB*)nftab)->page[pos]);
    }
}

static void _in_handler_del_main(gint page)
{
    if (page == mcf.del.menu_pos[SYS_SUB7_OPERATION_DELETE])
    {
        vw_del_tab1_in_handler();
    }
}

static void _out_handler_del_main(gint page)
{
    if (page == mcf.del.menu_pos[SYS_SUB7_OPERATION_DELETE])
    {
        vw_del_tab1_out_handler();
    }
}

//use
static int _wait_for_thumb_stop()
{
	nfui_nfthumbnail_image_close();

	while (1) {
		usleep(100000);
		if (!nfui_nfthumbnail_check_running()) break;
	}
	return 0;
}

//use
static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) 
	{		
		if (idel.lst) 
		{
			idel.lst->start();
			vsm_destroy_livestart_obj(idel.lst);
		}

		//scm_end_query();
		g_curwnd = 0;
		gtk_main_quit();
	}
	else if(evt->type == WND_PRE_CLOSE) {
		_wait_for_thumb_stop();
	}

	return FALSE;
}

static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type) {
	case NFEVENT_TAB_BEFORE_CHANGE:
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

        _out_handler_del_main(cur_page);
        _in_handler_del_main(new_page);
	default:
		break;
	}
	return FALSE;
}


static gboolean _wait_destory(gpointer data)
{
	if (nfui_nfthumbnail_check_running())
		return TRUE;
	
	nftool_destroy_setup_window(idel.wnd);

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    _wait_for_thumb_stop();

	return TRUE;
}

// [ Public Member Function and Method ]

void VW_Delete_Data_Open(NFWINDOW *parent, LIVESTART_T *lst, int from_pb)
{
	NFOBJECT *fixed, *nftab;

	if (lst == NULL)	
	{
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		g_assert(0);
	}
	
 	idel.lst = lst;
  
	idel.wnd = nftool_create_setup_window(parent, NFSETUP_WINDOW_DISK_DELETE, 0);
	g_curwnd = idel.wnd;
	nfui_nfwindow_set_title(idel.wnd, "ERASE VIDEO");
	nftab = nftool_get_nftab_from_setup_window(idel.wnd);

    _init_del_main(nftab);

	nfui_regi_post_event_callback(idel.wnd, post_main_wnd_event_handler);
	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);
	nfui_nfwindow_set_returnkey_proc(idel.wnd, returnkey_proc);

	nfui_nfobject_show(idel.wnd);
	nfui_make_key_hierarchy(idel.wnd);
	nfui_set_key_focus(nftab, TRUE);

	if (from_pb) vw_del_tab1_show();

	gtk_main();

}

void VW_Delete_Data_Close()
{ 
	nfui_nfthumbnail_image_close();

	g_timeout_add(100, _wait_destory, NULL);	
}

void vw_delete_show(LIVESTART_T *lst)
{
	vw_del_tab1_show();
	nfui_nfobject_show(idel.wnd);
	idel.lst = lst;
}

