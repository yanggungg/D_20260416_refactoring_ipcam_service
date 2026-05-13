#include "nf_keyctrl.h"
#include <sys/ioctl.h>

#define CMD_GRUNDIG_LENGTH 7
#define CMD_GRUNDIG_LENGTH1 9

//#define KEYCTRL_GRUNDIG_DEGUG

int _grundig_keyctrl_receive(const int fd);
guint _grundig_keyctrl_read_command(const NfKeyctrl *self);

static guint nf_cmd_grundig(unsigned char *cmd, NfKeyctrl *self);
static guint _cmd_grundig(unsigned char data1, unsigned char data2, unsigned char data3);
static int cmd_length = CMD_GRUNDIG_LENGTH;
static unsigned char data[CMD_GRUNDIG_LENGTH1] = {0,};
static guint prev_cmd = 0;

static unsigned char grundig_checksum(unsigned char *data)
{
	int i;
	int sum = 0;

	for(i = 0; i < cmd_length-1; i++) {
		sum += data[i];
	}
	sum ^= 0xa5;
#ifdef KEYCTRL_GRUNDIG_DEGUG
	g_message("%s, sum = %d ", __FUNCTION__, sum);
#endif
	return (unsigned char)(sum % 0x100);
}                                   

int _grundig_keyctrl_receive(const int fd)
{
	int ret;
	unsigned char rxchar = 0;
	static int read_count = 0;

	ret = read(fd, &rxchar, 1);
	if(ret < 0) {
		read_count = -1;
		memset(data, 0x00, sizeof(data));
		g_message("%s read error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	data[read_count++] = rxchar;

#ifdef KEYCTRL_GRUNDIG_DEGUG
	g_message("rxchar : %x", rxchar);
	g_message("read_count : %d", read_count);
#endif

	if((read_count == 1) && (data[0] != 0xe5))
	{
		read_count = 0;
		memset(data, 0x00, sizeof(data));
#ifdef KEYCTRL_GRUNDIG_DEGUG
		g_message("%s read header packet error", __FUNCTION__);
#endif
		return KEYCTRL_PACKET_ERR;
	}
	if(read_count == 4) 
	{
		if(data[3] == 0x80)
		{
			cmd_length = CMD_GRUNDIG_LENGTH1;
		}
		else
		{
			cmd_length = CMD_GRUNDIG_LENGTH;
		}
	}

	if(cmd_length == CMD_GRUNDIG_LENGTH1)
	{
		if((read_count == 9) && (data[8] != grundig_checksum(data)))
		{
			read_count = 0;
			memset(data, 0x00, sizeof(data));
#ifdef KEYCTRL_GRUNDIG_DEGUG
			g_message("%s read checksum packet error", __FUNCTION__);
#endif
			return KEYCTRL_PACKET_ERR;
		}
	}
	else
	{
		if((read_count == 7) && (data[6] != grundig_checksum(data)))
		{
			read_count = 0;
			memset(data, 0x00, sizeof(data));
#ifdef KEYCTRL_GRUNDIG_DEGUG
			g_message("%s read checksum packet error", __FUNCTION__);
#endif
			return KEYCTRL_PACKET_ERR;
		}
	}

	if(read_count != cmd_length)
	{
		return KEYCTRL_PACKET_PILE;
	}

#ifdef KEYCTRL_GRUNDIG_DEGUG
	g_message("%x|%x|%x|%x|%x|%x|%x|%x|%x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
#endif
	read_count = 0;
	return KEYCTRL_PACKET_EMPTY;
}

guint _grundig_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey = 0;

	vkey = nf_cmd_grundig(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}
	
	return vkey;
}

static guint _cmd_grundig(unsigned char data1, unsigned char data2, unsigned char data3)
{
	guint cmd = 0;
		
#ifdef KEYCTRL_GRUNDIG_DEGUG
	g_message("%s : %x|%x|%x\n", __FUNCTION__, data1, data2, data3);
#endif

	switch(data1)
	{
		case 0x01:
	
		break;
		case 0x02:
			cmd = NF_KEYCTRL_BUTTON_MAP_SETUP;
		break;
		case 0x03:
			if(prev_cmd == NF_KEYCTRL_BUTTON_MAP_RW)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_RF;
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_RW;
			}
			prev_cmd = NF_KEYCTRL_BUTTON_MAP_RW;
		break;
		case 0x04:
			cmd = NF_KEYCTRL_BUTTON_MAP_ENTER;
		break;
		case 0x05:
			if(prev_cmd == NF_KEYCTRL_BUTTON_MAP_FW)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_FF;
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_FW;
			}
			prev_cmd = NF_KEYCTRL_BUTTON_MAP_FW;
		break;
		case 0x06:
			cmd = NF_KEYCTRL_BUTTON_MAP_RETURN;
		break;
		case 0x07:
			cmd = NF_KEYCTRL_BUTTON_MAP_PANIC;
		break;
		case 0x19:
			if(data3 == 1) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 2) cmd = NF_KEYCTRL_BUTTON_MAP_NUM2;
			else if(data3 == 3) cmd = NF_KEYCTRL_BUTTON_MAP_NUM3;
			else if(data3 == 4) cmd = NF_KEYCTRL_BUTTON_MAP_NUM4;
			else if(data3 == 5) cmd = NF_KEYCTRL_BUTTON_MAP_NUM5;
			else if(data3 == 6) cmd = NF_KEYCTRL_BUTTON_MAP_NUM6;
			else if(data3 == 7) cmd = NF_KEYCTRL_BUTTON_MAP_NUM7;
			else if(data3 == 8) cmd = NF_KEYCTRL_BUTTON_MAP_NUM8;
			else if(data3 == 9) cmd = NF_KEYCTRL_BUTTON_MAP_NUM9;
			else if(data3 == 10) cmd = NF_KEYCTRL_BUTTON_MAP_NUM10;
			else if(data3 == 11) cmd = NF_KEYCTRL_BUTTON_MAP_NUM11;
			else if(data3 == 12) cmd = NF_KEYCTRL_BUTTON_MAP_NUM12;
			else if(data3 == 13) cmd = NF_KEYCTRL_BUTTON_MAP_NUM13;
			else if(data3 == 14) cmd = NF_KEYCTRL_BUTTON_MAP_NUM14;
			else if(data3 == 15) cmd = NF_KEYCTRL_BUTTON_MAP_NUM15;
			else if(data3 == 16) cmd = NF_KEYCTRL_BUTTON_MAP_NUM16;
		break;
		case 0x20:
			cmd = NF_KEYCTRL_BUTTON_MAP_SEQ;
		break;
		case 0x21:
			cmd = NF_KEYCTRL_BUTTON_MAP_SEARCH;
		break;
		case 0x22:
			cmd = NF_KEYCTRL_BUTTON_MAP_PTZ;
		break;
		case 0x23:
			cmd = NF_KEYCTRL_BUTTON_MAP_RELAY;
		break;
		case 0x24:
		break;
		case 0x25:
		break;
		case 0x31:
		break;
		case 0x32:
			cmd = NF_KEYCTRL_BUTTON_MAP_ARCHIVE; 
		break;
		case 0x33:
			cmd = NF_KEYCTRL_BUTTON_MAP_LOGOUT;
		break;
		case 0x34:
			cmd = NF_KEYCTRL_BUTTON_MAP_ZOOM;
		break;
		case 0x35:
			cmd = NF_KEYCTRL_BUTTON_MAP_DISPLAY;
		break;
		default:
			cmd = 0;
		break;
	}

	return cmd;
}

