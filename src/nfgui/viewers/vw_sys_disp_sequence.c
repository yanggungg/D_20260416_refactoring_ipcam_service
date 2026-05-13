#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_main.h"

#include "vw_sys_disp_sequence.h"
#include "vw_sys_disp_sequence_edit.h"
#include "vw_sys_disp_sequence_setup.h"
#include "scm.h"


//#define	NUM_DS_ROWS	16

#define	MAX_SEQ_DATA 			(4)
#define	DS_ROWS			(guint)(MAX_SEQ_DATA + 1)
#define	DS_COL_SPACE			(2)
#define	DS_ROW_SPACE			(1)

#define	DS_TABLE_LEFT			(28)
#define	DS_TABLE_TOP			(42)

#define	DS_LABEL_HEIGHT			(40)


enum {
	DS_ACTIVATION = 0,
	DS_LIST,
	DS_EDIT,
	DS_DELETE,
	DS_CREATED_BY,
	NUM_DS_COLUMNS
};


enum {
	DSB_CANCEL = 0,
	DSB_ADD,
	DSB_APPLY,
	DSB_CLOSE,
	DSB_BUTTONS
};

static SeqData seqdata[MAX_SEQ_DATA];
static SeqData org_seqdata[MAX_SEQ_DATA];
static guint seq_count;
static guint org_seq_count;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *active[MAX_SEQ_DATA];
static NFOBJECT *name[MAX_SEQ_DATA];
static NFOBJECT *created[MAX_SEQ_DATA];
static NFOBJECT *edit[MAX_SEQ_DATA];
static NFOBJECT *delete[MAX_SEQ_DATA];



static gboolean post_name_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		guint i, j;
		guint ret=0;

		if((evt->type == GDK_2BUTTON_PRESS) && (evt->button.button == MOUSE_RIGTH_BUTTON))
			return FALSE;

		for(i=0; i<MAX_SEQ_DATA; i++)
		{
			if(name[i] == obj)
				break;
		}

		if(i==MAX_SEQ_DATA)
			return FALSE;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
			seqdata[i].valid_mode = SEQ_MODE_VALID;
		else
			seqdata[i].valid_mode = SEQ_MODE_INVALID;

		if(i==0)	ret = SequenceEditDlg_Open(g_curwnd, SEQ_MODE_EDIT, &seqdata[i], FALSE);
		else		ret = SequenceEditDlg_Open(g_curwnd, SEQ_MODE_EDIT, &seqdata[i], TRUE);

		if(ret==SEQ_EDIT_RET_SAVE || ret==SEQ_EDIT_RET_MODIFY)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID) {
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], TRUE);
				
				for(j=0; j<seq_count; j++) {
					if(i != j) {
						seqdata[j].valid_mode = SEQ_MODE_INVALID;
						nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);
					}
				}

			}else if(seqdata[i].valid_mode == SEQ_MODE_INVALID) 
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], FALSE);

			nfui_nflabel_set_text((NFLABEL*)name[i], seqdata[i].title);

			nfui_signal_emit(active[i]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(edit[i], GDK_EXPOSE, FALSE);
//			nfui_signal_emit(created[i], GDK_EXPOSE, TRUE);
		}
		else if(ret==SEQ_EDIT_RET_DELETE)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID)
			{
				seqdata[0].valid_mode = SEQ_MODE_VALID;
				nfui_check_button_set_active((NFCHECKBUTTON*)active[0], TRUE);
			}

			for(j=i; j<seq_count; j++)
			{
				if(j+1 >= seq_count)
					break;

				g_memmove(&seqdata[j], &seqdata[j+1], sizeof(SeqData));
				seqdata[j+1].valid_mode = SEQ_MODE_DELETED;

				if(seqdata[j].valid_mode == SEQ_MODE_VALID)
					nfui_check_button_set_active((NFCHECKBUTTON*)active[j], TRUE);
				else if(seqdata[j].valid_mode == SEQ_MODE_INVALID)
					nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);

				nfui_nflabel_set_text((NFLABEL*)name[j], seqdata[j].title);
//				nfui_nflabel_set_text((NFLABEL*)created[j], seqdata[j].createdby);
			}

			seq_count--;

			memset(&seqdata[seq_count], 0, sizeof(SeqData));

			nfui_nfobject_hide(active[seq_count]->parent);
			nfui_nfobject_hide(name[seq_count]);
			nfui_nfobject_hide(edit[seq_count]);
			nfui_nfobject_hide(delete[seq_count]);
