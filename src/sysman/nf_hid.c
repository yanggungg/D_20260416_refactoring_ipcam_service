#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <errno.h>
#include <math.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

#include <linux/hidraw.h>
#include <linux/version.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "nf_common.h"
#include "nf_hid.h"
#include "nf_ptz.h"
#include "nf_sysman.h"
#include "vsm.h"
#include "vw_live_ptz_internal.h"
#include "../nfgui/support/nf_ui_page_manager.h"
#include "../nfgui/support/event_loop.h"

#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#endif
#ifndef HIDIOCGFEATURE
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif


#define HID_PTZ_DELAY 1000*100

static GObjectClass *parent_class = NULL;
static NfHid		*_nf_hid = NULL;

static hid_input_rpt_desc 	input_rpt_desc[INPUT_REPORT_DESC_BUFF_SIZE];
static hid_input_rpt 		input_rpt[INPUT_REPORT_BUFF_SIZE];
static NF_PREV_PTZ_CMD	prev_ptz_cmd;
static unsigned short vendor_id = 0, product_id = 0;
static char dev_number=0;
int hid_joystick_connected_flag=0;
int hid_mouse_flag=0;
int hid_mode = 0;
extern int mouse_connected_flag;

static __u32 kernel_version = 0;
static HID_BUTTON button[HID_MAX_BUTTON_CNT] = {0xFF,};

hid_device *new_hid_device()
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = -1;
	dev->blocking = 1;
	dev->uses_numbered_reports = 0;
	dev->rpt_desc_size = 0;
	dev->rpt_desc_value = NULL;

	return dev;
}

static gint uses_numbered_reports(__u8 *report_descriptor, __u32 size)
{
	gint i = 0;
	gint size_code;
	gint data_len, key_size;
	
	while(i < size)
	{
		gint key = report_descriptor[i];

		if (key == 0x85)
		{
			return 1;
		}
				
		if ((key & 0xf0) == 0xf0)
		{
			if (i+1 < size)
				data_len = report_descriptor[i+1];
			else
				data_len = 0; 
			
			key_size = 3;
		}
		else
		{
			size_code = key & 0x3;
			switch (size_code)
			{
				case 0:
				case 1:
				case 2:
					data_len = size_code;
					break;
				case 3:
					data_len = 4;
					break;
				default:
					data_len = 0;
					break;
			};
			key_size = 1;
		}
		
		i += data_len + key_size;
	}
	
	return 0;
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number, char dev_num)
{
	struct hid_device_info *devs, *cur_dev;
	char path_to_open[16]={0,};
	hid_device *handle = NULL;

	snprintf(path_to_open, sizeof(path_to_open), "/dev/hidraw%c", dev_num);
	printf("\e[31m ## open_dev_path[%s] \e[0m\n", path_to_open);

	if (path_to_open) {
		handle = hid_open_path(path_to_open);
	}
	
	return handle;
}

hid_device * hid_open_path(const char *path)
{
	hid_device *dev = NULL;

	dev = new_hid_device();

	if (kernel_version == 0)
	{
		struct utsname name;
		gint major, minor, release;
		gint ret;
		uname(&name);
		ret = sscanf(name.release, "%d.%d.%d", &major, &minor, &release);
		if (ret == 3)
		{
			kernel_version = major << 16 | minor << 8 | release;
		}
		else
		{
			g_warning("Couldn't sscanf() version string %s", name.release);
		}
	}

	dev->device_handle = open(path, O_RDONLY);

	if (dev->device_handle < 0)
	{
		g_warning("HID open error");
		free(dev);
		return NULL;
	}
	else
	{
		gint res, desc_size = 0;
		struct hidraw_report_descriptor rpt_desc;

		memset(&rpt_desc, 0x0, sizeof(rpt_desc));

		res = ioctl(dev->device_handle, HIDIOCGRDESCSIZE, &desc_size);
		if (res < 0)
		{
			g_warning("HIDIOCGRDESCSIZE error");
			free(dev);
			return NULL;
		}
		printf("\e[31m>>>>>[%s|%d|desc_size = %d]\e[0m\n\n", __FUNCTION__, __LINE__, desc_size);

		rpt_desc.size = desc_size;

		res = ioctl(dev->device_handle, HIDIOCGRDESC, &rpt_desc);
		if (res < 0)
		{
			g_warning("HIDIOCGRDESC error");
			free(dev);
			return NULL;
		}
		else
		{
			dev->uses_numbered_reports = uses_numbered_reports(rpt_desc.value, rpt_desc.size);
			
			#if 0	// For Debug
				gint i = 0;
				g_message("Report Descriptor:");
				for (i = 0; i < rpt_desc.size; i++)
					g_message("%04x ", rpt_desc.value[i]);
			#endif
		}
		
		dev->rpt_desc_size = rpt_desc.size;
		dev->rpt_desc_value = rpt_desc.value;
		
		return dev;
	}
}

gint hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, gint milliseconds)
{
	gint bytes_read;

	if (milliseconds != 0) {
		gint ret;
		struct pollfd fds;

		fds.fd = dev->device_handle;
		fds.events = POLLIN;
		fds.revents = 0;
		ret = poll(&fds, 1, milliseconds);
		if (ret == -1 || ret == 0)
			return ret;
	}

	bytes_read = read(dev->device_handle, data, length);
	if (bytes_read < 0 && errno == EAGAIN)
		bytes_read = 0;
	
	if (bytes_read >= 0 &&	kernel_version < KERNEL_VERSION(2,6,34) && dev->uses_numbered_reports)
	{
		memmove(data, data+1, bytes_read);
		bytes_read--;
	}

	return bytes_read;
}

gint hid_read(hid_device *dev, unsigned char *data, size_t length)
{
	return hid_read_timeout(dev, data, length, (dev->blocking)? -1: 0);
}

gint hid_set_nonblocking(hid_device *dev, gint nonblock)
{
	gint flags, res;

	flags = fcntl(dev->device_handle, F_GETFL, 0);
	if (flags >= 0) {
		if (nonblock)
			res = fcntl(dev->device_handle, F_SETFL, flags | O_NONBLOCK);
		else
			res = fcntl(dev->device_handle, F_SETFL, flags & ~O_NONBLOCK);
	}
	else
		return -1;

	if (res < 0) {
		return -1;
	}
	else {
		dev->blocking = !nonblock;
		return 0;
	}
}

gboolean nf_hid_init()
{
	g_timeout_add(1000, nf_hid_get_dev_info, NULL);

	return 1;
}
gboolean nf_hid_initialize(int wait)
{
	hid_device *handle=NULL;

	hid_rpt_desc_item rpt_desc[HID_MAX_DESCRIPTOR_SIZE] = {0,};
	gint rpt_desc_item_cnt = 0;
	gint input_rpt_desc_cnt = 0;
	gint data_sequence = 0;
	gint input_rpt_cnt = 0;
	int i=0, j=0;
	
	g_message("%s start", __FUNCTION__);
	//g_return_val_if_fail (_nf_hid == NULL, FALSE);	
	
	memset(button, 0x0, sizeof(HID_BUTTON)*HID_MAX_BUTTON_CNT);

	for(i = 0; i < HID_MAX_BUTTON_CNT; i++)
	{
		for(j = 0; j < 5; j++)
		{
			button[i].press_flag[j] = 0;
			button[i].repeat_flag[j] = 0;
			button[i].event_flag[j] = 0;
		}
	}

	button_func_mapping();
	
	if(_nf_hid==NULL)
		_nf_hid = g_object_new( NF_TYPE_HID , NULL );

	printf("\e[31m@@@@@@ vendor_id[%d], product_id[%d], dev_num[%c] @@@@@@\e[0m\n", vendor_id, product_id, dev_number);

	handle = hid_open(vendor_id, product_id, NULL, dev_number);	// hard cording

	memset(&input_rpt_desc, 0x0, sizeof(input_rpt_desc));
	memset(&input_rpt, 0x0, sizeof(input_rpt));
	memset(&prev_ptz_cmd, 0x0, sizeof(prev_ptz_cmd));
	memcpy(&_nf_hid->handle, handle, sizeof(hid_device));
	
	free(handle);

	hid_set_nonblocking(&_nf_hid->handle, 0);

	hid_put_rpt_desc_item(&_nf_hid->handle, rpt_desc, &rpt_desc_item_cnt);

	g_message("Report Descriptor Print");
	hid_rpt_desc_parse(rpt_desc, rpt_desc_item_cnt, &input_rpt_desc_cnt, &data_sequence, &input_rpt_cnt);

	if( wait )
	{
		while( _nf_hid->init_done != 1)
			g_usleep(1000*100);
	}
	
	g_message("%s end", __FUNCTION__);
	
	return TRUE;
}

