/*
 * dvatext.c
 * 	- dva text module
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 21, 2018
 *
 */

#include "dvatext.h"
#include "nfdal.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"DVATEXT"



////////////////////////////////////////////////////////////
//
// private data type 
//




////////////////////////////////////////////////////////////
//
// private variable
//





////////////////////////////////////////////////////////////
//
// private functions
//

static gint _translate_to_uxitem(gchar *engitem, gchar *item, gchar *uxitem)
{
	if (!strcmp(item, "person")) {
		if (strstr(engitem, "person")) {strcpy(uxitem, "person"); return 1;}
	}
	else if (!strcmp(item, "bike")) {
		if (strstr(engitem, "bike")) {strcpy(uxitem, "bike"); return 1;}
	}
/*
	else if (!strcmp(item, "bicycle")) {
		if (strstr(engitem, "bicycle")) {strcpy(uxitem, "bike"); return 1;}
	}
	else if (!strcmp(item, "motorbike")) {
		if (strstr(engitem, "motorbike")) {strcpy(uxitem, "bike"); return 1;}
	}
*/	
	else if (!strcmp(item, "bus")) {
		if (strstr(engitem, "bus")) {strcpy(uxitem, "bus"); return 1;}
	}
	else if (!strcmp(item, "car")) {
		if (strstr(engitem, "car")) {strcpy(uxitem, "car"); return 1;}
	}
	else if (!strcmp(item, "truck")) {
		if (strstr(engitem, "truck")) {strcpy(uxitem, "truck"); return 1;}
	}	
	else if (!strcmp(item, "bird")) {
		if (strstr(engitem, "bird")) {strcpy(uxitem, "animal"); return 1;}
	}
	else if (!strcmp(item, "cat")) {
		if (strstr(engitem, "cat")) {strcpy(uxitem, "animal"); return 1;}
	}
	else if (!strcmp(item, "dog")) {
		if (strstr(engitem, "dog")) {strcpy(uxitem, "animal"); return 1;}
	}					
	else if (!strcmp(item, "cow")) {
		if (strstr(engitem, "cow")) {strcpy(uxitem, "animal"); return 1;}
	}	
	else if (!strcmp(item, "horse")) {
		if (strstr(engitem, "horse")) {strcpy(uxitem, "animal"); return 1;}
	}

	return 0;
}

static gint _translate_to_engitem(gchar *uxitem, gchar *item, gchar *engitem)
{
	if (!strcmp(item, "person")) {
		if (strstr(uxitem, "person")) {strcpy(engitem, "person"); return 1;}
	}
	else if (!strcmp(item, "bike")) {
		//if (strstr(uxitem, "bike")) {strcpy(engitem, "bicycle,motorbike"); return 2;}
		if (strstr(uxitem, "bike")) {strcpy(engitem, "bike"); return 2;}
	}
	else if (!strcmp(item, "bus")) {
		if (strstr(uxitem, "bus")) {strcpy(engitem, "bus"); return 1;}
	}
	else if (!strcmp(item, "car")) {
		if (strstr(uxitem, "car")) {strcpy(engitem, "car"); return 1;}
	}
	else if (!strcmp(item, "truck")) {
		if (strstr(uxitem, "truck")) {strcpy(engitem, "truck"); return 1;}
	}	
	else if (!strcmp(item, "animal")) {
		if (strstr(uxitem, "animal")) {strcpy(engitem, "bird,cat,dog,cow,horse"); return 5;}
	}

	return 0;
}

