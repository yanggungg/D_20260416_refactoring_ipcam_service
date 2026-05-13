#ifdef _SNF_MODEL

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "nf_api_live.h"
#include "nfdal.h"
#include "nf_tw2880_disp.h"

#define GETBITMASK(a, x)		((a) & (1L << (x)))

static tw2880_live_param_type *TW2880_LiveDisplay = NULL;
static SOLO_DISP *SoloDisplay = NULL;

const unsigned int disp_scale[] = {1, 3, 4, 5, 4, 5};
const unsigned int disp_div[] = {1, 2, 3, 4, 3, 4};


tw2880_live_param_type **tw2880_disp_get_address(void)
{
	return &TW2880_LiveDisplay;
}

tw2880_live_param_type *tw2880_disp_get_struct(void)
{
	return TW2880_LiveDisplay;
}

SOLO_DISP **solo_disp_get_address(void)
{
	return &SoloDisplay;
}

SOLO_DISP *solo_disp_get_struct(void)
{
	return SoloDisplay;
}


gboolean nf_tw2880_live_start(void)
{
	tw2880_live_param_type *live_param = tw2880_disp_get_struct();
	unsigned char i = 0;

	if (nf_tw2880_playback_stop() == FALSE) {
		g_warning("<%s><%d> nf_tw2880_playback_stop fail", __FUNCTION__, __LINE__);
	}
		
	memset(live_param->ch_arr, 0xff, sizeof(live_param->ch_arr));

	live_param->div_mode = TW2880_SCR_DIV_MODE_16;

	for( i=0; i< NUM_ACTIVE_CH; i++ )
	{
		live_param->ch_arr[i] = i;
	}
/*
	if ( itx_tw2880_live_start(*live_param) == -1 )
	{
		g_warning("Error [ itx_tw2880_live_start fail.]");
		return FALSE;		
	}
*/	
	return TRUE;
}

int tw2880_live_change(tw2880_live_param_type *live_param)
{
	int ret_val = 0;
/*
	ret_val = itx_tw2880_live_change(*live_param);
*/
	return ret_val;	
}

void nf_tw2880_live_change( DPREQ_LIVE_CHANGE *data )
{
	unsigned int ch;
	tw2880_live_param_type *live_param = tw2880_disp_get_struct();
	int ret_val = 0;

	switch(data->display_mode)
	{
		case NF_DISPLAY_FULL:
			live_param->div_mode = TW2880_SCR_DIV_MODE_1;
		break;

		case NF_DISPLAY_QUAD:
			live_param->div_mode = TW2880_SCR_DIV_MODE_4;
		break;

		case NF_DISPLAY_HEXA_A:
			live_param->div_mode = TW2880_SCR_DIV_MODE_6;
		break;

		case NF_DISPLAY_OCTA_A:
			live_param->div_mode = TW2880_SCR_DIV_MODE_8;
		break;

		case NF_DISPLAY_NONA:
			live_param->div_mode = TW2880_SCR_DIV_MODE_9;
		break;

		case NF_DISPLAY_HEXADECA:
			live_param->div_mode = TW2880_SCR_DIV_MODE_16;
		break;

		default:
			g_warning("%s unsupported live mode[%d]", __FUNCTION__, data->display_mode);
	}

	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
		live_param->ch_arr[ch] = 0xff;

	for(ch=0; ch<NUM_ACTIVE_CH; ch++) {
		if(data->screen_num[ch] != 0xff)
			live_param->ch_arr[data->screen_num[ch]] = ch;
	}

	if ( tw2880_live_change(live_param) == -1 )
	{
		g_warning("Error [ nf_tw2880_live_change fail.]");

		for(ch=0; ch<NUM_ACTIVE_CH; ch++)
		{
			g_message("<%s><%d>, screen_num[%d]=%d", __FUNCTION__, __LINE__, ch, data->screen_num[ch]);
			g_message("<%s><%d>, ch_arr[%d]=%d", __FUNCTION__, __LINE__, ch, live_param->ch_arr[ch]);
		}
		
	}
}

void nf_tw2880_live_stop(void)
{
/*	
	if ( itx_tw2880_live_stop () == -1 )
		g_warning("Error [ nf_tw2880_live_stop fail.]");
*/		
}

