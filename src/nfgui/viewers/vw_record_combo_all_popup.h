
#ifndef _VW_RECORD_COMBO_ALL_POPUP_H_
#define _VW_RECORD_COMBO_ALL_POPUP_H_

#include "objects/nfobject.h"

typedef enum _ALL_TYPE_E
{
	TYPE_SIZE 		= 0,
	TYPE_FPS
} ALL_TYPE_E;

gboolean VW_RecComboAll_Popup_Open(NFWINDOW *parent, gint x, gint y, gchar **info, ALL_TYPE_E type);
void VW_RecComboAll_Popup_Close();

#endif

