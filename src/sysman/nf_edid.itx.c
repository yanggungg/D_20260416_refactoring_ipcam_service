#include <glib.h>

#include "nf_common.h"
#include "jbshell.h"

#include "nf_util_device.h"

#if defined(USE_DEV_EDID)
#include <itx_edid.h>       // in driver/edid/itx_edid.h
#endif
#include "nf_edid.itx.h"

/**
	Extern Function Definition
 **/

#if defined(USE_DEV_EDID)
	extern gboolean nf_dev_edid_get_raw_data(struct edid_data *info, gboolean is_vga);
#endif

/**
	Function Start
**/
gboolean nf_edid_vga(struct edid_data *info)
{
	#if defined(USE_DEV_EDID)
		nf_dev_edid_get_raw_data(info, TRUE);
	#endif

	if(info->is_fail)
		return FALSE;
	else
		return TRUE;
}

gboolean nf_edid_hdmi(struct edid_data *info)
{

	return FALSE;
}

