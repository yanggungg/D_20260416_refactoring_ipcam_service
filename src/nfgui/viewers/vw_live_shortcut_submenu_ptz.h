#ifndef _VW_LIVE_SHORTCUT_SUBMENU_PTZ_H
#define _VW_LIVE_SHORTCUT_SUBMENU_PTZ_H

#include "vw_live_shortcut_menu.h"

//int VW_move_pb_submenu(int x, int y);
//int VW_destroy_camchange_submenu();
//void VW_show_pb_submenu();

int VW_move_ptz_submenu(int x, int y);
int VW_destroy_ptz_submenu();
void VW_show_ptz_submenu();
void VW_hide_ptz_submenu();

//void VW_create_camchange_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item);
void VW_create_ptz_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item);

#endif 


