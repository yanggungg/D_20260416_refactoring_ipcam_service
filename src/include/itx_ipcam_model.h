
//baek.add.2010.10.29.fixme
//#include "../../junsun/js_comdef.h"

#ifndef ITX_IPCAM_MODEL_H
#define ITX_IPCAM_MODEL_H

#define NAMELEN				64	// From Sysdb limitation.

#ifdef BAEK_FEATURE_VENDOR_SWVER


//define.vendor.code
#define	V_VCN	96
#define	V_KBD	78
#define	V_ERV	92
#define	V_CBC	32
#define	V_Y3K	93
#define	V_ITX	100
#define	V_GPS	95
#define	V_PSP	100

#ifdef BAEK_FEATURE_CHECK_VENDOR_CODE

typedef struct _vendor_info
{
	char success;
	unsigned char vendor;
	unsigned char country;
	char p_date[13];
	unsigned char model;
	unsigned char lens;
} vendor_info;

enum {
	COUNTRY_KR = 0,
	COUNTRY_US,
	COUNTRY_ER,
	COUNTRY_UK,
	COUNTRY_TH
};

//Model Index : See the model table(itx_ipcam_model.c)
enum {   
	STD_MODEL_NAME = 0,
	ERV_MODEL_NAME,
	VCN_MODEL_NAME,
	CBC_MODEL_NAME,
};

//check vendor code
#define CHECK_TOKEN_SUCCESS		(1<<0)
#define CHECK_VENDOR_SUCCESS	(1<<1)
#define CHECK_COUNTRY_SUCCESS	(1<<2)
#define CHECK_PDATE_SUCCESS		(1<<3)
#define CHECK_MODEL_SUCCESS		(1<<4)
#define CHECK_LENS_SUCCESS		(1<<5)
#define CHECK_ETC_SUCCESS		(1<<6)

typedef struct _vendor_vendor {
	const char 		name[4];
	const char 		swver[6];
	unsigned int	model;
	const char		brand[32];
	unsigned int	code;
}vendor_vendor;

typedef struct _vendor_country {
	const char 		*name;
	unsigned int	code;
}vendor_country;

#else  /*BAEK_FEATURE_CHECK_VENDOR_CODE*/

typedef struct _vendor_info
{
	char vendor[4];
	char country[3];
	char p_date[13];
	char etc[12];
} vendor_info;
#endif

enum TZ_INFO
{
/* 00 */     ETC_GMT_12	=	0,
/* 01 */     PACIFIC_MIDWAY,
/* 02 */     US_HAWAII,
/* 03 */     AMERICA_ANCHORAGE,
/* 04 */     AMERICA_LOS_ANGELES,
/* 05 */     AMERICA_PHOENIX,
/* 06 */     US_CENTRAL,
/* 07 */     US_EASTERN,
/* 08 */     AMERICA_HALIFAX,
/* 09 */     AMERICA_ST_JOHNS,
/* 10 */     AMERICA_SAO_PAULO,
/* 11 */     ETC_GMT_2,
/* 12 */     ATLANTIC_AZORES,
/* 13 */     EUROPE_LONDON,
/* 14 */     EUROPE_BERLIN,
/* 15 */     EUROPE_ISTANBUL,
/* 16 */     AFRICA_CAIRO,
/* 17 */     EUROPE_MOSCOW,
/* 18 */     ASIA_TEHRAN,
/* 19 */     ASIA_MUSCAT,
/* 20 */     ASIA_KABUL,
/* 21 */     ASIA_KARACHI,
/* 22 */     ASIA_CALCUTTA,
/* 23 */     ASIA_KATMANDU,
/* 24 */     ASIA_DHAKA,
/* 25 */     ASIA_RANGOON,
/* 26 */     ASIA_BANGKOK,
/* 27 */     ASIA_SHANGHAI,
/* 28 */     ASIA_TOKYO,
/* 29 */     ASIA_SEOUL,
/* 30 */     AUSTRALIA_DARWIN,
/* 31 */     AUSTRALIA_ADELAIDE,
/* 32 */     AUSTRALIA_BRISBANE,
/* 33 */     PACIFIC_NOUMEA,
/* 34 */     NZ,
/* 35 */     AUSTRALIA_PERTH,
/* 36 */     ASIA_AMMAN,
/* 37 */     ASIA_BEIRUT,
/* 38 */     ASIA_DAMASCUS,
/* 39 */     ASIA_RIYADH,
/* 40 */     ASIA_BAGHDAD,
/* 41 */     ASIA_TEHRAN_41,
/* 42 */     ASIA_DUBAI,
/* 43 */     AUSTRALIA_SYDNEY
};
#endif


