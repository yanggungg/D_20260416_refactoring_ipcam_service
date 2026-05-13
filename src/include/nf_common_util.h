#ifndef _NF_COMMON_UTIL_H
#define _NF_COMMON_UTIL_H

#include <glib.h>

#if 0
	#define CHECK_SERR(A,B,C,D) if(A==B) {perror(C); g_assert(1);}
	#define CHECK_HERR(A,B,C) if(A==B) {printf("%s HERROR:"C"\n", __FUNCTION__); g_assert(1);}
	#define MSG(A)	printf(A"\n");
#endif

#define	HWORD(x)	(((x) >> 16) & 0xffff)
#define	LWORD(x)	((x) & 0xffff)
#define	HBYTE(x)	(((x) >> 8) & 0xff)
#define	LBYTE(x)	((x) & 0xff)
#define	HNIBBLE(x)	(((x) >> 4) & 0xf)
#define	LNIBBLE(x)	((x) & 0xf)

#ifndef MAX
	#define	MAX(x, y)		(((x) >= (y))?(x):(y))
#endif

#ifndef MIN
	#define	MIN(x, y)		(((x) <= (y))?(y):(x))
#endif

#ifndef MIN3
	#define MIN3(x, y, z)	(( MIN(x, y) > z ) ? z : MIN(x, y) )
#endif

#ifndef MAX3
	#define MAX3(x, y, z)	(( MAX(x, y) > z ) ? MAX(x, y) : z )
#endif

#ifndef ABS
	#define ABS(x)			( ((x) > 0) ? (x) : (-(x)) )
#endif

#ifndef SIGN
	#define SIGN(X)			(((X)>0)?1:-1)
#endif

#ifndef ALIGN
	#define ALIGN(t, s, n)		( (s+n-1) & ( (t)~(n-1) ) )
#endif
#ifndef IS_ALIGN
	#define IS_ALIGN(t, s, n)	( (s&((t)(n-1)) ) == 0 )
#endif

#define GETBIT(x, y)        (((x)>>(y)) & 0x01)
#define	SETBIT(x, y)		((x) |= (1 << (y)))
#define RESETBIT(x, y)		((x) &= ~(1 << (y)))

#define	BSWAP32(a)			(a)
#define	BSWAP16(a)			(a)	

// time convert 
#define GUINT64_SECOND  (G_USEC_PER_SEC * G_GINT64_CONSTANT (1000))
#define GUINT64_USECOND (GUINT64_SECOND / G_GINT64_CONSTANT (1000000))

#define GTIMEVAL_TO_GUINT64(tv)	(guint64)((tv).tv_sec * GUINT64_SECOND + (tv).tv_usec * GUINT64_USECOND)
#define GUINT64_TO_GTIMEVAL(t,tv)							\
G_STMT_START {												\
  (tv).tv_sec  = ((guint64) (t)) / GUINT64_SECOND;			\
  (tv).tv_usec = (((guint64) (t)) -							\
                  ((guint64) (tv).tv_sec) * GUINT64_SECOND)	\
                 / GUINT64_USECOND;							\
} G_STMT_END


#define DEBUG_CHECK_POINT(msg)	g_message("%s(%d) %s : %s",__FILE__, __LINE__, __FUNCTION__, (msg) );

#endif
