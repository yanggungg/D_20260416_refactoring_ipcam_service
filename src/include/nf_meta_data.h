#ifndef __NF_META_DATA_H__
#define __NF_META_DATA_H__

//#include <gst/gst.h>
#include "nf_object.h"
#include "nf_common.h"
#include "nf_codec_header.h"

#include "ivca_def.h"
#include "libivcam.h"

#include "nf_record.h"
#include "itx_ai_def.h"

//#include "nf_ipcam_zmq_utils.h"

#define MAX_META_DATA_CH (NUM_ACTIVE_CH + 1)

typedef void (*MatadataCbHandoff)(gint ch, gint cnt, ivcam_obj_t* data);
typedef void (*CounterCbHandoff)(gint ch, gint cnt, ivca_meta_cnt_t* data);

typedef struct _NF_META {
	pthread_t		thread;
	gint			init_done;
	gint			thread_run;
	gint			init;

	GMutex			*lock;
	GAsyncQueue		*queue;

	gint 	pre_rec_time;

	NF_RECORD_MAN		man[NUM_TOTAL_CHANNEL];	
	
	NF_RECORD_SST		sst[NUM_TOTAL_CHANNEL];
	
	void* 		vca_model[MAX_META_DATA_CH];
	ivcam_obj_t 	m_TrackObjs[200];

	gint sched[NUM_ACTIVE_CH];
	gboolean	is_dst;

	MatadataCbHandoff fxn_get_meta_data;
	CounterCbHandoff fxn_get_counter;
} NF_META;

typedef enum {
	META_DATA_DISPLAY_OFF = 0,
	META_DATA_DISPLAY_LIVE_ON = 1,
	META_DATA_DISPLAY_PLAYBACK_ON = 2
}Meta_Data_Display_Mode;

typedef struct _NF_AI_META {
	pthread_t		thread;
	gint			init_done;
	gint			thread_run;
	gint			init;

	GMutex			*lock;
	GAsyncQueue		*queue;

	void* 		ai_model[MAX_META_DATA_CH];
	ai_obj_t 	m_TrackObjs[200];
	ai_fr_obj_t fr_data[MAX_META_DATA_CH];
	ai_lpr_obj_t lpr_data[MAX_META_DATA_CH];

	gint sched[NUM_ACTIVE_CH];
	gboolean	is_dst;
} NF_AI_META;

//*******NF_AI_META_EXT
//RTP EXTENSION SEND STRUCTURE
//STRUCTURE SIZE = 96 BYTE (4 BYTE ALIGN FIX)
typedef struct _NF_AI_META_EXT {
	unsigned int type;					/**< Type of this event: IVCA_ET_XXXX. */
	int object_id;					/**< Object id for this event. */
	short int rule_id;					/**< Rule id for this event. */	
	char object_class[32];   			/**< Object class */	
	double bbx_position[4];			/**< Rectangle(bounding box) of the event region. */
	unsigned int timestamp;			/**< Timestamp of this event. */
     	unsigned int timestampl;			/**< Timestamp of this event. */
	double confidence;				/**< confidence. */
}NF_AI_META_EXT;


void nf_meta_data_init();

int nf_meta_data_analysis(GObject* frame, guint chan);

void nf_meta_data_push(gpointer frame);

int nf_meta_data_display_live_on(int ch, int src);

int nf_meta_data_display_live_off();

int nf_meta_data_display_playback_on();

int nf_meta_data_display_playback_off();

void cb_display_pipe_put_meta_data(GObject* buffer, 
                                gpointer user_data);


int nf_smart_search_set_rule(int chan, ivca_rule_t *rules,time_t from, time_t to);
int nf_smart_search_reset(int chan, ivca_rule_t *rules);


int get_vca_enable(int chan);
int get_ai_enable(int chan);


gchar *nf_vca_event_type_string(guint32 event_type);
gchar *nf_vca_class_type_string(guint8 class_type);
int nf_api_vca_get_event_string(ivca_rule_event_t *evt, gchar *out);

void set_meta_data_callback(MatadataCbHandoff cb_handoff_meta_data , CounterCbHandoff cb_handoff_counter);

int nf_meta_data_get_frame_cnt();
void nf_meta_data_reset_cnt();
void init_gst_buf_meta();
void nf_record_meta_close();
void nf_event_data_func(gpointer *p_frame);

int nf_ai_set_rule(int chan);
int nf_get_counter_value(int chan, int* value);

#endif
