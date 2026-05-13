#ifndef __VW_SEARCH_BY_THUMBNAIL_H
#define __VW_SEARCH_BY_THUMBNAIL_H

#include "objects/nfobject.h"

void vw_init_SearchByThumbnail_page(NFOBJECT *parent);
gboolean vw_SearchByThumbnail_tab_out_handler();
gboolean vw_SearchByThumbnail_tab_in_handler();
gboolean vw_SearchByThumbnail_tab_show();

#endif

