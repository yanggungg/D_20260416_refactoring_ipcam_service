#ifndef _VW_PLAYBACK_SHORTCUT_SUBMENU_CC_H
#define _VW_PLAYBACK_SHORTCUT_SUBMENU_CC_H

#include "vw_playback_shortcut_menu.h"

int VW_move_cc_submenu_pb(int x, int y);
int VW_destroy_camchange_submenu_pb();
void VW_show_cc_submenu_pb();
void cam_change_enable_obj_pb();

void VW_create_camchange_submenu_pb(NFWINDOW *parent, NFOBJECT *parent_item);


#endif
