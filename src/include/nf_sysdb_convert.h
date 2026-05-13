#ifndef __NF_SYSDB_CONVERT_H__
#define __NF_SYSDB_CONVERT_H__

typedef enum _SYSDB_TYPE_E
{
	SYSDB_TYPE_NULL         =0,
	SYSDB_TYPE_INT          =1,
	SYSDB_TYPE_UINT         =2,
	SYSDB_TYPE_STRING       =3,
	SYSDB_TYPE_BOOL         =4
}SYSDB_TYPE;

#define NF_SYSDB_CONVERT_MAX_PARAM    1024

typedef struct _SYSDB_CONVERT_TABLE_T
{
	char    *prop_name;
	int     val_type;
	int     idx;
	int     strlen;
}SYSDB_CONVERT_TABLE;

extern const gchar *_sysdb_convert_cate_str[];

typedef enum _NF_SYSDB_CONVERT_TYPE_E
{
	NF_SYSDB_CONVERT_TYPE_SYS                 =0,
	NF_SYSDB_CONVERT_TYPE_NET                 =1,
	NF_SYSDB_CONVERT_TYPE_AUDIO               =2,
	NF_SYSDB_CONVERT_TYPE_DISK                =3,
	NF_SYSDB_CONVERT_TYPE_CAM                 =4,
	NF_SYSDB_CONVERT_TYPE_USR                 =5,
	NF_SYSDB_CONVERT_TYPE_ALARM               =6,
	NF_SYSDB_CONVERT_TYPE_ACT                 =7,
	NF_SYSDB_CONVERT_TYPE_DISP                =8,
	NF_SYSDB_CONVERT_TYPE_REC                 =9,
	NF_SYSDB_CONVERT_TYPE_IPCAM               =10,
	NF_SYSDB_CONVERT_TYPE_IPREC               =11,
	NF_SYSDB_CONVERT_TYPE_NR
}NF_SYSDB_CONVERT_TYPE_E;

#endif

