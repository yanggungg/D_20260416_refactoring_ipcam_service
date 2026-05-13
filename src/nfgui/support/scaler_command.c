#if defined(_ANF_0824CL)

#include "string.h"

#include "nf_util_device.h"

#include "scaler_command.h"
#include "event_loop.h"

#define RECV_CMD_BUFF_SIZE				1024
#define RECV_CMD_VOL_LENGTH				12

static GENESIS_INFO g_info;
static guchar g_tx_cmd[G_TXCMD_COUNT][G_TXCMD_SIZE] = {{0x04, 0xff, 0x03, 0xfa},		// pip on off
														{0x04, 0xff, 0x08, 0xf5},		// pip vga
														{0x04, 0xff, 0x09, 0xf4},		// pip component
														{0x04, 0xff, 0x0f, 0xee},		// mode dvr
														{0x04, 0xff, 0x10, 0xed},		// mode dvi
														{0x04, 0xff, 0x11, 0xec},		// mode component
														{0x04, 0xff, 0x0e, 0xef},		// mode dlg
														{0x04, 0xff, 0x14, 0xe9},		// key up
														{0x04, 0xff, 0x15, 0xe8},		// key down
														{0x04, 0xff, 0x16, 0xe7},		// key left
														{0x04, 0xff, 0x17, 0xe6},		// key right
														{0x04, 0xff, 0x0c, 0xf1},		// key enter
														{0x04, 0xff, 0x13, 0xea},		// key exit	
														{0x04, 0xff, 0x0b, 0xf2},		// key setup	
														{0x04, 0xff, 0x1a, 0xe3},		// aspectRatio 16:9	
														{0x04, 0xff, 0x1b, 0xe2},		// aspectRatio 4:3	
														{0x04, 0xff, 0x18, 0xe5},		// volume up +1
														{0x04, 0xff, 0x19, 0xe4}};		// volume down -1	

static guchar g_rx_cmd[RECV_CMD_BUFF_SIZE]; 
static guint recv_timer = 0;

// for sample
static gint mode_idx = 0;		// min:0 ~ max:2


static gint parse_recv_cmd(gint rx_size, gint index)
{
	gint i, start_idx = -1;
	gchar chkSum = 0;

	for(i=index; i<=rx_size; i++) {
		g_printf("search 0x0F == RX[%02x]...............\n", g_rx_cmd[i]);

		if(g_rx_cmd[i] == 0x0f) {

			g_printf("0x0F OK!!!!\n");

			if((i+G_RXCMD_SIZE) <=  rx_size) {
				start_idx = i;
				g_printf("=======================\nstart command: %d\n=============================\n", i);

				for(i; i<(start_idx+G_RXCMD_VAL_SIZE); i++)
				{ 
					g_printf("chkSum <- g_rx_cmd[%d]: %02x\n", i, g_rx_cmd[i]);
					chkSum += g_rx_cmd[i];	
				}

				g_printf("chkSum = %02x\n", chkSum);

				if((~chkSum + 1) == g_rx_cmd[i]) {
					g_message("ChkSum succese!!\n");
					return start_idx;
				}else
				{
					g_message("ChkSum fail!!\n");
					continue;
				}
			}
		}else {
			continue;
		}
	}

	return -1;
}


