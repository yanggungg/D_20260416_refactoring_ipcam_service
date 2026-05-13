#include <stdio.h>
#include <string.h>
#include <unistd.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h> 
// #include <gst/nf/gstnfappsrc.h>
// #include <gst/nf/gstnfappsink.h>
// #include <gst/nf/gstnfbuddybuffer.h>
// #include <gst/nf/gstnflistbuffer.h>
#include <gobjmedia.h>
#include <gobjmrtpsrc.h>
// #include <gst/base/gstbasetransform.h>      
#include <gobjmrtppipe.h>
#include <pthread.h>

// static const gchar *video_caps =
//     "video/x-h264";


G_DEFINE_TYPE (GobjMrtpPipe, gobj_mrtp_pipe, G_TYPE_OBJECT)


void
mrtp_pipe_process(GobjMrtpPipe *arg)
{
	GobjMrtpPipe *pipe = arg;
	GobjListBuffer *list_buf = NULL;
	int ret = 0;
	// guint64 start, elapsed;

	pipe->is_ready = TRUE;

	while (1) {
#if 0 // for debugging		
		GList *walk;
#endif
		ret = nmf_mrtp_pipe_create_buffer(pipe, &list_buf);
		if (list_buf == NULL) {
			continue;
		}
		if (pipe->is_ready == FALSE) {
			continue;
		}
#if 0 // for debugging
		walk = list_buf->buffer_list;
		while (walk) {
			GobjBuddyBuffer *child_buf = walk->data;
			if (GOBJ_IS_BUDDY_BUFFER(child_buf)) {
				guint8 *buf = gobj_buddy_buffer_buf_get_addr(child_buf);
				icodec_header_t *ih = (icodec_header_t *) buf;
				printf("mrtp_pipe_process() ih->chan=%d,codec=%d,frame_size=%d,flag=0x%x,resolution=0x%x\n", 
												ih->chan, ih->codec, ih->frameSize, ih->flags, ih->resolution);
			} else {
				g_message("mrtp_pipe_process() not buddy buffer type!\n");
			}
			walk = g_list_next(walk);
		}
#endif
		if (GOBJ_IS_LIST_BUFFER(list_buf)) {
			gobj_media_push_buffers(GOBJ_ID_IPLIVE, list_buf);
		} else {
			g_message("mrtp_pipe_process() not list buffer type!\n");
		}

	}

}

GobjMrtpPipe *nmf_mrtp_pipe_new(int channel_num)
{
#if 1
	GobjMrtpPipe *h_mrtp_pipe;
	GobjMrtpSrc *src;
	src = gobj_mrtp_src_new();
	h_mrtp_pipe = g_object_new(GOBJ_TYPE_MRTP_PIPE, NULL);
	h_mrtp_pipe->src = src;

	h_mrtp_pipe->is_ready = FALSE;
	g_thread_create( (GThreadFunc)mrtp_pipe_process, h_mrtp_pipe, FALSE, NULL);

	return h_mrtp_pipe;
#else
	GobjMrtpPipe *h_mrtp_pipe;
	gint max_ch;

	max_ch = channel_num;
	h_mrtp_pipe = g_new0(GobjMrtpPipe,1);
	g_return_val_if_fail(h_mrtp_pipe != NULL, NULL);

	h_mrtp_pipe->is_ready = FALSE;
	h_mrtp_pipe->h_thread = g_thread_create_full (mrtp_pipe_thread, 
													h_mrtp_pipe,
													0, 
													TRUE,
													TRUE, 
													G_THREAD_PRIORITY_HIGH,
													NULL);
	g_return_val_if_fail(h_mrtp_pipe->h_thread != NULL, NULL);
	
	while(h_mrtp_pipe->is_ready != TRUE)
		usleep(1000);

	h_mrtp_pipe->is_ready = TRUE;
	return h_mrtp_pipe;
#endif
#if 0
fail:
	if(h_mrtp_pipe)
		g_free(h_mrtp_pipe);
	return NULL;
#endif
}

/* object finalizer */
static void
gobj_mrtp_pipe_finalize (GObject *object)
{	
	G_OBJECT_CLASS (gobj_mrtp_pipe_parent_class)->finalize (object);
}

/* class initializer */
static void
gobj_mrtp_pipe_class_init (GobjMrtpPipeClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = gobj_mrtp_pipe_finalize;
	g_print("gobj_mrtp_src_class_init()\n");
}

