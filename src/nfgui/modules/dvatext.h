/*
 * dvatext.h
 * 	- dva text module
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Jan 16, 2011
 *
 */

#ifndef __DVATEXT_H
#define __DVATEXT_H

#include "cmm.h"
#include "iux_afx.h"
#include "ix_func.h"



////////////////////////////////////////////////////////////
//
// public data type 
//



////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint dvatext_translate_to_uxitem(gchar *engitem, gchar *uxitem, gint uxitem_len);
gint dvatext_translate_to_engitem(gchar *uxitem, gchar *engitem, gint engitem_len);

gint dvatext_translate_to_uxstring_db(gchar *engstring, gchar *uxstring, gint uxstring_len);
gint dvatext_translate_to_engstring_db(gchar *uxsting, gchar *engstring, gint engstring_len);

#endif
