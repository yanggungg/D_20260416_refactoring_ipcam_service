#ifndef	__NF_UI_SPOT_CONF_H__
#define	__NF_UI_SPOT_CONF_H__


typedef enum _FROM_MENU_E{
    SPOT_OUT = 0,
    DUAL_MONITOR
}FROM_MENU_E;


void SpotConf_Open(NFWINDOW *parent, SpotElementData *elem_data, guint *num_items, guint split_ch, guint output_ch, FROM_MENU_E menu);

#endif	// __NF_UI_SPOT_CONF_H__

