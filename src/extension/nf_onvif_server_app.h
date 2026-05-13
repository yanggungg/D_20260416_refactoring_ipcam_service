#ifndef __NF_ONVIF_SERVER_APP_H__

#include "feature_def.h"
#include "onvif_common.h"
#include "nf_api_eventlog.h"
#include "nf_util_time.h"

#define __NF_ONVIF_SERVER_APP_H__

////////////////////////////////////////////////////////////////////////////////
///////////////////ENUM/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct _search_filter 
{
	int 			type; /* Recordings, Events, Metadata, PTZs  */
	unsigned int 	max_matches; 
	unsigned int	timeout;
	char 		search_token[COMMON_SIZE];
	NF_LOG_PARAM param;
} search_filter;



typedef enum _ONVIF_FIELD_NAME_E {
	ONVIF_FIELD_NR,
} ONVIF_FIELD_NAME_E;

//void VW_SetupSystem_set_changeflag(guint flag);
gboolean nf_zoneinfo_get_activex_posix_onvif( const gchar *tzname, NF_TZINFO_ACTIVEX *tzout);
gboolean nf_action_relay_test(gboolean is_test_on, gint relay_num, gboolean type);
int set_search_rec_state_shm(unsigned int token, char search_state);
int init_search_rec_shm(unsigned int token);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////Application/////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif//__NF_ONVIF_SERVER_APP_H__

