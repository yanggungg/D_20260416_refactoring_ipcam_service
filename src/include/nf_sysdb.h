#ifndef __NF_SYSDB_H__
#define __NF_SYSDB_H__

#include "nf_object.h"
#include "nf_sysdb_convert.h"

typedef enum _NF_SYSDB_CATE_E {

	NF_SYSDB_CATE_FACTORY_DEFAULT = 0xFFFF0000,
	NF_SYSDB_CATE_LOAD_DATA	= 0xEEEE0000,
//onvif_porting
	NF_SYSDB_CATE_UPDATE = 0xDDDD0000,	
//onvif_porting
	
	NF_SYSDB_CATE_SYS		= 0,
	
	NF_SYSDB_CATE_NET		= 1,
	NF_SYSDB_CATE_AUDIO		= 2,
	NF_SYSDB_CATE_DISK		= 3,
	NF_SYSDB_CATE_CAM		= 4,
	NF_SYSDB_CATE_USR		= 5,

	NF_SYSDB_CATE_ALARM		= 6,
	NF_SYSDB_CATE_ACT		= 7,
	NF_SYSDB_CATE_DISP		= 8,
	NF_SYSDB_CATE_REC		= 9,	

#ifdef ENABLE_HNF_IPCAM
	NF_SYSDB_CATE_IPCAM		= 10,
	NF_SYSDB_CATE_IPREC		= 11,
	NF_SYSDB_CATE_IPACT		= 12,
#endif
#ifdef ENABLE_ONVIF_DEVICE
	NF_SYSDB_CATE_ONVIF,
#endif
	NF_SYSDB_CATE_NR   		

	
} NF_SYSDB_CATE_E;

typedef enum _NF_SYSDB_TMP_CHANGE_EVENTID_E {
	NF_SYSDB_TMP_CHANGE_EVENTID_NULL	= 0,
	NF_SYSDB_TMP_CHANGE_EVENTID_PTZ 	= 1,		//param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_MOTION	= 2,		//param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_ONVIF	= 3,		//param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_IMAGE	= 4,		//param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_MOUNT   = 5,        //param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_DEWARP  = 6,        //param[1] : ch
	NF_SYSDB_TMP_CHANGE_EVENTID_NR		= 7 
} NF_SYSDB_TMP_CHANGE_EVENT;

/* type macro */
#define NF_TYPE_SYSDB					(nf_sysdb_get_type ())

#define NF_IS_SYSDB(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_SYSDB))
#define NF_IS_SYSDB_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_SYSDB))

#define NF_SYSDB_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_SYSDB, NfSysDbClass))
#define NF_SYSDB(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_SYSDB, NfSysDb))
#define NF_SYSDB_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_SYSDB, NfSysDbClass))

#define NF_SYSDB_CAST(obj)				((NfSysDb*)(obj))
#define NF_SYSDB_CLASS_CAST(klass)		((NfSysDbClass*)(klass))

typedef struct _NfSysDb NfSysDb;
typedef struct _NfSysDbClass NfSysDbClass;

/**
 * NfSysDb:
 *
 * NfDVR Sysdb class
 */

struct _NfSysDb {
	NfObject 	 	object;
	
	/*< public >*/
	gint			init_done;
	
	GMutex *cate_lock[NF_SYSDB_CATE_NR];
	
	/*< public >*/ /* with LOCK */
	
	/*< private >*/		    	
	GParamSpecPool  *spec_pool;
};

