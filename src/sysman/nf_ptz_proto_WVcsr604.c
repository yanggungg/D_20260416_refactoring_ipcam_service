#include "nf_common.h"
#include "nf_ptz.h"
#include "unp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ptz"

#define WVcsr604_LEN		18



static int _WVcsr604_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch);


int _WVcsr604_pan_left(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}

int _WVcsr604_pan_right(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}

int _WVcsr604_tilt_up(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}

int _WVcsr604_tilt_down(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}

int _WVcsr604_pt_leftup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}
int _WVcsr604_pt_leftdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}
int _WVcsr604_pt_rightup(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}
int _WVcsr604_pt_rightdown(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}


int _WVcsr604_zoom_wide(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);

}

int _WVcsr604_zoom_tele(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);

}

int _WVcsr604_iris_open(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);

}

int _WVcsr604_iris_close(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);

}

int _WVcsr604_focus_near(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);

}

int _WVcsr604_focus_far(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}


int _WVcsr604_stop(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command(cmd_buff, cmd, sysdb_ch);
}


int _WVcsr604_set_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command( cmd_buff, cmd , sysdb_ch);
}

int _WVcsr604_clear_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return 0;
}

int _WVcsr604_goto_preset(struct _NF_PTZ_PROTOCOL_T *ptz,  NF_PTZ_SYSDB_CH *sysdb_ch, NF_PTZ_CMD *cmd, unsigned char *cmd_buff)
{
	g_message("%s", __FUNCTION__);	
	return _WVcsr604_command( cmd_buff, cmd , sysdb_ch);
}



