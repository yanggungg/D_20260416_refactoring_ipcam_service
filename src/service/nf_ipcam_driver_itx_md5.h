/*
 * ITX Security
 *  System software group
 *
 *  2013-07-14 jykim
 */

#ifndef __NF_IPCAM_DRIVER_ITX_MD5_H__
#define __NF_IPCAM_DRIVER_ITX_MD5_H__

#include "nf_issm_ctl.h"

#define UNIT_TEST
#undef UNIT_TEST

#ifndef PUT_32
#define PUT_32(p,v)    ((p)[0]=((v)>>24)&0xff,(p)[1]=((v)>>16)&0xff,(p)[2]=((v)>>8)&0xff,(p)[3]=(v)&0xff)
#endif

static unsigned short cam_setup_reboot_idx = 0xea2e;

// SWIPXXP-790 exception

static const char str_api_raw_get[] = 
	"GET /cgi-bin/action.fcgi?action=get_setup&menu=system.info HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"%s";	// HTTP API




static const char str_api_raw[] = 
	"POST /cgi-bin/action.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"%s";	// HTTP API

static const char str_api_raw_va[] = 
	"POST /cgi-bin/vca.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	//"Content-Type: application/x-www-form-urlencoded\r\n"
	"Content-Type: multipart/form-data; boundary=---------------------------7db301281e86059e\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"\r\n"
	"%s";	// HTTP API

static const char str_api_raw_va_digest[] = 
	"POST /cgi-bin/vca.fcgi HTTP/1.1\r\n"
	"Accept: */*\r\n"
	"Accept-Language: ko\r\n"
	//"Content-Type: application/x-www-form-urlencoded\r\n"
	"Content-Type: multipart/form-data; boundary=---------------------------7db301281e86059e\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"User-Agent: IPX-NVR\r\n"
	"Host: %s\r\n"	// IP address
	"Content-Length: %d\r\n" // length
	"Connection: Keep-Alive\r\n"
	"Cache-Control: no-cache\r\n"
	"%s\r\n"
	"\r\n"
	"%s";	// HTTP API

static const char str_reboot_raw[] = 
	"GET /cgi-bin/reboot.cgi?api=%s HTTP/1.1\r\n"	// api
	"Host: %s\r\n"	// IP address
	"Connection: Keep-Alive\r\n"
	"Authorization: Basic %s\r\n"	// ID:PASSWORD b64 encoding
	"User-Agent: IPX-NVR\r\n"
	"Accept: */*\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"Accept-Language: ko\r\n"
	"\r\n";

static const char str_factory_raw[] = 
	"GET /cgi-bin/reboot.cgi?api=factory HTTP/1.1\r\n"	// api
	"Host: %s\r\n"	// IP address
	"Connection: Keep-Alive\r\n"
	"%s\r\n"	// Basic ID:PASSWORD b64 encoding
	"User-Agent: IPX-NVR\r\n"
	"Accept: */*\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"Accept-Language: ko\r\n"
	"\r\n";

static char* resol_string[] = {
	"1920x1080", "1920x1080_w", "1280x1024", "1024x768", "1280x720",
	"1280x720_w", "720x576", "720x480", "704x576", "704x480",
	"640x480", "640x352", "352x288", "352x240", "320x240",
	"640x360", "320x180", "640x360_w", "600x400", "800x450",
	"1440x900", "800x600", "1600x1200","2304x1296", "2048x1536",
	"2560x1440", "2688x1520",  "2560x1600","2560x1920", "2592x1920",
	"2592x1944", "2992x1680", "2880x1800", "3200x1800", "2880x2160",
    "3072x2048", "3200x2400", "3840x2160", "2592x1520", "3000x3000", "2048x2048", 
	"1280x1280", "640x640", "320x320",
};
static char* bitctrl_string[] = {
	"cbr", "vbr", "mbr", "idnr"
};

static char* vcodec_string[] = {
	"h264", "h265"
};

static char* fps_string[] = {
	"30", "15", "10", "7", "4", "2", "1", "25", "12", "6", "3", "12.5", "7.5", "6.3"
};
static char* gop_string[] = {
	"15", "7",  "5",  "3", "2", "1", "1", "12", "6",  "3", "2", "6",    "3",   "3"
};

