#ifndef __NF_API_PARAM_FWVER_H__
#define __NF_API_PARAM_FWVER_H__

typedef enum _NF_API_FWVER_VENDOR_E
{
	VENDOR_VIDECON      = 28,
	VENDOR_S1           = 30,
	VENDOR_CBC          = 32,
	VENDOR_TAKENAKA     = 65,
	VENDOR_KOBI         = 76,
	VENDOR_KBDEVICE     = 78,
	VENDOR_ITX_46GUI    = 1400,

	VENDOR_UNKNOWN      = 0xFF,
} NF_API_FWVER_VENDOR;

gboolean nf_api_param_fwver_init(void);

guint nf_api_param_fwver_get_product(void);
guint nf_api_param_fwver_get_protocol(void);
guint nf_api_param_fwver_get_minor(void);
guint nf_api_param_fwver_get_vendor(void);
gchar *nf_api_param_fwver_get_option2(void);

#endif

