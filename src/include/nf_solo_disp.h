#ifndef _NF_SOLO_DISP_H_
#define _NF_SOLO_DISP_H_

#include "nf_solo_common.h"

#define IOCTL_DISP_DEV			"/dev/solo6110_disp0"

#define IOCTL_DISP_MAGIC		'd'


struct DISP_CTRL
{
	unsigned int window_id;
	unsigned int channel;
	unsigned int scale;
	unsigned int sx;
	unsigned int sy;
	unsigned int ex;
	unsigned int ey;
	unsigned int mode;
};

struct tw2865_video_picture {
	unsigned int	channel;
	unsigned int	brightness;
	unsigned int	hue;
	unsigned int	colour;
	unsigned int	contrast;
};

struct tw2865_debug_reg {
	unsigned char	dev;
	unsigned char	addr;
	unsigned char	data;
};


struct ZOOM_CTRL {
	unsigned int on;
	unsigned int x;
	unsigned int y;
};

struct RECTANGLE
{
	unsigned int id;
	unsigned int sx;
	unsigned int sy;
	unsigned int ex;
	unsigned int ey;
	unsigned int line;
	unsigned int fill;
};

struct CURSOR
{
	unsigned int x;
	unsigned int y;
};

struct CURSOR_MASK
{
	unsigned int mask[20];
};

struct BORDER
{
	unsigned int x[5];
	unsigned int y[5];
	unsigned int line_mask;
	unsigned int line_color;
};

struct MD_SENSOR_COLOR {
	char cy;
	char cb;
	char cr;
};

typedef struct _solo_disp
{
	GMutex *mode_change_lock;
	int init_done;
	int fd;	//Handle open device.
	int is_pb_mode;
	unsigned int width;
	unsigned int height;
	unsigned int mux_mode;
	unsigned int live_covert_mask;
	unsigned int pb_covert_mask;
	int zoom;
	int prev_zoom;
	unsigned char screen_num[DSP_MAX_CHAN];
	struct DISP_CTRL disp_ctrl[16];
	struct DISP_CTRL *pip_ctrl;
} SOLO_DISP;

typedef struct _solo_dec_disp_t
{
	gboolean status;
	gchar 	 disp_mode;
	guchar 	 current_ch;
	gchar 	 reserved[2];	
	guchar 	 switch_ch[DSP_MAX_CHAN];
	gint 	 num_ch;
	guint 	 channel_mask;
	gint  	 width;
	gint  	 height;
} SOLO_DEC_DISP_T;

#define _NO_DISP_VIDEO 0xff
#define _NO_COVERT_PB  0x00

#define IOCTL_DISP_ERASE_ON		_IOW(IOCTL_DISP_MAGIC, 0, int)
#define IOCTL_DISP_ERASE_OFF	_IO(IOCTL_DISP_MAGIC, 1)
#define IOCTL_DISP_CHANNEL		_IOW(IOCTL_DISP_MAGIC, 2, struct DISP_CTRL)
#define IOCTL_DISP_ZOOM			_IOW(IOCTL_DISP_MAGIC, 3, struct ZOOM_CTRL)
#define IOCTL_DISP_FREEZE_ON	_IO(IOCTL_DISP_MAGIC, 4)
#define IOCTL_DISP_FREEZE_OFF	_IO(IOCTL_DISP_MAGIC, 5)
#define IOCTL_DISP_EXPANSION	_IOW(IOCTL_DISP_MAGIC, 6, unsigned int)

#define IOCTL_DISP_BORDER		_IOW(IOCTL_DISP_MAGIC, 7, struct BORDER)
#define IOCTL_DISP_RECTANGLE	_IOW(IOCTL_DISP_MAGIC, 8, struct RECTANGLE)

#define IOCTL_DISP_MOTION_TRACE_ON	_IOW(IOCTL_DISP_MAGIC, 9, struct MD_SENSOR_COLOR)
#define IOCTL_DISP_MOTION_TRACE_OFF	_IO(IOCTL_DISP_MAGIC, 10)

