#ifndef _VW_EVENT_EMAIL_EDIT_H_
#define _VW_EVENT_EMAIL_EDIT_H_

#define EMAIL_COUNT					(8)
#define EMAIL_STRING_LENGTH			(65)

void VW_EvtNoti_Email_Edit(NFWINDOW *parent, gchar (*address)[EMAIL_STRING_LENGTH]);

#endif

