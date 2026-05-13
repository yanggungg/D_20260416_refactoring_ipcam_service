#ifndef __GST_MRTP_MD5_C_
#define __GST_MRTP_MD5_C_

/* typedef a 32 bit type */
#if 0
typedef unsigned long int UINT4;
#else
typedef unsigned int UINT4; //for 64Bit
#endif

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
	UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                                    /* scratch buffer */
	unsigned char in[64];                              /* input buffer */
	unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;


void MD5Init (MD5_CTX *mdContext);
void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf,unsigned int inLen);
void MD5Final (MD5_CTX *mdContext);
static void Transform (UINT4 *buf, UINT4 *in);

static unsigned char PADDING[64] = {
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z))) 

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
{(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) \
{(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) \
{(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}
#define II(a, b, c, d, x, s, ac) \
{(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(a) = ROTATE_LEFT ((a), (s)); \
	(a) += (b); \
}

void MD5Init (MD5_CTX *mdContext)
{
	mdContext->i[0] = mdContext->i[1] = (UINT4)0;

	/* Load magic initialization constants.
	 */
	mdContext->buf[0] = (UINT4)0x67452301;
	mdContext->buf[1] = (UINT4)0xefcdab89;
	mdContext->buf[2] = (UINT4)0x98badcfe;
	mdContext->buf[3] = (UINT4)0x10325476;
}

void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf,unsigned int inLen)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* update number of bits */
	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
		mdContext->i[1]++;
	mdContext->i[0] += ((UINT4)inLen << 3);
	mdContext->i[1] += ((UINT4)inLen >> 29);

	while (inLen--) {
		/* add new character to buffer, increment mdi */
		mdContext->in[mdi++] = *inBuf++;

		/* transform if necessary */
		if (mdi == 0x40) {
			for (i = 0, ii = 0; i < 16; i++, ii += 4)
				in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
					(((UINT4)mdContext->in[ii+2]) << 16) |
					(((UINT4)mdContext->in[ii+1]) << 8) |
					((UINT4)mdContext->in[ii]);
			Transform (mdContext->buf, in);
			mdi = 0;
		}
	}
}

void MD5Final (MD5_CTX *mdContext)
{
	UINT4 in[16];
	int mdi;
	unsigned int i, ii;
	unsigned int padLen;

	/* save number of bits */
	in[14] = mdContext->i[0];
	in[15] = mdContext->i[1];

	/* compute number of bytes mod 64 */
	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

	/* pad out to 56 mod 64 */
	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
	MD5Update (mdContext, PADDING, padLen);

	/* append length in bits and transform */
	for (i = 0, ii = 0; i < 14; i++, ii += 4)
		in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
			(((UINT4)mdContext->in[ii+2]) << 16) |
			(((UINT4)mdContext->in[ii+1]) << 8) |
			((UINT4)mdContext->in[ii]);
	Transform (mdContext->buf, in);

	/* store buffer in digest */
	for (i = 0, ii = 0; i < 4; i++, ii += 4) {
		mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
		mdContext->digest[ii+1] =
			(unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
		mdContext->digest[ii+2] =
			(unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
		mdContext->digest[ii+3] =
			(unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
	}
}

/* Basic MD5 step. Transform buf based on in.
 */
static void Transform (UINT4 *buf, UINT4 *in)
{
	UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	unsigned int ff1  = 0xd76aa478;
	unsigned int ff2  = 0xe8c7b756;
	unsigned int ff3  = 0x242070db;
	unsigned int ff4  = 0xc1bdceee;
	unsigned int ff5  = 0xf57c0faf;
	unsigned int ff6  = 0x4787c62a;
	unsigned int ff7  = 0xa8304613;
	unsigned int ff8  = 0xfd469501;
	unsigned int ff9  = 0x698098d8;
	unsigned int ff10 = 0x8b44f7af;
	unsigned int ff11 = 0xffff5bb1;
	unsigned int ff12 = 0x895cd7be;
	unsigned int ff13 = 0x6b901122;
	unsigned int ff14 = 0xfd987193;
	unsigned int ff15 = 0xa679438e;
	unsigned int ff16 = 0x49b40821;

	unsigned int gg1  = 0xf61e2562;
	unsigned int gg2  = 0xc040b340;
	unsigned int gg3  = 0x265e5a51;
	unsigned int gg4  = 0xe9b6c7aa;
	unsigned int gg5  = 0xd62f105d;
	unsigned int gg6  = 0x2441453;
	unsigned int gg7  = 0xd8a1e681;
	unsigned int gg8  = 0xe7d3fbc8;
	unsigned int gg9  = 0x21e1cde6;
	unsigned int gg10 = 0xc33707d6;
	unsigned int gg11 = 0xf4d50d87;
	unsigned int gg12 = 0x455a14ed;
	unsigned int gg13 = 0xa9e3e905;
	unsigned int gg14 = 0xfcefa3f8;
	unsigned int gg15 = 0x676f02d9;
	unsigned int gg16 = 0x8d2a4c8a;

	unsigned int hh1  = 0xfffa3942;
	unsigned int hh2  = 0x8771f681;
	unsigned int hh3  = 0x6d9d6122;
	unsigned int hh4  = 0xfde5380c;
	unsigned int hh5  = 0xa4beea44;
	unsigned int hh6  = 0x4bdecfa9;
	unsigned int hh7  = 0xf6bb4b60;
	unsigned int hh8  = 0xbebfbc70;
	unsigned int hh9  = 0x289b7ec6;
	unsigned int hh10 = 0xeaa127fa;
	unsigned int hh11 = 0xd4ef3085;
	unsigned int hh12 = 0x4881d05;
	unsigned int hh13 = 0xd9d4d039;
	unsigned int hh14 = 0xe6db99e5;
	unsigned int hh15 = 0x1fa27cf8;
	unsigned int hh16 = 0xc4ac5665;

	unsigned int ii1  = 0xf4292244;
	unsigned int ii2  = 0x432aff97;
	unsigned int ii3  = 0xab9423a7;
	unsigned int ii4  = 0xfc93a039;
	unsigned int ii5  = 0x655b59c3;
	unsigned int ii6  = 0x8f0ccc92;
	unsigned int ii7  = 0xffeff47d;
	unsigned int ii8  = 0x85845dd1;
	unsigned int ii9  = 0x6fa87e4f;
	unsigned int ii10 = 0xfe2ce6e0;
	unsigned int ii11 = 0xa3014314;
	unsigned int ii12 = 0x4e0811a1;
	unsigned int ii13 = 0xf7537e82;
	unsigned int ii14 = 0xbd3af235;
	unsigned int ii15 = 0x2ad7d2bb;
	unsigned int ii16 = 0xeb86d391;

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF ( a, b, c, d, in[ 0], S11, ff1	 ); /* 1 */
	FF ( d, a, b, c, in[ 1], S12, ff2	 ); /* 2 */
	FF ( c, d, a, b, in[ 2], S13, ff3	 ); /* 3 */
	FF ( b, c, d, a, in[ 3], S14, ff4	 ); /* 4 */
	FF ( a, b, c, d, in[ 4], S11, ff5	 ); /* 5 */
	FF ( d, a, b, c, in[ 5], S12, ff6	 ); /* 6 */
	FF ( c, d, a, b, in[ 6], S13, ff7	 ); /* 7 */
	FF ( b, c, d, a, in[ 7], S14, ff8	 ); /* 8 */
	FF ( a, b, c, d, in[ 8], S11, ff9	 ); /* 9 */
	FF ( d, a, b, c, in[ 9], S12, ff10	 ); /* 10 */
	FF ( c, d, a, b, in[10], S13, ff11	 ); /* 11 */
	FF ( b, c, d, a, in[11], S14, ff12	 ); /* 12 */
	FF ( a, b, c, d, in[12], S11, ff13	 ); /* 13 */
	FF ( d, a, b, c, in[13], S12, ff14	 ); /* 14 */
	FF ( c, d, a, b, in[14], S13, ff15	 ); /* 15 */
	FF ( b, c, d, a, in[15], S14, ff16	 ); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG ( a, b, c, d, in[ 1], S21, gg1 ); /* 17 */
	GG ( d, a, b, c, in[ 6], S22, gg2 ); /* 18 */
	GG ( c, d, a, b, in[11], S23, gg3 ); /* 19 */
	GG ( b, c, d, a, in[ 0], S24, gg4 ); /* 20 */
	GG ( a, b, c, d, in[ 5], S21, gg5 ); /* 21 */
	GG ( d, a, b, c, in[10], S22, gg6 ); /* 22 */
	GG ( c, d, a, b, in[15], S23, gg7 ); /* 23 */
	GG ( b, c, d, a, in[ 4], S24, gg8 ); /* 24 */
	GG ( a, b, c, d, in[ 9], S21, gg9 ); /* 25 */
	GG ( d, a, b, c, in[14], S22, gg10); /* 26 */
	GG ( c, d, a, b, in[ 3], S23, gg11); /* 27 */
	GG ( b, c, d, a, in[ 8], S24, gg12); /* 28 */
	GG ( a, b, c, d, in[13], S21, gg13); /* 29 */
	GG ( d, a, b, c, in[ 2], S22, gg14); /* 30 */
	GG ( c, d, a, b, in[ 7], S23, gg15); /* 31 */
	GG ( b, c, d, a, in[12], S24, gg16); /* 32 */

	/* Round 3 */
#define S31 4
#define gint 11
#define S33 16
#define S34 23
	HH ( a, b, c, d, in[ 5], S31, hh1 ); /* 33 */
	HH ( d, a, b, c, in[ 8], gint, hh2 ); /* 34 */
	HH ( c, d, a, b, in[11], S33, hh3 ); /* 35 */
	HH ( b, c, d, a, in[14], S34, hh4 ); /* 36 */
	HH ( a, b, c, d, in[ 1], S31, hh5 ); /* 37 */
	HH ( d, a, b, c, in[ 4], gint, hh6 ); /* 38 */
	HH ( c, d, a, b, in[ 7], S33, hh7 ); /* 39 */
	HH ( b, c, d, a, in[10], S34, hh8 ); /* 40 */
	HH ( a, b, c, d, in[13], S31, hh9 ); /* 41 */
	HH ( d, a, b, c, in[ 0], gint, hh10); /* 42 */
	HH ( c, d, a, b, in[ 3], S33, hh11); /* 43 */
	HH ( b, c, d, a, in[ 6], S34, hh12); /* 44 */
	HH ( a, b, c, d, in[ 9], S31, hh13); /* 45 */
	HH ( d, a, b, c, in[12], gint, hh14); /* 46 */
	HH ( c, d, a, b, in[15], S33, hh15); /* 47 */
	HH ( b, c, d, a, in[ 2], S34, hh16); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II ( a, b, c, d, in[ 0], S41, ii1 ); /* 49 */
	II ( d, a, b, c, in[ 7], S42, ii2 ); /* 50 */
	II ( c, d, a, b, in[14], S43, ii3 ); /* 51 */
	II ( b, c, d, a, in[ 5], S44, ii4 ); /* 52 */
	II ( a, b, c, d, in[12], S41, ii5 ); /* 53 */
	II ( d, a, b, c, in[ 3], S42, ii6 ); /* 54 */
	II ( c, d, a, b, in[10], S43, ii7 ); /* 55 */
	II ( b, c, d, a, in[ 1], S44, ii8 ); /* 56 */
	II ( a, b, c, d, in[ 8], S41, ii9 ); /* 57 */
	II ( d, a, b, c, in[15], S42, ii10); /* 58 */
	II ( c, d, a, b, in[ 6], S43, ii11); /* 59 */
	II ( b, c, d, a, in[13], S44, ii12); /* 60 */
	II ( a, b, c, d, in[ 4], S41, ii13); /* 61 */
	II ( d, a, b, c, in[11], S42, ii14); /* 62 */
	II ( c, d, a, b, in[ 2], S43, ii15); /* 63 */
	II ( b, c, d, a, in[ 9], S44, ii16); /* 64 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*
 **********************************************************************
 ** End of md5.c                                                     **
 ******************************* (cut) ********************************
 */

/*
 **********************************************************************
 ** md5driver.c -- sample routines to test                           **
 ** RSA Data Security, Inc. MD5 message digest algorithm.            **
 ** Created: 2/16/90 RLR                                             **
 ** Updated: 1/91 SRD                                                **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
/* -- include the following file if the file md5.h is separate -- */
/* #include "md5.h" */

/* size of test block */
#define TEST_BLOCK_SIZE 1000

/* number of blocks to process */
#define TEST_BLOCKS 10000

/* number of test bytes = TEST_BLOCK_SIZE * TEST_BLOCKS */

/* Computes the message digest for string inString.
   Prints out message digest, a space, the string (in quotes) and a
   carriage return.
 */

extern void mrtpsrc_rtsp_client_md5(char *inString, char* outString)
{
	MD5_CTX mdContext;
	unsigned int len = strlen (inString);

	MD5Init (&mdContext);
	MD5Update (&mdContext, (unsigned char*)inString, len);
	MD5Final (&mdContext);
	g_snprintf(outString, 36,
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			mdContext.digest[0], mdContext.digest[1], mdContext.digest[2],
			mdContext.digest[3], mdContext.digest[4], mdContext.digest[5],
			mdContext.digest[6], mdContext.digest[7], mdContext.digest[8],
			mdContext.digest[9], mdContext.digest[10], mdContext.digest[11],
			mdContext.digest[12], mdContext.digest[13], mdContext.digest[14],
			mdContext.digest[15]);
	//MDPrint (&mdContext);
	//printf (" \"%s\"\n\n", inString);
}

#endif //__GST_MRTP_MD5_C_
