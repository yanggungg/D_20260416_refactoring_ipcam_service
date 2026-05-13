
#ifndef _VW_LIVE_AUDIO_INPUT_H_
#define _VW_LIVE_AUDIO_INPUT_H_


gboolean VW_Create_AudioInput(NFWINDOW *parent, gint x, gint y);
void VW_AudioInput_Show();
void VW_AudioInput_Hide();
gboolean VW_AudioInput_IsShown();
void VW_AudioInput_Change_CH(guint ch);


#endif
