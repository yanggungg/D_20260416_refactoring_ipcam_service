/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
29/01/2014 Min-Jung Kim   Created. (ONVIF Analytics Service ver 2.3)
*/

// alswnd

#ifndef __NF_ONVIF_ANALYTICS_C__
#define __NF_ONVIF_ANALYTICS_C__

#include <stdsoap2.h>
#include <onvifH.h>
#include "wsaapi.h"
#include <nf_ipcam_defs.h>
#include <nf_api_ipcam.h>

#include <nf_api_ipcam.h>
#include "nf_onvif_analytics.h"

enum _ANALYTICS_CAPABILITY_T
{
	CAP_ATTRIBUTE_RULE,
	CAP_ATTRIBUTE_MODULE,
	CAP_ATTRIBUTE_CELLBASE,
	CAP_ATTRIBUTE_MAX
};

struct __MOTION_WINDOW_T_
{
	int x;
	int y;
	int width;
	int height;
};

// Get Day Night Now ? 
extern int get_dn_now(int ch);


// DECLARATIONS: STATIC FUNCTION 
// ============================================================================

static int _cell_layout_string_parser(const char *str, struct _AnalyticsModules *analyticsModule);
static int _cell_layout_dom_parser(struct soap_dom_element *el, struct _AnalyticsModules *analyticsModule);
static void* _get_string_value(const char *str, const char *find_str, char *value);
static int rect_to_window(int rows, int columns, MAREA *marea, struct __MOTION_WINDOW_T_ *window);


static long packbits(char *src,char *dst,long n);
static unsigned char* rect_to_cells(int rows, int columns, int area_num, MAREA *marea, int *length);
static char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) ;

// GSOAP CALL
ONVIF_MSG _nf_onvif_analytics_get_service_capabilities(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, int att[]);
ONVIF_MSG _nf_onvif_analytics_modify_analytics_modules(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule);
ONVIF_MSG _nf_onvif_analytics_modify_rules(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, NFIPCamSetupMotionArea *info);
ONVIF_MSG _nf_onvif_analytics_get_analytics_modules(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule);
ONVIF_MSG _nf_onvif_analytics_modify_analytics_modules_cells(
    ipcam_onvif_auth_info_t*, const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule, int sensitivity);
// RULE
ONVIF_MSG _nf_onvif_analytics_rules_remove(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, char **rule);
ONVIF_MSG _nf_analytics_rules_remove_all(ipcam_onvif_auth_info_t*, const char* endpoint, char* token);
ONVIF_MSG _nf_onvif_analytics_rules_create(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, char *rule_name, MAREA *marea, int ch);
ONVIF_MSG _nf_onvif_analytics_rules_create_window(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, int index, struct __MOTION_WINDOW_T_ *window, int sensitivity, int ch);
ONVIF_MSG _nf_analytics_rules_create(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, NFIPCamSetupMotionArea *info);
ONVIF_MSG _nf_analytics_rules_create_window(ipcam_onvif_auth_info_t*, const char* endpoint, char* token, NFIPCamSetupMotionArea *info);
ONVIF_MSG _nf_onvif_analytics_modify_rules_cells(
    ipcam_onvif_auth_info_t*, const char* endpoint, char* token, NFIPCamSetupMotionArea *info, unsigned char *cells);


// DEFINITIONS : API FUNCTION
// ============================================================================

// ONVIF VIDEO ANALTYICS SET ANALYTICS MODULE ENABLE
ONVIF_API nf_onvif_va_set_analytics_modules_enable(int ch)
{
	char endpoint[256];
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	struct _AnalyticsModules analyticsModule;
	memset(&analyticsModule, 0x00, sizeof(struct _AnalyticsModules));


	NF_ONVIF_DBG(MINOR,"START\n");

	analyticsModule.Enable = 1;

	mtable *runtime = get_runtime();

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_ANALYTICS]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_analytics_modify_analytics_modules(&auth_info, endpoint, runtime[ch].onvif.vac_token, &analyticsModule);
	
	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MOTION, -1);
	NF_ONVIF_DBG(MAJOR,"END\n");

	return IPCAM_SETUP_RTN_DONE;
}

// ONVIF VIDEO ANALYTICS SET MOTION
ONVIF_API nf_onvif_va_set_motion(NFIPCamSetupMotionArea* info, int ch)
{
	char endpoint[256];
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR,"START\n");

	mtable *runtime = get_runtime();

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_ANALYTICS]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	//
	// lagacy rules remove and create rules
	//
	
	// Rules Remove All
	rtn = _nf_analytics_rules_remove_all(
			&auth_info, endpoint, runtime[ch].onvif.vac_token);

	// Create Rules
	rtn = _nf_analytics_rules_create(
			&auth_info, endpoint, runtime[ch].onvif.vac_token, info);


#if 0  // old 
	rtn = _nf_onvif_analytics_modify_rules(
			runtime[ch].onvif.auth_method, 
			endpoint, runtime[ch].username, runtime[ch].password, token, info);
#endif
	NF_ONVIF_DBG(MAJOR,"END\n");

	return IPCAM_SETUP_RTN_DONE;



}

ONVIF_API nf_onvif_va_set_motion_cells(NFIPCamSetupMotionArea* info, int ch)
{
	char endpoint[256];
	int rtn;

	unsigned char *cells;
	int len;
	unsigned char dst[2048];
	int out_len;
	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR,"START\n");

	mtable *runtime = get_runtime();

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_ANALYTICS]);

	// Rectangle to Cells
	cells = rect_to_cells(info->block_height, info->block_width, info->area_num, &info->marea[0], &len);
	if(cells == NULL)
	{
		rtn = IPCAM_SETUP_RTN_FAILED;
		NF_ONVIF_DBG(MINOR,"\e[31m rect_to_cells return NULL\n\e[0m");
		goto end_err;
	}

	// Packbits 
	len = packbits(cells, dst, len);
	free(cells);

	// Base64 Encoded Cells
	cells = base64_encode(dst, len, &out_len);

	if(cells == NULL)
	{
		rtn = IPCAM_SETUP_RTN_FAILED;
		NF_ONVIF_DBG(WARN,"\e[31m base64_encode return NULL\n\e[0m");
		goto end_err;
	}

	NF_ONVIF_DBG(MINOR,"info->block_height : %d\n", info->block_height);
	NF_ONVIF_DBG(MINOR,"info->block_width  : %d\n", info->block_width);
	NF_ONVIF_DBG(MINOR,"cells : %s\n", cells);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_analytics_modify_rules_cells(
			&auth_info, endpoint, runtime[ch].onvif.vac_token, info, cells);

	rtn = _nf_onvif_analytics_modify_analytics_modules_cells(
			&auth_info, endpoint, runtime[ch].onvif.vac_token,  NULL, info->marea[0].sensitivity);



	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MOTION, -1);