#define IOCTL_DISP_GET_XRES			_IOR(IOCTL_DISP_MAGIC, 11, unsigned int)
#define IOCTL_DISP_GET_YRES			_IOR(IOCTL_DISP_MAGIC, 12, unsigned int)	// field size
#define IOCTL_DISP_COVERT			_IOR(IOCTL_DISP_MAGIC, 13, unsigned int)
#define IOCTL_DISP_COVERT_LIVE			_IO(IOCTL_DISP_MAGIC, 14)
#define IOCTL_DISP_COVERT_PLAYBACK		_IO(IOCTL_DISP_MAGIC, 15)

#define IOCTL_DISP_MAXNR			16

#define DISP_MODE_NORMAL			(1<<0)
#define DISP_MODE_PIP				(1<<1)
#define DISP_MODE_PB				(1<<2)

typedef enum _mux_mode_e
{
	MUX_MODE_1CH	= 0,
	MUX_MODE_4CH,	//1
	MUX_MODE_6CH,	//2
	MUX_MODE_8CH,	//3
	MUX_MODE_9CH,	//4
	MUX_MODE_16CH,	//5
	MUX_MODE_2CH	//6
} mux_mode_e;

#define BORDER_OFF		0
#define BORDER_ON		1
#define BORDER_PLAYBACK			0xFF
#define BORDER_COLOR_PLAYBACK	0xFF

#define IOCTL_VIN_MAGIC 'v'

struct SOLO6x10_VIN_SWITCH
{
	unsigned int src;
	unsigned int dst;
};

struct SOLO6x10_VIN_MOSAIC
{
	unsigned int channel;
	unsigned int sx;
	unsigned int ex;
	unsigned int sy;
	unsigned int ey;
};

struct SOLO6x10_VIN_MOTION_THRESHOLD
{
	unsigned int channel;
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	unsigned long ptr;
};

struct SOLO6x10_MD_AREA {
	unsigned short sensitivity;
	unsigned char reserved[2];	
	unsigned long md_area[MAX_MD_ROW]; 
};

struct SOLO6x10_SET_MD_CFG{
	struct SOLO6x10_MD_AREA cfg[NUM_CHANNEL];
}; 

#define IOCTL_VIN_SWITCH		_IOW(IOCTL_VIN_MAGIC, 0, struct SOLO6x10_VIN_SWITCH)
#define IOCTL_VIN_MOSAIC		_IOW(IOCTL_VIN_MAGIC, 1, struct SOLO6x10_VIN_MOSAIC)
#define IOCTL_VIN_MOTION_THRESHOLD	_IOW(IOCTL_VIN_MAGIC, 2, struct SOLO6x10_VIN_MOTION_THRESHOLD)
#define IOCTL_VIN_SET_MD_AREA		_IOW(IOCTL_VIN_MAGIC, 3, struct SOLO6x10_SET_MD_CFG)
#define IOCTL_VIN_MAXNR		4


int solo_disp_pos2channel(SOLO_DISP *disp, int x, int y);
void solo_disp_change_live_mode(SOLO_DISP *disp, unsigned int channel, unsigned int mode);
int solo_disp_mux_mode( SOLO_DISP *disp , unsigned int live_pb_mode );
int solo_disp_select_channel(SOLO_DISP *disp, int channel);
int solo_disp_zoom(SOLO_DISP *disp, int on, int x, int y);
int solo_disp_motion_trace(SOLO_DISP *disp, int on);

SOLO_DISP **solo_disp_get_address(void);
SOLO_DISP *solo_disp_get_struct(void);

int solo_disp_create(SOLO_DISP **p);
int solo_disp_init(SOLO_DISP **p);
int solo_disp_open(SOLO_DISP *t, char *path);
void solo_disp_close(SOLO_DISP *disp);

int nf_solo_send_live_change( DPREQ_LIVE_CHANGE *data );
int nf_solo_send_play_change( SOLO_DEC_DISP_T *data );
int solo_disp_covert(unsigned int covert_mask_ch);
#endif /* _NF_SOLO_DISP_H_ */

