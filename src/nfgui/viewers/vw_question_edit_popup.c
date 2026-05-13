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
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "vw_question_edit_popup.h"
#include "vw_init_userinfo.h"
#include "ix_mem.h"
#include "uxm.h"


#define WIN_SIZE_WID				(840)
#define WIN_SIZE_WID_EDIT			(1115)
#define WIN_SIZE_HEI				(330)

#define LABEL_SIZE_WID			    (200)
#define LABEL_SIZE_HEI              (40)
#define QUE_SIZE_WID		        (520)
#define ANS_SIZE_WID		        (260)

enum
{
	EDIT = 0,
	ADD
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_answer_obj[QNA_COUNT];
static NFOBJECT *g_answer_confirm_obj[QNA_COUNT];
static NFOBJECT *g_question_obj[QNA_COUNT];

UserManageData *g_userdata;
gint g_edit_type;

static gint g_user_idx = 0;
static gint user_data_max = 0;
static gint *g_question[QNA_COUNT];
static gchar *g_answer[QNA_COUNT];

static gint g_retVal = 0;

static gboolean post_input_answer_event(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *email;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        gint ret = 0;
		gint inputMode;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}
		gchar *answer = NULL;
		
		answer = nfui_nflabel_get_text((NFLABEL*)obj);
		if(g_edit_type == EDIT) {
			inputMode = VKEY_PASSWORD;
		} else {
			inputMode = VKEY_NORMAL;
		}

		strTemp = VirtualKey_Open(g_curwnd, answer, x, y, 64, inputMode); 

		if (strTemp) 
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gchar *autherStr;	
		gchar *id;
		gchar *email = 0;
		gchar new_password[64];	
		gchar answer_tmp[QNA_COUNT][65];
		guint opt = 0;
		gint isExist = 0;
		gint i, j;

