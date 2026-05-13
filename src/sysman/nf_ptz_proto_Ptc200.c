#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define PTC200_LEN      6



static int _Ptc200_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _Ptc200_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}

int _Ptc200_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}

int _Ptc200_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}

int _Ptc200_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}

int _Ptc200_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}
int _Ptc200_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}
int _Ptc200_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}
int _Ptc200_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}


int _Ptc200_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);

}

int _Ptc200_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);

}

int _Ptc200_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);

}

int _Ptc200_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);

}

int _Ptc200_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);

}

int _Ptc200_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}


int _Ptc200_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command(cmd_buff, cmd, sysdb_ch);
}


int _Ptc200_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command( cmd_buff, cmd , sysdb_ch);
}

int _Ptc200_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Ptc200_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Ptc200_command( cmd_buff, cmd , sysdb_ch);
}



NF_PTZ_PROTOCOL	_nf_ptz_proto_Ptc200 = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "PTC-200", 
	.func_pan_left           = _Ptc200_pan_left,
	.func_pan_right          = _Ptc200_pan_right,
	.func_tilt_up            = _Ptc200_tilt_up,
	.func_tilt_down          = _Ptc200_tilt_down,
	.func_pt_leftup          = _Ptc200_pt_leftup,
	.func_pt_leftdown        = _Ptc200_pt_leftdown,
	.func_pt_rightup         = _Ptc200_pt_rightup,
	.func_pt_rightdown       = _Ptc200_pt_rightdown,
	.func_zoom_wide          = _Ptc200_zoom_wide,
	.func_zoom_tele          = _Ptc200_zoom_tele,
	.func_iris_open          = _Ptc200_iris_open,
	.func_iris_close         = _Ptc200_iris_close,
	.func_focus_near         = _Ptc200_focus_near,
	.func_focus_far          = _Ptc200_focus_far,
	.func_stop               = _Ptc200_stop,
	.func_set_preset         = _Ptc200_set_preset,
	.func_clear_preset       = _Ptc200_clear_preset,
	.func_goto_preset        = _Ptc200_goto_preset,
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




static unsigned char _Ptc200_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum = 0;

	g_return_val_if_fail(pos1>=0, FALSE);
	g_return_val_if_fail(pos2>=pos1, FALSE);

	for(i=pos1;i<=pos2;i++) 
		sum ^= *(data+i);

	return (unsigned char)sum;
}

static void select_stop(int before_cmd, char *pbuff)
{
	if((before_cmd == NF_PTZ_CMD_FOCUS_FAR) || (before_cmd == NF_PTZ_CMD_FOCUS_NEAR))
	{
		pbuff[2] = 0x25;
		pbuff[3] = 0x04;
		pbuff[4] = 0x00;
	}
	else if((before_cmd == NF_PTZ_CMD_IRIS_OPEN) || (before_cmd == NF_PTZ_CMD_IRIS_CLOSE))
	{
		pbuff[2] = 0x23;
		pbuff[3] = 0x05;
		pbuff[4] = 0x00;
	}
	else if((before_cmd == NF_PTZ_CMD_PAN_LEFT) || (before_cmd == NF_PTZ_CMD_PAN_RIGHT))
	{
		pbuff[2] = 0x13;
		pbuff[3] = 0x00;
		pbuff[4] = 0x00;
	}
	else if((before_cmd == NF_PTZ_CMD_TILT_UP) || (before_cmd == NF_PTZ_CMD_TILT_DOWN))
	{
		pbuff[2] = 0x14;
		pbuff[3] = 0x00;
		pbuff[4] = 0x00;
	}
}


static int _Ptc200_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{
	static int before_cmd;

	int ret = PTC200_LEN;
	unsigned char ptSpeed = 0;
	unsigned char array_ptSpeed[10] = {0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x10};
	unsigned char array_zoomSpeed[10] = {0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02};
	unsigned char array_fresetSpeed[10] = {0x00, 0x00, 0x00,  0x00, 0x00, 0x01,  0x01, 0x01, 0x01, 0x01};


	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);

	if ( cmd->params[0] )
	{
		ptSpeed = array_ptSpeed[(cmd->params[0]/10) - 1];
	}
	else
	{
		//g_message("%s sysdb_ch->pt_spd : %d\n", __FUNCTION__,sysdb_ch->pt_spd);
		ptSpeed = array_ptSpeed[(sysdb_ch->pt_spd/10) - 1];
	}
	
	buff[0] = sysdb_ch->addr;
	buff[1] = 0x00;

	if(cmd->cmd != NF_PTZ_CMD_STOP)
		before_cmd = cmd->cmd;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[2] = 0x18;
			buff[3] = 0x01;
			buff[4] = ptSpeed;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[2] = 0x18;
			buff[3] = 0x00;
			buff[4] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[2] = 0x18;
			buff[3] = 0x02;
			buff[4] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[2] = 0x18;
			buff[3] = 0x03;
			buff[4] = ptSpeed;
			break;

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[2] = 0x24;
			buff[3] = 0x01;
			buff[4] = 0x00;
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[2] = 0x24;
			buff[3] = 0x00;
			buff[4] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
			buff[2] = 0x23;
			buff[3] = 0x02;
			buff[4] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[2] = 0x23;
			buff[3] = 0x03;
			buff[4] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[2] = 0x25;
			buff[3] = 0x06;
			buff[4] = 0x0f;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[2] = 0x25;
			buff[3] = 0x05;
			buff[4] = 0x0f;
			break;

		case NF_PTZ_CMD_STOP:
			select_stop(before_cmd, buff);
			break;

		case NF_PTZ_CMD_SET_PRESET:
			buff[2] = 0x1d;
			buff[3] = 0x00;
			buff[4] = cmd->params[0];
			break;

		case NF_PTZ_CMD_GOTO_PRESET:
			buff[2] = 0x11;
			buff[3] = 0x00;
			buff[4] = cmd->params[0];
			break;

		default:
			break;
	}


	buff[5] = _Ptc200_checksum(buff, 0, ret - 2);

	return ret;
}