end_err:
	NF_ONVIF_DBG(MAJOR,"END\n");
	return IPCAM_SETUP_RTN_DONE;



}

ONVIF_API nf_onvif_va_set_motion_window(NFIPCamSetupMotionArea* info, int ch)
{
	char endpoint[256];
	int rtn;

	ipcam_onvif_auth_info_t auth_info;


	NF_ONVIF_DBG(MAJOR,"START\n");

	mtable *runtime = get_runtime();

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_ANALYTICS]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	// Rules Remove All
	rtn = _nf_analytics_rules_remove_all(
			&auth_info, endpoint, runtime[ch].onvif.vac_token);

	// Rules Create
	rtn = _nf_analytics_rules_create_window(
			&auth_info, endpoint, runtime[ch].onvif.vac_token, info);

	NF_ONVIF_DBG(MAJOR,"END\n");

	nf_ipcam_setup_waiting(ch, NF_IPCAM_TYPE_SET_MOTION, -1);

end_err:
	return IPCAM_SETUP_RTN_DONE;


}

// GET LAYOUT ROWS, COLUMNS
ONVIF_API nf_onvif_va_get_analytics_modules(struct _AnalyticsModules *analyticsModule, int ch)
{
	char endpoint[256];
	int rtn;

	ipcam_onvif_auth_info_t auth_info;

	NF_ONVIF_DBG(MAJOR,"START\n");

	mtable *runtime = get_runtime();

	memset(endpoint, 0x00, 256);
	strcpy(endpoint, runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_ANALYTICS]);

	auth_info.auth_method = runtime[ch].onvif.auth_method;
	auth_info.username = runtime[ch].username;
	auth_info.password = runtime[ch].password;
	auth_info.endpoint = runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE];

	rtn = _nf_onvif_analytics_get_analytics_modules(
			&auth_info, endpoint, runtime[ch].onvif.vac_token, analyticsModule);
	if(rtn != 0)
	{

		NF_ONVIF_DBG(WARN,"Get Analytics Module Fail\n");
		rtn = IPCAM_SETUP_RTN_FAILED;
		goto exit_err;
	}


	rtn = IPCAM_SETUP_RTN_DONE;
exit_err:
	NF_ONVIF_DBG(MAJOR,"END\n");

	return rtn;


}

// DEFINITIONS : STATIC FUNCTION
// ============================================================================

// MODIFY ANALYTICS MODULES
ONVIF_MSG _nf_onvif_analytics_modify_analytics_modules(
    ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule)
{
	int rtn = 0;
	int size, size_count;
	char buf[1024];


	NF_ONVIF_DBG(MAJOR,"START\n");
	struct soap *soap = NULL;
	struct _va__ModifyAnalyticsModules req;
	struct _va__ModifyAnalyticsModulesResponse res;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	struct _tt__ItemList_SimpleItem SimpleItem[2];
	memset(SimpleItem, 0x00, sizeof(SimpleItem));
	SimpleItem[0].Name = "Enable";
	SimpleItem[0].Value = "1";
	SimpleItem[1].Name = "DayNight";
	SimpleItem[1].Value = "1";


	struct tt__ItemList Parameters;
	memset(&Parameters, 0x00, sizeof(struct tt__ItemList));
	Parameters.__sizeSimpleItem = 2;
	Parameters.SimpleItem = SimpleItem;
	Parameters.__sizeElementItem = 0;
	Parameters.ElementItem = NULL;
	Parameters.Extension = NULL;
	Parameters.__anyAttribute;

	struct tt__Config AnalyticsModule;
	memset(&AnalyticsModule, 0x00, sizeof(struct tt__Config));
	AnalyticsModule.Name = "";
	AnalyticsModule.Type = "tanht:RegionMotionEngine";
	AnalyticsModule.Parameters = &Parameters;

	

	req.ConfigurationToken = token;
	req.__sizeAnalyticsModule = 1;
	req.AnalyticsModule = &AnalyticsModule;

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}

	

	rtn = soap_call___analytics2__ModifyAnalyticsModules(soap, endpoint, NULL, &req, &res);

	if(rtn !=0)
	{
		NF_ONVIF_DBG(WARN,"Modify Fail\n");
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"Fail Message:\n%s",buf);
		return 1;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}

