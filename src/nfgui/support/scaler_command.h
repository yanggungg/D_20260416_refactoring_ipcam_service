
#ifndef _SCALER_COMMAND_H_
#define _SCALER_COMMAND_H_

#include <glib.h> 

#define G_TXCMD_SIZE						4
#define G_RXCMD_SIZE						15
#define G_RXCMD_VAL_SIZE					14


typedef enum {
	DVR_MODE = 0,
	DVI_MODE,
	COMPONENT_MODE,
	DVR_MODE_DLG_MODE,
	DVI_MODE_DLG_MODE,
	COMPONENT_MODE_DLG_MODE
}ModeType;

typedef enum {
	G_TXCMD_PIP_ONOFF = 0,
	G_TXCMD_PIP_VGA,
	G_TXCMD_PIP_COMPONENT,
	G_TXCMD_MODE_DVR,
	G_TXCMD_MODE_DVI,
	G_TXCMD_MODE_COMPONENT,
	G_TXCMD_MODE_DLG,
	G_TXCMD_KEY_UP,
	G_TXCMD_KEY_DOWN,
	G_TXCMD_KEY_LEFT,
	G_TXCMD_KEY_RIGHT,
	G_TXCMD_KEY_ENTER,
	G_TXCMD_KEY_EXIT,
	G_TXCMD_KEY_SETUP,
	G_TXCMD_RATIO_16_9,
	G_TXCMD_RATIO_4_3,
	G_TXCMD_VOL_UP,
	G_TXCMD_VOL_DOWN,
	G_TXCMD_COUNT
}TransferCmd;

typedef enum {
	G_RXCMD_MODE_DVR = 1,
	G_RXCMD_MODE_DVI,
	G_RXCMD_MODE_COMPONENT,
	G_RXCMD_MODE_DLG_ON = 6,
	G_RXCMD_MODE_DLG_OFF,
	G_RXCMD_VOLUME = 10,
	G_RXCMD_COUNT
}ReceiveCmd;

typedef enum {
	PIP_OFF = 0,
	PIP_ON
}PipStatus;

typedef struct _GENESIS_INFO {
	ModeType mode;
	gboolean modeMenuOnOff;
	gboolean pipOnOff;
}GENESIS_INFO;


void genesis_info_init();
void genesis_set_pip_status(PipStatus status);
PipStatus genesis_get_pip_status();
gboolean genesis_transfer_command(TransferCmd cmd);
gboolean genesis_transfer_mode_change_command(guchar code);
void genesis_set_current_mode_type(ModeType mode);
ModeType genesis_get_current_mode_type();

#endif

