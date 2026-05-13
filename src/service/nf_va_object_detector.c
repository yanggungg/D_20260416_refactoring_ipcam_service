#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <glib.h>
#include <math.h>
#include "nf_api_live.h"
#include "libitxdnn.h"
#include "nf_va_object_detector.h"
#include "nf_ipcam_defs.h"
#include "nf_api_eventlog.h"
// #include "nmf_display.h"
#include "nf_dva_event.h"

struct detect_time
{
	long time;
};

pthread_mutex_t mutex_va_ch_mask = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_va_event_flag = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_va_running = PTHREAD_MUTEX_INITIALIZER;
static unsigned int va_channel_mask = 0x00000000;
static unsigned int va_setup_flag = 0;
static int va_event_flag = 1;
static int va_running = 1;

static struct detect_time detect_times[AVAILABLE_MAX_CH] = {{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};

static nf_va_object_detector_callback result_callback = NULL;

static struct bbx objdetect_roi[AVAILABLE_MAX_CH];
static itxdnn_detector_t *detector;

static int get_roi_from_sysdb(unsigned int channel, struct bbx *data)
{
	int rtn = 1;
	char buf[128];
	GValue ret_value = {0,};

	memset(buf, 0x00, sizeof(buf));
	if (nf_sysdb_get_key1("cam.dva.D%d.roi", channel, &ret_value, NULL))
	{
		if (g_value_get_string(&ret_value))
		{
			//printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, g_value_get_string(&ret_value));
			sscanf(g_value_get_string(&ret_value), "%f %f %f %f", &(data->top).x, &(data->top).y, &(data->bottom).x, &(data->bottom).y);
			//printf("[%s:%d] %.3f, %.3f, %.3f %.3f\n", __FUNCTION__, __LINE__, data->top.x, data->top.y, data->bottom.x, data->bottom.y);
		}
		else
		{
			data->top.x = 0.0;
			data->top.y = 0.0;
			data->bottom.x = 1.0;
			data->bottom.y = 1.0;
		}
		g_value_unset(&ret_value);

		rtn = 0;
	}
	else
	{
		data->top.x = 0.0;
		data->top.y = 0.0;
		data->bottom.x = 1.0;
		data->bottom.y = 1.0;
	}

	return rtn;
}

static unsigned char *rgb888p_convert(const char *data, int iw, int ih)
{
	int w = iw;
	int h = ih; 
	int c = 3;

	// convert to planar
	int i, j, k;
	unsigned char *planar = malloc((size_t)(w * h * c));
	if ( planar ) {
		for (k = 0; k < c; k++) {
			for (j = 0; j < h; j++)
				for (i = 0; i < w; i++) {
					planar[w * h * k + w * j + i] = (unsigned char)data[w * c * j + c * i + k];
				}
		}
	}

	return planar;
}

static int object_detect(itxdnn_detector_t *detector, unsigned char *img)
{
	/*
	struct timespec request;
	request.tv_sec = 0;
	request.tv_nsec = 300000000L; // 300ms
	clock_nanosleep(CLOCK_MONOTONIC, 0, &request , NULL);
	*/

	int n;
	//ksi_test
	// n = itxdnn_detector_predict(detector, img, 300, 300);

	return n;
}

static float va_object_detector_correction(float value)
{
	if(value < 0.0f) return 0.0f;
	if(value > 1.0f) return 1.0f;
	return value;
}

static int va_object_detector(itxdnn_detector_t *detector, VaImageCallbackInfo *frame, struct objects *objs)
{
	int obj_num, i;
	int valid_obj_num = 0;
	struct timespec sta, end;
	unsigned char *tmp_bmp = NULL;
	int ch = 0;

	clock_gettime(CLOCK_MONOTONIC , &sta);


	tmp_bmp = rgb888p_convert((char*)frame->bmp, (int)frame->bmp_width, (int)frame->bmp_height);
	if(tmp_bmp == NULL)  return -1;

	obj_num = object_detect(detector, tmp_bmp);
	free(tmp_bmp);

	clock_gettime(CLOCK_MONOTONIC , &end);

	ch = frame->ch;
	
	long sta_msec = (sta.tv_sec * 1000) + (sta.tv_nsec/1000/1000);
	long end_msec = (end.tv_sec * 1000) + (end.tv_nsec/1000/1000);
	printf("\e[32m-- DLVA --\e[0m [%s:%d] uptime [%ld.%09ld] Interval: %ldms Processing Time:\e[31m%03ldms\e[0m\n", __FUNCTION__, __LINE__, 
			sta.tv_sec, sta.tv_nsec, 
			sta_msec - detect_times[ch].time,
			end_msec - sta_msec);

	detect_times[ch].time = sta_msec;

	for (i = 0; i < obj_num; i++) 
	{
		itxdnn_detector_result_t *r = &detector->res[i];

#if 0
		printf("\e[33m [%2d] %-11s(%02d): %.4f: [%.3f, %.3f, %.3f, %.3f]\e[0m\n", i,
				detector->names[r->classid], r->classid, r->confidence,
				r->bb[0], r->bb[1], r->bb[2], r->bb[3]);
#endif
	}

	if(obj_num > MAX_DETECT_OBJECTS)
		obj_num = MAX_DETECT_OBJECTS;

	valid_obj_num = 0;
	for (i = 0; i < obj_num; i++) 
	{
		itxdnn_detector_result_t *r = &detector->res[i];

		strncpy(objs->obj[valid_obj_num].name, detector->names[r->classid], 64);
		objs->obj[valid_obj_num].id = r->classid;
		objs->obj[valid_obj_num].confidence = r->confidence;
		objs->obj[valid_obj_num].bbx.top.x = va_object_detector_correction(r->bb[0]);
		objs->obj[valid_obj_num].bbx.top.y = va_object_detector_correction(r->bb[1]);
		objs->obj[valid_obj_num].bbx.bottom.x = va_object_detector_correction(r->bb[2]);
		objs->obj[valid_obj_num].bbx.bottom.y = va_object_detector_correction(r->bb[3]);
		
		++valid_obj_num;
	}

	objs->obj_num = valid_obj_num;

	return valid_obj_num;
}

static void* nf_va_object_detector_queuing_process(void *var)
{
	int ch = 0; 
	int obj_num = 0;
	int queue_cnt = 0;
	int dlva_license = 1;
	struct objects *tmp_objects = NULL;
	VaImageCallbackInfo *frame = NULL;
	static GAsyncQueue *va_queuing_queue = NULL;
	NF_SYSDB_LICENSE_INFO license_info;

	tmp_objects = malloc(sizeof(struct objects));

	va_queuing_queue = (GAsyncQueue *)var;

	while(1)
	{
		frame = g_async_queue_pop(va_queuing_queue);

		//dlva_license = nf_sysdb_license_get_from_sysdb( "DLVA", &license_info);
		//printf("-- DLVA -- [%s:%d] license(%d)\n", __FUNCTION__, __LINE__, dlva_license);

		if(frame == NULL)
			continue;

		if( frame->jpeg_size > MAX_JPEG_SIZE )
		{
		    printf("[%s:%d] jpeg data size big:%d ! drop frame !\n", __FUNCTION__, __LINE__, frame->jpeg_size);
		    if(frame != NULL)
		    {
				if(frame->bmp != NULL)
				{
					free(frame->bmp);
					frame->bmp = NULL;
				}
				if(frame->jpeg != NULL)
				{
					free(frame->jpeg);
					frame->jpeg = NULL;
				}

				free(frame);
				frame = NULL;
                  }
		    continue;
		}

		ch = frame->ch;

		//printf("-- DLVA -- [%s:%d] CH(%d) invalid_ch_flag(%d) result_callback(%p) frame->bmp(%p) frame->bmp_width(%d)\n", __FUNCTION__, __LINE__, ch, 
		//		invalid_ch_flag, result_callback, frame->bmp, frame->bmp_width);

		if(result_callback != NULL && frame->bmp != NULL && frame->bmp_width != 0)
		{
			int i;
			float width, height;
			unsigned int itx_mask;
			unsigned int is_itx;
			struct objects *objs = malloc(sizeof(struct objects));

			memset(objs, 0x00, sizeof(struct objects));
			memset(tmp_objects, 0x00, sizeof(*tmp_objects));


			itx_mask = nf_ipcam_get_supported_dlva_ch_mask();

			is_itx = (itx_mask & (unsigned int)(1 << ch));
		       //printf("-- DLVA -- [%s:%d] CH(%d) is_itx(%d) va_running(%d) license(%d)\n", __FUNCTION__, __LINE__, ch,
				//is_itx, va_running, dlva_license);

			if(is_itx && (va_running == 1) && dlva_license)
			{
				obj_num = va_object_detector(detector, frame, tmp_objects);

				width = objdetect_roi[ch].bottom.x - objdetect_roi[ch].top.x;
				height = objdetect_roi[ch].bottom.y - objdetect_roi[ch].top.y;
				//printf("\e[32m-- DLVA --\e[0m [%s:%d] CH(%d) ROI TOP(%.3f, %.3f) BOTTOM(%.3f, %.3f) CNT(%d)\n", __FUNCTION__, __LINE__, ch, 
				//		objdetect_roi[ch].top.x,
				//		objdetect_roi[ch].top.y,
				//		objdetect_roi[ch].bottom.x,
				//		objdetect_roi[ch].bottom.y,
				//		tmp_objects->obj_num);

				for(i = 0; i < tmp_objects->obj_num; i++)
				{
					tmp_objects->obj[i].bbx.top.x = objdetect_roi[ch].top.x + (width * tmp_objects->obj[i].bbx.top.x);
					tmp_objects->obj[i].bbx.top.y = objdetect_roi[ch].top.y + (height* tmp_objects->obj[i].bbx.top.y);
					tmp_objects->obj[i].bbx.bottom.x = objdetect_roi[ch].top.x + (width * tmp_objects->obj[i].bbx.bottom.x);
					tmp_objects->obj[i].bbx.bottom.y = objdetect_roi[ch].top.y + (height * tmp_objects->obj[i].bbx.bottom.y);
					printf("\e[34m [%d][%2d] %-11s(%02d): %.4f: [%.3f, %.3f, %.3f, %.3f]\e[0m\n", ch,  i,
							detector->names[tmp_objects->obj[i].id], tmp_objects->obj[i].id, tmp_objects->obj[i].confidence,
							tmp_objects->obj[i].bbx.top.x,
							tmp_objects->obj[i].bbx.top.y,
							tmp_objects->obj[i].bbx.bottom.x,
							tmp_objects->obj[i].bbx.bottom.y);
				}
			}

			if(objs != NULL)
			{
				tmp_objects->ch = ch;
				tmp_objects->time = frame->time;
				tmp_objects->timel = frame->timel;

				tmp_objects->img.width = frame->jpeg_width;
				tmp_objects->img.height = frame->jpeg_height;
				tmp_objects->img.size = frame->jpeg_size;

				memcpy(objs, tmp_objects, sizeof(*tmp_objects));
				if(frame->jpeg != NULL)
				{
					memcpy(objs->img.data, frame->jpeg, (size_t)frame->jpeg_size);
				}

				result_callback(objs);

				free(objs);
			}
		}
		else
		{
			printf("\e[32m-- DLVA --\e[0m [%s:%d] frame(%p) frame->bmp(%dx%d:%p) frame->jpeg(%dx%d:%p) dlva_license(%d)\n", __func__, __LINE__, 
					frame,
					frame->bmp_width, frame->bmp_height, frame->bmp, 
					frame->jpeg_width, frame->jpeg_height, frame->jpeg, dlva_license);
		}

		if(frame != NULL)
		{
			if(frame->bmp != NULL)
			{
				free(frame->bmp);
				frame->bmp = NULL;
			}
			if(frame->jpeg != NULL)
			{
				free(frame->jpeg);
				frame->jpeg = NULL;
			}

			free(frame);
			frame = NULL;
		}
	}

	return NULL;
}

int nmf_display_set_va_roi(unsigned int ch, float top_x, float top_y, float bottom_x, float bottom_y);
void nf_va_object_detector_update_roi()
{
	unsigned int i;
	for(i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		get_roi_from_sysdb(i, &objdetect_roi[i]);

		if((objdetect_roi[i].top.x < 0.0f || objdetect_roi[i].top.x > objdetect_roi[i].bottom.x) ||
				(objdetect_roi[i].top.y < 0.0f || objdetect_roi[i].top.y > objdetect_roi[i].bottom.y) ||
				(objdetect_roi[i].bottom.x > 1.0f || objdetect_roi[i].bottom.x < objdetect_roi[i].top.x) ||
				(objdetect_roi[i].bottom.y > 1.0f || objdetect_roi[i].bottom.y < objdetect_roi[i].top.y))
		{
			printf("[%s:%d] CH(%d) Invalid ROI Value(%f.%f, %f.%f)", __func__, __LINE__, i,
					objdetect_roi[i].top.x, objdetect_roi[i].top.y,
					objdetect_roi[i].bottom.x, objdetect_roi[i].bottom.y);

			objdetect_roi[i].top.x = 0.0f;
			objdetect_roi[i].top.y = 0.0f;

			objdetect_roi[i].bottom.x = 1.0f;
			objdetect_roi[i].bottom.y = 1.0f;
		}
	//ksi_test			
		// nmf_display_set_va_roi(i, 
		// 		objdetect_roi[i].top.x, 
		// 		objdetect_roi[i].top.y, 
		// 		objdetect_roi[i].bottom.x, 
		// 		objdetect_roi[i].bottom.y);
	}
}

int nf_va_object_detector_set_event(const int flag)
{
	int tmp_flag;

	pthread_mutex_lock(&mutex_va_event_flag);
	va_event_flag = flag;
	tmp_flag = va_event_flag;
	pthread_mutex_unlock(&mutex_va_event_flag);

	printf("\e[32m-- DLVA --\e[0m [%s:%d] flag(%d)\n", __func__, __LINE__, tmp_flag);
	return tmp_flag;
}

int nf_va_object_detector_get_event()
{
	int tmp_flag;

	pthread_mutex_lock(&mutex_va_event_flag);
	tmp_flag = va_event_flag;
	pthread_mutex_unlock(&mutex_va_event_flag);

	//printf("\e[32m-- DLVA --\e[0m [%s:%d] flag(%d)\n", __func__, __LINE__, tmp_flag);
	return tmp_flag;
}

int nf_va_object_detector_set_running(const int flag)
{
	int tmp_flag;

	pthread_mutex_lock(&mutex_va_running);
	va_running = flag;
	tmp_flag = va_running;
	pthread_mutex_unlock(&mutex_va_running);

	printf("\e[32m-- DLVA --\e[0m [%s:%d] flag(%d)\n", __func__, __LINE__, tmp_flag);
	return tmp_flag;
}

int nf_va_object_detector_get_running()
{
	int tmp_flag;

	pthread_mutex_lock(&mutex_va_running);
	tmp_flag = va_running;
	pthread_mutex_unlock(&mutex_va_running);

	printf("\e[32m-- DLVA --\e[0m [%s:%d] flag(%d)\n", __func__, __LINE__, tmp_flag);
	return tmp_flag;
}


unsigned int nf_va_object_detector_set_ch_mask(const unsigned int ch_bit_mask, const unsigned int flags)
{
	unsigned int tmp_mask;
	
	tmp_mask = ch_bit_mask;

	pthread_mutex_lock(&mutex_va_ch_mask);
	va_channel_mask = tmp_mask;
	va_setup_flag = flags;
	pthread_mutex_unlock(&mutex_va_ch_mask);

	printf("\e[32m-- DLVA --\e[0m [%s:%d] channel_mask(0x%08X) event(0x%08X)\n", __FUNCTION__, __LINE__, tmp_mask, flags);
	// nmf_display_va_object_detector_set_schedule_ch(va_channel_mask);ksi_test
	return tmp_mask;
}

unsigned int nf_va_object_detector_get_ch_mask()
{
	unsigned int tmp_mask;
	pthread_mutex_lock(&mutex_va_ch_mask);
	tmp_mask = va_channel_mask;
	pthread_mutex_unlock(&mutex_va_ch_mask);

	return tmp_mask;
}

unsigned int nf_va_object_detector_get_flags()
{
	unsigned int tmp_mask;
	pthread_mutex_lock(&mutex_va_ch_mask);
	tmp_mask = va_setup_flag;
	pthread_mutex_unlock(&mutex_va_ch_mask);

	return tmp_mask;
}

/* CALLBACK REGISTER */
void nf_va_object_detector_callback_register(nf_va_object_detector_callback callback)
{
	result_callback = callback;
}


/* INIT MODULE */
int nf_va_object_detector_module_init()
{
	char *cfgfile = "/NFDVR/mobilenet-ssd-voc_lcm2.cfg";
	char *weightfile = "/NFDVR/mobilenet-ssd-voc_lcm.weights";


	if(access(cfgfile, R_OK | W_OK))
	{
		printf("\e[1;31m[%s:%d] access faile(%s)\e[0m\n", __FUNCTION__, __LINE__, cfgfile);
		abort();
		return -1;
	}
	if(access(weightfile, R_OK | W_OK))
	{
		printf("\e[1;31m[%s:%d] access faile(%s)\e[0m\n", __FUNCTION__, __LINE__, cfgfile);
		abort();
		return -1;
	}
//ksi_test
	// itxdnn_init(0);

	// detector = itxdnn_detector_create(NULL, cfgfile, weightfile, -1.f, 100);

	nf_va_object_detector_update_roi();

	return 0;
}


/* Processing Main Thread*/
void* nf_va_object_detector_process_thread(void *var)
{
	static GAsyncQueue *va_queue = NULL;
	static GAsyncQueue *va_queuing_queue = NULL;
	VaImageCallbackInfo *frame = NULL;
	int queue_cnt = 0;
	pthread_t queuing_thread;

	va_queue = (GAsyncQueue*)var;

	va_queuing_queue = g_async_queue_new();

	pthread_create(&queuing_thread, NULL, nf_va_object_detector_queuing_process, va_queuing_queue);



	while(1)
	{
		frame = g_async_queue_pop(va_queue);

		queue_cnt = g_async_queue_length(va_queuing_queue);

		if(queue_cnt < 1)
		{
			{
				VaImageCallbackInfo *new_frame = malloc(sizeof(VaImageCallbackInfo));
				memset(new_frame, 0x00, sizeof(VaImageCallbackInfo));
				memcpy(new_frame, frame, sizeof(VaImageCallbackInfo));

				new_frame->bmp = malloc(new_frame->bmp_size);
				memcpy(new_frame->bmp, frame->bmp, new_frame->bmp_size);

				new_frame->jpeg = malloc(new_frame->jpeg_size);
				memcpy(new_frame->jpeg, frame->jpeg, new_frame->jpeg_size);

				g_async_queue_push(va_queuing_queue, new_frame);
			}
		}
		else
		{
			printf("[%s:%d] ch(%d) queue length(%d) != 0\n", __FUNCTION__, __LINE__, frame->ch, queue_cnt);
		}

		
		// Release
		if(frame != NULL)
		{
			if(frame->bmp != NULL)
			{
				free(frame->bmp);
				frame->bmp = NULL;
			}

			if(frame->jpeg)
			{
				free(frame->jpeg);
				frame->jpeg = NULL;
			}
			free(frame);
			frame = NULL;
		}
	} // END: while(1)
}

int nf_dva_object_detector_init(void)
{
	nf_va_object_detector_callback_register(nf_dva_detector_cb_func);
	
	return 0;
}
