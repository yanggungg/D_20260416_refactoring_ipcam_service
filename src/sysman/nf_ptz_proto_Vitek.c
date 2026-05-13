#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define VITEK_LEN		7

static int _Vitek_ptz_flag = 0;

static int 				_Vitek_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_Vitek_checksum(unsigned char *data, int pos1, int pos2);

int _Vitek_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}
int _Vitek_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}
int _Vitek_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}
int _Vitek_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);

}

int _Vitek_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);

}

int _Vitek_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);

}

int _Vitek_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);

}

int _Vitek_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);

}

int _Vitek_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = TRUE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Vitek_ptz_flag = FALSE;
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Vitek_command( cmd_buff, cmd , sysdb_ch);
}

int _Vitek_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Vitek_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_Vitek = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "VITEK", 
	.func_pan_left           = _Vitek_pan_left,
	.func_pan_right          = _Vitek_pan_right,
	.func_tilt_up            = _Vitek_tilt_up,
	.func_tilt_down          = _Vitek_tilt_down,
	.func_pt_leftup          = _Vitek_pt_leftup,
	.func_pt_leftdown        = _Vitek_pt_leftdown,
	.func_pt_rightup         = _Vitek_pt_rightup,
	.func_pt_rightdown       = _Vitek_pt_rightdown,
	.func_zoom_wide          = _Vitek_zoom_wide,
	.func_zoom_tele          = _Vitek_zoom_tele,
	.func_iris_open          = _Vitek_iris_open,
	.func_iris_close         = _Vitek_iris_close,
	.func_focus_near         = _Vitek_focus_near,
	.func_focus_far          = _Vitek_focus_far,
	.func_stop               = _Vitek_stop,
	.func_set_preset         = _Vitek_set_preset,
	.func_clear_preset       = _Vitek_clear_preset,
	.func_goto_preset        = _Vitek_goto_preset,
	.func_pattern_start      = _Vitek_pattern_start,
	.func_pattern_stop       = _Vitek_pattern_stop,
	.func_pattern_set        = _Vitek_pattern_set,
	.func_run_pattern        = _Vitek_run_pattern,
	.func_set_zoom_speed     = _Vitek_set_zoom_speed,
	.func_set_focus_speed    = _Vitek_set_focus_speed,
	.func_set_iris_speed     = _Vitek_set_iris_speed,
	.func_set_pantilt_speed  = _Vitek_set_pantilt_speed,
	.func_set_auto_focus     = _Vitek_set_auto_focus,
	.func_set_auto_iris      = _Vitek_set_auto_iris,
	.func_osd_up_key	     = _Vitek_osd_up_key,	  
	.func_osd_down_key     	 = _Vitek_osd_down_key,    
	.func_osd_left_key       = _Vitek_osd_left_key,    
	.func_osd_right_key      = _Vitek_osd_right_key,   
	.func_osd_enter_key      = _Vitek_osd_enter_key,   
	.func_osd_stop_key       = _Vitek_osd_stop_key,
	.func_reserved0          = _Vitek_reserved0,
	.func_reserved1          = _Vitek_reserved1,
	.func_reserved2          = _Vitek_reserved2,
	.func_reserved3          = _Vitek_reserved3,
	.func_reserved4          = _Vitek_reserved4,
	.func_reserved5          = _Vitek_reserved5,
	.func_reserved6          = _Vitek_reserved6,
	.func_reserved7	         = _Vitek_reserved7
};


static unsigned char _Vitek_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum=0;

	g_return_val_if_fail ( pos1 >= 0,  FALSE);
	g_return_val_if_fail ( pos2 >= pos1,  FALSE);
	
	for(i=pos1; i <= pos2; i++){
		sum += data[i];
	}
	return (unsigned char)(sum ^ 0xa5);
}

static int _Vitek_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = VITEK_LEN;
	//unsigned char ptSpeed[] = {0x00,0x04,0x07,0x14,0x17,0x24,0x27,0x34,0x3c,0x3F};
	// Speed 100%단위 비율
	unsigned char sp, zsp;
	char ptspeed[] = {0x06, 0x0C, 0x12, 0x18, 0x1E, 0x24, 0x2A, 0x30, 0x36, 0x3F};
	char zmspeed[] = {   0,    0,    1,    2,    3,    4,    5,    6,    7,    7};

	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		sp = cmd->params[0] / 10;
		sp--;
		zsp = cmd->params[0] / 10;
		zsp--;
		g_message("%s RA rcv Speed[%d]", __FUNCTION__, cmd->params[0]);
	}
	else
	{
		sp = (sysdb_ch->pt_spd) / 10;
		sp--;
		zsp = (sysdb_ch->zoom_spd) / 10;
		zsp--;
		g_message("%s DVR PT Speed[%d]", __FUNCTION__, sysdb_ch->pt_spd);
		g_message("%s DVR ZOOM Speed[%d]", __FUNCTION__, sysdb_ch->zoom_spd);
	}
	
	// Header
	buff[0] = 0xE5;
	buff[1] = 0x10;

	// addr
	buff[2] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x00;

			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00 | ptspeed[sp];
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x80 | ptspeed[sp];
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;

		case NF_PTZ_CMD_PT_LEFTUP:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x00 | ptspeed[sp];

			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x80 | ptspeed[sp];

			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x00 | ptspeed[sp];

			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x80 | ptspeed[sp];

			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[3] = 0x10 | zmspeed[zsp];
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[3] = 0x18 | zmspeed[zsp];
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[3] = 0x01;
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[3] = 0x02;
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[3] = 0x04;
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[3] = 0x08;
			buff[4] = 0x00;
			buff[5] = 0x00;
			if(_Vitek_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[3] = 0x82;
			buff[4] = cmd->params[0];
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[3] = 0x81;
			buff[4] = cmd->params[0];
			buff[5] = 0x00;
			break;	
		case NF_PTZ_CMD_STOP:
			nf_ptz_releaseCmd(cmd);
			_Vitek_ptz_flag = FALSE;
			buff[3] = 0x00;
			buff[4] = 0x80;
			buff[5] = 0x00;
			break;

			/* continuous PTZ CMD */
		case NF_PTZ_CMD_PAN_LEFT_CON:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_PAN_RIGHT_CON:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_TILT_UP_CON:
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_TILT_DOWN_CON:
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x80 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_PT_LEFTUP_CON:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x00 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN_CON:
			buff[3] = 0x00;
			buff[4] = 0x80 | ptspeed[sp];
			buff[5] = 0x80 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_PT_RIGHTUP_CON:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x00 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN_CON:
			buff[3] = 0x00;
			buff[4] = 0x00 | ptspeed[sp];
			buff[5] = 0x80 | ptspeed[sp];
			break;
		case NF_PTZ_CMD_ZOOM_TELE_CON:
			buff[3] = 0x10 | zmspeed[zsp];
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_ZOOM_WIDE_CON:
			buff[3] = 0x18 | zmspeed[zsp];
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_IRIS_OPEN_CON:
			buff[3] = 0x01;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_IRIS_CLOSE_CON:
			buff[3] = 0x02;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_FOCUS_FAR_CON:
			buff[3] = 0x04;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_FOCUS_NEAR_CON:
			buff[3] = 0x08;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;		

		default:
			break;
	}

	buff[6] = _Vitek_checksum( buff, 0, ret - 2);

	//g_message("%s data [%x][%x][%x][%x][%x][%x][%x]", __FUNCTION__, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6]);

	return ret;	
}

