#ifndef __VW_SNAPSHOT_H__
#define __VW_SNAPSHOT_H__


typedef struct _SNAPSHOT_INFO_T {
	gint 	ch;
	time_t 	time;
	gint 	size;
	int 	width;
	int 	height;
	void 	*buffer;
} SNAPSHOT_INFO_T;

typedef enum _SNAPSHOT_MODE_E {
	SS_MODE_BURN 			= 0,
	SS_MODE_BURN_RESERVE 	= 1,
} SNAPSHOT_MODE_E;

gboolean VW_Snapshot_Open(NFWINDOW *parent, SNAPSHOT_INFO_T *info, SNAPSHOT_MODE_E mode);

#endif

