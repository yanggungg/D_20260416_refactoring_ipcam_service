#ifndef __NF_IPCAM_VBOX_OPERATOR_C__
#define __NF_IPCAM_VBOX_OPERATOR_C__

#include <glib.h>
#include "nf_ipcam_defs.h"

typedef struct __ICM_VA_BOX_OPERATOR__
{
	int is_run;
	GThread *thread_id;
	GAsyncQueue *vabox_data_queue;
	GAsyncQueue *vabox_event_data_queue;

} VB_DATA_OPERATOR;

typedef struct __ICM_VA_PROCESS_TIME_UPDATOR__
{
	int is_run;
	GThread *thread_id;
	GMutex *mutex;

} VB_TIME_UPDATOR;

int nf_ipcam_vabox_data_operator_start();
void nf_ipcam_vabox_data_operator_stop();
VA_EVT_DATA* nf_ipcam_get_vabox_popped_data();
VA_GNR_EVT_DATA* nf_ipcam_get_vabox_popped_event_data();
int nf_ipcam_parse_vabox_metadata_stream(int ch, char* p_text);
GAsyncQueue * get_vabox_data_queue();

#endif // __NF_IPCAM_VBOX_OPERATOR_C__
