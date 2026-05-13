#ifndef _VA_OBJECT_DETECTOR_
#define _VA_OBJECT_DETECTOR_

#include "nf_api_dva_eventlog.h"

#define MAX_DETECT_OBJECTS	(16)
#define OBJ_CLS_MAX_CNT	(256)

enum NF_VA_OBJ_FLAGS_E
{
	NF_VA_OBJ_SETUP = 1<<0,
};

#define MAX_JPEG_SIZE 800*1024
struct img 
{
	unsigned int width;
	unsigned int height;
	unsigned int size;
	unsigned char data[MAX_JPEG_SIZE];
};

struct fpoint
{
	float x;
	float y;
};

struct bbx
{
	struct fpoint top;
	struct fpoint bottom;
};

struct object
{
	char name[64];
	int id;
	float confidence;
	struct bbx bbx;
};

struct objects
{
	int ch;
	unsigned int time;
	unsigned int timel;
	int obj_num;
	struct object obj[MAX_DETECT_OBJECTS];
	struct img img;
};

struct objs_coords
{
	int ch;
	unsigned int time;
	unsigned int timel;
	int obj_num;
	struct object obj[MAX_DETECT_OBJECTS];
};

typedef void (*nf_va_object_detector_callback)(struct objects *);

void nf_va_object_detector_callback_register(nf_va_object_detector_callback callback);
void* nf_va_object_detector_process_thread(void *var);

unsigned int nf_va_object_detector_set_ch_mask(const unsigned int ch_bit_mask, const unsigned int flags);
unsigned int nf_va_object_detector_get_ch_mask(void);
unsigned int nf_va_object_detector_get_flags();

int nf_va_object_detector_module_init(void);
void nf_va_object_detector_update_roi(void);

int nf_va_object_detector_init(void);
int nf_va_object_detector_ch_update(void);

int nf_va_object_detector_set_event(const int flag);
int nf_va_object_detector_get_event();

int nf_va_object_detector_set_running(const int flag);
int nf_va_object_detector_get_running();

#endif