void nf_tw2880_border(guint border, guint border_color)
{
	unsigned char border_r, border_g, border_b;
/*
	if (itx_tw2880_channel_boundary_set_width( 1 ) == -1)
		g_warning("Error [ itx_tw2880_channel_boundary_set_width fail.]");


	if (itx_tw2880_channel_boundary_enable( 0 ) == -1)
		g_warning("Error [ itx_tw2880_channel_boundary_enable fail.]");
*/
	switch ( border_color )
	{
		case 0:					//white
			border_r = 0xff;
			border_g = 0xff;
			border_b = 0xff;
		break;
		case 1:					//gray
			border_r = 0xbe;
			border_g = 0xbe;
			border_b = 0xbe;
		break;
		case 2:					//yellow
			border_r = 0xff;
			border_g = 0xff;
			border_b = 0;
		break;
		case 3:					//blue
			border_r = 0;
			border_g = 0;
			border_b = 0xff;		
		break;
		case 4:					//green
			border_r = 0;
			border_g = 0xff;
			border_b = 0;		
		break;
		case 5:					//red
			border_r = 0xff;
			border_g = 0;
			border_b = 0;		
		break;
		default:
			border_r = 0;
			border_g = 0;
			border_b = 0xff;		
		break;
	}
	
//	itx_tw2880_channel_boundary_set_color( border_r, border_g, border_b );	
	
}

void nf_tw2880_freeze( guint freeze_mask_ch )
{
	unsigned int ch;
	tw2880_live_param_type *live_param = tw2880_disp_get_struct();

	for(ch=0; ch<NUM_ACTIVE_CH; ch++) {
		if(GETBITMASK(freeze_mask_ch, ch))
			live_param->freeze_ch[ch] = 1;
		else
			live_param->freeze_ch[ch] = 0;
	}

	if ( tw2880_live_change(live_param) == -1 )
	{
		g_warning("Error [ nf_tw2880_freeze fail.]");
		g_message("<%s><%d>, freeze_mask_ch=%08X", __FUNCTION__, __LINE__, freeze_mask_ch);

		for(ch=0; ch<NUM_ACTIVE_CH; ch++)
			g_message("<%s><%d>, live_param->freeze_ch[%d]=%d", __FUNCTION__, __LINE__, ch, live_param->freeze_ch[ch]);

	}

}


void nf_tw2880_covert( guint covert_mask_ch )
{
	unsigned int ch;

	tw2880_live_param_type *live_param = tw2880_disp_get_struct();

	for(ch=0; ch<NUM_ACTIVE_CH; ch++) {
		if(GETBITMASK(covert_mask_ch, ch))
			live_param->covert_ch[ch] = 1;
		else
			live_param->covert_ch[ch] = 0;
	}

	if ( tw2880_live_change(live_param) == -1 )
	{
		g_warning("Error [ nf_tw2880_covert fail.]");
		g_message("<%s><%d>, covert_mask_ch=%08X", __FUNCTION__, __LINE__, covert_mask_ch);

		for(ch=0; ch<NUM_ACTIVE_CH; ch++)
			g_message("<%s><%d>, live_param->covert_ch[%d]=%d", __FUNCTION__, __LINE__, ch, live_param->covert_ch[ch]);
	}

}

void nf_tw2880_zoom( unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy )
{
/*
	if ( itx_tw2880_zoom(sx, sy, dx, dy) == -1 )
	{
		g_warning("Error [ nf_tw2880_zoom fail.]");
		g_message("<%s><%d>, sx:%d, sy:%d, dx:%d, dy:%d",__FUNCTION__,__LINE__,sx,sy,dx,dy);
	}
*/
}

void nf_tw2880_set_md (void)
{
	tw2880_motion_param_type tw2880_mdcfg;

	guint ch;
    guint col, row, total_line;	
	char buff[256];
	guint sensitivity;
	char *motion_area;

	tw2880_mdcfg.disp_onoff = nf_sysdb_get_uint("disp.osd.motion");
	
	switch ( nf_sysdb_get_uint("disp.osd.motion_color") )
	{
		case 0:					//Yellow
			tw2880_mdcfg.disp_color.r = 0xff;
			tw2880_mdcfg.disp_color.g = 0xff;
			tw2880_mdcfg.disp_color.b = 0;
		break;
		case 1:					//Blue
			tw2880_mdcfg.disp_color.r = 0;
			tw2880_mdcfg.disp_color.g = 0;
			tw2880_mdcfg.disp_color.b = 0xff;
		break;
		case 2:					//Green
			tw2880_mdcfg.disp_color.r = 0;
			tw2880_mdcfg.disp_color.g = 0xff;
			tw2880_mdcfg.disp_color.b = 0;
		break;
		case 3:					//Red
			tw2880_mdcfg.disp_color.r = 0xff;
			tw2880_mdcfg.disp_color.g = 0;
			tw2880_mdcfg.disp_color.b = 0;		
		break;
		default:
			tw2880_mdcfg.disp_color.r = 0;
			tw2880_mdcfg.disp_color.g = 0;
			tw2880_mdcfg.disp_color.b = 0xff;		
		break;
	}

	for(ch=0; ch< NUM_ACTIVE_CH; ch++)
	{
		snprintf(buff, sizeof(buff), "alarm.motion.M%d.act", ch);
		if( nf_sysdb_get_bool(buff) == 0)
			continue;

		snprintf(buff, sizeof(buff), "alarm.motion.M%d.sense", ch);
		sensitivity = nf_sysdb_get_uint(buff);
				
		if (sensitivity <= 10)
		    sensitivity = 0;
		else if(sensitivity>=100)
            sensitivity = 9;
		else
            sensitivity = (sensitivity/10)-1;

		tw2880_mdcfg.ch_info[ch].onoff = 1;
		tw2880_mdcfg.ch_info[ch].check_interval = 2;
		tw2880_mdcfg.ch_info[ch].sensitivity = (3*sensitivity)+4;

		snprintf(buff, sizeof(buff), "alarm.motion.M%d.area", ch);
		motion_area = nf_sysdb_get_str_nocopy(buff);

		for( row = 0; row < MAX_MD_ROW; row++ )
		{
			for( col = 0; col < MAX_MD_COL; col++ )
			{			
				if( motion_area[row*MAX_MD_COL+col] == '1' )
					tw2880_mdcfg.ch_info[ch].area[row][col] = 1;
				else
					tw2880_mdcfg.ch_info[ch].area[row][col] = 0;
			}
		}	
	}

    #ifdef DEBUG_LIVE_APPLY_MOTION
	nf_debug_hexdump(&tw2880_mdcfg.ch_info[ch].area, sizeof(tw2880_mdcfg.ch_info[ch].area));
#endif
/*
	if ( itx_tw2880_set_motion_detection(tw2880_mdcfg) == -1 )
		g_warning("Error [ nf_tw2880_set_md fail.]");
*/
}

