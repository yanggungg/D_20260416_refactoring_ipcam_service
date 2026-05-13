#ifndef _SETUP_DISP_HD_SPOT_EDIT_H_
#define _SETUP_DISP_HD_SPOT_EDIT_H_

enum {
	SPOT_EDIT_RET_MODIFY = 0,
	SPOT_EDIT_RET_DELETE,
	SPOT_EDIT_RET_CANCEL,
	SPOT_EDIT_RET_SAVE,
};

guint HD_SpotEditDlg_Open(NFWINDOW *parent, SpotData *seq_data);

#endif

