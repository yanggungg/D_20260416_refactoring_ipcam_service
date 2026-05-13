#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define Mrx1000_LEN		9



static int _Mrx1000_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _Mrx1000_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}
int _Mrx1000_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}
int _Mrx1000_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}

int _Mrx1000_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);

}

int _Mrx1000_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);

}

int _Mrx1000_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);

}

int _Mrx1000_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);

}

int _Mrx1000_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);

}

int _Mrx1000_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}


int _Mrx1000_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command(cmd_buff, cmd, sysdb_ch);
}


int _Mrx1000_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command( cmd_buff, cmd , sysdb_ch);
}

int _Mrx1000_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _Mrx1000_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _Mrx1000_command( cmd_buff, cmd , sysdb_ch);
}



NF_PTZ_PROTOCOL	_nf_ptz_proto_Mrx1000 = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "MRX_1000", 
	.func_pan_left           = _Mrx1000_pan_left,
	.func_pan_right          = _Mrx1000_pan_right,
	.func_tilt_up            = _Mrx1000_tilt_up,
	.func_tilt_down          = _Mrx1000_tilt_down,
	.func_pt_leftup          = _Mrx1000_pt_leftup,
	.func_pt_leftdown        = _Mrx1000_pt_leftdown,
	.func_pt_rightup         = _Mrx1000_pt_rightup,
	.func_pt_rightdown       = _Mrx1000_pt_rightdown,
	.func_zoom_wide          = _Mrx1000_zoom_wide,
	.func_zoom_tele          = _Mrx1000_zoom_tele,
	.func_iris_open          = _Mrx1000_iris_open,
	.func_iris_close         = _Mrx1000_iris_close,
	.func_focus_near         = _Mrx1000_focus_near,
	.func_focus_far          = _Mrx1000_focus_far,
	.func_stop               = _Mrx1000_stop,
	.func_set_preset         = _Mrx1000_set_preset,
	.func_clear_preset       = _Mrx1000_clear_preset,
	.func_goto_preset        = _Mrx1000_goto_preset,
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




static unsigned char _Mrx1000_checksum(unsigned char *data, int pos1, int pos2)
{
	int i;
	int sum = 0;

	g_return_val_if_fail(pos1>=0, FALSE);
	g_return_val_if_fail(pos2>=pos1, FALSE);

	for(i=pos1;i<=pos2;i++) 
		sum += data[i];

	return (unsigned char)(sum ^ 0xff);
}



static int _Mrx1000_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{

	int ret = Mrx1000_LEN;
	unsigned char ptSpeed = 0;
	unsigned char array_ptSpeed[10] = {0x0f, 0x19, 0x32, 0x4b, 0x64, 0x7d, 0x96, 0xaf, 0xc8, 0xff};


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
	
	// Start Code
	buff[0] = 0xa0;
	// Sender Addr
	buff[1] = sysdb_ch->addr;
	// Target Addr
	buff[2] = sysdb_ch->addr;

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[3] = 0x01;
			buff[4] = 0x02; // auto focus
			buff[5] = 0x01;
			buff[6] = ptSpeed;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x02;
			buff[6] = ptSpeed;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x04;
			buff[6] = 0x00;
			buff[7] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x08;
			buff[6] = 0x00;
			buff[7] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_LEFTUP:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x05;
			buff[6] = ptSpeed;
			buff[7] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x09;
			buff[6] = ptSpeed;
			buff[7] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x06;
			buff[6] = ptSpeed;
			buff[7] = ptSpeed;
			break;
		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x0a;
			buff[6] = ptSpeed;
			buff[7] = ptSpeed;
			break;

		case NF_PTZ_CMD_ZOOM_TELE:
			buff[3] = 0x01;
			buff[4] = 0x20;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[3] = 0x01;
			buff[4] = 0x40;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
			buff[3] = 0x01;
			buff[4] = 0x08;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[3] = 0x01;
			buff[4] = 0x10;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[3] = 0x01;
			buff[4] = 0x01;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[3] = 0x01;
			buff[4] = 0x02;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_STOP:
			buff[3] = 0x01;
			buff[4] = 0x00;
			buff[5] = 0x00;
			buff[6] = 0x00;
			buff[7] = 0x00;
			break;

		case NF_PTZ_CMD_SET_PRESET:
			buff[3] = 0x03;
			buff[4] = 0x50;
			buff[5] = cmd->params[0];
			buff[6] = 0xff;
			buff[7] = 0xff;
			break;

		case NF_PTZ_CMD_GOTO_PRESET:
			buff[3] = 0x03;
			buff[4] = 0x19;
			buff[5] = cmd->params[0];
			buff[6] = 0xff;
			buff[7] = 0xff;
			break;

		default:
			break;
	}


	buff[8] = _Mrx1000_checksum(buff, 1, ret - 2);

	return ret;
}

