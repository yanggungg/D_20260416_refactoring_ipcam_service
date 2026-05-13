/***************************************************************

 @NAME : tw2880_api.h

 @Description : tw2880 driver API header file

 @History:

  2010.01.27 by ttl7905  
     - create file

****************************************************************/


#ifndef __TW2880_API_H__
#define __TW2880_API_H__

#include <asm/byteorder.h>  //kbulls 100204 

/*----------------------------------------------------------------------------*/


#ifdef	BOOL
#undef	BOOL
#endif
//nskim typedef	unsigned char             BOOL;
typedef	unsigned short            TW2880_COLOR;

typedef struct {
	short		x;
	short		y;
} TW2880_ST_POINT;

typedef struct {
	short		cx;
	short		cy;
} TW2880_ST_SIZE;

typedef struct {
	TW2880_ST_POINT	pt;
	TW2880_ST_SIZE	size;
} TW2880_ST_RECT_WH;

typedef struct {
#if defined(__BIG_ENDIAN_BITFIELD)
	unsigned short			unused     : 11;
	unsigned short			wheel_down : 1;
	unsigned short			wheel_up   : 1;
	unsigned short			btn_right  : 1;
	unsigned short			btn_middle : 1;
	unsigned short			btn_left   : 1;
	TW2880_ST_POINT	point;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
	TW2880_ST_POINT	point;
	unsigned short			btn_left   : 1;
	unsigned short			btn_middle : 1;
	unsigned short			btn_right  : 1;
	unsigned short			wheel_up   : 1;
	unsigned short			wheel_down : 1;
	unsigned short			unused     : 11;
#endif
} TW2880_ST_MOUSE;



/***************************************************************

  TW2880 GENERAL DATA TYPE DEFINITION
  
****************************************************************/
#define TW2880_SUCCESS (0)
#define TW2880_FAIL    (-1)

#define TW2880_MAX_CH (35)

#define TW2880_MOTION_CH_CNT (16)
#define TW2880_MOTION_H_CNT  (16)
#define TW2880_MOTION_V_CNT  (12)

#define	TW2880_SPOT_SEQUENCE_NUM	16				//sequence 화면의 총 갯수
#define    TW2880_SPOT_NUM_OUTPUT_CHANNEL     4

#define TW2880_OSG_WIDTH  (1920)
#define TW2880_OSG_HEIGHT  (1080)

typedef enum
{
	TW2880_SCR_DIV_MODE_NULL = 0,
	TW2880_SCR_DIV_MODE_1,
	TW2880_SCR_DIV_MODE_4,
	TW2880_SCR_DIV_MODE_6,
	TW2880_SCR_DIV_MODE_8,
	TW2880_SCR_DIV_MODE_9,
	TW2880_SCR_DIV_MODE_16,
	TW2880_SCR_DIV_MODE_20,
	TW2880_SCR_DIV_MODE_35,
//	TW2880_SCR_DIV_MODE_8,   // = 8
	// extention // [ v2.02.05 ]
	TW2880_SCR_DIV_FREETYPE_MASK		= 0x80,
	TW2880_SCR_DIV_MODE_ITX_17	= (TW2880_SCR_DIV_FREETYPE_MASK | 0x03),	// 17 division
	
	TW2880_SCR_DIV_DEFAULT = TW2880_SCR_DIV_MODE_20,
} tw2880_scr_div_mode_e;

typedef enum
{
  TW2880_LIVE_MODE = 1,
  TW2880_PB_MODE = 2,
  
} tw2880_mode_e;

typedef enum
{
	TW2880_SCR_MODE_NULL		= 0x0001,
	TW2880_SCR_1280x1024		= 0x0002,
	TW2880_SCR_1680x1050		= 0x0004,
	TW2880_SCR_1920x1080		= 0x0008,
	TW2880_SCR_1440x900		= 0x0010,
	TW2880_SCR_1600x900		= 0x0020,
	TW2880_SCR_1024x768		= 0x0040,
	TW2880_SCR_1280x720		= 0x0080,
	TW2880_SCR_800x600		= 0x0100,
	TW2880_SCR_MODE_DEFAULT = TW2880_SCR_1280x1024
} tw2880_screen_mode_e;