int nf_tw2880_set_spot_title(void)
{
	unsigned char i;
	char cam_title[10];

// spot
//	itx_tw2880_set_spot_osd_com_str("parangi spot test");

	for( i=0; i< NUM_ACTIVE_CH ; i++ )
	{
//		sprintf(cam_title, "CH %d", i+1);
		DAL_get_camera_title(cam_title, i);
//		itx_tw2880_set_spot_osd_ch_str1(i, cam_title);
//		itx_tw2880_set_spot_osd_ch_str2(i, cam_title);	
	}

	return TRUE;
}

int nf_tw2880_set_spot(tw2880_spot_param_type spot_param)
{
/*	
	if ( itx_tw2880_set_spot(spot_param) == -1 ) {
		g_warning("Error [ nf_tw2880_set_spot fail.]");

		return FALSE;		
	}
*/
	return TRUE;
}

int nf_solo_cmd_playback( guchar start_ch, tw2880_scr_div_mode_e div_mode)
{
	SOLO_DISP *disp = solo_disp_get_struct();
	volatile unsigned int width;
	volatile unsigned int height;
	volatile unsigned int mux_mode;
	unsigned char dev, channel, act_solo, act_window, cnt_act_ch=0;
	int erase_force = 1;

	unsigned int disp_dev[4][16] = {
/* solo dev_0*/	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
/* solo dev_1*/	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
/* solo dev_2*/	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
/* solo dev_3*/	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}
	};
	
//	is_mode = DISP_MODE_NORMAL;