//			nfui_nfobject_hide(created[seq_count]);

			nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
			if(i==seq_count)
				nfui_set_key_focus(name[seq_count-1], TRUE);

			//nfui_signal_emit(name[0]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[0]->parent->parent, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_editbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, j;
		guint ret=0;

		for(i=0; i<MAX_SEQ_DATA; i++)
		{
			if(edit[i] == obj)
				break;
		}

		if(i==MAX_SEQ_DATA)
			return FALSE;

		if(i==0)	ret = SequenceEditDlg_Open(g_curwnd, SEQ_MODE_EDIT, &seqdata[i], FALSE);
		else		ret = SequenceEditDlg_Open(g_curwnd, SEQ_MODE_EDIT, &seqdata[i], TRUE);

		if(ret==SEQ_EDIT_RET_SAVE || ret==SEQ_EDIT_RET_MODIFY)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID) {
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], TRUE);
				
				for(j=0; j<seq_count; j++) {
					if(i != j) {
						seqdata[j].valid_mode = SEQ_MODE_INVALID;
						nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);
					}
				}

			}else if(seqdata[i].valid_mode == SEQ_MODE_INVALID) 
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], FALSE);

			nfui_nflabel_set_text((NFLABEL*)name[i], seqdata[i].title);

			nfui_signal_emit(active[i]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(edit[i], GDK_EXPOSE, FALSE);
//			nfui_signal_emit(created[i], GDK_EXPOSE, TRUE);
		}
		else if(ret==SEQ_EDIT_RET_DELETE)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID)
			{
				seqdata[0].valid_mode = SEQ_MODE_VALID;
				nfui_check_button_set_active((NFCHECKBUTTON*)active[0], TRUE);
			}

			for(j=i; j<seq_count; j++)
			{
				if(j+1 >= seq_count)
					break;

				g_memmove(&seqdata[j], &seqdata[j+1], sizeof(SeqData));
				seqdata[j+1].valid_mode = SEQ_MODE_DELETED;

				if(seqdata[j].valid_mode == SEQ_MODE_VALID)
					nfui_check_button_set_active((NFCHECKBUTTON*)active[j], TRUE);
				else if(seqdata[j].valid_mode == SEQ_MODE_INVALID)
					nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);

				nfui_nflabel_set_text((NFLABEL*)name[j], seqdata[j].title);
//				nfui_nflabel_set_text((NFLABEL*)created[j], seqdata[j].createdby);
			}

			seq_count--;

			memset(&seqdata[seq_count], 0, sizeof(SeqData));

			nfui_nfobject_hide(active[seq_count]->parent);
			nfui_nfobject_hide(name[seq_count]);
			nfui_nfobject_hide(edit[seq_count]);
			nfui_nfobject_hide(delete[seq_count]);
//			nfui_nfobject_hide(created[seq_count]);

			nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
			if(i==seq_count)
				nfui_set_key_focus(name[seq_count-1], TRUE);

			//nfui_signal_emit(name[0]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[0]->parent->parent, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_delbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, j;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		for(i=0; i<MAX_SEQ_DATA; i++)
		{
			if(delete[i] == obj)
				break;
		}

		if(i==MAX_SEQ_DATA)
			return FALSE;
		else if (i == 0)
		{
			seqdata[0].valid_mode = SEQ_MODE_VALID;
			nfui_check_button_set_active((NFCHECKBUTTON*)active[0], TRUE);

			for(j=1; j<seq_count; j++)
			{
				memset(&seqdata[j], 0, sizeof(SeqData));

				nfui_nfobject_hide(active[j]->parent);
				nfui_nfobject_hide(name[j]);
				nfui_nfobject_hide(edit[j]);
				nfui_nfobject_hide(delete[j]);
			}

			seq_count = 1;
			nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
			nfui_signal_emit(name[0]->parent->parent, GDK_EXPOSE, TRUE);

			return FALSE;
		}

		if(seqdata[i].valid_mode == SEQ_MODE_VALID)
		{
			seqdata[0].valid_mode = SEQ_MODE_VALID;
			nfui_check_button_set_active((NFCHECKBUTTON*)active[0], TRUE);
		}

		for(j=i; j<seq_count; j++)
		{
			if(j+1 >= seq_count)
				break;

			g_memmove(&seqdata[j], &seqdata[j+1], sizeof(SeqData));
			seqdata[j+1].valid_mode = SEQ_MODE_DELETED;

			if(seqdata[j].valid_mode == SEQ_MODE_VALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[j], TRUE);
			else if(seqdata[j].valid_mode == SEQ_MODE_INVALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);

			nfui_nflabel_set_text((NFLABEL*)name[j], seqdata[j].title);
		}

		seq_count--;

		memset(&seqdata[seq_count], 0, sizeof(SeqData));

		nfui_nfobject_hide(active[seq_count]->parent);
		nfui_nfobject_hide(name[seq_count]);
		nfui_nfobject_hide(edit[seq_count]);
		nfui_nfobject_hide(delete[seq_count]);

		nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
		if(i==seq_count)
			nfui_set_key_focus(name[seq_count-1], TRUE);

		nfui_signal_emit(name[0]->parent->parent, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		seq_count = org_seq_count;
		g_memmove(seqdata, org_seqdata, sizeof(SeqData)*MAX_SEQ_DATA);

		for(i=0; i<seq_count; i++)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], TRUE);
			else
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], FALSE);
			nfui_nflabel_set_text((NFLABEL*)name[i], seqdata[i].title);
