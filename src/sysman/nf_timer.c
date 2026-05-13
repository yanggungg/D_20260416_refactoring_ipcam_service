#include "nf_timer.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "timer"


#define DEBUG_JBSHELL_TIMER

#ifdef DEBUG_JBSHELL_TIMER
#include "jbshell.h"
#endif

extern GMainLoop *NF_Loop; 
extern GMainContext *NF_Context;

static gboolean
nf_tmpl_timeout_cb (gpointer data)
{
	static gint cnt = 0;
	g_message("%s %d", __FUNCTION__, ++cnt);		
	return TRUE;
}

/*
	GMainLoop에 timer source를 사용해서 타이머 구현
	
	nf_host main thread가 동작하면서 NF_Context,NF_Loop 등이 초기화 되는데
	이 context와 loop에서 timer 구현된다.		
*/

/**
	@brief				timer 등록  interval in milliseconds
	@return	gboolean	%TRUE on success
*/
guint
nf_timer_add( guint interval, GSourceFunc cb_func, gpointer data)
{
	GSource			*source;
	guint			ret = 0;

	g_return_val_if_fail (NF_Context != NULL, 0);
	g_return_val_if_fail (cb_func != NULL, 0);
						
	source = g_timeout_source_new (interval);
	
	// init event_timeout_cb
	g_source_set_callback (source, (GSourceFunc)cb_func, data, NULL);
	g_source_set_priority (source, G_PRIORITY_HIGH);


	ret = g_source_attach (source, NF_Context );

	g_source_unref(source);
	
	return ret;
}

/**
	@brief				timer 제거
	@return	gboolean	%TRUE on success
*/
gboolean
nf_timer_remove (guint tag)
{
	GSource *source;
	
	g_return_val_if_fail (NF_Context != NULL, 0);
	g_return_val_if_fail (tag > 0, 0);
	
	source = g_main_context_find_source_by_id (NF_Context, tag);
	
	if (source)
		g_source_destroy (source);
	
	return source != NULL;  
}

#ifdef DEBUG_JBSHELL_TIMER

#define MAX_CMD_BUFF 512	

typedef struct _TIMER_LIST_T
{
	gchar	cmd[MAX_CMD_BUFF];
	guint	timer_id;
	gint	interval;
	gint	print_timestamp;	
}TIMER_LIST;

static void print_list();
static GList* find_list(gint tid);

GList *_timer_list = NULL; 	/* our list pointer */

static gboolean jbshell_timer_cb( gpointer plist)
{
	gint ret = TRUE;
	gchar buf[128] = {0, };
	time_t cur_time;

	TIMER_LIST *tlist = (TIMER_LIST *)plist;

	time( &cur_time);
	ctime_r(&cur_time, buf);
	
	if(tlist->print_timestamp)
		g_message("%s cmd[%s] [%s]", __FUNCTION__, tlist->cmd, buf);

	if(tlist->print_timestamp)
		my_command(tlist->cmd);
	else
		my_command_slient(tlist->cmd);
		
	return ret;
}

static char timer_add_help [] = "nf_timer_add [sec] [print timestamp:1] [cmd]";
static int timer_add(int argc, char **argv)
{
	gint sec=0, print_timestamp = 1;
	guint tid=0;
	
	gint i=0;
	
	TIMER_LIST *tlist;

	if(argc < 4)
	{		
		printf("Invalid arguments\n%s\n", timer_add_help);
		return -1;
	}
	g_message("%s sec[%s] print_timestamp[%s] func[%s]", __FUNCTION__, argv[1], argv[2], argv[3]);
	
	sec = atoi(argv[1]);
	print_timestamp = atoi(argv[2]);
	
	tlist = (TIMER_LIST*)g_malloc0(sizeof(TIMER_LIST));
	if(tlist == NULL)
	{
		g_warning("%s add node error.. tlist is null", __FUNCTION__);
		return -1;
	}

	tlist->interval = sec;
	tlist->print_timestamp = print_timestamp;
			
	for(i=3; i<argc;i++)
	{
		strncat(tlist->cmd, argv[i], MAX_CMD_BUFF);
		strncat(tlist->cmd," ", MAX_CMD_BUFF);
	}
	
	tid = nf_timer_add( sec*1000, jbshell_timer_cb , tlist);
	tlist->timer_id = tid;

	_timer_list = g_list_append(_timer_list, tlist);

	return 0;
}
__commandlist(timer_add, "timer_add", timer_add_help, timer_add_help);

static void print_list()
{
	GList *li;
	TIMER_LIST* tlist;

	li = _timer_list;
	g_print("=============================================\n");	
	while(li != NULL)
	{
		tlist = (TIMER_LIST *)li->data;
		g_print("%s tid[%d] cmd[%s] interval[%d]\n", __FUNCTION__, 
				tlist->timer_id, tlist->cmd, tlist->interval);
		li = li->next;
	}
	g_print("=============================================\n");	
}

static char timer_rm_help [] = "timer_rm [tid]";
static int timer_rm(int argc, char **argv)
{
	GList *li = NULL;
	TIMER_LIST *tlist =NULL;
	gint tid=0;	

	if(argc < 2)
	{
		g_print("Invalid arguments\n%s\n", timer_rm_help);
		print_list();
		return -1;
	}

	tid = atoi(argv[1]);	
	li  = find_list(tid);
	if(li == NULL)
	{
		g_warning("%s cannot find tid[%d]", __FUNCTION__, tid);
		return -1;
	}

	tlist = li->data;
	printf("%s tid[%d] name[%s]	interval[%d]\n", __FUNCTION__, 
			tlist->timer_id, tlist->cmd, tlist->interval);

	//delete from node(link remove, free);
	_timer_list = g_list_delete_link(_timer_list,li);	
	
	g_free(tlist);
	
	//remove from timer
	nf_timer_remove(tid);
	return 0;
}

__commandlist(timer_rm, "timer_rm", timer_rm_help , timer_rm_help);

static GList* find_list(gint tid)
{
	GList *pWalker = NULL;
	TIMER_LIST *tmp = NULL;
	
	pWalker = _timer_list;
	while(pWalker != NULL)
	{
		tmp = (TIMER_LIST*) pWalker->data;
		if(tmp->timer_id == tid)
			break;
		
		pWalker = pWalker->next;
	}

	return  pWalker;
}

#endif
