#ifndef _SETUP_DISP_SPOT_EDIT_H_
#define _SETUP_DISP_SPOT_EDIT_H_

enum {
	SPOT_EDIT_RET_MODIFY = 0,
	SPOT_EDIT_RET_DELETE,
	SPOT_EDIT_RET_CANCEL,
	SPOT_EDIT_RET_SAVE,
};

guint SpotEditDlg_Open(NFWINDOW *parent, SpotData *spot_data, guint split_ch, guint output_ch);

#endif

