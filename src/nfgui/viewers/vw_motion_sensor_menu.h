#ifndef _VW_MOTION_SENSOR_MENU_H
#define _VW_MOTION_SENSOR_MENU_H

enum {
	// channel : 0 ~ 15 
	SELECT_ALL = 16,
	DESELECT_ALL,
	OPEN_CONF,
	SAVE,
	SAVE_N_EXIT,
	CANCEL
};


gint VW_MotionSensorMenu_Open(NFWINDOW *parent, int x, int y);
void VW_MotionSensorMenu_Destroy();
int VW_select_menu_item(int item);
int VW_get_motion_sensor_menu_pos(int *x, int *y);
int VW_get_motion_sensor_menu_size(int *w, int *h);

#endif
