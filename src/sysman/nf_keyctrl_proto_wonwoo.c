#include "nf_keyctrl.h"
#include <sys/ioctl.h>


#define CMD_WONWOO_LENGTH 5

static guint nf_cmd_WonWoo(unsigned char *cmd, NfKeyctrl *self);
guint _WonWoo_keyctrl_read_command(const NfKeyctrl *self);
static guint _cmd_WonWoo(unsigned char data2, int set_status);

static unsigned char data[CMD_WONWOO_LENGTH] = {0,};
static unsigned char uData[20]={0,};
static volatile int iKeyCnt = 0;


int _WonWoo_keyctrl_receive(const int fd)
{
	int ret;
	unsigned char rxchar = 0;
	int cmd_length = CMD_WONWOO_LENGTH;
	static int read_count = 0;

	ret = read(fd, &rxchar, 1);
	if(ret < 0) {
		read_count = 0;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	data[read_count++] = rxchar;

	if((read_count == 1) && (data[0] != 0xa1))
	{
		read_count = 0;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read command packet error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	if(read_count != cmd_length)
	{
		return KEYCTRL_PACKET_PILE;
	}

	read_count = 0;
	return KEYCTRL_PACKET_EMPTY;
}


guint _WonWoo_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey;

	vkey = nf_cmd_WonWoo(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}

#ifdef DEBUG_KEYCTRL_WONWOO
	HexDump(data, CMD_WONWOO_LENGTH, 0);
	g_message("%s NF_KEYCONT_BUTTON_NAME [%s]",__FUNCTION__, _NF_KEYCTRL_BUTTON_STR[vkey - 1]);
#endif

	return vkey;
}

static guint _cmd_WonWoo(unsigned char data2, int set_status)
{
	guint vkey = 0;
	unsigned char data = 0x00;
	static int delay_move = 0;

	if(iKeyCnt==2)
		data = ( uData[0] << 4 ) + uData[1];
	else 
		data = uData[0];

#ifdef DEBUG_KEYCTRL_WONWOO
	g_message("%s data[%x] data2[%x] iKeyCnt[%d]", __FUNCTION__, data, data2, iKeyCnt);
#endif
	
	if ( ( set_status == NF_DVR_STATUS_LIVE ) || ( set_status == NF_DVR_STATUS_INIT ) || ( set_status == NF_DVR_STATUS_ZOOM ) || ( set_status == NF_DVR_STATUS_RUN_PLAYBACK ) || ( set_status == NF_DVR_STATUS_PTZ ))
	{

		if(iKeyCnt==1 || iKeyCnt==2)
		{	
			switch ( data )
			{
				case 0x01: case 0xA1:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM1; break;
				case 0x02: case 0xA2:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM2; break;
				case 0x03: case 0xA3:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM3; break;
				case 0x04: case 0xA4:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM4; break;
				case 0x05: case 0xA5:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM5; break;
				case 0x06: case 0xA6:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM6; break;
				case 0x07: case 0xA7:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM7; break;
				case 0x08: case 0xA8:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM8; break;
				case 0x09: case 0xA9:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM9; break;
				case 0x1A:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM10; break;
				case 0x11:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM11; break;
				case 0x12:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM12; break;
				case 0x13:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM13; break;
				case 0x14:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM14; break;
				case 0x15:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM15; break;
				case 0x16:		vkey = NF_KEYCTRL_BUTTON_MAP_NUM16; break;
				default:
					iKeyCnt = 0;	
					return 0;
			}
			iKeyCnt = 0;
			return vkey;
		}
		
	}

	if ( set_status == NF_DVR_STATUS_ZOOM )
	{
		switch (data)
		{
			case 0x20: vkey = NF_KEYCTRL_BUTTON_MAP_RETURN; break;  //return
			case 0x25:  vkey = NF_KEYCTRL_BUTTON_MAP_RF; break;
			case 0x26:  vkey = NF_KEYCTRL_BUTTON_MAP_FF; break;
			case 0x27: vkey = NF_KEYCTRL_BUTTON_MAP_RF; break;
			case 0x28: vkey = NF_KEYCTRL_BUTTON_MAP_FF; break;

			case 0x80:
				switch(data2)
				{
					case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07: 	 vkey = NF_KEYCTRL_BUTTON_MAP_UP;     break; // up
					case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:case 0x0E:case 0x0F: 	 vkey = NF_KEYCTRL_BUTTON_MAP_DOWN;   break; // down
					case 0x10:case 0x20:case 0x30:case 0x40:case 0x50:case 0x60:case 0x70: 	 vkey = NF_KEYCTRL_BUTTON_MAP_LEFT;   break; // left
					case 0x90:case 0xA0:case 0xB0:case 0xC0:case 0xD0:case 0xE0:case 0xF0: 	 vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT;  break; // right
					default:
						return 0;
				}
				
				break;
			default:
				return 0;
		}
		
		return vkey;
	}
	else if ( set_status == NF_DVR_STATUS_PTZ )
	{
		switch (data)
		{
			case 0x14: vkey = NF_KEYCTRL_BUTTON_MAP_PTZ; break;
			case 0x15: vkey = NF_KEYCTRL_BUTTON_MAP_SETUP; break;   
			case 0x20: vkey = NF_KEYCTRL_BUTTON_MAP_RETURN; break;  //return
			case 0x21:	vkey = NF_KEYCTRL_BUTTON_MAP_ENTER; break;
			case 0x27: vkey = NF_KEYCTRL_BUTTON_MAP_RF; break;
			case 0x28: vkey = NF_KEYCTRL_BUTTON_MAP_FF; break;
			case 0x81: vkey = NF_KEYCTRL_BUTTON_MAP_TURN_RIGHT; break;
			case 0x89: vkey = NF_KEYCTRL_BUTTON_MAP_TURN_LEFT; break;

			case 0x80:
				switch(data2)
				{
					case 0x01:case 0x02:case 0x03:case 0x04:case 0x05:case 0x06:case 0x07: 	 vkey = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_UP;     break; // up
					case 0x09:case 0x0A:case 0x0B:case 0x0C:case 0x0D:case 0x0E:case 0x0F: 	 vkey = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_DOWN;   break; // down
					case 0x10:case 0x20:case 0x30:case 0x40:case 0x50:case 0x60:case 0x70: 	 vkey = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT;   break; // left
					case 0x90:case 0xA0:case 0xB0:case 0xC0:case 0xD0:case 0xE0:case 0xF0: 	 vkey = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT;  break; // right
					case 0x0: 	 vkey = NF_KEYCTRL_BUTTON_MAP_STOP;  break; // stop
					default:
						return 0;
				}
				
				break;
			default:
				return 0;
		}
		
		return vkey;
	}
	else if ( set_status == NF_DVR_STATUS_RUN_PLAYBACK )
	{
		switch(data)
		{	
			case 0x11: vkey = NF_KEYCTRL_BUTTON_MAP_LEFT; break;
			case 0x12: vkey = NF_KEYCTRL_BUTTON_MAP_PAUSE; break;
			case 0x13: vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT; break;
			case 0x14: vkey = NF_KEYCTRL_BUTTON_MAP_DISPLAY; break;
			case 0x15: vkey = NF_KEYCTRL_BUTTON_MAP_ARCHIVE; break; // MENU : ¢º¢º
			case 0x20:	vkey = NF_KEYCTRL_BUTTON_MAP_RETURN; break; //return
			case 0x21:	vkey = NF_KEYCTRL_BUTTON_MAP_ENTER; break; //return
			case 0x25:  vkey = NF_KEYCTRL_BUTTON_MAP_RF; break;
			case 0x26:  vkey = NF_KEYCTRL_BUTTON_MAP_FF; break;

			case 0x80:
			{
				switch ( data2 )
				{
					case 0x01: case 0x02: case 0x03: case 0x04:
					case 0x05: case 0x06: case 0x07:
						vkey = NF_KEYCTRL_BUTTON_MAP_UP; break; // up:¢¸¢¸
					case 0x09: case 0x0a: case 0x0b: case 0x0c:
					case 0x0d: case 0x0e:
						vkey = NF_KEYCTRL_BUTTON_MAP_DOWN; break; // down:¢º¢º
					case 0x10: case 0x20: case 0x30: case 0x40:
					case 0x50: case 0x60: case 0x70:
						vkey = NF_KEYCTRL_BUTTON_MAP_LEFT; break; // left: ¢º
					case 0x90: case 0xa0: case 0xb0: case 0xc0:
					case 0xd0: case 0xe0: case 0xf0:
						vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT; break; // right: ¢¸

					default: 
						return 0;
				}
			}
			break;

			default:
					return 0;
		}

		return vkey;
	}
	else 
	{	
		switch (data)
		{
			case 0x11:	vkey = NF_KEYCTRL_BUTTON_MAP_DISPLAY; break; // scr mode
			case 0x12:      vkey = NF_KEYCTRL_BUTTON_MAP_LOCK; break; // ptz
			case 0x13:      vkey = NF_KEYCTRL_BUTTON_MAP_ZOOM; break; // zoom
			case 0x14:      vkey = NF_KEYCTRL_BUTTON_MAP_SEARCH; break; // search
			case 0x15: 	vkey = NF_KEYCTRL_BUTTON_MAP_SETUP; break;   
			case 0x20:	vkey = NF_KEYCTRL_BUTTON_MAP_RETURN; break; //return
			case 0x21:	vkey = NF_KEYCTRL_BUTTON_MAP_ENTER; break; //return

			case 0x80:
				if(delay_move > 0) {
					delay_move = 0;
					return 0;
				}
				switch(data2)
				{
					case 0x01: case 0x02: case 0x03: case 0x04:
					case 0x05: case 0x06: case 0x07:
						vkey = NF_KEYCTRL_BUTTON_MAP_UP; 
						break; // up

					case 0x09: case 0x0a: case 0x0b: case 0x0c:
					case 0x0d: case 0x0e:
						vkey = NF_KEYCTRL_BUTTON_MAP_DOWN; 
						break; // down

					case 0x10: case 0x20: case 0x30: case 0x40:
					case 0x50: case 0x60: case 0x70:
						vkey = NF_KEYCTRL_BUTTON_MAP_LEFT; 
						break; // left

					case 0x90: case 0xa0: case 0xb0: case 0xc0:
					case 0xd0: case 0xe0: case 0xf0:
						vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT;  
						break; // right
					
					default:
						return 0;
				}
				delay_move++;
				break;
			default:
				return 0;
		}
		return vkey;
	}

	return vkey;
}

static guint nf_cmd_WonWoo(unsigned char *cmd, NfKeyctrl *self)
{
	unsigned char addr, oc1, oc2;
	guint vkey = 0;

	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;

	guint set_status = NF_DVR_STATUS_LIVE;
	
	g_return_val_if_fail(cmd != NULL, 0);
	g_return_val_if_fail(sysdb_keyctrl != NULL, 0);

	addr = cmd[1];
	oc1  = cmd[2];
	oc2  = cmd[3];

	if(addr != sysdb_keyctrl->addr) 
	{
		g_warning("%s addr[%d] sysdb_keyctrl->addr[%d]", __FUNCTION__, addr, sysdb_keyctrl->addr);
		return 0;
	}

	set_status = nf_notify_get_param0("dvr_status");

	g_return_val_if_fail(set_status < NF_DVR_STATUS_NR , 0);

#ifdef DEBUG_KEYCTRL_WONWOO
	g_message("%s set_status[%d]", __FUNCTION__, set_status);
#endif	
		
	if ( ( set_status == NF_DVR_STATUS_LIVE ) || ( set_status == NF_DVR_STATUS_INIT ) || ( set_status == NF_DVR_STATUS_ZOOM ) || ( set_status == NF_DVR_STATUS_RUN_PLAYBACK )  || ( set_status == NF_DVR_STATUS_PTZ ))
	{
		uData[iKeyCnt] = oc1;

		if( uData[iKeyCnt] == 0x21 ) //enter
		{
			if(iKeyCnt == 1 || iKeyCnt == 2) {
				vkey = _cmd_WonWoo(oc2, set_status);
			}
			else if(iKeyCnt > 2) {
				iKeyCnt = 0;
				return 0;
			}
			else
			{
				uData[0] = oc1;
				vkey = _cmd_WonWoo(oc2, set_status);
			}
		}
		else if(  !((uData[iKeyCnt]>=0x01) && (uData[iKeyCnt]<= 0x0A)) )
		{
				uData[0] = oc1;
				vkey = _cmd_WonWoo(oc2, set_status);			
		}
		else 
		{
			iKeyCnt++;		
		}

	}
	else
	{
		uData[0] = oc1;
		vkey = _cmd_WonWoo(oc2, set_status);
	}

	return vkey;
}


NF_KEYCTRL_DECODE _nf_keyctrl_proto_wonwoo = {
	.func_receive       = _WonWoo_keyctrl_receive,
	.func_read_command  = _WonWoo_keyctrl_read_command,
	.idx				= 0,
	.proto_name		  	= "ICK-3000"
};

