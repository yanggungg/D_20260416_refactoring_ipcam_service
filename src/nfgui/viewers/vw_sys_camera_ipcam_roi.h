#ifndef __VW_INIT_IPCAMROI_PAGE_H__
#define __VW_INIT_IPCAMROI_PAGE_H__

void VW_Init_IPCamROI_Page(NFOBJECT *parent, gint ch);
gint IPCamROI_start_preview(gint ch);
gint IPCamROI_update_channel(gint ch);
gboolean IPCamROI_tab_in_handler();
gboolean IPCamROI_tab_out_handler();
gint IPCamROI_stop_preview();

#endif

