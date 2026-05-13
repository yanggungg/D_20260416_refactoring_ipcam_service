
#ifndef _VW_RECORD_AUTO_INTENSIVE_POPUP_H_
#define _VW_RECORD_AUTO_INTENSIVE_POPUP_H_

#include "objects/nfobject.h"

typedef enum {
	OPEN_AUTO_ITS_MOT 						= 0,
	OPEN_AUTO_ITS_ALARM 					= 1,
	OPEN_AUTO_ITS_MOT_ALARM
} OPEN_AUTO_ITS;

gboolean VW_RecAutoItsPopup_Open(NFWINDOW *parent, gchar* (*info)[], OPEN_AUTO_ITS mode);
void VW_RecAutoItsPopup_Close();

#endif

