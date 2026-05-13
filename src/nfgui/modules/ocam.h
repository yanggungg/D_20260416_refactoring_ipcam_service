
#ifndef __OCAM_H
#define __OCAM_H

#include "nvm.h"

typedef enum _OCAM_FILTER_F {
	OCAM_F_HOSTADDR,
	OCAM_F_MODEL,
	OCAM_F_HIDDEN,
	OCAM_F_ASSIGNED,
	OCAM_F_NO
} OCAM_FILTER_F;

typedef enum _OCAM_FILTER_CMB {
	OCAM_C_AND,
	OCAM_C_OR,
	OCAM_C_RST,
	OCAM_C_NO
} OCAM_FILTER_CMB;

typedef enum _OCAM_DEV_TYPE {
	OCAM_T_NORMAL,
	OCAM_T_DIRECT,
	OCAM_T_VCAM,
	OCAM_T_NO
} OCAM_DEV_TYPE;

enum {
	OCAM_B_SORT	= 0x00000001,
	OCAM_B_HIDE = 0x00000002,
	OCAM_B_FILT	= 0x00000004,
	OCAM_B_ALL	= 0x00000007
};

int ocam_init();
int ocam_add_cam(CAMERA_INFO_T *info);
int ocam_reg_cam(CAMERA_INFO_T *info);
int ocam_remove_cam(char *hostname);
int ocam_hide_cam(char *hostname);
int ocam_show_cam(char *hostname);
int ocam_assign(char *hostname);
int ocam_unassign(char *hostname);
int ocam_get_card_idx();
int ocam_save();
int ocam_reload();
int ocam_sort_by_hostname(int dir);
int ocam_clear_sort();
int ocam_filter(OCAM_FILTER_F field, char *op, char *value, OCAM_FILTER_CMB cmb);
int ocam_clear_filter();
int ocam_check_filter();

#endif
