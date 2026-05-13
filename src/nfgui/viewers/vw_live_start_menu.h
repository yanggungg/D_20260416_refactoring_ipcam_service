#ifndef _VW_LIVE_START_MENU_H
#define _VW_LIVE_START_MENU_H



void LiveStart_Popup_Menu(NFWINDOW *parent);
gboolean LiveStart_Popup_Refresh(void);

void LiveStart_Popup_Menu_Enable();
void LiveStart_Popup_Menu_Disable();

gboolean LiveStart_Popup_Hide(void);
int LiveStart_Popup_Menu_Destroy();

int LiveStart_Popup_Pos(int *x, int *y);
int LiveStart_Popup_Size(int *w, int *h);


#endif
