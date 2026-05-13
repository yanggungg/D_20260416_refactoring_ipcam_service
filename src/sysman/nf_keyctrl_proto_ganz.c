#include "nf_keyctrl.h"
#include <sys/ioctl.h>


#define CMD_GANZ_L_LENGTH 9
#define CMD_GANZ_S_LENGTH 8

static guint nf_cmd_Ganz(unsigned char *cmd, NfKeyctrl *self);
guint _Ganz_keyctrl_read_command(const NfKeyctrl *self);

static unsigned char data[CMD_GANZ_L_LENGTH] = {0,};

int _Ganz_keyctrl_receive(const int fd)
{
	int ret;
	unsigned char rxchar = 0;
	int cmd_length = CMD_GANZ_S_LENGTH;
	static int read_count = 0;

	ret = read(fd, &rxchar, 1);
	if(ret < 0) {
		read_count = 0;
		cmd_length = CMD_GANZ_S_LENGTH;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	data[read_count++] = rxchar;

	if((read_count == 1) && (data[0] != 0x82))
	{
		read_count = 0;
		cmd_length = CMD_GANZ_S_LENGTH;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read command length error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	if((data[5] == 0xc0) || (data[5] == 0xce))
	{
		cmd_length = CMD_GANZ_L_LENGTH;
	}
	if(read_count != cmd_length)
	{
		return KEYCTRL_PACKET_PILE;
	}

	read_count = 0;
	cmd_length = CMD_GANZ_S_LENGTH;
	return KEYCTRL_PACKET_EMPTY;
}

guint _Ganz_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey;

	vkey = nf_cmd_Ganz(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}

#ifdef DEBUG_KEYCTRL_GANZ
	HexDump(data, cmd_length, 0);
	g_message("%s NF_KEYCONT_BUTTON_NAME [%s]",__FUNCTION__, _NF_KEYCTRL_BUTTON_STR[vkey - 1]);
#endif

	return vkey;
}

static guint nf_cmd_Ganz(unsigned char *cmd, NfKeyctrl *self)
{
	unsigned char valid, data , addr;
	guint	vkey = 0;
	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;
	guint set_status = NF_DVR_STATUS_LIVE;

	g_return_val_if_fail(cmd != NULL, 0);
	g_return_val_if_fail(sysdb_keyctrl != NULL, 0);

	set_status = nf_notify_get_param0("dvr_status");

	g_return_val_if_fail(set_status < NF_DVR_STATUS_NR , 0);

	addr = cmd[4];
	valid = cmd[5];
	data = cmd[7];
	
	if(addr != sysdb_keyctrl->addr) 
		return 0;

	switch (valid)
	{
		case 0xC0:
			switch (data)
			{
				case 0x00:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM1;			//0 + 1
					break;
				case 0x01:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM2;			//0 + 2
					break;
				case 0x02:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM3;			//0 + 3
					break;
				case 0x03:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM4;			//0 + 4
					break;
				case 0x04:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM5;			//0 + 5
					break;
				case 0x05:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM6;			//0 + 6
					break;
				case 0x06:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM7;			//0 + 7
					break;
				case 0x07:
						vkey = NF_KEYCTRL_BUTTON_MAP_NUM8;			//0 + 8
					break;
				case 0x08:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM9;
					break;
				case 0x09:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM10;
					break;
				case 0x0A:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM11;
					break;
				case 0x0B:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM12;
					break;
				case 0x0C:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM13;
					break;
				case 0x0D:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM14;
					break;
				case 0x0E:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM15;
					break;
				case 0x0F:
						vkey=NF_KEYCTRL_BUTTON_MAP_NUM16;
					break;
			}
			break;
		case 0xC7:
				vkey = NF_KEYCTRL_BUTTON_MAP_SEARCH;			//SEARCH
			break;
		case 0xC9:
				vkey = NF_KEYCTRL_BUTTON_MAP_ENTER;				//ENTER
			break;
		case 0xCB:
				vkey = NF_KEYCTRL_BUTTON_MAP_LEFT;
			break;
		case 0xCA:
				vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT;
			break;
		case 0xCC:
				vkey = NF_KEYCTRL_BUTTON_MAP_UP;			
			break;
		case 0xCD:
				vkey = NF_KEYCTRL_BUTTON_MAP_DOWN;			
			break;
		case 0xCE:
			if(data == 0x01)
				vkey = NF_KEYCTRL_BUTTON_MAP_RETURN;			//STOP
			break;
		case 0xCF:
			vkey = NF_KEYCTRL_BUTTON_MAP_PAUSE;					//PAUSE
			break;
		case 0xC2:
				vkey = NF_KEYCTRL_BUTTON_MAP_POWER;				//USER
			break;
		case 0xC1:
				vkey = NF_KEYCTRL_BUTTON_MAP_DISPLAY;			//DISPLAY
			break;
		case 0xC4:
				vkey = NF_KEYCTRL_BUTTON_MAP_ZOOM;				//FULL ZOOM
			break;
		case 0xC5:
				vkey = NF_KEYCTRL_BUTTON_MAP_SEQ;				//PANORAMA
			break;
		case 0xD9:
		case 0xDA:
				vkey = NF_KEYCTRL_BUTTON_MAP_PTZ;				//PTZ
			break;
		case 0xC3:
				vkey = NF_KEYCTRL_BUTTON_MAP_ARCHIVE;			//SPOT
			break;
		case 0xC6:
				vkey = NF_KEYCTRL_BUTTON_MAP_LOCK;				//PIP
			break;
		case 0xDE:
				vkey = NF_KEYCTRL_BUTTON_MAP_RF;				//PREV
			break;
		case 0xDD:
				vkey = NF_KEYCTRL_BUTTON_MAP_FF;				//NEXT
			break;
		case 0xC8:
				vkey = NF_KEYCTRL_BUTTON_MAP_SETUP;				//MENU
						//not BUTTON Press
			break;
		default:
			return 0;
	}
	return vkey;
}

NF_KEYCTRL_DECODE _nf_keyctrl_proto_ganz = {
	.func_receive       = _Ganz_keyctrl_receive,
	.func_read_command  = _Ganz_keyctrl_read_command,
	.idx				= 0,
	.proto_name		  	= "Ganz"
};
