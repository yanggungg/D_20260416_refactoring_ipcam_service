#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"

#include "vw_sys_user_main.h"
#include "vw_sys_user_manage.h"
#include "vw_sys_user_add.h"
#include "vw_channel_mask_ctrl.h"
#include "scm.h"
#include "ssm.h"



#define	UM_COLUMNS					(6)
#define	UM_ROWS			            (15)
#define UM_PAGE_TOT_CNT             (8)

#define	UM_LABLE_LEFT				(18)
#define	UM_LABLE_TOP				(10)

#define UM_CONTENT_TABLE_X          (UM_LABLE_LEFT)
#define UM_CONTENT_TABLE_Y          (UM_LABLE_TOP + UM_ROW_SPACE + UM_LABLE1_HEIGHT)

#define	UM_LABLE0_WIDTH				(90)
#define	UM_LABLE0_HEIGHT			(40)

#define	UM_LABLE1_WIDTH				(210)
#define	UM_LABLE1_HEIGHT			(40)

#define	UM_LABLE2_WIDTH				(211)

#define	UM_LABLE2_HEIGHT			(UM_LABLE1_HEIGHT)

#define	UM_LABLE3_WIDTH				(UM_LABLE2_WIDTH + 50)
#define	UM_LABLE3_HEIGHT			(UM_LABLE2_HEIGHT)

#define	UM_LABLE4_WIDTH				(190)
#define	UM_LABLE4_HEIGHT			(UM_LABLE3_HEIGHT)

#define	UM_CHECK_BUTTON_LEFT		(70)
#define	UM_CHECK_BUTTON_TOP			(10)

#define	UM_COL_SPACE				(2)
#define	UM_ROW_SPACE				(1)
#define	UM_LABLE_HEIGHT_WITH_SPACE	(41)


enum {
	UMB_CANCEL = 0,
	UMB_ADD,
	UMB_APPLY,
	UMB_CLOSE,
	UMB_BUTTONS
};

typedef enum {
    INFO_NORMAL = 0,
    INFO_EDIT,
    INFO_ADD,
    INFO_DEL,
}CHANGED_USER_INFO_E;

typedef struct changed_user_info_t
{
    gchar id[STRING_SIZE_16];
    CHANGED_USER_INFO_E res;
}CHANGED_USER_INFO_T;

static UserManageData userdata[MAX_USER_COUNT];
static UserManageData org_userdata[MAX_USER_COUNT];
static UserManageData pre_userdata[MAX_USER_COUNT];

static CHANGED_USER_INFO_T cuidata[MAX_USER_COUNT];

static guint user_count;
static guint org_user_count;
static guint user_idx;
static guint g_cur_page = 0;
static guint g_max_page = 0;
static gint g_pre_page = -1;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_content_fixed;
static NFOBJECT *g_fixedTemp[UM_ROWS];

static NFOBJECT *userid[UM_ROWS];
static NFOBJECT *group[UM_ROWS];
static NFOBJECT *covert[UM_ROWS];
static NFOBJECT *email[UM_ROWS];
static NFOBJECT *email_noti[UM_ROWS];
static NFOBJECT *edit_btn[UM_ROWS];
static NFOBJECT *del_btn[UM_ROWS];
static NFOBJECT *g_no[UM_ROWS];
static NFOBJECT *g_radio[UM_PAGE_TOT_CNT];



static gint _init_data_changed_user_info()
{
    memset(cuidata, 0x00, sizeof(CHANGED_USER_INFO_T)*MAX_USER_COUNT);

    return 0;
}

static gint _set_log_changed_user_info()
{
    gint i;

    for (i = 0; i < MAX_USER_COUNT; i++)
    {
        if (cuidata[i].res == INFO_EDIT) {
        }
        else if (cuidata[i].res == INFO_ADD) {
            scm_put_log_t(CHANGE_USER_ADD, 0, 0, cuidata[i].id);
        }
        else if (cuidata[i].res == INFO_DEL) {
            scm_put_log_t(CHANGE_USER_DEL, 0, 0, cuidata[i].id);
        }
    }

    _init_data_changed_user_info();

    return 0;
}

