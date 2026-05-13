/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#ifndef __GOBJ_MRTP_SRC_H__
#define __GOBJ_MRTP_SRC_H__

#include <glib-object.h>

#include <gobjbuddybuffer.h>
#include <gobjlistbuffer.h>
/*
 * Potentially, include other headers on which this header depends.
 */ 
G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define GOBJ_TYPE_MRTP_SRC (gobj_mrtp_src_get_type ())
G_DECLARE_FINAL_TYPE (GobjMrtpSrc, gobj_mrtp_src, GOBJ, MRTP_SRC, GObject)

#define MAX_1BYTES		(0xff)
#define MAX_2BYTES		(0xffff)
#define MAX_4BYTES		(0xffffffff)
#define RTSP_PORT		(554)

typedef struct _GobjMrtpSrcInfoMotion GobjMrtpSrcInfoMotion;
typedef struct _GobjMrtpSrcHeaderX GobjMrtpSrcHeaderX;

typedef struct _GobjMrtpSrcAudioT GobjMrtpSrcAudioT;

typedef struct _GobjMrtpSrcStream GobjMrtpSrcStream;
typedef struct _GobjMrtpSrcChannel GobjMrtpSrcChannel;

typedef void (*GobjMrtpSrcFxnCbCmd)(gint op, gint ch, gint stream, gint err_no, gpointer user_data);
typedef void (*GobjMrtpSrcFxnCbMotion)(GobjMrtpSrcInfoMotion *info, gpointer user_data);
typedef void (*GobjMrtpSrcFxnCbAlarm)(guint ch_mask, gpointer user_data);
typedef void (*GobjMrtpSrcFxnCbHandoff)(GobjBuddyBuffer *buffer, gpointer user_data);
typedef void (*GobjMrtpSrcFxnCbHandoffStreamer)(GobjBuddyBuffer *buffer);
typedef void (*GobjMrtpSrcFxnCbHeaderX)(gint ch, guchar* payload, gint len, GobjMrtpSrcHeaderX *rtn_header_x_data, gpointer user_data);
typedef void (*GobjMrtpSrcFxnCbOnvifMeta)(gint ch, guchar* payload, gint len, gpointer user_data);

struct _GobjMrtpSrc {
    GObject parent_instance;
	
	/* Other members, including private data. */
    /* pointer to data and its size */
	gint ch_num;
	gint stream_num;
	gint port;
	guint ipaddr;
	guint fps;
	guint resolution;
	guint model;
	guint ionlych;
	gchar *location;
	gchar *rtspaddr;
	gchar *username;
	gchar *password;
	gint max_fps;
	guint rtp_method;
	guint codec_type;

	gboolean silent;

    GobjMrtpSrcFxnCbCmd          cmd_callback;
    GobjMrtpSrcFxnCbMotion       motion_callback;
	GobjMrtpSrcFxnCbAlarm		alarm_callback;
    GobjMrtpSrcFxnCbHandoff      handoff_callback;
    GobjMrtpSrcFxnCbHandoffStreamer      handoff_callback_streamer;
	GobjMrtpSrcFxnCbHeaderX		header_x_callback;
	GobjMrtpSrcFxnCbOnvifMeta	onvif_meta_callback;

    gpointer cmd_user_data;
    gpointer motion_user_data;
	gpointer alarm_user_data;
    gpointer handoff_user_data;
	gpointer header_x_user_data;
	gpointer onvif_meta_user_data;
};

struct _GobjMrtpSrcStream
{
	guint codec_type;
	guint resolution;
	guint ip_addr;
	guint rtsp_port;
	gchar *rtsp_addr;
};
struct _GobjMrtpSrcChannel
{
	gint ch_num;

	guint model_code;
	gchar *username;
	gchar *password;

	gint video_cnt;
	GobjMrtpSrcStream video[3];

	gint audio_cnt;
	GobjMrtpSrcStream audio;

	guint rtp_method;
	gboolean metadata_on;
};

struct _GobjMrtpSrcHeaderX
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

struct _GobjMrtpSrcInfoMotion
{
	gint ch_num;
	gint stream_num;
	gint width;
	gint height;
	guint timestamp;
	guint timestampl;
	guchar mraw[512];
};

enum _SEND_AUDIO_TYPE__
{
	AUDIO_START,
	AUDIO_END,
	AUDIO_DATA
};

struct _GobjMrtpSrcAudioT
{
	gint ch_num;
	gint type;
	GobjBuddyBuffer *buf;
};

/*
 * Method definitions.
 */
GobjMrtpSrc *gobj_mrtp_src_new (void);

void gst_mrtp_src_set_live_audio_ch(gint ch);
void gst_mrtp_src_set_dev_mac(gint ch, gchar* mac);
void gst_mrtp_src_set_cmd_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbCmd, gpointer);
void gst_mrtp_src_set_motion_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbMotion, gpointer);
void gst_mrtp_src_set_alarm_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbAlarm, gpointer);
void gst_mrtp_src_set_handoff_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbHandoff, gpointer);
void gst_mrtp_src_set_handoff_callback_streamer(GobjMrtpSrc*, GobjMrtpSrcFxnCbHandoffStreamer);
void gst_mrtp_src_set_header_x_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbHeaderX, gpointer);

void gst_mrtp_src_set_onvif_meta_callback(GobjMrtpSrc*, GobjMrtpSrcFxnCbOnvifMeta, gpointer);

int gst_mrtp_src_send_audio(GobjMrtpSrc*, GobjMrtpSrcAudioT*, gpointer);

int gst_mrtp_src_open_ch(GobjMrtpSrc*, GobjMrtpSrcChannel*, gpointer);
int gst_mrtp_src_close_ch(GobjMrtpSrc*, gint, gpointer);
int gst_mrtp_src_close_all(GobjMrtpSrc*, gpointer);
int gst_mrtp_src_update_live_time(GobjMrtpSrc*, guint, guint, gpointer);

// void gst_mrtp_src_run_cmd_callback();
void gst_mrtp_src_get_recv_rate(gchar*);

int gst_mrtp_src_create (GobjMrtpSrc *src, GobjListBuffer **buffer);

G_END_DECLS

#endif /*__GOBJ_MRTP_SRC_H__*/