ONVIF_MSG _nf_onvif_analytics_modify_analytics_modules_cells(
	ipcam_onvif_auth_info_t *auth_info,
    const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule, int sensitivity)
{
	int rtn = 0;
	int size, size_count;
	int i;
	char buf[1024];
	char sens[10];


	NF_ONVIF_DBG(MAJOR,"START\n");
	struct soap *soap = NULL;

	struct _va__GetAnalyticsModules get_req;
	struct _va__GetAnalyticsModulesResponse get_res;

	struct _va__ModifyAnalyticsModules req;
	struct _va__ModifyAnalyticsModulesResponse res;

	memset(&get_req, 0x00, sizeof(get_req));
	memset(&get_res, 0x00, sizeof(get_res));
	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	soap = soap_new();
	nf_onvif_soap_init_set(soap);


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);


	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}



	memset(&get_req, 0x00, sizeof(get_req));
	memset(&get_res, 0x00, sizeof(get_res));

	get_req.ConfigurationToken = token;
	rtn = soap_call___analytics2__GetAnalyticsModules(soap, endpoint, NULL, &get_req, &get_res);
	if(rtn !=0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"\n%s",buf);
		goto ends_label;
	}


	struct tt__Config *cell_conf;

	for(i = 0; i < get_res.__sizeAnalyticsModule; i++) {
		if(strstr(get_res.AnalyticsModule[i].Type, ":CellMotionEngine") != NULL) {
			cell_conf = &get_res.AnalyticsModule[i];
			break;
		}
	}


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}

	
	char *value_back;
	memset(sens, 0x00, sizeof(sens));
	sprintf(sens, "%d",sensitivity);

	for(i = 0; i < cell_conf->Parameters->__sizeSimpleItem; i++) {
		if(strcmp(cell_conf->Parameters->SimpleItem[i].Name, "Sensitivity") == 0) {
			value_back = cell_conf->Parameters->SimpleItem[i].Value;
			cell_conf->Parameters->SimpleItem[i].Value = sens;
		}
	}


	req.ConfigurationToken = token;
	req.__sizeAnalyticsModule = 1;
	req.AnalyticsModule = cell_conf;


	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}



	rtn = soap_call___analytics2__ModifyAnalyticsModules(soap, endpoint, NULL, &req, &res);

	cell_conf->Parameters->SimpleItem[i].Value = value_back;
	if(rtn !=0)
	{
		NF_ONVIF_DBG(WARN,"Modify Fail\n");
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"Fail Message:\n%s",buf);
		goto ends_label;
		
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}

// MODIFY RULE 

#if 1  // lagacy set motion
ONVIF_MSG _nf_onvif_analytics_modify_rules(
	ipcam_onvif_auth_info_t* auth_info,
    const char* endpoint, char* token, NFIPCamSetupMotionArea *info)
{
	int rtn = 0, size, size_count;
	unsigned int sense_d;
	char key[64];
	char buf[1024];
	
	//struct soap soap;
	struct soap *soap = NULL;
	struct _va__ModifyRules req;
	struct _va__ModifyRulesResponse res;

	struct _tt__ItemList_SimpleItem SimpleItem[10];

	struct tt__Config Rule;
	struct tt__ItemList itemList;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	memset(SimpleItem, 0x00, sizeof(SimpleItem));
	memset(&Rule, 0x00, sizeof(Rule));
	memset(&itemList, 0x00, sizeof(itemList));

	char left_top_x[3];
	char left_top_y[3];
	char right_bottom_x[3];
	char right_bottom_y[3];
	char sensitivity_d[3];
	char sensitivity_n[3];

	NF_ONVIF_DBG(MAJOR,"START\n");

	/* ------------------------------- */
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}

	// ====== Debug Print
	{
		char key[64];
		unsigned int sense_d;
		unsigned int sense_n;

		snprintf(key, 64, "alarm.motion.M%d.sense_d", info->ch);
		sense_d = nf_sysdb_get_uint(key);
		NF_ONVIF_DBG(MINOR,"@Sense_d: %u, @info.ch: %u\n",sense_d,info->ch);

		snprintf(key, 64, "alarm.motion.M%d.sense_n", info->ch);
		sense_d = nf_sysdb_get_uint(key);
		NF_ONVIF_DBG(MINOR,"@Sense_n: %u, @info.ch: %u\n",sense_n,info->ch);

	}


	/* ------------------------------- */
	
	req.ConfigurationToken = token;

	req.__sizeRule = 1;


	Rule.Name = "New";
	Rule.Type = "tanht:RegionMotionDetector";


	itemList.__sizeSimpleItem = 10;
	itemList.__sizeElementItem = 0;
	itemList.ElementItem = NULL;
	itemList.Extension = NULL;
//	itemList.__anyAttribute = NULL;


	SimpleItem[0].Name = "Type";
	SimpleItem[0].Value = "1";

	// Sensitivity Date
	SimpleItem[1].Name = "Sensitivity";		
	//sprintf(sensitivity,"%d",info->marea[0].sensitivity);
	snprintf(key, 64, "alarm.motion.M%d.sense_d", info->ch);
	sense_d = nf_sysdb_get_uint(key);
	sprintf(sensitivity_d,"%d",sense_d);
	SimpleItem[1].Value = sensitivity_d;

	//Threshold
	SimpleItem[2].Name = "Threshold";		
	SimpleItem[2].Value = "7";

	//Dwell
	SimpleItem[3].Name = "Dwell";			
	SimpleItem[3].Value = "1";

	//Left
	SimpleItem[4].Name = "Left";			
	sprintf(left_top_x,"%d",info->marea[0].FIGURE.RECTANGLE.left_top.x);
	SimpleItem[4].Value = left_top_x;

	//Right
	SimpleItem[5].Name = "Right";			
	sprintf(right_bottom_x,"%d",(info->marea[0].FIGURE.RECTANGLE.right_bottom.x+1));
	SimpleItem[5].Value = right_bottom_x;

	//Bottom
	SimpleItem[6].Name = "Bottom";			
	sprintf(right_bottom_y,"%d",(info->marea[0].FIGURE.RECTANGLE.right_bottom.y+1));
	SimpleItem[6].Value = right_bottom_y;

	// Top 
	SimpleItem[7].Name = "Top";				
	SimpleItem[7].Value = "1";
	sprintf(left_top_y,"%d",info->marea[0].FIGURE.RECTANGLE.left_top.y);
	SimpleItem[7].Value = left_top_y;

	// Sensitivity Night
	SimpleItem[8].Name = "NightSensitivity";
	snprintf(key, 64, "alarm.motion.M%d.sense_n", info->ch);
	sense_d = nf_sysdb_get_uint(key);
	sprintf(sensitivity_n,"%d",sense_d);
	SimpleItem[8].Value = sensitivity_n;

	// Night Tight Threshold
	SimpleItem[9].Name = "NightThreshold";	
	SimpleItem[9].Value = 0;

	req.Rule = &Rule;
	req.Rule->Parameters = &itemList;
	req.Rule->Parameters->SimpleItem = SimpleItem;

	rtn = soap_call___analytics1__ModifyRules(soap, endpoint, NULL, &req, &res);

	if(rtn !=0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"%s",buf);
		return 1;
	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;

}