static gint _set_data_changed_user_info(gchar *changed_id, CHANGED_USER_INFO_E res)
{
    gint i;

    for (i = 0; i < MAX_USER_COUNT; i++)
    {
//        g_message("###yanggungg : %s, %d, cuidata[%d].id : %s, changed_id : %s", __FUNCTION__, __LINE__, i, cuidata[i].id, changed_id);
        if (strcmp(cuidata[i].id, changed_id) == 0)
        {
            if (cuidata[i].res == INFO_EDIT) {
                if (res == INFO_DEL) cuidata[i].res = INFO_DEL;
            }
            else if (cuidata[i].res == INFO_ADD) {
                if (res == INFO_DEL) {
                    strcpy(cuidata[i].id, "");
                    cuidata[i].res = INFO_NORMAL;
                }
            }
            else if (cuidata[i].res == INFO_DEL) {
                if (res == INFO_ADD) cuidata[i].res = INFO_EDIT;
            }

            return 0;
        }
        else if (strlen(cuidata[i].id) == 0)
        {
            strcpy(cuidata[i].id, changed_id);
            cuidata[i].res = res;

            return 0;
        }
    }

    if (i == MAX_USER_COUNT)
    {
        g_message("###yanggungg : %s, %d, There is not empty slot!!", __FUNCTION__, __LINE__);
        //g_assert(0);
    }

    return 0;
}

static void _update_ud_page_button(gint user_cnt, gint cur_page, gint expose)
{
    gint need_page_cnt, rest, i;

    need_page_cnt = user_cnt / UM_ROWS;
    rest = user_cnt % UM_ROWS;

    if (rest == 0) need_page_cnt--;

    for (i = 0; i < UM_PAGE_TOT_CNT; i++)
    {
        if (i <= need_page_cnt) { 
            nfui_nfobject_enable(g_radio[i]);
        }
        else {
            nfui_nfobject_disable(g_radio[i]);
        }

        if (i == cur_page) {
            nfui_radio_button_set_toggled((NFBUTTON*)g_radio[i], TRUE);
        }

        if (expose) nfui_signal_emit(g_radio[i], GDK_EXPOSE, TRUE);
    }

}

static void _update_ud_button(gint user_cnt, gint cur_page, gint tb_idx, gint expose, gchar *grp_auth)
{
    gint ud_idx;
    ud_idx = ((cur_page) * UM_ROWS) + tb_idx;

    if (cur_page == 0 && ud_idx == 0)
    {
        nfui_nfobject_enable(edit_btn[tb_idx]);
        nfui_nfobject_disable(del_btn[tb_idx]);
    }
    else if (ud_idx < user_cnt)
    {
        nfui_nfobject_enable(edit_btn[tb_idx]);
        nfui_nfobject_enable(del_btn[tb_idx]);
    }
    else
    {
        nfui_nfobject_disable(edit_btn[tb_idx]);
        nfui_nfobject_disable(del_btn[tb_idx]);
    }
	
	if (strcmp(grp_auth, "ADMIN") != 0) {
		nfui_nfobject_disable(del_btn[tb_idx]);
	}

	if ((ud_idx == 0) && (strcmp(grp_auth, "ADMIN") == 0) && !ssm_is_admin())
	{
		nfui_nfobject_disable(edit_btn[tb_idx]);
	}
	else if (!ssm_is_admin() && (strcmp(grp_auth, "ADMIN") != 0) && (strcmp(ssm_get_cur_id(NULL), userdata[ud_idx].id) != 0)) 
	{
		nfui_nfobject_disable(edit_btn[tb_idx]);
	}

    if (expose)
    {
        nfui_signal_emit(edit_btn[tb_idx], GDK_EXPOSE, TRUE);
        nfui_signal_emit(del_btn[tb_idx], GDK_EXPOSE, TRUE);
    }
}

