#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define TAKEX_LEN      12


static int _Takex_ptz_flag = TRUE;

static int _Takex_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _Takex_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}

int _Takex_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}

int _Takex_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}

int _Takex_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
} 

int _Takex_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
} 
int _Takex_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
} 
int _Takex_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
} 
int _Takex_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
} 
int _Takex_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff) 
{ 
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}

int _Takex_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}

int _Takex_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);

}

int _Takex_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);

}

int _Takex_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);

}

int _Takex_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}


int _Takex_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command(cmd_buff, cmd, sysdb_ch);
}


int _Takex_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Takex_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_run_sequence(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_auto_pan(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_menu(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_enter(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

int _Takex_osd_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Takex_command( cmd_buff, cmd , sysdb_ch);
}

NF_PTZ_PROTOCOL	_nf_ptz_proto_Takex = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "TAKEX",
	.func_pan_left           = _Takex_pan_left,
	.func_pan_right          = _Takex_pan_right,
	.func_tilt_up            = _Takex_tilt_up,
	.func_tilt_down          = _Takex_tilt_down,
	.func_pt_leftup          = _Takex_pt_leftup,
	.func_pt_leftdown        = _Takex_pt_leftdown,
	.func_pt_rightup         = _Takex_pt_rightup,
	.func_pt_rightdown       = _Takex_pt_rightdown,
	.func_zoom_wide          = _Takex_zoom_wide,
	.func_zoom_tele          = _Takex_zoom_tele,
	.func_iris_open          = _Takex_iris_open,
	.func_iris_close         = _Takex_iris_close,
	.func_focus_near         = _Takex_focus_near,
	.func_focus_far          = _Takex_focus_far,
	.func_stop               = _Takex_stop,
	.func_set_preset         = _Takex_set_preset,
	.func_clear_preset       = _Takex_clear_preset,
	.func_goto_preset        = _Takex_goto_preset,
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
	.func_osd_up_key	     = _Takex_osd_up,
	.func_osd_down_key     	 = _Takex_osd_down,
	.func_osd_left_key       = _Takex_osd_left,
	.func_osd_right_key      = _Takex_osd_right,
	.func_osd_enter_key      = _Takex_osd_enter,
	.func_osd_stop_key       = _Takex_osd_stop,
	// osd_menu
	.func_reserved0          = _Takex_osd_menu,
	// auto_pan
	.func_reserved1          = _Takex_auto_pan,
	// sequence start
	.func_reserved2          = _Takex_run_sequence,
	.func_reserved3          = NULL,
	.func_reserved4          = NULL,
	.func_reserved5          = NULL,
	.func_reserved6          = NULL,
	.func_reserved7	         = NULL,
};



/*  패킷 정의
 *  my define 1byte : 0xd0 
 *            2byte : 0xe0
 *            3byte : 0xf0
 *            4byte : microsecond sleep
 *            5byte : repeat_check
 *            other bytes TAKENAKA PACKET (7 byte)
 *
*/
static int _Takex_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{
	int ret = TAKEX_LEN;
	unsigned char ptSpeed = 0;
	unsigned char array_ptSpeed[10] = {0x01, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07};


	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT))
			ptSpeed = array_ptSpeed[(cmd->params[0]/10) - 1];
		else if((cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN)) 
		{
			ptSpeed = array_ptSpeed[(cmd->params[0]/10) - 1];
			ptSpeed<<=3;
		}
	}
	else
	{
		//g_message("%s sysdb_ch->pt_spd : %d\n", __FUNCTION__,sysdb_ch->pt_spd);
		ptSpeed = array_ptSpeed[(sysdb_ch->pt_spd/10) - 1];
	}
	buff[0] = 0xd0;
	buff[1] = 0xe0;
	buff[2] = 0xf0;
	buff[3] = 10;
	buff[4] = TRUE;
	
	buff[5] = 0xf1;
	buff[6] = 0x00;
	buff[7] = 0xd4;
	buff[8] = sysdb_ch->addr;


	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[9] = 0x25;
			buff[10] = 0x02;
			buff[11] = ptSpeed;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[9] = 0x25;
			buff[10] = 0x01;
			buff[11] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[9] = 0x25;
			buff[10] = 0x04;
			buff[11] = ptSpeed << 3;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[9] = 0x25;
			buff[10] = 0x08;
			buff[11] = ptSpeed << 3;
			break;
		
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[9] = 0x25;
			buff[10] = 0x06;
			buff[11] = ptSpeed + (ptSpeed << 3);
			break;

		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[9] = 0x25;
			buff[10] = 0x0a;
			buff[11] = ptSpeed + (ptSpeed << 3);
			break;

		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[9] = 0x25;
			buff[10] = 0x05;
			buff[11] = ptSpeed + (ptSpeed << 3);
			break;

		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[9] = 0x25;
			buff[10] = 0x09;
			buff[11] = ptSpeed + (ptSpeed << 3);
			break;
		

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[9] = 0x25;
			buff[10] = 0x00;
			buff[11] = 0x80;
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[9] = 0x25;
			buff[10] = 0x00;
			buff[11] = 0x40;
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
			buff[9] = 0x14;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[9] = 0x15;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[9] = 0x24;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[9] = 0x23;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_STOP:
			buff[9] = 0x1e;
			buff[10] = 0x00;
			buff[11] = 0x00;
			nf_ptz_releaseCmd(cmd);
			break;

		case NF_PTZ_CMD_SET_PRESET:
			buff[9] = 0x26;
			buff[10] = cmd->params[0] + 1;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_GOTO_PRESET:
			buff[9] = 0xb8;
			buff[10] = cmd->params[0] + 1;
			buff[11] = 0x00;
			break;


		case NF_PTZ_CMD_OSD_UP_KEY:
			buff[9] = 0x54;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_OSD_DOWN_KEY:
			buff[9] = 0x64;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_OSD_LEFT_KEY:
			buff[9] = 0x53;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_OSD_RIGHT_KEY:
			buff[9] = 0x63;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_OSD_ENTER_KEY:
			buff[9] = 0x40;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		case NF_PTZ_CMD_OSD_STOP_KEY:
			buff[9] = 0x8e;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		/* OSD_MENU OPEN */
		case NF_PTZ_CMD_RESERVED0:
			buff[9] = 0x74;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		/* AUTO PAN */
		case NF_PTZ_CMD_RESERVED1:
			buff[9] = 0x28;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		/* SEQUENSE START */
		case NF_PTZ_CMD_RESERVED2:
			buff[9] = 0x27;
			buff[10] = 0x00;
			buff[11] = 0x00;
			break;

		default:
			break;
	}

	switch(cmd->cmd) 
	{
		case NF_PTZ_CMD_PAN_LEFT:
		case NF_PTZ_CMD_PAN_RIGHT:
		case NF_PTZ_CMD_TILT_UP:
		case NF_PTZ_CMD_TILT_DOWN:
		case NF_PTZ_CMD_PT_LEFTUP:
		case NF_PTZ_CMD_PT_LEFTDOWN:
		case NF_PTZ_CMD_PT_RIGHTUP:
		case NF_PTZ_CMD_PT_RIGHTDOWN:
		case NF_PTZ_CMD_ZOOM_TELE:
		case NF_PTZ_CMD_ZOOM_WIDE:
			if(_Takex_ptz_flag == TRUE)  {
				buff[4] = TRUE;
				_Takex_ptz_flag = FALSE;
			}
			else if(_Takex_ptz_flag == FALSE) {
				buff[4] = FALSE;
				_Takex_ptz_flag = TRUE;
			}
			nf_ptz_RepeatCmd(cmd, cmd->cmd);
			break;

		default:
			_Takex_ptz_flag = TRUE;
			buff[4] = TRUE;
			break;
	}

	return ret;
}



void takex_write(int fd, const unsigned char *data, unsigned int ptz_cmd_length)
{
	int ret = 0;
	int i;
	unsigned long micro_sec = data[3] * 1000;


	if(data[4] == FALSE)
		return;

	// 5 num is takex_packet start byte
	for(i=5;i<=ptz_cmd_length;i++)
	{
		ret = write(fd, &data[i], sizeof(unsigned char));

		if (ret == 0) {
			g_warning("%s Write no data! Write size[%d] ret[%d]", __FUNCTION__, ptz_cmd_length, ret);
		}
		else if (ret == -1) {
			g_warning("%s Write error", __FUNCTION__);
		}

		usleep(micro_sec);
	}
}