ONVIF_MSG _nf_onvif_analytics_modify_rules_cells(
    ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token, NFIPCamSetupMotionArea *info, unsigned char *cells)
{
	int rtn = 0, size, size_count;
	int i;
	int size_rule = 0;
	unsigned int sense_d;
	char key[64];
	char buf[1024];
	char rule_name[64];
	
	//struct soap soap;
	struct soap *soap = NULL;
	struct _va__ModifyRules req;
	struct _va__ModifyRulesResponse res;

	struct _tt__ItemList_SimpleItem SimpleItem[1];

	struct tt__Config Rule;
	struct tt__ItemList itemList;

	struct tt__Config *rule = NULL;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));
	memset(SimpleItem, 0x00, sizeof(SimpleItem));
	memset(&Rule, 0x00, sizeof(Rule));
	memset(&itemList, 0x00, sizeof(itemList));


	NF_ONVIF_DBG(MAJOR,"START\n");

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	// 
	// ONVIF AUTH
	//
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}

	//
	// DEBUG PRINT
	//
	{
		char key[64];
		unsigned int sense_d;
		unsigned int sense_n;

		snprintf(key, 64, "alarm.motion.M%d.sense_d", info->ch);
		sense_d = nf_sysdb_get_uint(key);
		NF_ONVIF_DBG(MINOR,"@Sense_d: %u, @info.ch: %u\n",sense_d,info->ch);

		snprintf(key, 64, "alarm.motion.M%d.sense_n", info->ch);
		sense_n = nf_sysdb_get_uint(key);
		NF_ONVIF_DBG(MINOR,"@Sense_n: %u, @info.ch: %u\n",sense_n,info->ch);

	}

	struct _va__GetRules get_req;
	struct _va__GetRulesResponse get_res;

	memset(&get_req, 0x00, sizeof(get_req));
	memset(&get_res, 0x00, sizeof(get_res));

	get_req.ConfigurationToken = token;

	rtn = soap_call___analytics1__GetRules(soap, endpoint, NULL, &get_req, &get_res);
	if(rtn !=0) {
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"%s",buf);
		goto ends_label;
	}

	if(get_res.__sizeRule == 0 || get_res.Rule == NULL)
		goto ends_label;


	size_rule = get_res.__sizeRule;

	for(i = 0; i < size_rule; i++) {
		if(get_res.Rule[i].Name != NULL && get_res.Rule[i].Type != NULL) {
			if(strstr(get_res.Rule[i].Type, ":CellMotionDetector") != NULL) {
				strncpy(rule_name, get_res.Rule[i].Name, 64);
				break;
			}
		}
	}

	rule = &get_res.Rule[i];




	// 
	// ONVIF AUTH
	//
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}

	/* ------------------------------- */
	memset(&itemList, 0x00, sizeof(itemList));
	memset(&Rule, 0x00, sizeof(Rule));
	memset(&req, 0x00, sizeof(req));
	memset(SimpleItem, 0x00, sizeof(SimpleItem));

	
	// CONFIGURATION TOKEN
	req.ConfigurationToken = token;
	
	// RULE SIZE
	req.__sizeRule = 1;


	char *value_back;

	// Active Cells Set

	for(i = 0; i < rule->Parameters->__sizeSimpleItem; i++) {
		if(strcmp(rule->Parameters->SimpleItem[i].Name, "ActiveCells") == 0) {
			value_back = rule->Parameters->SimpleItem[i].Value;
			rule->Parameters->SimpleItem[i].Value = cells;
			break;
		}
	}
#if 0
	req.Rule = &Rule;
	req.Rule->Parameters = &itemList;
	req.Rule->Parameters->SimpleItem = SimpleItem;
#endif
	req.Rule = rule;

	rtn = soap_call___analytics1__ModifyRules(soap, endpoint, NULL, &req, &res);

	if(rtn !=0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"%s",buf);
		goto ends_label;
	}

	rule->Parameters->SimpleItem[i].Value = value_back ;


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;


}

#endif


// GET ANALYTICS MODULES
ONVIF_MSG _nf_onvif_analytics_get_analytics_modules(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, char* token, struct _AnalyticsModules *analyticsModule)
{	
	int rtn = 0, size, size_count;
	char buf[1024];
	
	struct soap *soap = NULL;
	struct _va__GetAnalyticsModules req;
	struct _va__GetAnalyticsModulesResponse res;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));


	NF_ONVIF_DBG(MAJOR,"START\n");

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
	}
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto exit_end;
		}
	}


	req.ConfigurationToken = token;

	rtn = soap_call___analytics2__GetAnalyticsModules(soap, endpoint, NULL, &req, &res);
	if(rtn !=0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"\n%s",buf);
		goto exit_end;
	}

	NF_ONVIF_DBG(MINOR,"AnalyticsModule Size: %d\n", res.__sizeAnalyticsModule);

	if(0 < res.__sizeAnalyticsModule && res.AnalyticsModule != NULL)
//	for(size_count = 0; size_count < res.__sizeAnalyticsModule; size_count++)
	{
		struct tt__ItemList *para 			 = res.AnalyticsModule[0].Parameters;

		int count;

		NF_ONVIF_DBG(MINOR,"Analytics | Name(%s), Type(%s)\n", res.AnalyticsModule[0].Name, res.AnalyticsModule[0].Type);

		for(count = 0; count < para->__sizeSimpleItem ; count++)
		{
			NF_ONVIF_DBG(MINOR,"SimpleItem[%d] | Name(%s), Value(%s)\n",count,(para->SimpleItem+count)->Name,(para->SimpleItem+count)->Value);
			if(strcmp((para->SimpleItem+count)->Name,"Enable") == 0)
			{
				NF_ONVIF_DBG(MINOR,"Enalbe:%d\n",atoi((para->SimpleItem+count)->Value));
				analyticsModule->Enable = atoi((para->SimpleItem+count)->Value);
			}
			if(strcmp((para->SimpleItem+count)->Name,"DayNight") == 0)
			{
				NF_ONVIF_DBG(MINOR,"DayNight:%d\n",atoi((para->SimpleItem+count)->Value));
				analyticsModule->DayNight = atoi((para->SimpleItem+count)->Value);
			}
		}


		for(count = 0; count < para->__sizeElementItem; count++)
		{
			if(strcmp(para->ElementItem[count].Name,"Layout") == 0)
			{
				_cell_layout_dom_parser(&para->ElementItem[count].__any, analyticsModule);
			}
		}
	
	}
