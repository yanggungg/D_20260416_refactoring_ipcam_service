#ifndef _VW_ALARM_NAME_POPUP_H_
#define _VW_ALARM_NAME_POPUP_H_


void VW_Create_OSD_Popup(NFWINDOW *parent);
int VW_Destroy_OSD_Popup();

gboolean VW_Show_OSD_Popup(gchar *str);
gboolean VW_Hide_OSD_Popup();
gint VW_OSD_Popup_is_untilkey();

#endif