//			nfui_nflabel_set_text((NFLABEL*)created[i], seqdata[i].createdby);

			nfui_nfobject_show(active[i]->parent);
			nfui_nfobject_show(name[i]);
			//nfui_nfobject_show(created[i]);
			nfui_nfobject_show(edit[i]);
			nfui_nfobject_show(delete[i]);
		}

		for(i=seq_count; i<MAX_SEQ_DATA; i++)
		{
			nfui_nfobject_hide(active[i]->parent);
			nfui_nfobject_hide(name[i]);
			nfui_nfobject_hide(edit[i]);
			nfui_nfobject_hide(delete[i]);
//			nfui_nfobject_hide(created[i]);
		}

		nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_signal_emit(name[0]->parent->parent, GDK_EXPOSE, TRUE);
	}
	
	return FALSE;
	
}

static gboolean post_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i, j;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint ret=0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		if(seq_count>=MAX_SEQ_DATA)
		{
			nftool_mbox(g_curwnd, "NOTICE", "Too many sequences.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret = SequenceEditDlg_Open(g_curwnd, SEQ_MODE_ADD, &seqdata[seq_count], TRUE);

		if(ret==SEQ_EDIT_RET_SAVE)
		{
			gchar *strName;
			NFOBJECT *top;

			if(seqdata[seq_count].valid_mode == SEQ_MODE_VALID)
			{
				nfui_check_button_set_active((NFCHECKBUTTON*)active[seq_count], TRUE);

				for(j=0; j<MAX_SEQ_DATA; j++) {
					if(seq_count != j) {
						seqdata[j].valid_mode = SEQ_MODE_INVALID;
						nfui_check_button_set_active((NFCHECKBUTTON*)active[j], FALSE);
					}
				}
			}
			else if(seqdata[seq_count].valid_mode == SEQ_MODE_INVALID)
			{
				nfui_check_button_set_active((NFCHECKBUTTON*)active[seq_count], FALSE);
			}

			top = nfui_nfobject_get_top(obj);
			strName = nfui_get_page_user(PGID_SETUP, top);
			g_sprintf(seqdata[seq_count].createdby, strName);

			nfui_nflabel_set_text((NFLABEL*)name[seq_count], seqdata[seq_count].title);
//			nfui_nflabel_set_text((NFLABEL*)created[seq_count], seqdata[seq_count].createdby);

			nfui_nfobject_show(active[seq_count]->parent);
			nfui_nfobject_show(active[seq_count]);
			nfui_nfobject_show(name[seq_count]);
			nfui_nfobject_show(edit[seq_count]);
			nfui_nfobject_show(delete[seq_count]);
			//nfui_nfobject_show(created[seq_count]);


			nfui_signal_emit(active[seq_count]->parent, GDK_EXPOSE, TRUE);
			nfui_signal_emit(name[seq_count], GDK_EXPOSE, TRUE);
			nfui_signal_emit(edit[seq_count], GDK_EXPOSE, FALSE);
			nfui_signal_emit(delete[seq_count], GDK_EXPOSE, TRUE);
//			nfui_signal_emit(created[seq_count], GDK_EXPOSE, TRUE);

			seq_count++;

			nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		for(i=0; i<seq_count; i++)
		{
			if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
				seqdata[i].valid_mode = SEQ_MODE_VALID;
			else
				seqdata[i].valid_mode = SEQ_MODE_INVALID;
			g_stpcpy(seqdata[i].title, nfui_nflabel_get_text((NFLABEL*)name[i]));
//			g_stpcpy(seqdata[i].createdby, nfui_nflabel_get_text((NFLABEL*)created[i]));
		}

		if(memcmp(org_seqdata, seqdata, sizeof(SeqData)*MAX_SEQ_DATA))
		{
			scm_put_log(CHANGE_DISP_MAINSEQ, 0, 0);

			g_memmove(org_seqdata, seqdata, sizeof(SeqData)*MAX_SEQ_DATA);
			org_seq_count = seq_count;

			DAL_set_seq_data_all(seqdata, seq_count);
			DAL_set_sequence_count(seq_count);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			sysdisp_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean 
post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
	
		Sequence_tab_out_handler();

		SystemSetupDisp_Destroy(obj);
	}

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



void init_DispSequence_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixedTemp;
	NFOBJECT *ntb;
	NFOBJECT *title_object[NUM_DS_COLUMNS];
	NFOBJECT *seq_btns[DSB_BUTTONS];

	const gchar *strTitle[] = {"ACTIVATION", "LIST", "CREATED BY"};
	guint width[NUM_DS_COLUMNS];
	guint btn_x, btn_y, btn_space;
	guint chk_w, chk_h;
	guint i;


	g_curwnd = nfui_nfobject_get_top(parent);

	width[0] = 205;
	width[1] = 477;
	width[2] = 100;
	width[3] = 204;

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	ntb = nfui_nftable_new(NUM_DS_COLUMNS, DS_ROWS, DS_COL_SPACE, DS_ROW_SPACE, width, DS_LABEL_HEIGHT);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put(content_fixed, ntb, DS_TABLE_LEFT, DS_TABLE_TOP);


	for(i=0; i<NUM_DS_COLUMNS-2; i++)
	{
		title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		
		nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
		if(i != 2) nfui_nfobject_show(title_object[i]);
		nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
	}

	memset(seqdata, 0x00, sizeof(SeqData)*MAX_SEQ_DATA);
	memset(org_seqdata, 0x00, sizeof(SeqData)*MAX_SEQ_DATA);

	seq_count = DAL_get_sequence_count();

	for(i=0; i<MAX_SEQ_DATA; i++)
	{
		fixedTemp = nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(fixedTemp, width[0], DS_LABEL_HEIGHT);

		active[i] = nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(active[i]), NFCHECK_TYPE_NORMAL);
		nfui_check_button_sensitive(NF_CHECKBUTTON(active[i]), FALSE);
		nfui_check_get_size(active[i], &chk_w, &chk_h);
		nfui_nfobject_use_focus(active[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixedTemp, active[i], (width[0]-chk_w)/2, (DS_LABEL_HEIGHT-chk_w)/2);

		if(i == 0)
			name[i] = nfui_nflabel_new_text_box("default", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		else
			name[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)name[i], NFTEXTBOX_TYPE_INPUT);
		nfui_nfobject_support_multi_lang((NFOBJECT*)name[i], FALSE);
		nfui_regi_post_event_callback(name[i], post_name_label_event_handler);

//		created[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
//		nfui_nflabel_set_skin_type((NFLABEL*)created[i], NFTEXTBOX_TYPE_INPUT);
//		nfui_nfobject_use_focus(created[i], NFOBJECT_FOCUS_OFF);

		edit[i] = nftool_normal_button_create_type3("EDIT", 100);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(edit[i]), NFALIGN_CENTER, 0);	
		nfui_regi_post_event_callback(edit[i], post_editbtn_event_handler);

		if (i == 0)	
		{
			delete[i] = nftool_normal_button_create_type3("DELETE ALL", 120);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(delete[i]), NFALIGN_CENTER, 0);	
			nfui_regi_post_event_callback(delete[i], post_delbtn_event_handler);
		}
		else
		{
			delete[i] = nftool_normal_button_create_type3("DELETE", 120);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(delete[i]), NFALIGN_CENTER, 0);	
			nfui_regi_post_event_callback(delete[i], post_delbtn_event_handler);
		}

		nfui_nftable_attach((NFTABLE*)ntb, fixedTemp, 0, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, name[i], 1, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, edit[i], 2, i+1);
		nfui_nftable_attach((NFTABLE*)ntb, delete[i], 3, i+1);
		//nfui_nftable_attach((NFTABLE*)ntb, created[i], 2, i+1);

		if(i<seq_count)
		{
			DAL_get_seq_data(&seqdata[i], i);

			if(seqdata[i].valid_mode == SEQ_MODE_VALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], TRUE);
			else if(seqdata[i].valid_mode == SEQ_MODE_INVALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], FALSE);

			nfui_nflabel_set_text((NFLABEL*)name[i], seqdata[i].title);
