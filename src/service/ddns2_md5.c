#ifdef _WIN32
#include <wtypes.h>
#else
#include <sys/types.h>
//typedef uint32_t UINT32;		parangi
typedef unsigned int UINT32;
typedef unsigned char BYTE;
//typedef boolean BOOL;
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "ddns2_md5.h"

#define N_ARRAY(a)				(sizeof(a)/sizeof(a[0]))

#define ROL(a, b)		((a) << (b) | (a) >> (32-b))
// read little endian, a is PBYTE
#define READ_LE(a)		(a[0] | a[1] << 8 | a[2] << 16 | a[3] << 24)

static void PerBlock(UINT32 state[4], BYTE src[64])
{
	static UINT32 k[64] = {/* k[i] = (UINT32)(fabs(sin(i+1)) * 4294967296.0 */
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
	};
	int isLittleEndian = *(UINT32 *)"\x01\x02\x03\x04" == 0x04030201L;
	g_assert(sizeof(int) >= 4);
	{
		UINT32 a = state[0], b = state[1], c = state[2], d = state[3], x;
		UINT32 w[16], i;

		// prepare 16 little endian UINT32 src words
		if (isLittleEndian)
			memcpy(w, src, 64);
		else {
			for (i = 0; i < 16; i++, src += 4)
				w[i] = READ_LE(src);
		}

#define X(i, a, b, c, d, r) x = a + F(b, c, d) + k[i] + w[G(i)];\
		a = b + ROL(x, r);

#define Y(i, r1, r2, r3, r4) X(i, a, b, c, d, r1)\
		X(i+1, d, a, b, c, r2)\
		X(i+2, c, d, a, b, r3)\
		X(i+3, b, c, d, a, r4)

#define Z(i, r1, r2, r3, r4) Y(i, r1, r2, r3, r4)\
		Y(i+4, r1, r2, r3, r4)\
		Y(i+8, r1, r2, r3, r4)\
		Y(i+12, r1, r2, r3, r4)

		// round 1
#define F(b, c, d)			(((b)&(c)) | ((~b)&(d)))
#define G(i)				(i)
		Z(0, 7, 12, 17, 22)
#undef F
#undef G

		// round 2
#define F(b, c, d)			(((d)&(b)) | ((~d)&(c)))
#define G(i)				(5*(i)+1) % 16
		Z(16, 5, 9, 14, 20)
#undef F
#undef G

		// round 3
#define F(b, c, d)			(b ^ c ^ d)
#define G(i)				(3*(i)+5) % 16
		Z(32, 4, 11, 16, 23)
#undef F
#undef G

		// round 4
#define F(b, c, d)			(c ^ ((b) | (~d)))
#define G(i)				(7*(i)) % 16
		Z(48, 6, 10, 15, 21)
#undef F
#undef G

#undef X
#undef Y
#undef Z
		state[0] += a; state[1] += b; state[2] += c; state[3] += d;
	}
}

void Md5(void *pDest, const void *pSrc, UINT srcSize)
{
	UINT32 state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
	BYTE *s = (BYTE *)pSrc;
	UINT n = srcSize;
	for (; n >= 64; n -= 64, s += 64)
		PerBlock(state, s);

	// padding
	{
		BYTE tail[64*2];
		UINT m;
		memcpy(tail, s, n);
		tail[n] = 0x80;
		m = (64*2 - 9 - n) % 64;
		memset(tail+n+1, 0, m);
		{
			UINT32 nBits[2] = {(UINT32)(srcSize << 3), (UINT32)(srcSize >> 29)};
			int i;
			for (i = 0; i < sizeof(nBits); i++)
				tail[n+1+m+i] = nBits[i/4] >> ((i%4)*8) & 0xff;
		}
		PerBlock(state, tail);
		if (n+1+m+8 > 64) PerBlock(state, tail + 64);
	}
	{
		int i;
		for (i = 0; i < sizeof(state); i++)
			((BYTE *)pDest)[i] = state[i/4] >> ((i%4)*8) & 0xff;
	}
}

void Md5Str(void *pDest, LPCSTR pSrc)
{
	Md5(pDest, pSrc, strlen(pSrc));
}

