#ifndef __VW_ARCHIVING_H__
#define __VW_ARCHIVING_H__

#include <gtk/gtk.h>
#include "../viewers/objects/cw_calendar.h"
#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nftab.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nftable.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nftimespin.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "../viewers/objects/nflistbox.h"
#include "../viewers/objects/nfspinbutton.h"
#include "../viewers/objects/nfbutton.h"

#include "tools/nf_ui_tool.h"
#include "vw_playback_main.h"
#include "vsm.h"



// [ Public Member Function ]

void VW_Archiving_Open(NFWINDOW *parent, LIVESTART_T *lst, int from_pb);
void VW_Archiving_Close();
void vw_archiving_start_playback(PB_OPEN_BY open);
void vw_archiving_show(LIVESTART_T *lst);
#endif

