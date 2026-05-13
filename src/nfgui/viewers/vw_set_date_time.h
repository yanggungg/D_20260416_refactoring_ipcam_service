#ifndef __VW_SET_DATE_TIME_H__
#define __VW_SET_DATE_TIME_H__

#include "objects/nfobject.h"

typedef enum _SDT_LIMIT_E
{
	LIMIT_NOT_USE = 0,
	LIMIT_LOWER,
	LIMIT_UPPER
} SDT_LIMIT_E;

typedef enum _SDT_TYPE_E
{
	SDT_TYPE_YEAR = 0,
	SDT_TYPE_MON,
	SDT_TYPE_DAY,
	SDT_TYPE_HOUR,
	SDT_TYPE_MIN,	
	SDT_TYPE_SEC
} SDT_TYPE_E;

typedef enum _SDT_MODE_E
{
    SDT_LOCAL = 0,
    SDT_GMT
} SDT_MODE_E;

time_t VW_Set_DateTime_Open(NFWINDOW *parent, gchar *title, gint sdt_x, gint sdt_y, time_t c_time, SDT_TYPE_E type, time_t start_time, time_t end_time);
time_t VW_Set_DateTime_Open_Ver2(NFWINDOW *parent, gchar *title, gint sdt_x, gint sdt_y, time_t c_time, SDT_TYPE_E type, time_t start_time, time_t end_time);


#endif

