#ifndef _VW_LIVE_SHORTCUT_SUBMENU_FISHEYE_H
#define _VW_LIVE_SHORTCUT_SUBMENU_FISHEYE_H

#include "vw_live_shortcut_menu.h"

int VW_move_fisheye_submenu(int x, int y);
int VW_destroy_fisheye_submenu();
void VW_show_fisheye_submenu();

void VW_create_fisheye_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item);

#endif 


