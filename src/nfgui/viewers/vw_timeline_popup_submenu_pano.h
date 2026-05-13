#ifndef _VW_TIMELINE_POPUP_SUBMENU_PANO_H_
#define _VW_TIMELINE_POPUP_SUBMENU_PANO_H_


gboolean VW_Create_Pano_SubMenu(NFWINDOW *parent);
gboolean VW_Hide_Pano_SubMenu();
gboolean VW_Show_Pano_SubMenu();
gboolean VW_IsShown_Pano_SubMenu();

int VW_Move_Pano_SubMenu(int x, int y);
int VW_Get_Pano_SubMenu_Geometry(int *x, int *y, int *w, int *h);

#endif



