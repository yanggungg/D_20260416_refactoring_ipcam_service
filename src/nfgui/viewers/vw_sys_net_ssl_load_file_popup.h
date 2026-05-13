#ifndef __VW_SYS_NET_SSL_LOAD_FILE_POPUP_H__
#define __VW_SYS_NET_SSL_LOAD_FILE_POPUP_H__

typedef enum
{
    SSL_MODE_BUILTIN = 0,
    SSL_MODE_CA,
    SSL_MODE_DCV
} SSL_MODE_E;

gboolean VW_SSL_Load_File_Open(NFWINDOW *parent, gchar *title, guint pos_x, guint pos_y, gchar *dir, gchar *fname, SSL_MODE_E mode);

#endif