static gboolean set_genesis_info(gint start_index)
{
	guint8 ucmd;
	
	if(g_rx_cmd[start_index + 1] != 0x0d)
		return FALSE;
	
	g_printf("Genesis cmd: 0x0D OK!!!\n");

	if((g_rx_cmd[start_index + 2] != 0x53)
		&& (g_rx_cmd[start_index + 3] != 0x4d) 
		&& (g_rx_cmd[start_index + 4] != 0x43) 
		&& (g_rx_cmd[start_index + 5] != 0x4d) 
		&& (g_rx_cmd[start_index + 6] != 0x44) 
		&& (g_rx_cmd[start_index + 7] != 0x5f)
	)
		return FALSE;
	
	g_printf("SMCMD OK!!!\n");

	ucmd = (g_rx_cmd[start_index + 8] - '0') * 100 + (g_rx_cmd[start_index + 9] - '0') * 10 + (g_rx_cmd[start_index + 10] - '0');


	g_printf("Command: %d\n", ucmd);

	switch(ucmd)
	{
		case G_RXCMD_MODE_DVR:
			{
				MonitorData mondata;
				guint volume = 0;

				g_printf("DVR mode\n");
				g_info.mode = DVR_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;

				memset(&mondata, 0x00, sizeof(MonitorData));
				DAL_get_monitor_data(&mondata);

				g_usleep(10*1000);

				if(mondata.aspectRatio) genesis_transfer_command(G_TXCMD_RATIO_4_3);
				else 					genesis_transfer_command(G_TXCMD_RATIO_16_9);

				if(!DAL_get_audio_volume(&volume))
					g_warning("%s [%d] : error get audio volume", __FUNCTION__, __LINE__);

				nf_dev_njw1156a_set_live_audio_volume(NJW1156A_MODE_DVR, NJW1156A_MAX_VOLUME, (guchar)volume);
			}
			break;
		case G_RXCMD_MODE_DVI:
			{
				g_printf("DVI mode\n");
				g_info.mode = DVI_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;

				nf_dev_njw1156a_set_live_audio_volume(NJW1156A_MODE_COMPONENT, NJW1156A_MAX_VOLUME, g_rx_cmd[start_index + RECV_CMD_VOL_LENGTH]);
			}
			break;
		case G_RXCMD_MODE_COMPONENT:
			{
				g_printf("Component mode\n");
				g_info.mode = COMPONENT_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;

				nf_dev_njw1156a_set_live_audio_volume(NJW1156A_MODE_COMPONENT, NJW1156A_MAX_VOLUME, g_rx_cmd[start_index + RECV_CMD_VOL_LENGTH]);
			}
			break;
		case G_RXCMD_MODE_DLG_ON:
			{
				g_printf("Dialog ON\n");
				if(g_info.mode == DVR_MODE)
					g_info.mode = DVR_MODE_DLG_MODE;
				else if(g_info.mode == DVI_MODE)
					g_info.mode = DVI_MODE_DLG_MODE;
				else if(g_info.mode == COMPONENT_MODE)
					g_info.mode = COMPONENT_MODE_DLG_MODE;

				g_info.modeMenuOnOff = TRUE;
				//g_info.pipOnOff = PIP_OFF;
			}
			break;
		case G_RXCMD_MODE_DLG_OFF:
			{
				g_printf("Dialog OFF\n");
				if(g_info.mode == DVR_MODE_DLG_MODE)
					g_info.mode = DVR_MODE;
				else if(g_info.mode == DVI_MODE_DLG_MODE)
					g_info.mode = DVI_MODE;
				else if(g_info.mode == COMPONENT_MODE_DLG_MODE)
					g_info.mode = COMPONENT_MODE;

				g_info.modeMenuOnOff = FALSE;
				//g_info.pipOnOff = PIP_OFF;
			}
			break;
		
		case G_RXCMD_VOLUME:
			{
				nf_dev_njw1156a_set_live_audio_volume(NJW1156A_MODE_COMPONENT, NJW1156A_MAX_VOLUME, g_rx_cmd[start_index + RECV_CMD_VOL_LENGTH]);
			}
			break;


		default:
			g_printf("Command error!!!\n");
			break;
	}

	return TRUE;
}

static gboolean receive_command(gpointer data)
{
	gint rx_size = 0;
	gint index = 0;
	gint i;
	
	memset(g_rx_cmd, 0x00, sizeof(g_rx_cmd));

#if defined(USE_DEV_GENESIS_UART2)
	if(!nf_dev_uart2_request_rx(g_rx_cmd, &rx_size, 100)) {
		recv_timer = 0;
		g_message("######################### nf_dev_uart2_request_rx returns FALSE ##############################");
		return FALSE;
	}
#endif  /*USE_DEV_GENESIS_UART2*/

	g_message(":::::::rx_size = %d\n", rx_size);

	for(i=0; i<rx_size; i++)
		g_printf(" %02x ", g_rx_cmd[i]);

	g_printf("\n");

	while(index != -1)
	{

		g_printf("index = %d\n", index);	

		if((index = parse_recv_cmd(rx_size, index)) >= 0) {
			set_genesis_info(index);

			recv_timer = 0;

			index += G_RXCMD_SIZE;

		}else
			return FALSE;
	}

	return TRUE;
}


void genesis_info_init()
{
	memset(&g_info, 0x00, sizeof(GENESIS_INFO));
	memset(g_rx_cmd, 0x00, sizeof(g_rx_cmd));
}


void genesis_set_pip_status(PipStatus status)
{
	g_info.pipOnOff = status;
}


PipStatus genesis_get_pip_status()
{
	return g_info.pipOnOff;
}

gboolean genesis_transfer_command(TransferCmd cmd)
{
#if defined(USE_DEV_GENESIS_UART2)
	return nf_dev_uart2_request_tx(g_tx_cmd[cmd], G_TXCMD_SIZE);
	
#else   /**/
    return TRUE;
#endif  /*USE_DEV_GENESIS_UART2*/
}

