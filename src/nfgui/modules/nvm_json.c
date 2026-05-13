/*
 * json.c
 * 	- json api for UI
 *	- dependency :
 *		
 *		
 *
 * Written by Jeong-Ho Yang. <yanggungg@itxm2m.com>
 * Copyright (c) ITX security, Sept 8, 2017
 *
 */
/*
{
    "msg" : 0,                                              //NVM MSG
    "sender id" : "ADMIN",
    "sender pw" : "1",
    "sender addr" : "192.168.100.46",
    "sender port" : 8080,
    "system_id" : "SYSTEM ID",
    "active ch cnt" : GUI_CHANNEL_CNT,
    "model" : "TEST MODEL",
    "fwver" : "TEST VERSION",
    "sys_time" : 2039740,
    "camera list" : [
        {"ch" : 0, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 1, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 2, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 3, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 4, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 5, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 6, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 7, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 8, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 9, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 10, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 11, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 12, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 13, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 14, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 15, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 16, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 17, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 18, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 19, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 20, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 21, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 22, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 23, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 24, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 25, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 26, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 27, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 28, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 29, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 30, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""},
        {"ch" : 31, "model" : "ASO-2MP-1DHAR4", "url" : "192.168.100.36", "id" : "", "pw" : ""}
    ]
}
*/

#include <jansson.h>
#include "iux_afx.h"
#include "ix_func.h"
#include "nvm_json.h"
#include "nf_sysdb.h"
#include "scm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"NVM_JSON"


