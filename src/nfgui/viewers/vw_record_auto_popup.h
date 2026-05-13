
#ifndef _VW_RECORD_AUTO_POPUP_H_
#define _VW_RECORD_AUTO_POPUP_H_

#include "objects/nfobject.h"

typedef enum {
	OPEN_AUTO_MOT 						= 0,
	OPEN_AUTO_ALARM 					= 1,
	OPEN_AUTO_MOT_ALARM
} OPEN_AUTO;


gboolean VW_RecAutoPopup_Open(NFWINDOW *parent, gchar* (*info)[], OPEN_AUTO mode);
void VW_RecAutoPopup_Close();
#endif

