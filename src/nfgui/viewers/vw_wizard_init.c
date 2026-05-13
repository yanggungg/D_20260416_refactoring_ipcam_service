#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "tools/nf_ui_tool.h"

#include "ix_mem.h"
#include "scm.h"
#include "ssm.h"

#include "vw_desc.h"
#include "vw_wizard_init.h"

enum {
    WIZARD_TYPE_LANGUAGE = 0,
    WIZARD_TYPE_SIGTYPE,
    WIZARD_TYPE_WELCOME,
    WIZARD_TYPE_PASSWORD,
    WIZARD_TYPE_DATETIME,
    WIZARD_TYPE_RECORD,
	WIZARD_TYPE_NETWORK,	
	WIZARD_TYPE_MAX
};

typedef struct _WIZARD_PAGE_T {
    gint type;
    gint numbering;
    gint curno;
    gint maxno;    
    WIZARD_SETUP_FUNC func;
} WIZARD_PAGE_T;

static NFWINDOW *g_parent;

static WIZARD_USERDATA_T g_wizard_data;
static WIZARD_PAGE_T g_wizard_page[64];

static guint g_timer_src = 0;
static gint g_page_cnt = 0;
static gint g_page_idx = 0;
static gint g_cur_page = 0;


static gboolean _tmr_destory_wizard(void *data)
{
    if (g_wizard_data.topwnd) {
    	nfui_nfobject_destroy(g_wizard_data.topwnd);
    }

	g_timer_src = 0;

	return FALSE;
} 

static gint _delete_tmr_destory()
{
	if (g_timer_src) {
        g_source_remove(g_timer_src);
        g_timer_src = 0;
    }
    
    return 0;
}

static gint _save_wizard_settings()
{
	DAL_save_setup_db(NFSETUP_WINDOW_USER);
	DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
	DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
	DAL_save_setup_db(NFSETUP_WINDOW_RECORDING);
}

static gint _get_page_title(WIZARD_USERDATA_T *wizard_data)
{
    gint idx = g_page_idx;
    gchar strBuf[16];


    memset(wizard_data->title, 0x00, sizeof(wizard_data->title));

    if (g_wizard_page[idx].type == WIZARD_TYPE_LANGUAGE)
    {
        strcpy(wizard_data->title, lookup_string("LANGUAGE SETUP"));
    }
    else if (g_wizard_page[idx].type == WIZARD_TYPE_WELCOME)
    {
        strcpy(wizard_data->title, "SETUP WIZARD");
        return 0;
    }
    else if (g_wizard_page[idx].type == WIZARD_TYPE_SIGTYPE)
    {
        strcpy(wizard_data->title, lookup_string("AC POWER FREQUENCY SETUP"));
    }
    else
    {
        strcpy(wizard_data->title, lookup_string("SETUP WIZARD"));
    }

    if ((g_wizard_page[idx].numbering) && (g_wizard_page[idx].curno > 0))
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        //g_sprintf(strBuf, " (%d / %d)", g_wizard_page[idx].curno, g_wizard_page[idx].maxno);
        g_sprintf(strBuf, " (%d / %d)", g_wizard_page[idx].curno, g_cur_page);
        strcat(wizard_data->title, strBuf);
    }

    return 0;

}
static gint _get_language_data(WIZARD_USERDATA_T *wizard_data)
{
    DAL_get_language(wizard_data->langData.lang);
    wizard_data->langData.langCnt = DAL_get_support_lang_cnt();

    g_memmove(&wizard_data->org_langData, &wizard_data->langData, sizeof(LANGUAGE_DATA_T));

	return 0;
}

static gint _get_sigtype_data(WIZARD_USERDATA_T *wizard_data)
{
    wizard_data->sigData.sigtype = DAL_get_sig_type();
    wizard_data->sigData.is_changed = 0;
    
    g_memmove(&wizard_data->org_sigData, &wizard_data->sigData, sizeof(SIGTYPE_DATA_T));

	return 0;
}

static gint _get_user_account_data(WIZARD_USERDATA_T *wizard_data)
{
    DAL_get_user_id(wizard_data->accData.userid, 0);
    DAL_get_user_passwd(wizard_data->accData.pw, 0);    

    return 0;
}

