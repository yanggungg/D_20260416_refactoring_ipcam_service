#ifndef _VW_CHANNEL_MASK_CTRL_H_
#define _VW_CHANNEL_MASK_CTRL_H_

static guint64 CAMERA_MASK_TYPE = (1LL << 63);
static guint64 ALARM_OUT_MASK_TYPE = (1LL << 62);

gboolean VW_ChannelMask_Ctrl(NFWINDOW *parent, gchar *title, gint x, gint y, guint64 *mask);


#endif