static gint _translate_to_uxstring_db(gchar *engstring, gchar *item, gchar *uxstring)
{
	if (!strcmp(item, "person")) {
		if (strstr(engstring, "[person:1]")) {strcpy(uxstring, "[person:1]"); return 1;}
		else if (strstr(engstring, "[person:0]")) {strcpy(uxstring, "[person:0]"); return 1;}
	}
	else if (!strcmp(item, "bike")) {
		//if (strstr(engstring, "[bicycle:1],[motorbike:1]")) {strcpy(uxstring, "[bike:1]"); return 1;}
		//else if (strstr(engstring, "[bicycle:0],[motorbike:0]")) {strcpy(uxstring, "[bike:0]"); return 1;}
		if (strstr(engstring, "[bike:1]")) {strcpy(uxstring, "[bike:1]"); return 1;}
		else if (strstr(engstring, "[bike:0]")) {strcpy(uxstring, "[bike:0]"); return 1;}		
	}
	else if (!strcmp(item, "bus")) {
		if (strstr(engstring, "[bus:1]")) {strcpy(uxstring, "[bus:1]"); return 1;}
		else if (strstr(engstring, "[bus:0]")) {strcpy(uxstring, "[bus:0]"); return 1;}
	}
	else if (!strcmp(item, "car")) {
		if (strstr(engstring, "[car:1]")) {strcpy(uxstring, "[car:1]"); return 1;}
		else if (strstr(engstring, "[car:0]")) {strcpy(uxstring, "[car:0]"); return 1;}
	}	
	else if (!strcmp(item, "truck")) {
		if (strstr(engstring, "[truck:1]")) {strcpy(uxstring, "[truck:1]"); return 1;}
		else if (strstr(engstring, "[truck:0]")) {strcpy(uxstring, "[truck:0]"); return 1;}
	}		
	else if (!strcmp(item, "animal")) {
		if (strstr(engstring, "[bird:1],[cat:1],[dog:1],[cow:1],[horse:1]")) {strcpy(uxstring, "[animal:1]"); return 1;}
		else if (strstr(engstring, "[bird:0],[cat:0],[dog:0],[cow:0],[horse:0]")) {strcpy(uxstring, "[animal:0]"); return 1;}
	}

	return 0;
}