exit_end:

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return rtn;

}

ONVIF_MSG _nf_analytics_rules_remove_all(
	ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token)
{
	int rtn;			// Return Value
	int rule_cnt;		// Rules Count
	int sizeRules;
	char buf[1024];		// SOAP Error Message Buffer


	struct soap *soap = NULL;

	struct _va__GetRules			req;
	struct _va__GetRulesResponse	res;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	NF_ONVIF_DBG(MAJOR,"START\n");

	// soap init
	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	// auth
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0 )
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
	}
	// endpoint check
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
		}
	}


	// Setting Request
	req.ConfigurationToken = token;

	// Send Request
	rtn = soap_call___analytics1__GetRules(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"\n%s",buf);
		goto ends_label;
	}

	 
	{
		char *rule_name = (char*)malloc(25);
		char **rules = &rule_name;

		// Responsed Rules Delete
		sizeRules = res.__sizeRule;
		NF_ONVIF_DBG(MINOR,"Remove Rule size: %d\n",sizeRules);
		//for(rule_cnt = 0; rule_cnt < sizeRules; rule_cnt++)
		for(rule_cnt = sizeRules; rule_cnt != 0; rule_cnt--)
		{
			memset(rule_name , 0x00, 25);
			strcpy(rule_name, (res.Rule + (rule_cnt-1))->Name);
			NF_ONVIF_DBG(MINOR,"Remove Rule List[%d] : %s\n",rule_cnt-1, *rules);
			rtn = _nf_onvif_analytics_rules_remove(auth_info, endpoint,  token, rules);
			if(rtn != 0)
			{
				// Remove Analytics Rules Faile
			}
		}

		if(rule_name != NULL)
		{
			free(rule_name);
			rule_name = NULL;
		}

	}


ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
	
}

static int _cell_layout_string_parser(const char *str, struct _AnalyticsModules *analyticsModule)
{
	char data[5];


	memset(data, 0x00, sizeof(data));

	if(_get_string_value(str,"Columns", data) == NULL)
	{
		return 1;
	}
	strcpy(analyticsModule->Columns, data);


	memset(data, 0x00, sizeof(data));
	if(_get_string_value(str,"Rows", data) == NULL)
	{
		return 1;
	}
	strcpy(analyticsModule->Rows, data);

	return 1;

}

static int _cell_layout_dom_parser(struct soap_dom_element *el, struct _AnalyticsModules *analyticsModule)
{
	struct soap_dom_attribute *att = NULL;
	if(el != NULL)
	{
		NF_ONVIF_DBG(MINOR,"Element Name: [%s] \n", el->name);
		if(strcmp(el->name,"CellLayout") == 0)
		{
			att = el->atts;
			while(att != NULL)
			{
				if(strcmp(att->name, "Rows") == 0)
				{
					strcpy(analyticsModule->Rows, att->data);

				}
				else if(strcmp(att->name, "Columns") == 0)
				{
					strcpy(analyticsModule->Columns, att->data);
				}
				att = att->next;
			}
		}
	}

}

static void* _get_string_value(const char *str, const char *find_str, char *value)
{
	char *str1, *str2;

	str1 = strstr(str, find_str);
	if(str1 == NULL)
	{
		return NULL;
	}
	str1 = strstr(str1, "\"");
	if(str1 == NULL)
	{
		return NULL;
	}

	str1 +=1;
	str2 = strstr(str1, "\"");
	if(str2 == NULL)
	{
		return NULL;
	}
	str1 = strncpy(value, str1,(size_t)(str2 - str1));
	if(str1 == NULL)
	{
		return NULL;
	}
	return str1;

}


// RULE REMOVE
ONVIF_MSG _nf_onvif_analytics_rules_remove(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, char* token, char **rule)
{
	int rtn;			// Return Value
	char buf[1024];		// SOAP Error Message Buffer

	struct soap *soap = NULL;
	struct _va__DeleteRules			req;
	struct _va__DeleteRulesResponse	res;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	NF_ONVIF_DBG(MINOR,"START\n");
	// soap init
	soap = soap_new();

	nf_onvif_soap_init_set(soap);

	// auth
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);
	if(rtn != 0 )
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
	}

	// endpoint check
	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
		}
	}


	// request setting
	req.ConfigurationToken = token;
	req.__sizeRuleName = 1;
	req.RuleName = rule;
	rtn = soap_call___analytics1__DeleteRules(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		soap_sprint_fault(soap, buf, 1024);
		NF_ONVIF_DBG(WARN,"\n%s",buf);
		return 1;
	}

ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;

}


// CREATE RULE OF SET MOTION
ONVIF_MSG _nf_analytics_rules_create(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token, NFIPCamSetupMotionArea *info)
{

	int rtn, cnt; 				// Return Value
	MAREA *marea;
	char rule_name[4] = {'N','e','w','\0'};

	NF_ONVIF_DBG(MAJOR,"START\n");
	NF_ONVIF_DBG(MINOR,"create info->area_num = %d\n",info->area_num);


	for(cnt = 0; cnt < info->area_num; cnt++)
	{
		marea = &info->marea[cnt];

		_nf_onvif_analytics_rules_create(auth_info, endpoint, token, rule_name, marea, info->ch);
		rule_name[2]--;
	}

	nf_ipcam_setup_waiting(info->ch, NF_IPCAM_TYPE_SET_MOTION, -1);
	
	return 0;
}