GType
nf_hid_get_type (void)
{
	static GType nf_hid_type = 0;

	if (G_UNLIKELY (nf_hid_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfHidClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_hid_class_init,
			NULL,
			NULL,
			sizeof (NfHid),
			0,
			(GInstanceInitFunc) nf_hid_instance_init,
			NULL
		};

		nf_hid_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfHid", &object_info, 0);
	}
	
	return nf_hid_type;
}

static void
nf_hid_class_init (NfHidClass * klass)
{
	GObjectClass *gobject_class;
	gint i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_hid_set_property;
	gobject_class->get_property = nf_hid_get_property;
			
	gobject_class->dispose = nf_hid_dispose;
	gobject_class->finalize = nf_hid_finalize;

}

static void
nf_hid_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
 	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_hid_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_hid_dispose (GObject * object)
{
	parent_class->dispose (object);  
}

static void
nf_hid_finalize (GObject * object)
{
	parent_class->finalize (object);
}

static void
nf_hid_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfHid *self = NF_HID (instance);

	self->init_done = 0;

	_get_hid_data_from_sysdb(self);

	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)hid_thread_func, 
									self, FALSE, NULL);
}

static void
_get_hid_data_from_sysdb(NfHid *self)
{	
	gint i;
	
	g_message("%s",__FUNCTION__);
}

static void
_hid_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	g_return_if_fail(pinfo != NULL);
	g_return_if_fail(_nf_hid != NULL);
        
	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS)	
		_nf_hid->sysdb_reload = 1;
} 

static void nf_hid_set_zoom_stop(NF_PTZ_CMD *pcmd, gchar ch)
{
	pcmd->ch = (gint) ch;
	pcmd->cmd = NF_PTZ_CMD_ZOOM_STOP;
	pcmd->params[0] = 0;
}

static void nf_hid_set_ptz_stop(NF_PTZ_CMD *pcmd, gchar ch)
{
	pcmd->ch = (gint) ch;
	pcmd->cmd = NF_PTZ_CMD_STOP;
	pcmd->params[0] = 0;
}

static void nf_hid_set_zoom(NF_PTZ_CMD *pcmd, gint z, gchar ch)
{
	if (z > 0)
		pcmd->cmd = NF_PTZ_CMD_ZOOM_TELE;
	else
		pcmd->cmd = NF_PTZ_CMD_ZOOM_WIDE;
	pcmd->ch = (gint) ch;
	pcmd->params[0] = 60;
}		

#define NF_HID_PTZ_STOP_VAL 25
static void nf_hid_set_pan_tilt(NF_PTZ_CMD *pcmd, gint x, gint y, gchar ch)
{
	gdouble speed = 0.;

	speed = (gint)sqrt((gdouble)pow(x, 2.0) + pow((gdouble)y, 2.0));
	
	if (abs(x) <= 25 && abs(y) <= 25)
	{
		gfloat grade = 0.;
		grade = (gfloat)y / x;

		if (x > 0)
		{
			if (y > 0)
			{
				if (grade >= 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_TILT_UP;
					speed /= 11;
				}
				else if (grade >= 0.8 && grade < 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_PT_RIGHTUP;
					speed /= 15;
				}
				else 
				{
					pcmd->cmd = NF_PTZ_CMD_PAN_RIGHT;
					speed /= 11;
				}
			}
			else 
			{
				if (abs(grade) >= 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_TILT_DOWN;
					speed /= 11;
				}
				else if (abs(grade) >= 0.8 && abs(grade) < 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_PT_RIGHTDOWN;
					speed /= 15;
				}
				else
				{
					pcmd->cmd = NF_PTZ_CMD_PAN_RIGHT;
					speed /= 11;
				}
			}
		}
		else
		{
			if (y < 0)
			{
				if (grade >= 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_TILT_DOWN;
					speed /= 11;
				}
				else if (grade >= 0.8 && grade < 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_PT_LEFTDOWN;
					speed /= 15;
				}
				else 
				{
					pcmd->cmd = NF_PTZ_CMD_PAN_LEFT;
					speed /= 11;
				}
			}
			else 
			{
				if (abs(grade) >= 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_TILT_UP;
					speed /= 11;
				}
				else if (abs(grade) >= 0.8 && abs(grade) < 1.2)
				{
					pcmd->cmd = NF_PTZ_CMD_PT_LEFTUP;
					speed /= 15;
				}
				else
				{
					pcmd->cmd = NF_PTZ_CMD_PAN_LEFT;
					speed /= 11;
				}
			}
		}

	}
	else
	{
		if (x < -NF_HID_PTZ_STOP_VAL)
		{
			if (y > NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_PT_LEFTUP;
				speed /= 15;
			}
			else if (y >= -NF_HID_PTZ_STOP_VAL && y <= NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_PAN_LEFT;
				speed /= 11;
			}
			else
			{
				pcmd->cmd = NF_PTZ_CMD_PT_LEFTDOWN;
				speed /= 15;
			}
		}
		else if (x >= -NF_HID_PTZ_STOP_VAL && x <= NF_HID_PTZ_STOP_VAL)
		{
			if (y > NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_TILT_UP;
				speed /= 11;
			}
			else if (y < -NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_TILT_DOWN;
				speed /= 11;
			}
		}
		else
		{
			if (y > NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_PT_RIGHTUP;
				speed /= 15;
			}
			else if (y >= -NF_HID_PTZ_STOP_VAL && y <= NF_HID_PTZ_STOP_VAL)
			{
				pcmd->cmd = NF_PTZ_CMD_PAN_RIGHT;
				speed /= 11;
			}
			else
			{
				pcmd->cmd = NF_PTZ_CMD_PT_RIGHTDOWN;
				speed /= 15;
			}
		}
	}

	speed += 1;
	switch ((gint)speed)
	{
		case 0 :	
		case 1 :	speed = 10;		break;
		case 2 :	speed = 15;		break;
		case 3 :	speed = 20;		break;
		case 4 :	speed = 25;		break;
		case 5 :	speed = 30;		break;
		case 6 : 	speed = 40;		break;
		case 7 :	speed = 50;		break;
		case 8 :	speed = 60;		break;
		case 9 :	speed = 70;		break;
		default :	speed = 80;//speed = 100;	break;
	}
	
	pcmd->ch = (gint) ch;
	pcmd->params[0] = speed;
}

static void nf_hid_set_prev_cmd(NF_PREV_PTZ_CMD *prev_cmd, NF_PTZ_CMD *cur_cmd)
{
	prev_cmd->ptz_cmd = cur_cmd->cmd;
	prev_cmd->ptz_speed = cur_cmd->params[0];
}



static gchar _btn_num = -1;

static void _nf_hid_get_btn_num(void)
{
	gchar i;

	if (_btn_num == -1)
	{
		for(i = 3; i < HID_MAX_BUTTON_CNT ; i++)
		{	
			if (input_rpt[i].data == 1 && button[i].press_flag[2] == 0)		
			{
				_btn_num = i;
				button[_btn_num].press_flag[2] = 1; // start
				break;
			}
		}
	}
	else
	{
		if (button[_btn_num].repeat_flag[2] == 0)
		{
			if(input_rpt[_btn_num].data == 1 && button[_btn_num].press_flag[2] == 1)		
			{
				button[_btn_num].repeat_flag[2] = 1; // pressing
			}
		}

		if (button[_btn_num].event_flag[2] == 0)
		{
			if(input_rpt[_btn_num].data == 0 && button[_btn_num].press_flag[2] == 1)		
			{
				button[_btn_num].press_flag[2] = 0; // end
				button[_btn_num].repeat_flag[2] = 0;
				button[_btn_num].event_flag[2] = 1;
			}
		}
		else if (button[_btn_num].event_flag[2] == 2)
		{
			button[_btn_num].event_flag[2] = 0;
			_btn_num = -1;
		}
	}
}

gint get_hid_dtype()
{
	gint ret;

	ret = vsm_get_div();

	switch(ret)
	{
		case 0 : ret = 1; break;
		case 1 : ret = 4; break;
		case 2 : ret = 6; break;
		case 3 : ret = 8; break;
		case 4 : ret = 9; break;
		case 5 : ret = 16; break;
		case 6 : ret = 32; break;
		default : ret = -1; break;
	}

	return ret;
}

