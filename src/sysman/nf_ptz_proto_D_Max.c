#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define D_MAX_LEN		11

static int 				_D_Max_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_D_Max_checksum(unsigned char *data, int pos1, int pos2);

int _D_Max_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}
int _D_Max_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}
int _D_Max_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}
int _D_Max_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}
int _D_Max_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);

}

int _D_Max_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _D_Max_command( cmd_buff, cmd , sysdb_ch);
}

int _D_Max_reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _D_Max_reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_D_Max = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "D-Max", 
	.func_pan_left           = _D_Max_pan_left,
	.func_pan_right          = _D_Max_pan_right,
	.func_tilt_up            = _D_Max_tilt_up,
	.func_tilt_down          = _D_Max_tilt_down,
	.func_pt_leftup          = _D_Max_pt_leftup,
	.func_pt_leftdown        = _D_Max_pt_leftdown,
	.func_pt_rightup         = _D_Max_pt_rightup,
	.func_pt_rightdown       = _D_Max_pt_rightdown,
	.func_zoom_wide          = _D_Max_zoom_wide,
	.func_zoom_tele          = _D_Max_zoom_tele,
	.func_iris_open          = _D_Max_iris_open,
	.func_iris_close         = _D_Max_iris_close,
	.func_focus_near         = _D_Max_focus_near,
	.func_focus_far          = _D_Max_focus_far,
	.func_stop               = _D_Max_stop,
	.func_set_preset         = _D_Max_set_preset,
	.func_clear_preset       = _D_Max_clear_preset,
	.func_goto_preset        = _D_Max_goto_preset,
	.func_pattern_start      = _D_Max_pattern_start,
	.func_pattern_stop       = _D_Max_pattern_stop,
	.func_pattern_set        = _D_Max_pattern_set,
	.func_run_pattern        = _D_Max_run_pattern,
	.func_set_zoom_speed     = _D_Max_set_zoom_speed,
	.func_set_focus_speed    = _D_Max_set_focus_speed,
	.func_set_iris_speed     = _D_Max_set_iris_speed,
	.func_set_pantilt_speed  = _D_Max_set_pantilt_speed,
	.func_set_auto_focus     = _D_Max_set_auto_focus,
	.func_set_auto_iris      = _D_Max_set_auto_iris,
	.func_osd_up_key	     = _D_Max_osd_up_key,	  
	.func_osd_down_key      = _D_Max_osd_down_key,    
	.func_osd_left_key      = _D_Max_osd_left_key,    
	.func_osd_right_key     = _D_Max_osd_right_key,   
	.func_osd_enter_key     = _D_Max_osd_enter_key,   
	.func_osd_stop_key      = _D_Max_osd_stop_key, 
	.func_reserved0          = _D_Max_reserved0,
	.func_reserved1          = _D_Max_reserved1,
	.func_reserved2          = _D_Max_reserved2,
	.func_reserved3          = _D_Max_reserved3,
	.func_reserved4          = _D_Max_reserved4,
	.func_reserved5          = _D_Max_reserved5,
	.func_reserved6          = _D_Max_reserved6,
	.func_reserved7	         = _D_Max_reserved7
};


static unsigned char _D_Max_checksum(unsigned char *data, int pos1, int pos2)
{
	int i,cal,sum;

	g_return_val_if_fail ( pos1 >= 0,  FALSE);
	g_return_val_if_fail ( pos2 >= pos1,  FALSE);

	sum = 0;

	for( i = pos1; i <= pos2; ++i )
	{
		sum += *( data + i );
	}
	
	cal = ( 0x2020 - sum ) & 0xff;

	return (unsigned char)cal;
	
}

static int _D_Max_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = D_MAX_LEN;

	// Speed 100%단위 비율
	unsigned char ptSpeed;
	unsigned char zfSpeed;
	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);


	if ( cmd->params[0] )
	{
		// Pan/Tilt Speed : 0x81~0xBF (129~191)
		ptSpeed = 0xff & (((63 * cmd->params[0]) / 100) + 0x81);
		// Zoom / focus : 0xD5~0xDD (213~221)
		zfSpeed = 0xff & ((8 * cmd->params[0]) / 100);
	}
	else
	{
		// Pan/Tilt Speed : 0x81~0xBF (129~191)
		ptSpeed = 0xff & (((63 * sysdb_ch->pt_spd) / 100) + 0x81);
	
		// Zoom / focus : 0xD5~0xDD (213~221)
		zfSpeed = 0xff & ((8 * sysdb_ch->zoom_spd) / 100);
	}
	
	// Header
	buff[0] = 0x55;

	// Controller Address (0x00)
	buff[1] = 0x00;
	
	// Tartget Address
	buff[2] = sysdb_ch->addr;
	
	// Command1
	// Command2 

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[3] = 0x00;
			buff[4] = 0x04;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[3] = 0x00;
			buff[4] = 0x02;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[3] = 0x00;
			buff[4] = 0x08;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[3] = 0x00;
			buff[4] = 0x10;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[3] = 0x00;
			buff[4] = 0x0c;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x14;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[3] = 0x00;
			buff[4] = 0x0a;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[3] = 0x00;
			buff[4] = 0x12;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[3] = 0x00;
			buff[4] = 0x20;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[3] = 0x00;
			buff[4] = 0x40;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[3] = 0x00;
			buff[4] = 0x57;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x81;
			buff[8] = 0x82;
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[3] = 0x00;
			buff[4] = 0x57;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x81;
			buff[8] = 0x83;
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[3] = 0x01;
			buff[4] = 0x00;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[3] = 0x02;
			buff[4] = 0x00;
			buff[5] = ptSpeed;
			buff[6] = ptSpeed;
			buff[7] = zfSpeed;
			buff[8] = 0x00;
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[3] = 0x00;
			buff[4] = 0x03;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = cmd->params[0];
			buff[8] = 0x00;
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[3] = 0x00;
			buff[4] = 0x07;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = cmd->params[0];
			buff[8] = 0x00;
			break;	
		default:
			break;
	}

	buff[9] = 0xAA;
	buff[10] = _D_Max_checksum( buff, 0, ret - 2);	

	return ret;	
}