// CREATE WINDOW RULES
ONVIF_MSG _nf_analytics_rules_create_window(ipcam_onvif_auth_info_t* auth_info, const char* endpoint, char* token, NFIPCamSetupMotionArea *info)
{

	int rtn, cnt; 				// Return Value
	struct __MOTION_WINDOW_T_ window;
	MAREA *marea = NULL;

	memset(&window, 0x00, sizeof(window));

	NF_ONVIF_DBG(MAJOR,"START\n");
	NF_ONVIF_DBG(MINOR,"create info->area_num = %d\n",info->area_num);


	for(cnt = 0; cnt < info->area_num; cnt++)
	{
		marea = &info->marea[cnt];

		rtn = rect_to_window(info->block_height, info->block_width, &info->marea[0], &window);

		_nf_onvif_analytics_rules_create_window(auth_info, endpoint, token, cnt, &window, marea->sensitivity, info->ch);
	}

	nf_ipcam_setup_waiting(info->ch, NF_IPCAM_TYPE_SET_MOTION, -1);
	
	return 0;
}

// CREATE RULE
ONVIF_MSG _nf_onvif_analytics_rules_create(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, char* token, char *rule_name, MAREA *marea, int ch)
{

	int rtn; 				// Return Value
	unsigned int sense;		// Sensitivity
	char buf[1024];			// SOAP Error Message Buffer
	char key[64];			// Enviroment Value Key
	char value[10][5];

	struct soap *soap = NULL;

	struct _va__CreateRules req;
	struct _va__CreateRulesResponse res;

	memset(&req, 0x00, sizeof(req));
	memset(&req, 0x00, sizeof(res));


	NF_ONVIF_DBG(MAJOR,"START\n");

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	// auth
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}


	// Create Rule Item
	struct _tt__ItemList_SimpleItem SimpleItem[10];
	memset(SimpleItem, 0x00, sizeof(SimpleItem));

	// Setting
	// Type
	SimpleItem[0].Name = "Type";			
	SimpleItem[0].Value = "1";

	//Sensitivity
	{
#if 1
		if (get_dn_now(ch) == 1)
		{
			snprintf(key, 64, "alarm.motion.M%d.sense_d", ch);
		}
		else
		{
			snprintf(key, 64, "alarm.motion.M%d.sense_n", ch);
		}
		sense = nf_sysdb_get_uint(key);
#else
		sense = mare->sensitivity;

#endif

		SimpleItem[1].Name = "Sensitivity";
		sprintf(value[1],"%d",sense);
		SimpleItem[1].Value = value[1];
	}

	//Threshold
	{
		SimpleItem[2].Name = "Threshold";		
		SimpleItem[2].Value = "7";
	}

	//Dwell
	{
		SimpleItem[3].Name = "Dwell";			
		SimpleItem[3].Value = "1";
	}

	//Left
	{
		sprintf(value[4],"%d",marea->FIGURE.RECTANGLE.left_top.x);
		SimpleItem[4].Name = "Left";			
		SimpleItem[4].Value = value[4];
	}

	//Right
	{
		sprintf(value[5],"%d",(marea->FIGURE.RECTANGLE.right_bottom.x+1));
		SimpleItem[5].Name = "Right";			
		SimpleItem[5].Value = value[5];
	}


	//Bottom
	{
		sprintf(value[6],"%d",(marea->FIGURE.RECTANGLE.right_bottom.y+1));
		SimpleItem[6].Name = "Bottom";			
		SimpleItem[6].Value = value[6];
	}

	//top
	{
		
		sprintf(value[7],"%d",marea->FIGURE.RECTANGLE.left_top.y);
		SimpleItem[7].Name = "Top";				
		SimpleItem[7].Value = value[7];
	}

	//NightSensitivity
	{
		unsigned int sense_n;
		snprintf(key, 64, "alarm.motion.M%d.sense_n", ch);
		sense_n = nf_sysdb_get_uint(key);
		sprintf(value[8],"%d",sense_n);

		SimpleItem[8].Name = "NightSensitivity";
		SimpleItem[8].Value = value[8];
	}


	//NightThreshold
	SimpleItem[9].Name = "NightThreshold";	
	SimpleItem[9].Value = "7";

	// Create Parameter
	struct tt__ItemList Parameter;
	memset(&Parameter, 0x00, sizeof(Parameter));
	Parameter.__sizeSimpleItem = 10;
	Parameter.SimpleItem = SimpleItem;
	Parameter.__sizeElementItem = 0;
	Parameter.ElementItem = NULL;
	Parameter.Extension = NULL;

	// Rule Create 
	struct tt__Config Rule;
	memset(&Rule, 0x00, sizeof(Rule));

	Rule.Parameters = &Parameter;
	Rule.Name = rule_name;
	Rule.Type = "tanht:RegionMotionDetector";

	// Request Setting
	req.ConfigurationToken = token;
	req.__sizeRule = 1;
	req.Rule = &Rule;

	rtn = soap_call___analytics1__CreateRules(soap, endpoint, NULL, &req, &res);
	if(rtn !=0)
	{
		NF_ONVIF_DBG(WARN,"Faile\n");
		soap_sprint_fault(soap, buf, 1024);
		puts(buf);
		return 1;
	}

	NF_ONVIF_DBG(MAJOR,"END\n");
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}