#define NF_HID_REPEAT_CNT	3
static gchar _nf_hid_set_ptz_ch(guint set_status)
{
	static gchar prev_dtype = -1;
	static gint ptz_repeat_cnt = 0;
	static gchar ptz_ch = -1;
	static gchar flag_repeat = -1;
	
	gint hid_win_id = -1;
	
	if (set_status == NF_DVR_STATUS_LIVE)
	{
		gchar hid_dtype = get_hid_dtype();

		if (NUM_ACTIVE_CH == 8 && hid_dtype == 9)
		{
			hid_dtype = NUM_ACTIVE_CH;
		}

		if (prev_dtype != hid_dtype)
		{
			prev_dtype = hid_dtype;
			hid_win_id = 0;
		}

		if (vsm_is_focus_win())
		{
			hid_win_id = vsm_get_focused_channel();
		}
		else
		{
			hid_win_id = 0;
			vsm_draw_focus_win(hid_win_id % hid_dtype);
		}

		if (button[_btn_num].func[2] == NF_HID_CTL_LEFT_BTN)
		{
			if (button[_btn_num].repeat_flag[2] == 1)
			{
				ptz_repeat_cnt++;
				if (ptz_repeat_cnt == NF_HID_REPEAT_CNT)
				{
					hid_win_id--;
					if (hid_win_id == -1)
					{
						hid_win_id = hid_dtype - 1;
					}

					vsm_draw_focus_win(hid_win_id % hid_dtype);
					ptz_repeat_cnt = 0;
					flag_repeat = 1;
				}
			}
			else
			{
				if (flag_repeat == 1)
				{
					flag_repeat = 0;
				}
				else
				{
					if (button[_btn_num].event_flag[2] == 1)
					{
						hid_win_id--;
						if (hid_win_id == -1)
						{
							hid_win_id = hid_dtype - 1;
						}

						vsm_draw_focus_win(hid_win_id % hid_dtype);

						ptz_repeat_cnt = 0;
						button[_btn_num].event_flag[2] == 2;
					}
				}
			}
		}
		else if (button[_btn_num].func[2] == NF_HID_CTL_RIGHT_BTN)
		{
			if (button[_btn_num].repeat_flag[2] == 1)
			{
				ptz_repeat_cnt++;
				if (ptz_repeat_cnt == NF_HID_REPEAT_CNT)
				{
					hid_win_id++;
					if (hid_win_id == hid_dtype)
					{
						hid_win_id = 0;
					}

					vsm_draw_focus_win(hid_win_id % hid_dtype);
					ptz_repeat_cnt = 0;
					flag_repeat = 1;
				}
			}
			else
			{
				if (flag_repeat == 1)
				{
					flag_repeat = 0;
				}
				else
				{
					if (button[_btn_num].event_flag[2] == 1)
					{
						hid_win_id++;

						if (hid_win_id == hid_dtype)
						{
							hid_win_id = 0;
						}
						vsm_draw_focus_win(hid_win_id % hid_dtype);

						ptz_repeat_cnt = 0;
						button[_btn_num].event_flag[2] == 2;
					}
				}
			}
		}

		if (button[_btn_num].func[2] == KEYPAD_PTZ && button[_btn_num].event_flag[2] == 1)
		{
			hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, button[_btn_num].func[2]);
			button[_btn_num].event_flag[2] == 2;
		}
		else
		{
			ptz_ch = vsm_get_focused_channel();
		}
	}
	else if (set_status == NF_DVR_STATUS_PTZ)
	{
		if (ptz_ch != VW_Live_Ptz_Main_Get_Channel())
		{
			ptz_ch = VW_Live_Ptz_Main_Get_Channel();
		}

		if (button[_btn_num].func[2] == NF_HID_CTL_LEFT_BTN)
		{
			if (button[_btn_num].repeat_flag[2] == 1)
			{
				ptz_repeat_cnt++;
				if (ptz_repeat_cnt == NF_HID_REPEAT_CNT)
				{
					ptz_ch--;
					if (ptz_ch == -1)
					{
						ptz_ch = NUM_ACTIVE_CH - 1;
					}

					hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, ptz_ch);
					ptz_repeat_cnt = 0;
					flag_repeat = 1;
				}
			}
			else
			{
				if (flag_repeat == 1)
				{
					flag_repeat = 0;
				}
				else
				{
					if (button[_btn_num].event_flag[2] == 1)
					{
						ptz_ch--;
						if (ptz_ch == -1)
						{
							ptz_ch = NUM_ACTIVE_CH - 1;
						}

						hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, ptz_ch);

						ptz_repeat_cnt = 0;
						button[_btn_num].event_flag[2] == 2;
					}
				}
			}
		}
		else if (button[_btn_num].func[2] == NF_HID_CTL_RIGHT_BTN)
		{
			if (button[_btn_num].repeat_flag[2] == 1)
			{
				ptz_repeat_cnt++;
				if (ptz_repeat_cnt == NF_HID_REPEAT_CNT)
				{
					ptz_ch++;
					if (ptz_ch == NUM_ACTIVE_CH)
					{
						ptz_ch = 0;
					}

					hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, ptz_ch);
					ptz_repeat_cnt = 0;
					flag_repeat = 1;
				}
			}
			else
			{
				if (flag_repeat == 1)
				{
					flag_repeat = 0;
				}
				else
				{
					if (button[_btn_num].event_flag[2] == 1)
					{
						ptz_ch++;
						if (ptz_ch == NUM_ACTIVE_CH)
						{
							ptz_ch = 0;
						}

						hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, ptz_ch);
						ptz_repeat_cnt = 0;
						button[_btn_num].event_flag[2] == 2;
					}
				}
			}
		}
	}

	return ptz_ch;
}



static void
hid_thread_func (NfHid *self)
{       
	g_message("%s start", __FUNCTION__);

	gint input_rpt_cnt = 0;
	gint i = 0;
	guchar buf[BUFF_SIZE] = {0,};
	int x=0, y=0;
	unsigned long event_mask = KeyPressMask | KeyReleaseMask | ButtonPress | ButtonReleaseMask | Button1MotionMask | PointerMotionMask | ExposureMask | ButtonPressMask
		| ButtonReleaseMask | ButtonMotionMask | PointerMotionHintMask;
	Display *dpy;
	Screen *screen;
	Window root_window;
	dpy = XOpenDisplay(NULL);
	screen = DefaultScreen(dpy);
	root_window = XRootWindow(dpy, screen);
	XSelectInput(dpy, root_window, event_mask);

	gchar ptz_ch = 0;

	// wait init complete
	while( _nf_hid == NULL ) g_usleep(10*1000);
	
	self->init_done = 1;

	while(self->thread_run)
	{
		if(!hid_joystick_connected_flag)
		{
			if(hid_mode == 1)
			{
				nfui_hid_change_cursor_send_signal(NF_CURSOR_ARROW, 0);
			}
			hid_mode = 0;

			sleep(1);
			continue;
		}
		
		input_rpt_cnt = hid_rpt_parse(&_nf_hid->handle, buf, BUFF_SIZE);
		
		guint set_status = nf_notify_get_param0("dvr_status");
		
		_nf_hid_get_btn_num();

		if (button[_btn_num].func[2] == NF_HID_CTL_CHANGE_MODE && button[_btn_num].event_flag[2] == 1)
		{
			hid_mode++;
			if (hid_mode == 2)
			{
				hid_mode = 0;
			}
			button[_btn_num].event_flag[2] = 2;
			if(hid_mode == 0)		// NVR
			{
				nfui_hid_change_cursor_send_signal(NF_CURSOR_ARROW, 0);
			}
			else			// PTZ
			{
				nfui_hid_change_cursor_send_signal(NF_CURSOR_FLEUR, 0);
			}
		}
	
		if (hid_mode == 0)		// NVR Set Mode(Mouse Cursor)
		{
			nf_hid_nvr_set_control(input_rpt_cnt, &x, &y, dpy, screen, root_window);
			g_usleep( 1000 * 10);
		}
		else if (hid_mode == 1)		// PTZ Control Mode (FLUER Cursor)
		{	
			ptz_ch = _nf_hid_set_ptz_ch(set_status);
			
			if (set_status == NF_DVR_STATUS_LIVE || set_status == NF_DVR_STATUS_PTZ)
				nf_hid_ptz_control(input_rpt_cnt, ptz_ch);

			g_usleep( HID_PTZ_DELAY);
		}

		if (button[_btn_num].event_flag[2] == 1)
		{
			button[_btn_num].event_flag[2] = 2;
		}
	}
	g_message("%s end", __FUNCTION__);
}

