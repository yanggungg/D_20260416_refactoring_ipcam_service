#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define fastraxII_2_LEN		7

static int 				_fastraxII_2_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_fastraxII_2_checksum(unsigned char *data, int pos1, int pos2);

static int _fastrax_ptz_flag = 0;

int _fastraxII_2_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}
int _fastraxII_2_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}
int _fastraxII_2_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}
int _fastraxII_2_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}
int _fastraxII_2_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);

}

int _fastraxII_2_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);

}

int _fastraxII_2_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);

}

int _fastraxII_2_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);

}

int _fastraxII_2_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);

}

int _fastraxII_2_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	_fastrax_ptz_flag = TRUE;
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	

	_fastrax_ptz_flag = FALSE;
	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _fastraxII_2_command( cmd_buff, cmd , sysdb_ch);
}

int _fastraxII_2_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _fastraxII_2_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_fastraxII_2 = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "FastraxII_2", 
	.func_pan_left           = _fastraxII_2_pan_left,
	.func_pan_right          = _fastraxII_2_pan_right,
	.func_tilt_up            = _fastraxII_2_tilt_up,
	.func_tilt_down          = _fastraxII_2_tilt_down,
	.func_pt_leftup          = _fastraxII_2_pt_leftup,
	.func_pt_leftdown        = _fastraxII_2_pt_leftdown,
	.func_pt_rightup         = _fastraxII_2_pt_rightup,
	.func_pt_rightdown       = _fastraxII_2_pt_rightdown,
	.func_zoom_wide          = _fastraxII_2_zoom_wide,
	.func_zoom_tele          = _fastraxII_2_zoom_tele,
	.func_iris_open          = _fastraxII_2_iris_open,
	.func_iris_close         = _fastraxII_2_iris_close,
	.func_focus_near         = _fastraxII_2_focus_near,
	.func_focus_far          = _fastraxII_2_focus_far,
	.func_stop               = _fastraxII_2_stop,
	.func_set_preset         = _fastraxII_2_set_preset,
	.func_clear_preset       = _fastraxII_2_clear_preset,
	.func_goto_preset        = _fastraxII_2_goto_preset,
	.func_pattern_start      = _fastraxII_2_pattern_start,
	.func_pattern_stop       = _fastraxII_2_pattern_stop,
	.func_pattern_set        = _fastraxII_2_pattern_set,
	.func_run_pattern        = _fastraxII_2_run_pattern,
	.func_set_zoom_speed     = _fastraxII_2_set_zoom_speed,
	.func_set_focus_speed    = _fastraxII_2_set_focus_speed,
	.func_set_iris_speed     = _fastraxII_2_set_iris_speed,
	.func_set_pantilt_speed  = _fastraxII_2_set_pantilt_speed,
	.func_set_auto_focus     = _fastraxII_2_set_auto_focus,
	.func_set_auto_iris      = _fastraxII_2_set_auto_iris,
	.func_osd_up_key	     = _fastraxII_2_osd_up_key,	  
	.func_osd_down_key       = _fastraxII_2_osd_down_key,    
	.func_osd_left_key       = _fastraxII_2_osd_left_key,    
	.func_osd_right_key      = _fastraxII_2_osd_right_key,   
	.func_osd_enter_key      = _fastraxII_2_osd_enter_key,   
	.func_osd_stop_key       = _fastraxII_2_osd_stop_key, 
	.func_reserved0          = _fastraxII_2_reserved0,
	.func_reserved1          = _fastraxII_2_reserved1,
	.func_reserved2          = _fastraxII_2_reserved2,
	.func_reserved3          = _fastraxII_2_reserved3,
	.func_reserved4          = _fastraxII_2_reserved4,
	.func_reserved5          = _fastraxII_2_reserved5,
	.func_reserved6          = _fastraxII_2_reserved6,
	.func_reserved7	         = _fastraxII_2_reserved7
};


static unsigned char _fastraxII_2_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum=0;

	g_return_val_if_fail ( pos1 >= 0,  FALSE);
	g_return_val_if_fail ( pos2 >= pos1,  FALSE);
	
	for(i=pos1; i <= pos2; i++){
		sum += data[i];
	}
	
	return (unsigned char)(sum & 0xff);
}

static int _fastraxII_2_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	char plspeed[] = {7, 6, 5, 4, 3, 2, 1};
	char prspeed[] = {9, 10, 11, 12, 13, 14, 15};
	
	int ret = fastraxII_2_LEN;

	// Speed 100%단위 비율
	unsigned char speed;
	unsigned char pltuspeed;
	unsigned char prtdspeed;
	
	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		speed = 0xff & ((7 * cmd->params[0]) / 100);
	}
	else
	{
		speed = 0xff & ((7 * sysdb_ch->pt_spd) / 100);
	}

	pltuspeed = plspeed[speed];
	prtdspeed = prspeed[speed];
	
	// Header
	buff[0] = 0xA5;
	// Address
	buff[1] = sysdb_ch->addr;
	// Command1
	// Command2 
	// Data speed 0x00 ~ 0x3F

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = (pltuspeed << 4) | 0x08;
			
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PAN_LEFT);
			}
			
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = (prtdspeed << 4) | 0x08;
			
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PAN_RIGHT);
			}
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = pltuspeed | 0x80;

			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_TILT_UP);
			}
			
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = prtdspeed | 0x80;

			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_TILT_DOWN);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = ((pltuspeed << 4) | 0x08) + (pltuspeed | 0x80);
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_LEFTUP);
			}
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = ((pltuspeed << 4) | 0x08) + (prtdspeed | 0x80);
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_LEFTDOWN);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = ((prtdspeed << 4) | 0x08) + (pltuspeed | 0x80);
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_RIGHTUP);
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[2] = 0x10;
			buff[3] = 0x0D;
			buff[4] = ((prtdspeed << 4) | 0x08) + (prtdspeed | 0x80);
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_PT_RIGHTDOWN);
			}
			break;
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[2] = 0x10;
			buff[3] = 0x0E;
			buff[4] = 0x88;

			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_ZOOM_TELE);
			}
			
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[2] = 0x10;
			buff[3] = 0x0C;
			buff[4] = 0x88;

			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_ZOOM_WIDE);
			}
			
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[2] = 0x10;
			buff[3] = 0x06;
			buff[4] = 0x88;
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_IRIS_OPEN);
			}
			
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[2] = 0x10;
			buff[3] = 0x05;
			buff[4] = 0x88;
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_IRIS_CLOSE);
			}
			
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[2] = 0x10;
			buff[3] = 0x04;
			buff[4] = 0x88;
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_FOCUS_FAR);
			}
			
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[2] = 0x10;
			buff[3] = 0x03;
			buff[4] = 0x88;
			if ( _fastrax_ptz_flag )
			{
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_FOCUS_NEAR);
			}
			
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[2] = 0x25;
			buff[3] = 0x80 | ((cmd->params[0] >> 4) & 0x0f);
			buff[4] = 0x80 | (cmd->params[0] & 0x0f);
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[2] = 0x11;
			buff[3] = 0x80 | ((cmd->params[0] >> 4) & 0x0f);
			buff[4] = 0x80 | (cmd->params[0] & 0x0f);
			break;	
		case NF_PTZ_CMD_STOP:
			nf_ptz_releaseCmd(cmd);
			_fastrax_ptz_flag = FALSE;
			buff[2] = 0x10;
			buff[3] = 0x0d;
			buff[4] = 0x88;
			break;
		default:
			break;
	}

	buff[5] = 0x55;
	buff[6] = _fastraxII_2_checksum( buff, 0, ret - 2);	

	return ret;	
}


