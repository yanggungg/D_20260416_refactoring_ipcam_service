#include "nf_common.h"

#include "nf_api_param_app.h"
#include "nf_util_flash.h"
#include "nf_util_fw_single.h"

//#define DEBUG_PARAM_APP_JBSHELL
#ifdef DEBUG_PARAM_APP_JBSHELL
    #include "jbshell.h"
#endif
//#define DEBUG_PARAM_APP_LOG
#include "nf_debug.h"

/*
   Gloval Function Definition
 */
static void nf_api_param_app_print(NF_PARAM_APP *app_param);
#ifdef DEBUG_PARAM_APP_JBSHELL
	static int nf_api_param_app_jbshell(int argc, char **argv);
#endif


static const char *_DEBUG_PARAM_APP_str[8] =
{
	"PARAM_APP_IDX_DUMP",
	"PARAM_APP_IDX_NR"
};

static gint _DEBUG_PARAM_APP_log[8] =
{
	0,0,0,0, 0,0,0,0
};

gboolean nf_api_param_app_init(void)
{
	gboolean ret=0;

	ret=nf_debug_category_add( "param_app", (const char *)_DEBUG_PARAM_APP_str, _DEBUG_PARAM_APP_log, DEBUG_PARAM_APP_IDX_NR);
	if(!ret) {
		g_warning("%s failed..", __FUNCTION__);
	}

	return TRUE;
}

