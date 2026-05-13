#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define LPT_A100L_LEN		6



static int _LPT_A100L_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _LPT_A100L_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}

int _LPT_A100L_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}

int _LPT_A100L_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}

int _LPT_A100L_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}

int _LPT_A100L_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}
int _LPT_A100L_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}
int _LPT_A100L_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}
int _LPT_A100L_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}


int _LPT_A100L_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);

}

int _LPT_A100L_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);

}

int _LPT_A100L_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);

}

int _LPT_A100L_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);

}

int _LPT_A100L_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);

}

int _LPT_A100L_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}


int _LPT_A100L_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command(cmd_buff, cmd, sysdb_ch);
}


int _LPT_A100L_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command( cmd_buff, cmd , sysdb_ch);
}

int _LPT_A100L_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _LPT_A100L_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _LPT_A100L_command( cmd_buff, cmd , sysdb_ch);
}





NF_PTZ_PROTOCOL	_nf_ptz_proto_LPT_A100L = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "LPT-A100L", 
	.func_pan_left           = _LPT_A100L_pan_left,
	.func_pan_right          = _LPT_A100L_pan_right,
	.func_tilt_up            = _LPT_A100L_tilt_up,
	.func_tilt_down          = _LPT_A100L_tilt_down,
	.func_pt_leftup          = _LPT_A100L_pt_leftup,
	.func_pt_leftdown        = _LPT_A100L_pt_leftdown,
	.func_pt_rightup         = _LPT_A100L_pt_rightup,
	.func_pt_rightdown       = _LPT_A100L_pt_rightdown,
	.func_zoom_wide          = _LPT_A100L_zoom_wide,
	.func_zoom_tele          = _LPT_A100L_zoom_tele,
	.func_iris_open          = _LPT_A100L_iris_open,
	.func_iris_close         = _LPT_A100L_iris_close,
	.func_focus_near         = _LPT_A100L_focus_near,
	.func_focus_far          = _LPT_A100L_focus_far,
	.func_stop               = _LPT_A100L_stop,
	.func_set_preset         = _LPT_A100L_set_preset,
	.func_clear_preset       = _LPT_A100L_clear_preset,
	.func_goto_preset        = _LPT_A100L_goto_preset,
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











static unsigned char _LPT_A100L_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum = 0;

	g_return_val_if_fail(pos1>=0, FALSE);
	g_return_val_if_fail(pos2>=pos1, FALSE);

	for(i=pos1;i<=pos2;i++) 
		sum += data[i];

	return (unsigned char)sum & 0x7f;
}

static int _LPT_A100L_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = LPT_A100L_LEN;
	unsigned char ptSpeed = 0;
	unsigned char zoomSpeed = 0;
	unsigned char focusSpeed = 0;
	unsigned char array_Speed[10] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31, 0x31};


	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		ptSpeed = array_Speed[(cmd->params[0]/10) - 1];
	}
	else
	{
		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT) || 
				(cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_LEFTUP) || (cmd->cmd == NF_PTZ_CMD_PT_LEFTDOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_RIGHTUP) || (cmd->cmd == NF_PTZ_CMD_PT_RIGHTDOWN))
		{
			//g_message("%s sysdb_ch->pt_spd : %d\n", __FUNCTION__,sysdb_ch->pt_spd);
			ptSpeed = array_Speed[(sysdb_ch->pt_spd/10) - 1];
		}
		else if((cmd->cmd == NF_PTZ_CMD_ZOOM_TELE) || (cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE)) 
		{
			zoomSpeed = array_Speed[(sysdb_ch->zoom_spd/10) - 1];
		}
		else if((cmd->cmd == NF_PTZ_CMD_FOCUS_FAR) || (cmd->cmd == NF_PTZ_CMD_FOCUS_NEAR))
		{
			focusSpeed = array_Speed[(sysdb_ch->focus_spd/10) - 1];
		}

	}
	
	// Start Code
	buff[0] = 0x02;
	// address
	buff[1] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[2] = 0x38;
			buff[3] = ptSpeed;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[2] = 0x39;
			buff[3] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[2] = 0x36;
			buff[3] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[2] = 0x37;
			buff[3] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[2] = 0x3a;
			buff[3] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[2] = 0x3b;
			buff[3] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[2] = 0x3d;
			buff[3] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[2] = 0x3c;
			buff[3] = ptSpeed;
			break;

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[2] = 0x3e;
			buff[3] = zoomSpeed;
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[2] = 0x3f;
			buff[3] = zoomSpeed;
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
		case NF_PTZ_CMD_IRIS_CLOSE:
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[2] = 0x41;
			buff[3] = focusSpeed;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[2] = 0x40;
			buff[3] = focusSpeed;
			break;

		case NF_PTZ_CMD_STOP:
			buff[2] = 0x00;
			buff[3] = 0x00;
			break;

		case NF_PTZ_CMD_SET_PRESET:
		case NF_PTZ_CMD_GOTO_PRESET:
			break;

		default:
			break;
	}

	// ETX 
	buff[4] = 0x03;
	// checksum
	buff[5] = _LPT_A100L_checksum(buff, 0, ret - 1);

	return ret;
}


