#ifndef	__NF_UI_COMMON_DATA_H__
#define	__NF_UI_COMMON_DATA_H__


guint nfcd_get_disk_count();
guint nfcd_get_real_disk_count();
void nfcd_set_disk_count(guint real, guint fake);



void nfcd_init_remocon_id();
guint nfcd_get_remocon_id();
void nfcd_set_remocon_id(guint id);

#endif

