#include "nf_keyctrl.h"
#include "nf_sysman.h"
#include "nf_qc.h"
#include <sys/ioctl.h>

//#define DEBUG_KEYCTRL_D_MAX	
#define CMD_DMAX_LENGTH	11

static guint nf_cmd_DMax(unsigned char *cmd, NfKeyctrl *self);
guint _D_Max_keyctrl_read_command(const NfKeyctrl *self);

static unsigned char data[CMD_DMAX_LENGTH] = {0,};

int _D_Max_keyctrl_receive(const int fd)
{
	int ret;
	unsigned char rxchar = 0;
	int cmd_length = CMD_DMAX_LENGTH;
	static int read_count = 0;

// #if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
       if (nf_sysman_qcmode_is_enable()) {   //for RS-485 QC Test( RX <-> TX )
       
                unsigned char data[NF_SYSMAN_QC_RS485_TEXT_LEN] = {0, };
                read(fd, data, NF_SYSMAN_QC_RS485_TEXT_LEN);

                int i;
                for(i=0; i<NF_SYSMAN_QC_RS485_TEXT_LEN; i++)
                {
                        printf("check_check fd is [%d] . data[%d] is [%c]  \n", fd, i, data[i]);
                }
                if (strstr(data, NF_SYSMAN_QC_RS485_TEXT))
                        return NF_SYSMAN_QC_RS485_OK;
        }
// #endif 

	ret = read(fd, &rxchar, 1);
#ifdef DEBUG_KEYCTRL_D_MAX
	g_message("==== %s : %d =====", __FUNCTION__, ret);
#endif
	if(ret < 0) {
		read_count = 0;
		memset(data, 0x00, sizeof(data));
//		g_message("%s read error", __FUNCTION__);
		return KEYCTRL_PACKET_ERR;
	}
	data[read_count++] = rxchar;

	if((read_count == 1) && (data[0] != 0x55))
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

guint _D_Max_keyctrl_read_command(const NfKeyctrl *self)
{
	guint vkey;

	vkey = nf_cmd_DMax(data, self);
	memset(data, 0x00, sizeof(data));
	if(!vkey)
	{
		g_message("%s KeyMapping Not Match", __FUNCTION__);
		return 0;
	}


#ifdef DEBUG_KEYCTRL_D_MAX	
	HexDump(data, CMD_DMAX_LENGTH, 0);
	g_message("%s NF_KEYCONT_BUTTON_NAME [%s]",__FUNCTION__, _NF_KEYCTRL_BUTTON_STR[vkey - 1]);
#endif

	return vkey;
}

static int keysetnoflag = 0;

static guint nf_cmd_DMax(unsigned char *cmd, NfKeyctrl *self)
{
	unsigned char uSM;
	unsigned char uOch;
	unsigned char uOcl;
	unsigned char uDB1;
	unsigned char uDB2;
	unsigned char uDA;
	guint			  vkey = 0;
	int 		  keyCtrlActionflag;
	guint set_status = NF_DVR_STATUS_LIVE;

	uSM = cmd[0];
	uDA = cmd[2];
	uOch = cmd[3];
	uOcl = cmd[4];
	uDB1 = cmd[5];
	uDB2 = cmd[6];

	NF_KEYCTRL_SYSDB		*sysdb_keyctrl = &self->sysdb_keyctrl;
	
	g_return_val_if_fail(cmd != NULL, 0);
	g_return_val_if_fail(sysdb_keyctrl != NULL, 0);
	
	
	if (uDA == sysdb_keyctrl->addr)
		keyCtrlActionflag = 1;
	else
		keyCtrlActionflag = 0;

	//g_message("****************keyCtrlActionflag[%d]", keyCtrlActionflag);

	set_status = nf_notify_get_param0("dvr_status");

	g_return_val_if_fail(set_status < NF_DVR_STATUS_NR , 0);
	
	if (keyCtrlActionflag)
	{
		switch (uSM)
		{
			case 0x55:
				switch (uOch)
				{
					case 0x00:
						switch (uOcl)
						{
							case 0x02:
								if (!keysetnoflag )
								{
									vkey = NF_KEYCTRL_BUTTON_MAP_RIGHT;	// right joystick	
									keysetnoflag = 1;
								}
								break;
							case 0x04:
								if (!keysetnoflag )
								{
									vkey = NF_KEYCTRL_BUTTON_MAP_LEFT;		// left joystick	
									keysetnoflag = 1;
								}
								break;
							case 0x20:
								if (!keysetnoflag )
								{
									if ( set_status == NF_DVR_STATUS_RUN_PLAYBACK )					
									{
										vkey = NF_KEYCTRL_BUTTON_MAP_FF;		//FF button when palyback
										keysetnoflag = 1;
									}
								}
								break;
							case 0x40:
								if (!keysetnoflag )
								{
									if ( set_status == NF_DVR_STATUS_RUN_PLAYBACK )					
									{
										vkey = NF_KEYCTRL_BUTTON_MAP_RF;		//RF button when palyback
										keysetnoflag = 1;
									}
								}
								break;
							case 0x08:
								if (!keysetnoflag )
								{
									vkey = NF_KEYCTRL_BUTTON_MAP_UP;			// up joystick
									keysetnoflag = 1;
								}
								
								break;
							case 0x10:
								if (!keysetnoflag )
								{
									vkey = NF_KEYCTRL_BUTTON_MAP_DOWN;			// down joystick
									keysetnoflag = 1;
								}
								break;
							case 0x5D:
								vkey = NF_KEYCTRL_BUTTON_MAP_SETUP;		//status button
								
								break;
							case 0x00:
								keysetnoflag = 0;
								break;
						}
						break;
					case 0x01:
						if (!keysetnoflag )
						{
							vkey = NF_KEYCTRL_BUTTON_MAP_ENTER;
							keysetnoflag = 1;
						}
						break;
					case 0x02:
						if (!keysetnoflag )
						{
							vkey = NF_KEYCTRL_BUTTON_MAP_RETURN;
							keysetnoflag = 1;
						}
						break;
					case 0x20:
						switch (uDB1)
						{
							case 0x01:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM1;	// 1+RUN
								break;
							case 0x02:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM2;
								break;
							case 0x03:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM3;
								break;
							case 0x04:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM4;
								break;
							case 0x05:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM5;
								break;
							case 0x06:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM6;
								break;
							case 0x07:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM7;
								break;
							case 0x08:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM8;							
								break;
							case 0x09:								
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM9;
								break;
							case 0x0A:							
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM10;
								break;
							case 0x0B:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM11;
								break;
							case 0x0C:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM12;
								break;
							case 0x0D:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM13;
								break;
							case 0x0E:							
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM14;
								break;
							case 0x0F:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM15;
								break;
							case 0x10:
								vkey = NF_KEYCTRL_BUTTON_MAP_NUM16;
								break;
							case 0x11:
								vkey = NF_KEYCTRL_BUTTON_MAP_SEQ;	// SEQUENCE button, 17+RUN
								break;
							case 0x12:
								vkey = NF_KEYCTRL_BUTTON_MAP_PANIC;	// PANIC button, 18+RUN
								break;
							case 0x13:
								vkey = NF_KEYCTRL_BUTTON_MAP_ZOOM;	// ZOOM button, 19+RUN
								break;
							// for 8NUM, 16NUM 
							case 0x14:
								vkey = NF_KEYCTRL_BUTTON_MAP_PTZ;	//PTZ button, 20+RUN
								break;
							case 0x15:
								vkey = NF_KEYCTRL_BUTTON_MAP_DISPLAY;	//DISPLAY button, 21+RUN
								break;
							case 0x16:
								vkey = NF_KEYCTRL_BUTTON_MAP_LOCK;	//LOCK button, 22+RUN
								break;
							case 0x17:
								vkey = NF_KEYCTRL_BUTTON_MAP_ARCHIVE;	//ARCHIVE button, 23+RUN
								break;
						}
						break;
					case 0x25:
						vkey = NF_KEYCTRL_BUTTON_MAP_SEARCH;	// View button
						break;
					case 0x80:
						vkey = NF_KEYCTRL_BUTTON_MAP_PAUSE;
						break;
				}
				break;
			default:
				return 0;
		}
	}
	return vkey;
}

NF_KEYCTRL_DECODE _nf_keyctrl_proto_d_max = {
	.func_receive       = _D_Max_keyctrl_receive,
	.func_read_command  = _D_Max_keyctrl_read_command,
	.idx				= 0,
#ifdef VIDECON
	.proto_name		  	= "VA-KBDPRO+"
#else
	.proto_name		  	= "D-Max"
#endif
};
