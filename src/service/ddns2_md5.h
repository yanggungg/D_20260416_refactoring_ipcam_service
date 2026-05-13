/*
	md5 hash.

	http://en.wikipedia.org/wiki/Md5

	2008.7.16
	Ahope
	Shin-myoung-serp
*/

#ifndef _MD5_H_
#define _MD5_H_

#ifdef _WIN32
//#include <wtypes.h>
#else
typedef unsigned int UINT;
typedef const char *LPCSTR;
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define MD5_DEST_SIZE		16
#define MD5_BLOCK_SIZE  64

// pDest should be size of MD5_DEST_SIZE bytes
void Md5(void *pDest, const void *pSrc, UINT srcSize);
void Md5Str(void *pDest, LPCSTR pSrc);

#ifdef __cplusplus
}
#endif

#endif
