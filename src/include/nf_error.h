#ifndef _NF_ERROR_H_
#define _NF_ERROR_H_

#include "nf_common.h"

/*
enum _nf_errno_
{
	ERR_CREATION,
	ERR_MALLOC,
	ERR_QUE_FULL,
	ERR_QUE,
	ERR_DEQUE,
	ERR_POST,
	ERR_WHO,
	ERR_CHANNEL_ID,
	ERR_WAIT,
	END_OF_ERROR_MESSAGE
};
*/

#define	ERR_CREATION	0
#define	ERR_MALLOC		1
#define	ERR_QUE_FULL	2
#define	ERR_QUE			3
#define	ERR_DEQUE		4
#define	ERR_POST		5
#define	ERR_WHO			7
#define	ERR_CHANNEL_ID	8
#define	ERR_WAIT		9

#endif

