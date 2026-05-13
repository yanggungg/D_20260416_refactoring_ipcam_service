#ifndef _VW_MOTION_DRAW_H_
#define _VW_MOTION_DRAW_H_

void VW_MotionDraw_init(NFWINDOW *parent);
void VW_MotionDraw_finalize();

gint VW_MotionDraw_set_obj(gint ch, NFOBJECT *mdraw_obj);
gint VW_MotionDraw_set_plt_position(gint pos_x, gint pos_y, gint width, gint height);
gint VW_MotionDraw_set_selectArea(guchar area[1024]);

gint VW_MotionDraw_set_daytime(gint start, gint end);
gint VW_MotionDraw_set_sense(gint day_sense, gint night_sense);

gint VW_MotionDraw_start();
gint VW_MotionDraw_stop();
#endif
