

#ifndef _VW_LIVE_SHORTCUT_MENU_H
#define _VW_LIVE_SHORTCUT_MENU_H

enum {
	PLAYBACK_MENU = 0,
	ZOOM_MENU,
#if 0	// v2.0
	FREEZE_MENU,
#endif
	SNAPSHOT_MENU,
	AUDIO_MENU,
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	MIC_MENU,
#endif	
    PTZ_CTRL,
	ONEPUSH_MENU,
	FISHEYE_CTRL,

	CAM_CHANGE,
	FR_REGIST,
	LPR_REGIST,
	MENU_CNT
};

typedef struct _SHORTCUT_MENU_CONF MENU_CONF;

struct _SHORTCUT_MENU_CONF {
	guint wsize_w;									// window width 
	guint msize_w;									// button width

	GdkPixbuf *menu_img[MENU_CNT][NFOBJECT_STATE_COUNT];	// images	
	GdkPixbuf *submenu_img[3][NFOBJECT_STATE_COUNT];
};


gboolean VW_Create_ShortCut_Menu(NFWINDOW *parent);
int VW_Destroy_ShortCut_Menu();
void VW_ShortCut_Menu_Show(gint x, gint y, guint ch);
gboolean VW_ShortCut_Menu_Is_Shown();
void VW_ShortCut_Menu_Hide();
void VW_ShortCut_Menu_Refresh();
int VW_ShortCut_Menu_Pos(int *x, int *y);
int VW_ShortCut_Menu_Size(int *w, int *h);

/*inline*/ guint get_menu_channel();

NFOBJECT* get_main_parent();
#endif