struct _NfSysDbClass {

  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef enum _NF_SYSDB_INIT_E {
	NF_SYSDB_INIT_FACTORY_DEFAULT = 0,
	NF_SYSDB_INIT_LOAD_DATA = 1,
	NF_SYSDB_INIT_FW_UPGRADE = 2,
} NF_SYSDB_INIT_E;

gboolean 
nf_sysdb_init( int wait, int load_data, char *mtd_path );

gboolean 
nf_sysdb_test();

gboolean 
nf_sysdb_dump_all();

gboolean 
nf_sysdb_dump();

void 
nf_sysdb_lock(NF_SYSDB_CATE_E cate);

gboolean 
nf_sysdb_trylock(NF_SYSDB_CATE_E cate);

void 
nf_sysdb_unlock(NF_SYSDB_CATE_E cate);  


gboolean 
nf_sysdb_import( const gchar *filename );

gboolean 
nf_sysdb_export( const gchar *filename );

gboolean 
nf_sysdb_import_test( const gchar *filename );

gboolean
nf_sysdb_import_fwup( const gchar *path );

#if 0

gboolean
nf_sysdb_db_get (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error);

GValue *
nf_sysdb_db_get_gvalue (NfSysDb   *self, 
						const gchar *property_name );

gboolean
nf_sysdb_db_set (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error);
		      
gboolean
nf_sysdb_db_validate (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error);
#endif 

gboolean 
nf_sysdb_get_key0(	const gchar *key, 
						GValue *retval, GError **error);					

gboolean 
nf_sysdb_get_key1( 	const gchar *key, 
						const guint idx0 , 
						GValue *retval, GError **error);						
gboolean 
nf_sysdb_get_key2(	const gchar *key, 
						const guint idx0, const guint idx1 , 
						GValue *retval,	GError **error);

gboolean
nf_sysdb_get_key3(	const gchar *key,
						const guint idx0, const guint idx1 , const guint idx2 ,
						GValue *retval,	GError **error);

gchar*
nf_sysdb_get_str( const gchar *property_name );

gchar*
nf_sysdb_get_str_nocopy( const gchar *property_name );

gint
nf_sysdb_get_strmap( const gchar *property_name, gint idx );

gboolean
nf_sysdb_get_bool( const gchar *property_name );

guint
nf_sysdb_get_uint ( const gchar *property_name );

gint
nf_sysdb_get_int ( const gchar *property_name );


gboolean 
nf_sysdb_set_key0(	const gchar *key, 
						GValue *retval, GError **error);
gboolean 
nf_sysdb_set_key1( 	const gchar *key, 
						const guint idx0 , 
						GValue *retval, GError **error);
gboolean 
nf_sysdb_set_key2(	const gchar *key, 
						const guint idx0, const guint idx1 , 
						GValue *retval,	GError **error);

gboolean 
nf_sysdb_set_key3(	const gchar *key, 
						const guint idx0, const guint idx1 , const guint idx2 ,
						GValue *retval,	GError **error);
gboolean
nf_sysdb_set_int ( const gchar *property_name, gint value );

gboolean
nf_sysdb_set_uint ( const gchar *property_name, guint value );

gboolean
nf_sysdb_set_bool ( const gchar *property_name, gboolean value );

gboolean
nf_sysdb_set_str ( const gchar *property_name, gchar *value );

gboolean 
nf_sysdb_validate_key0(	const gchar *key, 
						GValue *retval, GError **error);

gboolean
nf_sysdb_validate_int ( const gchar *property_name, gint value );

gboolean
nf_sysdb_validate_uint ( const gchar *property_name, guint value );

gboolean
nf_sysdb_validate_bool ( const gchar *property_name, gboolean value );

gboolean
nf_sysdb_validate_str ( const gchar *property_name, gchar *value );


						
gboolean 
nf_sysdb_load( const gchar *category );

gboolean 
nf_sysdb_default( const gchar *category );

gboolean 
nf_sysdb_save( const gchar *category );
						
void 
nf_sysdb_save_all();

void
nf_sysdb_save_flush();

void
nf_sysdb_convert_test();
 
gboolean
nf_sysdb_buff_to_sysdb(guint idx, gchar *buff, gint size);

gboolean
nf_sysdb_sysdb_to_buff(guint idx , gchar *buff, gint *size);

gboolean
nf_sysdb_convert_value_validate_check(guint idx, gchar *buff, gint size);

#define NF_SYSDB_LICENSE_KEY_STR_LEN	35
#define NF_SYSDB_LICENSE_KEY_RAW_LEN	30

#define NF_SYSDB_LICENSE_KEY_NAME_LEN	8
#define NF_SYSDB_LICENSE_KEY_NAME

#define NF_SYSDB_LICENSE_MAC_LEN		6
#define NF_SYSDB_LICENSE_MODEL_LEN		10

typedef struct _NF_SYSDB_LICENSE_SECRET_KEY {	
	// iv
	gchar	mac_addr[NF_SYSDB_LICENSE_MAC_LEN];  	// { 0x00, 0x11, 0x5f, 0xab, 0xcd, 0xef }
	gchar	model[NF_SYSDB_LICENSE_MODEL_LEN];		// "xxxxxx"		

	// secret key		
	// gchar	secret_key[16];
			
} NF_SYSDB_LICENSE_SECRET_KEY;


typedef struct _NF_SYSDB_LICENSE_INFO {		
	gchar	name[NF_SYSDB_LICENSE_KEY_NAME_LEN];	
	guint	param1;
	guint	param2;

	// OK74A-AEWSX-LVK2A-N6NQE-FA35E-IAAAA	    						
	gchar	key[NF_SYSDB_LICENSE_KEY_STR_LEN+1];	
} NF_SYSDB_LICENSE_INFO;



// 0  ok
// -1 wrong format
// -2 wrong model 
// -3 wrong mac_address
// -4 null  param
// -5 wrong lic
gint nf_sysdb_license_decoding( NF_SYSDB_LICENSE_SECRET_KEY *secret_key,
									char *license_key,	NF_SYSDB_LICENSE_INFO *out_key );

// >= 0 ok   license count
// -1 connect error
// -2 				
gint nf_sysdb_license_recive_from_svr( NF_SYSDB_LICENSE_SECRET_KEY *secret_key,
									NF_SYSDB_LICENSE_INFO *out_arr_keys, 
									int out_max_count);

/*
on success
	sysdb idx
	
on error
	-1 : param error
	-2 : not found
*/		
gint nf_sysdb_license_get_from_sysdb( char *lic_name, NF_SYSDB_LICENSE_INFO *out);

#endif