static gint _translate_db_to_engstring(gchar *uxstring, gchar *item, gchar *engstring)
{
	if (!strcmp(item, "person")) {
		if (strstr(uxstring, "[person:1]")) {strcpy(engstring, "[person:1]"); return 1;}
		else if (strstr(uxstring, "[person:0]")) {strcpy(engstring, "[person:0]"); return 1;}
	}
	else if (!strcmp(item, "bike")) {
		//if (strstr(uxstring, "[bike:1]")) {strcpy(engstring, "[bicycle:1],[motorbike:1]"); return 2;}
		//else if (strstr(uxstring, "[bike:0]")) {strcpy(engstring, "[bicycle:0],[motorbike:0]"); return 2;}
		if (strstr(uxstring, "[bike:1]")) {strcpy(engstring, "[bike:1]"); return 2;}
		else if (strstr(uxstring, "[bike:0]")) {strcpy(engstring, "[bike:0]"); return 2;}
	}
	else if (!strcmp(item, "bus")) {
		if (strstr(uxstring, "[bus:1]")) {strcpy(engstring, "[bus:1]"); return 1;}
		else if (strstr(uxstring, "[bus:0]")) {strcpy(engstring, "[bus:0]"); return 1;}
	}
	else if (!strcmp(item, "car")) {
		if (strstr(uxstring, "[car:1]")) {strcpy(engstring, "[car:1]"); return 1;}
		else if (strstr(uxstring, "[car:0]")) {strcpy(engstring, "[car:0]"); return 1;}
	}	
	else if (!strcmp(item, "truck")) {
		if (strstr(uxstring, "[truck:1]")) {strcpy(engstring, "[truck:1]"); return 1;}
		else if (strstr(uxstring, "[truck:0]")) {strcpy(engstring, "[truck:0]"); return 1;}
	}		
	else if (!strcmp(item, "animal")) {
		if (strstr(uxstring, "[animal:1]")) {strcpy(engstring, "[bird:1],[cat:1],[dog:1],[cow:1],[horse:1]"); return 5;}
		else if (strstr(uxstring, "[animal:0]")) {strcpy(engstring, "[bird:0],[cat:0],[dog:0],[cow:0],[horse:0]"); return 5;}
	}

	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

gint dvatext_translate_to_uxitem(gchar *engitem, gchar *uxitem, gint uxitem_len)
{
	gchar *object[] = {"person", "bike", /*"bicycle", "motorbike",*/ "bus", "car", "truck", "bird", "cat", "dog", "cow", "horse"};
	gchar strtmp[64];
	gint item_cnt = 0, tmp_cnt, dst_len;
	gint i;

	memset(uxitem, 0x00, uxitem_len);

	//g_message("%s, %d, engitem:%s", __FUNCTION__, __LINE__, engitem);

	for (i = 0; i < sizeof(object)/sizeof(object[0]); i++)
	{
		memset(strtmp, 0x00, sizeof(strtmp));
		tmp_cnt = _translate_to_uxitem(engitem, object[i], strtmp);
		if ((tmp_cnt > 0) && (!strstr(uxitem, strtmp)))
		{
			dst_len = strlen(uxitem);
			if (dst_len) g_snprintf(uxitem+dst_len, uxitem_len-dst_len, ",%s", strtmp);
			else g_snprintf(uxitem+dst_len, uxitem_len-dst_len, "%s", strtmp);
			
			item_cnt += tmp_cnt;
		}	
	}

	//g_message("%s, %d, uxitem:%s", __FUNCTION__, __LINE__, uxitem);
	return item_cnt;
}

gint dvatext_translate_to_engitem(gchar *uxitem, gchar *engitem, gint engitem_len)
{
	gchar *object[] = {"person", "bike", "bus", "car", "truck", "animal"};
	gchar strtmp[64];
	gint item_cnt = 0, tmp_cnt, dst_len;
	gint i;

	memset(engitem, 0x00, engitem_len);

	//g_message("%s, %d, uxitem:%s", __FUNCTION__, __LINE__, uxitem);

	for (i = 0; i < sizeof(object)/sizeof(object[0]); i++)
	{
		memset(strtmp, 0x00, sizeof(strtmp));
		tmp_cnt = _translate_to_engitem(uxitem, object[i], strtmp);
		
		if ((tmp_cnt > 0) && (!strstr(engitem, strtmp)))
		{
			dst_len = strlen(engitem);
			if (dst_len) g_snprintf(engitem+dst_len, engitem_len-dst_len, ",%s", strtmp);
			else g_snprintf(engitem+dst_len, engitem_len-dst_len, "%s", strtmp);
			
			item_cnt += tmp_cnt;
		}	
	}

	//g_message("%s, %d, engitem:%s", __FUNCTION__, __LINE__, engitem);
	return item_cnt;
}


gint dvatext_translate_to_uxstring_db(gchar *engstring, gchar *uxstring, gint uxstring_len)
{
	gchar *object[] = {"person", "bike", "bus", "car", "truck", "animal"};
	gchar strtmp[64];
	gint item_cnt = 0, tmp_cnt, dst_len;
	gint i;

	memset(uxstring, 0x00, uxstring_len);

	//g_message("%s, %d, engstring:%s", __FUNCTION__, __LINE__, engstring);

	for (i = 0; i < sizeof(object)/sizeof(object[0]); i++)
	{
		memset(strtmp, 0x00, sizeof(strtmp));
		tmp_cnt = _translate_to_uxstring_db(engstring, object[i], strtmp);
		if (tmp_cnt) 
		{
			dst_len = strlen(uxstring);
			if (dst_len) g_snprintf(uxstring+dst_len, uxstring_len-dst_len, ",%s", strtmp);
			else g_snprintf(uxstring+dst_len, uxstring_len-dst_len, "%s", strtmp);
			
			item_cnt += tmp_cnt;
		}	
	}

	//g_message("%s, %d, uxstring:%s", __FUNCTION__, __LINE__, uxstring);
	return item_cnt;
}

gint dvatext_translate_to_engstring_db(gchar *uxsting, gchar *engstring, gint engstring_len)
{
	gchar *object[] = {"person", "bike", "bus", "car", "truck", "animal"};
	gchar strtmp[64];
	gint item_cnt = 0, tmp_cnt, dst_len;
	gint i;

	memset(engstring, 0x00, engstring_len);

	//g_message("%s, %d, engstring:%s", __FUNCTION__, __LINE__, engstring);

	for (i = 0; i < sizeof(object)/sizeof(object[0]); i++)
	{
		memset(strtmp, 0x00, sizeof(strtmp));
		tmp_cnt = _translate_db_to_engstring(uxsting, object[i], strtmp);
		if (tmp_cnt) 
		{
			dst_len = strlen(engstring);
			if (dst_len) g_snprintf(engstring+dst_len, engstring_len-dst_len, ",%s", strtmp);
			else g_snprintf(engstring+dst_len, engstring_len-dst_len, "%s", strtmp);
			
			item_cnt += tmp_cnt;
		}	
	}

	//g_message("%s, %d, engstring:%s", __FUNCTION__, __LINE__, engstring);
	return item_cnt;
}

