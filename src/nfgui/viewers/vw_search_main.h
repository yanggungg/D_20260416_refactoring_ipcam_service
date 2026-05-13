#ifndef _VW_SEARCH_MAIN_H_
#define _VW_SEARCH_MAIN_H_

#include "nf_common.h"             
#include "nf_api_play.h"
#include "nf_api_live.h"
#include "nf_api_eventlog.h"

#include "vw_playback_main.h"

#include "objects/nfobject.h"
#include "vsm.h"
#include "scm.h"


void VW_Search_Open(NFWINDOW *parent, gint default_page, LIVESTART_T *lst);
time_t VW_Search_Get_Record_FirstTime();
time_t VW_Search_Get_Record_LastTime();
void VW_Search_Destroy();
void VW_Search_key_remake();
void VW_Search_start_playback();
void VW_Search_Show(gint page, LIVESTART_T *lst);
void VW_Search_config_smart(gboolean search_started);

extern NF_PLAY_PARAM G_PlayParam;

#endif



