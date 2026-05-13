#ifndef	__NF_UI_VKEYBD2_H__
#define	__NF_UI_VKEYBD2_H__

typedef enum
{
    VKEY2_ID            = 0,
    VKEY2_PASSWORD      = 1,
    VEKY2_MODE_MAX
}vkey2_mode_type;

gchar* VirtualKey_Open2(NFWINDOW *parent, const gchar *id, const gchar *password, guint x, guint y, gint max_ch, vkey2_mode_type vkey2_mode);

#endif

