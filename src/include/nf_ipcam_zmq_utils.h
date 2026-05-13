#ifndef __NF_IPCAM_ZMQ_UTILS_C__
#define __NF_IPCAM_ZMQ_UTILS_C__

#include <glib.h>
#include <zmq.h>
// #include <zhelpers.h>
#include <jansson.h>

#define VA_META_DATA_MAX	100 
#define FACE_GROUP_MAX	16
#define FACE_CNT_MAX 8	
#define IPCAM_VA_MSG_QUEUE_MAX (100)

typedef struct _VA_FACE_RECOGNITION_INFO__
{
	unsigned int face_id;
	double search_score; 

	char name[32];
	char group_name[FACE_GROUP_MAX][32];
	int group_cnt;
	int age;
	char gender[32];
	char headwear[32];
	char glasses[32];

}VA_FACE_REC_INFO;

typedef struct _VA_FACE_RECOGNITION_DATA__
{	
	int info_cnt;
	VA_FACE_REC_INFO info[FACE_CNT_MAX];

}VA_FACE_REC_DATA;

typedef struct _VA_META_DATA_
{
	int id;
	char class[64];
	double confidence;
	double bbx_position[4];

	gboolean is_face_data;
	VA_FACE_REC_DATA face_data;
	char lp_text[64];

}VA_META_DATA;

/*
typedef struct _VA_FACE_RECOGNITION_DATA__
{
	char name[32];
	char group_name[FACE_GROUP_MAX][32];
	int group_cnt;
	int age;
	char gender[32];
	char headwear[32];
	char glasses[32];

}VA_FACE_REC_DATA;
*/


typedef enum _VA_ONVIF_TRANSDATA_TYPE_E {
    VA_TYPE_NONE            = 0,
	VA_TYPE_GENERIC_EVENT   = 1,
} VA_ONVIF_TRANSDATA_TYPE_E;

typedef struct _VA_ONVIF_TRANSFER_DATA_
{
    int type;           //data type VA_ONVIF_TRANSDATA_TYPE_E
	int ch;             //nvr channel
    json_t *json_root;  //json event data receved from aibox
}VA_ONVIF_EVENT_DATA;

typedef struct _VA_POINT_T_
{
    float x;
    float y;
}VA_POINT;

typedef struct _VA_GENERIC_EVENT_DATA_T_
{
	int ch;
	unsigned int timestamp;
	unsigned int timestampl;

    VA_POINT event_area[2]; //bbox left_top, right_bottom

    //event message
    char caption[45];
    char title[84];
    char description[100];

    //zone
    char trigger_type[45];
    char trigger_name[45];
    size_t trigger_zone_count;
    VA_POINT *trigger_zone_list;
}VA_GNR_EVT_DATA;

typedef struct _VA_EVENT_DATA_T_
{
	int ch;
	int resol_width;
	int resol_height;
	char topic[64];
	unsigned int timestamp;
	unsigned int timestampl;
	unsigned int process_time;
	int meta_count;
	VA_META_DATA meta_data[VA_META_DATA_MAX];

	gboolean is_face_data;
	gboolean is_lpr_data;

}VA_EVT_DATA;

typedef enum __ICM_ZMQ_STATE__ {
	ICM_ZMQ_ERROR,
	ICM_ZMQ_OK,
} icm_state;

typedef struct __ZMQ_RECV_MANAGER__
{
	int is_run;
	GThread *thread_id;
	GAsyncQueue *msg_queue;
	void *g_zmq_ctx;
	void *g_zmq_sock;

} ZMQ_RECV_MANAGER;

int zmq_receiver_initialize(void** p_ctx, void** p_receiver, char* p_addr);
void zmq_receiver_finalize(void** p_ctx, void** p_receiver);
int zmq_receiver_start();
void zmq_receiver_stop();
extern ZMQ_RECV_MANAGER* nf_ipcam_get_zmq_manager(void);
extern GAsyncQueue* get_zmq_msg_queue(void); 

#endif // __NF_IPCAM_ZMQ_UTILS_C__
