#ifndef	__NF_UI_VKEYBD_H__
#define	__NF_UI_VKEYBD_H__



typedef enum
{
    VKEY_NORMAL 		= 0,
    VKEY_ALPHANUMERIC 	= 1,
	VKEY_ITXSTYLE_TITLE = 2,
	VKEY_FTP_DIR 		= 3,
	VKEY_NUMERIC		= 4,
	VKEY_NVR_NAME		= 5,
	VKEY_MAIL			= 6,
	VKEY_PASSWORD		= 7,	
	VKEY_FILENAME       = 8,
	VKEY_RTSP           = 9,
    VKEY_CAMTITLE_SPACE = 10,	
    VKEY_LICENSE_CODE   = 11,	
	VKEY_ID				= 12,
    VEKY_MODE_MAX,
}vkey_mode_type;

typedef enum
{
	VKEY_MULTIKEYPD          = 1 << 16,
};

gchar* VirtualKey_Open(NFWINDOW *parent, const gchar *str, guint x, guint y, gint max_ch, vkey_mode_type vkey_mode);
void VirtualKey_Close();

#endif