//			nfui_nflabel_set_text((NFLABEL*)created[i], seqdata[i].createdby);

			nfui_nfobject_show(fixedTemp);
			nfui_nfobject_show(active[i]);
			nfui_nfobject_show(name[i]);
			nfui_nfobject_show(edit[i]);
			nfui_nfobject_show(delete[i]);
//			nfui_nfobject_hide(created[i]);
		}
	}

	seq_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(seq_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(seq_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, seq_btns[0], MENU_V_BTN_R4_X, MENU_V_BTN_Y);

	seq_btns[1] = nftool_normal_button_create_type1("ADD", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(seq_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(seq_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, seq_btns[1], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
			
	seq_btns[2] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(seq_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(seq_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, seq_btns[2], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	seq_btns[3] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(seq_btns[3]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(seq_btns[3]);
	nfui_nffixed_put((NFFIXED*)parent, seq_btns[3], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(seq_btns[DSB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(seq_btns[DSB_ADD], post_addbutton_event_handler);
	nfui_regi_post_event_callback(seq_btns[DSB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(seq_btns[DSB_CLOSE], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_seqdata, seqdata, sizeof(SeqData)*MAX_SEQ_DATA);
	org_seq_count = seq_count;
}

gboolean Sequence_tab_out_handler()
{
	mb_type ret;
	int i;

	for(i=0; i<seq_count; i++)
	{
		if(nfui_check_button_get_active((NFCHECKBUTTON*)active[i]))
			seqdata[i].valid_mode = SEQ_MODE_VALID;
		else
			seqdata[i].valid_mode = SEQ_MODE_INVALID;
		
		g_stpcpy(seqdata[i].title, nfui_nflabel_get_text((NFLABEL*)name[i]));
//		g_stpcpy(seqdata[i].createdby, nfui_nflabel_get_text((NFLABEL*)created[i]));
	}

	if(!memcmp(org_seqdata, seqdata, sizeof(SeqData)*MAX_SEQ_DATA))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
							 NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_put_log(CHANGE_DISP_MAINSEQ, 0, 0);

		g_memmove(org_seqdata, seqdata, sizeof(SeqData)*MAX_SEQ_DATA);
		org_seq_count = seq_count;

		DAL_set_seq_data_all(seqdata, seq_count);
		DAL_set_sequence_count(seq_count);

		sysdisp_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(seqdata, org_seqdata, sizeof(SeqData)*MAX_SEQ_DATA);
		seq_count = org_seq_count;

		for(i=0; i<seq_count; i++)
		{
			if(seqdata[i].valid_mode == SEQ_MODE_VALID)
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], TRUE);
			else
				nfui_check_button_set_active((NFCHECKBUTTON*)active[i], FALSE);
			nfui_nflabel_set_text((NFLABEL*)name[i], seqdata[i].title);
//			nfui_nflabel_set_text((NFLABEL*)created[i], seqdata[i].createdby);

			nfui_nfobject_show(active[i]->parent);
			nfui_nfobject_show(name[i]);
			nfui_nfobject_show(edit[i]);
			nfui_nfobject_show(delete[i]);
			//nfui_nfobject_show(created[i]);
		}

		for(i=seq_count; i<MAX_SEQ_DATA; i++)
		{
			nfui_nfobject_hide(active[i]->parent);
			nfui_nfobject_hide(name[i]);
			nfui_nfobject_hide(edit[i]);
			nfui_nfobject_hide(delete[i]);
//			nfui_nfobject_hide(created[i]);
		}
	}
	return FALSE;
}






