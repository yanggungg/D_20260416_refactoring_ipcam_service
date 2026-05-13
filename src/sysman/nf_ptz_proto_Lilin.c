#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define LILIN_LEN		3

static int _Lilin_ptz_flag = 0;

static int 				_Lilin_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_Lilin_checksum(unsigned char *data, int pos1, int pos2);

int _Lilin_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}
int _Lilin_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}
int _Lilin_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}
int _Lilin_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);

}

int _Lilin_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);

}

int _Lilin_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);

}

int _Lilin_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);

}

int _Lilin_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);

}

int _Lilin_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = TRUE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_Lilin_ptz_flag = FALSE;
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Lilin_command( cmd_buff, cmd , sysdb_ch);
}

int _Lilin_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Lilin_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_Lilin = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "LILIN", 
	.func_pan_left           = _Lilin_pan_left,
	.func_pan_right          = _Lilin_pan_right,
	.func_tilt_up            = _Lilin_tilt_up,
	.func_tilt_down          = _Lilin_tilt_down,
	.func_pt_leftup          = _Lilin_pt_leftup,
	.func_pt_leftdown        = _Lilin_pt_leftdown,
	.func_pt_rightup         = _Lilin_pt_rightup,
	.func_pt_rightdown       = _Lilin_pt_rightdown,
	.func_zoom_wide          = _Lilin_zoom_wide,
	.func_zoom_tele          = _Lilin_zoom_tele,
	.func_iris_open          = _Lilin_iris_open,
	.func_iris_close         = _Lilin_iris_close,
	.func_focus_near         = _Lilin_focus_near,
	.func_focus_far          = _Lilin_focus_far,
	.func_stop               = _Lilin_stop,
	.func_set_preset         = _Lilin_set_preset,
	.func_clear_preset       = _Lilin_clear_preset,
	.func_goto_preset        = _Lilin_goto_preset,
	.func_pattern_start      = _Lilin_pattern_start,
	.func_pattern_stop       = _Lilin_pattern_stop,
	.func_pattern_set        = _Lilin_pattern_set,
	.func_run_pattern        = _Lilin_run_pattern,
	.func_set_zoom_speed     = _Lilin_set_zoom_speed,
	.func_set_focus_speed    = _Lilin_set_focus_speed,
	.func_set_iris_speed     = _Lilin_set_iris_speed,
	.func_set_pantilt_speed  = _Lilin_set_pantilt_speed,
	.func_set_auto_focus     = _Lilin_set_auto_focus,
	.func_set_auto_iris      = _Lilin_set_auto_iris,
	.func_osd_up_key	     = _Lilin_osd_up_key,	  
	.func_osd_down_key     	 = _Lilin_osd_down_key,    
	.func_osd_left_key       = _Lilin_osd_left_key,    
	.func_osd_right_key      = _Lilin_osd_right_key,   
	.func_osd_enter_key      = _Lilin_osd_enter_key,   
	.func_osd_stop_key       = _Lilin_osd_stop_key,
	.func_reserved0          = _Lilin_reserved0,
	.func_reserved1          = _Lilin_reserved1,
	.func_reserved2          = _Lilin_reserved2,
	.func_reserved3          = _Lilin_reserved3,
	.func_reserved4          = _Lilin_reserved4,
	.func_reserved5          = _Lilin_reserved5,
	.func_reserved6          = _Lilin_reserved6,
	.func_reserved7	         = _Lilin_reserved7
};



static int _Lilin_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = LILIN_LEN;
	unsigned char pan_sp[10] = {0x01, 0x02, 0x02, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07};
	unsigned char tilt_sp[10] ={0x08, 0x0a, 0x0a, 0x0a, 0x20, 0x20, 0x28, 0x30, 0x38, 0x38};
	unsigned char ptsp;
	unsigned char sp, tp;
	

	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		sp = cmd->params[0] / 10;
		sp--;
		g_message("%s RA rcv Speed[%d]", __FUNCTION__, cmd->params[0]);
	}
	else
	{
		sp = (sysdb_ch->pt_spd) / 10;
		sp--;
		g_message("%s DVR Speed[%d]", __FUNCTION__, sysdb_ch->pt_spd);
	}

	ptsp = pan_sp[sp] + tilt_sp[sp];
	sp = pan_sp[sp];
	tp = tilt_sp[sp];
	
	// addr
	buff[0] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[1] = 0x02;
			buff[2] = sp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[1] = 0x01;
			buff[2] = sp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[1] = 0x04;
			buff[2] = tp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[1] = 0x08;
			buff[2] = tp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[1] = 0x06;
			buff[2] = ptsp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[1] = 0x0a;
			buff[2] = ptsp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[1] = 0x05;
			buff[2] = ptsp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[1] = 0x09;
			buff[2] = ptsp;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[1] = 0x10;
			buff[2] = 0xa4;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[1] = 0x20;
			buff[2] = 0xa4;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[1] = 0x40;
			buff[2] = 0xa4;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[1] = 0x80;
			buff[2] = 0xa4;
			if(_Lilin_ptz_flag) 
			{
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[0] = 0x80 + sysdb_ch->addr;
			buff[1] = cmd->params[0];
			buff[2] = 0xa4;
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[0] = 0x40 + sysdb_ch->addr;
			buff[1] = cmd->params[0];
			buff[2] = 0xa4;
			break;	
		case NF_PTZ_CMD_STOP:
			nf_ptz_releaseCmd(cmd);
			_Lilin_ptz_flag = FALSE;
			buff[1] = 0x00;
			buff[2] = 0xff;
			break;

		default:
			break;
	}

	return ret;	
}

