#include <glib.h>

#include "nf_common.h"
#include "nf_util_device.h"
// #include "nf_tw2880_disp.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "display"

/* compensation factor */
#if defined(_ANF_1648)
	#define DISPLAY_D1_COMP_W   (0)
	#define DISPLAY_D1_COMP_H   (0)

#elif defined(_ATM_1624)
	#define DISPLAY_D1_COMP_W   (0)
	#define DISPLAY_D1_COMP_H   (0)

#elif defined(_ATM_0424)
	#define DISPLAY_D1_COMP_W   (0)
	#define DISPLAY_D1_COMP_H   (0)

#elif defined(_OTM_MODEL)
	#define DISPLAY_D1_COMP_W   (0)
	#define DISPLAY_D1_COMP_H   (0)

#elif defined(_SNF_MODEL)
	#define DISPLAY_D1_COMP_W   (0)
	#define DISPLAY_D1_COMP_H   (0)

#else   /*_NF_BASE_MODEL*/
	#define DISPLAY_D1_COMP_W   (32)
	#define DISPLAY_D1_COMP_H   (24)
#endif  /*_ANF_1648*/

#if defined(_SNF_MODEL)
	#define DISPLAY_4D1_COMP_W   (0)
	#define DISPLAY_4D1_COMP_H   (0)

#else   /*_NF_BASE_MODEL*/
	#define DISPLAY_4D1_COMP_W   (40)
	#define DISPLAY_4D1_COMP_H   (24)
#endif  /*_ANF_1648*/

static gint	NF_display_is_pal	= 0;
static gint	NF_display_is_d1	= 0;

static gfloat	NF_display_rate		= DISPLAY_NTSC_RATE;

static gint	NF_display_width		= DISPLAY_D4_WIDTH;
static gint	NF_display_height		= DISPLAY_NTSC4_HEIGHT;

static gint	NF_display_active_width	= DISPLAY_D4_WIDTH - 40;        //this size is depend on DM647 display output 
static gint	NF_display_active_height= DISPLAY_NTSC4_HEIGHT - 24;    //this size is depend on DM647 display output

static gint	NF_display_D1_height	= D1_NTSC_HEIGHT;
static gint	NF_display_CIF_height	= CIF_NTSC_HEIGHT;

static gint	NF_video_width		= D1_WIDTH;
static gint	NF_video_height		= D1_NTSC_HEIGHT;

gint nf_display_is_pal()
{
	return NF_display_is_pal;
} 
gint nf_display_is_d1()
{
	return NF_display_is_d1;
}

gfloat nf_display_get_rate()
{
	return NF_display_rate;
}
gint nf_display_get_width()
{
	return NF_display_width;
}
gint nf_display_get_height()
{
	return NF_display_height;
}
gint nf_display_get_d1_height()
{
	return NF_display_D1_height;
}
gint nf_display_get_cif_height()
{
	return NF_display_CIF_height;
}

gint nf_display_get_active_width()
{
	return NF_display_active_width;
}
gint nf_display_get_active_height()
{
	return NF_display_active_height;
}
gint nf_video_get_width()
{
	return NF_video_width;
}
gint nf_video_get_height()
{
	return NF_video_height;
}