gboolean genesis_transfer_mode_change_command(guchar code)
{
	TransferCmd cmd;
	gboolean start_timer = FALSE;
	gboolean ret = FALSE;

//	g_message(":::%02x:::::::::::::::::::::: %d: %s ", code, __LINE__, __FUNCTION__);

	switch(code) 
	{
		case RMC_MODE:
			{
				cmd = G_TXCMD_MODE_DLG;
				start_timer = TRUE;
#if 0
				if(g_info.mode == DVR_MODE)
					g_info.mode = DVR_MODE_DLG_MODE;
				else if(g_info.mode == DVI_MODE)
					g_info.mode = DVI_MODE_DLG_MODE;
				else if(g_info.mode == COMPONENT_MODE)
					g_info.mode = COMPONENT_MODE_DLG_MODE;

				g_info.modeMenuOnOff = TRUE;
#endif
			}
			break;
		case RMC_DVR:
			{
				cmd = G_TXCMD_MODE_DVR;
				start_timer = TRUE;
#if 0
				g_info.mode = DVR_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;
#endif
			}
			break;
		case RMC_DVI:
			{
				cmd = G_TXCMD_MODE_DVI;
				start_timer = TRUE;
#if 0
				g_info.mode = DVI_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;
#endif
			}
			break;
		case RMC_COMPONENT:
			{
				cmd = G_TXCMD_MODE_COMPONENT;
				start_timer = TRUE;
#if 0
				g_info.mode = COMPONENT_MODE;
				g_info.modeMenuOnOff = FALSE;
				g_info.pipOnOff = PIP_OFF;
#endif
			}
			break;
		case KEYPAD_UP:
			cmd = G_TXCMD_KEY_UP;
#if 0
			if(--mode_idx < 0) 
				mode_idx = 2;
				g_printf("up::::::::::::::: mode idx : %d \n", mode_idx);
#endif
			break;
		case KEYPAD_DOWN:
			cmd = G_TXCMD_KEY_DOWN;
#if 0
			if(++mode_idx > 2) 
				mode_idx = 0;
				g_printf("down::::::::::::::: mode idx : %d \n", mode_idx);
#endif

			break;
		case KEYPAD_LEFT:
			cmd = G_TXCMD_KEY_LEFT;
			break;
		case KEYPAD_RIGHT:
			cmd = G_TXCMD_KEY_RIGHT;
			break;
		case KEYPAD_ENTER:
			{
				cmd = G_TXCMD_KEY_ENTER;
#if 0
				if(g_info.mode >= DVR_MODE_DLG_MODE && g_info.mode <= COMPONENT_MODE_DLG_MODE)
					start_timer = TRUE;

				if(mode_idx == 0) {
					g_info.mode = DVR_MODE;
					g_info.modeMenuOnOff = FALSE;
					g_info.pipOnOff = PIP_OFF;
				}else if(mode_idx == 1) {
					g_info.mode = DVI_MODE;
					g_info.modeMenuOnOff = FALSE;
					g_info.pipOnOff = PIP_OFF;
				}else if(mode_idx == 2) {
					g_info.mode = COMPONENT_MODE;
					g_info.modeMenuOnOff = FALSE;
					g_info.pipOnOff = PIP_OFF;
				}else 
					return;

				mode_idx = 0;
				g_printf("::::::::::::::: mode idx : %d \n", mode_idx);
#endif
			}
			break;
		case KEYPAD_EXIT:
			{
				cmd = G_TXCMD_KEY_EXIT;
#if 0
				if(g_info.mode >= DVR_MODE_DLG_MODE && g_info.mode <= COMPONENT_MODE_DLG_MODE)
					start_timer = TRUE;

				if(g_info.modeMenuOnOff) {
					if(g_info.mode == DVR_MODE_DLG_MODE)
						g_info.mode = DVR_MODE;
					else if(g_info.mode == DVI_MODE_DLG_MODE)
						g_info.mode = DVI_MODE;
					else if(g_info.mode == COMPONENT_MODE_DLG_MODE)
						g_info.mode = COMPONENT_MODE;

					g_info.modeMenuOnOff = FALSE;
				}
				g_printf("::::::::::::::: mode idx : %d \n", mode_idx);

				mode_idx = 0;
#endif
			}
			break;
		case KEYPAD_SETUP:
			cmd = G_TXCMD_KEY_SETUP;
			break;
		case RMC_PIP:
			cmd = G_TXCMD_PIP_ONOFF;
			break;
		case RMC_VOLUME_UP:
			cmd = G_TXCMD_VOL_UP;
			break;
		case RMC_VOLUME_DOWN:
			cmd = G_TXCMD_VOL_DOWN;
			break;
		default:
			return FALSE;
	}

#if defined(USE_DEV_GENESIS_UART2)
	ret = nf_dev_uart2_request_tx(g_tx_cmd[cmd], G_TXCMD_SIZE);
#endif  /*USE_DEV_GENESIS_UART2*/

#if 0
	if(start_timer)
		recv_timer = g_timeout_add(100, receive_command, NULL);	
	g_message("::::::::::::::;%d:::::%s:::::\n", __LINE__, __FUNCTION__);
#endif
	receive_command(NULL);

	return ret;
}

void genesis_set_current_mode_type(ModeType mode)
{
	g_info.mode = mode;
}

ModeType genesis_get_current_mode_type()
{
	return g_info.mode;
}



#endif