/* object initializer */
static void
gobj_mrtp_pipe_init (GobjMrtpPipe *self)
{
	g_print("gobj_mrtp_pipe_init()\n");
}

int nmf_mrtp_pipe_start(GobjMrtpPipe *h_mrtp_pipe)
{
	// GstState state;
    // gst_element_set_state (h_mrtp_pipe->pipeline, GST_STATE_PLAYING);
    // gst_element_get_state (h_mrtp_pipe->pipeline, &state, NULL, 10000*GST_MSECOND);    //20081104 for fast change call B

	return 0;
}
int nmf_mrtp_pipe_stop(GobjMrtpPipe *h_mrtp_pipe)
{
	// GstState state;
	// gst_element_set_state (h_mrtp_pipe->pipeline, GST_STATE_NULL);
	// gst_element_get_state (h_mrtp_pipe->pipeline, &state, NULL, 10000*GST_MSECOND);

	return 0;
}


void nmf_mrtp_pipe_delete(GobjMrtpPipe *h_mrtp_pipe)
{
	g_return_if_fail(h_mrtp_pipe != NULL);
	
	/* TODO flush buffer */

	// g_object_unref (h_mrtp_pipe->src);
	// g_object_unref (h_mrtp_pipe->sink);
	g_free(h_mrtp_pipe);
}

GObject *nmf_mrtp_pipe_get_src(GobjMrtpPipe *h_mrtp_pipe)
{
	return (GObject *)h_mrtp_pipe->src;
}

// void nmf_mrtp_pipe_set_src(GobjMrtpPipe *h_mrtp_pipe, GobjMrtpSrc *src)
// {
// 	if (h_mrtp_pipe->src == NULL) {
// 		h_mrtp_pipe->src = src;
// 	} 
// }

// GObject *nmf_mrtp_pipe_get_nfappsink(GobjMrtpPipe *h_mrtp_pipe)
// {
// 	return (GObject *)h_mrtp_pipe->sink;
// }

int nmf_mrtp_pipe_open_ch(GobjMrtpPipe *h_mrtp_pipe, NMFMrtpPipeChannel *info)
{
	return gst_mrtp_src_open_ch((GobjMrtpSrc*)h_mrtp_pipe->src, (GobjMrtpSrcChannel*)info, NULL);
}

int nmf_mrtp_pipe_update_live_time(GobjMrtpPipe *h_mrtp_pipe, guint ch_num, guint p_time)
{
	return gst_mrtp_src_update_live_time((GobjMrtpSrc*)h_mrtp_pipe->src, ch_num, p_time, NULL);
}

int nmf_mrtp_pipe_close_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch_num)
{
	return gst_mrtp_src_close_ch((GobjMrtpSrc*)h_mrtp_pipe->src, ch_num, NULL);
}

#if 0
void nmf_mrtp_pipe_open_ch(GobjMrtpPipe *h_mrtp_pipe,
					gint ch, gint stream, guint16 rtsp_port, guint ipaddr,
					guint fps, guint resolution,
					gchar* rtsp_addr, gchar* user, gchar* pass,
					gint model)
{
	gint rtn = 0;
	GstElement *mrtp_source = NULL;

	mrtp_source = (GstElement*) nmf_mrtp_pipe_get_src(h_mrtp_pipe);
	g_object_set (G_OBJECT(mrtp_source), 
					"channel", ch,
					"stream", stream,
					"port", rtsp_port,
					"ipaddr", ipaddr,
					"fps", fps,
					"resolution", resolution,
					"rtspaddr", rtsp_addr,
					"username", user,
					"password", pass,
					"model", model,
					"open-play", 1,
					NULL);
}

void nmf_mrtp_pipe_close_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream)
{
	gint rtn = 0;
	GstElement *mrtp_source = NULL;

	mrtp_source = (GstElement*) nmf_mrtp_pipe_get_src(h_mrtp_pipe);
	g_object_set (G_OBJECT(mrtp_source),
					"channel", ch,
					"stream", stream,
					"teardown", 1,
					NULL);
}
#endif

void nmf_mrtp_pipe_pause_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream)
{
	GobjMrtpSrc *mrtp_source = NULL;

	mrtp_source = (GobjMrtpSrc *)nmf_mrtp_pipe_get_src(h_mrtp_pipe);
	g_object_set (G_OBJECT(mrtp_source),
					"channel", ch,
					"stream", stream,
					"pause", 1,
					NULL);
}