typedef enum {
	TW2880_RECORD_NOT,
	TW2880_RECORD_D1,
	TW2880_RECORD_D1_FIELD,
	TW2880_RECORD_HD1,
	TW2880_RECORD_CIF,	// CIF should always be progressive(Frame Interleave).				27Mhz 4 port 16ch quad
	TW2880_RECORD_CIF_FIELD, // CIF should always be field.								27Mhz 4 port 16ch quad
	TW2880_RECORD_CIF_27_2, // CIF should always be progressive(Frame Interleave).			27Mhz 2 port 16ch quad
	TW2880_RECORD_CIF_27_4, // CIF should always be progressive(Frame Interleave).			27Mhz 4 port 16ch quad
	TW2880_RECORD_1120, // D1(640x480) should always be progressive(Frame Interleave).		148Mhz 2 port 12ch 6div
	TW2880_RECORD_601_D4,
	TW2880_CASCADE_D1_27_4CH,	// 2865 27Mhz 1 ch + 9919 27Mhz 1 ch
	TW2880_CASCADE_D1_27_16CH, // 2865 27Mhz 1 ch + 9919 27Mhz 1 ch
	TW2880_CASCADE_D1_108_4CH, // 2865 108Mhz 4 ch 
	TW2880_CASCADE_D1_108_16CH, // 2865 108Mhz 4 ch 
	TW2880_LIVE_PB_D1_4CH,
	TW2880_LIVE_PB_D4_4CH,
	TW2880_TEST_RECORD_NONREAL_D1_16,
	TW2880_TEST_RECORD_NONREAL_D1_8,
	TW2880_TEST_RECORD_CIF_SW_16,	// [ v2.02.03 ]
	TW2880_TEST_RECORD_CIF_SPOT_16,	// [ v2.02.04 ]
	TW2880_TEST_RECORD_BYTE_D1_8,	// [ v2.03.00 ] // 27Mhz clock, 54Mhz Data D1 2channel byte Interleaved of 1 pin
	TW2880_RECORD_MODE_MAX
}tw2880_record_mode_e;

typedef enum
{
	PBPIN_MODE_NC,
	PBPIN_MODE_BT601,
	PBPIN_MODE_BT1120
} tw2880_pbpin_mode_e;

typedef enum
{
	SNF_1648,
	SNF_0824,
	HATM_2060,
	PNF_0000,
} tw2880_project_e;


/***************************************************************

  TW2880 LIVE DISPLAY DATA TYPE DEFINITION
  
****************************************************************/
typedef struct 
{
  tw2880_scr_div_mode_e div_mode;
  unsigned char ch_arr[TW2880_MAX_CH];  
  char covert_ch[TW2880_MAX_CH];
  char freeze_ch[TW2880_MAX_CH];  
  int  reserved;
} tw2880_live_param_type;




/***************************************************************

  TW2880 MOTION DECTION DATA TYPE DEFINITION
  
****************************************************************/
typedef struct 
{
	char onoff;
	unsigned char sensitivity; // 0~ 0x 1F
	unsigned char check_interval;  // 0 ~ 63(0x3F)
	char area[TW2880_MOTION_V_CNT][TW2880_MOTION_H_CNT];
	int  reserved;  
} tw2880_motion_ch_param_type;

typedef struct {
	unsigned char	r;
	unsigned char	g;
	unsigned char	b;
	unsigned char	none;
} tw2880_color;

typedef struct 
{
	char disp_onoff;
	tw2880_color disp_color;  
	tw2880_motion_ch_param_type ch_info[TW2880_MOTION_CH_CNT];
	int  reserved;
} tw2880_motion_param_type;


typedef struct
{
#if 1 //kbulls 100707
	unsigned short dection_flag;  // bit flag
#else
	char dection_flag[TW2880_MOTION_CH_CNT];
#endif
	char dection_data[TW2880_MOTION_CH_CNT][TW2880_MOTION_V_CNT][TW2880_MOTION_H_CNT];
} tw2880_motion_data_type;



/***************************************************************

  TW2880 PLAYBACK DISPLAY DATA TYPE DEFINITION
  
****************************************************************/
typedef struct 
{
	tw2880_scr_div_mode_e div_mode;
	unsigned char ch_arr[TW2880_MAX_CH];  
	int  reserved;
} tw2880_pb_param_type;




/***************************************************************

  TW2880 SPOP DATA TYPE DEFINITION
  
****************************************************************/
typedef struct 
{
	unsigned int	spot_num;			//0:1번 포트, 1:2번 포트, 2:3번 포트, 3:4번 포트
	unsigned char	enable_f;			//0 : spot disable, 1 : spot enable
	unsigned int	div;			// 1,4,15 : div number
	unsigned char channel[TW2880_SPOT_SEQUENCE_NUM];		//각 시퀀서 화면의 display 될 화면 설정  채널 7번 비트는 covert를 나타내고 하위 4비트는 채널 번호를 나타냄
} tw2880_spot_param_type;




/***************************************************************

  TW2880 MOUSE DATA TYPE DEFINITION
  
****************************************************************/
typedef struct {
	unsigned short	x;
	unsigned short	y;
	unsigned short	w;
	unsigned short	h;
} tw2880_update_rect;

typedef struct {
	unsigned char	win;
	unsigned char	win_sub;
	tw2880_update_rect	rect;
	tw2880_color		color;
} tw2880_fillrect;