NF_PTZ_PROTOCOL	_nf_ptz_proto_WVcsr604 = {
	.idx				     = 0, 
	.attr				     = NF_PTZ_ATTR_ALL,
	.proto_name			     = "WV-csr604", 
	.func_pan_left           = _WVcsr604_pan_left,
	.func_pan_right          = _WVcsr604_pan_right,
	.func_tilt_up            = _WVcsr604_tilt_up,
	.func_tilt_down          = _WVcsr604_tilt_down,
	.func_pt_leftup          = _WVcsr604_pt_leftup,
	.func_pt_leftdown        = _WVcsr604_pt_leftdown,
	.func_pt_rightup         = _WVcsr604_pt_rightup,
	.func_pt_rightdown       = _WVcsr604_pt_rightdown,
	.func_zoom_wide          = _WVcsr604_zoom_wide,
	.func_zoom_tele          = _WVcsr604_zoom_tele,
	.func_iris_open          = _WVcsr604_iris_open,
	.func_iris_close         = _WVcsr604_iris_close,
	.func_focus_near         = _WVcsr604_focus_near,
	.func_focus_far          = _WVcsr604_focus_far,
	.func_stop               = _WVcsr604_stop,
	.func_set_preset         = _WVcsr604_set_preset,
	.func_clear_preset       = _WVcsr604_clear_preset,
	.func_goto_preset        = _WVcsr604_goto_preset,
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


static void set_address(int cno, unsigned char *data)
{
	*data++ = (cno/10)+'0';
	*data = (cno%10)+'0';
}


static int _WVcsr604_command(unsigned char *buff, NF_PTZ_CMD *cmd, NF_PTZ_SYSDB_CH *sysdb_ch)
{
	int ret = WVcsr604_LEN;
	unsigned char ptSpeed = 0, zoomSpeed = 0;
	unsigned char array_ptSpeed[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	unsigned char array_zoomSpeed[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};



	g_return_val_if_fail ( cmd->cmd < NF_PTZ_CMD_NR,  FALSE);
	g_return_val_if_fail ( buff != NULL,  FALSE);
	g_return_val_if_fail ( cmd != NULL,  FALSE);
	g_return_val_if_fail ( sysdb_ch != NULL,  FALSE);

	memset(buff, 0x00, ret);


	if ( cmd->params[0] )
	{
		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT) ||
				(cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_LEFTUP) || (cmd->cmd == NF_PTZ_CMD_PT_LEFTDOWN) ||	
				(cmd->cmd == NF_PTZ_CMD_PT_RIGHTUP) || (cmd->cmd == NF_PTZ_CMD_PT_RIGHTDOWN))
		{
			// pan & tilt speed
			ptSpeed = array_ptSpeed[(cmd->params[0]/10) - 1];
		}
		else if((cmd->cmd == NF_PTZ_CMD_ZOOM_TELE) || (cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE)) 
		{
			// zoom speed
			zoomSpeed = array_zoomSpeed[(cmd->params[0]/10) - 1];
		}
	}
	else
	{
		//g_message("%s sysdb_ch->pt_spd : %d\n", __FUNCTION__,sysdb_ch->pt_spd);

		if((cmd->cmd == NF_PTZ_CMD_PAN_LEFT) || (cmd->cmd == NF_PTZ_CMD_PAN_RIGHT) || 
				(cmd->cmd == NF_PTZ_CMD_TILT_UP) || (cmd->cmd == NF_PTZ_CMD_TILT_DOWN) ||
				(cmd->cmd == NF_PTZ_CMD_PT_LEFTUP) || (cmd->cmd == NF_PTZ_CMD_PT_LEFTDOWN) ||	
				(cmd->cmd == NF_PTZ_CMD_PT_RIGHTUP) || (cmd->cmd == NF_PTZ_CMD_PT_RIGHTDOWN))
		{

			ptSpeed = array_ptSpeed[(sysdb_ch->pt_spd/10) - 1];
//			fprintf(stderr, "speed : %d\n", speed);
//			fprintf(stderr, "sysdb_ch->pt_spd : %d\n", sysdb_ch->pt_spd);
		}
		else if((cmd->cmd == NF_PTZ_CMD_ZOOM_TELE) || (cmd->cmd == NF_PTZ_CMD_ZOOM_WIDE)) {
			zoomSpeed = array_zoomSpeed[(sysdb_ch->zoom_spd/10) - 1];
		}
	}


	// Start Code
	buff[0] = 0x02;
	buff[1] = 0x41;
	buff[2] = 0x44;
	set_address(sysdb_ch->addr, &buff[3]);
	buff[4] = 0x00;
	buff[5] = 0x3b;
	buff[6] = 0x47;
	buff[7] = 0x43;
	buff[8] = 0x37;
	buff[9] = 0x3a;
	buff[10] = 0x39;
	buff[11] = 0x30;
	

	switch(cmd->cmd)
	{
		case NF_PTZ_CMD_PAN_LEFT:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x38;
			buff[15] = ptSpeed;
			buff[16] = 0x39;
			break;

		case NF_PTZ_CMD_PAN_RIGHT:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x43;
			buff[15] = ptSpeed;
			buff[16] = 0x39;
			break;

		case NF_PTZ_CMD_TILT_UP:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x41;
			buff[15] = 0x39;
			buff[16] = ptSpeed;
			break;

		case NF_PTZ_CMD_TILT_DOWN:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x45;
			buff[15] = 0x39;
			buff[16] = ptSpeed;
			break;

		case NF_PTZ_CMD_PT_LEFTUP:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x39;
			buff[15] = ptSpeed;
			buff[16] = ptSpeed;
			break;

		case NF_PTZ_CMD_PT_LEFTDOWN:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x46;
			buff[15] = ptSpeed;
			buff[16] = ptSpeed;
			break;

		case NF_PTZ_CMD_PT_RIGHTUP:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x42;
			buff[15] = ptSpeed;
			buff[16] = ptSpeed;
			break;

		case NF_PTZ_CMD_PT_RIGHTDOWN:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x44;
			buff[15] = ptSpeed;
			buff[16] = ptSpeed;
			break;

			
		case NF_PTZ_CMD_ZOOM_TELE:
			buff[5]  = zoomSpeed;
			buff[12] = 0x32;
			buff[13] = 0x35;
			buff[14] = 0x31;
			buff[15] = 0x30;
			buff[16] = 0x30;
			break;

		case NF_PTZ_CMD_ZOOM_WIDE:
			buff[5]  = zoomSpeed;
			buff[12] = 0x32;
			buff[13] = 0x31;
			buff[14] = 0x31;
			buff[15] = 0x30;
			buff[16] = 0x30;
			break;

		case NF_PTZ_CMD_IRIS_OPEN:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x00;
			buff[15] = 0x39;
			buff[16] = 0x39;
			break;

		case NF_PTZ_CMD_IRIS_CLOSE:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x00;
			buff[15] = 0x39;
			buff[16] = 0x39;
			break;

		case NF_PTZ_CMD_FOCUS_FAR:
			buff[12] = 0x33;
			buff[13] = 0x34;
			buff[14] = 0x30;
			buff[15] = 0x30;
			buff[16] = 0x34;
			break;

		case NF_PTZ_CMD_FOCUS_NEAR:
			buff[12] = 0x33;
			buff[13] = 0x34;
			buff[14] = 0x30;
			buff[15] = 0x30;
			buff[16] = 0x31;
			break;

		case NF_PTZ_CMD_STOP:
			buff[12] = 0x32;
			buff[13] = 0x38;
			buff[14] = 0x31;
			buff[15] = 0x39;
			buff[16] = 0x39;
			break;

		case NF_PTZ_CMD_SET_PRESET:
			buff[12] = 0x34;
			buff[13] = 0x34;
			buff[14] = 0x00;
			buff[15] = 0x64 + cmd->params[0];
			buff[16] = 0x64 + cmd->params[0];
			break;

		case NF_PTZ_CMD_GOTO_PRESET:
			buff[12] = 0x34;
			buff[13] = 0x35;
			buff[14] = 0x00;
			buff[15] = 0x64 + cmd->params[0];
			buff[16] = 0x64 + cmd->params[0];
			break;

		default:
			break;
	}

	buff[17] = 0x03;

	return ret;
}

