#ifndef	__THUMBNAIL_MANAGER_H__
#define	__THUMBNAIL_MANAGER_H__

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfthumbnail.h"

typedef enum _TH_INST_E {
	TH_OPEN		= 0,
	TH_CLOSE
} TH_INST_E;

typedef enum _TH_STATE_E {
	TH_START		= 0,
	TH_CHANGE,
	TH_STOP
} TH_STATE_E;


void start_thumbnail_manager();
void thumbnail_manager_object_connect(NFTHUMBNAIL *thumbnail);
void thumbnail_manager_obj_disconnect(NFTHUMBNAIL *thumbnail);
void thumbnail_manager_set_inst(TH_INST_E inst);
void thumbnail_manager_set_state(TH_STATE_E state);
void thumbnail_manager_attach_image_buf(NFTHUMBNAIL *thumbnail, guchar *buf);
void thumbnail_manager_attach_ch(NFTHUMBNAIL *thumbnail, gint ch);
void thumbnail_manager_attach_period(NFTHUMBNAIL *thumbnail, time_t start_time, time_t end_time);
gboolean thumbnail_manager_get_result(NFTHUMBNAIL *thumbnail);
gboolean thumbnail_manager_get_covert(NFTHUMBNAIL *thumbnail);
gboolean thumbnail_manager_get_covert_shown_as();
gboolean thumbnail_manager_get_time(NFTHUMBNAIL *thumbnail, time_t *img_tv);
gint thumbnail_manager_get_focused_time(NFTHUMBNAIL *thumbnail, time_t *img_tv);
gboolean thumbnail_manager_is_running();
void thumbnail_manager_own_image_load(NFTHUMBNAIL *thumbnail);
gboolean thumbnail_manager_get_start_time(NFTHUMBNAIL *thumbnail, time_t *img_tv);
void thumbnail_manager_get_period(NFTHUMBNAIL *thumbnail, time_t *start_time, time_t *end_time);


#endif	// __THUMBNAIL_MANAGER_H__


