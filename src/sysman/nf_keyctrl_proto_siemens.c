#include "nf_keyctrl.h"
#include <sys/ioctl.h>

#define CMD_SIEMENS_L_LENGTH 19
#define CMD_SIEMENS_S_LENGTH 16

static guint nf_cmd_Siemens(unsigned char *cmd, NfKeyctrl *self);
guint _Siemens_keyctrl_read_command(const NfKeyctrl *self);

static unsigned char data[CMD_SIEMENS_L_LENGTH] = {0,};

int _Siemens_keyctrl_receive(const int fd)
{
	int ret;
	unsigned char rxchar = 0;
	int cmd_length = CMD_SIEMENS_S_LENGTH;
	static int read_count = 0;

	ret = read(fd, &rxchar, 1);
	if(ret < 0) {
		read_count = 0;
		cmd_length = CMD_SIEMENS_S_LENGTH;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	data[read_count++] = rxchar;

	if((read_count == 1) && ((data[0] == 0x0a) || (data[0] == 0x40)))
	{
		read_count = 0;
		cmd_length = CMD_SIEMENS_S_LENGTH;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read command length error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	if((data[0] == 0x0a) || ((data[0] == 0x40) && (data[10] == 0x30)))
	{
		cmd_length = CMD_SIEMENS_L_LENGTH;
	}
	if(read_count != cmd_length)
	{
		return KEYCTRL_PACKET_PILE;
	}

	read_count = 0;
	cmd_length = CMD_SIEMENS_S_LENGTH;
	return KEYCTRL_PACKET_EMPTY;
}

guint _Siemens_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey;

	vkey = nf_cmd_Siemens(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}
		
#ifdef DEBUG_KEYCTRL_SIEMENS
	HexDump(data, cmd_length, 0);
	g_message("%s NF_KEYCONT_BUTTON_NAME [%s]",__FUNCTION__, _NF_KEYCTRL_BUTTON_STR[vkey - 1]);
#endif

	return vkey;
}


static guint nf_cmd_Siemens(unsigned char *cmd, NfKeyctrl *self)
{
	guint vkey = 0;

	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;
	
	g_return_val_if_fail(cmd != NULL, 0);
	g_return_val_if_fail(sysdb_keyctrl != NULL, 0);
	
	switch(cmd[0])
	{	
		//AX-NO.3 Telem  COM2 
		case 0x0a:
			if((cmd[11] == 0x50) || (cmd[11] == 0x47))	// num + Mem or num + Pos -> number
			{
				switch(cmd[13])
				{
					case 0x30:
						switch(cmd[14])
						{
							case 0x31:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM1; break;
							case 0x32:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM2; break;
							case 0x33:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM3; break;
							case 0x34:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM4; break;
							case 0x35:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM5; break;
							case 0x36:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM6; break;
							case 0x37:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM7; break;
							case 0x38:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM8; break;
							case 0x39:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM9; break;			
							}
						break;
					case 0x31:
						switch(cmd[14])
						{
							case 0x30:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM10; break;
							case 0x31:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM11; break;
							case 0x32:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM12; break;
							case 0x33:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM13; break;
							case 0x34:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM14; break;
							case 0x35:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM15; break;
							case 0x36:	vkey = NF_KEYCTRL_BUTTON_MAP_NUM16; break;
						}
						break;
				}
			}
				
			break;
	}

	if (cmd[0] == 0x40) 
	{
		//AX-NO.3 Direct  COM2 	
		if ((cmd[10] == 0x32) && (cmd[11] == 0x34)) 		//joystick left -> left
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_LEFT;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x37))	//joystick right -> right
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_RIGHT; 
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x35))	//joystick up -> up
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_UP;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x36))	//joystick down -> down
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_DOWN;
		}
		else if ((cmd[10] == 0x33) && (cmd[11] == 0x32))	//Start -> enter
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_ENTER; 
		}
		else if ((cmd[10] == 0x33) && (cmd[11] == 0x30))    //Stop -> return
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_RETURN; 
		}
		else if ((cmd[10] == 0x31) && (cmd[11] == 0x38))	//F2 -> Display
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_DISPLAY;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x30))	//F3 -> SEQ
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_SEQ;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x31))	//F4 -> Panic
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_PANIC;
		}
		else if ((cmd[10] == 0x33) && (cmd[11] == 0x31))	//F5 -> Zoom
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_ZOOM;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x39))    //F6 -> Archive
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_ARCHIVE;
		}
//		else if ((cmd[10] == 0x33) && (cmd[11] == 0x32))	//F7 -> 
//		{
//			vkey=NF_KEYCTRL_BUTTON_MAP_ARCHIVE;
//		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x32))	//F8 -> Setup
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_SETUP;
		}
		else if ((cmd[10] == 0x31) && (cmd[11] == 0x37))	//F9 -> Search
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_SEARCH;
		}
		else if ((cmd[10] == 0x32) && (cmd[11] == 0x38))    //F10 -> ¡«
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_PAUSE;
		}
		//AX-NO.3 Telem  COM2 
		else if ((cmd[10] == 0x30) && (cmd[11] == 0x36))	// < -> ¢¸
		{
			vkey = NF_KEYCTRL_BUTTON_MAP_LEFT;
		}
		else if ((cmd[10] == 0x30) && (cmd[11] == 0x38))	// -> ¢º
		{
			vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT;
		}
		else if ((cmd[10] == 0x30) && (cmd[11] == 0x37))	// -> ¢º¢º
		{
			vkey = NF_KEYCTRL_BUTTON_MAP_FF;
		}
		else if ((cmd[10] == 0x30) && (cmd[11] == 0x35))    // -> ¢¸¢¸
		{
			vkey=NF_KEYCTRL_BUTTON_MAP_RF;
		}
		else
		{
			return 0;
		}
	}
	
	return vkey;
}

NF_KEYCTRL_DECODE _nf_keyctrl_proto_siemens = {
	.func_receive       = _Siemens_keyctrl_receive,
	.func_read_command  = _Siemens_keyctrl_read_command,
	.idx				= 0,
	.proto_name		  	= "Siemens"
};