ONVIF_MSG _nf_onvif_analytics_rules_create_window(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, char* token, int index, struct __MOTION_WINDOW_T_ *window, int sensitivity, int ch)
{

	int rtn; 				// Return Value
	unsigned int sense;		// Sensitivity
	char buf[1024];			// SOAP Error Message Buffer
	char key[64];			// Enviroment Value Key
	char value[10][5];
	struct __RULE_INFO_T_ {
		char width[4];
		char height[4];
		char x[4];
		char y[4];
		char index[4];
		char sensitivity[4];
	};
	typedef struct __RULE_INFO_T_ rule_t;

	struct soap *soap = NULL;

	struct _va__CreateRules req;
	struct _va__CreateRulesResponse res;

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));

	//char rule_name[] = "tt:MotionDetector";
	//char rule_type[] = "tt:MotionDetector";

	NF_ONVIF_DBG(MAJOR,"START\n");

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	// auth
	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if (rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto ends_label;
	}

	if(strstr(endpoint, "https://") != NULL)
	{
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);

		if (rtn != 0)
		{
			char buf[1024];
			soap_sprint_fault(soap, buf, 1024);
			NF_ONVIF_DBG(WARN,"ERROR - %s\n", buf);
			goto ends_label;
		}
	}
	rule_t rule_info;

	memset(&rule_info, 0x00, sizeof(rule_info));
	snprintf(rule_info.width, 4, "%d", window->width);
	snprintf(rule_info.height, 4, "%d", window->height);
	snprintf(rule_info.x, 4, "%d", window->x);
	snprintf(rule_info.y, 4, "%d", window->y);
	snprintf(rule_info.index, 4, "%d", index);
	snprintf(rule_info.sensitivity, 4, "%d", sensitivity);

	NF_ONVIF_DBG(MINOR,"rule_info.width(%s)\n",rule_info.width);
	NF_ONVIF_DBG(MINOR,"rule_info.height(%s)\n",rule_info.height);
	NF_ONVIF_DBG(MINOR,"rule_info.x(%s)\n",rule_info.x);
	NF_ONVIF_DBG(MINOR,"rule_info.y(%s)\n",rule_info.y);
	NF_ONVIF_DBG(MINOR,"rule_info.index(%s)\n",rule_info.index);
	NF_ONVIF_DBG(MINOR,"rule_info.sensitivity(%s)\n",rule_info.sensitivity);


	// IntRectangle Attribute
	struct soap_dom_attribute atts[4];
	memset(&atts, 0x00, sizeof(atts));
	atts[0].name = "height";
	atts[0].data = rule_info.height;
	atts[0].next = &atts[1];

	atts[1].name = "width";
	atts[1].data = rule_info.width;
	atts[1].next = &atts[2];

	atts[2].name = "x";
	atts[2].data = rule_info.x;
	atts[2].next = &atts[3];

	atts[3].name = "y";
	atts[3].data = rule_info.y;
	atts[3].next = NULL;

	// IntRectangle Element
	struct _tt__ItemList_ElementItem element;
	memset(&element, 0x00, sizeof(element));
	element.Name = "DetectWindow";
	element.__any.atts = atts;
	element.__any.name = "IntRectangle";
	element.__any.nstr = "http://www.onvif.org/ver10/schema";
	element.__any.soap = &soap;

	// SimpleItem
	struct _tt__ItemList_SimpleItem item[2];
	memset(&item, 0x00, sizeof(item));
	item[0].Name = "Index";
	item[0].Value = rule_info.index;
	item[1].Name = "Sensitivity";
	item[1].Value = rule_info.sensitivity;


	// Rules Parameter 
	struct tt__ItemList param;
	memset(&param, 0x00, sizeof(param));
	param.__sizeSimpleItem = 2;
	param.SimpleItem = item;
	param.__sizeElementItem = 1;
	param.ElementItem = &element;

	// Rules
	struct tt__Config rule;
	memset(&rule, 0x00, sizeof(rule));
	rule.Name = "tt:MotionDetector";
	rule.Type = "tt:MotionDetector";
	rule.Parameters = &param;


	// Request Set
	req.ConfigurationToken = token;
	req.__sizeRule = 1;
	req.Rule = &rule;




	rtn = soap_call___analytics1__CreateRules(soap, endpoint, NULL, &req, &res);
	if(rtn !=0)
	{
		NF_ONVIF_DBG(WARN,"Faile\n");
		soap_sprint_fault(soap, buf, 1024);
		puts(buf);
		return 1;
	}

	NF_ONVIF_DBG(MAJOR,"END\n");
ends_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}

// GET SERVICE CAPABILITIES
ONVIF_MSG _nf_onvif_analytics_get_service_capabilities(
	ipcam_onvif_auth_info_t* auth_info,
	const char* endpoint, int att[])
{
	struct soap *soap = NULL;

	struct _va__GetServiceCapabilities req;
	struct _va__GetServiceCapabilitiesResponse res;

	int rtn;
	char buf[1024];

	memset(&req, 0x00, sizeof(req));
	memset(&res, 0x00, sizeof(res));


	NF_ONVIF_DBG(MAJOR,"START\n");

	soap = soap_new();
	nf_onvif_soap_init_set(soap);

	rtn = _nf_onvif_add_auth(soap, auth_info->auth_method, auth_info->username, auth_info->password, auth_info->endpoint);

	if(rtn != 0)
	{
		NF_ONVIF_DBG(WARN,"ERROR: Add Security(%d)\n", rtn);
		goto end_label;
	}

	if(strstr(endpoint, "https://") != NULL)
	{		
		rtn = SOAP_SSL_CLIENT_CONTEXT(soap, SOAP_SSL_NO_AUTHENTICATION,
				NULL, NULL, NULL, NULL, NULL);
		if (rtn != 0)
		{
			NF_ONVIF_DBG(WARN,"ERROR \n");
			soap_print_fault(soap, stderr);
			goto end_label;
		}
	}
	


	rtn = soap_call___analytics2__GetServiceCapabilities(soap, endpoint, NULL, &req, &res);
	if(rtn != 0)
	{
		soap_print_fault(soap, stderr);
		goto end_label;
	}

	if(res.Capabilities != NULL)
	{
		att[CAP_ATTRIBUTE_RULE] = *res.Capabilities->RuleSupport;
		att[CAP_ATTRIBUTE_MODULE] = *res.Capabilities->AnalyticsModuleSupport;
		att[CAP_ATTRIBUTE_CELLBASE] = *res.Capabilities->CellBasedSceneDescriptionSupported;
	}


	NF_ONVIF_DBG(MAJOR,"END\n");
end_label:
	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);
	return rtn;
}


