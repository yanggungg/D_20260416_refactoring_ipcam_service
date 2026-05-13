#ifndef _VW_SYS_DISP_SEQ_CONF_SUBMENU_H
#define _VW_SYS_DISP_SEQ_CONF_SUBMENU_H

// return : 0 - modify, 1 - delete, 2 - cancel

gint VW_Create_Seq_SubMenu(NFWINDOW *parent, gint x, gint y);
void VW_Destroy_Seq_SubMenu();

#endif
