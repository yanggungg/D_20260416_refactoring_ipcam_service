/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#ifndef __GOBJ_MRTP_PIPE_H__
#define __GOBJ_MRTP_PIPE_H__

#include <glib-object.h>
#include <gobjbuddybuffer.h>
#include <gobjlistbuffer.h>
#include <gobjmrtpsrc.h>

/*
 * Potentially, include other headers on which this header depends.
 */ 
G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define GOBJ_TYPE_MRTP_PIPE (gobj_mrtp_pipe_get_type ())
G_DECLARE_FINAL_TYPE (GobjMrtpPipe, gobj_mrtp_Pipe, GOBJ, MRTP_PIPE, GObject)

// typedef struct _GobjMrtpPipe GobjMrtpPipe;

typedef struct _NMFMrtpPipeInfoMotion NMFMrtpPipeInfoMotion;
typedef struct _NMFMrtpPipeHeaderX NMFMrtpPipeHeaderX;
typedef struct _NMFMrtpPipeAudio NMFMrtpPipeAudio;

typedef void (*NMFMrtpPipeFxnCbCmd)(gint op, gint ch, gint stream, gint err_no, gpointer user_data);
typedef void (*NMFMrtpPipeFxnCbMotion)(NMFMrtpPipeInfoMotion *info, gpointer user_data);
typedef void (*NMFMrtpPipeFxnCbAlarm)(guint ch_mask, gpointer user_data);
typedef void (*NMFMrtpPipeFxnCbHandoff)(GobjBuddyBuffer *buffer, gpointer user_data);
typedef void (*NMFMrtpPipeFxnCbHandoffAudioFragment)(GobjBuddyBuffer *buffer);
typedef void (*NMFMrtpPipeFxnCbHeaderX)(gint ch, guchar* payload, gint len, NMFMrtpPipeHeaderX *rtn_header_x_data, gpointer user_data);
typedef void (*NMFMrtpPipeFxnCbOnvifMeta)(gint ch, guchar* payload, gint len, gpointer user_data);


struct _GobjMrtpPipe {
	GObject parent_instance;

	/* Other members, including private data. */
    /* pointer to data and its size */	
	GThread *h_thread;
	GobjMrtpSrc *src;
	// GstElement *src;
	// GstElement *sink;
	// GstElement *pipeline;
	int channel_num;
	int audio_channel_num;
	gboolean is_ready;
};

struct _NMFMrtpPipeHeaderX
{
	gint aflag;
	gint mflag;
	gint mrd_len;
	gint mrd_width;
	gint mrd_height;
	gint mrd_min;
	gint mrd_max;
	guchar mraw[512];
};

struct _NMFMrtpPipeInfoMotion
{
	gint ch;
	gint stream_num;
	gint width;
	gint height;
	guint timestamp;
	guint timestampl;
	guchar mraw[512];
};


struct _NMFMrtpPipeAudio
{
	gint ch_num;
	gint type;
	GobjBuddyBuffer *buf;
};

typedef struct _NMFMrtpPipeStream NMFMrtpPipeStream;
typedef struct _NMFMrtpPipeChannel NMFMrtpPipeChannel;
struct _NMFMrtpPipeStream
{
	guint codec_type;
	guint resolution;
	guint ip_addr;
	guint rtsp_port;
	gchar *rtsp_addr;
};
struct _NMFMrtpPipeChannel
{
	gint ch_num;

	guint model_code;
	gchar *username;
	gchar *password;

	gint video_cnt;
	NMFMrtpPipeStream video[3];

	gint audio_cnt;
	NMFMrtpPipeStream audio;

	guint rtp_method;
	gboolean metadata_on;
};


GobjMrtpPipe* nmf_mrtp_pipe_new(int channel_num);
int nmf_mrtp_pipe_start(GobjMrtpPipe *h_mrtp_pipe);
int nmf_mrtp_pipe_stop(GobjMrtpPipe *h_mrtp_pipe);
void nmf_mrtp_pipe_delete(GobjMrtpPipe *h_mrtp_pipe);
GObject *nmf_mrtp_pipe_get_src(GobjMrtpPipe *h_mrtp_pipe);
// void nmf_mrtp_pipe_set_src(GobjMrtpPipe *h_mrtp_pipe, GobjMrtpSrc *src);
// GObject *nmf_mrtp_pipe_get_nfappsink(GobjMrtpPipe *h_mrtp_pipe);

int nmf_mrtp_pipe_open_ch(GobjMrtpPipe *h_mrtp_pipe, NMFMrtpPipeChannel* info);
int nmf_mrtp_pipe_close_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch_num);
int nmf_mrtp_pipe_update_live_time(GobjMrtpPipe *h_mrtp_pipe, guint ch_num, guint p_time);

#if 0
void nmf_mrtp_pipe_open_ch(GobjMrtpPipe *h_mrtp_pipe,
					gint ch, gint stream, guint16 rtsp_port, guint ipaddr,
					guint fps, guint resolution,
					gchar* rtsp_addr, gchar* user, gchar* pass,
					gint model);
void nmf_mrtp_pipe_close_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream);
#endif
void nmf_mrtp_pipe_pause_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream);
void nmf_mrtp_pipe_resume_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream);
void nmf_mrtp_pipe_i_only_req(GobjMrtpPipe *h_mrtp_pipe, guint ch_mask);
void nmf_mrtp_pipe_set_live_audio_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch);
void nmf_mrtp_pipe_set_dev_mac(GobjMrtpPipe *h_mrtp_pipe, gint ch, gchar* mac);


/*********************     IP Camera Callback API      *********************/

/* user callback functions */

void nmf_mrtp_pipe_set_cmd_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbCmd cb_cmd_func, gpointer user_data);

void nmf_mrtp_pipe_set_motion_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbMotion cb_motion_func, gpointer user_data);

void nmf_mrtp_pipe_set_alarm_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbAlarm cb_alarm_func, gpointer user_data);

void nmf_mrtp_pipe_set_src_handoff(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbHandoff cb_handoff_func, gpointer user_data);

void nmf_mrtp_pipe_set_src_handoff_audio_fragment(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbHandoffAudioFragment cb_handoff_func);

void nmf_mrtp_pipe_set_header_x_callback(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeFxnCbHeaderX cb_header_x_func, gpointer user_data);

void nmf_mrtp_pipe_set_onvif_meta_callback(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeFxnCbOnvifMeta cb_onvif_meta_func, gpointer user_data);

int nmf_mrtp_pipe_send_audio(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeAudio* audio, gpointer user_data);

int nmf_mrtp_pipe_create_buffer(GobjMrtpPipe *h_mrtp_pipe, GobjListBuffer **buffer);

G_END_DECLS

#endif /* __GOBJ_MRTP_PIPE_H__ */
