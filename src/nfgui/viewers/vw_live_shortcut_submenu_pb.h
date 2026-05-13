#ifndef _VW_LIVE_SHORTCUT_SUBMENU_PB_H
#define _VW_LIVE_SHORTCUT_SUBMENU_PB_H

#include "vw_live_shortcut_menu.h"

int VW_move_pb_submenu(int x, int y);
void VW_create_playback_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item);
void VW_show_pb_submenu();
void VW_hide_pb_submenu();
int VW_destroy_playback_submenu();

#endif