/***************************************************************

  TW2880 INITIALIZE CONFIG DEFINITION
  
****************************************************************/
typedef struct 
{
	TW2880_COLOR SrcColor;
	TW2880_COLOR DstColor;
} tw2880_color_convert_type;

typedef struct {
	unsigned char	y;
	unsigned char	u;
	unsigned char	v;
} tw2880_color_yuv;

typedef struct {
	tw2880_color_yuv	v1;
	tw2880_color_yuv	v2;
	tw2880_color_yuv	v3;
} tw2880_font_color_palette;

typedef struct 
{
	tw2880_color_yuv	bdry_color;   // yuv color
	tw2880_font_color_palette font_palette;   // yuv color

	TW2880_ST_POINT com_str;      // D1 사이즈 기준.
	TW2880_ST_POINT ch_str1;      // D1 사이즈 기준.
	TW2880_ST_POINT ch_str2;      // D1 사이즈 기준.
} tw2880_spot_param;

typedef struct 
{
	tw2880_project_e	pjt_name;
	tw2880_screen_mode_e output_resolution;
	unsigned char isNTSC;
	tw2880_color_convert_type color_convert[4];
	tw2880_spot_param spot_param;    // spot 관련 초기설정.
	unsigned char reserved;
} tw2880_init_config_type;


/*----------------------------------------------------------------------------*/





/***************************************************************

  TW2880 GENERNAL API DECLARATION 

****************************************************************/
extern int itx_tw2880_init(tw2880_init_config_type tw2880_cfg);
extern int itx_tw2880_get_env( unsigned int *monitor_sel, unsigned int *ddc_width, unsigned int *ddc_height);


extern void itx_tw2880_change_monitor(unsigned char monitor);


/***************************************************************

  LIVE DISPLAY API DECLARATION

****************************************************************/
extern int itx_tw2880_live_start( tw2880_live_param_type param);

extern int itx_tw2880_live_change( tw2880_live_param_type param);

extern int itx_tw2880_live_stop( void );

#define TW2880_LIVE_CAM(ch)	(ch)
#define TW2880_IP_CAM(ch)		(ch+16)

extern int		itx_tw2880_pb_ch_disable( void );


/***************************************************************

  Channel boundary API DECLARATION

****************************************************************/
extern int		itx_tw2880_channel_boundary_enable( unsigned char enable );
extern int		itx_tw2880_channel_boundary_set_width( unsigned char width );
extern int		itx_tw2880_channel_boundary_set_color( unsigned char r, unsigned char g, unsigned char b );

//extern int		itx_tw2880_zoom( unsigned int zoom_level );
extern int		itx_tw2880_zoom( unsigned int sx, unsigned int sy, unsigned int dx, unsigned int dy );

/***************************************************************

  MOTION DETECTION API DECLARATION

****************************************************************/
extern int itx_tw2880_set_motion_detection(tw2880_motion_param_type param);

extern int itx_tw2880_get_motion_detection(tw2880_motion_data_type *param);





/***************************************************************

  PLAYBACK DISPLAY API DECLARATION

****************************************************************/
extern int itx_tw2880_pb_start( tw2880_pb_param_type param);

extern int itx_tw2880_pb_change( tw2880_pb_param_type param);

extern int itx_tw2880_pb_stop( void );





/***************************************************************

  SPOT API DECLARATION

****************************************************************/
extern int itx_tw2880_set_spot(tw2880_spot_param_type param);
extern int itx_tw2880_set_spot_osd_com_str(unsigned char *com_str);
extern int itx_tw2880_set_spot_osd_ch_str1(unsigned char ch, unsigned char *ch_str1);
extern int itx_tw2880_set_spot_osd_ch_str2(unsigned char ch, unsigned char *ch_str2);


/***************************************************************

  MOUSE API DECLARATION

****************************************************************/
extern int	itx_tw2880_mouse_init( char scanmouse );
extern int	itx_tw2880_mouse_scan(TW2880_ST_MOUSE * pMouse);
extern int	itx_tw2880_mouse_setactivecursor( unsigned char cursornum );
extern int	itx_tw2880_mouse_inactivecursor( void );
extern int	itx_tw2880_mouse_set_screen_size( TW2880_ST_SIZE size_main, TW2880_ST_SIZE size_dual );
#if 0 //kbulls 100526
extern int	itx_tw2880_mouse_set_mouse_pos( unsigned char monitor, TW2880_ST_POINT point );
#else
extern int	itx_tw2880_mouse_set_mouse_pos( TW2880_ST_POINT point );
#endif


/***************************************************************

  FRAME BUFFER API DECLARATION

****************************************************************/
extern int itx_tw2880_fb_update_rect(int fd, tw2880_update_rect * prect);
extern int itx_tw2880_fb_update_rectfill(int fd, tw2880_fillrect * prectfill);
	
#endif /* __TW2880_API_H__ */