//	for(channel=0; channel<NUM_ACTIVE_CH; channel++)
//		g_message("<%s, %d>, ch=%d, ch_arr=%d, mode=%d", __FUNCTION__, __LINE__, channel, pb_param.ch_arr[channel], pb_param.div_mode);

	switch(div_mode)
	{
		case TW2880_SCR_DIV_MODE_1:
			mux_mode = MUX_MODE_1CH;
			act_window = 1;
			act_solo = 1;
		break;

		case TW2880_SCR_DIV_MODE_4:
#if defined(_SNF_0824) || defined(_SNF_0424)
			mux_mode = MUX_MODE_4CH;
			act_window = 4;
			start_ch = 0;
#elif defined(_SNF_1648)
			mux_mode = MUX_MODE_1CH;
			act_window = 1;
#endif
			act_solo = NUM_SOLO;
		break;

		case TW2880_SCR_DIV_MODE_9:
			mux_mode = MUX_MODE_4CH;
			act_window = 4;
			start_ch = 0;
			act_solo = NUM_SOLO;		
		break;

#if !defined(_SNF_0824)		//except
		case TW2880_SCR_DIV_MODE_16:
			mux_mode = MUX_MODE_4CH;
			act_window = 4;
			act_solo = NUM_SOLO;			
		break;
#endif		
		default:
			g_warning("%s unsupported live mode[%d]", __FUNCTION__, div_mode);
			return 0;	
	}

	for(dev=0; dev<act_solo; dev++)
	{
		for(channel=0; channel<act_window; channel++)
		{
			disp_dev[dev][channel] = start_ch;
			disp_dev[dev][start_ch] = channel;
			start_ch++;
		}
	}

	g_return_val_if_fail( mux_mode<6, 0);

	width = (disp->width / disp_div[mux_mode]) & (~7);
	height = (disp->height / disp_div[mux_mode]) & (~7);

	g_return_val_if_fail( width<=disp->width, 0);
	g_return_val_if_fail( height<=disp->height, 0);

	for(dev=0; dev<NUM_SOLO; dev++)
	{
		ioctl(disp->fd[dev], IOCTL_DISP_ERASE_ON, (long)&erase_force);

		cnt_act_ch = 0;

		for(channel=0; channel<NUM_ACTIVE_CH; channel++)
		{
			struct DISP_CTRL *disp_ctrl;

			disp_ctrl = &disp->disp_ctrl[dev][channel];

			disp_ctrl->window_id = disp_dev[dev][channel];

	     	if(channel < act_window)
			{			
				disp_ctrl->scale = disp_scale[mux_mode];
				disp_ctrl->sx = width * (cnt_act_ch%disp_div[mux_mode]);
				disp_ctrl->sy = height * (cnt_act_ch/disp_div[mux_mode]);
				disp_ctrl->ex = disp_ctrl->sx + width;
				disp_ctrl->ey = disp_ctrl->sy + height;

				cnt_act_ch++;
			}
			else
			{
				disp_ctrl->scale = 0;
				disp_ctrl->sx = 0;
				disp_ctrl->sy = 0;
				disp_ctrl->ex = 0;
				disp_ctrl->ey = 0;
			}

			g_return_val_if_fail(disp_ctrl->scale<8, 0);
			g_return_val_if_fail(disp_ctrl->channel<NUM_ANALOG_CHANNEL, 0);
			g_return_val_if_fail(disp_ctrl->sx<=disp->width, 0);
			g_return_val_if_fail(disp_ctrl->sy<=disp->height, 0);
			g_return_val_if_fail(disp_ctrl->ex<=disp->width, 0);
			g_return_val_if_fail(disp_ctrl->ey<=disp->height, 0);

//			g_message("<%s>,<%d>,<dev:%d><window:%d><channel:%d><sx:%d>,<sy:%d>,<ex:%d>,<ey:%d> ", __FUNCTION__, __LINE__, 
//						dev, disp->disp_ctrl[dev][channel].window_id, disp->disp_ctrl[dev][channel].channel, 
//						disp->disp_ctrl[dev][channel].sx, disp->disp_ctrl[dev][channel].sy, 
//						disp->disp_ctrl[dev][channel].ex, disp->disp_ctrl[dev][channel].ey);

			ioctl(disp->fd[dev], IOCTL_DISP_CHANNEL, &disp->disp_ctrl[dev][channel]);
		}
		
		ioctl(disp->fd[dev], IOCTL_DISP_ERASE_OFF);	
	}

	return 0;
}

gboolean nf_tw2880_playback_start(void)
{
	unsigned char i = 0;
	tw2880_pb_param_type pb_param;
	struct DISP_CTRL disp_ctrl;
	guchar start_ch, ch_cnt;	
	
	memset(pb_param.ch_arr, 0xff, sizeof(pb_param.ch_arr));

#if defined(_SNF_1648)
	pb_param.div_mode = TW2880_SCR_DIV_MODE_16;
#elif defined(_SNF_0824)
	pb_param.div_mode = TW2880_SCR_DIV_MODE_9;
#elif defined(_SNF_0424)
	pb_param.div_mode = TW2880_SCR_DIV_MODE_4;	
#else
	pb_param.div_mode = TW2880_SCR_DIV_MODE_16;
#endif

	for (i=0; i<NUM_ACTIVE_CH; i++)
		pb_param.ch_arr[i] = i;

	start_ch = 0;
#if defined(_SNF_1648)
	ch_cnt = 16;
#elif defined(_SNF_0824)
	ch_cnt = 9;
#elif defined(_SNF_0424)
	ch_cnt = 4;
#endif

	if ( nf_solo_cmd_playback( start_ch, pb_param.div_mode) == -1 ) {
		g_warning("Error [ nf_tw2880_playback_start fail.]");

		return FALSE;
	}
/*	
	if ( itx_tw2880_pb_start(pb_param) == -1 ) {
		g_warning("Error [ nf_tw2880_playback_start fail.]");

		for (i=0; i<NUM_ACTIVE_CH; i++)
			g_message("nf_tw2880_playback_start %d",pb_param.ch_arr[i] );

		return FALSE;
	}
*/
	return TRUE;
}

