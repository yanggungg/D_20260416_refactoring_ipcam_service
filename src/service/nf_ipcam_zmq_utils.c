#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nf_ipcam_zmq_utils.h"
#include "nf_ipcam_defs.h"

#define IPCAM_ZMQ_DEBUG (1)

ZMQ_RECV_MANAGER recv_manager;

static int zmq_receiver_bind(void* p_receiver, char* p_addr);
void zmq_recv_thread_func ();
static char *zstr_recv (void *socket);
static int zstr_send (void *socket, char *string); 

static struct timespec curTime;
static struct timespec pasTime; 

extern ZMQ_RECV_MANAGER* nf_ipcam_get_zmq_manager()
{
	return &recv_manager;
}

extern GAsyncQueue* get_zmq_msg_queue(void) 
{
	return recv_manager.msg_queue; 
}

int zmq_receiver_initialize(void** p_ctx, void** p_receiver, char* p_addr)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	memset(&recv_manager, 0x00, sizeof(ZMQ_RECV_MANAGER));
	*p_ctx = zmq_ctx_new();
	*p_receiver = zmq_socket (*p_ctx, ZMQ_PULL);

	int size = 2048;
	zmq_setsockopt(*p_receiver, ZMQ_RCVBUF, &size, sizeof(int));

	if(*p_receiver == NULL)
		return rtn;

	rtn = zmq_receiver_bind(*p_receiver, p_addr);
	if(rtn != IPCAM_SETUP_RTN_DONE)
		return rtn;

#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
	return rtn;
}

void zmq_receiver_finalize(void** p_ctx, void** p_receiver)
{
	zmq_close(*p_receiver);
	zmq_ctx_destroy(*p_ctx);
#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
}

int zmq_receiver_start()
{
	int rtn;

	recv_manager.is_run = 1;
	recv_manager.msg_queue = g_async_queue_new();

	recv_manager.thread_id = g_thread_create((GThreadFunc)zmq_recv_thread_func, NULL, TRUE, NULL);
	if (recv_manager.thread_id == 0)
	{
		printf("[%s] g_thread_create is failed\n", __FUNCTION__);
		return IPCAM_SETUP_RTN_FAILED;
	}

#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

void zmq_receiver_stop()
{
	if(recv_manager.is_run)
	{
		recv_manager.is_run = 0;
		g_thread_join(recv_manager.thread_id);

		while(g_async_queue_length(recv_manager.msg_queue) > 0)
		{
#if IPCAM_ZMQ_DEBUG
			printf("\e[33m [%s][%d] g_async_queue_length(%d) \e[0m\n", __func__, __LINE__, 
					g_async_queue_length(recv_manager.msg_queue));
#endif

			void *data = NULL;
			data = g_async_queue_try_pop(recv_manager.msg_queue);

			if(data == NULL)
			{
				g_usleep(3*1000);
				continue;
			}
			//g_object_unref
			free(data);
		}
		g_async_queue_unref(recv_manager.msg_queue);
	}
#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
}

static int zmq_receiver_bind(void* p_receiver, char* p_addr)
{
	char bind_str[32];
	memset(bind_str, 0x00,32);
	//snprintf(bind_str, 32, "tcp://%s:1015", p_addr);
	snprintf(bind_str, 32, "tcp://0.0.0.0:1015");

	if(zmq_bind(p_receiver, bind_str) != 0)	
		return IPCAM_SETUP_RTN_FAILED;

#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

static double TimeSpecToSeconds(struct timespec* ts)
{
	    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}

void zmq_recv_thread_func ()
{
	while(recv_manager.is_run)
	{
		//s_recv malloc (g_object_ref)
        char *string = zstr_recv (recv_manager.g_zmq_sock);

		int msg_queue_length = g_async_queue_length(recv_manager.msg_queue);
		if (msg_queue_length % 20 == 0 && msg_queue_length > 0)
		{
			printf("\e[32m###yanggungg : %s, %d AI_DATA_queue length[%d]\e[0m\n", __func__, __LINE__, g_async_queue_length(recv_manager.msg_queue));
		}

		if(string != NULL) 
		{
			//time dump test
			//double diff_time = 0;
			//clock_gettime(CLOCK_MONOTONIC, &curTime);
			//diff_time = TimeSpecToSeconds(&curTime) - TimeSpecToSeconds(&pasTime);
			//printf("\e[104m zmq recv diff time (%f) \e[0m\n", diff_time);
			//memcpy(&pasTime, &curTime, sizeof(struct timespec));

			if (g_async_queue_length(recv_manager.msg_queue) >= IPCAM_VA_MSG_QUEUE_MAX)
			{
				// printf("\e[31m[%s, %d] msg_queue_length is over max (%d)\e[0m\n", __FUNCTION__, __LINE__, g_async_queue_length(recv_manager.msg_queue));
				free(string);
				continue;
			}

			g_async_queue_push(recv_manager.msg_queue, (gpointer)string);
#if IPCAM_ZMQ_DEBUG
			// printf("\e[33m [%s][%d] g_async_queue_length(%d) \e[0m\n", __func__, __LINE__, 
			// 		g_async_queue_length(recv_manager.msg_queue));
#endif
		}

        usleep (5*1000);          //  Do some 'work'
	}

#if IPCAM_ZMQ_DEBUG
	printf("\e[33m [%s][%d] gthread end... \e[0m\n",__func__, __LINE__);
#endif
}

static char *
zstr_recv (void *socket) {
	zmq_msg_t message;
	char ip[16];

	zmq_msg_init (&message);
	int size = zmq_msg_recv (&message, socket, ZMQ_DONTWAIT);
	if (size == -1) {
		// printf("[%s:%d] zmq_msg_recv error: %s\n", __FUNCTION__, __LINE__, zmq_strerror(zmq_errno()));
		zmq_msg_close (&message);
		return NULL;
	}
	char *string = malloc (size + 1 + 4);
	
	memset(ip, 0x00, sizeof(ip));
	strncpy(ip, zmq_msg_gets(&message, "Peer-Address"), sizeof(ip)-1);
    *(int *)string = nf_ipcam_get_channel_from_ipaddr(ip);
	memcpy (string+4, zmq_msg_data (&message), size);

	// printf("[%s:%d] zmq[%d] client ip[%s] string[%s]\n", __FUNCTION__, __LINE__, ZMQ_VERSION, ip, string+4);

	zmq_msg_close (&message);
	string [size+4] = 0;
	return (string);
}

static int
zstr_send (void *socket, char *string) {
	zmq_msg_t message;
	zmq_msg_init_size (&message, strlen (string));
	memcpy (zmq_msg_data (&message), string, strlen (string));
	int size = zmq_msg_send (&message, socket, 0);
	zmq_msg_close (&message);
	return (size);
}

