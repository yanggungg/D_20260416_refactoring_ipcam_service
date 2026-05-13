#include "nf_common.h"

#include "nf_api_param_fwver.h"

static guint _fwver_product = 0;
static guint _fwver_procotol = 0;
static guint _fwver_minor = 0;
static guint _fwver_vendor = 0;
static gchar _fwver_option[16] = {0};

gboolean nf_sysman_fwver_init(void)
{
	char tmp[128];
	int ret;

	memset( tmp, 0x00, sizeof(tmp));
	memset( _fwver_option, 0x00, sizeof(_fwver_option));

	strncpy( tmp, nf_sysdb_get_str_nocopy("sys.info.swver"), sizeof(tmp));

	ret = sscanf( tmp, "%d.%d.%d.%d%s", &_fwver_product,
										&_fwver_procotol,
										&_fwver_minor,
										&_fwver_vendor, _fwver_option);

	g_message("%s : swver[%s] ret[%d] [%d].[%d].[%d].[%d%s]", __FUNCTION__,
				tmp, ret,
				_fwver_product,
				_fwver_procotol,
				_fwver_minor,
				_fwver_vendor, _fwver_option);

	return TRUE;
}

// from sys.info.swver
guint nf_api_param_fwver_get_product(void)
{
	return _fwver_product;
}

guint nf_api_param_fwver_get_protocol(void)
{
	return _fwver_procotol;
}

guint nf_api_param_fwver_get_minor(void)
{
	return _fwver_minor;
}

guint nf_api_param_fwver_get_vendor(void)
{
	return _fwver_vendor;
}

gchar *nf_api_param_fwver_get_option2(void)
{
	return _fwver_option;
}

