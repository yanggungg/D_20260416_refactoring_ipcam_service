#ifndef _NF_SOLO_DISP_H_
#define _NF_SOLO_DISP_H_

#include "nf_common.h"
#include "nf_dspcomm_app.h"
#include "tw2880_api.h"
#include "spot_anf_ioctl.h"

#define DISP_MODE_NORMAL			(1<<0)
#define DISP_MODE_PIP				(1<<1)
#define DISP_MODE_PB				(1<<2)

#define _NO_DISP_VIDEO 0xff

struct ZOOM_CTRL {
	unsigned int on;
	unsigned int x;
	unsigned int y;
};

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

typedef struct _solo_disp
{
	int fd[NUM_SOLO];
	guint  	 width;
	guint  	 height;	
	struct DISP_CTRL disp_ctrl[NUM_SOLO][NUM_CHANNEL];
} SOLO_DISP;


#define IOCTL_DISP_DEV_0		"/dev/solo6110_disp0"
#define IOCTL_DISP_DEV_1		"/dev/solo6110_disp1"
#define IOCTL_DISP_DEV_2		"/dev/solo6110_disp2"
#define IOCTL_DISP_DEV_3		"/dev/solo6110_disp3"

#define IOCTL_DISP_MAGIC		'd'

#define IOCTL_DISP_ERASE_ON		_IOW(IOCTL_DISP_MAGIC, 0, int)
#define IOCTL_DISP_ERASE_OFF	_IO(IOCTL_DISP_MAGIC, 1)
#define IOCTL_DISP_CHANNEL		_IOW(IOCTL_DISP_MAGIC, 2, struct DISP_CTRL)
#define IOCTL_DISP_ZOOM			_IOW(IOCTL_DISP_MAGIC, 3, struct ZOOM_CTRL)
#define IOCTL_DISP_GET_XRES		_IOR(IOCTL_DISP_MAGIC, 11, unsigned int)
#define IOCTL_DISP_GET_YRES		_IOR(IOCTL_DISP_MAGIC, 12, unsigned int)	// field size

#define IOCTL_DISP_MAXNR		16

gboolean nf_tw2880_live_start(void);
int tw2880_live_change(tw2880_live_param_type *live_param);
void nf_tw2880_live_change( DPREQ_LIVE_CHANGE *data );
void nf_tw2880_live_stop(void);

void nf_tw2880_border(guint border, guint border_color);
void nf_tw2880_freeze( guint freeze_mask_ch );
void nf_tw2880_covert( guint covert_mask_ch );
void nf_tw2880_zoom( unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy );
void nf_tw2880_set_md (void);
int nf_tw2880_set_spot(tw2880_spot_param_type spot_param);
int nf_solo_cmd_playback( guchar start_ch, tw2880_scr_div_mode_e div_mode);
gboolean nf_tw2880_playback_start(void);
gboolean nf_tw2880_playback_change( guchar ch_arr[DSP_MAX_CHAN], tw2880_scr_div_mode_e div_mode, guchar start_ch, guchar ch_cnt);
gboolean nf_tw2880_playback_stop(void);

SOLO_DISP **solo_disp_get_address(void);
SOLO_DISP *solo_disp_get_struct(void);
int solo_disp_init(SOLO_DISP **p);

void tw2880_get_position_info(int scrsize_x, int scrsize_y, int div, TW2880_ST_RECT_WH *pRectArray);

tw2880_live_param_type *tw2880_disp_get_struct(void);
tw2880_live_param_type **tw2880_disp_get_address(void);
int tw2880_disp_init(tw2880_live_param_type **live_param);
int tw2880_config_init(void);
#endif /* _NF_SOLO_DISP_H_ */