void nf_display_init(void)
{
	unsigned int monitor_sel, ddc_width, ddc_height;

/*  SNF_MODEL_XXXX
	tw2880_config_init();
		
	itx_tw2880_get_env( &monitor_sel, &ddc_width, &ddc_height );
*/

//monitor_sel 0:CVBS, 1:VGA, 2:HDMI

	 ddc_width = 1920; 
#if defined(_ANF_1648)||defined(_ATM_1624)||defined(_ATM_0424)
	NF_display_is_d1	= 0;    //always D1 output.
#elif defined(_SNF_MODEL)
	NF_display_is_pal	= nf_dev_board_pp_is_pal();
#else
	NF_display_is_d1	= nf_dev_board_pp_is_d1();
#endif

	NF_display_is_pal	= nf_sysdb_get_bool("sys.info.sig_type");
	
#if defined(_SNF_MODEL)

	if(NF_display_is_pal)
	{
		NF_display_rate	= DISPLAY_PAL_RATE;
		NF_video_width = D1_WIDTH;
		NF_video_height = D1_PAL_HEIGHT;
	}
	else
	{
		NF_display_rate	= DISPLAY_NTSC_RATE;
		NF_video_width = D1_WIDTH;
		NF_video_height = D1_NTSC_HEIGHT;		
	}
				
	if( ddc_width == 704 ) 
	{
		NF_display_is_d1 = 1;
		
		if(NF_display_is_pal) {				
			NF_display_width	= D1_WIDTH;
			NF_display_height	= D1_PAL_HEIGHT;

			NF_display_D1_height	= D1_PAL_HEIGHT;
			NF_display_CIF_height	= D1_PAL_HEIGHT/2;

			NF_display_active_width	 = D1_WIDTH;
			NF_display_active_height = D1_PAL_HEIGHT;

		}else{
			NF_display_width	= D1_WIDTH;
			NF_display_height	= D1_NTSC_HEIGHT;

			NF_display_D1_height	= D1_NTSC_HEIGHT;
			NF_display_CIF_height	= D1_NTSC_HEIGHT/2;

			NF_display_active_width	 = D1_WIDTH;
			NF_display_active_height = D1_NTSC_HEIGHT;
		}
	}
	else if ( ddc_width == 800 ) 
	{
			NF_display_is_d1	= 1;	
			NF_display_width	= WIDTH_800;
			NF_display_height	= HEIGHT_600;

			NF_display_D1_height	= HEIGHT_600;
			NF_display_CIF_height	= HEIGHT_600/2;

			NF_display_active_width	 = WIDTH_800;
			NF_display_active_height = HEIGHT_600;
	}
	else if ( ddc_width == 1280 ) 
	{
			NF_display_is_d1	= 0;	
			NF_display_width	= WIDTH_1280;
			NF_display_height	= HEIGHT_1024;

			NF_display_D1_height	= HEIGHT_1024;
			NF_display_CIF_height	= HEIGHT_1024/2;

			NF_display_active_width	 = WIDTH_1280;
			NF_display_active_height = HEIGHT_1024;
	}
	else if ( ddc_width == 1920 ) 
	{
			NF_display_is_d1	= 0;		
			NF_display_width	= WIDTH_1920;
			NF_display_height	= HEIGHT_1080;

			NF_display_D1_height	= HEIGHT_1080;
			NF_display_CIF_height	= HEIGHT_1080/2;
#if 0
			NF_display_active_width	 = WIDTH_1920 - 192;
			NF_display_active_height = HEIGHT_1080 - 108;
#else
			NF_display_active_width	 = WIDTH_1920;
			NF_display_active_height = HEIGHT_1080;
#endif
	}
	else
	{
			g_message("<%s, %d> itx_tw2880_get_env width fail. set display 800x600", __FUNCTION__, __LINE__);
			
			NF_display_is_d1	= 1;	
			NF_display_width	= WIDTH_800;
			NF_display_height	= HEIGHT_600;

			NF_display_D1_height	= HEIGHT_600;
			NF_display_CIF_height	= HEIGHT_600/2;

			NF_display_active_width	 = WIDTH_800;
			NF_display_active_height = HEIGHT_600;
	}
	
#else
	if(	NF_display_is_d1 )
	{
		if(NF_display_is_pal) {				
			NF_display_rate		= DISPLAY_PAL_RATE;
			NF_display_width	= DISPLAY_D1_WIDTH;
			NF_display_height	= DISPLAY_PAL_HEIGHT;

			NF_display_D1_height	= D1_PAL_HEIGHT;
			NF_display_CIF_height	= CIF_PAL_HEIGHT;

			NF_display_active_width	 = DISPLAY_D1_WIDTH-DISPLAY_D1_COMP_W;
			NF_display_active_height = DISPLAY_PAL_HEIGHT-DISPLAY_D1_COMP_H;

		}else{
			NF_display_rate		= DISPLAY_NTSC_RATE;
			NF_display_width	= DISPLAY_D1_WIDTH;
			NF_display_height	= DISPLAY_NTSC_HEIGHT;

			NF_display_D1_height	= D1_NTSC_HEIGHT;
			NF_display_CIF_height	= CIF_NTSC_HEIGHT;

			NF_display_active_width	 = DISPLAY_D1_WIDTH-DISPLAY_D1_COMP_W;
			NF_display_active_height = DISPLAY_NTSC_HEIGHT-DISPLAY_D1_COMP_H;
		}
	}else{	// 4D1

		if(NF_display_is_pal) {		
			NF_display_rate		= DISPLAY_PAL_RATE;
			NF_display_width	= DISPLAY_D4_WIDTH;
			NF_display_height	= DISPLAY_PAL4_HEIGHT;				
			NF_display_D1_height	= D1_PAL_HEIGHT;
			NF_display_CIF_height	= CIF_PAL_HEIGHT;				

			NF_display_active_width	 = (NF_display_width - DISPLAY_4D1_COMP_W);
			NF_display_active_height = (NF_display_height - DISPLAY_4D1_COMP_H);		

		} else{

			NF_display_rate		= DISPLAY_NTSC_RATE;
			NF_display_width	= DISPLAY_D4_WIDTH;
			NF_display_height	= DISPLAY_NTSC4_HEIGHT;				
			NF_display_D1_height	= D1_NTSC_HEIGHT;
			NF_display_CIF_height	= CIF_NTSC_HEIGHT;				

			NF_display_active_width	 = (NF_display_width - DISPLAY_4D1_COMP_W);
			NF_display_active_height = (NF_display_height - DISPLAY_4D1_COMP_H);		

		}
	}
#endif
				
	g_message("%s ======================================", __FUNCTION__ );		
	g_message("%s is_pal                [%d]", __FUNCTION__, NF_display_is_pal	);		
	g_message("%s is_d1                 [%d]", __FUNCTION__, NF_display_is_d1	);		
	g_message("%s display_rate          [%.2f]", __FUNCTION__, NF_display_rate	);
	g_message("%s display_width         [%d]", __FUNCTION__, NF_display_width	);		
	g_message("%s display_height        [%d]", __FUNCTION__, NF_display_height	);		
	g_message("%s display_active_width  [%d]", __FUNCTION__, NF_display_active_width	);		
	g_message("%s display_active_height [%d]", __FUNCTION__, NF_display_active_height	);		
	g_message("%s display_D1_height     [%d]", __FUNCTION__, NF_display_D1_height );		
	g_message("%s display_CIF_height    [%d]", __FUNCTION__, NF_display_CIF_height	);
	g_message("%s video_width     		[%d]", __FUNCTION__, NF_video_width );		
	g_message("%s video_height    		[%d]", __FUNCTION__, NF_video_height );
	g_message("%s monitor_sel     		[%d]", __FUNCTION__, monitor_sel );			
	g_message("%s ======================================", __FUNCTION__ );		
}