static gint _get_datetime_data(WIZARD_USERDATA_T *wizard_data)
{
    DAL_get_dateTime_format(&wizard_data->dtData.dateFormat, &wizard_data->dtData.timeFormat);
    DAL_get_tz_data(&wizard_data->dtData.timeZone);
    wizard_data->dtData.dst = DAL_get_dst();

    return 0;
}

static gint _get_record_data(WIZARD_USERDATA_T *wizard_data)
{
    DAL_get_wizard_record_data(&wizard_data->recordData);
    
    return 0;
}

static gint _get_network_data(WIZARD_USERDATA_T *wizard_data)
{
	NF_NETIF_GET_INFO ret_net_info;

	DAL_get_ipSetup_data(&wizard_data->networkData.ipsetup_data);
	scm_get_sys_netinfo(&ret_net_info);

	convertIntToIP(wizard_data->networkData.ipsetup_data.ip, ret_net_info.ipaddr);
	convertIntToIP(wizard_data->networkData.ipsetup_data.dns1, ret_net_info.dnsserver1);
	convertIntToIP(wizard_data->networkData.ipsetup_data.dns2, ret_net_info.dnsserver2);
	convertIntToIP(wizard_data->networkData.ipsetup_data.gateway, ret_net_info.gateway);
	convertIntToIP(wizard_data->networkData.ipsetup_data.subnet, ret_net_info.netmask);

    DAL_get_ddns_data(&wizard_data->networkData.ddns_data);
    DAL_get_Sequrinet_Status(&wizard_data->networkData.use_sequrinet);

    g_memmove(&wizard_data->org_networkData, &wizard_data->networkData, sizeof(NETWORK_DATA_T));
    

	return 0;
}