static void _update_ud_contents(UserManageData *ud, gint cur_page, gint tb_idx, gint expose, gchar *grp_auth, gint user_cnt)
{
    gint ud_idx;
	gchar tmp_covert[80];
	gchar tmp_covert_value[10];
	gint tmp_counts=0;
	gint j;

    ud_idx = ((cur_page) * UM_ROWS) + tb_idx;
    
    nfui_nflabel_set_text((NFLABEL*)userid[tb_idx], ud[ud_idx].id);
    nfui_nflabel_set_text((NFLABEL*)group[tb_idx], ud[ud_idx].group);
	///////////////////////////////////////////////
	memset(tmp_covert, '\0', sizeof(tmp_covert));
	tmp_counts = 0;
	for(j = 0 ; j < GUI_CHANNEL_CNT ; j++)
	{
		if(ud[ud_idx].covert[j] == '1')
		{
			tmp_counts += sprintf(&tmp_covert[tmp_counts], "%d,", j+1);
		}
	}
	
	gint covert_len = strlen(tmp_covert);
	if(covert_len > 1) tmp_covert[covert_len-1] = '\0';
				
	nfui_nflabel_set_text((NFLABEL*)covert[tb_idx], tmp_covert);
	////////////////////////////////////////////////
    nfui_nflabel_set_text((NFLABEL*)email[tb_idx], ud[ud_idx].email);

	if( expose )
    	nfui_check_button_set_active((NFCHECKBUTTON*)email_noti[tb_idx], ud[ud_idx].email_noti);
	else
		nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)email_noti[tb_idx], ud[ud_idx].email_noti);

	if(strcmp(ud[ud_idx].email, "") != 0) 
		nfui_check_button_sensitive((NFCHECKBUTTON*)email_noti[tb_idx], TRUE);
	else 
		nfui_check_button_sensitive((NFCHECKBUTTON*)email_noti[tb_idx], FALSE);

	if (ud_idx < user_cnt)
    {
		nfui_nfobject_enable(userid[tb_idx]);
		nfui_nfobject_enable(group[tb_idx]);
		nfui_nfobject_enable(covert[tb_idx]);
		nfui_nfobject_enable(email[tb_idx]);
		nfui_nfobject_enable(email_noti[tb_idx]);
		nfui_nfobject_enable(g_fixedTemp[tb_idx]);
    }
    else
    {
		nfui_nfobject_disable(userid[tb_idx]);
		nfui_nfobject_disable(group[tb_idx]);
		nfui_nfobject_disable(covert[tb_idx]);
		nfui_nfobject_disable(email[tb_idx]);
		nfui_nfobject_disable(email_noti[tb_idx]);
		nfui_nfobject_disable(g_fixedTemp[tb_idx]);
    }

	if ((ud_idx == 0) && (strcmp(grp_auth, "ADMIN") == 0) && !ssm_is_admin())
	{
		nfui_nfobject_disable(userid[tb_idx]);
		nfui_nfobject_disable(group[tb_idx]);
		nfui_nfobject_disable(covert[tb_idx]);
		nfui_nfobject_disable(email[tb_idx]);
		nfui_nfobject_disable(email_noti[tb_idx]);
		nfui_nfobject_disable(g_fixedTemp[tb_idx]);
	}
	else if (!ssm_is_admin() && (strcmp(grp_auth, "ADMIN") != 0) && (strcmp(ssm_get_cur_id(NULL), userdata[ud_idx].id) != 0)) 
	{
		nfui_nfobject_disable(userid[tb_idx]);
		nfui_nfobject_disable(group[tb_idx]);
		nfui_nfobject_disable(covert[tb_idx]);
		nfui_nfobject_disable(email[tb_idx]);
		nfui_nfobject_disable(email_noti[tb_idx]);
		nfui_nfobject_disable(g_fixedTemp[tb_idx]);
	}

	if (strlen(nfui_nflabel_get_text((NFLABEL*)email[tb_idx])) == 0)
	{
		nfui_nfobject_disable(email_noti[tb_idx]);
		nfui_nfobject_disable(g_fixedTemp[tb_idx]);
	}
	else
	{
		nfui_nfobject_enable(email_noti[tb_idx]);
		nfui_nfobject_enable(g_fixedTemp[tb_idx]);
	}


    if (expose)
    {
        nfui_signal_emit(userid[tb_idx], GDK_EXPOSE, FALSE);
        nfui_signal_emit(group[tb_idx], GDK_EXPOSE, FALSE);
        nfui_signal_emit(covert[tb_idx], GDK_EXPOSE, FALSE);
        nfui_signal_emit(email[tb_idx], GDK_EXPOSE, FALSE);
		nfui_signal_emit(email_noti[tb_idx], GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_fixedTemp[tb_idx], GDK_EXPOSE, TRUE);
    }

	g_memmove(&pre_userdata[ud_idx], &userdata[ud_idx], sizeof(UserManageData));
}

static void _update_ud_number(gint cur_page, gint tb_idx)
{
    gchar buf[8];
    gint ud_idx;

    ud_idx = ((cur_page) * UM_ROWS) + tb_idx;

    g_sprintf(buf, "%d", ud_idx + 1);
    
    nfui_nflabel_set_text((NFLABEL*)g_no[tb_idx], buf);
    nfui_signal_emit(g_no[tb_idx], GDK_EXPOSE, FALSE);
}

