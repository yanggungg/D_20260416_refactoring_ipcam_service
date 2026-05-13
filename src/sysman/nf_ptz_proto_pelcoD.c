#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define PELCO_D_LEN		7

static int 				_pelcoD_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_pelcoD_checksum(unsigned char *data, int pos1, int pos2);

int _pelcoD_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}
int _pelcoD_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}
int _pelcoD_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}
int _pelcoD_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}
int _pelcoD_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);

}

int _pelcoD_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _pelcoD_command( cmd_buff, cmd , sysdb_ch);
}

int _pelcoD_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _pelcoD_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_pelcoD = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "PELCO-D", 
	.func_pan_left           = _pelcoD_pan_left,
	.func_pan_right          = _pelcoD_pan_right,
	.func_tilt_up            = _pelcoD_tilt_up,
	.func_tilt_down          = _pelcoD_tilt_down,
	.func_pt_leftup          = _pelcoD_pt_leftup,
	.func_pt_leftdown        = _pelcoD_pt_leftdown,
	.func_pt_rightup         = _pelcoD_pt_rightup,
	.func_pt_rightdown       = _pelcoD_pt_rightdown,
	.func_zoom_wide          = _pelcoD_zoom_wide,
	.func_zoom_tele          = _pelcoD_zoom_tele,
	.func_iris_open          = _pelcoD_iris_open,
	.func_iris_close         = _pelcoD_iris_close,
	.func_focus_near         = _pelcoD_focus_near,
	.func_focus_far          = _pelcoD_focus_far,
	.func_stop               = _pelcoD_stop,
	.func_set_preset         = _pelcoD_set_preset,
	.func_clear_preset       = _pelcoD_clear_preset,
	.func_goto_preset        = _pelcoD_goto_preset,
	.func_pattern_start      = _pelcoD_pattern_start,
	.func_pattern_stop       = _pelcoD_pattern_stop,
	.func_pattern_set        = _pelcoD_pattern_set,
	.func_run_pattern        = _pelcoD_run_pattern,
	.func_set_zoom_speed     = _pelcoD_set_zoom_speed,
	.func_set_focus_speed    = _pelcoD_set_focus_speed,
	.func_set_iris_speed     = _pelcoD_set_iris_speed,
	.func_set_pantilt_speed  = _pelcoD_set_pantilt_speed,
	.func_set_auto_focus     = _pelcoD_set_auto_focus,
	.func_set_auto_iris      = _pelcoD_set_auto_iris,
	.func_osd_up_key	     = _pelcoD_osd_up_key,	  
	.func_osd_down_key     	 = _pelcoD_osd_down_key,    
	.func_osd_left_key       = _pelcoD_osd_left_key,    
	.func_osd_right_key      = _pelcoD_osd_right_key,   
	.func_osd_enter_key      = _pelcoD_osd_enter_key,   
	.func_osd_stop_key       = _pelcoD_osd_stop_key,
	.func_reserved0          = _pelcoD_reserved0,
	.func_reserved1          = _pelcoD_reserved1,
	.func_reserved2          = _pelcoD_reserved2,
	.func_reserved3          = _pelcoD_reserved3,
	.func_reserved4          = _pelcoD_reserved4,
	.func_reserved5          = _pelcoD_reserved5,
	.func_reserved6          = _pelcoD_reserved6,
	.func_reserved7	         = _pelcoD_reserved7
};


static unsigned char _pelcoD_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum=0;

	g_return_val_if_fail ( pos1 >= 0,  FALSE);
	g_return_val_if_fail ( pos2 >= pos1,  FALSE);
	
	for(i=pos1; i <= pos2; i++){
		sum += data[i];
	}
	return (unsigned char)(sum % 256);
}

static int _pelcoD_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = PELCO_D_LEN;
	//unsigned char ptSpeed[] = {0x00,0x04,0x07,0x14,0x17,0x24,0x27,0x34,0x3c,0x3F};
	// Speed 100%단위 비율
	unsigned char ptSpeed;
	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		ptSpeed = 0xff & ((63 * cmd->params[0]) / 100);
		g_message("%s RA rcv Speed[%d]", __FUNCTION__, cmd->params[0]);
	}
	else
	{
		ptSpeed = 0xff & ((63 * sysdb_ch->pt_spd) / 100);
		g_message("%s DVR Speed[%d]", __FUNCTION__, sysdb_ch->pt_spd);
	}
	
	// Header
	buff[0] = 0xFF;
	// Address
	buff[1] = sysdb_ch->addr;
	// Command1
	// Command2 
	// Data speed 0x00 ~ 0x3F

	switch(cmd->cmd)
	{
		
		case NF_PTZ_CMD_PAN_LEFT:
			buff[2] = 0x00;
			buff[3] = 0x04;
			buff[4] = ptSpeed;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[2] = 0x00;
			buff[3] = 0x02;
			buff[4] = ptSpeed;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[2] = 0x00;
			buff[3] = 0x08;
			buff[4] = 0x00;
			buff[5] = ptSpeed;
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[2] = 0x00;
			buff[3] = 0x10;
			buff[4] = 0x00;
			buff[5] = ptSpeed;
			break;
		
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[2] = 0x00;
			buff[3] = 0x0c;
			buff[4] = ptSpeed;
			buff[5] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[2] = 0x00;
			buff[3] = 0x14;
			buff[4] = ptSpeed;
			buff[5] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[2] = 0x00;
			buff[3] = 0x0a;
			buff[4] = ptSpeed;
			buff[5] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[2] = 0x00;
			buff[3] = 0x12;
			buff[4] = ptSpeed;
			buff[5] = ptSpeed;
			break;


		case NF_PTZ_CMD_ZOOM_TELE:
			buff[2] = 0x00;
			buff[3] = 0x20;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[2] = 0x00;
			buff[3] = 0x40;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[2] = 0x02;
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[2] = 0x04;
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[2] = 0x00;
			buff[3] = 0x80;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[2] = 0x01;
			buff[3] = 0x00;
			buff[4] = 0x00;
			buff[5] = 0x00;
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[2] = 0x00;
			buff[3] = 0x03;
			buff[4] = 0x00;
			buff[5] = cmd->params[0];
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[2] = 0x00;
			buff[3] = 0x07;
			buff[4] = 0x00;
			buff[5] = cmd->params[0];
			break;	

		case NF_PTZ_CMD_SET_AUTO_FOCUS:
			if(sysdb_ch->auto_focus) {
				buff[2] = 0x00;
				buff[3] = 0x2b;
				buff[4] = 0x00;
				buff[5] = 0x00;
			}
			else {
				buff[2] = 0x00;
				buff[3] = 0x2b;
				buff[4] = 0x00;
				buff[5] = 0x02;
			}
			break;
		case NF_PTZ_CMD_SET_AUTO_IRIS:
			if(sysdb_ch->auto_iris) {
				buff[2] = 0x00;
				buff[3] = 0x2d;
				buff[4] = 0x00;
				buff[5] = 0x00;
			}
			else {
				buff[2] = 0x00;
				buff[3] = 0x2d;
				buff[4] = 0x00;
				buff[5] = 0x02;
			}
			break;
		default:
			break;
	}

	buff[6] = _pelcoD_checksum( buff, 1, ret - 2);	

	return ret;	
}