		GTimeVal last_temp;
		mb_type retType;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (nfui_combobox_get_cur_index(g_question_obj[0]) == nfui_combobox_get_cur_index(g_question_obj[1])
            || nfui_combobox_get_cur_index(g_question_obj[1]) == nfui_combobox_get_cur_index(g_question_obj[2])
            || nfui_combobox_get_cur_index(g_question_obj[2]) == nfui_combobox_get_cur_index(g_question_obj[0])) 
        {
            nftool_mbox(g_curwnd, "WARNING", "Duplicate question. Please select another question.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[0]))
        || !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[1]))
        || !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[2])))
        {
            nftool_mbox(g_curwnd, "WARNING", "Please input answers for each question.", NFTOOL_MB_OK);
            return FALSE;
        }

		if(g_edit_type == EDIT) {
			if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[0]))
			|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[1]))
			|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[2])))
			{
				nftool_mbox(g_curwnd, "WARNING", "Enter a confirmed answer for each question.", NFTOOL_MB_OK);
				return FALSE;
			}
			
			if(strcmp(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[0]), nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[0])) != 0 
			|| strcmp(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[1]), nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[1])) != 0
			|| strcmp(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[2]), nfui_nflabel_get_text((NFLABEL*)g_answer_confirm_obj[2])) != 0) {
				nftool_mbox(g_curwnd, "WARNING", "Answer and confirmed answer do not match.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

        for (i = 0; i < QNA_COUNT; i++){
            *g_question[i] = nfui_combobox_get_cur_index(g_question_obj[i]);
            strcpy(g_answer[i], nfui_nflabel_get_text((NFLABEL*)g_answer_obj[i]));
        }

        g_retVal = 1;
        topwin = nfui_nfobject_get_top(obj);			
            nfui_nfobject_destroy(topwin);
	}
	
	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	int i;

	if (event->type == GDK_BUTTON_RELEASE) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		if(g_edit_type == EDIT) {
			for (i = 0; i < QNA_COUNT; i++){
				*g_question[i] = g_userdata[g_user_idx].question[i];
				strcpy(g_answer[i], g_userdata[g_user_idx].answer[i]);
			}
		}

        g_retVal = 0;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_question_edit_popup_open(NFWINDOW *parent, gint *question[], gchar *answer[], UserManageData *userdata, gint user_idx, guint user_count)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *lbTemp;
	NFOBJECT *obj;

	gint btn_w, btn_h;
	gint pos_x, pos_y;	
	guint i, j;
	gint win_size_wid;

    for(i=0; i<QNA_COUNT; i++) {
        g_question[i] = question[i];
        g_answer[i] = answer[i];
    }

	g_userdata = userdata;

	g_retVal = 0;

	g_user_idx = user_idx;

	if(g_user_idx >= 0) g_edit_type = EDIT;
	else g_edit_type = ADD;

// <---- WINDOW
	if(g_edit_type == EDIT)
		win_size_wid = WIN_SIZE_WID_EDIT;
	else
		win_size_wid = WIN_SIZE_WID;
	
	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, (DISPLAY_ACTIVE_WIDTH-win_size_wid)/2, (DISPLAY_ACTIVE_HEIGHT-WIN_SIZE_HEI)/2, win_size_wid, WIN_SIZE_HEI, "SET SECURITY QUESTIONS", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_nfobject_show(main_fixed);
	g_curwnd = main_wnd;

    pos_x = 15;
    pos_y = 65;

	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("QUESTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, LABEL_SIZE_WID, LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, pos_x+8, pos_y);

	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANSWER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, LABEL_SIZE_WID, LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, pos_x+8+QUE_SIZE_WID+10, pos_y);
	
	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONFIRM ANSWER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, LABEL_SIZE_WID, LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	if(g_edit_type == EDIT) nfui_nfobject_show(lbTemp);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, pos_x+8+QUE_SIZE_WID+ANS_SIZE_WID+20, pos_y);

    for(i = 0; i < QNA_COUNT; i++) {
        pos_y += 44;
        obj = nfui_combobox_new(QUESTIONS, 11, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
        nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
		
		if(g_edit_type == EDIT) {
			if(*g_question[i] != NULL){
				nfui_combobox_set_index(NF_COMBOBOX(obj), *g_question[i]);
			} else if(userdata[g_user_idx].question[i] != NULL){
				nfui_combobox_set_index(NF_COMBOBOX(obj), userdata[g_user_idx].question[i]);
			}
		} else {
			if(*g_question[i] != NULL){
				nfui_combobox_set_index(NF_COMBOBOX(obj), *g_question[i]);
			}
		}
		
        nfui_nfobject_set_size(obj, QUE_SIZE_WID, LABEL_SIZE_HEI);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+8, pos_y);
        g_question_obj[i] = obj;

		if(g_edit_type == EDIT) {
			if(strlen(g_answer[i])){
				obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_answer[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			}
			else if(strlen(userdata[g_user_idx].answer[i])) {
				obj = (NFOBJECT*)nfui_nflabel_new_text_box(userdata[g_user_idx].answer[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			}
			else {
				obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			}
		} else {
			if(strlen(g_answer[i])){
				obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_answer[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			} else {
				obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			}
		}

        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
        nfui_nfobject_set_size(obj, ANS_SIZE_WID, LABEL_SIZE_HEI);
		if(g_edit_type == EDIT) {
			nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
		}
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+8+QUE_SIZE_WID+10, pos_y);
        nfui_regi_post_event_callback(obj, post_input_answer_event);
        g_answer_obj[i] = obj;

		if(g_edit_type == EDIT) {
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_set_size(obj, ANS_SIZE_WID, LABEL_SIZE_HEI);
			nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+8+QUE_SIZE_WID+ANS_SIZE_WID+20, pos_y);
			nfui_regi_post_event_callback(obj, post_input_answer_event);
			g_answer_confirm_obj[i] = obj;
		}
    }

	obj = nftool_normal_button_create_type1("OK", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_size_wid/2-4-160 , WIN_SIZE_HEI-64);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);  

	obj = nftool_normal_button_create_type1("CANCEL", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_size_wid/2+4, WIN_SIZE_HEI-64);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);  

	nfui_nfobject_show(main_wnd);

	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);
	
	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return g_retVal;
}
