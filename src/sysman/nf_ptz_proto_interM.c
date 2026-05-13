#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define interM_LEN		6

static int _interM_ptz_flag = 0;

static int 				_interM_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);

int _interM_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}
int _interM_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}
int _interM_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}
int _interM_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}
int _interM_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);

}

int _interM_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);

}

int _interM_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);

}

int _interM_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);

}

int _interM_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);

}

int _interM_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = TRUE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_interM_ptz_flag = FALSE;
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _interM_command( cmd_buff, cmd , sysdb_ch);
}

int _interM_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _interM_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_interM = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "interM", 
	.func_pan_left           = _interM_pan_left,
	.func_pan_right          = _interM_pan_right,
	.func_tilt_up            = _interM_tilt_up,
	.func_tilt_down          = _interM_tilt_down,
	.func_pt_leftup          = _interM_pt_leftup,
	.func_pt_leftdown        = _interM_pt_leftdown,
	.func_pt_rightup         = _interM_pt_rightup,
	.func_pt_rightdown       = _interM_pt_rightdown,
	.func_zoom_wide          = _interM_zoom_wide,
	.func_zoom_tele          = _interM_zoom_tele,
	.func_iris_open          = _interM_iris_open,
	.func_iris_close         = _interM_iris_close,
	.func_focus_near         = _interM_focus_near,
	.func_focus_far          = _interM_focus_far,
	.func_stop               = _interM_stop,
	.func_set_preset         = _interM_set_preset,
	.func_clear_preset       = _interM_clear_preset,
	.func_goto_preset        = _interM_goto_preset,
	.func_pattern_start      = _interM_pattern_start,
	.func_pattern_stop       = _interM_pattern_stop,
	.func_pattern_set        = _interM_pattern_set,
	.func_run_pattern        = _interM_run_pattern,
	.func_set_zoom_speed     = _interM_set_zoom_speed,
	.func_set_focus_speed    = _interM_set_focus_speed,
	.func_set_iris_speed     = _interM_set_iris_speed,
	.func_set_pantilt_speed  = _interM_set_pantilt_speed,
	.func_set_auto_focus     = _interM_set_auto_focus,
	.func_set_auto_iris      = _interM_set_auto_iris,
	.func_osd_up_key	     = _interM_osd_up_key,	  
	.func_osd_down_key     	 = _interM_osd_down_key,    
	.func_osd_left_key       = _interM_osd_left_key,    
	.func_osd_right_key      = _interM_osd_right_key,   
	.func_osd_enter_key      = _interM_osd_enter_key,   
	.func_osd_stop_key       = _interM_osd_stop_key,
	.func_reserved0          = _interM_reserved0,
	.func_reserved1          = _interM_reserved1,
	.func_reserved2          = _interM_reserved2,
	.func_reserved3          = _interM_reserved3,
	.func_reserved4          = _interM_reserved4,  
	.func_reserved5          = _interM_reserved5,  
	.func_reserved6          = _interM_reserved6, 
	.func_reserved7	         = _interM_reserved7   
};



static int _interM_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = interM_LEN;
	//unsigned char ptSpeed[] = {0x00,0x04,0x07,0x14,0x17,0x24,0x27,0x34,0x3c,0x3F};
	// Speed 100%단위 비율
	unsigned char ptSpeed, zoomSpeed, focusSpeed;

	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	if ( cmd->params[0] )
	{
	//	ptSpeed = 0xff & ((63 * cmd->params[0]) / 100);
		ptSpeed = cmd->params[0];
		zoomSpeed = cmd->params[0];
		focusSpeed = cmd->params[0];
		g_message("%s RA rcv Speed[%d]", __FUNCTION__, cmd->params[0]);
	}
	else
	{
	//	ptSpeed = 0xff & ((63 * sysdb_ch->pt_spd) / 100);
		ptSpeed = sysdb_ch->pt_spd;
		zoomSpeed = sysdb_ch->zoom_spd;
		focusSpeed = sysdb_ch->focus_spd;
		g_message("%s DVR Speed[%d]", __FUNCTION__, sysdb_ch->pt_spd);
	}
	

	// Address
	buff[0] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[1] = 0x02;
			buff[2] = 0x79 - ptSpeed;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x80;

			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PAN_LEFT);
			}
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[1] = 0x02;
			buff[2] = 0x81 + ptSpeed;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x80;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PAN_RIGHT);
			}
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x81 + ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_TILT_UP);
			}
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x79 - ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_TILT_DOWN);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[1] = 0x02;
			buff[2] = 0x79 - ptSpeed;
			buff[3] = 0x81 + ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;

			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_LEFTUP);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[1] = 0x02;
			buff[2] = 0x79 - ptSpeed;
			buff[3] = 0x79 - ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;

			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_LEFTDOWN);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[1] = 0x02;
			buff[2] = 0x81 + ptSpeed;
			buff[3] = 0x81 + ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;

			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_RIGHTUP);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[1] = 0x02;
			buff[2] = 0x81 + ptSpeed;
			buff[3] = 0x79 - ptSpeed;
			buff[4] = 0x80;
			buff[5] = 0x80;

			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_RIGHTDOWN);
			}
			break;

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x81 + zoomSpeed;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_ZOOM_TELE);
			}
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x79 - zoomSpeed;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_ZOOM_WIDE);
			}
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x80;
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x80;
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x81 + focusSpeed;
			buff[5] = 0x80;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_FOCUS_FAR);
			}
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x71 - focusSpeed;
			buff[5] = 0x80;
			if(_interM_ptz_flag)
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_FOCUS_NEAR);
			}
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[1] = 0x03;
			buff[2] = cmd->params[0];
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[1] = 0x01;
			buff[2] = cmd->params[0];
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;	
		case NF_PTZ_CMD_STOP:
			nf_ptz_releaseCmd(cmd);
			_interM_ptz_flag = FALSE;
			buff[1] = 0x02;
			buff[2] = 0x80;
			buff[3] = 0x80;
			buff[4] = 0x80;
			buff[5] = 0x80;
			break;

		default:
			break;
	}


	return ret;	
}

