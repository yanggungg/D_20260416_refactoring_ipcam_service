/*
 * ITX Security
 *  System software group
 *
 *  2012-03-05 jykim
 */

#ifndef __NF_IPCAM_DRIVER_AXIS_H__
#define __NF_IPCAM_DRIVER_AXIS_H__

#include <nf_ipcam_defs.h>

typedef struct enumDataType 
{
	int 	enumCnt;
	char 	value[32][64];			//1st array 10 to 32
	char 	nice_value[32][64];		//1st array 10 to 32
} EntryEnumData;

typedef struct intDataType 
{
	int min;
	int max;

} EntryIntData;

struct DataType 
{
	int 			type;
	char 			name[64];
	char 			nice_name[64];
	char 			e_value[64];
	int  			i_value;
	char 			b_value[64];

	EntryEnumData 	eData;
	EntryIntData 	iData;

	struct DataType *next;
};


// nf_axis_imagesource_parser.c
extern struct DataType* getAxisImageListHead(void);
extern struct DataType* getAxisImageListTail(void);
extern int nf_axis_get_imagesource(char* source);
extern int nf_axis_get_appearance(char* source);
extern void nf_axis_image_free();


// nf_ipcam_driver_axis.c
extern int axis_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);
extern int axis_init_profiles(int cam_id);
extern int axis_set_vcodec_m311x(int cam_id);
extern int axis_set_vcodec_p3346(int cam_id);

extern int nf_axis_set_motion(NFIPCamSetupMotionArea *motion_info, int cam_id);
extern int nf_axis_get_image(int ch);
extern int nf_axis_set_image(image_info_onvif* info_set, int cam_id);
extern int nf_axis_set_mirror(cam_info* info, int cam_id);
extern int nf_axis_set_user_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password);
extern int nf_axis_set_webservice_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password);
extern int nf_axis_soft_factory_default_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password);
extern int nf_axis_restart_server_raw(const unsigned int ip, const unsigned short port, const char *username, const char *password);


#endif	//__NF_IPCAM_DRIVER_AXIS_H__
