#ifndef	__VW_PROGRESS_FWUPDATE_128MB_H__
#define	__VW_PROGRESS_FWUPDATE_128MB_H__

enum {
    UPGRADE_TYPE_LOCAL  = 0,
    UPGRADE_TYPE_WEB,
    UPGRADE_TYPE_REMOTESERVER
};

gint vw_progress_fwup_128MB_open(NFWINDOW *parent);
gint vw_progress_fwup_128MB_update(gint step, gint result);
gint vw_progress_fwup_128MB_close();

#endif	// __VW_PROGRESS_FWUPDATE_128MB_H__