static guint _cmd_grundig1(unsigned char data1, unsigned char data2, unsigned char data3)
{
	guint cmd = 0;
	guint set_status = NF_DVR_STATUS_LIVE;

#ifdef KEYCTRL_GRUNDIG_DEGUG
	g_message("%s : %x|%x|%x\n", __FUNCTION__, data1, data2, data3);
#endif

	set_status = nf_notify_get_param0("dvr_status");

	if(data1 == 0xa1)
	{
		cmd = NF_KEYCTRL_BUTTON_MAP_RETURN;	
	}
	else if(data3 == 0x10)
	{
		cmd = NF_KEYCTRL_BUTTON_MAP_PAUSE;
		prev_cmd = NF_KEYCTRL_BUTTON_MAP_PAUSE;
	}
	else 
	{
		if((data2 > 0x07) && (data2 <= 0x7f))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT;	
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_RIGHT;	
			}
		}
		else if((data2 > 0x80) && (data2 <= 0xff))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT;	
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_LEFT;	
			}
		}
		else if((data3 > 0x10) && (data3 <= 0x7f))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_UP;	
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_UP;	
			}
		}
		else if((data3 > 0x80) && (data3 <= 0xff))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_JOYSTIC_DOWN;	
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_DOWN;	
			}
		}
		else if((data1 > 0x10) && (data1 < 0x17))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_TURN_RIGHT;
			}
			if(set_status == NF_DVR_STATUS_ZOOM)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_FF;
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_ENTER;
			}
		}
		else if((data1 > 0x1a) && (data1 < 0x1f))
		{
			if(set_status == NF_DVR_STATUS_PTZ)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_TURN_LEFT;
			}
			if(set_status == NF_DVR_STATUS_ZOOM)
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_RF;
			}
			else
			{
				cmd = NF_KEYCTRL_BUTTON_MAP_RETURN;	
			}
		}
		else if((data1 == 0x00) && (data2 == 0x00) && (data3 == 0x00))
		{
			cmd = NF_KEYCTRL_BUTTON_MAP_STOP;	
		}
		else
		{
			cmd = 0;	
		}
	}
	return cmd;
}
static guint nf_cmd_grundig(unsigned char *cmd, NfKeyctrl *self)
{
	unsigned char addr;
	guint vkey = 0;

	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;

	addr = cmd[2];

	if(addr != sysdb_keyctrl->addr) 
	{
		g_warning("%s addr[%d] sysdb_keyctrl->addr[%d]", __FUNCTION__, addr, sysdb_keyctrl->addr);
		return 0;
	}
	if(cmd_length == CMD_GRUNDIG_LENGTH)
	{
		vkey = _cmd_grundig(cmd[3], cmd[4], cmd[5]);
	}
	else
	{
		vkey = _cmd_grundig1(cmd[5], cmd[6], cmd[7]);	
	}
	
	return vkey;
}

NF_KEYCTRL_DECODE _nf_keyctrl_proto_grundig = {
	.func_receive       = _grundig_keyctrl_receive,
	.func_read_command  = _grundig_keyctrl_read_command,
	.idx				= 0,
	.proto_name		  	= "Grundig"
};
