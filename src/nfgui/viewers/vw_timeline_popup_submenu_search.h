#ifndef _VW_TIMELINE_POPUP_SUBMENU_SEARCH_H_
#define _VW_TIMELINE_POPUP_SUBMENU_SEARCH_H_


gboolean VW_Create_Search_SubMenu(NFWINDOW *parent);
gboolean VW_Show_Search_SubMenu();
gboolean VW_Hide_Search_SubMenu();
gboolean VW_IsShown_Search_SubMenu();

int VW_Move_Search_SubMenu(int x, int y);
int VW_Get_Search_SubMenu_Geometry(int *x, int *y, int *w, int *h);

int VW_Destory_Search_SubMenu();
#endif