static void _update_userdata_table(UserManageData *ud, gint user_cnt, gint cur_page, gint expose)
{
    gint i;
    gint ud_idx;
	gchar grp_auth[16];

	memset(grp_auth, 0x00, sizeof(grp_auth));
	ssm_get_cur_group(grp_auth);
    for (i = 0; i < UM_ROWS; i++)
    {
        ud_idx = ((cur_page) * UM_ROWS) + i;

        if (memcmp(&pre_userdata[ud_idx], &userdata[ud_idx], sizeof(UserManageData)) == 0 &&
            g_pre_page == cur_page) {
            continue;
        }
        _update_ud_number(cur_page, i);
        _update_ud_contents(ud, cur_page, i, expose, grp_auth, user_cnt);
        _update_ud_button(user_cnt, cur_page, i, expose, grp_auth);
    }

    g_pre_page = cur_page;

    if (expose)
    {
        nfui_make_key_hierarchy(g_curwnd);
    }
}

static gboolean _update_table(gpointer data)
{
    _update_userdata_table(userdata, user_count, g_cur_page, 1);
    _update_ud_page_button(user_count, g_cur_page, 1);

    return FALSE;
}

static gboolean post_page_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_RADIO_GET_FOCUS)
    {
        gint idx;
        
        g_cur_page = nfui_radio_button_get_index((NFBUTTON*)obj);

        _update_userdata_table(userdata, user_count, g_cur_page, 1);
    }
    
    return FALSE;
}

static gboolean _get_active_allbtn()
{
    gint i, dis_cnt = 0;

    for (i = 0; i < user_count; i++)
    {
        if (nfui_nfobject_is_disabled(email_noti[i])) {
            dis_cnt++;
            continue;
        }
        if (!nfui_check_button_get_active((NFCHECKBUTTON*)email_noti[i])) return FALSE;
    }
    if (dis_cnt == user_count) return FALSE;

    return TRUE;
}

static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
		}
		break;

		default :
			break;
	}

	return FALSE;

}

static gboolean post_noti_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	gint ud_idx;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		for(i=0; i<user_count; i++)
		{
			if(obj == email_noti[i])
				break;
		}

		if(obj != email_noti[i])
			g_message("email_noti find error!!!\n");

		if(strcmp(nfui_nflabel_get_text((NFLABEL*)email[i]), "") == 0)
			return TRUE;	

		ud_idx = (g_cur_page * UM_ROWS) + i;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)email_noti[i]))
			userdata[ud_idx].email_noti = 1;
		else
			userdata[ud_idx].email_noti = 0;

		g_memmove(&pre_userdata[ud_idx], &userdata[ud_idx], sizeof(UserManageData));

	}
	return FALSE;
}

static gboolean post_userid_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint ret;
		guint tb_idx;
		gint ud_idx;
		guint col=0;
		gchar grp_auth[16];

		memset(grp_auth, 0x00, sizeof(grp_auth));
		ssm_get_cur_group(grp_auth);

		if((evt->type == GDK_2BUTTON_PRESS) && (evt->button.button == MOUSE_RIGTH_BUTTON)) {
			return FALSE;
		}


		for(tb_idx = 0; tb_idx<UM_ROWS; tb_idx++)
		{
			if(obj == userid[tb_idx]){	col=0;	break;}
			if(obj == group[tb_idx]){	col=1;	break;}
			if(obj == covert[tb_idx]){	col=2;	break;}
			if(obj == email[tb_idx]){	col=3;	break;}
		}
		
		if(tb_idx == UM_ROWS) return FALSE;

        ud_idx = ((g_cur_page) * UM_ROWS) + tb_idx;
		
		if(nfui_check_button_get_active((NFCHECKBUTTON*)email_noti[tb_idx]))
			userdata[ud_idx].email_noti = 1;
		else
			userdata[ud_idx].email_noti = 0;

		if(ud_idx==0)	ret = UserAddDlg_Open(g_curwnd, USER_MODE_EDIT, FALSE, userdata, ud_idx, user_count);
		else			ret = UserAddDlg_Open(g_curwnd, USER_MODE_EDIT, TRUE, userdata, ud_idx, user_count);

		if(ret == USER_EDIT_RET_OK)
		{
		    _set_data_changed_user_info(userdata[ud_idx].id, INFO_EDIT);
			_update_ud_contents(userdata, g_cur_page, tb_idx, 1, grp_auth, user_count);
		}
	}

	return FALSE;
}
	