static gint _init_language_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.language.support) return -1;

    _get_language_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.language.setup[i].func)
        {
            if (ivsc.dfunc.wizard.language.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.language.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_LANGUAGE;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.language.setup[i].func;

            if (ivsc.dfunc.wizard.language.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.language.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _init_sigtype_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.sigtype.support) return -1;

    _get_sigtype_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.sigtype.setup[i].func)
        {
            if (ivsc.dfunc.wizard.sigtype.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.sigtype.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_SIGTYPE;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.sigtype.setup[i].func;

            if (ivsc.dfunc.wizard.sigtype.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.sigtype.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _init_welcome_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.welcome.support) return -1;

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.welcome.setup[i].func)
        {
            if (ivsc.dfunc.wizard.welcome.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.welcome.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_WELCOME;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.welcome.setup[i].func;

            if (ivsc.dfunc.wizard.welcome.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.welcome.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _init_password_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.password.support) return -1;

    _get_user_account_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.password.setup[i].func)
        {
            if (ivsc.dfunc.wizard.password.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.password.setup[i].func) 
        {   
            g_wizard_page[idx].type = WIZARD_TYPE_PASSWORD;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.password.setup[i].func;

            if (ivsc.dfunc.wizard.password.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.password.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _init_datetime_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.datetime.support) return -1;
    
    _get_datetime_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.datetime.setup[i].func)
        {
            if (ivsc.dfunc.wizard.datetime.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.datetime.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_DATETIME;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.datetime.setup[i].func;

            if (ivsc.dfunc.wizard.datetime.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.datetime.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _init_record_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.record.support) return -1;

    _get_record_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.record.setup[i].func)
        {
            if (ivsc.dfunc.wizard.record.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.record.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_RECORD;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.record.setup[i].func;

            if (ivsc.dfunc.wizard.record.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.record.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}

static gint _get_system_data(WIZARD_USERDATA_T *wizard_data)
{
    wizard_data->systemData.agr_policy = DAL_get_agr_policy();

    g_memmove(&wizard_data->org_systemData, &wizard_data->systemData, sizeof(SYSTEM_DATA_T));

    return 0;
}

static gint _init_network_setup_wizard(gint *page_cnt)
{
    gint i, idx = *page_cnt;
    gint curno = 0, maxno = 0;    

    if (!ivsc.dfunc.wizard.network.support) return -1;

    _get_network_data(&g_wizard_data);
    _get_system_data(&g_wizard_data);

    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.network.setup[i].func)
        {
            if (ivsc.dfunc.wizard.network.setup[i].use_numbering) maxno++;
        }
    }
    
    for (i = 0; i < 16; i++)
    {          
        if (ivsc.dfunc.wizard.network.setup[i].func) 
        {
            g_wizard_page[idx].type = WIZARD_TYPE_NETWORK;
            g_wizard_page[idx].func = ivsc.dfunc.wizard.network.setup[i].func;

            if (ivsc.dfunc.wizard.network.setup[i].use_numbering)
            {
                g_wizard_page[idx].numbering = ivsc.dfunc.wizard.network.setup[i].use_numbering;
                g_wizard_page[idx].maxno = maxno;
                //g_wizard_page[idx].curno = ++curno;
                g_wizard_page[idx].curno = ++g_cur_page;
            }
                        
            idx++;
        }
    }

    *page_cnt = idx;

	return 0;
}



//////////////////////

gint _wizard_cancel()
{
    _delete_tmr_destory();

    if (g_wizard_data.run_import)
    {
        ssm_run_auto_logout();
    }

    return 0;
}

gint _wizard_close()
{
    _delete_tmr_destory();

    _save_wizard_settings();

    if (g_wizard_data.run_import)
    {
        g_wizard_data.run_import = 0;
        ssm_run_auto_logout();
    }
    
    return 0;
}

gint _wizard_finish()
{   
    _delete_tmr_destory();

    g_page_idx = g_page_cnt-1;

    _get_page_title(&g_wizard_data);   

    g_wizard_page[g_page_idx].func(g_parent, &g_wizard_data);    
    
    return 0;
}

gint _wizard_prev_step(gint cnt)
{
    if (g_page_idx == 0) return -1;

    _delete_tmr_destory();
    
    g_page_idx -= cnt;
    
    _get_page_title(&g_wizard_data);   

    g_wizard_page[g_page_idx].func(g_parent, &g_wizard_data);      
    
    return 0;
}

gint _wizard_next_step(gint cnt)
{
    if (g_page_idx+1 == g_page_cnt) return -1;

    _delete_tmr_destory();

    g_page_idx += cnt;
    
    _get_page_title(&g_wizard_data);  

    g_wizard_page[g_page_idx].func(g_parent, &g_wizard_data);  

    return 0;
}

gint vw_wizard_settings_reload(WIZARD_USERDATA_T *wizard_data)
{
    _get_user_account_data(wizard_data);
    _get_datetime_data(wizard_data);
    _get_record_data(wizard_data);
    _get_network_data(wizard_data);
    
    return 0;
}

//////////////////////

gint vw_wizard_init(NFWINDOW *parent, gint remove_sec)
{
    WIZARD_SETUP_FUNC wizard_func = 0;   
    gint maxno, curno;
    gint page_cnt = 0;
    gint run = 0;

    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    g_parent = parent;
    g_timer_src = 0;
    g_page_cnt = 0;
    g_page_idx = 0;
    g_cur_page = 0;

    memset(&g_wizard_data, 0x00, sizeof(WIZARD_USERDATA_T));
    memset(g_wizard_page, 0x00, sizeof(WIZARD_PAGE_T)*64);

    DAL_get_Langwizard_func(&run);
    DAL_get_recordOper_mode_data(&g_wizard_data.recordData.mode);
    
    if (run)
    {
        _init_language_setup_wizard(&page_cnt);
        _init_sigtype_setup_wizard(&page_cnt);
    }
    _init_welcome_setup_wizard(&page_cnt);
    _init_password_setup_wizard(&page_cnt);
    _init_datetime_setup_wizard(&page_cnt);
    if (g_wizard_data.recordData.mode == AUTO_CONFIG) {
        _init_record_setup_wizard(&page_cnt);
    }
    _init_network_setup_wizard(&page_cnt);

    g_page_cnt = page_cnt;
   
    if (remove_sec) {
        g_timer_src = g_timeout_add(1000*remove_sec, _tmr_destory_wizard, NULL);
    }

    wizard_func = g_wizard_page[0].func;
    if (!wizard_func) g_assert(0);

    _get_page_title(&g_wizard_data);
    wizard_func(parent, &g_wizard_data);         //include gtk_main()
 
    return 0;
}

