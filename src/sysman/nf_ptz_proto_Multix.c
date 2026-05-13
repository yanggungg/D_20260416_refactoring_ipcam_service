#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define MULTIX_LEN		8



static int _Multix_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _Multix_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}

int _Multix_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}

int _Multix_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}

int _Multix_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}

int _Multix_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}
int _Multix_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}
int _Multix_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}
int _Multix_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}


int _Multix_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);

}

int _Multix_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);

}

int _Multix_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);

}

int _Multix_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);

}

int _Multix_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);

}

int _Multix_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}


int _Multix_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command(cmd_buff, cmd, sysdb_ch);
}


int _Multix_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command( cmd_buff, cmd , sysdb_ch);
}

int _Multix_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Multix_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Multix_command( cmd_buff, cmd , sysdb_ch);
}





NF_PTZ_PROTOCOL	_nf_ptz_proto_Multix = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "Multix", 
	.func_pan_left           = _Multix_pan_left,
	.func_pan_right          = _Multix_pan_right,
	.func_tilt_up            = _Multix_tilt_up,
	.func_tilt_down          = _Multix_tilt_down,
	.func_pt_leftup          = _Multix_pt_leftup,
	.func_pt_leftdown        = _Multix_pt_leftdown,
	.func_pt_rightup         = _Multix_pt_rightup,
	.func_pt_rightdown       = _Multix_pt_rightdown,
	.func_zoom_wide          = _Multix_zoom_wide,
	.func_zoom_tele          = _Multix_zoom_tele,
	.func_iris_open          = _Multix_iris_open,
	.func_iris_close         = _Multix_iris_close,
	.func_focus_near         = _Multix_focus_near,
	.func_focus_far          = _Multix_focus_far,
	.func_stop               = _Multix_stop,
	.func_set_preset         = _Multix_set_preset,
	.func_clear_preset       = _Multix_clear_preset,
	.func_goto_preset        = _Multix_goto_preset,
	.func_pattern_start      = NULL,
	.func_pattern_stop       = NULL,
	.func_pattern_set        = NULL,
	.func_run_pattern        = NULL,
	.func_set_zoom_speed     = NULL,
	.func_set_focus_speed    = NULL,
	.func_set_iris_speed     = NULL,
	.func_set_pantilt_speed  = NULL,
	.func_set_auto_focus     = NULL,
	.func_set_auto_iris      = NULL,
	.func_osd_up_key	     = NULL,
	.func_osd_down_key     	 = NULL,
	.func_osd_left_key       = NULL,
	.func_osd_right_key      = NULL,
	.func_osd_enter_key      = NULL,
	.func_osd_stop_key       = NULL,
	.func_reserved0          = NULL,
	.func_reserved1          = NULL,
	.func_reserved2          = NULL,
	.func_reserved3          = NULL,
	.func_reserved4          = NULL,
	.func_reserved5          = NULL,
	.func_reserved6          = NULL,
	.func_reserved7	         = NULL,
};











static unsigned char _Multix_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum = 0;

	g_return_val_if_fail(pos1>=0, FALSE);
	g_return_val_if_fail(pos2>=pos1, FALSE);

	for(i=pos1;i<=pos2;i++) 
		sum += data[i];

	return (unsigned char)sum;
}