typedef enum _module
{
	FRONT_MODULE = 0,
	NETWORK_MODULE,
	REAR_MODULE
} module_t;

typedef struct _cam_modules
{
	int FM;
	int NM;
	int RM;
} cam_modules_t;

#define OP_OFF			(0)
#define OP_ON			(1)

#define OP_LENS_DEF		(0)		// Any lens wo iris
#define OP_LENS_NCX		(1)		// Any lans wi DC-iris	: NCX-all
#define OP_LENS_FIX		(2)		// very Focal DC-iris	: NCD, NCDi, NCB-0350
#define OP_LENS_MFZ		(3)		// Auto Focus DC-iris	: NCD, NCDi, NCB-1300, 2000
#define OP_LENS_PMFZ	(4)		// Auto Focus P-iris	: NCD, NCDi, NCB-1300, 2000

typedef struct _ip_cam
{
	cam_modules_t modules;
	
	int op_af;		// Dome 1300, 2000 only
	int op_heater;	// Dome and Bullet
	int op_dc_iris;	// DC-iris type
} ip_cam_t;


// Define Front end Module
#define FM_BASE				0x00

#define FM_M_AP				FM_BASE		// Aptina Maga Sensor
#define FM_D1_AP			0x02		// Aptina D1 Sensor (H/W is same Maga Sensor)

// Define Network Module
#define NM_BASE				0x00

#define NM_288S				0x01
#define NM_255S				NM_BASE
#define NM_233S				0x02

// Defina Rear Module
#define RM_BASE				0x00

#define RM_X_MODEL			RM_BASE
#define RM_D_MODEL			0x02
#define RM_B_MODEL			0x04
#define	RM_Di_MODEL			0x07


#define MODEL_NCX_2000P_ID		((FM_M_AP)|(NM_288S<<8)|(RM_X_MODEL<<16))		/* 0.1.0 */
#define MODEL_NCD_2000P_ID		((FM_M_AP)|(NM_288S<<8)|(RM_D_MODEL<<16))		/* 0.1.2 */
#define MODEL_NCB_2000P_ID		((FM_M_AP)|(NM_288S<<8)|(RM_B_MODEL<<16))		/* 0.1.4 */
#define MODEL_NCDi_2000P_ID		((FM_M_AP)|(NM_288S<<8)|(RM_Di_MODEL<<16))		/* 0.1.7 */

#define MODEL_NCX_1300P_ID		((FM_M_AP)|(NM_255S<<8)|(RM_X_MODEL<<16))		/* 0.0.0 */
#define MODEL_NCD_1300P_ID		((FM_M_AP)|(NM_255S<<8)|(RM_D_MODEL<<16))		/* 0.0.2 */
#define MODEL_NCB_1300P_ID		((FM_M_AP)|(NM_255S<<8)|(RM_B_MODEL<<16))		/* 0.0.4 */
#define MODEL_NCDi_1300P_ID		((FM_M_AP)|(NM_255S<<8)|(RM_Di_MODEL<<16))		/* 0.0.7 */

#define MODEL_NCX_0350P_ID		((FM_D1_AP)|(NM_255S<<8)|(RM_X_MODEL<<16))		/* 2.0.0 */
#define MODEL_NCD_0350P_ID		((FM_D1_AP)|(NM_255S<<8)|(RM_D_MODEL<<16))		/* 2.0.2 */
#define MODEL_NCB_0350P_ID		((FM_D1_AP)|(NM_255S<<8)|(RM_B_MODEL<<16))		/* 2.0.4 */
#define MODEL_NCDi_0350P_ID		((FM_D1_AP)|(NM_255S<<8)|(RM_Di_MODEL<<16))		/* 2.0.7 */


const char*	get_brand_name(void);
char* 		get_model_name(const char* str_model_id);

#ifndef USE_CHECKER
int 		get_module_id(const char* str_model_id, module_t m);
int 		get_module_id_all(const char* str_model_id, cam_modules_t *modules);
int			get_model_options(ip_cam_t *ip_cam);
void		print_model_options(ip_cam_t *ip_cam);
int 		set_model_sysdb_options(ip_cam_t *ip_cam);
//baek.add.2010.10.29
#ifdef BAEK_FEATURE_VENDOR
char*		load_vendor_code(void);
int			set_vendor_sysdb_options(vendor_info vinfo);
vendor_info get_vendor_info(char* vd);
#endif

#ifdef BAEK_FEATURE_VENDOR_SWVER
int set_vendor_swver(vendor_info vinfo);
#endif

#endif

#endif//ITX_IPCAM_MODEL_H
