#include <string.h>

#include "nf_afx.h"

#include "tools/nf_ui_tool.h"


#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "scm.h"
#include "ix_mem.h"



static RecordOperData *oper_data = NULL;



////////////////////////////////////////////////////////////
//
// private functions
//





////////////////////////////////////////////////////////////
//
// public interfaces
//

void init_oper_data(void)
{
	oper_data = imalloc(sizeof(RecordOperData));
	if(!oper_data) {
		g_warning("%s : oper_data allocation error", __FUNCTION__);
		
		return FALSE;
	}

	if(!DAL_get_recordOper_data(oper_data)) {
		g_warning("%s :DAL_get_recordOper_data returns FALSE", __FUNCTION__);

		return FALSE;
	}

	return TRUE;
}

void remove_oper_data(void)
{
	if(oper_data) {
		ifree(oper_data);
		oper_data = NULL;
	}

}

/*inline*/ guint get_record_mode()
{
	return oper_data->mode;
}

/*inline*/ guint get_record_ssc()
{
	return oper_data->smart_storage;
}

/*inline*/ guint get_record_autoConfig()
{
	return oper_data->autoConfig;
}

/*inline*/ guint get_record_priMode()
{
	return oper_data->priMode;
}

/*inline*/ guint get_record_schedMode()
{
	return oper_data->schedMode;
}

/*inline*/ guint get_record_pre_time()
{
	return oper_data->preRecTime;
}

/*inline*/ guint get_record_post_time()
{
	return oper_data->postRecTime;
}

/*inline*/ guint get_record_panic_time()
{
	return oper_data->panicTime;
}

gboolean set_record_operation_data(RecordOperData *data)
{
	g_return_val_if_fail(data != NULL, FALSE);

	if(memcmp(oper_data, data, sizeof(RecordOperData))) 
	{	
		if (oper_data->mode != data->mode) scm_put_log(CHANGE_REC_MODE, 0, 0);
		if (oper_data->smart_storage != data->smart_storage) nf_ipcam_set_bitrate_ssc_off();

		memcpy(oper_data, data, sizeof(RecordOperData));
		DAL_set_recordOper_data(data);
		set_changed_record_data();
		return TRUE;
	}

	return FALSE;
}

gboolean is_changed_operation_data(RecordOperData *data)
{
	mb_type ret;

	if(!memcmp(oper_data, data, sizeof(RecordOperData)))	
		ret = FALSE;
	else
		ret = TRUE;
	
	return ret;
}

gboolean get_record_operation_data(RecordOperData *data)
{
	g_return_val_if_fail(data != NULL, FALSE);
	
	memcpy(data, oper_data, sizeof(RecordOperData));

	return TRUE;
}


