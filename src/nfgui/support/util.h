#ifndef	__UTIL_H__
#define	__UTIL_H__

#include <linux/fs.h>

#include "nf_afx.h"

#include "nf_ui_font.h"
#include "nf_ui_image.h"
#include "nf_ui_color.h"

#define	NF_PANGO_CASHING_KEY_MAX	(1024)
typedef enum {
	NFALIGN_LEFT = 0,
	NFALIGN_CENTER,
	NFALIGN_RIGHT,
	NFALIGN_CENTER_LEFT,
	NFALIGN_CENTER_RIGHT,
	NFALIGN_CENTER_DOWN,
	NFALIGN_LEFT_UP	
} nfalign_type;

typedef struct {
	guint x;
	guint y;
} nfpoint;

typedef struct {
	guint x;
	guint y;
	guint width;
	guint height;
} nfrect;


typedef enum
{
  NORMAL_SPACING = 0,
  SEMI_CONDENSED_SPACING,
  CONDENSED_SPACING,
  EXTRA_CONDENSED_SPACING,
  ULTRA_CONDENSED_SPACING,
}nfutil_pango_spacing_type;

#define SEMI_CONDENSED_SPACING_GAP  ((-1)*(PANGO_SCALE-300))
#define CONDENSED_SPACING_GAP       ((-1)*PANGO_SCALE)
#define EXTRA_CONDENSED_SPACING_GAP ((-1)*(PANGO_SCALE+300))
#define ULTRA_CONDENSED_SPACING_GAP ((-1)*(PANGO_SCALE+500))

#define	DST_UNKNOWN		(-1)
#define	DST_NORMAL_DAY	(0)
#define	DST_DST_DAY		(1)
#define	DST_START_DAY	(2)
#define	DST_END_DAY		(3)

#define MOUSE_LEFT_BUTTON 		1
#define MOUSE_MIDDLE_BUTTON		2
#define MOUSE_RIGTH_BUTTON 		3




void nfutil_nfpoint_set(nfpoint* ptPoint, guint x, guint y);
void nfutil_nfrect_set(nfrect* rtRect, guint x, guint y, guint width, guint height);

void nfutil_draw_pixbuf(GdkDrawable *drawable, GdkGC *gc, GdkPixbuf *pm, gint x, gint y, gint w, gint h, nfalign_type halign, guint margin);
void nfutil_draw_image(GdkDrawable *drawable, GdkGC *gc, const gchar *path, gint x, gint y, gint w, gint h, nfalign_type halign, guint margin);

//void nfutil_draw_text_with_pango(GdkGC *gc, GdkDrawable *drawable, gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin);
gint nfutil_is_text_cashed(const gchar *key);
void nfutil_draw_text_with_pango(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin);
void nfutil_draw_text_with_pango_outline(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin);
void nfutil_draw_text_with_pango_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin);
void nfutil_draw_text_with_pango_outline_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin);


guint nfutil_string_width(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type);
guint nfutil_string_height(GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type);

time_t GET_DST_INTERVAL();
struct tm* NFLOCALTIME(time_t *ttime);
struct tm* NFLOCALTIME_TV(GTimeVal tv);
time_t NFMKTIME(struct tm *stime, gint isdst);

struct tm *LOCALTIME_R(time_t *ttime);

gint nfutil_string_get_valid_count(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, gint drawing_width);
gint nfutil_string_get_valid_count2(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, gint drawing_width);

gint nfutil_get_start_time(GTimeVal *tv);
void nfutil_get_start_datetime(GTimeVal *tv);
gint nfutil_check_DST_day(GTimeVal tv);
guint nfutil_spacing_string_width(GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type);


gboolean nfutil_device_mount(const gchar *device_name, const gchar *mnt_path, const gchar *fs, unsigned long fs_flag);
gboolean nfutil_device_unmount(const gchar *device_name, const gchar *mnt_path);

void nfutil_draw_text_with_pango_spacing(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol,
                                         GdkDrawable *drawable, GdkGC* gc,  
                                         const gchar* str, gint x, gint y, gint w, gint h, 
                                         const gchar *font, GdkColor *font_color, 
                                         nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type);
void nfutil_draw_text_with_pango_spacing_outline(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                                 GdkDrawable *drawable, GdkGC* gc,  
                                                 const gchar* str, gint x, gint y, gint w, gint h, 
                                                 const gchar *font, GdkColor *font_color, 
                                                 nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type);

void nfutil_draw_text_with_pango_spacing_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol,
                                         GdkDrawable *drawable, GdkGC* gc,  
                                         const gchar* str, gint x, gint y, gint w, gint h, 
                                         const gchar *font, GdkColor *font_color, 
                                         nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type);
void nfutil_draw_text_with_pango_spacing_outline_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                                 GdkDrawable *drawable, GdkGC* gc,  
                                                 const gchar* str, gint x, gint y, gint w, gint h, 
                                                 const gchar *font, GdkColor *font_color, 
                                                 nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type);

gint nfutil_pango_get_spacing(nfutil_pango_spacing_type spacing_type);

guint nfutil_not_cashing_string_width(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str);


void nfutil_draw_short_text(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type);
void nfutil_draw_short_text_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type);
void nfutil_draw_short_text_vk(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type);
void nfutil_draw_short_text_eng_vk(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type);

gint nfutil_get_line_feed_string(gchar *in_str, gint layout_w, const gchar *font, gchar *out_str, gint out_str_size);



#define NFUTIL_THREADS_INIT()		gdk_threads_init()
#define	NFUTIL_THREADS_ENTER()		//gdk_threads_enter()
#define	NFUTIL_THREADS_LEAVE()		//gdk_threads_leave()


#endif	// __UTIL_H__

