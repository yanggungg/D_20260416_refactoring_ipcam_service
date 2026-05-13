// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2011 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2011/12/30 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_colorsel.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/color.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
//captainnn
#include "viewers/objects/nfbutton.h"
#include "vw_colorsel.h"
#include "vw_tools.h"

#include "ivca_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	CSEL_WIN_W		544-180
#define	CSEL_WIN_H		416-120

#define	CSEL_FXD_X		12
#define	CSEL_FXD_Y		52
#define	CSEL_FXD_W		(CSEL_WIN_W - 12 * 2)
#define	CSEL_FXD_H		(CSEL_WIN_H - CSEL_FXD_Y - 12)

#define	CSEL_BTN1_X		((CSEL_FXD_W - 10) / 2 - CSEL_BTN_W)
#define	CSEL_BTN2_X		((CSEL_FXD_W + 10) / 2)
#define	CSEL_BTN_Y		(CSEL_FXD_H - CSEL_FXD_Y)
#define	CSEL_BTN_W		150

#define	CSEL_CUR_W		60
#define	CSEL_CUR_H		60
#define	CSEL_TMPL_W		40
#define	CSEL_TMPL_H		40
#define	CSEL_TMPL_SP	8

#define	N_COLORS		8
#define	NN				4

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFOBJECT *main_wnd;
static NFOBJECT *lb_color_cur;
static NFOBJECT *lb_color[N_COLORS];
static NFOBJECT *btn_ok;
static NFOBJECT *btn_cancel;

static gboolean retval = FALSE;

static GdkColor color_cur = {0, 0xFF00, 0xFF00, 0xFF00};

static GdkColor color_template[N_COLORS] = {
	/*
	{0, 0x1000, 0x1000, 0x1000},
	{0, 0x0000, 0x0000, 0x8000},
	{0, 0x0000, 0x0000, 0xFF00},
	{0, 0x0000, 0x8000, 0x0000},
	{0, 0x0000, 0x8000, 0x8000},
	{0, 0x0000, 0x8000, 0xFF00},
	{0, 0x0000, 0xFF00, 0x0000},
	{0, 0x0000, 0xFF00, 0x8000},
	{0, 0x0000, 0xFF00, 0xFF00},

	{0, 0x8000, 0x0000, 0x0000},
	{0, 0x8000, 0x0000, 0x8000},
	{0, 0x8000, 0x0000, 0xFF00},
	{0, 0x8000, 0x8000, 0x0000},
	{0, 0x8000, 0x8000, 0x8000},
	{0, 0x8000, 0x8000, 0xFF00},
	{0, 0x8000, 0xFF00, 0x0000},
	{0, 0x8000, 0xFF00, 0x8000},
	{0, 0x8000, 0xFF00, 0xFF00},

	{0, 0xFF00, 0x0000, 0x0000},
	{0, 0xFF00, 0x0000, 0x8000},
	{0, 0xFF00, 0x0000, 0xFF00},
	{0, 0xFF00, 0x8000, 0x0000},
	{0, 0xFF00, 0x8000, 0x8000},
	{0, 0xFF00, 0x8000, 0xFF00},
	{0, 0xFF00, 0xFF00, 0x0000},
	{0, 0xFF00, 0xFF00, 0x8000},
	{0, 0xFF00, 0xFF00, 0xFF00}
	*/
	
	{0, 0xFF00, 0x0000, 0x0000},
	//{0, 0xFF00, 0x3F00, 0x0000},
	//{0, 0xFF00, 0x7F00, 0x0000},
	//{0, 0xFF00, 0xBF00, 0x0000},
	{0, 0xFF00, 0xFF00, 0x0000},
	//{0, 0xBF00, 0xFF00, 0x0000},
	//{0, 0x2000, 0x2000, 0x2000},
	
	//{0, 0x7F00, 0xFF00, 0x0000},
	//{0, 0x3F00, 0xFF00, 0x0000},
	{0, 0x0000, 0xFF00, 0x0000},
	//{0, 0x0000, 0xFF00, 0x3F00},
	//{0, 0x0000, 0xFF00, 0x7F00},
	//{0, 0x0000, 0xFF00, 0xBF00},
	//{0, 0x6000, 0x6000, 0x6000},
	
	{0, 0x0000, 0xFF00, 0xFF00},
	//{0, 0x0000, 0xBF00, 0xFF00},
	//{0, 0x0000, 0x7F00, 0xFF00},
	//{0, 0x0000, 0x3F00, 0xFF00},
	{0, 0x0000, 0x0000, 0xFF00},
	//{0, 0x3F00, 0x0000, 0xFF00},
	//{0, 0xa000, 0xa000, 0xa000},
	
	//{0, 0x7F00, 0x0000, 0xFF00},
	//{0, 0xBF00, 0x0000, 0xFF00},
	{0, 0xFF00, 0x0000, 0xFF00},
	//{0, 0xFF00, 0x0000, 0xBF00},
	//{0, 0xFF00, 0x0000, 0x7F00},
	//{0, 0xFF00, 0x0000, 0x3F00},
	{0, 0xFF00, 0xFF00, 0xFF00},
	{0, 0x2000, 0x2000, 0x2000},
};