void nf_hid_nvr_set_control(gint input_rpt_cnt, int *x, int *y, Display *dpy, Screen *screen, Window root_window)
{
	static int before_mouse_x, before_mouse_y;
	int mouse_diff=5;
	gint i = 0;
	gint raw_data = 0;
	gint xy_coordinate[2] = {0,};

	gint data_max = 0;
	gint data_min = 0;
	guint cmd_idx=0;

	XEvent event;
	memset(&event, 0x00, sizeof(event));

	for(i = 0; i < input_rpt_cnt; i++)
	{
		raw_data = input_rpt[i].data;
		data_max = input_rpt[i].max;
		data_min = input_rpt[i].min;
	
		switch(input_rpt[i].id)
		{
			case(X):
				if(data_max - data_min == 0)
					return;
				xy_coordinate[0] = -100 + (fabs(((gdouble)raw_data - data_min) / (data_max - data_min)) * 200);
				break;
			case(Y):
				if(data_max - data_min == 0)
					return;
				xy_coordinate[1] = (-100 + (fabs(((gdouble)raw_data - data_min) / (data_max - data_min)) * 200)) * (-1);
				break;
			default:
				break;
		}
	}

	// Mouse curruent position query
	XQueryPointer(dpy, XRootWindow(dpy, screen), &event.xbutton.root, &event.xbutton.window, x, y, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

	if((pow((gdouble)xy_coordinate[0], 2.0) + pow((gdouble)xy_coordinate[1], 2.0)) <= 64)
	{}
	else
	{
		*x += xy_coordinate[0] / 3;
		*y -= xy_coordinate[1] / 3;
	
		if(*x < 0)
			*x = 0;
		if(*y < 0)
			*y = 0;
		if(*x > DISPLAY_ACTIVE_WIDTH)
			*x = DISPLAY_ACTIVE_WIDTH;
		if(*y > DISPLAY_ACTIVE_HEIGHT)
			*y = DISPLAY_ACTIVE_HEIGHT;
	}
	// Mouse Position Update
	if(*x != before_mouse_x || *y != before_mouse_y)
	{		
		XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, *x, *y);
		XFlush(dpy);
	}
	before_mouse_x = *x;
	before_mouse_y = *y;

	//Button
	for(i=2;i<HID_MAX_BUTTON_CNT;i++)
	{
		if(button[i].func[0] == KEYPAD_NONE)
			continue;
		
		if(input_rpt[i].data == 1 && button[i].press_flag[0] == 0)		// button click
		{
			if(button[i].func[0] == HID_MOUSE_LEFT_BUTTON){
				mouse_click_event(dpy, screen, root_window, Button1, 1);
			}
			else if(button[i].func[0] == HID_MOUSE_RIGHT_BUTTON){
				mouse_click_event(dpy, screen, root_window, Button3, 1);
			}
			else if(button[i].func[0] != -100)
			{
				cmd_idx = 0;		
				hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, button[i].func[cmd_idx]);
				
			}
			button[i].press_flag[0] = 1;
		}
		if(input_rpt[i].data == 0 && button[i].press_flag[0] == 1)			// button release
		{
			if(button[i].func[0] == HID_MOUSE_LEFT_BUTTON){
				mouse_click_event(dpy, screen, root_window, Button1, 0);
			}
			else if(button[i].func[0] == HID_MOUSE_RIGHT_BUTTON){
				mouse_click_event(dpy, screen, root_window, Button3, 0);
			}
			else if(button[i].func[0] != -100)
			{
				cmd_idx = 0;				
				hid_remote_control_send_signal(NFEVENT_KEYPAD_RELEASE, button[i].func[cmd_idx]);
				
			}		
			button[i].press_flag[0] = 0;
		}
		
		if(button[i].func[0] == KEYPAD_ENTER)
		{
			if(input_rpt[i].data == 0)
				continue;
			if(input_rpt[i].data >= 700 && button[i].press_flag[0] == 0)
			{
				hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, button[i].func[0]);
				button[i].press_flag[0] = 1;
			}
			
			if(input_rpt[i].data <= 300 && button[i].press_flag[1] == 0)
			{
				hid_remote_control_send_signal(NFEVENT_KEYPAD_PRESS, button[i].func[1]);
				button[i].press_flag[1] = 1;
			}
			
			if(input_rpt[i].data < 515 && input_rpt[i].data > 500)		// Release Button
			{
				if(button[i].press_flag[0] == 1)
				{
					hid_remote_control_send_signal(NFEVENT_KEYPAD_RELEASE, button[i].func[0]);
					button[i].press_flag[0] = 0;
				}
				
				if(button[i].press_flag[1] == 1)
				{
					hid_remote_control_send_signal(NFEVENT_KEYPAD_RELEASE, button[i].func[1]);
					button[i].press_flag[1] = 0;
				}
			}
		}
	}
}

void nf_hid_ptz_control(gint input_rpt_cnt, gchar ch)
{
	gint i = 0;
	gint raw_data = 0;
	gint xy_coordinate[2] = {0,};
	gint zoom_data = 0;
	gint data_max = 0;
	gint data_min = 0;
	gint data_div = 0;

	static NF_PREV_PTZ_CMD	prev_ptz_pt_cmd;
	static NF_PREV_PTZ_CMD	prev_ptz_z_cmd;

	NF_PTZ_CMD hid_ptz_pt_ctl;
	NF_PTZ_CMD hid_ptz_z_ctl;
	NF_PTZ_CMD hid_ptz_btn_ctl;

	static gint flag_z = 0;
	static gint flag_delay = 0;
	
	memset(&hid_ptz_pt_ctl, 0x0, sizeof(NF_PTZ_CMD));
	memset(&hid_ptz_z_ctl, 0x0, sizeof(NF_PTZ_CMD));
	memset(&hid_ptz_btn_ctl, 0x0, sizeof(NF_PTZ_CMD));
	
	for(i = 0; i < input_rpt_cnt; i++)
	{
		raw_data = input_rpt[i].data;
		data_max = input_rpt[i].max;
		data_min = input_rpt[i].min;
		
		switch(input_rpt[i].id)
		{
			case(X):

				if(data_max - data_min == 0)
				{
					return;
				}
				xy_coordinate[0] = -100 + (fabs(((gdouble)(raw_data - data_min)) / (data_max - data_min)) * 200);
				break;

			case(Y):

				if(data_max - data_min == 0)
				{
					return;
				}
				xy_coordinate[1] = (-100 + (fabs(((gdouble)raw_data - data_min) / (data_max - data_min)) * 200)) * (-1);

				if (abs(xy_coordinate[0]) < 10 && abs(xy_coordinate[1]) < 10)
				{
					nf_hid_set_ptz_stop(&hid_ptz_pt_ctl, ch);
				}
				else
				{
					nf_hid_set_pan_tilt(&hid_ptz_pt_ctl, xy_coordinate[0], xy_coordinate[1], ch);
				}
				break;

			case(Z):

				if(data_max - data_min == 0)
				{
					return;
				}
				zoom_data =	-100 + (fabs(((gdouble)raw_data - data_min) / (data_max - data_min)) * 200);	

				if (abs(zoom_data) > 25)
				{
					nf_hid_set_zoom(&hid_ptz_z_ctl, zoom_data, ch);
					flag_z = 1;
				}
				else
				{
					nf_hid_set_zoom_stop(&hid_ptz_z_ctl, ch);
					flag_z = 0;
				}
				break;

			default:
				break;

		}
	}

	if (_btn_num != -1)
	{
		switch(button[_btn_num].func[2])
		{
			case(NF_HID_CTL_FOCUS_NEAR):
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_FOCUS_NEAR;
				break;
			case(NF_HID_CTL_FOCUS_FAR):
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_FOCUS_FAR;
				break;
			case(NF_HID_CTL_IRIS_CLOSE):
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_IRIS_CLOSE;
				break;
			case(NF_HID_CTL_IRIS_OPEN):
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_IRIS_OPEN;
				break;
			case(NF_HID_CTL_HOME):
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_GOTO_PRESET;
				hid_ptz_btn_ctl.params[0] = 241;
				break;
			case(NF_HID_CTL_ONE_PUSH):				
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_GOTO_PRESET;
				hid_ptz_btn_ctl.params[0] = 240;
				break;
			default:
				hid_ptz_btn_ctl.cmd = NF_PTZ_CMD_STOP;
				break;
		}
		
		if (button[_btn_num].event_flag[2] == 1)
		{
			hid_ptz_btn_ctl.ch = ch;
			nf_ptz_cmd(&hid_ptz_btn_ctl);
			
			button[_btn_num].event_flag[2] = 2;
		}
	}
		
	if (prev_ptz_z_cmd.ptz_cmd != hid_ptz_z_ctl.cmd)
	{
		nf_ptz_cmd(&hid_ptz_z_ctl);
		nf_hid_set_prev_cmd(&prev_ptz_z_cmd, &hid_ptz_z_ctl);
		flag_delay = 1;
	}

	if (prev_ptz_pt_cmd.ptz_cmd != hid_ptz_pt_ctl.cmd || prev_ptz_pt_cmd.ptz_speed != hid_ptz_pt_ctl.params[0])
	{
		if (flag_delay)
		{
			g_usleep ( HID_PTZ_DELAY );
			flag_delay = 0;
		}
		
		nf_ptz_cmd(&hid_ptz_pt_ctl);
		nf_hid_set_prev_cmd(&prev_ptz_pt_cmd, &hid_ptz_pt_ctl);

		if (flag_z && hid_ptz_pt_ctl.cmd == NF_PTZ_CMD_STOP)
		{
			nf_ptz_cmd(&hid_ptz_pt_ctl);
			g_usleep( HID_PTZ_DELAY );
			nf_ptz_cmd(&hid_ptz_z_ctl);
		}
	}
}