void nmf_mrtp_pipe_resume_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch, gint stream)
{
	GobjMrtpSrc *mrtp_source = NULL;

	mrtp_source = (GobjMrtpSrc *)nmf_mrtp_pipe_get_src(h_mrtp_pipe);
	g_object_set (G_OBJECT(mrtp_source),
					"channel", ch,
					"stream", stream,
					"resume", 1,
					NULL);
}

void nmf_mrtp_pipe_i_only_req(GobjMrtpPipe *h_mrtp_pipe, guint ch_mask)
{
	GobjMrtpSrc *mrtp_source = NULL;

	mrtp_source = (GobjMrtpSrc *)nmf_mrtp_pipe_get_src(h_mrtp_pipe);
	g_object_set (G_OBJECT(mrtp_source),
					"ionlych", ch_mask,
					NULL);
}

void nmf_mrtp_pipe_set_live_audio_ch(GobjMrtpPipe *h_mrtp_pipe, gint ch)
{
	gst_mrtp_src_set_live_audio_ch(ch);
}

void nmf_mrtp_pipe_set_dev_mac(GobjMrtpPipe *h_mrtp_pipe, gint ch, gchar* mac)
{
	gst_mrtp_src_set_dev_mac(ch, mac);
}


/* User Callback Reg Function */
void nmf_mrtp_pipe_set_cmd_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbCmd cb_cmd_func, gpointer user_data)
{
	gst_mrtp_src_set_cmd_callback((GobjMrtpSrc*)h_mrtp_pipe->src, cb_cmd_func, user_data);
}
void nmf_mrtp_pipe_set_motion_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbMotion cb_motion_func, gpointer user_data)
{
	gst_mrtp_src_set_motion_callback((GobjMrtpSrc*)h_mrtp_pipe->src, (GobjMrtpSrcFxnCbMotion)cb_motion_func, user_data);
}
void nmf_mrtp_pipe_set_alarm_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbAlarm cb_alarm_func, gpointer user_data)
{
	gst_mrtp_src_set_alarm_callback((GobjMrtpSrc*)h_mrtp_pipe->src, cb_alarm_func, user_data);
}
void nmf_mrtp_pipe_set_src_handoff(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbHandoff cb_handoff_func, gpointer user_data)
{
	gst_mrtp_src_set_handoff_callback((GobjMrtpSrc*)h_mrtp_pipe->src, cb_handoff_func, user_data);
}
void nmf_mrtp_pipe_set_src_handoff_audio_fragment(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeFxnCbHandoffAudioFragment cb_handoff_func)
{
	gst_mrtp_src_set_handoff_callback_streamer((GobjMrtpSrc*)h_mrtp_pipe->src, cb_handoff_func);
}
void nmf_mrtp_pipe_set_header_x_callback(GobjMrtpPipe *h_mrtp_pipe, 
                    NMFMrtpPipeFxnCbHeaderX cb_header_x_func, gpointer user_data)
{
	gst_mrtp_src_set_header_x_callback((GobjMrtpSrc*)h_mrtp_pipe->src, (GobjMrtpSrcFxnCbHeaderX)cb_header_x_func, user_data);
}

void nmf_mrtp_pipe_set_onvif_meta_callback(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeFxnCbOnvifMeta cb_onvif_meta_func, gpointer user_data)
{
	gst_mrtp_src_set_onvif_meta_callback((GobjMrtpSrc*)h_mrtp_pipe->src, (GobjMrtpSrcFxnCbOnvifMeta)cb_onvif_meta_func, user_data);
}

int nmf_mrtp_pipe_send_audio(GobjMrtpPipe *h_mrtp_pipe,
					NMFMrtpPipeAudio* audio, gpointer user_data)
{
	gint rtn = 0;
	rtn = gst_mrtp_src_send_audio((GobjMrtpSrc*)h_mrtp_pipe->src, (GobjMrtpSrcAudioT*)audio, user_data);
	return rtn;
}

int nmf_mrtp_pipe_create_buffer(GobjMrtpPipe *h_mrtp_pipe, GobjListBuffer **buffer)
{
	return gst_mrtp_src_create((GobjMrtpSrc*)h_mrtp_pipe->src, buffer);
}