/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static gboolean
post_mainwin_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_DELETE ) {
	    main_wnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean
post_lb_color_cur_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	int x, y;

	if ( evt->type == GDK_EXPOSE && !nfui_nfobject_is_disabled(obj) ) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);
		gdk_gc_set_rgb_fg_color(gc, &color_cur);
		gdk_draw_rectangle(drawable, gc, TRUE, x, y, obj->width, obj->height);
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean
post_lb_color_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint i, x, y;
	KEYPAD_KID kpid = -1;

	if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS )
		kpid = evt->key.keyval;

	if ( (evt->type == GDK_BUTTON_PRESS &&
			evt->button.button == MOUSE_LEFT_BUTTON) || kpid == KEYPAD_ENTER ) {
		for (i = 0; i < N_COLORS; i++)
			if ( obj == lb_color[i] )
				break;
		if ( i == N_COLORS )
			return FALSE;

		color_cur = color_template[i];
		nfui_signal_emit(lb_color_cur, GDK_EXPOSE, FALSE);
	}
	else if ( evt->type == GDK_2BUTTON_PRESS &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		retval = TRUE;
		nfui_nfobject_destroy(main_wnd);
	}
	else if ( (evt->type == GDK_EXPOSE || evt->type == GDK_LEAVE_NOTIFY) && !nfui_nfobject_is_disabled(obj) ) {
		NFLABEL *lb = (NFLABEL *)obj;

		for (i = 0; i < N_COLORS; i++)
			if ( obj == lb_color[i] )
				break;
		if ( i == N_COLORS )
			return FALSE;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);
		gdk_gc_set_rgb_fg_color(gc, &color_template[i]);
		if ( lb->draw_outline && lb->object.kfocus == NFOBJECT_FOCUS )
			gdk_draw_rectangle(drawable, gc, TRUE, x + 2, y + 2,
					obj->width - 4, obj->height - 4);
		else
			gdk_draw_rectangle(drawable, gc, TRUE, x, y,
					obj->width, obj->height);
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean
post_btn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		retval = obj == btn_ok;
		nfui_nfobject_destroy(main_wnd);
	}
	return FALSE;
}
//captainnn
/*
void HSVtoRGB(int *r,int *g , int *b , int h,int s, int v)
{
	int f;
	long p,q,t;

	if(s ==0){
		*r=*g=*b=v;
			return;

		}

	f = ((h%60)*255)/60;
	h/=60;

	p = (v*(256-s))/256;
	q = (v*(256-(s*f)/256))/256;
	t = (v*(256-(s*(256-f))/256))/256;

	switch(h){
		case 0:
			*r =v;
			*g=t;
			*b=p;
			break;
		case 1:
			*r =q;
			*g=v;
			*b=p;
			break;
		case 2:
			*r =p;
			*g=v;
			*b=t;
			break;
		case 3:
			*r =p;
			*g=q;
			*b=v;
			break;
		case 4:
			*r =t;
			*g=p;
			*b=v;
			break;
		default:
			*r =v;
			*g=p;
			*b=q;
			break;

		}

}
*/
static void
_hsv2rgb(int H, int S, int V, int *a, int *c, int *d)
{
	int R, G, B, I, F, M, N, K;

	if ( S == 0 )
		R = G = B = V;
	else {
		H = (H << 8) / 64;	// H & F: in Q.8
		I = H >> 8;
		F = H & 0xFF;
		M = (V * (128 - S)) / 128;
		N = (V * (128 - ((S * F) >> 8))) / 128;
		K = (V * (128 - ((S * (0x100 - F)) >> 8))) / 128;
		if ( I == 0 ) {R = V; G = K; B = M;}
		else if ( I == 1 ) {R = N; G = V; B = M;}
		else if ( I == 2 ) {R = M; G = V; B = K;}
		else if ( I == 3 ) {R = M; G = N; B = V;}
		else if ( I == 4 ) {R = K; G = M; B = V;}
		else {R = V; G = M; B = N;}
	}
	*a = R;
	*c = G;
	*d = B;
}	/* _hsv2rgb(... */

static void 	_IDX2HSV(int i, int* H, int* S, int* V)							
{														
	(*H) = (i) < IVCA_NRBIN_HSVHIST_C ? (i) * (384/IVCA_NRBIN_HSVHIST_C) : 0;		
	(*S) = (i) < IVCA_NRBIN_HSVHIST_C ? 128 : 0;					
	(*V) = (i) < IVCA_NRBIN_HSVHIST_C ? 255 : (((i) - IVCA_NRBIN_HSVHIST_C) *	(255/IVCA_NRBIN_HSVHIST_G) + (255/IVCA_NRBIN_HSVHIST_G) / 2);					
}

