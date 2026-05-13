#ifndef _VW_RECORD_NET_STREAMING_H
#define _VW_RECORD_NET_STREAMING_H

#include "objects/nfobject.h"

void VW_Init_NetStream_Page(NFOBJECT *parent);
gboolean VW_NetStream_tab_out_handler();
void VW_NetStream_info_refresh();

#endif


