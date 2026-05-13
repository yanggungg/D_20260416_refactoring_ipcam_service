#ifndef __NF_IPCAMCBC_H__
#define __NF_IPCAMCBC_H__

#include "nf_object.h"

/* type macro */
#define NF_TYPE_IPCAMCBC				(nf_ipcamcbc_get_type ())

#define NF_IS_IPCAMCBC(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_IPCAMCBC))
#define NF_IS_IPCAMCBC_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_IPCAMCBC))

#define NF_IPCAMCBC_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_IPCAMCBC, NfIpCamCBCClass))
#define NF_IPCAMCBC(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_IPCAMCBC, NfIpCamCBC))
#define NF_IPCAMCBC_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_IPCAMCBC, NfIpCamCBCClass))

#define NF_IPCAMCBC_CAST(obj)			((NfIpCamCBC*)(obj))
#define NF_IPCAMCBC_CLASS_CAST(klass)	((NfIpCamCBCClass*)(klass))

typedef struct _NfIpCamCBC NfIpCamCBC;
typedef struct _NfIpCamCBCClass NfIpCamCBCClass;

/**
 * NfIpCamCBC:
 *
 * NfDVR Sysdb class
 */

struct _NfIpCamCBC {
	NfObject 	 	object;
	
	/*< public >*/
			
	/*< public >*/ /* with LOCK */
/*
	기능들만 모아서 ip camera (class)	
		
	http 수신
	제어 신호

	rtsp 수신
	ptz 제어
	
*/	
	/*< private >*/	
};

struct _NfIpCamCBCClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

#endif