void hid_print_input_report(gint input_rpt_cnt)
{
	gint i = 0;

	for(i =0; i < input_rpt_cnt; i++)
	{
		if(input_rpt[i].id & X)
			g_message("\tX : %5d", input_rpt[i].data);
		else if(input_rpt[i].id & Y)
			g_message("\tY : %5d", input_rpt[i].data);
		else if(input_rpt[i].id & Z)
			g_message("\tZ : %5d", input_rpt[i].data);
		else if(input_rpt[i].id & WHEEL)
			g_message("\tWHEEL : %5d", input_rpt[i].data);
		else if(input_rpt[i].id & BUTTON)
			g_message("\tBUTTON : %5d", input_rpt[i].data);
		else if(input_rpt[i].id & HATSWITCH)
			g_message("\tHATSWITCH : %5d", input_rpt[i].data);
	}
}

gint hid_rpt_parse(hid_device* handle, unsigned char* buf, gint buf_size)
{
	gint res = 0;
	gint i = 0, j, k;
	gint tmp_data = 0, buf_data = 0;
	gint buf_cnt = 0;
	gint bit_mask_cnt = 0;
	gint usage_cnt = 0;
	gint input_rpt_desc_cnt = 0;
	gint input_rpt_cnt = 0;
	gint total_size = 0;
	
	res = hid_read(handle, buf, buf_size);
	if (res < 0)
	{
		g_warning("Error hid_read");
		return ;
	}
	
	total_size = res * BYTE_SIZE;

	// Report parsing
	for(i = 0; i < total_size; i++)
	{
		if((i % BYTE_SIZE) == 0)
		{
			tmp_data = 0;
			tmp_data = buf[buf_cnt];
		}

		buf_data |= ((tmp_data >> (i % BYTE_SIZE)) & 0x01) << bit_mask_cnt;
		bit_mask_cnt++;

		if(bit_mask_cnt >= input_rpt_desc[input_rpt_desc_cnt].rpt_size)
		{
			bit_mask_cnt = 0;
			usage_cnt++;
			hid_data_sign_handler(input_rpt_desc[input_rpt_desc_cnt].rpt_size, &buf_data);
			input_rpt[input_rpt_cnt].data = buf_data;
			input_rpt[input_rpt_cnt].max = input_rpt_desc[input_rpt_desc_cnt].logic_max;
			input_rpt[input_rpt_cnt].min = input_rpt_desc[input_rpt_desc_cnt].logic_min;
			buf_data = 0;
			input_rpt_cnt++;
		}

		if(usage_cnt >= input_rpt_desc[input_rpt_desc_cnt].rpt_cnt)
		{
			bit_mask_cnt = 0;
			buf_data = 0;
			usage_cnt = 0;
			input_rpt_desc_cnt++;
		}

		if((i % BYTE_SIZE) == 7)
			buf_cnt++;
	}
/*
	// Raw data For Debug
		for(i = 0; i < res; i++)
		{
			g_message("%4x", buf[res-1-i]);
		}
	// For Debug
		hid_print_input_report(input_rpt_cnt);


	// For Debug
		for(i = 0; i < input_rpt_cnt; i++)
		{
			g_message("\t** Input Report %d **", i+1);
			g_message("\tid = 0x%x", input_rpt[i].id);
			g_message("\tdata = %d", input_rpt[i].data);
		}

	// For Debug
		for(i = 0; i < input_rpt_desc_cnt; i++)
		{
			g_message("\t** Input Report Descriptor %d **", i+1);
			g_message("\tid = 0x%x", input_rpt_desc[i].id);
			g_message("\tx = %d", input_rpt_desc[i].x);
			g_message("\ty = %d", input_rpt_desc[i].y);
			g_message("\tz = %d", input_rpt_desc[i].z);
			g_message("\twheel = %d", input_rpt_desc[i].wheel);
			g_message("\tbutton = %d", input_rpt_desc[i].button);
			g_message("\thatswitch = %d", input_rpt_desc[i].hatswitch);
			g_message("\tlogic_max = %d", input_rpt_desc[i].logic_max);
			g_message("\tlogic_min = %d", input_rpt_desc[i].logic_min);
			g_message("\tphy_max = %d", input_rpt_desc[i].phy_max);
			g_message("\tphy_min = %d", input_rpt_desc[i].phy_min);
			g_message("\tusage_max = %d", input_rpt_desc[i].usage_max);
			g_message("\tusage_min = %d", input_rpt_desc[i].usage_min);
			g_message("\trpt_size = %d", input_rpt_desc[i].rpt_size);
			g_message("\trpt_cnt = %d", input_rpt_desc[i].rpt_cnt);
		}
*/
	return input_rpt_cnt;
}

void hid_rpt_desc_parse(hid_rpt_desc_item* rpt_desc, gint rpt_desc_item_cnt, gint* input_rpt_desc_cnt, gint* data_sequence, gint* input_rpt_cnt)
{
	gint i = 0;

	for(i = 0; i < rpt_desc_item_cnt; i++)
	{
		hid_prefix_parse(&rpt_desc[i], input_rpt_desc_cnt, data_sequence, input_rpt_cnt);
		g_message("%5x%10x", rpt_desc[i].prefix, rpt_desc[i].data);
	}

	for(i = 0; i < (*input_rpt_desc_cnt); i++)
	{
		g_message("%2d : ", i+1);
		if(input_rpt_desc[i].id & X)
			g_message("X ");
		if(input_rpt_desc[i].id & Y)
			g_message("Y ");
		if(input_rpt_desc[i].id & Z)
			g_message("Z ");
		if(input_rpt_desc[i].id & WHEEL)
			g_message("WHEEL ");
		if(input_rpt_desc[i].id  & BUTTON)
			g_message("BUTTON ");
		if(input_rpt_desc[i].id  & HATSWITCH)
			g_message("HATSWITCH ");
		if(input_rpt_desc[i].id == 0)
			g_message("Empty");
		
		if(input_rpt_desc[i].usage_max != 0)
			g_message("\tUsage Count (%d ~ %d)", input_rpt_desc[i].usage_min, input_rpt_desc[i].usage_max);
		if(input_rpt_desc[i].logic_max != 0)
			g_message("\tLogical Range (%d ~ %d)", input_rpt_desc[i].logic_min, input_rpt_desc[i].logic_max);
		if(input_rpt_desc[i].phy_max != 0)
			g_message("\tPhysical Range (%d ~ %d)", input_rpt_desc[i].phy_min, input_rpt_desc[i].phy_max);
		if(input_rpt_desc[i].rpt_size != 0)
			g_message("\tReport Size (%d)", input_rpt_desc[i].rpt_size);
		if(input_rpt_desc[i].rpt_cnt != 0)
			g_message("\tReport Count (%d)", input_rpt_desc[i].rpt_cnt);
	}
}