gboolean nf_tw2880_playback_change( guchar ch_arr[DSP_MAX_CHAN], tw2880_scr_div_mode_e div_mode, guchar start_ch, guchar ch_cnt)
{
	unsigned char i = 0;
	tw2880_pb_param_type pb_param;

	pb_param.div_mode = div_mode;

	for (i=0; i<NUM_ACTIVE_CH; i++)
		pb_param.ch_arr[i] = ch_arr[i];

	if ( nf_solo_cmd_playback( start_ch, pb_param.div_mode) == -1 ) {
		g_warning("Error [ nf_tw2880_playback_start fail.]");

		return FALSE;
	}
/*	
	if ( itx_tw2880_pb_change(pb_param) == -1 ) {
		g_warning("Error [ nf_tw2880_playback_change fail.]");

		for (i=0; i<NUM_ACTIVE_CH; i++)
			g_message("nf_tw2880_playback_change %d",pb_param.ch_arr[i] );

		return FALSE;
	}
*/
	return TRUE;
}

gboolean nf_tw2880_playback_stop(void)
{
/*
	if ( itx_tw2880_pb_stop () == -1 ) {
		g_warning("Error [ nf_tw2880_playback_stop fail.]");

		return FALSE;		
	}
*/	
	return TRUE;
}

int solo_disp_create(SOLO_DISP **p)
{
	if ( *p != NULL )
		return -1;

	// Alloc struct.
	*p = malloc( sizeof(SOLO_DISP) );

	// Exception.
	if( *p == NULL )
		return -1;
	
	// Init memory.
	bzero( *p, sizeof(SOLO_DISP) );
	
	return 1;
}

int tw2880_disp_create(tw2880_live_param_type **live_param)
{
	if ( *live_param != NULL )
		return -1;

	// Alloc struct.
	*live_param = malloc( sizeof(tw2880_live_param_type) );

	// Exception.
	if( *live_param == NULL )
		return -1;
	
	// Init memory.
	bzero( *live_param, sizeof(tw2880_live_param_type) );
	
	return 1;
}

int solo_disp_open(SOLO_DISP *t)
{
	unsigned char dev, channel;
	unsigned int width;
	unsigned int height;
	unsigned int border;
	unsigned int border_color;

	tw2880_pb_param_type pb_param;

	if (t == NULL)
	{
    	g_warning("<%s> <%d> : pointer is null ", __FUNCTION__, __LINE__);	
		return -1;
	}

	for( dev=0; dev< NUM_SOLO ; dev++ )
	{
		if (dev == 0)
			t->fd[dev] = open( IOCTL_DISP_DEV_0, O_RDWR);
		else if (dev == 1)
			t->fd[dev] = open( IOCTL_DISP_DEV_1, O_RDWR);
		else if (dev == 2)
			t->fd[dev] = open( IOCTL_DISP_DEV_2, O_RDWR);
		else if (dev == 3)		
			t->fd[dev] = open( IOCTL_DISP_DEV_3, O_RDWR);
		
		if ( t->fd[dev] < 0 )
		{
			g_warning("<%s> <%d> : display fd is null ", __FUNCTION__, __LINE__);	
			return -1;
		}

		ioctl(t->fd[dev], IOCTL_DISP_GET_XRES, &width);
		ioctl(t->fd[dev], IOCTL_DISP_GET_YRES, &height);

		height = height * 2;

		t->width = width;
		t->height = height;

		for( channel=0; channel< NUM_ACTIVE_CH ; channel++ )
		{
			struct DISP_CTRL *disp_ctrl;

			disp_ctrl = &t->disp_ctrl[dev][channel];

			disp_ctrl->channel = channel;
			disp_ctrl->mode = DISP_MODE_PB;			
		}
	}

	return 1;
}


