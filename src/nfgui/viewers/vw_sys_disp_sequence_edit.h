#ifndef _SETUP_DISP_SEQUENCE_EDIT_H_
#define _SETUP_DISP_SEQUENCE_EDIT_H_

#define	SEQ_MODE_ADD	0
#define	SEQ_MODE_EDIT	1

enum {
	SEQ_EDIT_RET_MODIFY = 0,
	SEQ_EDIT_RET_DELETE,
	SEQ_EDIT_RET_CANCEL,
	SEQ_EDIT_RET_SAVE,
};

guint SequenceEditDlg_Open(NFWINDOW *parent, guint mode, SeqData *seq_data, gboolean edit);

#endif

