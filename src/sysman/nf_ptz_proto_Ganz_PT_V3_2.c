#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define GANZPT_LEN		6

static int 				_Ganz_PT_V3_2command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);
static unsigned char 	_Ganz_PT_V3_2checksum(unsigned char *data, int pos1, int pos2);

static gboolean _zoomstopflag = FALSE;
static gboolean _zoomspeedflag = FALSE;
static gboolean _iristopflag = FALSE;
static gboolean _focusstopflag = FALSE;
static gboolean _focusflag = 0;
static gboolean _panstopflag = FALSE;
static gboolean _tiltstopflag = FALSE;
static gboolean _ptstopflag = FALSE;


int _Ganz_PT_V3_2pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}
int _Ganz_PT_V3_2pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}
int _Ganz_PT_V3_2pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}
int _Ganz_PT_V3_2pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}
int _Ganz_PT_V3_2zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);

}

int _Ganz_PT_V3_2zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);

}

int _Ganz_PT_V3_2iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);

}

int _Ganz_PT_V3_2iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);

}

int _Ganz_PT_V3_2focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);

}

int _Ganz_PT_V3_2focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	if ( _zoomstopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED0;
		_zoomstopflag = FALSE;
		g_message("%s zoomstop", __FUNCTION__);
	}
	else if ( _iristopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED1;
		_iristopflag = FALSE;
		g_message("%s irisstop", __FUNCTION__);
	}
	else if ( _focusstopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED2;
		_focusstopflag = FALSE;
		g_message("%s focusstop", __FUNCTION__);
	}
	else if ( _panstopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED3;
		_panstopflag = FALSE;
		g_message("%s panstop", __FUNCTION__);
	}
	else if ( _tiltstopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED4;
		_tiltstopflag = FALSE;
		g_message("%s tiltstop", __FUNCTION__);
	}
	else if ( _ptstopflag )
	{
		cmd->cmd = NF_PTZ_CMD_RESERVED3;
		g_message("%s panstop", __FUNCTION__);
	}
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2pattern_start(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2pattern_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2pattern_set(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2run_pattern(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_zoom_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_focus_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_iris_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_pantilt_speed(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_auto_focus(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2set_auto_iris(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2_osd_up_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2_osd_down_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2_osd_left_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2_osd_right_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2_osd_enter_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2_osd_stop_key(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ganz_PT_V3_2command( cmd_buff, cmd , sysdb_ch);
}

int _Ganz_PT_V3_2reserved0(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved1(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved2(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved3(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved4(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved5(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved6(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ganz_PT_V3_2reserved7(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_Ganz_PT_V3_2 = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "Ganz_PT_V3_2", 
	.func_pan_left           = _Ganz_PT_V3_2pan_left,
	.func_pan_right          = _Ganz_PT_V3_2pan_right,
	.func_tilt_up            = _Ganz_PT_V3_2tilt_up,
	.func_tilt_down          = _Ganz_PT_V3_2tilt_down,
	.func_pt_leftup          = _Ganz_PT_V3_2pt_leftup,
	.func_pt_leftdown        = _Ganz_PT_V3_2pt_leftdown,
	.func_pt_rightup         = _Ganz_PT_V3_2pt_rightup,
	.func_pt_rightdown       = _Ganz_PT_V3_2pt_rightdown,
	.func_zoom_wide          = _Ganz_PT_V3_2zoom_wide,
	.func_zoom_tele          = _Ganz_PT_V3_2zoom_tele,
	.func_iris_open          = _Ganz_PT_V3_2iris_open,
	.func_iris_close         = _Ganz_PT_V3_2iris_close,
	.func_focus_near         = _Ganz_PT_V3_2focus_near,
	.func_focus_far          = _Ganz_PT_V3_2focus_far,
	.func_stop               = _Ganz_PT_V3_2stop,
	.func_set_preset         = _Ganz_PT_V3_2set_preset,
	.func_clear_preset       = _Ganz_PT_V3_2clear_preset,
	.func_goto_preset        = _Ganz_PT_V3_2goto_preset,
	.func_pattern_start      = _Ganz_PT_V3_2pattern_start,
	.func_pattern_stop       = _Ganz_PT_V3_2pattern_stop,
	.func_pattern_set        = _Ganz_PT_V3_2pattern_set,
	.func_run_pattern        = _Ganz_PT_V3_2run_pattern,
	.func_set_zoom_speed     = _Ganz_PT_V3_2set_zoom_speed,
	.func_set_focus_speed    = _Ganz_PT_V3_2set_focus_speed,
	.func_set_iris_speed     = _Ganz_PT_V3_2set_iris_speed,
	.func_set_pantilt_speed  = _Ganz_PT_V3_2set_pantilt_speed,
	.func_set_auto_focus     = _Ganz_PT_V3_2set_auto_focus,
	.func_set_auto_iris      = _Ganz_PT_V3_2set_auto_iris,
	.func_osd_up_key	     = _Ganz_PT_V3_2_osd_up_key,	  
	.func_osd_down_key       = _Ganz_PT_V3_2_osd_down_key,    
	.func_osd_left_key       = _Ganz_PT_V3_2_osd_left_key,    
	.func_osd_right_key      = _Ganz_PT_V3_2_osd_right_key,   
	.func_osd_enter_key      = _Ganz_PT_V3_2_osd_enter_key,   
	.func_osd_stop_key       = _Ganz_PT_V3_2_osd_stop_key,    
	.func_reserved0          = _Ganz_PT_V3_2reserved0,
	.func_reserved1          = _Ganz_PT_V3_2reserved1,
	.func_reserved2          = _Ganz_PT_V3_2reserved2,
	.func_reserved3          = _Ganz_PT_V3_2reserved3,
	.func_reserved4          = _Ganz_PT_V3_2reserved4,
	.func_reserved5          = _Ganz_PT_V3_2reserved5,
	.func_reserved6          = _Ganz_PT_V3_2reserved6,
	.func_reserved7	         = _Ganz_PT_V3_2reserved7
};


static unsigned char _Ganz_PT_V3_2checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum=0;

	g_return_val_if_fail ( pos1 >= 0,  FALSE);
	g_return_val_if_fail ( pos2 >= pos1,  FALSE);
	
	for(i=pos1; i<=pos2 ;++i){
		sum ^= *(data+i);
	}
	return sum;
}

static int _Ganz_PT_V3_2command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{
	static int dual_pt = 0;
	int ret = GANZPT_LEN;
		
	// Speed 100%단위 비율
	unsigned char ptSpeed;
	unsigned char focusSpeed;
	unsigned char zoomSpeed;
	
	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		ptSpeed = 0xff & ((15 * cmd->params[0]) / 100);
		focusSpeed = 0xff & ((3 * cmd->params[0]) / 100);
		zoomSpeed = 0xff & ((3 * cmd->params[0]) / 100);
	}
	else
	{
		ptSpeed = 0xff & ((15 * sysdb_ch->pt_spd) / 100);
		focusSpeed = 0xff & ((3 * sysdb_ch->focus_spd) / 100);
		zoomSpeed = 0xff & ((3 * sysdb_ch->zoom_spd) / 100);
	}
	
	
	// Address
	buff[0] = sysdb_ch->addr;

	// Transmitter Address
	buff[1] = 0x00;

	// Op_code
	// Data0
	// Data1
	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[2] = 0x18;
			buff[3] = 0x01;
			buff[4] = ptSpeed;

			_panstopflag = TRUE;
			break;
		case NF_PTZ_CMD_PAN_RIGHT:
			buff[2] = 0x18;
			buff[3] = 0x00;
			buff[4] = ptSpeed;

			_panstopflag = TRUE;
			break;
		case NF_PTZ_CMD_TILT_UP:
			buff[2] = 0x18;
			buff[3] = 0x02;
			buff[4] = ptSpeed;

			_tiltstopflag = TRUE;
			break;
		case NF_PTZ_CMD_TILT_DOWN:
			buff[2] = 0x18;
			buff[3] = 0x03;
			buff[4] = ptSpeed;

			_tiltstopflag = TRUE;
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			if(dual_pt == 0)
			{
				buff[2] = 0x18;
				buff[3] = 0x01;
				buff[4] = ptSpeed;

				dual_pt = 1;
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			else if(dual_pt == 1)
			{
				usleep(100000);
				buff[2] = 0x18;
				buff[3] = 0x02;
				buff[4] = ptSpeed;

				dual_pt = 0;
				_ptstopflag = TRUE;
			}
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			if(dual_pt == 0)
			{
				buff[2] = 0x18;
				buff[3] = 0x01;
				buff[4] = ptSpeed;

				dual_pt = 1;
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			else if(dual_pt == 1)
			{
				usleep(100000);
				buff[2] = 0x18;
				buff[3] = 0x03;
				buff[4] = ptSpeed;

				dual_pt = 0;
				_ptstopflag = TRUE;
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			if(dual_pt == 0)
			{
				buff[2] = 0x18;
				buff[3] = 0x00;
				buff[4] = ptSpeed;

				dual_pt = 1;
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			else if(dual_pt == 1)
			{
				usleep(100000);
				buff[2] = 0x18;
				buff[3] = 0x02;
				buff[4] = ptSpeed;

				dual_pt = 0;
				_ptstopflag = TRUE;
			}
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			if(dual_pt == 0)
			{
				buff[2] = 0x18;
				buff[3] = 0x00;
				buff[4] = ptSpeed;

				dual_pt = 1;
				nf_ptz_RepeatCmd(cmd, cmd->cmd);
			}
			else if(dual_pt == 1)
			{
				usleep(100000);
				buff[2] = 0x18;
				buff[3] = 0x03;
				buff[4] = ptSpeed;

				dual_pt = 0;
				_ptstopflag = TRUE;
			}
			break;
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[2] = 0x24;
			buff[3] = 0x01;
			buff[4] = 0x00;

			_zoomstopflag = TRUE;
			break;
		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[2] = 0x24;
			buff[3] = 0x00;
			buff[4] = 0x00;

			_zoomstopflag = TRUE;
			break;
		case NF_PTZ_CMD_IRIS_OPEN:
			buff[2] = 0x23;
			buff[3] = 0x02;
			buff[4] = 0x00;

			_iristopflag = TRUE;
			break;
		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[2] = 0x23;
			buff[3] = 0x03;
			buff[4] = 0x00;

			_iristopflag = TRUE;
			break;
		case NF_PTZ_CMD_FOCUS_FAR:
			buff[2] = 0x25;
			buff[3] = 0x01;
			buff[4] = 0x01;

			_focusstopflag = TRUE;
			break;
		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[2] = 0x25;
			buff[3] = 0x00;
			buff[4] = 0x01;

			_focusstopflag = TRUE;
			break;		
		case NF_PTZ_CMD_SET_PRESET:
			buff[2] = 0x1D;
			buff[3] = 0x00;
			buff[4] = cmd->params[0];
			break;
		case NF_PTZ_CMD_GOTO_PRESET:
			buff[2] = 0x11;
			buff[3] = 0x00;
			buff[4] = cmd->params[0];
			break;		
		case NF_PTZ_CMD_RESERVED0:	// zoom stop
			buff[2] = 0x24;
			buff[3] = 0x04;
			buff[4] = 0x00;
			break;
		case NF_PTZ_CMD_RESERVED1:	// iris stop
			buff[2] = 0x23;
			buff[3] = 0x05;
			buff[4] = 0x00;
			break;
		case NF_PTZ_CMD_RESERVED2:	// focus stop
			buff[2] = 0x25;
			buff[3] = 0x04;
			buff[4] = 0x00;
			break;
		case NF_PTZ_CMD_RESERVED3:	// pan stop
			buff[2] = 0x13;
			buff[3] = 0x00;
			buff[4] = 0x00;

			if(_ptstopflag == TRUE) 
			{
				_ptstopflag = FALSE;
				_tiltstopflag =TRUE;
				nf_ptz_RepeatCmd(cmd, NF_PTZ_CMD_STOP);
			}
			break;
		case NF_PTZ_CMD_RESERVED4:	// tilt stop
			usleep(100000);
			buff[2] = 0x14;
			buff[3] = 0x00;
			buff[4] = 0x00;
			break;
		case NF_PTZ_CMD_OSD_UP_KEY:	
			buff[2] = 0x28;
			buff[3] = 0x00;
			buff[4] = 0x00;			
		  	break;
		case NF_PTZ_CMD_OSD_DOWN_KEY:	
			buff[2] = 0x28;
			buff[3] = 0x01;
			buff[4] = 0x00;		
	 		 break;
		case NF_PTZ_CMD_OSD_LEFT_KEY:
			buff[2] = 0x28;
			buff[3] = 0x02;
			buff[4] = 0x00;		
	  		break;
		case NF_PTZ_CMD_OSD_RIGHT_KEY:	
			buff[2] = 0x28;
			buff[3] = 0x03;
			buff[4] = 0x00;	
	 		 break;
		case NF_PTZ_CMD_OSD_ENTER_KEY:	
			buff[2] = 0x28;
			buff[3] = 0x04;
			buff[4] = 0x00;		
	 		 break;
		case NF_PTZ_CMD_OSD_STOP_KEY:	
			buff[2] = 0x28;
			buff[3] = 0xff;
			buff[4] = 0x00;			
	 		 break;	
		default:
			break;
	}

	buff[5] = _Ganz_PT_V3_2checksum( buff, 0, ret - 2);	

	return ret;	
}

