
#ifndef _VW_PASS_EXPIRED_DLG_H
#define _VW_PASS_EXPIRED_DLG_H

#include "objects/nfwindow.h"


gint VW_CheckPasswordExpired(NFWINDOW *parent);
gint VW_CheckPasswordExpired_id(NFWINDOW *parent, gchar *id);
gint VW_CheckPasswordExpired_Close();

#endif