static gboolean post_covert_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		guint off_x, off_y;
		guint mask = CAMERA_MASK_TYPE;
		gint tmp_counts=0;
		gint covert_len=0;
		gchar tmp_covert[80];
		gchar cv_d[32+1];
		gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		nfui_nfobject_get_offset(obj, &off_x, &off_y);

		if(!VW_ChannelMask_Ctrl(g_curwnd, "COVERT CHANNEL", (gint)(off_x - UM_LABLE3_WIDTH), (gint)(off_y + obj->height), &mask))
			return FALSE;


		memset(tmp_covert, '\0', sizeof(tmp_covert));
		memset(cv_d, '0', sizeof(cv_d));

		for (i=0; i<GUI_CHANNEL_CNT; i++) 
		{
			if(mask & (1 << i)) 
			{
				tmp_counts += sprintf(&tmp_covert[tmp_counts], "%d,", i+1);
				cv_d[i] = '1';
			}
			else 
			{
				cv_d[i] = '0';
			}
		}

			for(i=0; i<user_count; i++)
				memcpy(userdata[i].covert, cv_d, sizeof(cv_d));
		
		covert_len = strlen(tmp_covert);

		if (covert_len > 1) 
		    tmp_covert[covert_len-1] = '\0';
	
		for (i = 0; i < user_count; i++) 
		{
			nfui_nflabel_set_text((NFLABEL*)covert[i], tmp_covert);
			nfui_signal_emit(covert[i], GDK_EXPOSE, FALSE);
		}
	}
	return FALSE;
}

static gboolean post_email_noti_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gint i;
    	gboolean state;
		
		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
		
		for(i=0; i<user_count; i++) {
			if(!nfui_nfobject_is_disabled(email_noti[i])) {
				nfui_check_button_set_active((NFCHECKBUTTON*)email_noti[i], state);

				if(state) userdata[i].email_noti = 1;
				else userdata[i].email_noti = 0;
			}
		}
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		if ((memcmp(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT)) || (user_count != org_user_count))
		{
		g_memmove(userdata, org_userdata, sizeof(UserManageData)*MAX_USER_COUNT);

		user_count = org_user_count;
		g_cur_page = 0;

		_update_table(NULL);
        _init_data_changed_user_info();
		}
	}
	
	return FALSE;
}


static gboolean post_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint ret = 0;
        gint last_page, rest;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(user_count>=MAX_USER_COUNT)
		{
			nftool_mbox(g_curwnd, "NOTICE", "Too many users.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret = UserAddDlg_Open(g_curwnd, USER_MODE_ADD, TRUE, userdata, -1, user_count);

		if(ret == USER_EDIT_RET_OK)
		{
            _set_data_changed_user_info(userdata[user_count].id, INFO_ADD);
		    user_count++;

            last_page = user_count / UM_ROWS;
            rest = user_count % UM_ROWS;

	        if (rest == 0) last_page--;

	        g_cur_page = last_page;

	        g_timeout_add(300, _update_table, NULL);
		}
		
	}

	return FALSE;
}

static gboolean post_editbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	//if(evt->type == GDK_BUTTON_RELEASE || kpid == KEYPAD_ENTER)
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint ret;
		guint i, tb_idx;
		guint col=0;
		guint ud_idx;
		gchar grp_auth[16];

		memset(grp_auth, 0x00, sizeof(grp_auth));
		ssm_get_cur_group(grp_auth);

		if((evt->type == GDK_2BUTTON_PRESS) || (evt->button.button == MOUSE_RIGTH_BUTTON)) {
			return FALSE;
		}

		for(i=0; i<UM_ROWS; i++)
		{
			if(obj == edit_btn[i]) tb_idx = i;
		}

		ud_idx = (g_cur_page * UM_ROWS) + tb_idx;

		if(nfui_check_button_get_active((NFCHECKBUTTON*)email_noti[tb_idx]))
			userdata[ud_idx].email_noti = 1;
		else
			userdata[ud_idx].email_noti = 0;

		if(ud_idx == 0) ret = UserAddDlg_Open(g_curwnd, USER_MODE_EDIT, FALSE, userdata, ud_idx, user_count);
		else 			ret = UserAddDlg_Open(g_curwnd, USER_MODE_EDIT, TRUE, userdata, ud_idx, user_count);

		if(ret == USER_EDIT_RET_OK)
		{
		    _set_data_changed_user_info(userdata[ud_idx].id, INFO_EDIT);
			_update_ud_contents(userdata, g_cur_page, tb_idx, 1, grp_auth, user_count);
		}
	}

	return FALSE;
	
}