void hid_put_rpt_desc_item(hid_device* handle, hid_rpt_desc_item* rpt_desc, gint* rpt_desc_item_cnt)
{
	gint i =0;
	gint item_cnt = 0;
	
	while(item_cnt < handle->rpt_desc_size)
	{
		gint item_buf = 0;
		gint item_byte = 0;
		gint item_header = 0;
		gint item_size = 0;
		
		item_header = handle->rpt_desc_value[item_cnt] & 0xff;
		item_size = item_header & 0x03;

		if(item_size == DATA_SIZE_4)
			item_size = 4;

		for(i = 0; i < item_size; i++)
		{
			item_byte = 0;
			
			item_byte = handle->rpt_desc_value[item_cnt+i+1] & 0xff;
			item_byte = item_byte << (BYTE_SIZE * i);
			
			item_buf |= item_byte;
		}

		rpt_desc[*rpt_desc_item_cnt].prefix = item_header;
		rpt_desc[*rpt_desc_item_cnt].size = item_size;
		rpt_desc[*rpt_desc_item_cnt].data = item_buf;
		(*rpt_desc_item_cnt)++;

		item_cnt += item_size + 1;
	}
}

void hid_prefix_parse(hid_rpt_desc_item* rpt_desc, gint* input_rpt_desc_cnt, gint* data_sequence, gint* input_rpt_cnt)
{
	gint item_Prefix = 0;
	gint item_Size = 0;
	gint item_Data = 0;
	gint i = 0, j = 0;
	gint usage_size = 0;
	gint tmp_seq = 0;
	gint tmp_index = 0;

	item_Prefix = rpt_desc->prefix & 0xff;
	item_Size = rpt_desc->size;
	item_Data = rpt_desc->data;

	if((input_rpt_desc[*input_rpt_desc_cnt].usage_max == 0)
		&& (input_rpt_desc[*input_rpt_desc_cnt].usage_min == 0))
		usage_size = 1;
	else
		usage_size = input_rpt_desc[*input_rpt_desc_cnt].usage_max - input_rpt_desc[*input_rpt_desc_cnt].usage_min + 1;

	switch(item_Prefix & TYPE_MASK)
	{
		case (TYPE_MAIN):
			switch(item_Prefix & TAG_MASK)
			{
				case (TAG_MAIN_COLLECTION):
					g_message("%-15s", "Collection");
					break;
				case (TAG_MAIN_ENDCOLLECTION):
					g_message("%-30s", "End Collection");
					break;
				case (TAG_MAIN_INPUT):
					g_message("%-30s", "Input");
					for(i = 0; i < USAGE_TYPE_MAX; i++)
					{
						if(*((&input_rpt_desc[*input_rpt_desc_cnt].x) + i) != 0)
						{
							tmp_seq = *((&input_rpt_desc[*input_rpt_desc_cnt].x) + i);
							for(j = 0; j < usage_size; j++)
							{
								tmp_index = (*input_rpt_cnt) + (tmp_seq - 1) + (j * (*data_sequence));
								input_rpt[tmp_index].id |= (0x01 << i);
							}
						}
					}
					
					if(input_rpt_desc[*input_rpt_desc_cnt].id == 0)
						(*data_sequence)++;
					
					*input_rpt_cnt += (usage_size * (*data_sequence));
					*data_sequence = 0;
					(*input_rpt_desc_cnt)++;
					break;
				case (TAG_MAIN_OUTPUT):
					g_message("%-30s", "Output");
					break;
				case (TAG_MAIN_FEATURE):
					g_message("%-30s", "Feature");
					break;
			}
			break;
		case (TYPE_GLOBAL):
			switch(item_Prefix & TAG_MASK)
			{
				case (TAG_GLOBAL_USAGEPAGE):
					g_message("%-15s", "Usage Page");
					hid_item_usage_page(item_Data, input_rpt_desc_cnt, data_sequence);
					break;
				case (TAG_GLOBAL_LOGICALMIN):
					g_message("%-15s", "Logical Min");
					hid_data_sign_handler(item_Size * BYTE_SIZE, &item_Data);
					input_rpt_desc[*input_rpt_desc_cnt].logic_min = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].logic_min);
					break;
				case (TAG_GLOBAL_LOGICALMAX):
					g_message("%-15s", "Logical Max");
					hid_data_sign_handler(item_Size * BYTE_SIZE, &item_Data);
					input_rpt_desc[*input_rpt_desc_cnt].logic_max = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].logic_max);
					break;
				case (TAG_GLOBAL_PHYSMIN):
					g_message("%-15s", "Physical Min");
					hid_data_sign_handler(item_Size * BYTE_SIZE, &item_Data);
					input_rpt_desc[*input_rpt_desc_cnt].phy_min = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].phy_min);
					break;
				case (TAG_GLOBAL_PHYSMAX):
					g_message("%-15s", "Physical Max");
					hid_data_sign_handler(item_Size * BYTE_SIZE, &item_Data);
					input_rpt_desc[*input_rpt_desc_cnt].phy_max = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].phy_max);
					break;
				case (TAG_GLOBAL_REPORTSIZE):
					g_message("%-15s", "Report Size");
					input_rpt_desc[*input_rpt_desc_cnt].rpt_size = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].rpt_size);
					break;
				case (TAG_GLOBAL_REPORTCOUNT):
					g_message("%-15s", "Report Count");
					input_rpt_desc[*input_rpt_desc_cnt].rpt_cnt = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].rpt_cnt);
					break;
				case (TAG_GLOBAL_PUSH):
					g_message("%-15s", "Push");
					break;
				case (TAG_GLOBAL_POP):
					g_message("%-15s", "Pop");
					break;
				case (TAG_GLOBAL_UNITEXP):
					g_message("%-15s", "Unit Exp");
					break;
				case (TAG_GLOBAL_UNIT):
					g_message("%-15s", "Unit");
					break;
				case (TAG_GLOBAL_REPORTID):
					g_message("%-15s", "Report Id");
					break;
			}
			break;
		case (TYPE_LOCAL):
			switch(item_Prefix & TAG_MASK)
			{		
				case (TAG_LOCAL_USAGE):
					g_message("%-15s", "Usage");
					hid_item_usage(item_Data, input_rpt_desc_cnt, data_sequence);
					break;
				case (TAG_LOCAL_USAGEMIN):
					g_message("%-15s", "Usage Min");
					input_rpt_desc[*input_rpt_desc_cnt].usage_min = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].usage_min);
					break;
				case (TAG_LOCAL_USAGEMAX):
					g_message("%-15s", "Usage Max");
					input_rpt_desc[*input_rpt_desc_cnt].usage_max = item_Data;
					g_message("%-15d", input_rpt_desc[*input_rpt_desc_cnt].usage_max);
					break;
			}
			break;
		default:
			g_warning("prefix error");
			break;
	}
}

void hid_item_usage_page(gint data, gint* input_rpt_desc_cnt, gint* data_sequence)
{
	switch(data & 0xff)
	{
		case (USAGE_PAGE_UNDEFINED):
			g_message("%-15s", "Undefined");
			break;
		case (USAGE_PAGE_GENERIC_DESKTOP):
			g_message("%-15s", "Generic Desktop");
			break;
		case (USAGE_PAGE_SIMULATION):
			g_message("%-15s", "Simulation");
			break;
		case (USAGE_PAGE_VR):
			g_message("%-15s", "VR");
			break;
		case (USAGE_PAGE_SPORT):
			g_message("%-15s", "Spor ");
			break;
		case (USAGE_PAGE_GENERIC_DEVICE):
			g_message("%-15s", "Generic_Device");
			break;
		case (USAGE_PAGE_KEBOARD_KEYPAD):
			g_message("%-15s", "Keyboard_Keypad");
			break;
		case (USAGE_PAGE_LED):
			g_message("%-15s", "LED");
			break;
		case (USAGE_PAGE_BUTTON):
			g_message("%-15s", "Button");
			input_rpt_desc[*input_rpt_desc_cnt].id |= BUTTON;
			input_rpt_desc[*input_rpt_desc_cnt].button = ++(*data_sequence);
			break;
		case (USAGE_PAGE_ORDINAL):
			g_message("%-15s", "Ordinal");
			break;
		case (USAGE_PAGE_TELEPHONY):
			g_message("%-15s", "Telephony");
			break;
		case (USAGE_PAGE_CONSUMER):
			g_message("%-15s", "Consumer");
			break;
		case (USAGE_PAGE_DIGITIZER):
			g_message("%-15s", "Digitizer");
			break;
		default:
			g_warning("prefix error");
			break;
	}
}

