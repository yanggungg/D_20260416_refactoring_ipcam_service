/*
 * nvm_json.c
 * 	- json api for NVM
 *	- dependency :
 *		
 *		
 *
 * Written by Jeong-Ho Yang. <yanggungg@itxm2m.com>
 * Copyright (c) ITX security, Agu, 2020
 *
 */


#ifndef __NVM_JSON_H__
#define __NVM_JSON_H__

#include "nvm.h"

////////////////////////////////////////////////////////////
//
// public interfaces
//
NVM_MSG nvm_json_get_msg(char *jtext);
char *nvm_json_set_msg(NVM_MSG msg);
char *nvm_json_convert_ndata_to_json(NVR_DATA_T *ndata, NVM_MSG msg);
NVR_DATA_T *nvm_json_convert_json_to_ndata(char *jtext);
int nvm_json_export_ndata(NVR_DATA_T *ndata, char *path);
char *nvm_json_set_time(NVM_MSG msg, unsigned int time);
unsigned int nvm_json_get_time(char *jtext);

#endif