/* typedef a 32 bit type */
typedef unsigned int UINT4;//ksi_test 64bit issue , unsigned long int -> unsigned int

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
	UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                                    /* scratch buffer */
	unsigned char in[64];                              /* input buffer */
	unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;


static void MD5Init (MD5_CTX *mdContext);
static void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf,unsigned int inLen);
static void MD5Final (MD5_CTX *mdContext);
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

static void MD5Init (MD5_CTX *mdContext)
{
	mdContext->i[0] = mdContext->i[1] = (UINT4)0;

	/* Load magic initialization constants.
	 */
	mdContext->buf[0] = (UINT4)0x67452301;
	mdContext->buf[1] = (UINT4)0xefcdab89;
	mdContext->buf[2] = (UINT4)0x98badcfe;
	mdContext->buf[3] = (UINT4)0x10325476;
}

static void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf,unsigned int inLen)
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

static void MD5Final (MD5_CTX *mdContext)
{
	UINT4 in[16];
	unsigned int mdi;
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

	/* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
	FF ( a, b, c, d, in[ 0], S11, 3614090360); /* 1 */
	FF ( d, a, b, c, in[ 1], S12, 3905402710); /* 2 */
	FF ( c, d, a, b, in[ 2], S13,  606105819); /* 3 */
	FF ( b, c, d, a, in[ 3], S14, 3250441966); /* 4 */
	FF ( a, b, c, d, in[ 4], S11, 4118548399); /* 5 */
	FF ( d, a, b, c, in[ 5], S12, 1200080426); /* 6 */
	FF ( c, d, a, b, in[ 6], S13, 2821735955); /* 7 */
	FF ( b, c, d, a, in[ 7], S14, 4249261313); /* 8 */
	FF ( a, b, c, d, in[ 8], S11, 1770035416); /* 9 */
	FF ( d, a, b, c, in[ 9], S12, 2336552879); /* 10 */
	FF ( c, d, a, b, in[10], S13, 4294925233); /* 11 */
	FF ( b, c, d, a, in[11], S14, 2304563134); /* 12 */
	FF ( a, b, c, d, in[12], S11, 1804603682); /* 13 */
	FF ( d, a, b, c, in[13], S12, 4254626195); /* 14 */
	FF ( c, d, a, b, in[14], S13, 2792965006); /* 15 */
	FF ( b, c, d, a, in[15], S14, 1236535329); /* 16 */

	/* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
	GG ( a, b, c, d, in[ 1], S21, 4129170786); /* 17 */
	GG ( d, a, b, c, in[ 6], S22, 3225465664); /* 18 */
	GG ( c, d, a, b, in[11], S23,  643717713); /* 19 */
	GG ( b, c, d, a, in[ 0], S24, 3921069994); /* 20 */
	GG ( a, b, c, d, in[ 5], S21, 3593408605); /* 21 */
	GG ( d, a, b, c, in[10], S22,   38016083); /* 22 */
	GG ( c, d, a, b, in[15], S23, 3634488961); /* 23 */
	GG ( b, c, d, a, in[ 4], S24, 3889429448); /* 24 */
	GG ( a, b, c, d, in[ 9], S21,  568446438); /* 25 */
	GG ( d, a, b, c, in[14], S22, 3275163606); /* 26 */
	GG ( c, d, a, b, in[ 3], S23, 4107603335); /* 27 */
	GG ( b, c, d, a, in[ 8], S24, 1163531501); /* 28 */
	GG ( a, b, c, d, in[13], S21, 2850285829); /* 29 */
	GG ( d, a, b, c, in[ 2], S22, 4243563512); /* 30 */
	GG ( c, d, a, b, in[ 7], S23, 1735328473); /* 31 */
	GG ( b, c, d, a, in[12], S24, 2368359562); /* 32 */

	/* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
	HH ( a, b, c, d, in[ 5], S31, 4294588738); /* 33 */
	HH ( d, a, b, c, in[ 8], S32, 2272392833); /* 34 */
	HH ( c, d, a, b, in[11], S33, 1839030562); /* 35 */
	HH ( b, c, d, a, in[14], S34, 4259657740); /* 36 */
	HH ( a, b, c, d, in[ 1], S31, 2763975236); /* 37 */
	HH ( d, a, b, c, in[ 4], S32, 1272893353); /* 38 */
	HH ( c, d, a, b, in[ 7], S33, 4139469664); /* 39 */
	HH ( b, c, d, a, in[10], S34, 3200236656); /* 40 */
	HH ( a, b, c, d, in[13], S31,  681279174); /* 41 */
	HH ( d, a, b, c, in[ 0], S32, 3936430074); /* 42 */
	HH ( c, d, a, b, in[ 3], S33, 3572445317); /* 43 */
	HH ( b, c, d, a, in[ 6], S34,   76029189); /* 44 */
	HH ( a, b, c, d, in[ 9], S31, 3654602809); /* 45 */
	HH ( d, a, b, c, in[12], S32, 3873151461); /* 46 */
	HH ( c, d, a, b, in[15], S33,  530742520); /* 47 */
	HH ( b, c, d, a, in[ 2], S34, 3299628645); /* 48 */

	/* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
	II ( a, b, c, d, in[ 0], S41, 4096336452); /* 49 */
	II ( d, a, b, c, in[ 7], S42, 1126891415); /* 50 */
	II ( c, d, a, b, in[14], S43, 2878612391); /* 51 */
	II ( b, c, d, a, in[ 5], S44, 4237533241); /* 52 */
	II ( a, b, c, d, in[12], S41, 1700485571); /* 53 */
	II ( d, a, b, c, in[ 3], S42, 2399980690); /* 54 */
	II ( c, d, a, b, in[10], S43, 4293915773); /* 55 */
	II ( b, c, d, a, in[ 1], S44, 2240044497); /* 56 */
	II ( a, b, c, d, in[ 8], S41, 1873313359); /* 57 */
	II ( d, a, b, c, in[15], S42, 4264355552); /* 58 */
	II ( c, d, a, b, in[ 6], S43, 2734768916); /* 59 */
	II ( b, c, d, a, in[13], S44, 1309151649); /* 60 */
	II ( a, b, c, d, in[ 4], S41, 4149444226); /* 61 */
	II ( d, a, b, c, in[11], S42, 3174756917); /* 62 */
	II ( c, d, a, b, in[ 2], S43,  718787259); /* 63 */
	II ( b, c, d, a, in[ 9], S44, 3951481745); /* 64 */

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

static void md5(char *inString, char* outString)
{
	MD5_CTX mdContext;
	unsigned int len = strlen (inString);

	MD5Init (&mdContext);
	MD5Update (&mdContext, (unsigned char*)inString, len);
	MD5Final (&mdContext);
	snprintf(outString, 36,
			"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			mdContext.digest[0], mdContext.digest[1], mdContext.digest[2],
			mdContext.digest[3], mdContext.digest[4], mdContext.digest[5],
			mdContext.digest[6], mdContext.digest[7], mdContext.digest[8],
			mdContext.digest[9], mdContext.digest[10], mdContext.digest[11],
			mdContext.digest[12], mdContext.digest[13], mdContext.digest[14],
			mdContext.digest[15]);
}

static void itx_digest_auth_str(char *user, char *pass, char *realm, char *nonce, char* uri, char *method, char *auth_str)
{
	char ha1[36];
	char ha2[36];
	char resp[128];
	char in_buf[1024];


	memset(in_buf, 0x00, 1024);
	memset(ha1, 0x00, 36);
	snprintf(in_buf, 1023, "%s:%s:%s", user, realm, pass);
	md5(in_buf, ha1);
	//LOG(MINOR, "%s | hash_1(%s)", __FUNCTION__, ha1);

	memset(in_buf, 0x00, 1024);
	memset(ha2, 0x00, 36);
	snprintf(in_buf, 511, "%s:%s", method, uri);
	md5(in_buf, ha2);
	//LOG(MINOR, "%s | hash_2(%s)", __FUNCTION__, ha2);

	memset(in_buf, 0x00, 1024);
	memset(resp, 0x00, 128);
	snprintf(in_buf, 511, "%s:%s:%s", ha1, nonce, ha2);
	md5(in_buf, resp);
	//LOG(MINOR, "%s | response(%s)", __FUNCTION__, resp);

	snprintf(auth_str, 1023, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"", user, realm, nonce, uri, resp);
}

static void itx_digest_auth_str_qop(char *user, char *pass, char *realm, char *nonce,
		char* uri, char *method, int noncecount, char *cnonce, char *qop, char *opaque,
		char *auth_str)
{
	char ha1[36];
	char ha2[36];
	char resp[128];
	char in_buf[1024];

	const char auth_raw[] =
			"Authorization: Digest username=\"%s\","
			"realm=\"%s\",nonce=\"%s\",uri=\"%s\","
			"cnonce=\"%s\",nc=%08x,response=\"%s\","
			"qop=\"%s\",opaque=\"%s\"";

	const char auth_none_opaque_raw[] =
			"Authorization: Digest username=\"%s\","
			"realm=\"%s\",nonce=\"%s\",uri=\"%s\","
			"cnonce=\"%s\",nc=%08x,response=\"%s\","
			"qop=\"%s\"";

	memset(in_buf, 0x00, 1024);
	memset(ha1, 0x00, 36);
	snprintf(in_buf, 1024, "%s:%s:%s", user, realm, pass);
	md5(in_buf, ha1);

	memset(in_buf, 0x00, 1024);
	memset(ha2, 0x00, 36);
	snprintf(in_buf, 512, "%s:%s", method, uri);
	md5(in_buf, ha2);

	memset(in_buf, 0x00, 1024);
	memset(resp, 0x00, 128);
	snprintf(in_buf, 512, "%s:%s:%08x:%s:%s:%s", ha1, nonce, noncecount, cnonce, qop, ha2);
	md5(in_buf, resp);

	if(strlen(opaque) > 0) {
		snprintf(auth_str, 512, auth_raw, user,
				realm, nonce, uri,
				cnonce, noncecount, resp,
				qop, opaque);
	} else {
		snprintf(auth_str, 512, auth_none_opaque_raw, user,
				realm, nonce, uri,
				cnonce, noncecount, resp,
				qop);
	}
}

static void create_nonce(char *nonce, char *digest_secret )
{
	unsigned char token[16];
	unsigned char tmp[128], tmp2[128], output[128];
	char *v[2] = { nonce, digest_secret };
	struct timeval now;
	int i;

	gettimeofday( &now, NULL );
	PUT_32( token, now.tv_sec );
	random_bytes( token + 4, sizeof( token ) - 4 );
	for( i = 0; i < 16; ++i )
		sprintf( tmp + (i<<1), "%02x", token[i] );
	tmp[32] = 0;

	snprintf(tmp2, 128, "%s:%s", tmp, digest_secret);
	md5(tmp2, output);

	memcpy(nonce, tmp, 32);
	memcpy(nonce+32, output, strlen(output));
	nonce[32] = '\0';
}

static size_t b64_encode_string_to_buffer(char *p_dst, size_t i_dst, const char *p_src)
{
	static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char in[3], out[4];
	int i, len, src_id, src_len = 0;
	size_t dst_len = 0;

	src_len = strlen(p_src);
	memset(p_dst, 0x00, i_dst);

	for (src_id = 0; src_id < src_len;)
	{
		len = 0;
		for (i=0; i<3; i++)
		{
			in[i] = *(p_src+src_id++);
			if (src_id <= src_len)
				len++;
			else
				in[i] = 0;
		}
		if (len)
		{
			out[0] = b64[in[0] >> 2];
			out[1] = b64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
			out[2] = (len > 1 ? b64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
			out[3] = (len > 2 ? b64[in[2] & 0x3f] : '=');
			for (i=0; i<4; i++)
			{
				*(p_dst+i) = out[i];
				dst_len++;
			}
			p_dst += 4;
		}
	}

	return dst_len;
}

#endif //__NF_IPCAM_DRIVER_ITX_MD5_H__
