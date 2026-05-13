#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/timeb.h>
#include <time.h>
#include <glib.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// #include <gst/nf/gstnfbuddybuffer.h>
#include <gobj.h>
#include <gobjmedia.h>
#include "issm.h"
#include "nf_common.h"
#include "nf_common_util.h"

#include "libsst.h"
#include "nf_codec_header.h"
#include "nf_issm_ctl.h"
#include "nf_issm_ctl_frame.h"


void* sd_get_data_without_ich_func(void *p_data)
{
	GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)p_data;

	// if (buffer->frame)
	// 	return buffer->frame + sizeof(ICODEC_HEADER);

	return (gobj_buddy_buffer_buf_get_addr(p_data) + sizeof(ICODEC_HEADER));
}

void* sd_get_data_with_ich_func(void *p_data)
{
	GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)p_data;

	// if (buffer->frame)
	// 	return buffer->frame;

	return gobj_buddy_buffer_buf_get_addr(p_data);
}

int sd_get_info_func(void *p_data, void *p_ref)
{
	ISSM_CB_GET_FRAME_INFO *frame_info;
	ICODEC_HEADER *ich;

	static guint ich_timestamp = 0;
	static guchar ich_timestampl = 0;
	unsigned int rtp_timestamp;

    if (!GOBJ_IS_BUDDY_BUFFER(p_data))
    {
        unsigned char *dump = (unsigned char *)p_data;
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d] p_addr[%p]\n", __FUNCTION__, __LINE__, p_data);
        printf("[%s][%d] dump\n", __FUNCTION__, __LINE__);
/*
        printf(" 00 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[0], dump[1], dump[2], dump[3], dump[4], dump[5], dump[6], dump[7]);
        printf(" 01 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[8], dump[9], dump[10], dump[11], dump[12], dump[13], dump[14], dump[15]);
        printf(" 02 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[16], dump[17], dump[18], dump[19], dump[20], dump[21], dump[22], dump[23]);
        printf(" 03 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[24], dump[25], dump[26], dump[27], dump[28], dump[29], dump[30], dump[31]);

        GstBuffer *gst_buf = (GstBuffer*)p_data;
        printf("[%s][%d] g_type[%d]\n", __FUNCTION__, __LINE__, gst_buf->mini_object.instance.g_class->g_type);
*/
        return ISSM_ERROR;
    }

	frame_info = (ISSM_CB_GET_FRAME_INFO*)(p_ref);
	ich = (ICODEC_HEADER*)gobj_buddy_buffer_buf_get_addr(p_data);

	frame_info->length			= ich->frame_size;
	frame_info->ch_num			= ich->chan % MAX_ISSM_V_CH;

	// Calculate rtp_timestamp using by ich time val of first frame.
	if (ich_timestamp == 0)
		ich_timestamp = ich->timestamp;

	if (ich_timestampl == 0)
		ich_timestampl = ich->timestampl;

	// Calculate rtp_timestamp.
	if (ich->frame_type == NF_FRAME_TYPE_P || ich->frame_type == NF_FRAME_TYPE_I || ich->codec == NF_CODEC_TYPE_JPEG)
	{
		long long first_timestamp;
		long long current_timestamp;

		first_timestamp = ich_timestamp * 1000LL + ich_timestampl * 5LL;
		current_timestamp = ich->timestamp * 1000LL + ich->timestampl * 5LL;

		if (first_timestamp == current_timestamp)
		{
			rtp_timestamp = 90000;
		}
		else
		{
			rtp_timestamp = (unsigned int)((((current_timestamp - first_timestamp) * 90) + 90000) & 0xFFFFFFFF);
		}
		/*
		if (first_timestamp < current_timestamp)
		{
			rtp_timestamp = (((current_timestamp - first_timestamp) * 90) + 90000) & 0xFFFFFFFF;
		}
		if (first_timestamp > current_timestamp)
		{
			rtp_timestamp = (90000 - ((first_timestamp - current_timestamp) * 90)) & 0xFFFFFFFF;
		}
		*/
	}
	if (ich->frame_type == NF_FRAME_TYPE_AUDIO)
	{
		long long first_timestamp;
		long long current_timestamp;

		first_timestamp = ich_timestamp * 1000LL + ich_timestampl * 5LL;
		current_timestamp = ich->timestamp * 1000LL + ich->timestampl * 5LL;

		if (first_timestamp == current_timestamp)
		{
			rtp_timestamp = 90000;
		}
		else
		{
			rtp_timestamp = (unsigned int)((((current_timestamp - first_timestamp) * 8) + 90000) & 0xFFFFFFFF);
		}
		/*
		if (first_timestamp < current_timestamp)
		{
			rtp_timestamp = (((current_timestamp - first_timestamp) * 8) + 90000) & 0xFFFFFFFF;
		}
		if (first_timestamp > current_timestamp)
		{
			rtp_timestamp = (90000 - ((first_timestamp - current_timestamp) * 8)) & 0xFFFFFFFF;
		}
		*/
	}

	frame_info->rtp_ext_size = 0;
	frame_info->rtp_ext = 0;

	frame_info->rtp_timestamp = rtp_timestamp;
	frame_info->frame_type = ich->frame_type;
	frame_info->ich_sec = ich->timestamp;
	frame_info->ich_msec = ich->timestampl;

	return ISSM_OK;
}

int sd_ref_func(void *p_data)
{
	void *tmp = NULL;
	tmp = g_object_ref(p_data);

	if(tmp == NULL) {
			fprintf(stderr, "- ERROR - [%s:%d] g_object_ref ret is NULL\n", __FUNCTION__, __LINE__);
	}

	return 0;
}

int sd_unref_func(void *p_data)
{
	if (!GOBJ_IS_BUDDY_BUFFER(p_data))
	{
        unsigned char *dump = (unsigned char *)p_data;
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d] p_addr[%p]\n", __FUNCTION__, __LINE__, p_data);
        printf("[%s][%d] dump\n", __FUNCTION__, __LINE__);
/*
        printf(" 00 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[0], dump[1], dump[2], dump[3], dump[4], dump[5], dump[6], dump[7]);
        printf(" 01 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[8], dump[9], dump[10], dump[11], dump[12], dump[13], dump[14], dump[15]);
        printf(" 02 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[16], dump[17], dump[18], dump[19], dump[20], dump[21], dump[22], dump[23]);
        printf(" 03 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[24], dump[25], dump[26], dump[27], dump[28], dump[29], dump[30], dump[31]);

        GstBuffer *gst_buf = (GstBuffer*)p_data;
        printf("[%s][%d] g_type[%d]\n", __FUNCTION__, __LINE__, gst_buf->mini_object.instance.g_class->g_type);
*/
        return 0;
	}

	g_object_unref(p_data);
	return 0;
}
