#ifndef	__IUX_TYPES_H
#define __IUX_TYPES_H

////////////////////////////////////////////////////////////
//
// data types
//

typedef unsigned int bool; 
typedef unsigned int BITMASK;
typedef guint64		 BITMASK64;
typedef unsigned int SECOND_D;

typedef enum _ONOFF_E {
	ONOFF_OFF			= 0,
	ONOFF_ON			= 1,
} ONOFF_E;

typedef GThread*	CALLID;

#endif