gboolean
ColorSel_Open(NFWINDOW *parent, GdkColor *color, guint x, guint y)
{
	guint i, lx, ly;
	NFOBJECT *main_fixed, *fixed1;

	color_cur = *color;
	if ( x + CSEL_WIN_W >= 1920 )
		x = 1920 - CSEL_WIN_W - 10;
	if ( y + CSEL_WIN_H >= 1080 )
		y = 1080 - CSEL_WIN_H - 10;

	main_wnd = (NFOBJECT *)nftool_create_popup_window(parent, x, y,
			CSEL_WIN_W, CSEL_WIN_H, "COLOR SELECTOR", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_mainwin_cb);

	main_fixed = ((NFWINDOW *)main_wnd)->child;

//captainnn
	fixed1 = vw_fixed_create(main_fixed, -1, 1,
			CSEL_FXD_X, CSEL_FXD_Y, CSEL_FXD_W, CSEL_FXD_H, NULL);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	lb_color_cur = (NFOBJECT *)nfui_nflabel_new("");
	nfui_nfobject_modify_bg(lb_color_cur, NFOBJECT_STATE_NORMAL,
			COLOR_IDX(1));
	nfui_nfobject_set_size(lb_color_cur, CSEL_CUR_W, CSEL_CUR_H);
	nfui_nfobject_use_focus(lb_color_cur, FALSE);
	nfui_nffixed_put((NFFIXED *)fixed1, lb_color_cur,
			(CSEL_FXD_W - CSEL_CUR_W) / 2, CSEL_TMPL_SP);
	nfui_nfobject_show(lb_color_cur);
	nfui_regi_post_event_callback(lb_color_cur, post_lb_color_cur_cb);

	/* Color templates. */
	for (i = 0; i < N_COLORS; i++) {
		lx = (CSEL_FXD_W - CSEL_TMPL_W * NN - CSEL_TMPL_SP * (NN - 1)) / 2 +
				(i % NN) * (CSEL_TMPL_W + CSEL_TMPL_SP);
		ly = CSEL_TMPL_SP * 3 + CSEL_CUR_H +
				(i / NN) * (CSEL_TMPL_H + CSEL_TMPL_SP);
		lb_color[i] = (NFOBJECT *)nfui_nflabel_new("");
		nfui_nfobject_modify_bg(lb_color[i], NFOBJECT_STATE_NORMAL,
				COLOR_IDX(1));
		nfui_nfobject_set_size(lb_color[i], CSEL_TMPL_W, CSEL_TMPL_H);
		nfui_nffixed_put((NFFIXED *)fixed1, lb_color[i], lx, ly);
		nfui_nfobject_show(lb_color[i]);
		nfui_regi_post_event_callback(lb_color[i], post_lb_color_cb);

		
	}

	/* OK, CANCEL buttons. */
	//captainnn
	btn_ok = vw_nmbutton_create(fixed1, "OK", 1, TRUE,
			CSEL_BTN1_X, CSEL_BTN_Y, CSEL_BTN_W, post_btn_cb);
	
	btn_cancel = vw_nmbutton_create(fixed1, "CANCEL", 1, TRUE,
			CSEL_BTN2_X, CSEL_BTN_Y, CSEL_BTN_W, post_btn_cb);

	nfui_make_key_hierarchy((NFWINDOW *)main_wnd);
	for (i = 0; i < N_COLORS; i++)
		if ( color_template[i].red == color->red &&
				color_template[i].green == color->green &&
				color_template[i].blue == color->blue )
			break;
	if ( i < N_COLORS )
		nfui_set_key_focus(lb_color[i], TRUE);
	else
		nfui_set_key_focus(btn_ok, TRUE);

	gtk_main();

	if ( retval == TRUE )
		*color = color_cur;

	//test
	if(0){
		int i,r,g,b,h,v,s;


		
		for(i=0;i<N_COLORS;i++){
			_IDX2HSV(i, &h, &s, &v);
			printf("h %d s %d v %d \n",h,s,v);
			_hsv2rgb(h, s, v, &r,&g,&b);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
		}

		
		/*
		h=0;
		s = 128;
		v=255;
		for(i=0;i<24;i++){
			HSVtoRGB(&r,&g,&b,h,s,v);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
			h+= 15;
		}
		
		h=0;
		s = 0;
		v=32;
			HSVtoRGB(&r,&g,&b,h,s,v);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
			v += 64;
			
			HSVtoRGB(&r,&g,&b,h,s,v);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
			v += 64;
			
			HSVtoRGB(&r,&g,&b,h,s,v);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
			v += 64;
			
			HSVtoRGB(&r,&g,&b,h,s,v);
			printf("h %d s %d v %d r %x g %x b %x \n",h,s,v,r,g,b);
			*/

		
	}

	return retval;
}	/* ColorSel_Open(... */