gboolean nf_api_param_app_set(NF_PARAM_APP *app_param)
{   
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs =0;
	
	g_return_val_if_fail(dataBuf != NULL, 0);
	offs = 0;
	
	memset(dataBuf, 0xff, sizeof(dataBuf));
	memcpy(dataBuf, app_param, sizeof(NF_PARAM_APP));

	#ifdef DEBUG_PARAM_APP_LOG
		if( _DEBUG_PARAM_APP_log[ DEBUG_PARAM_APP_IDX_DUMP ] )
			nf_debug_hexdump( app_param, sizeof(NF_PARAM_APP));
	#endif
	
	ret = nf_flash_erase(NF_FLASH_PING_APP_PARAM_MTD_NUM, NULL, NULL);
	if(!ret)
	{
		g_warning("%s Set App Param Nand Erase Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	ret = nf_flash_page_write(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
	if(!ret)
	{
		g_warning("%s Set App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}

gboolean nf_api_param_app_get(NF_PARAM_APP *app_param)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs = 0;

	g_return_val_if_fail(dataBuf != NULL, 0);
	g_return_val_if_fail(app_param != NULL, 0);

	offs = 0;

	ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
	if(!ret)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	#ifdef DEBUG_PARAM_APP
		flash_print(dataBuf, NULL, 0);
	#endif

	memcpy(app_param, dataBuf, sizeof(NF_PARAM_APP));

	#ifdef DEBUG_PARAM_APP_LOG
		if( _DEBUG_PARAM_APP_log[ DEBUG_PARAM_APP_IDX_DUMP ] )
			nf_debug_hexdump( app_param, sizeof(NF_PARAM_APP));
	#endif

	if(strncmp(app_param->magic, NF_APP_PARAM_MAGIC, 4) != 0 )
	{
		g_warning("%s magic failed[%4.4s]", __FUNCTION__, app_param->magic);
		/* setting default value */

		memset(app_param, 0x00, sizeof(NF_PARAM_APP));
		strncpy(app_param->magic , NF_APP_PARAM_MAGIC, sizeof(NF_APP_PARAM_MAGIC));

		nf_api_param_app_set(app_param);

		return TRUE;
	}

	return TRUE;
}

gboolean nf_api_param_app_set_cate(gint cate, gint val)
{
	gint ret=0;
	NF_PARAM_APP app_param;

	memset(&app_param, 0x0, sizeof(NF_PARAM_APP));
	nf_api_param_app_get(&app_param);

	g_message("%s cate %d val %d", __FUNCTION__, cate, val);
	switch(cate)
	{
		case NF_PARAM_APP_CATE_IS_NORMAL:
			app_param.is_normal = val;
			break;
		case NF_PARAM_APP_CATE_IS_UPGRADE :
			app_param.is_upgrade = val;
			break;
		case NF_PARAM_APP_CATE_IS_PAL :
			app_param.is_pal = val;
			break;
		case NF_PARAM_APP_CATE_DUAL :
			app_param.is_dual = val;
			break;
		case NF_PARAM_APP_CATE_DUAL_TYPE :
			app_param.info_dual_disp.dual_type = val;
			break;
		case NF_PARAM_APP_CATE_SET_MONITOR_MAIN :
			app_param.info_dual_disp.monitor_main = val;
			break;
		case NF_PARAM_APP_CATE_SET_RESOLUTION :
			app_param.val_res = val;
			break;
		case NF_PARAM_APP_CATE_SET_RESOLUTION_HDMI :
			app_param.val_res_hdmi = val;
			break;
		case NF_PARAM_APP_CATE_IS_MICOM_UPGRADE :
			app_param.is_micom_up = val;
			break;
		case NF_PARAM_APP_CATE_HDMI2_VGA_OUT :
			app_param.hdmi2_vga_out = val;
			break;
		default:
			g_warning("[PARAM_APP][Cate][SET] Unknown Cate.. %d", cate);
			goto nf_api_param_app_set_fail;
	}

	ret = nf_api_param_app_set(&app_param);
	if(ret == FALSE)
	{
		g_warning("%s nf_api_param_app_set!!!", __FUNCTION__);
		return FALSE;
	}

	/** Verify **/
	memset(&app_param, 0x0, sizeof(NF_PARAM_APP));
	nf_api_param_app_get(&app_param);
	nf_api_param_app_print(&app_param);

	return TRUE;

nf_api_param_app_set_fail:
	return FALSE;
}

gboolean nf_api_param_app_get_cate(gint cate, gint *val)
{   
	gint ret=0;
	NF_PARAM_APP app_param;
	
	memset(&app_param, 0x0, sizeof(NF_PARAM_APP));
	nf_api_param_app_get(&app_param);
	
	switch(cate)
	{
		case NF_PARAM_APP_CATE_IS_NORMAL:
			*val=app_param.is_normal;
			break;
		case NF_PARAM_APP_CATE_IS_UPGRADE :
			*val=app_param.is_upgrade;
			break;
		case NF_PARAM_APP_CATE_IS_PAL :
			*val=app_param.is_pal;
			break;
		case NF_PARAM_APP_CATE_IS_MICOM_UPGRADE :
			*val=app_param.is_micom_up;
			break;
		case NF_PARAM_APP_CATE_HDMI2_VGA_OUT :
			*val=app_param.hdmi2_vga_out;
			break;
		default:
			g_warning("[PARAM_APP][Cate][GET] Unknown Cate.. %d", cate);
			goto nf_api_param_app_get_fail;
	}
	
	return TRUE;

nf_api_param_app_get_fail:
	return FALSE;
}

gboolean nf_api_param_app_set_upgrade_val(NF_PARAM_APP *app_param, gboolean is_upgrade)
{
	gint ret=0;
	g_message("%s called is_upgrade[%d]", __FUNCTION__, is_upgrade);

	app_param->is_upgrade = is_upgrade;
	#if defined(CONFIG_FWUPGRADE_SINGLE)
		if(is_upgrade == 0)
			app_param->fwup_status = NF_FWUP_STATUS_DONE;
	#endif

	ret = nf_api_param_app_set(app_param);
	if(ret == FALSE)
	{
		g_warning("%s nf_api_param_app_set!!!", __FUNCTION__);
		return FALSE;
	}

	g_message("%s is_normal      [%d]", __FUNCTION__, app_param->is_normal);
	g_message("%s is_upgrade     [%d]", __FUNCTION__, app_param->is_upgrade);
	g_message("%s is_pal         [%d]", __FUNCTION__, app_param->is_pal);

	return TRUE;
}

static void nf_api_param_app_print(NF_PARAM_APP *app_param)
{
	g_print("============= App Param =============\n");
	g_print("magic            [%4.4s]\n", app_param->magic);
	g_print("is_normal        [%d]\n", app_param->is_normal);
	g_print("is_upgrade       [%d]\n", app_param->is_upgrade);
	g_print("is_pal           [%d]\n", app_param->is_pal);
	g_print("is_dual          [%d]\n", app_param->is_dual);
	g_print("res              [%d]\n", app_param->val_res);
	g_print("res_hdmi         [%d]\n", app_param->val_res_hdmi);
	g_print("dual_is_prev     [%d]\n", app_param->info_dual_disp.is_prev);
	g_print("dual_output_mode [%d]\n", app_param->info_dual_disp.output_mode);
	g_print("dual_vga         [%d]\n", app_param->info_dual_disp.vga);
	g_print("dual_hdmi        [%d]\n", app_param->info_dual_disp.hdmi);
	g_print("dual_type        [%d]\n", app_param->info_dual_disp.dual_type);
	g_print("hdmi2_vga_out    [%d]\n", app_param->hdmi2_vga_out);
	g_print("is_micom_ip      [%d]\n", app_param->is_micom_up);

	g_print("=====================================\n");
}   

#ifdef DEBUG_PARAM_APP_JBSHELL

static char nf_api_param_app_jbshell_help[] = "sys_app_param 0 => read\n"
									"sys_app_param 1 [0 or 1] ==> write\n"
									"sys_app_param 2 ==> erase \n";
static int nf_api_param_app_jbshell(int argc, char **argv)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean is_write=0, ret=0;
	NF_PARAM_APP app_param;
	guint offs=0;

	if(argc < 2) {
		printf("%s\n", nf_api_param_app_jbshell_help);
		return -1;
	}

	is_write = (gboolean)strtoul(argv[1], NULL, 0);

	if(is_write == 1) {
		gint val=0;
		ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
		if(!ret) {
			g_warning("%s Get App Parameter Error!!!", __FUNCTION__);
			return FALSE;
		}

		memcpy(&app_param, dataBuf, sizeof(app_param));
		if(strncmp(app_param.magic, NF_APP_PARAM_MAGIC, 4) != 0) {
			/* setting default value */
			g_print("set default value\n");
			strncpy(app_param.magic , NF_APP_PARAM_MAGIC, sizeof(NF_APP_PARAM_MAGIC));
			app_param.is_normal = 0;
		}

		if(argc > 2) {
			val=(gint)strtoul(argv[2], NULL, 0);
		}
		else {
			return FALSE;
		}

		app_param.is_normal = val;

		ret = nf_api_param_app_set(&app_param);
		if(!ret) {
			g_warning("%s Set App Param Error!!!", __FUNCTION__);
			return FALSE;
		}
	}
	else if(is_write == 2) {
		ret = nf_flash_erase(NF_FLASH_PING_APP_PARAM_MTD_NUM, NULL, NULL);
		if(!ret) {
			g_warning("%s Set App Param Nand Erase Error!!!", __FUNCTION__);
			return FALSE;
		}
	}
	else if(is_write == 0) {
		nf_api_param_app_get(&app_param);

		g_print("%s is_normal      [%d]\n", __FUNCTION__, app_param.is_normal);
		g_print("%s is_upgrade     [%d]\n", __FUNCTION__, app_param.is_upgrade);
		g_print("%s is_pal         [%d]\n", __FUNCTION__, app_param.is_pal);

	}
	else {
		printf("%s\n", nf_api_param_app_jbshell_help);
		return -1;
	}

	return 0;
}
__commandlist(nf_api_param_app_jbshell, "sys_app_param", nf_api_param_app_jbshell_help, nf_api_param_app_jbshell_help);

#endif

