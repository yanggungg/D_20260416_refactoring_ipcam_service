
#ifndef _VW_RECORD_PARAM_SUB_H_
#define _VW_RECORD_PARAM_SUB_H_


typedef struct _ParamSubData {
	guint8 ch;
	guint8 param;
	gchar* value;
}ParamSubData;


typedef enum
{
  REC_PARAM_NORMAL_OPEN = 0,
  REC_PARAM_ALARM_OPEN 	= 1,
  REC_PANIC_OPEN 		= 2,
  REC_NETSTREAM_OPEN 	= 3,
  REC_PARAM_OPEN_MAX
}rec_param_open_type;


gboolean VW_RecParam_Submenu_Open(NFWINDOW *parent, gchar* (*info)[], rec_param_open_type open_type);
void VW_RecParam_Submenu_Close();
#endif

