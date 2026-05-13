#ifndef	__VW_PLAYBACK_ARCH__
#define	__VW_PLAYBACK_ARCH__

#include "nf_api_play.h"

enum {
	PA_RET_NONE = 0,
	PA_RET_RESERVE ,
	PA_RET_CONTINUE,
	PA_RET_START,
	PA_RET_STOP,
	PA_RET_CLOSE 
};

typedef enum _BMKWND_E {
	START,
	STOP
} BMKWND_E;

guint VW_PlaybackArch_Open(NFWINDOW *parent, BMKWND_E mode);
void VW_PlaybackArch_Close();

#endif