void hid_item_usage(gint data, gint* input_rpt_desc_cnt, gint* data_sequence)
{
	switch(data & 0xff)
	{
		case (GENERIC_DESKTOP_UNDEFINED):
			g_message("%-15s", "Undefined");
			break;
		case (GENERIC_DESKTOP_POINTER):
			g_message("%-15s", "Pointer");
			break;
		case (GENERIC_DESKTOP_MOUSE):
			g_message("%-15s", "Mouse");
			break;
		case (GENERIC_DESKTOP_RESERVED):
			g_message("%-15s", "Reserved");
			break;
		case (GENERIC_DESKTOP_JOYSTICK):
			g_message("%-15s", "Joystick");
			break;
		case (GENERIC_DESKTOP_GAMEPAD):
			g_message("%-15s", "Gamepad");
			break;
		case (GENERIC_DESKTOP_KEYBOARD):
			g_message("%-15s", "Keyboard");
			break;
		case (GENERIC_DESKTOP_KEYPAD):
			g_message("%-15s", "Keypad");
			break;
		case (GENERIC_DESKTOP_MULTI_AXIS):
			g_message("%-15s", "Multi_Axis");
			break;
		case (GENERIC_DESKTOP_X):
			g_message("%-15s", "X");
			input_rpt_desc[*input_rpt_desc_cnt].id |= X;
			input_rpt_desc[*input_rpt_desc_cnt].x = ++(*data_sequence);
			break;
		case (GENERIC_DESKTOP_Y):
			g_message("%-15s", "Y");
			input_rpt_desc[*input_rpt_desc_cnt].id |= Y;
			input_rpt_desc[*input_rpt_desc_cnt].y = ++(*data_sequence);
			break;
		case (GENERIC_DESKTOP_Z):
			g_message("%-15s", "Z");
			input_rpt_desc[*input_rpt_desc_cnt].id |= Z;
			input_rpt_desc[*input_rpt_desc_cnt].z = ++(*data_sequence);
			break;
		case (GENERIC_DESKTOP_RX):
			g_message("%-15s", "Rx");
			break;
		case (GENERIC_DESKTOP_RY):
			g_message("%-15s", "Ry");
			break;
		case (GENERIC_DESKTOP_RZ):
			g_message("%-15s", "Rz");
			break;
		case (GENERIC_DESKTOP_SLIDER):
			g_message("%-15s", "Slider");
			break;
		case (GENERIC_DESKTOP_DIAL):
			g_message("%-15s", "Dial");
			break;
		case (GENERIC_DESKTOP_WHEEL):
			g_message("%-15s", "Wheel");
			input_rpt_desc[*input_rpt_desc_cnt].id |= WHEEL;
			input_rpt_desc[*input_rpt_desc_cnt].wheel = ++(*data_sequence);
			break;
		case (GENERIC_DESKTOP_HATSWITCH):
			g_message("%-15s", "Hatswitch");
			input_rpt_desc[*input_rpt_desc_cnt].id |= HATSWITCH;
			input_rpt_desc[*input_rpt_desc_cnt].hatswitch = ++(*data_sequence);
			break;
		default:
			g_warning("prefix error");
			break;
	}
}

void hid_data_sign_handler(gint item_size_bit, gint* item_buf)
{
	gint i = 0;

	if(item_size_bit > 1)
		if((*item_buf) & (0x01 << (item_size_bit - 1)))
			for(i = item_size_bit; i < INT_SIZE; i++)
				*item_buf |= (0x01 << i);
}

#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)
gboolean nf_hid_get_dev_info()
{
	char device_info[1024]={0,};
	char *path=NULL;
	char bInterfaceProtocol_cmd[128] = {"/sys"};
	char *vendor=NULL;
	char *product=NULL;
	char *dev_numer_str=NULL;
	char vendor_num[5]={0,};
	char product_num[5]={0,};
	char protocol_path[20]={"/device/bInterfaceProtocol"};
	char dev_num;
	int i=0;
	int idx=0;
	static int prev_hid_connected_flag=0;
	
	FILE *read_fp = NULL;
	FILE *device_parse=NULL;
	
	int jflag=0;
	
	read_fp = fopen("/proc/bus/input/devices", "r");
	
	if (read_fp)
	{
		while(fgets(device_info, sizeof(device_info), read_fp) != NULL)
		{
			if(vendor = strstr(device_info ,"Vendor="))
			{
				for(i=0;i<4;i++)
				{
					vendor_num[i] = vendor[i+7];
				}
			}
			if(product = strstr(device_info ,"Product="))
			{
				for(i=0;i<4;i++)
				{
					product_num[i] = product[i+8];
				}
			}
			if(dev_numer_str = strstr(device_info ,"Handlers="))
			{
				dev_numer_str = strstr(dev_numer_str, "event");
				dev_num = dev_numer_str[5];
				if(jflag)
				{
					break;
				}
			}
			
			if(strstr(device_info ,"Sysfs"))
			{
				path = strstr(device_info, "/");

				if(path)
				{
					idx = strlen(path);

					dev_num = path[idx-2];
					path[idx-1] = '\0';
					strcat(bInterfaceProtocol_cmd, path);

					strcat(bInterfaceProtocol_cmd, "/device/bInterfaceProtocol");
				}

				if(device_parse = fopen(bInterfaceProtocol_cmd, "r"))
				{
					if(fgets(device_info, sizeof(device_info), device_parse))
					{
						strcpy(bInterfaceProtocol_cmd, "/sys");

						if(strncmp(device_info, "00", 2) == 0)
						{
							jflag = 1;
						}
						else
						{
							jflag = 0;
						}
					}
					fclose(device_parse);
				}
			}
	 		strcpy(bInterfaceProtocol_cmd, "/sys");			
		}
		fclose(read_fp);
	}

	hid_joystick_connected_flag = jflag;
	
	product_id = strtol(product_num, NULL, 16);
	dev_number = dev_num;
	vendor_id = strtol(vendor_num, NULL, 16);

	if(hid_joystick_connected_flag)
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
			printf("\e[31m ########################### \e[0m \n");
			printf("\e[31m #### JOYSTICK FIND[%c] #### \e[0m\n", dev_num);
			hid_mouse_flag = 1;
			nf_hid_initialize(1);
		}
	}
	else
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
			close(_nf_hid->handle.device_handle);
			hid_mouse_flag = 0;
		}
	}
	prev_hid_connected_flag = hid_joystick_connected_flag;
	return TRUE;
}

