#ifndef _VW_LIVE_SHORTCUT_SUBMENU_CC_H
#define _VW_LIVE_SHORTCUT_SUBMENU_CC_H

#include "vw_live_shortcut_menu.h"

int VW_move_cc_submenu(int x, int y);
int VW_destroy_camchange_submenu();
void VW_show_cc_submenu();
void VW_hide_cc_submenu();


void VW_create_camchange_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item);

#endif 

