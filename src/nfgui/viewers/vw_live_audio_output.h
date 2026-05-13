
#ifndef _VW_LIVE_AUDIO_OUTPUT_H_
#define _VW_LIVE_AUDIO_OUTPUT_H_


gboolean VW_Create_AudioOutput(NFWINDOW *parent, gint x, gint y);
void VW_AudioOutput_Show(gint x, gint y);
void VW_AudioOutput_Hide();
gboolean VW_AudioOutput_IsShown();

#endif