static gboolean post_delbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar user_id[40];
		gint i;
		guint ud_idx, tb_idx;
        gint last_page, rest;


		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		memset(user_id, 0x00, sizeof(user_id));
		ssm_get_cur_id(user_id);

		for(tb_idx=0; tb_idx<UM_ROWS; tb_idx++)
		{
			if(obj == del_btn[tb_idx]) break;
		}

        if (tb_idx == UM_ROWS) return FALSE;

        ud_idx = (g_cur_page * UM_ROWS) + tb_idx;

		if(!strcmp(user_id, userdata[ud_idx].id))
		{
			nftool_mbox(g_curwnd, "NOTICE", "It's unable to delete the ID.\nBecause it's a current login one.", NFTOOL_MB_OK);
			return FALSE;
		}

        _set_data_changed_user_info(userdata[ud_idx].id, INFO_DEL);
		user_count--;

    	for(i = ud_idx; i < user_count; i++)
    	{
    		g_memmove(&userdata[i], &userdata[i+1], sizeof(UserManageData));
    	}
		memset(&userdata[user_count], 0, sizeof(UserManageData));
		g_sprintf(&userdata[user_count].covert, "00000000000000000000000000000000");

        last_page = user_count / UM_ROWS;
        rest = user_count % UM_ROWS;

        if (rest == 0) last_page--;

        if (g_cur_page > last_page) g_cur_page = last_page;

        _update_table(NULL);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if ((memcmp(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT)) || (user_count != org_user_count))
		{
		    _set_log_changed_user_info();
			scm_put_log(CHANGE_USER_ID, 0, 0);

			g_memmove(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT);
			org_user_count = user_count;
				
			DAL_set_userManage_data_all(userdata, MAX_USER_COUNT);
			DAL_set_user_count(user_count);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			sysuser_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		UserManage_tab_out_handler();

		SystemSetupUser_Destroy(obj);
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


void init_UserManage_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *title_ntb;
	NFOBJECT *content_ntb;
	NFOBJECT *title_object[UM_COLUMNS];
	NFOBJECT *um_btns[UMB_BUTTONS];
	NFOBJECT *obj;

	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];

	const gchar *strButton[] = {"CANCEL", "ADD", "APPLY", "CLOSE"};
	const gchar *strTitle[] =  {"No.",
								"USER ID", 
								"GROUP", 
								"COVERT CHANNEL", 
								"E-MAIL", 
								"EMAIL NOTIFY"};
	
	guint width[UM_COLUMNS];
	guint btn_x, btn_y, btn_space;
	gint size_w, size_h;
	gint chk_w, chk_h;
	gint i;
	gchar buf[64];
	GSList *list;
	gint xpos, ypos;
	gchar grp_auth[16];

	memset(userdata, 0x00, sizeof(UserManageData)*MAX_USER_COUNT);
	memset(org_userdata, 0x00, sizeof(UserManageData)*MAX_USER_COUNT);
	memset(pre_userdata, 0x00, sizeof(UserManageData)*MAX_USER_COUNT);
	memset(cuidata, 0x00, sizeof(CHANGED_USER_INFO_T)*MAX_USER_COUNT);

    g_cur_page = 0;
    g_pre_page = -1;
	user_count = DAL_get_user_count();
	org_user_count = user_count;

	for (i=0; i < MAX_USER_COUNT; i++)
	{
		DAL_get_userManage_data(&userdata[i], i);
	}

	g_memmove(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT);

	g_curwnd = nfui_nfobject_get_top(parent);

	width[0] = UM_LABLE0_WIDTH - 10;
	width[1] = UM_LABLE1_WIDTH;
	width[2] = UM_LABLE2_WIDTH;
	width[3] = UM_LABLE3_WIDTH + 20;
	width[4] = UM_LABLE4_WIDTH + 70;
	width[5] = UM_LABLE4_WIDTH;


// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	g_content_fixed = content_fixed;

    title_ntb = nfui_nftable_new(UM_COLUMNS, 1, UM_COL_SPACE, UM_ROW_SPACE, width, UM_LABLE1_HEIGHT);
	nfui_nfobject_show(title_ntb);
	nfui_nffixed_put(content_fixed, title_ntb, UM_LABLE_LEFT, UM_LABLE_TOP);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);
	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	for(i=0; i<UM_COLUMNS; i++)
	{
		// SKSHIN, commented out
		if(i == 3) {	// covert
			title_object[i] = nfui_nffixed_new();
			nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_set_size(title_object[i], UM_LABLE3_WIDTH, UM_LABLE1_HEIGHT);
			nfui_nfobject_show(title_object[i]);

			obj = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			//nfui_nfobject_set_size(obj, UM_LABLE3_WIDTH-size_w, UM_LABLE1_HEIGHT);
			nfui_nfobject_set_size(obj, width[i], UM_LABLE1_HEIGHT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)title_object[i], obj, 0, 0);

			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image((NFBUTTON*)obj, dropdown_img);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			//nfui_nfobject_show(obj);
			nfui_regi_post_event_callback(obj, post_covert_all_event_handler);
			nfui_nffixed_put((NFFIXED*)title_object[i], obj, UM_LABLE3_WIDTH-size_w, 0);
		}
		else if(i == 5) {	// email noti
			title_object[i] = nfui_nffixed_new();
			nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_set_size(title_object[i], width[5], UM_LABLE1_HEIGHT);
			nfui_nfobject_show(title_object[i]);

			obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
			nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SMALL);
			nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
			//nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)title_object[i], obj, 10, (UM_LABLE1_HEIGHT - size_h)/2);
			nfui_regi_post_event_callback(obj, post_email_noti_all_event_handler);

			obj = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			//nfui_nfobject_set_size(obj, UM_LABLE4_WIDTH, UM_LABLE1_HEIGHT);
			nfui_nfobject_set_size(obj, width[5], UM_LABLE1_HEIGHT);
			nfui_nfobject_show(obj);
			//nfui_nffixed_put((NFFIXED*)title_object[i], obj, 10+size_w, 0);
			nfui_nffixed_put((NFFIXED*)title_object[i], obj, 0, 0);

		}
		else {
			title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
			nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
			nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(title_object[i]);
		}
		nfui_nftable_attach((NFTABLE*)title_ntb, title_object[i], i, 0);
	}

	content_ntb = nfui_nftable_new(UM_COLUMNS, UM_ROWS, UM_COL_SPACE, UM_ROW_SPACE, width, UM_LABLE1_HEIGHT);
	nfui_nfobject_show(content_ntb);
	nfui_nffixed_put(content_fixed, content_ntb, UM_CONTENT_TABLE_X, UM_CONTENT_TABLE_Y);

	for(i=0; i<UM_ROWS; i++)
	{
	    g_no[i] = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	    nfui_nflabel_set_align((NFLABEL*)g_no[i], NFALIGN_CENTER, 0);
	    nfui_nfobject_set_size(g_no[i], UM_LABLE0_WIDTH, UM_LABLE0_HEIGHT);
	    nfui_nfobject_use_focus(g_no[i], FALSE);
	    nfui_nfobject_modify_bg(g_no[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));

		userid[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_spacing((NFLABEL *)userid[i],SEMI_CONDENSED_SPACING);
		nfui_nflabel_set_skin_type(userid[i], NFTEXTBOX_TYPE_INPUT);

		group[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type(group[i], NFTEXTBOX_TYPE_INPUT);

		covert[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
		nfui_nflabel_set_skin_type(covert[i], NFTEXTBOX_TYPE_INPUT);
		
		email[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_spacing((NFLABEL *)email[i], CONDENSED_SPACING);		
		nfui_nflabel_set_skin_type(email[i], NFTEXTBOX_TYPE_INPUT);

		email_noti[i] = nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(email_noti[i]), NFCHECK_TYPE_NORMAL);
		nfui_check_get_size(email_noti[i], &chk_w, &chk_h);

		nfui_nfobject_support_multi_lang((NFOBJECT*)userid[i], FALSE);
		nfui_nfobject_support_multi_lang((NFOBJECT*)group[i], FALSE);
		nfui_nfobject_support_multi_lang((NFOBJECT*)email[i], FALSE);
	
		nfui_regi_post_event_callback(userid[i], post_userid_event_handler);
		nfui_regi_post_event_callback(group[i], post_userid_event_handler);
		nfui_regi_post_event_callback(covert[i], post_userid_event_handler);
		nfui_regi_post_event_callback(email[i], post_userid_event_handler);
		nfui_regi_post_event_callback(email_noti[i], post_noti_event_handler);

		g_fixedTemp[i] = nfui_nffixed_new();
		nfui_nfobject_modify_bg(g_fixedTemp[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_set_size(g_fixedTemp[i], width[5], UM_LABLE1_HEIGHT);
		nfui_nffixed_put((NFFIXED*)g_fixedTemp[i], email_noti[i], (width[5]-chk_w)/2, (UM_LABLE1_HEIGHT-chk_h)/2);

		nfui_nfobject_show(g_no[i]);
		nfui_nfobject_show(userid[i]);
		nfui_nfobject_show(group[i]);
		nfui_nfobject_show(covert[i]);
		nfui_nfobject_show(email[i]);
		nfui_nfobject_show(email_noti[i]);
		nfui_nfobject_show(g_fixedTemp[i]);

        nfui_nftable_attach((NFTABLE*)content_ntb, g_no[i], 0, i);
		nfui_nftable_attach((NFTABLE*)content_ntb, userid[i], 1, i);
		nfui_nftable_attach((NFTABLE*)content_ntb, group[i], 2, i);
		nfui_nftable_attach((NFTABLE*)content_ntb, covert[i], 3, i);
		nfui_nftable_attach((NFTABLE*)content_ntb, email[i], 4, i);
		nfui_nftable_attach((NFTABLE*)content_ntb, g_fixedTemp[i], 5, i);
	}

	xpos = UM_LABLE_LEFT;	
	ypos = UM_LABLE_TOP+UM_LABLE_HEIGHT_WITH_SPACE;
	
	for (i = 0; i < 6; i++)
	{
		xpos += width[i];
		xpos += UM_COL_SPACE;
	}

	xpos += UM_COL_SPACE;
	
	for(i = 0 ; i < UM_ROWS ; i++)
	{
		obj = nftool_normal_button_create_type3("EDIT", 120);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);
		nfui_regi_post_event_callback(obj, post_editbtn_event_handler);
		edit_btn[i] = obj;
				
		obj = nftool_normal_button_create_type3("DELETE", 120);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+122, ypos);
		nfui_regi_post_event_callback(obj, post_delbtn_event_handler);
		del_btn[i] = obj;

		ypos += UM_LABLE_HEIGHT_WITH_SPACE;
	}
	
    xpos = 0;

	for (i = 0; i < 6; i++)
	{
		xpos += width[i];
		xpos += UM_COL_SPACE;
	}

    xpos = UM_LABLE_LEFT + ((xpos - ((50 * UM_PAGE_TOT_CNT) + (10 * (UM_PAGE_TOT_CNT - 1)))) / 2);
    ypos += 30;

	for (i = 0; i < UM_PAGE_TOT_CNT; i++)
	{
	    g_sprintf(buf, "%d", i+1);
		obj = nftool_normal_button_create_type3(buf, 50);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);
		nfui_regi_post_event_callback(obj, post_page_btn_event_handler);
		g_radio[i] = obj;

        if (i == 0) {
    		list = nfui_radio_button_get_group((NFBUTTON*)obj);
		}
		else {
		    nfui_radio_button_add_group((NFBUTTON*)obj, list);
		}

		if (i == g_cur_page)
		{
	        nfui_radio_button_set_toggled((NFBUTTON*)obj, TRUE);
		}

		xpos += 50 + 10;
	}

	um_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(um_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(um_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, um_btns[0], MENU_V_BTN_R4_X, MENU_V_BTN_Y);

	um_btns[1] = nftool_normal_button_create_type1("ADD", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(um_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(um_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, um_btns[1], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	
	memset(grp_auth, 0x00, sizeof(grp_auth));
	ssm_get_cur_group(grp_auth);

	if (strcmp(grp_auth, "ADMIN") != 0) {
	    nfui_nfobject_disable(um_btns[1]);
	}

	um_btns[2] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(um_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(um_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, um_btns[2], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	um_btns[3] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(um_btns[3]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(um_btns[3]);
	nfui_nffixed_put((NFFIXED*)parent, um_btns[3], MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	
	nfui_regi_post_event_callback(um_btns[UMB_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(um_btns[UMB_ADD], post_addbutton_event_handler);
	nfui_regi_post_event_callback(um_btns[UMB_APPLY], post_applybutton_event_handler);
	nfui_regi_post_event_callback(um_btns[UMB_CLOSE], post_closebutton_event_handler);
	nfui_regi_post_event_callback(parent, post_page_event_handler);

	_update_userdata_table(userdata, user_count, g_cur_page, 0);
	_update_ud_page_button(user_count, g_cur_page, 0);

    nfui_make_key_hierarchy(g_curwnd);
}

gboolean UserManage_tab_out_handler()
{
	mb_type ret;
	guint i;

	if ((!memcmp(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT)) && (user_count == org_user_count))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
	    _set_log_changed_user_info();
		scm_put_log(CHANGE_USER_ID, 0, 0);

		g_memmove(org_userdata, userdata, sizeof(UserManageData)*MAX_USER_COUNT);
		org_user_count = user_count;
		
		DAL_set_userManage_data_all(userdata, MAX_USER_COUNT);
		DAL_set_user_count(user_count);

		sysuser_set_changeflag(1);
	}
	else
	{
    	g_memmove(userdata, org_userdata, sizeof(UserManageData)*MAX_USER_COUNT);

    	user_count = org_user_count;
        g_cur_page = 0;

    	_update_userdata_table(userdata, user_count, g_cur_page, 0);
    	_update_ud_page_button(user_count, g_cur_page, 0);
    	_init_data_changed_user_info();
	}

	return FALSE;
}


