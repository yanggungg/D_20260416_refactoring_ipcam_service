#include "nf_keyctrl.h"
#include <sys/ioctl.h>

#define CMD_VICON_LENGTH 7
#define CMD_VICON_LENGTH1 9

#define KEYCTRL_VICON_DEGUG

int _vicon_keyctrl_receive(const int fd);
guint _vicon_keyctrl_read_command(const NfKeyctrl *self);

static guint nf_cmd_vicon(unsigned char *cmd, NfKeyctrl *self);
static guint _cmd_vicon(unsigned char data1, unsigned char data2, unsigned char data3);
static int cmd_length = CMD_VICON_LENGTH;

static unsigned char data[CMD_VICON_LENGTH1] = {0,};

static unsigned char vicon_checksum(unsigned char *data)
{
	int i;
	int sum = 0;

	for(i = 0; i < cmd_length-1; i++) {
		sum += data[i];
	}
	sum ^= 0xa5;
	g_message("%s, sum = %d ", __FUNCTION__, sum);
	return (unsigned char)(sum % 0x100);
}                                   

int _vicon_keyctrl_receive(const int fd)
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

#ifdef KEYCTRL_VICON_DEGUG
	g_message("rxchar : %x", rxchar);
	g_message("read_count : %d", read_count);
#endif

	if((read_count == 1) && (data[0] != 0xe5))
	{
		read_count = 0;
		memset(data, 0x00, sizeof(data));
		g_message("%s read header packet error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	if(read_count == 4) 
	{
		if(data[3] == 0x80)
		{
			cmd_length = CMD_VICON_LENGTH1;
		}
		else
		{
			cmd_length = CMD_VICON_LENGTH;
		}
	}

	if(cmd_length == CMD_VICON_LENGTH1)
	{
		if((read_count == 9) && (data[8] != vicon_checksum(data)))
		{
			read_count = 0;
			memset(data, 0x00, sizeof(data));
			g_message("%s read checksum packet error", __FUNCTION__);
			return KEYCTRL_PACKET_ERR;
		}
	}
	else
	{
		if((read_count == 7) && (data[6] != vicon_checksum(data)))
		{
			read_count = 0;
			memset(data, 0x00, sizeof(data));
			g_message("%s read checksum packet error", __FUNCTION__);
			return KEYCTRL_PACKET_ERR;
		}
	}

	if(read_count != cmd_length)
	{
		return KEYCTRL_PACKET_PILE;
	}

	g_message("%x|%x|%x|%x|%x|%x|%x|%x|%x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
	read_count = 0;
	return KEYCTRL_PACKET_EMPTY;
}

guint _vicon_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey = 0;

	vkey = nf_cmd_vicon(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}
	
	return vkey;
}

static guint _cmd_vicon(unsigned char data1, unsigned char data2, unsigned char data3)
{
	guint cmd = 0;

#ifdef KEYCTRL_VICON_DEGUG
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
			cmd = NF_KEYCTRL_BUTTON_MAP_RW;
		break;
		case 0x04:
			cmd = NF_KEYCTRL_BUTTON_MAP_ENTER;
		break;
		case 0x05:
			cmd = NF_KEYCTRL_BUTTON_MAP_FW;
		break;
		case 0x06:
			cmd = NF_KEYCTRL_BUTTON_MAP_PAUSE;
		break;
		case 0x07:
			cmd = NF_KEYCTRL_BUTTON_MAP_PANIC;
		break;
		case 0x18:
			if(data3 == 1) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 2) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 3) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 4) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 5) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 6) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 7) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 8) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 9) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 10) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 11) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 12) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 13) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 15) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 16) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
			else if(data3 == 17) cmd = NF_KEYCTRL_BUTTON_MAP_NUM1;
		break;
		case 0x20:
			cmd = NF_KEYCTRL_BUTTON_MAP_SEQ;
		break;
		case 0x21:
			cmd = NF_KEYCTRL_BUTTON_MAP_SEARCH;
		break;
		case 0x22:
		break;
		case 0x23:
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
			cmd = NF_KEYCTRL_BUTTON_MAP_LOCK;
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

static guint _cmd_vicon1(unsigned char data1, unsigned char data2, unsigned char data3)
{
	guint cmd = 0;

#ifdef KEYCTRL_VICON_DEGUG
	g_message("%s : %x|%x|%x\n", __FUNCTION__, data1, data2, data3);
#endif

	if(data1 == 0xa1)
	{
		cmd = NF_KEYCTRL_BUTTON_MAP_RETURN;	
	}
	else 
	{
		if((data2 > 0x00) && (data2 <= 0x3f))
		{
			cmd = NF_KEYCTRL_BUTTON_MAP_RIGHT;	
		}
		else if((data2 > 0x80) && (data2 <= 0xbf))
		{
			cmd = NF_KEYCTRL_BUTTON_MAP_LEFT;	
		}
		else if((data3 > 0x00) && (data3 <= 0x3f))
		{
			cmd = NF_KEYCTRL_BUTTON_MAP_UP;	
		}
		else if((data3 > 0x80) && (data3 <= 0xbf))
		{
			cmd = NF_KEYCTRL_BUTTON_MAP_DOWN;	
		}
		else
		{
			cmd = 0;	
		}
	}
	return cmd;
}
static guint nf_cmd_vicon(unsigned char *cmd, NfKeyctrl *self)
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
	if(cmd_length == CMD_VICON_LENGTH)
	{
		vkey = _cmd_vicon(cmd[3], cmd[4], cmd[5]);
	}
	else
	{
		vkey = _cmd_vicon1(cmd[5], cmd[6], cmd[7]);	
	}
	
	return vkey;
}

NF_KEYCTRL_DECODE _nf_keyctrl_proto_vicon = {
	.func_receive       = _vicon_keyctrl_receive,
	.func_read_command  = _vicon_keyctrl_read_command,
	.idx				= 0,
	.proto_name		  	= "vicon"
};
