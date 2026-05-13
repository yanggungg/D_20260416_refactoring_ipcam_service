
#ifndef _VW_SYSTEM_SELF_INTERNAL_H_
#define _VW_SYSTEM_SELF_INTERNAL_H_

#include "support/nf_ui_image.h"
#include "support/nf_ui_color.h"

#define IN_DISP_DISK_COUNT	(5)
#define EX_DISP_DISK_COUNT	(5)

typedef enum _SELF_CHECK_MASK_E {
	MSK_POWER		= 0,
	MSK_NET			= 1,
	MSK_HDD			= 2,
	MSK_PORT		= 3,
	SELF_CHECK_MSK_MAX
} SELF_CHECK_MASK_E;

typedef struct _SELF_RESULT_T {
	guint power[GUI_CHANNEL_CNT];
	guint net[GUI_CHANNEL_CNT];
	guint hdd_in[IN_DISP_DISK_COUNT];
	guint hdd_ex[EX_DISP_DISK_COUNT];
	guint rtsp_port;
	guint web_port;
	BITMASK chkMask;
} SELF_RESULT_T;

gboolean VW_System_Self_Start_Open(NFWINDOW *parent);
gboolean VW_System_Self_Check_Open(NFWINDOW *parent, SELF_CHECK_MASK_E self_msk);
gboolean VW_System_Self_Result_Open(NFWINDOW *parent, SELF_RESULT_T *result);
void VW_System_Self_Start_Close();
void VW_System_Self_Result_Close();

#endif