void tw2880_get_position_info(int scrsize_x, int scrsize_y, int div, TW2880_ST_RECT_WH *pRectArray)
{
	guchar			div_x, div_y;
	TW2880_ST_SIZE		unitsize;
	int			i	= 0,x,y;
	int surplus_x, surplus_y;

	int unitsize_x = 704,unitsize_y=480;

	

	if( div <= 1 )
	{
		div_x = 1, div_y  =   1;
	}
	else if( div <= 4 )
	{
		div_x = 2, div_y  =   2;
	}
	else if( div == 6 )
	{
		div_x = 3, div_y  =   3;
		unitsize_x /=2;
		unitsize_y /=2;
	}
	else if( div == 8)
	{
		div_x = 4; div_y = 4;
		unitsize_x /=3;
		unitsize_y /=3;
	}
	else if( div <= 9 )
	{
		div_x = 3, div_y = 3;
	}
	else if( div <= 16 )
	{
		div_x = 4, div_y = 4;
	}
	else if( div <= 20 )
	{
		div_x = 5, div_y = 4;
	}
	else
	{
		div_x = 7, div_y = 5;
	}

#if 1  //kbulls 100726
	surplus_x = (scrsize_x % (4*div_x));		/* multiple 4 */
	surplus_x = surplus_x>>2<<2;
	surplus_y = (scrsize_y % (2*div_y));		/* make to even number */
	surplus_y = surplus_y>>1<<1;
#endif

	unitsize.cx	= (scrsize_x/div_x);
	unitsize.cy	= (scrsize_y/div_y);
#if 1 //kbulls 100611
{
	/* adjust size and position */
	if( unitsize.cx > unitsize_x - (surplus_x ? 4 : 0) )
		unitsize.cx = unitsize_x - (surplus_x ? 4 : 0);
	unitsize.cx = (unitsize.cx>>2<<2);		/* multiple 4 */

	if( unitsize.cy > unitsize_y - (surplus_y ? 2 : 0) )
		unitsize.cy = unitsize_y - (surplus_y ? 2 : 0);
	unitsize.cy = unitsize.cy>>1<<1;	/* make to even number */

//	printf(" unitsize.cx %d unitsize.cy %d \n",unitsize.cx,unitsize.cy);
//	printf(" scrsize_x %d scrsize_y %d \n",scrsize_x,scrsize_y);
//	printf(" div_x. %d div_y. %d \n",div_x,div_y);
//	printf(" surplus_x. %d surplus_y. %d \n",surplus_x,surplus_y);
	/* The registers are [0x614] to [0x663].
	   They must be set to even number ( Jan/13/2009 - Nick ) */
}
#else	
	/* adjust size and position */
	if( punitres )
	{
		if( unitsize.cx > unitsize_x )
			unitsize.cx = unitsize_x;
	}
	unitsize.cx = (unitsize.cx>>2<<2);		/* multiple 4 */

	if( punitres )
	{
		if( unitsize.cy > unitsize_y )
			unitsize.cy = unitsize_y;
	}

	/* The registers are [0x614] to [0x663].
	   They must be set to even number ( Jan/13/2009 - Nick ) */
	unitsize.cy = unitsize.cy>>1<<1;	/* make to even number */
#endif

	int surplus_x_backup = surplus_x;
	int surplus_y_backup = surplus_y;
	for( y=0, i=0; y<div_y; y++)
	{
		surplus_x = surplus_x_backup;
		for( x=0; x<div_x; x++)
		{
			if (div == 6)
			{
				if (x==0 && y==0)
				{
					pRectArray[i].size.cx	= unitsize.cx*2;
					pRectArray[i].size.cy	= unitsize.cy*2;
					pRectArray[i].pt.x	= 0;
					pRectArray[i].pt.y	= 0;		
					if(surplus_x >= 8)
					{
						pRectArray[i].size.cx += 8;
						surplus_x-=8;
					}
					else if(surplus_x >= 4)
					{
						pRectArray[i].size.cx += 4;
						surplus_x-=4;
					}
					if(surplus_y >= 4)
					{
						pRectArray[i].size.cy += 4;
					}
					else if(surplus_y >= 2)
					{
						pRectArray[i].size.cy += 2;
					}
					i++;
				}
				else if(x==1 && y<2)
				{
					surplus_x=0;
				}
				else if ((x==2 && y<2) || y==2)
				{
					pRectArray[i].size.cx	= unitsize.cx;
					pRectArray[i].size.cy	= unitsize.cy;
					pRectArray[i].pt.x	= 0 + unitsize.cx*x;
					pRectArray[i].pt.y	= 0 + unitsize.cy*y;		
					if(surplus_x >= 4)
					{
						pRectArray[i].size.cx += 4;
						surplus_x-=4;
					}
					if(surplus_y >= 2)
					{
						pRectArray[i].size.cy += 2;
					}

					if(x*4 <= surplus_x_backup)
						pRectArray[i].pt.x += x*4;
					else
						pRectArray[i].pt.x += surplus_x_backup;

					if(y*2 <= surplus_y_backup)
						pRectArray[i].pt.y += y*2;
					else
						pRectArray[i].pt.y += surplus_y_backup;
					i++;
				}
			}
			else if (div == 8)
			{
				if (x==0 && y==0)
				{
					pRectArray[i].size.cx	= unitsize.cx*3;
					pRectArray[i].size.cy	= unitsize.cy*3;
					pRectArray[i].pt.x	= 0;
					pRectArray[i].pt.y	= 0;		
					if(surplus_x >= 12)
					{
						pRectArray[i].size.cx += 12;
						surplus_x-=12;
					}
					else if(surplus_x >= 8)
					{
						pRectArray[i].size.cx += 8;
						surplus_x-=8;
					}
					else if(surplus_x >= 4)
					{
						pRectArray[i].size.cx += 4;
						surplus_x-=4;
					}
					if(surplus_y >= 6)
					{
						pRectArray[i].size.cy += 6;
					}
					else if(surplus_y >= 4)
					{
						pRectArray[i].size.cy += 4;
					}
					else if(surplus_y >= 2)
					{
						pRectArray[i].size.cy += 2;
					}
					i++;
				}
				else if(x==2 && y<3)
				{
					surplus_x=0;
				}
				else if ((x==3 && y<3) || y==3)
				{
					pRectArray[i].size.cx	= unitsize.cx;
					pRectArray[i].size.cy	= unitsize.cy;
					pRectArray[i].pt.x	= 0 + unitsize.cx*x;
					pRectArray[i].pt.y	= 0 + unitsize.cy*y;		
					if(surplus_x >= 4)
					{
						pRectArray[i].size.cx += 4;
						surplus_x-=4;
					}
					if(surplus_y >= 2)
					{
						pRectArray[i].size.cy += 2;
					}
					if(x*4 <= surplus_x_backup)
						pRectArray[i].pt.x += x*4;
					else
						pRectArray[i].pt.x += surplus_x_backup;

					if(y*2 <= surplus_y_backup)
						pRectArray[i].pt.y += y*2;
					else
						pRectArray[i].pt.y += surplus_y_backup;
					i++;
				}
			}
			else
			{
				pRectArray[i].size	= unitsize;
				pRectArray[i].pt.x	= 0 + (unitsize.cx*x);
				pRectArray[i].pt.y	= 0 + (unitsize.cy*y);
				if(surplus_x >= 4)
				{
					pRectArray[i].size.cx += 4;
					surplus_x-=4;
				}
				if(surplus_y >= 2)
				{
					pRectArray[i].size.cy += 2;
				}
				
				if(x*4 <= surplus_x_backup)
					pRectArray[i].pt.x += x*4;
				else
					pRectArray[i].pt.x += surplus_x_backup;

				if(y*2 <= surplus_y_backup)
					pRectArray[i].pt.y += y*2;
				else
					pRectArray[i].pt.y += surplus_y_backup;
				i++;
			}		
//			printf("-- %2d %1d %1d [ %4d %4d %4d %4d ] \r\n",
//								i-1, div_x, div_y,
//								pRectArray[i-1].pt.x, pRectArray[i-1].pt.y,
//								pRectArray[i-1].size.cx, pRectArray[i-1].size.cy);
		}
		surplus_y-=2;
	}

#if 1
	{
		int cxr = scrsize_x * 10000 / (div_x*unitsize.cx + surplus_x_backup);
		int cyr = scrsize_y * 10000 / (div_y*unitsize.cy + surplus_y_backup);
//		printf("cxr %d cyr %d (%d %d)\n",cxr,cyr, (div_x*unitsize.cx + surplus_x_backup), (div_y*unitsize.cy + surplus_y_backup));
#if 1
		for( i=0; i<div; i++ )
		{
			pRectArray[i].pt.x		= pRectArray[i].pt.x * cxr / 10000;
			pRectArray[i].pt.y		= pRectArray[i].pt.y * cyr / 10000;
			pRectArray[i].size.cx	= pRectArray[i].size.cx * cxr / 10000;
			pRectArray[i].size.cy	= pRectArray[i].size.cy * cyr / 10000;
		}
#else		
		for( i=0; i<div; i++ )
		{
			printf("(%d) %d %d %d %d \n",i,pRectArray[i].pt.x * cxr / 10000,pRectArray[i].pt.y * cyr / 10000,pRectArray[i].size.cx * cxr / 10000,pRectArray[i].size.cy * cyr / 10000);
		}
#endif		
	}
#endif

}