static int _Multix_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{
	int ret = MULTIX_LEN;
	unsigned char speed = 0, zoomSpeed = 0;
	unsigned char array_speed[10] = {0x0c, 0x18, 0x20, 0x2c, 0x38, 0x44, 0x52, 0x60, 0x6f, 0x7f};
	unsigned char array_zoomSpeed[10] = {0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04};



	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);


	if ( cmd->params[0] )
	{
		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT) ||
				(cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_LEFTUP) || (cmd->cmd == NF_PTZ_CMD_PT_LEFTDOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_RIGHTUP) || (cmd->cmd == NF_PTZ_CMD_PT_RIGHTDOWN))
		{
			// pan & tilt speed
			speed = array_speed[(cmd->params[0]/10) - 1];
		}
		else if((cmd->cmd == NF_PTZ_CMD_SET_PRESET) || (cmd->cmd == NF_PTZ_CMD_GOTO_PRESET)) 
		{
			speed = array_speed[(sysdb_ch->pt_spd/10) - 1];
		}
		else if((cmd->cmd == NF_PTZ_CMD_ZOOM_TELE) || (cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE)) 
		{
			// zoom speed
			zoomSpeed = array_zoomSpeed[(cmd->params[0]/10) - 1];
		}
	}
	else
	{
		//g_message("%s sysdb_ch->pt_spd : %d\n", __FUNCTION__,sysdb_ch->pt_spd);

		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT) || 
				(cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_LEFTUP) || (cmd->cmd == NF_PTZ_CMD_PT_LEFTDOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_RIGHTUP) || (cmd->cmd == NF_PTZ_CMD_PT_RIGHTDOWN) ||
				(cmd->cmd == NF_PTZ_CMD_SET_PRESET) || (cmd->cmd == NF_PTZ_CMD_GOTO_PRESET)) {

			speed = array_speed[(sysdb_ch->pt_spd/10) - 1];
//			fprintf(stderr, "speed : %d\n", speed);
//			fprintf(stderr, "sysdb_ch->pt_spd : %d\n", sysdb_ch->pt_spd);
		}
		else if((cmd->cmd == NF_PTZ_CMD_ZOOM_TELE) || (cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE)) {
			zoomSpeed = array_zoomSpeed[(sysdb_ch->zoom_spd/10) - 1];
		}
	}


	// Start Code
	buff[0] = 0xe5;

	/* Device ID
	 * 0x01 : Dome Camera, 0x02 : DVR, 0x03 ~ 0xFF : Reserved
	 */
	buff[1] = 0x01;

	// Host ID
	buff[2] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[3] = 0x00;
			buff[4] = 0x80;
			buff[5] = speed;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[3] = 0x00;
			buff[4] = 0x40;
			buff[5] = speed;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[3] = 0x00;
			buff[4] = 0x20;
			buff[5] = 0x00;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[3] = 0x00;
			buff[4] = 0x10;
			buff[5] = 0x00;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_PT_LEFTUP:
			buff[3] = 0x00;
			buff[4] = 0xa0;
			buff[5] = speed;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x90;
			buff[5] = speed;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[3] = 0x00;
			buff[4] = 0x60;
			buff[5] = speed;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x50;
			buff[5] = speed;
			buff[6] = speed;
			break;

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[3] = 0x00;
			buff[4] = 0x08;
			if(zoomSpeed == 0x00) {
				buff[5] = 0x00, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x01) {
				buff[5] = 0x00, buff[6] = 0x01;
			}
			else if(zoomSpeed == 0x02) {
				buff[5] = 0x01, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x03) {
				buff[5] = 0x01, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x04) {
				buff[5] = 0x01, buff[6] = 0x01;
			}
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[3] = 0x00;
			buff[4] = 0x04;
			if(zoomSpeed == 0x00) {
				buff[5] = 0x00, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x01) {
				buff[5] = 0x00, buff[6] = 0x01;
			}
			else if(zoomSpeed == 0x02) {
				buff[5] = 0x01, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x03) {
				buff[5] = 0x01, buff[6] = 0x00;
			}
			else if(zoomSpeed == 0x04) {
				buff[5] = 0x01, buff[6] = 0x01;
			}
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
			buff[3] = 0x04;
			buff[4] = 0x00;
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[3] = 0x02;
			buff[4] = 0x00;
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[3] = 0x00;
			buff[4] = 0x01;
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[3] = 0x00;
			buff[4] = 0x02;
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_STOP:
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		case NF_PTZ_CMD_SET_PRESET:
			buff[3] = 0xa1;
			buff[4] = cmd->params[0];
			buff[5] = speed;
			buff[6] = 0xff;
			break;

		case NF_PTZ_CMD_GOTO_PRESET:
			buff[3] = 0xa4;
			buff[4] = cmd->params[0];
			buff[5] = 0x00;
			buff[6] = 0x00;
			break;

		default:
			break;
	}

	buff[7] = _Multix_checksum(buff, 0, ret - 2);

	return ret;
}


