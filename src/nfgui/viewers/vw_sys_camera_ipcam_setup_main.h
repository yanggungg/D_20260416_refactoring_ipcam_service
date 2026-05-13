#ifndef _SETUP_IPCAM_SETUP_H_
#define _SETUP_IPCAM_SETUP_H_

#include "objects/nfobject.h"

#define COPY_BTN_X              (0)
#define COPY_BTN_Y              (MENU_V_IPCAMSET_SUBTAB_BTN_Y)
#define COPY_BTN_W              (226)

void init_IPCamSetup_page(NFOBJECT *parent);

gint IPCamSetup_start_preview();

gboolean IPCamSetup_tab_in_handler();
gboolean IPCamSetup_tab_out_handler();

extern NFOBJECT *g_on_off_expose_obj;
extern NFOBJECT *g_on_off_image_obj;
extern gboolean g_toggle[GUI_CHANNEL_CNT];

void init_poe_on_off(gint ch);
void set_camera_toggle_on_off(gint ch);

guint _get_copy_chmask(gint ch);
	
#endif

