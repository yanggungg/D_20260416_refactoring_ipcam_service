#ifndef _VW_STATISTIC_EXPORT_H_
#define _VW_STATISTIC_EXPORT_H_

#include "nf_api_archive.h"
#include "vw_internal.h"

typedef struct _STATISTIC_EXPORT_T {
	int ch;							 // -> ok
	time_t start_time;				 // -> time_t
	time_t end_time;				 // -> time_t
	int period;						 // -> ok
	int rule_list[IVCA_MAX_ZONES];	 // -> ok
	int total_events;				 // -> ok
	guint exist_zone;                // add
	GdkPixbuf *pbuf;	             // -> ok  
}STATISTIC_EXPORT_T;

void VW_StatisticExport_Open(NFWINDOW *parent, STATISTIC_EXPORT_T *export_info);


#endif