static void _insert_sender(json_t *root)
{
	char sender_id[65];
	char sender_pw[33];
	char sender_addr[256];
	int sender_port = 0;
	GValue ret_value = {0,};

	if (root == NULL) return;

	memset(sender_id, 0x00, sizeof(sender_id));	
	memset(sender_pw, 0x00, sizeof(sender_pw));	
	memset(sender_addr, 0x00, sizeof(sender_addr));	

	if(nf_sysdb_get_key0("usr.U0.name", &ret_value, NULL))
	{
		g_stpcpy(sender_id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
		g_value_unset(&ret_value);

	if(nf_sysdb_get_key0("usr.U0.pass", &ret_value, NULL))
	{
		g_stpcpy(sender_pw, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
		g_value_unset(&ret_value);

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		sender_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		g_value_unset(&ret_value);
		
    scm_get_ip_addr_str(sender_addr, 256);

	json_object_set_new(root, "sender addr", json_string(sender_addr));
	json_object_set_new(root, "sender port", json_integer(sender_port));
	json_object_set_new(root, "sender id", json_string(sender_id));
	json_object_set_new(root, "sender pw", json_string(sender_pw));
}

static void _insert_msg(json_t *root, NVM_MSG msg)
{
	if (root == NULL) return;

	DMSG(9, "msg : %d", msg);
	json_object_set_new(root, "msg", json_integer(msg));
}

static void _insert_time(json_t *root, unsigned int time)
{
	if (root == NULL) return;

	DMSG(9, "time : %d", time);
	json_object_set_new(root, "sys_time", json_integer(time));
}

static void _extract_msg(json_t *root, NVM_MSG *msg)
{
	if (root == NULL) return;
	
    *msg = (int)json_integer_value(json_object_get(root, "msg"));
	DMSG(9, "msg : %d", *msg);
}

static unsigned int _extract_time(json_t *root)
{
    unsigned int time = 0;
    
	if (root == NULL) return;
	
    time = (unsigned int)json_integer_value(json_object_get(root, "sys_time"));
	DMSG(9, "time : %d", time);

	return time;
}

static void _insert_export_ndata(json_t *root, NVR_DATA_T *ndata)
{
    json_t *clist = NULL;
    json_t *cinfo = NULL;
	char buf[128] = {0,};
	int i;

	if (root == NULL) return;
	
    memset(buf, 0x00, sizeof(buf));
    ifn_get_localtime_text_ex(time(0), YYYYMMDD, H24, buf);
    json_object_set_new(root, "system time", json_string(buf));
    
	json_object_set_new(root, "channel count", json_integer(ndata->active_ch_cnt));
	json_object_set_new(root, "system id", json_string(ndata->sysid));
	json_object_set_new(root, "model", json_string(ndata->model));
	json_object_set_new(root, "firmware version", json_string(ndata->fwver));
	json_object_set_new(root, "camera list", json_array());

	clist = json_object_get(root, "camera list");
	for (i = 0; i < ndata->active_ch_cnt; i++)
	{
	    memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf) - 1, "cam%d", i);
		json_array_append_new(clist, json_object());

		cinfo = json_array_get(clist, i);
		json_object_set_new(cinfo, "ch", json_integer(ndata->cam_info[i].ch));
		json_object_set_new(cinfo, "http port", json_integer(ndata->cam_info[i].port));
		json_object_set_new(cinfo, "model", json_string(ndata->cam_info[i].model));
		json_object_set_new(cinfo, "hostname", json_string(ndata->cam_info[i].hostname));
		
		if (ndata->cam_info[i].virtual_camera) {
        		json_object_set_new(cinfo, "vcam", json_integer(ndata->cam_info[i].virtual_camera));
        		json_object_set_new(cinfo, "vcam_addr0", json_string(ndata->cam_info[i].vcam_rtsp_addr[0]));
        		json_object_set_new(cinfo, "vcam_addr1", json_string(ndata->cam_info[i].vcam_rtsp_addr[1]));
    		}
	}
}

static void _insert_ndata(json_t *root, NVR_DATA_T *ndata)
{
    json_t *clist = NULL;
    json_t *cinfo = NULL;
	char buf[32] = {0,};
	int i;

	if (root == NULL) return;
    
	json_object_set_new(root, "active ch cnt", json_integer(ndata->active_ch_cnt));
	json_object_set_new(root, "sys_time", json_integer(time(0)));
	json_object_set_new(root, "system_id", json_string(ndata->sysid));
	json_object_set_new(root, "model", json_string(ndata->model));
	json_object_set_new(root, "fwver", json_string(ndata->fwver));
	json_object_set_new(root, "camera list", json_array());

	clist = json_object_get(root, "camera list");
	for (i = 0; i < ndata->active_ch_cnt; i++)
	{
	    memset(buf, 0x00, sizeof(buf));
		snprintf(buf, sizeof(buf) - 1, "cam%d", i);
		json_array_append_new(clist, json_object());

		cinfo = json_array_get(clist, i);
		json_object_set_new(cinfo, "ch", json_integer(ndata->cam_info[i].ch));
		json_object_set_new(cinfo, "port", json_integer(ndata->cam_info[i].port));
		json_object_set_new(cinfo, "vcam", json_integer(ndata->cam_info[i].virtual_camera));
		json_object_set_new(cinfo, "state", json_integer(ndata->cam_info[i].state));
		json_object_set_new(cinfo, "id", json_string(ndata->cam_info[i].id));
		json_object_set_new(cinfo, "pw", json_string(ndata->cam_info[i].pw));
		json_object_set_new(cinfo, "model", json_string(ndata->cam_info[i].model));
		json_object_set_new(cinfo, "url", json_string(ndata->cam_info[i].url));
		json_object_set_new(cinfo, "hostname", json_string(ndata->cam_info[i].hostname));
		json_object_set_new(cinfo, "vcam_addr0", json_string(ndata->cam_info[i].vcam_rtsp_addr[0]));
		json_object_set_new(cinfo, "vcam_addr1", json_string(ndata->cam_info[i].vcam_rtsp_addr[1]));
	}
}

static void _extract_ndata(json_t *root, NVR_DATA_T *ndata)
{
    json_t *clist;
    json_t *cinfo;
    int i;
    
	if (root == NULL) return;
    
    ndata->active_ch_cnt = (int)json_integer_value(json_object_get(root, "active ch cnt"));
    ndata->http_port = (int)json_integer_value(json_object_get(root, "sender port"));
    
	if (json_string_value(json_object_get(root, "sender addr"))) strncpy(ndata->hostname, json_string_value(json_object_get(root, "sender addr")), sizeof(ndata->hostname) - 1);
	if (json_string_value(json_object_get(root, "system_id"))) strncpy(ndata->sysid, json_string_value(json_object_get(root, "system_id")), sizeof(ndata->sysid) - 1);
	if (json_string_value(json_object_get(root, "model"))) strncpy(ndata->model, json_string_value(json_object_get(root, "model")), sizeof(ndata->model) - 1);
	if (json_string_value(json_object_get(root, "fwver"))) strncpy(ndata->fwver, json_string_value(json_object_get(root, "fwver")), sizeof(ndata->fwver) - 1);

	clist = json_object_get(root, "camera list");

	for (i = 0; i < ndata->active_ch_cnt; i++)
	{
	    cinfo = json_array_get(clist, i);
        ndata->cam_info[i].ch = (int)json_integer_value(json_object_get(cinfo, "ch"));
        ndata->cam_info[i].port = (int)json_integer_value(json_object_get(cinfo, "port"));
        ndata->cam_info[i].virtual_camera = (int)json_integer_value(json_object_get(cinfo, "vcam"));
        ndata->cam_info[i].state = (int)json_integer_value(json_object_get(cinfo, "state"));
    	if (json_string_value(json_object_get(cinfo, "id"))) strncpy(ndata->cam_info[i].id, json_string_value(json_object_get(cinfo, "id")), sizeof(ndata->cam_info[i].id) - 1);
    	if (json_string_value(json_object_get(cinfo, "pw"))) strncpy(ndata->cam_info[i].pw, json_string_value(json_object_get(cinfo, "pw")), sizeof(ndata->cam_info[i].pw) - 1);
    	if (json_string_value(json_object_get(cinfo, "model"))) strncpy(ndata->cam_info[i].model, json_string_value(json_object_get(cinfo, "model")), sizeof(ndata->cam_info[i].model) - 1);
    	if (json_string_value(json_object_get(cinfo, "url"))) strncpy(ndata->cam_info[i].url, json_string_value(json_object_get(cinfo, "url")), sizeof(ndata->cam_info[i].url) - 1);
    	if (json_string_value(json_object_get(cinfo, "hostname"))) strncpy(ndata->cam_info[i].hostname, json_string_value(json_object_get(cinfo, "hostname")), sizeof(ndata->cam_info[i].hostname) - 1);
    	if (json_string_value(json_object_get(cinfo, "vcam_addr0"))) strncpy(ndata->cam_info[i].vcam_rtsp_addr[0], json_string_value(json_object_get(cinfo, "vcam_addr0")), sizeof(ndata->cam_info[i].vcam_rtsp_addr[0]) - 1);
    	if (json_string_value(json_object_get(cinfo, "vcam_addr1"))) strncpy(ndata->cam_info[i].vcam_rtsp_addr[1], json_string_value(json_object_get(cinfo, "vcam_addr1")), sizeof(ndata->cam_info[i].vcam_rtsp_addr[1]) - 1);
	}
}

NVM_MSG nvm_json_get_msg(char *jtext)
{
    json_t *root;
    json_error_t error;
    NVM_MSG msg;

    if (!jtext) {
        DMSG(9, "Data is NULL!");
        return NVM_MSG_MAX_CNT;
    }
    
    root = json_loads(jtext, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return NVM_MSG_MAX_CNT;
    }
    
    _extract_msg(root, &msg);

	json_decref(root);

    return msg;
}

// you must free after use
char *nvm_json_set_msg(NVM_MSG msg)
{
	json_t* _json_root = json_object();
	char *json_dump_data = NULL;

	_insert_msg(_json_root, msg);	
    _insert_sender(_json_root);

	json_dump_data = json_dumps(_json_root, JSON_COMPACT);
	json_decref(_json_root);

	return json_dump_data;
}

// you must free after use
char *nvm_json_set_time(NVM_MSG msg, unsigned int time)
{
	json_t* _json_root = json_object();
	char *json_dump_data = NULL;

	_insert_msg(_json_root, msg);	
    _insert_sender(_json_root);
    _insert_time(_json_root, time);

	json_dump_data = json_dumps(_json_root, JSON_COMPACT);
	json_decref(_json_root);

	return json_dump_data;
}

unsigned int nvm_json_get_time(char *jtext)
{
    json_t *root;
    json_error_t error;
    unsigned int time = 0;

    if (!jtext) {
        DMSG(9, "Data is NULL!");
        return NVM_MSG_MAX_CNT;
    }
    
    root = json_loads(jtext, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return NVM_MSG_MAX_CNT;
    }
    
    time = _extract_time(root);

	json_decref(root);

    return time;
}

// you must free after use
char *nvm_json_convert_ndata_to_json(NVR_DATA_T *ndata, NVM_MSG msg)
{
	json_t* _json_root = json_object();
	char *json_dump_data = NULL;

	if ( ndata == NULL ) {
    	json_decref(_json_root);
		return NULL;
	}

	_insert_msg(_json_root, msg);	
    _insert_sender(_json_root);
    _insert_ndata(_json_root, ndata);
    _insert_time(_json_root, time(0));

	json_dump_data = json_dumps(_json_root, JSON_COMPACT);
	json_decref(_json_root);

	return json_dump_data;
}

// you must free after use
NVR_DATA_T *nvm_json_convert_json_to_ndata(char *jtext)
{
    NVR_DATA_T *ndata;
    json_t *root;
    json_error_t error;
    int i;

    if (!jtext) {
        DMSG(9, "Data is NULL!");
        return NULL;
    }
    
    root = json_loads(jtext, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return NULL;
    }

    ndata = (NVR_DATA_T*)imalloc(sizeof(NVR_DATA_T));

    if (!ndata) {
        json_decref(root);
        return NULL;
    }

    _extract_ndata(root, ndata);

	json_decref(root);

    return ndata;
}

// you must free after use
void nvm_json_get_sender(char *jtext, char *sender_addr, int sender_addr_size, int *sender_port, char *sender_id, char *sender_pw)
{
    json_t *root;
    json_error_t error;

    if (!jtext) {
        DMSG(9, "Data is NULL!");
        return NULL;
    }
    
    root = json_loads(jtext, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return NULL;
    }

    *sender_port = (int)json_integer_value(json_object_get(root, "sender port"));
	if (json_string_value(json_object_get(root, "sender addr"))) strncpy(sender_addr, json_string_value(json_object_get(root, "sender addr")), sender_addr_size - 1);
	if (json_string_value(json_object_get(root, "sender id"))) strncpy(sender_id, json_string_value(json_object_get(root, "sender id")), 64);
	if (json_string_value(json_object_get(root, "sender pw"))) strncpy(sender_pw, json_string_value(json_object_get(root, "sender pw")), 32);

	json_decref(root);

    return;
}

int nvm_json_export_ndata(NVR_DATA_T *ndata, char *path)
{
	json_t* _json_root = json_object();
	int ret;

	if ( ndata == NULL )
		return -1;

    _insert_export_ndata(_json_root, ndata);

    ret = json_dump_file(_json_root, path, JSON_COMPACT);
	json_decref(_json_root);

	return ret;
}