//
// Base64 and Packbit API
// ///////////////////////////////////////////////////////////////////////////


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'};

static int mod_table[] = {0, 2, 1};

static unsigned char set_bit[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};


// BASE64 ENCODE
// ///////////////////////////////////////////////////////////////////////////
static char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length) 
{
	unsigned int i;
	unsigned int j;

	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = malloc((*output_length)+1);
	memset(encoded_data, 0x00, (*output_length)+1);

	if (encoded_data == NULL) return NULL;

	for (i = 0, j = 0; i < input_length;) {

		unsigned int octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for ( i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}

// PACKBITS COMPRESS
// ////////////////////////////////////////////////////////////////////////////
static long packbits(char *src,char *dst,long n)
{
	char *p,*q,*run,*dataend,s[0x100];
	int count,maxrun;

#if 0
	/* trivial uncompressed PackBits format, for testing */
	*dst++ = n-1;
	memcpy(dst,src,n);
	return n+1;
#else
	dataend = src + n;
	for( run = src,q = dst ; n > 0 ; run = p, n -= count ){
		maxrun = n < 128 ? n : 128;
		if(run <= (dataend-3) && run[1] == run[0] && run[2] == run[0]){
			for( p = run+3 ; p < (run+maxrun) && *p == run[0] ; )
				++p;
			count = p-run;
			*q++ = 1+256-count; /* flag byte */
			*q++ = run[0];
		}else{
			for( p = run ; p < (run+maxrun) ; )
				if(p <= (dataend-3) && p[1] == p[0] && p[2] == p[0])
					break; // 3 bytes repeated end verbatim run
				else
					++p;
			count = p-run;
			*q++ = count-1; /* flag byte */
			memcpy(q,run,count);
			q += count;
		}
	}
	return q-dst;
#endif
}

// RECTANGLE TO CELLS
// ////////////////////////////////////////////////////////////////////////////

static unsigned char* rect_to_cells(int rows, int columns, int area_num, MAREA *marea, int *length)
{
	int i, j;
	int index		= 0;
	int bit			= 0;
	int count=0;
	int size;
	unsigned char *cells;

	int ltx;	// left top x
	int lty;	// left top y
	int rbx;	// right bottom x
	int rby;	// right bottom y

	if( marea->FIGURE.RECTANGLE.left_top.x < 0 || marea->FIGURE.RECTANGLE.left_top.y < 0 || marea->FIGURE.RECTANGLE.right_bottom.x < 0 || marea->FIGURE.RECTANGLE.right_bottom.y < 0)
	{
		cells = NULL;
		count = 0;
		NF_ONVIF_DBG(WARN,"x < 0 && y < 0\n");
		goto end_err;
	}
	if( marea->FIGURE.RECTANGLE.left_top.x > columns || marea->FIGURE.RECTANGLE.left_top.y > rows || marea->FIGURE.RECTANGLE.right_bottom.x > columns || marea->FIGURE.RECTANGLE.right_bottom.y > rows)
	{
		cells = NULL;
		count = 0;

		NF_ONVIF_DBG(WARN,"x > columns && y > rows\n");

		goto end_err;
	}

	ltx = marea->FIGURE.RECTANGLE.left_top.x +1;
	lty = marea->FIGURE.RECTANGLE.left_top.y +1;
	rbx = marea->FIGURE.RECTANGLE.right_bottom.x +1;
	rby = marea->FIGURE.RECTANGLE.right_bottom.y +1;

	size = (columns * rows) / 8;
	if( ((columns * rows) % 8) > 0 )
	{
		size += 1;
	}


	cells = (unsigned char *)calloc(sizeof(unsigned char) , (size_t) size);
	if(cells == NULL)
	{
		NF_ONVIF_DBG(WARN,"cells == null");
		goto end_err;
	}

	//memset(cells, 0x00, sizeof(unsigned char) * size);

	count = 0;
	for(i = 1; i <= rows; i++)
	{
		for(j = 1; j <= columns; j++)
		{

			if((ltx <= j && j <= rbx) && (lty <= i && i <=rby) )
			{
				index = count / 8;
				bit   = count % 8;
				cells[index] |= set_bit[bit];
			}

			count++;
		}
	}

	*length = size;

	if(area_num == 0)
		memset(cells, 0x00, size);

end_err:


	return cells;

}

static int rect_to_window(int rows, int columns, MAREA *marea, struct __MOTION_WINDOW_T_ *window)
{

	int x, y, width, height; // motion window
	int ltx;	// left top x
	int lty;	// left top y
	int rbx;	// right bottom x
	int rby;	// right bottom y

	if( marea->FIGURE.RECTANGLE.left_top.x < 0 || marea->FIGURE.RECTANGLE.left_top.y < 0 || marea->FIGURE.RECTANGLE.right_bottom.x < 0 || marea->FIGURE.RECTANGLE.right_bottom.y < 0)
	{
		NF_ONVIF_DBG(WARN,"x < 0 && y < 0\n");
		goto end_err;
	}
	if( marea->FIGURE.RECTANGLE.left_top.x > columns || marea->FIGURE.RECTANGLE.left_top.y > rows || marea->FIGURE.RECTANGLE.right_bottom.x > columns || marea->FIGURE.RECTANGLE.right_bottom.y > rows)
	{

		NF_ONVIF_DBG(WARN,"x > columns && y > rows\n");

		goto end_err;
	}

	ltx = marea->FIGURE.RECTANGLE.left_top.x;
	lty = marea->FIGURE.RECTANGLE.left_top.y;

	rbx = marea->FIGURE.RECTANGLE.right_bottom.x;
	rby = marea->FIGURE.RECTANGLE.right_bottom.y;

	x 		= ltx * 16;
	y 		= lty * 16;
	width 	= (rbx - ltx + 1) * 16;
	height 	= (rby - lty + 1) * 16;

	window->x = x;
	window->y = y;
	window->width = width;
	window->height = height;

	return 0;

end_err:
	return 1;

}

#endif // __NF_ONVIF_ANALYTICS_C__