//tw2880 : init -> change monitor -> live start
int tw2880_disp_open(tw2880_live_param_type *live_param)		
{
	unsigned char i = 0;
	unsigned int border, border_color;
	char cam_title[10];

	if (live_param == NULL)
	{
    	g_warning("<%s> <%d> : pointer is null ", __FUNCTION__, __LINE__);
		g_assert(0);
		return -1;
	}

	memset(live_param->ch_arr, 0xff, sizeof(live_param->ch_arr));

#if defined(_SNF_1648)
	live_param->div_mode = TW2880_SCR_DIV_MODE_16;
#elif defined(_SNF_0824)
	live_param->div_mode = TW2880_SCR_DIV_MODE_9;
#elif defined(_SNF_0424)
	live_param->div_mode = TW2880_SCR_DIV_MODE_4;
#else
	live_param->div_mode = TW2880_SCR_DIV_MODE_16;
#endif

	for( i=0; i< NUM_ACTIVE_CH ; i++ )
	{
		char buff[64];
		
		snprintf(buff, sizeof(buff), "cam.C%d.covert", i);
								
		if( nf_sysdb_get_bool(buff) ) 
		{
			live_param->covert_ch[i] = 1;
		}					
		else
		{
			live_param->covert_ch[i] = 0;
		}

		live_param->ch_arr[i] = i;
	}
/*
	if ( itx_tw2880_live_start(*live_param) == -1 )
	{
		g_warning("Error [ itx_tw2880_live_start fail.]");
		g_assert(0);
		return -1;		
	}
*/
// border
	border = nf_sysdb_get_uint("disp.osd.border");
	border_color = nf_sysdb_get_uint("disp.osd.border_color");		

	nf_tw2880_border(border, border_color); 

	nf_tw2880_set_spot_title();

	return 0;
}

