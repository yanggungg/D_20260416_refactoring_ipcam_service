#ifndef _VW_LIVE_STATUS_BAR_H
#define _VW_LIVE_STATUS_BAR_H





NFOBJECT* VW_Live_StatusBar_Open(NFWINDOW *parent);
int VW_Live_StatusBar_Close();
void VW_Live_StatusBar_Show();
void VW_Live_StatusBar_Hide();
gboolean VW_Live_StatusBar_IsShown();
gboolean VW_Live_StatusBar_IsInArea(guint x, guint y);
void VW_Live_StatusBar_Set_User(gchar *user);

void VW_Live_StatusBar_Menu_Enable();
void VW_Live_StatusBar_Menu_Disable();

guint VW_Live_StatusBar_On_Time();

#endif