#elif defined(_IPX_1648P4E)|| defined(_IPX_0824P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
gboolean nf_hid_get_dev_info(void)
{
    char device_info[1024] = {0};
    const size_t BUF_SIZE = sizeof(device_info);
    char bInterfaceProtocol_cmd[256] = "/sys";
    char vendor_num[5] = {0};
    char product_num[5] = {0};
	char *path=NULL;
	char *vendor=NULL;
	char *product=NULL;
	char *dev_numer_str=NULL;
    char dev_num = '\0';
    int jflag = 0;
	int i=0;
	int idx=0;
	static int prev_hid_connected_flag=0;
	
	FILE *read_fp = NULL;
	FILE *device_parse=NULL;
	
    read_fp = fopen("/proc/bus/input/devices", "r");
    if (read_fp == NULL) {
        perror("Failed to open /proc/bus/input/devices");
        return FALSE;
    }
	
    memset(vendor_num, 0, sizeof(vendor_num));
    memset(product_num, 0, sizeof(product_num));
	
    while (fgets(device_info, BUF_SIZE, read_fp) != NULL)
		{
        vendor = strstr(device_info, "Vendor=");
        if (vendor != NULL)
			{
            strncpy(vendor_num, vendor + 7, 4);
            vendor_num[4] = '\0';
				}

        product = strstr(device_info, "Product=");
        if (product != NULL)
			{
            strncpy(product_num, product + 8, 4);
            product_num[4] = '\0';
				}

        dev_numer_str = strstr(device_info, "Handlers=");
        if (dev_numer_str != NULL)
			{
				dev_numer_str = strstr(dev_numer_str, "event");
            if (dev_numer_str != NULL && strlen(dev_numer_str) > 5)
            {
				dev_num = dev_numer_str[5];
            }

				if(jflag)
				{
					break;
				}
			}
			
			if(strstr(device_info ,"Sysfs"))
			{
				path = strstr(device_info, "/");

            if (path != NULL)
            {
                idx = (int)strlen(path);
                if (idx < 2)
                {
                    continue;
                }

                if (path[idx - 1] == '\n' || path[idx - 1] == '\r')
				{
                    ((char *)path)[idx - 1] = '\0';
                    idx--;
                }

					dev_num = path[idx-2];

                int n = snprintf(bInterfaceProtocol_cmd, sizeof(bInterfaceProtocol_cmd),
                                 "/sys%s/device/bInterfaceProtocol", path);

                if (n < 0 || n >= (int)sizeof(bInterfaceProtocol_cmd))
                {
                    fprintf(stderr, "Error: path string too long\n");
                    continue;
                }
				}

            device_parse = fopen(bInterfaceProtocol_cmd, "r");
            if (device_parse != NULL)
				{
                if (fgets(device_info, BUF_SIZE, device_parse) != NULL)
					{
						if(strncmp(device_info, "00", 2) == 0)
						{
							jflag = 1;
						}
						else
						{
							jflag = 0;
						}
					}
					fclose(device_parse);
				}
            else
            {
            }
			}
		}

		fclose(read_fp);

    int product_id = 0, vendor_id = 0, dev_number = 0;

    product_id = (int)strtol(product_num, NULL, 16);
    vendor_id = (int)strtol(vendor_num, NULL, 16);
    dev_number = (int)dev_num;

	hid_joystick_connected_flag = jflag;
	
	if(hid_joystick_connected_flag)
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
			printf("\e[31m ########################### \e[0m \n");
			printf("\e[31m #### JOYSTICK FIND[%c] #### \e[0m\n", dev_num);
			hid_mouse_flag = 1;
			nf_hid_initialize(1);
		}
	}
	else
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
            if (_nf_hid && _nf_hid->handle.device_handle)
            {
			close(_nf_hid->handle.device_handle);
            }
			hid_mouse_flag = 0;
		}
	}
	prev_hid_connected_flag = hid_joystick_connected_flag;

	return TRUE;
}

#else
gboolean nf_hid_get_dev_info()
{
	char device_info[1024]={0,};
	char *path=NULL;
	char bInterfaceProtocol_cmd[128] = {"/sys"};
	char *vendor=NULL;
	char *product=NULL;
	char *dev_numer_str=NULL;
	char vendor_num[5]={0,};
	char product_num[5]={0,};
	char protocol_path[20]={"/device/platform"};
	char dev_num;
	int i=0;
	int idx=0;
	static int prev_hid_connected_flag=0;
	
	FILE *read_fp = NULL;
	FILE *device_parse=NULL;
	
	int jflag=0;
	
	read_fp = fopen("/proc/bus/input/devices", "r");
	
	if (read_fp)
	{
		while(fgets(device_info, sizeof(device_info), read_fp) != NULL)
		{
			if(product = strstr(device_info ,"Product="))
			{
				for(i=0;i<4;i++)
				{
					product_num[i] = product[i+8];
				}
			}
			if(!strncmp(product_num, "00ca", 4))    // for_Joystick Function 
			{
			if(vendor = strstr(device_info ,"Vendor="))
			{
				for(i=0;i<4;i++)
				{
					vendor_num[i] = vendor[i+7];
				}
			}
			if(dev_numer_str = strstr(device_info ,"Handlers="))
			{
				dev_numer_str = strstr(dev_numer_str, "event");
				dev_num = dev_numer_str[5];
				if(jflag)
				{
					break;
				}
			}
			if(path = strstr(device_info ,"Sysfs="))
			{
				path = strstr(path, "/devices");
				if(path)
				{
					idx = strlen(path);
					if(idx>86)     
					{	dev_num = path[idx-35];     
						path[idx-34] = '\0';
					}
					else
					{	dev_num = path[idx-34];     
						path[idx-33] = '\0';
					}
					strcat(bInterfaceProtocol_cmd, path);					
					strcat(bInterfaceProtocol_cmd, "bInterfaceProtocol");	
				}
				if(device_parse = fopen(bInterfaceProtocol_cmd, "r"))
				{
					if(fgets(device_info, sizeof(device_info), device_parse))
					{
						strcpy(bInterfaceProtocol_cmd, "/sys");
						if(strncmp(device_info, "00", 2) == 0)
						{
							jflag = 1;
						}
						else
						{
							jflag = 0;
						}
					}
					fclose(device_parse);
				}
			}
			}
		strcpy(bInterfaceProtocol_cmd, "/sys");
		}
		fclose(read_fp);
	}

	hid_joystick_connected_flag = jflag;
	
	product_id = strtol(product_num, NULL, 16);
	dev_number = dev_num;
	vendor_id = strtol(vendor_num, NULL, 16);

	if(hid_joystick_connected_flag)
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
			printf("\e[31m ########################### \e[0m \n");
			printf("\e[31m #### JOYSTICK FIND[%c] #### \e[0m\n", dev_num);
			hid_mouse_flag = 1;
			nf_hid_initialize(1);
		}
	}
	else
	{
		if(prev_hid_connected_flag != hid_joystick_connected_flag)
		{
			close(_nf_hid->handle.device_handle);
			hid_mouse_flag = 0;
		}
	}
	prev_hid_connected_flag = hid_joystick_connected_flag;
	return TRUE;
}

#endif 

void mouse_click_event(Display *dpy, Screen *screen, Window root_window, int button, int state)
{
    XEvent event;

    memset(&event, 0x00, sizeof(event));
	if(state)
	    event.type = ButtonPress;
	else
		event.type = ButtonRelease;
	
    event.xbutton.button = button;
    event.xbutton.same_screen = True;

    XQueryPointer(dpy, XRootWindow(dpy, screen), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	// Cursor State Read
    event.xbutton.subwindow = event.xbutton.window;

    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;

        XQueryPointer(dpy, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    }
    if(XSendEvent(dpy, PointerWindow, True, 0xfff, &event) == 0) fprintf(stderr, "Error\n");

    XFlush(dpy);
}
/*
	Button1 - button[3][0]
	Button2 - button[4][0]
	Button3 - button[5][0]
	Button4 - button[6][0]
	......................
	Button(n) - button[n+2][0]

	> button[*][0] is NVR Set Control Key Seleted
	> button[*][1] is PTZ Function Key Seleted
*/
void button_func_mapping()		// x[0], y[1], wheel[2] .... Button [3],[4]...
{
	//Wheel is defined Enter, Exit and PTZ is defined Zoom Control
	// NVR
	button[2].func[0] = KEYPAD_ENTER;
	button[2].func[1] = KEYPAD_EXIT;
	button[3].func[0] = KEYPAD_SEARCH;
	button[4].func[0] = KEYPAD_ARCH;
	button[5].func[0] = KEYPAD_SETUP;
	button[6].func[0] = RMC_ALARM;
	button[7].func[0] = KEYPAD_PTZ;
	button[8].func[0] = KEYPAD_ZOOM;
	button[9].func[0] = KEYPAD_DISP;
	button[10].func[0] = RMC_LOG;
	button[11].func[0] = KEYPAD_PANIC;
	button[12].func[0] = -100;
	button[13].func[0] = HID_MOUSE_LEFT_BUTTON;
	button[14].func[0] = HID_MOUSE_RIGHT_BUTTON;

	// PTZ
//	button[0].func[2] = NF_HID_CTL_X;
//	button[1].func[2] = NF_HID_CTL_Y;
//	button[2].func[2] = NF_HID_CTL_Z;
	button[3].func[2] = NF_HID_CTL_FOCUS_NEAR;
	button[4].func[2] = NF_HID_CTL_FOCUS_FAR;
	button[5].func[2] = NF_HID_CTL_IRIS_CLOSE;
	button[6].func[2] = NF_HID_CTL_IRIS_OPEN;
	button[7].func[2] = NF_HID_CTL_HOME;
	button[8].func[2] = NF_HID_CTL_ONE_PUSH;
	button[9].func[2] = KEYPAD_PTZ;
	button[10].func[2] = NF_HID_CTL_RESERVED1;
	button[11].func[2] = NF_HID_CTL_RESERVED2;
	button[12].func[2] = NF_HID_CTL_CHANGE_MODE;
	button[13].func[2] = NF_HID_CTL_LEFT_BTN;
	button[14].func[2] = NF_HID_CTL_RIGHT_BTN;

}