int solo_disp_init(SOLO_DISP **p)
{
	if ( *p != NULL )
		return -1;

	if ( solo_disp_create( p ) == -1 )
	{
		g_warning("*Error* [ Alloc SOLO_Display fail.]");
		return -1;
	}

	if ( solo_disp_open( *p ) == -1 )
	{
		g_warning("*Error* [ Open SOLO_Display_0 fail.]");
		return -1;
	}

	return 1;
}

int tw2880_disp_init(tw2880_live_param_type **live_param)
{
	if ( tw2880_disp_create( live_param ) == -1 )
	{
		g_warning("Error [ Alloc TW2880_Display fail.]");
		g_assert(0);
		return -1;
	}

	if ( tw2880_disp_open( *live_param ) == -1 )
	{
		g_warning("Error [ Open TW2880_Display fail.]");
		g_assert(0);
		return -1;
	}

	return 1;
}

int tw2880_config_init(void)
{
	tw2880_init_config_type tw2880_cfg={0,};
	
#if	defined(_SNF_0824) || defined(_SNF_0424)
	tw2880_cfg.pjt_name = SNF_0824;
#else 
	tw2880_cfg.pjt_name = SNF_1648;
#endif	
	
#if 1
	tw2880_cfg.output_resolution = TW2880_SCR_MODE_NULL;
#else
	if (DISPLAY_WIDTH == 704) {
		itx_tw2880_change_monitor(0);
		g_message("<%s, %d>, tw2880 display init size : 704x480, av monitor out", __FUNCTION__, __LINE__);		
	} else if (DISPLAY_WIDTH == 800) {
		tw2880_cfg.output_resolution = TW2880_SCR_800x600;
		g_message("<%s, %d>, tw2880 display init size : 800x600", __FUNCTION__, __LINE__);
	} else if (DISPLAY_WIDTH == 1280) {
		tw2880_cfg.output_resolution = TW2880_SCR_1280x1024;
		g_message("<%s, %d>, tw2880 display init size : 1280x1024", __FUNCTION__, __LINE__);
	} else if (DISPLAY_WIDTH == 1920) {
		tw2880_cfg.output_resolution = TW2880_SCR_1920x1080;
		g_message("<%s, %d>, tw2880 display init size : 1920x1080", __FUNCTION__, __LINE__);		
	} else {
		g_warning("<%s, %d>, tw2880 unsupported display size", __FUNCTION__, __LINE__);
		g_assert(0);
	}		
#endif
	
	tw2880_cfg.color_convert[0].SrcColor= 0xffff;
	tw2880_cfg.color_convert[0].DstColor= 0xfffe;
	tw2880_cfg.color_convert[1].SrcColor= 0x0000;
	tw2880_cfg.color_convert[1].DstColor= 0xffff;
	tw2880_cfg.color_convert[2].SrcColor= 0x0000;
	tw2880_cfg.color_convert[2].DstColor= 0xffff;
	tw2880_cfg.color_convert[3].SrcColor= 0x0000;
	tw2880_cfg.color_convert[3].DstColor= 0xffff;

	tw2880_cfg.spot_param.bdry_color.y = 235;
	tw2880_cfg.spot_param.bdry_color.u = 128;
	tw2880_cfg.spot_param.bdry_color.v = 128;
	tw2880_cfg.spot_param.font_palette.v1.y = 235;
	tw2880_cfg.spot_param.font_palette.v1.u = 128;
	tw2880_cfg.spot_param.font_palette.v1.v = 128;
	tw2880_cfg.spot_param.font_palette.v2.y = 235;
	tw2880_cfg.spot_param.font_palette.v2.u = 128;
	tw2880_cfg.spot_param.font_palette.v2.v = 128;
	tw2880_cfg.spot_param.font_palette.v3.y = 235;
	tw2880_cfg.spot_param.font_palette.v3.u = 128;
	tw2880_cfg.spot_param.font_palette.v3.v = 128;
	tw2880_cfg.spot_param.com_str.x = 70;
	tw2880_cfg.spot_param.com_str.y = 100;
	tw2880_cfg.spot_param.ch_str1.x = 70;
	tw2880_cfg.spot_param.ch_str1.y = 10;
	tw2880_cfg.spot_param.ch_str2.x = 70;
	tw2880_cfg.spot_param.ch_str2.y = 150;
/*
	if ( itx_tw2880_init(tw2880_cfg) == -1 )
	{
		g_warning("Error [ itx_tw2880_init fail.]");
		g_assert(0);
		return -1;		
	}
*/
	return 1;
}

#endif /* _SNF_MODEL */

/*EOF*/
