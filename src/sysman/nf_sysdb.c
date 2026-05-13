#include "nf_common.h"
#include "nf_sysdb.h"
#include "nf_debug.h"
#include "nf_notify.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "sysdb"

//#define DEBUG_SYSDB_ALWAYS_DEFAULT
#define NF_SYSDB_CONVERT
//#define DEBUG_SYSDB_LOAD
//#define DEBUG_SYSDB_SAVE
//#define DEBUG_SYSDB_TEST
//#define DEBUG_SYSDB_CONVERT
//#define DEBUG_SYSDB_CONVERT_TEST
#define DEBUG_SYSDB_SET_VALIDATE		// choissi 2009-09-07 ???? 5:21:20 enable
#define DEBUG_SYSDB_JBSHELL
//#define ENABLE_SYSDB_CONVERT_TABLE	// 2009-11-11 ???? 7:39:18

#ifdef DEBUG_SYSDB_JBSHELL
	#include "jbshell.h"
#endif

#ifdef NF_SYSDB_CONVERT
	#include "nf_sysdb_convert_struct_info.h"
	#include "nf_sysdb_convert_table.h"
#endif

#define DEBUG_SYSDB_LOG

typedef enum _DEBUG_SYSDB_IDX_E
{
	DEBUG_SYSDB_IDX_INIT 		= 0,
	DEBUG_SYSDB_IDX_GET 		= 1,
	DEBUG_SYSDB_IDX_SET 		= 2,
	DEBUG_SYSDB_IDX_VALIDATE	= 3,

	DEBUG_SYSDB_IDX_LOCK 		= 4,
	DEBUG_SYSDB_IDX_UNLCOK 		= 5,
	DEBUG_SYSDB_IDX_LOAD 		= 6,
	DEBUG_SYSDB_IDX_SAVE 		= 7,

	DEBUG_SYSDB_IDX_DEFAULT		= 8,
	DEBUG_SYSDB_IDX_GET_VALUE	= 9,
	DEBUG_SYSDB_IDX_SET_VALUE	= 10,
	DEBUG_SYSDB_IDX_LOAD_SKIP	= 11,
	DEBUG_SYSDB_IDX_NR			= 12
}DEBUG_SYSDB_IDX_E;

static const char *_DEBUG_SYSDB_str[32] =
{

	"SYSDB_IDX_INIT",
	"SYSDB_IDX_GET",
	"SYSDB_IDX_SET",
	"SYSDB_IDX_VALIDATE",

	"SYSDB_IDX_LOCK",
	"SYSDB_IDX_UNLCOK",
	"SYSDB_IDX_LOAD",
	"SYSDB_IDX_SAVE",

	"SYSDB_IDX_DEFAULT",
	"SYSDB_IDX_GET_VALUE",
	"SYSDB_IDX_SET_VALUE",
	"SYSDB_IDX_SKIP_LOAD",
	"SYSDB_IDX_NR"
};

static guint _DEBUG_SYSDB_log[32] =
{
	0,0,0,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

//#ifdef ENABLE_USER_PASSWORD_ENCODING
#if 1

#define NF_SYSDB_PASSWORD_ENC_PREFIX		",pw@@wp,"
#define NF_SYSDB_PASSWORD_KEY_START_MAKER	"usr.U"
#define NF_SYSDB_PASSWORD_KEY_END_MAKER		".pass"

/* ************************************** SEED   ********************************** */
#define KC0 	0x9e3779b9
#define KC1 	0x3c6ef373
#define KC2 	0x78dde6e6
#define KC3 	0xf1bbcdcc
#define KC4 	0xe3779b99
#define KC5 	0xc6ef3733
#define KC6 	0x8dde6e67
#define KC7 	0x1bbcdccf
#define KC8 	0x3779b99e
#define KC9 	0x6ef3733c
#define KC10	0xdde6e678
#define KC11	0xbbcdccf1
#define KC12	0x779b99e3
#define KC13	0xef3733c6
#define KC14	0xde6e678d
#define KC15	0xbcdccf1b

#define AI_ECB					1
#define AI_CBC					2
#define AI_OFB					3
#define AI_CFB					4

#define AI_NO_PADDING			1	//	Padding ??��(?Է??? 16????Ʈ?? ????)
#define AI_PKCS_PADDING			2	//	padding?Ǵ? ????Ʈ ???? padding
#define SEED_BLOCK_LEN			16	//	in BYTEs
#define SEED_USER_KEY_LEN		16	//	in BYTEs
#define SEED_NO_ROUNDS			16
#define SEED_NO_ROUNDKEY		(2*SEED_NO_ROUNDS)	//in DWORDs
#define ENC_TYPE		0
#define DEC_TYPE		1

//error code
#define CTR_SUCCESS			0
#define NULL_PTR_ERROR			0x0001  //	?Ķ????Ϳ? NULL ?????Ͱ? ?Էµ?.
#define CTR_FATAL_ERROR			0x1001
#define CTR_INVALID_USERKEYLEN		0x1002	//	????Ű?? ???̰? ????????.
#define CTR_PAD_CHECK_ERROR		0x1003	//	
#define CTR_DATA_LEN_ERROR		0x1004	//	?????? ???̰? ????????.
#define CTR_CIPHER_LEN_ERROR		0x1005	//	??ȣ???? ?????? ?????? ?ƴ?.
#define RET_VAL		unsigned int
#define DWORD		unsigned int
#define BYTE		unsigned char

typedef struct {
	DWORD ModeID;				//	ECB or CBC
	DWORD PadType;			//	???Ͼ?ȣ?? Padding type
	BYTE IV[SEED_BLOCK_LEN];		//	Initial Vector
	BYTE ChainVar[SEED_BLOCK_LEN];	//	Chaining Variable
	BYTE Buffer[SEED_BLOCK_LEN];		//	Buffer for unfilled block
	DWORD BufLen; 			//	Buffer?? ��ȿ ????Ʈ ??
	DWORD RoundKey[SEED_NO_ROUNDKEY];	//	?????? Ű?? DWORD ??
} SEED_ALG_INFO;

#if defined(_MSC_VER)
#define ROTL_DWORD(x, n) _lrotl((x), (n))
#define ROTR_DWORD(x, n) _lrotr((x), (n))
#else
#define ROTL_DWORD(x, n) ( (DWORD)((x) << (n)) | (DWORD)((x) >> (32-(n))) )
#define ROTR_DWORD(x, n) ( (DWORD)((x) >> (n)) | (DWORD)((x) << (32-(n))) )
#endif

////////	reverse the byte order of DWORD(DWORD:4-bytes integer) and WORD.
#define ENDIAN_REVERSE_DWORD(dwS)	( (ROTL_DWORD((dwS),  8) & 0x00ff00ff)	\
									 | (ROTL_DWORD((dwS), 24) & 0xff00ff00) )

////////	move DWORD type to BYTE type and BYTE type to DWORD type
#if 0
////	Big-Endian machine
#define BIG_B2D(B, D)		D = *(DWORD *)(B)
#define BIG_D2B(D, B)		*(DWORD *)(B) = (DWORD)(D)
#define LITTLE_B2D(B, D)	D = ENDIAN_REVERSE_DWORD(*(DWORD *)(B))
#define LITTLE_D2B(D, B)	*(DWORD *)(B) = ENDIAN_REVERSE_DWORD(D)
#else
////	Little-Endian machine
#define BIG_B2D(B, D)		D = ENDIAN_REVERSE_DWORD(*(DWORD *)(B))
#define BIG_D2B(D, B)		*(DWORD *)(B) = ENDIAN_REVERSE_DWORD(D)
#define LITTLE_B2D(B, D)	D = *(DWORD *)(B)
#define LITTLE_D2B(D, B)	*(DWORD *)(B) = (DWORD)(D)
#endif

#define EncRoundKeyUpdate0(K, A, B, C, D, KC) {					\
	T0 = A; 								\
	A = (A>>8) ^ (B<<24);							\
	B = (B>>8) ^ (T0<<24);							\
	T0 = A + C - KC;							\
	T1 = B + KC - D;							\
	(K)[0] = SEED_SL[0][(T0    )&0xFF] ^ SEED_SL[1][(T0>> 8)&0xFF]		\
		   ^ SEED_SL[2][(T0>>16)&0xFF] ^ SEED_SL[3][(T0>>24)&0xFF];	\
	(K)[1] = SEED_SL[0][(T1    )&0xFF] ^ SEED_SL[1][(T1>> 8)&0xFF]		\
		   ^ SEED_SL[2][(T1>>16)&0xFF] ^ SEED_SL[3][(T1>>24)&0xFF];	\
}

#define EncRoundKeyUpdate1(K, A, B, C, D, KC) {					\
	T0 = C; 								\
	C = (C<<8) ^ (D>>24);							\
	D = (D<<8) ^ (T0>>24);							\
	T0 = A + C - KC;							\
	T1 = B + KC - D;							\
	(K)[0] = SEED_SL[0][(T0    )&0xFF] ^ SEED_SL[1][(T0>> 8)&0xFF]		\
		   ^ SEED_SL[2][(T0>>16)&0xFF] ^ SEED_SL[3][(T0>>24)&0xFF];	\
	(K)[1] = SEED_SL[0][(T1    )&0xFF] ^ SEED_SL[1][(T1>> 8)&0xFF]		\
		   ^ SEED_SL[2][(T1>>16)&0xFF] ^ SEED_SL[3][(T1>>24)&0xFF];	\
}

#define BlockCopy(pbDst, pbSrc) {			\
	((DWORD *)(pbDst))[0] = ((DWORD *)(pbSrc))[0];	\
	((DWORD *)(pbDst))[1] = ((DWORD *)(pbSrc))[1];	\
	((DWORD *)(pbDst))[2] = ((DWORD *)(pbSrc))[2];	\
	((DWORD *)(pbDst))[3] = ((DWORD *)(pbSrc))[3];	\
}

#define BlockXor(pbDst, phSrc1, phSrc2) {					\
	((DWORD *)(pbDst))[0] = ((DWORD *)(phSrc1))[0]				\
						  ^ ((DWORD *)(phSrc2))[0];	\
	((DWORD *)(pbDst))[1] = ((DWORD *)(phSrc1))[1]				\
						  ^ ((DWORD *)(phSrc2))[1];	\
	((DWORD *)(pbDst))[2] = ((DWORD *)(phSrc1))[2]				\
						  ^ ((DWORD *)(phSrc2))[2];	\
	((DWORD *)(pbDst))[3] = ((DWORD *)(phSrc1))[3]				\
						  ^ ((DWORD *)(phSrc2))[3];	\
}

#define SeedRound(A,B,C,D, K) {							\
	DWORD T0, T1;								\
	T0 = C ^ (K)[0];							\
	T1 = D ^ (K)[1];							\
	T1 ^= T0;								\
	T1 = SEED_SL[0][(T1    )&0xFF] ^ SEED_SL[1][(T1>> 8)&0xFF]		\
	   ^ SEED_SL[2][(T1>>16)&0xFF] ^ SEED_SL[3][(T1>>24)&0xFF];		\
	T0 += T1;								\
	T0 = SEED_SL[0][(T0    )&0xFF] ^ SEED_SL[1][(T0>> 8)&0xFF]		\
	   ^ SEED_SL[2][(T0>>16)&0xFF] ^ SEED_SL[3][(T0>>24)&0xFF];		\
	T1 += T0;								\
	T1 = SEED_SL[0][(T1    )&0xFF] ^ SEED_SL[1][(T1>> 8)&0xFF]		\
	   ^ SEED_SL[2][(T1>>16)&0xFF] ^ SEED_SL[3][(T1>>24)&0xFF];		\
	T0 += T1;								\
	A ^= T0; B ^= T1;							\
}

/***************** CONST DATA *********************************************/

static DWORD SEED_SL[4][256] = { { 0x2989a1a8, 0x05858184, 0x16c6d2d4,
		0x13c3d3d0, 0x14445054, 0x1d0d111c, 0x2c8ca0ac, 0x25052124, 0x1d4d515c,
		0x03434340, 0x18081018, 0x1e0e121c, 0x11415150, 0x3cccf0fc, 0x0acac2c8,
		0x23436360, 0x28082028, 0x04444044, 0x20002020, 0x1d8d919c, 0x20c0e0e0,
		0x22c2e2e0, 0x08c8c0c8, 0x17071314, 0x2585a1a4, 0x0f8f838c, 0x03030300,
		0x3b4b7378, 0x3b8bb3b8, 0x13031310, 0x12c2d2d0, 0x2ecee2ec, 0x30407070,
		0x0c8c808c, 0x3f0f333c, 0x2888a0a8, 0x32023230, 0x1dcdd1dc, 0x36c6f2f4,
		0x34447074, 0x2ccce0ec, 0x15859194, 0x0b0b0308, 0x17475354, 0x1c4c505c,
		0x1b4b5358, 0x3d8db1bc, 0x01010100, 0x24042024, 0x1c0c101c, 0x33437370,
		0x18889098, 0x10001010, 0x0cccc0cc, 0x32c2f2f0, 0x19c9d1d8, 0x2c0c202c,
		0x27c7e3e4, 0x32427270, 0x03838380, 0x1b8b9398, 0x11c1d1d0, 0x06868284,
		0x09c9c1c8, 0x20406060, 0x10405050, 0x2383a3a0, 0x2bcbe3e8, 0x0d0d010c,
		0x3686b2b4, 0x1e8e929c, 0x0f4f434c, 0x3787b3b4, 0x1a4a5258, 0x06c6c2c4,
		0x38487078, 0x2686a2a4, 0x12021210, 0x2f8fa3ac, 0x15c5d1d4, 0x21416160,
		0x03c3c3c0, 0x3484b0b4, 0x01414140, 0x12425250, 0x3d4d717c, 0x0d8d818c,
		0x08080008, 0x1f0f131c, 0x19899198, 0x00000000, 0x19091118, 0x04040004,
		0x13435350, 0x37c7f3f4, 0x21c1e1e0, 0x3dcdf1fc, 0x36467274, 0x2f0f232c,
		0x27072324, 0x3080b0b0, 0x0b8b8388, 0x0e0e020c, 0x2b8ba3a8, 0x2282a2a0,
		0x2e4e626c, 0x13839390, 0x0d4d414c, 0x29496168, 0x3c4c707c, 0x09090108,
		0x0a0a0208, 0x3f8fb3bc, 0x2fcfe3ec, 0x33c3f3f0, 0x05c5c1c4, 0x07878384,
		0x14041014, 0x3ecef2fc, 0x24446064, 0x1eced2dc, 0x2e0e222c, 0x0b4b4348,
		0x1a0a1218, 0x06060204, 0x21012120, 0x2b4b6368, 0x26466264, 0x02020200,
		0x35c5f1f4, 0x12829290, 0x0a8a8288, 0x0c0c000c, 0x3383b3b0, 0x3e4e727c,
		0x10c0d0d0, 0x3a4a7278, 0x07474344, 0x16869294, 0x25c5e1e4, 0x26062224,
		0x00808080, 0x2d8da1ac, 0x1fcfd3dc, 0x2181a1a0, 0x30003030, 0x37073334,
		0x2e8ea2ac, 0x36063234, 0x15051114, 0x22022220, 0x38083038, 0x34c4f0f4,
		0x2787a3a4, 0x05454144, 0x0c4c404c, 0x01818180, 0x29c9e1e8, 0x04848084,
		0x17879394, 0x35053134, 0x0bcbc3c8, 0x0ecec2cc, 0x3c0c303c, 0x31417170,
		0x11011110, 0x07c7c3c4, 0x09898188, 0x35457174, 0x3bcbf3f8, 0x1acad2d8,
		0x38c8f0f8, 0x14849094, 0x19495158, 0x02828280, 0x04c4c0c4, 0x3fcff3fc,
		0x09494148, 0x39093138, 0x27476364, 0x00c0c0c0, 0x0fcfc3cc, 0x17c7d3d4,
		0x3888b0b8, 0x0f0f030c, 0x0e8e828c, 0x02424240, 0x23032320, 0x11819190,
		0x2c4c606c, 0x1bcbd3d8, 0x2484a0a4, 0x34043034, 0x31c1f1f0, 0x08484048,
		0x02c2c2c0, 0x2f4f636c, 0x3d0d313c, 0x2d0d212c, 0x00404040, 0x3e8eb2bc,
		0x3e0e323c, 0x3c8cb0bc, 0x01c1c1c0, 0x2a8aa2a8, 0x3a8ab2b8, 0x0e4e424c,
		0x15455154, 0x3b0b3338, 0x1cccd0dc, 0x28486068, 0x3f4f737c, 0x1c8c909c,
		0x18c8d0d8, 0x0a4a4248, 0x16465254, 0x37477374, 0x2080a0a0, 0x2dcde1ec,
		0x06464244, 0x3585b1b4, 0x2b0b2328, 0x25456164, 0x3acaf2f8, 0x23c3e3e0,
		0x3989b1b8, 0x3181b1b0, 0x1f8f939c, 0x1e4e525c, 0x39c9f1f8, 0x26c6e2e4,
		0x3282b2b0, 0x31013130, 0x2acae2e8, 0x2d4d616c, 0x1f4f535c, 0x24c4e0e4,
		0x30c0f0f0, 0x0dcdc1cc, 0x08888088, 0x16061214, 0x3a0a3238, 0x18485058,
		0x14c4d0d4, 0x22426260, 0x29092128, 0x07070304, 0x33033330, 0x28c8e0e8,
		0x1b0b1318, 0x05050104, 0x39497178, 0x10809090, 0x2a4a6268, 0x2a0a2228,
		0x1a8a9298 },
		{ 0x38380830, 0xe828c8e0, 0x2c2d0d21, 0xa42686a2, 0xcc0fcfc3,
				0xdc1eced2, 0xb03383b3, 0xb83888b0, 0xac2f8fa3, 0x60204060,
				0x54154551, 0xc407c7c3, 0x44044440, 0x6c2f4f63, 0x682b4b63,
				0x581b4b53, 0xc003c3c3, 0x60224262, 0x30330333, 0xb43585b1,
				0x28290921, 0xa02080a0, 0xe022c2e2, 0xa42787a3, 0xd013c3d3,
				0x90118191, 0x10110111, 0x04060602, 0x1c1c0c10, 0xbc3c8cb0,
				0x34360632, 0x480b4b43, 0xec2fcfe3, 0x88088880, 0x6c2c4c60,
				0xa82888a0, 0x14170713, 0xc404c4c0, 0x14160612, 0xf434c4f0,
				0xc002c2c2, 0x44054541, 0xe021c1e1, 0xd416c6d2, 0x3c3f0f33,
				0x3c3d0d31, 0x8c0e8e82, 0x98188890, 0x28280820, 0x4c0e4e42,
				0xf436c6f2, 0x3c3e0e32, 0xa42585a1, 0xf839c9f1, 0x0c0d0d01,
				0xdc1fcfd3, 0xd818c8d0, 0x282b0b23, 0x64264662, 0x783a4a72,
				0x24270723, 0x2c2f0f23, 0xf031c1f1, 0x70324272, 0x40024242,
				0xd414c4d0, 0x40014141, 0xc000c0c0, 0x70334373, 0x64274763,
				0xac2c8ca0, 0x880b8b83, 0xf437c7f3, 0xac2d8da1, 0x80008080,
				0x1c1f0f13, 0xc80acac2, 0x2c2c0c20, 0xa82a8aa2, 0x34340430,
				0xd012c2d2, 0x080b0b03, 0xec2ecee2, 0xe829c9e1, 0x5c1d4d51,
				0x94148490, 0x18180810, 0xf838c8f0, 0x54174753, 0xac2e8ea2,
				0x08080800, 0xc405c5c1, 0x10130313, 0xcc0dcdc1, 0x84068682,
				0xb83989b1, 0xfc3fcff3, 0x7c3d4d71, 0xc001c1c1, 0x30310131,
				0xf435c5f1, 0x880a8a82, 0x682a4a62, 0xb03181b1, 0xd011c1d1,
				0x20200020, 0xd417c7d3, 0x00020202, 0x20220222, 0x04040400,
				0x68284860, 0x70314171, 0x04070703, 0xd81bcbd3, 0x9c1d8d91,
				0x98198991, 0x60214161, 0xbc3e8eb2, 0xe426c6e2, 0x58194951,
				0xdc1dcdd1, 0x50114151, 0x90108090, 0xdc1cccd0, 0x981a8a92,
				0xa02383a3, 0xa82b8ba3, 0xd010c0d0, 0x80018181, 0x0c0f0f03,
				0x44074743, 0x181a0a12, 0xe023c3e3, 0xec2ccce0, 0x8c0d8d81,
				0xbc3f8fb3, 0x94168692, 0x783b4b73, 0x5c1c4c50, 0xa02282a2,
				0xa02181a1, 0x60234363, 0x20230323, 0x4c0d4d41, 0xc808c8c0,
				0x9c1e8e92, 0x9c1c8c90, 0x383a0a32, 0x0c0c0c00, 0x2c2e0e22,
				0xb83a8ab2, 0x6c2e4e62, 0x9c1f8f93, 0x581a4a52, 0xf032c2f2,
				0x90128292, 0xf033c3f3, 0x48094941, 0x78384870, 0xcc0cccc0,
				0x14150511, 0xf83bcbf3, 0x70304070, 0x74354571, 0x7c3f4f73,
				0x34350531, 0x10100010, 0x00030303, 0x64244460, 0x6c2d4d61,
				0xc406c6c2, 0x74344470, 0xd415c5d1, 0xb43484b0, 0xe82acae2,
				0x08090901, 0x74364672, 0x18190911, 0xfc3ecef2, 0x40004040,
				0x10120212, 0xe020c0e0, 0xbc3d8db1, 0x04050501, 0xf83acaf2,
				0x00010101, 0xf030c0f0, 0x282a0a22, 0x5c1e4e52, 0xa82989a1,
				0x54164652, 0x40034343, 0x84058581, 0x14140410, 0x88098981,
				0x981b8b93, 0xb03080b0, 0xe425c5e1, 0x48084840, 0x78394971,
				0x94178793, 0xfc3cccf0, 0x1c1e0e12, 0x80028282, 0x20210121,
				0x8c0c8c80, 0x181b0b13, 0x5c1f4f53, 0x74374773, 0x54144450,
				0xb03282b2, 0x1c1d0d11, 0x24250521, 0x4c0f4f43, 0x00000000,
				0x44064642, 0xec2dcde1, 0x58184850, 0x50124252, 0xe82bcbe3,
				0x7c3e4e72, 0xd81acad2, 0xc809c9c1, 0xfc3dcdf1, 0x30300030,
				0x94158591, 0x64254561, 0x3c3c0c30, 0xb43686b2, 0xe424c4e0,
				0xb83b8bb3, 0x7c3c4c70, 0x0c0e0e02, 0x50104050, 0x38390931,
				0x24260622, 0x30320232, 0x84048480, 0x68294961, 0x90138393,
				0x34370733, 0xe427c7e3, 0x24240420, 0xa42484a0, 0xc80bcbc3,
				0x50134353, 0x080a0a02, 0x84078783, 0xd819c9d1, 0x4c0c4c40,
				0x80038383, 0x8c0f8f83, 0xcc0ecec2, 0x383b0b33, 0x480a4a42,
				0xb43787b3 }, { 0xa1a82989, 0x81840585, 0xd2d416c6, 0xd3d013c3,
				0x50541444, 0x111c1d0d, 0xa0ac2c8c, 0x21242505, 0x515c1d4d,
				0x43400343, 0x10181808, 0x121c1e0e, 0x51501141, 0xf0fc3ccc,
				0xc2c80aca, 0x63602343, 0x20282808, 0x40440444, 0x20202000,
				0x919c1d8d, 0xe0e020c0, 0xe2e022c2, 0xc0c808c8, 0x13141707,
				0xa1a42585, 0x838c0f8f, 0x03000303, 0x73783b4b, 0xb3b83b8b,
				0x13101303, 0xd2d012c2, 0xe2ec2ece, 0x70703040, 0x808c0c8c,
				0x333c3f0f, 0xa0a82888, 0x32303202, 0xd1dc1dcd, 0xf2f436c6,
				0x70743444, 0xe0ec2ccc, 0x91941585, 0x03080b0b, 0x53541747,
				0x505c1c4c, 0x53581b4b, 0xb1bc3d8d, 0x01000101, 0x20242404,
				0x101c1c0c, 0x73703343, 0x90981888, 0x10101000, 0xc0cc0ccc,
				0xf2f032c2, 0xd1d819c9, 0x202c2c0c, 0xe3e427c7, 0x72703242,
				0x83800383, 0x93981b8b, 0xd1d011c1, 0x82840686, 0xc1c809c9,
				0x60602040, 0x50501040, 0xa3a02383, 0xe3e82bcb, 0x010c0d0d,
				0xb2b43686, 0x929c1e8e, 0x434c0f4f, 0xb3b43787, 0x52581a4a,
				0xc2c406c6, 0x70783848, 0xa2a42686, 0x12101202, 0xa3ac2f8f,
				0xd1d415c5, 0x61602141, 0xc3c003c3, 0xb0b43484, 0x41400141,
				0x52501242, 0x717c3d4d, 0x818c0d8d, 0x00080808, 0x131c1f0f,
				0x91981989, 0x00000000, 0x11181909, 0x00040404, 0x53501343,
				0xf3f437c7, 0xe1e021c1, 0xf1fc3dcd, 0x72743646, 0x232c2f0f,
				0x23242707, 0xb0b03080, 0x83880b8b, 0x020c0e0e, 0xa3a82b8b,
				0xa2a02282, 0x626c2e4e, 0x93901383, 0x414c0d4d, 0x61682949,
				0x707c3c4c, 0x01080909, 0x02080a0a, 0xb3bc3f8f, 0xe3ec2fcf,
				0xf3f033c3, 0xc1c405c5, 0x83840787, 0x10141404, 0xf2fc3ece,
				0x60642444, 0xd2dc1ece, 0x222c2e0e, 0x43480b4b, 0x12181a0a,
				0x02040606, 0x21202101, 0x63682b4b, 0x62642646, 0x02000202,
				0xf1f435c5, 0x92901282, 0x82880a8a, 0x000c0c0c, 0xb3b03383,
				0x727c3e4e, 0xd0d010c0, 0x72783a4a, 0x43440747, 0x92941686,
				0xe1e425c5, 0x22242606, 0x80800080, 0xa1ac2d8d, 0xd3dc1fcf,
				0xa1a02181, 0x30303000, 0x33343707, 0xa2ac2e8e, 0x32343606,
				0x11141505, 0x22202202, 0x30383808, 0xf0f434c4, 0xa3a42787,
				0x41440545, 0x404c0c4c, 0x81800181, 0xe1e829c9, 0x80840484,
				0x93941787, 0x31343505, 0xc3c80bcb, 0xc2cc0ece, 0x303c3c0c,
				0x71703141, 0x11101101, 0xc3c407c7, 0x81880989, 0x71743545,
				0xf3f83bcb, 0xd2d81aca, 0xf0f838c8, 0x90941484, 0x51581949,
				0x82800282, 0xc0c404c4, 0xf3fc3fcf, 0x41480949, 0x31383909,
				0x63642747, 0xc0c000c0, 0xc3cc0fcf, 0xd3d417c7, 0xb0b83888,
				0x030c0f0f, 0x828c0e8e, 0x42400242, 0x23202303, 0x91901181,
				0x606c2c4c, 0xd3d81bcb, 0xa0a42484, 0x30343404, 0xf1f031c1,
				0x40480848, 0xc2c002c2, 0x636c2f4f, 0x313c3d0d, 0x212c2d0d,
				0x40400040, 0xb2bc3e8e, 0x323c3e0e, 0xb0bc3c8c, 0xc1c001c1,
				0xa2a82a8a, 0xb2b83a8a, 0x424c0e4e, 0x51541545, 0x33383b0b,
				0xd0dc1ccc, 0x60682848, 0x737c3f4f, 0x909c1c8c, 0xd0d818c8,
				0x42480a4a, 0x52541646, 0x73743747, 0xa0a02080, 0xe1ec2dcd,
				0x42440646, 0xb1b43585, 0x23282b0b, 0x61642545, 0xf2f83aca,
				0xe3e023c3, 0xb1b83989, 0xb1b03181, 0x939c1f8f, 0x525c1e4e,
				0xf1f839c9, 0xe2e426c6, 0xb2b03282, 0x31303101, 0xe2e82aca,
				0x616c2d4d, 0x535c1f4f, 0xe0e424c4, 0xf0f030c0, 0xc1cc0dcd,
				0x80880888, 0x12141606, 0x32383a0a, 0x50581848, 0xd0d414c4,
				0x62602242, 0x21282909, 0x03040707, 0x33303303, 0xe0e828c8,
				0x13181b0b, 0x01040505, 0x71783949, 0x90901080, 0x62682a4a,
				0x22282a0a, 0x92981a8a }, { 0x08303838, 0xc8e0e828, 0x0d212c2d,
				0x86a2a426, 0xcfc3cc0f, 0xced2dc1e, 0x83b3b033, 0x88b0b838,
				0x8fa3ac2f, 0x40606020, 0x45515415, 0xc7c3c407, 0x44404404,
				0x4f636c2f, 0x4b63682b, 0x4b53581b, 0xc3c3c003, 0x42626022,
				0x03333033, 0x85b1b435, 0x09212829, 0x80a0a020, 0xc2e2e022,
				0x87a3a427, 0xc3d3d013, 0x81919011, 0x01111011, 0x06020406,
				0x0c101c1c, 0x8cb0bc3c, 0x06323436, 0x4b43480b, 0xcfe3ec2f,
				0x88808808, 0x4c606c2c, 0x88a0a828, 0x07131417, 0xc4c0c404,
				0x06121416, 0xc4f0f434, 0xc2c2c002, 0x45414405, 0xc1e1e021,
				0xc6d2d416, 0x0f333c3f, 0x0d313c3d, 0x8e828c0e, 0x88909818,
				0x08202828, 0x4e424c0e, 0xc6f2f436, 0x0e323c3e, 0x85a1a425,
				0xc9f1f839, 0x0d010c0d, 0xcfd3dc1f, 0xc8d0d818, 0x0b23282b,
				0x46626426, 0x4a72783a, 0x07232427, 0x0f232c2f, 0xc1f1f031,
				0x42727032, 0x42424002, 0xc4d0d414, 0x41414001, 0xc0c0c000,
				0x43737033, 0x47636427, 0x8ca0ac2c, 0x8b83880b, 0xc7f3f437,
				0x8da1ac2d, 0x80808000, 0x0f131c1f, 0xcac2c80a, 0x0c202c2c,
				0x8aa2a82a, 0x04303434, 0xc2d2d012, 0x0b03080b, 0xcee2ec2e,
				0xc9e1e829, 0x4d515c1d, 0x84909414, 0x08101818, 0xc8f0f838,
				0x47535417, 0x8ea2ac2e, 0x08000808, 0xc5c1c405, 0x03131013,
				0xcdc1cc0d, 0x86828406, 0x89b1b839, 0xcff3fc3f, 0x4d717c3d,
				0xc1c1c001, 0x01313031, 0xc5f1f435, 0x8a82880a, 0x4a62682a,
				0x81b1b031, 0xc1d1d011, 0x00202020, 0xc7d3d417, 0x02020002,
				0x02222022, 0x04000404, 0x48606828, 0x41717031, 0x07030407,
				0xcbd3d81b, 0x8d919c1d, 0x89919819, 0x41616021, 0x8eb2bc3e,
				0xc6e2e426, 0x49515819, 0xcdd1dc1d, 0x41515011, 0x80909010,
				0xccd0dc1c, 0x8a92981a, 0x83a3a023, 0x8ba3a82b, 0xc0d0d010,
				0x81818001, 0x0f030c0f, 0x47434407, 0x0a12181a, 0xc3e3e023,
				0xcce0ec2c, 0x8d818c0d, 0x8fb3bc3f, 0x86929416, 0x4b73783b,
				0x4c505c1c, 0x82a2a022, 0x81a1a021, 0x43636023, 0x03232023,
				0x4d414c0d, 0xc8c0c808, 0x8e929c1e, 0x8c909c1c, 0x0a32383a,
				0x0c000c0c, 0x0e222c2e, 0x8ab2b83a, 0x4e626c2e, 0x8f939c1f,
				0x4a52581a, 0xc2f2f032, 0x82929012, 0xc3f3f033, 0x49414809,
				0x48707838, 0xccc0cc0c, 0x05111415, 0xcbf3f83b, 0x40707030,
				0x45717435, 0x4f737c3f, 0x05313435, 0x00101010, 0x03030003,
				0x44606424, 0x4d616c2d, 0xc6c2c406, 0x44707434, 0xc5d1d415,
				0x84b0b434, 0xcae2e82a, 0x09010809, 0x46727436, 0x09111819,
				0xcef2fc3e, 0x40404000, 0x02121012, 0xc0e0e020, 0x8db1bc3d,
				0x05010405, 0xcaf2f83a, 0x01010001, 0xc0f0f030, 0x0a22282a,
				0x4e525c1e, 0x89a1a829, 0x46525416, 0x43434003, 0x85818405,
				0x04101414, 0x89818809, 0x8b93981b, 0x80b0b030, 0xc5e1e425,
				0x48404808, 0x49717839, 0x87939417, 0xccf0fc3c, 0x0e121c1e,
				0x82828002, 0x01212021, 0x8c808c0c, 0x0b13181b, 0x4f535c1f,
				0x47737437, 0x44505414, 0x82b2b032, 0x0d111c1d, 0x05212425,
				0x4f434c0f, 0x00000000, 0x46424406, 0xcde1ec2d, 0x48505818,
				0x42525012, 0xcbe3e82b, 0x4e727c3e, 0xcad2d81a, 0xc9c1c809,
				0xcdf1fc3d, 0x00303030, 0x85919415, 0x45616425, 0x0c303c3c,
				0x86b2b436, 0xc4e0e424, 0x8bb3b83b, 0x4c707c3c, 0x0e020c0e,
				0x40505010, 0x09313839, 0x06222426, 0x02323032, 0x84808404,
				0x49616829, 0x83939013, 0x07333437, 0xc7e3e427, 0x04202424,
				0x84a0a424, 0xcbc3c80b, 0x43535013, 0x0a02080a, 0x87838407,
				0xc9d1d819, 0x4c404c0c, 0x83838003, 0x8f838c0f, 0xcec2cc0e,
				0x0b33383b, 0x4a42480a, 0x87b3b437 } };


static void SEED_Encrypt(void *CipherKey, BYTE *Data);
static void SEED_Decrypt(void *CipherKey, BYTE *Data);
static void SEED_SetAlgInfo(DWORD ModeID, DWORD PadType, BYTE *IV,
		SEED_ALG_INFO *AlgInfo);

static RET_VAL CBC_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt,
		DWORD *CipherTxtLen);
static RET_VAL PaddSet(BYTE *pbOutBuffer, DWORD dRmdLen, DWORD dBlockLen,
		DWORD dPaddingType);
static RET_VAL CBC_DecUpdate(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt,
		DWORD CipherTxtLen, BYTE *PlainTxt, DWORD *PlainTxtLen);

static RET_VAL SEED_EncUpdate(SEED_ALG_INFO *AlgInfo, BYTE *PlainTxt,
		DWORD PlainTxtLen, BYTE *CipherTxt, DWORD *CipherTxtLen);
static RET_VAL SEED_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt,
		DWORD *CipherTxtLen);

static RET_VAL SEED_KeySchedule(BYTE *UserKey, DWORD UserKeyLen,
		SEED_ALG_INFO *AlgInfo);

static int EncodeData(BYTE* lpDst, DWORD *DLen, BYTE* lpSrc, DWORD *SLen);
static int DecodeData(BYTE* lpDst, DWORD *DLen, BYTE* lpSrc, DWORD *SLen);

/* ******************************************************************************** */

#endif

// onvif_porting
const char	*_sysdb_cate_list[] = {
								"sys","net","audio","disk","cam","usr",
								"alarm","act","disp","rec",
#ifdef ENABLE_HNF_IPCAM
								"ipcam","iprec","ipact",
#endif
						        "onvif",
								 NULL };
// onvif_porting

/* Object signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	ARG_0,
	/* FILL ME */
};

static void nf_sysdb_class_init (NfSysDbClass * klass);
static void nf_sysdb_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_sysdb_set_property (GObject * object, guint prop_id,
const GValue * value, GParamSpec * pspec);
static void nf_sysdb_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_sysdb_dispose (GObject * object);
static void nf_sysdb_finalize (GObject * object);

#ifdef DEBUG_SYSDB_TEST
gboolean  nf_sysdb_test();
#endif

/* static data */
static GQuark	quark_property_value = 0;
static GQuark	quark_property_min = 0;
static GQuark	quark_property_max = 0;
static GQuark	quark_property_cb_validate = 0;

static GObjectClass *parent_class = NULL;
static NfSysDb	*_nf_sysdb = NULL;


typedef enum _SYSDB_LOAD_TYPE_E{
	SYSDB_LOAD_INIT         = (1<<0),
	SYSDB_LOAD_FIRST        = (1<<1),
	SYSDB_LOAD_IMPORT_UESR  = (1<<2),
	SYSDB_LOAD_IMPORT_FWUP  = (1<<3),
	SYSDB_LOAD_FACTORY      = (1<<4),

    SYSDB_LOAD_IMPORT_WEBRA = (1<<5),
    SYSDB_LOAD_FACTORY_WEBRA= (1<<6),

// onvif_porting
    SYSDB_LOAD_IMPORT_ONVIF = (1<<7),
    SYSDB_LOAD_FACTORY_ONVIF= (1<<8),
// onvif_porting

	SYSDB_LOAD_SKIP_ALL		= 0xffffffff
} SYSDB_LOAD_TYPE_E;

typedef struct _SYSDB_PARSE_PARAM_T {
	char				category[64];
	SYSDB_LOAD_TYPE_E	type;
} SYSDB_PARSE_PARAM;

typedef struct _SYSDB_SKIP_LIST_T {
	char	key[64];
	int		key_len;
	int		skip_mask;
} SYSDB_SKIP_LIST;

// _start_vlaue_element_handler FIXME!!
SYSDB_SKIP_LIST _sysdb_skip_list[] = {

	// for gui/webra
	{"sys.info.swver"              , 14 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.hwver"              , 14 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.mac"                , 12 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.ptz.P"              , 14 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.keyctrl.K"          , 18 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.sig_type"           , 17 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"disp.osd.lang"               , 13 , SYSDB_LOAD_FACTORY },		// JIAR : SWIPXLP-338 (eunhye)
	{"disp.monitor.dualmonitor"    , 24 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"disp.osd.cnt_lang"   	       , 17 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_IMPORT_FWUP },
	{"disp.osd.lang.L"    	       , 15 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_IMPORT_FWUP },
	{"disp.osd.lang.A"    	       , 15 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_IMPORT_FWUP },
	{"net.ddns.hostname"           , 17 , SYSDB_LOAD_IMPORT_UESR },
	{"ipcam."				       ,  6 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"sys.info.hubver1"            , 16 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.hubver2"            , 16 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.hubver3"            , 16 , SYSDB_LOAD_SKIP_ALL},
	{"cam.install.mode"            , 16 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"disk.erase_section"          , 18 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"sys.info.fwup_time"          , 18 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },	// choissi SWIPXVETHR-219
	{"sys.info.fwup_state"         , 19 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY },
	{"sys.sequrinet.factory_enabled", 29 , SYSDB_LOAD_SKIP_ALL},
	{"sys.info.guard"   	        , 14 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY},
	{"net.proto.dack_enable",         21 , SYSDB_LOAD_IMPORT_FWUP},
	{"net.proto.dack",                14 , SYSDB_LOAD_IMPORT_FWUP},
	{"sys.lic"                      ,  7 , SYSDB_LOAD_FACTORY|SYSDB_LOAD_IMPORT_UESR},
	{"cam.install.dual_lan"         , 20 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY},

	// for webra only
	{"disk."                       ,  5 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.dhcpon"            , 16 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.ipaddr"            , 16 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.subnet"            , 16 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.gateway"           , 17 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.dns1"              , 14 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.dns2"              , 14 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.webport"           , 17 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.proto.rtspport"          , 18 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"sys.info.netwizard_enable"   , 25 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY_WEBRA},
	{"sys.info.langwizard_func"	   , 24 , SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY_WEBRA},
	{"net.8021x."					, 10 , SYSDB_LOAD_IMPORT_WEBRA|SYSDB_LOAD_FACTORY_WEBRA},
	{"sys.init"        		, 8 ,  SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_FACTORY|SYSDB_LOAD_IMPORT_FWUP }
	
};

const int _sysdb_skip_cnt = sizeof(_sysdb_skip_list)/sizeof(SYSDB_SKIP_LIST);

/* simple xml-parser callback function */
static void
indent (int extra);

static void
_start_element_handler  (GMarkupParseContext *context,
                        const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values,
                        gpointer             user_data,
                        GError             **error);

static void
_start_value_element_handler  (GMarkupParseContext *context,
                        const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values,
                        gpointer             user_data,
                        GError             **error);

static void
_end_element_handler    (GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error);

static void
_text_handler                      (GMarkupParseContext *context,
                        const gchar         *text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error);
static void
_passthrough_handler    (GMarkupParseContext *context,
                        const gchar         *passthrough_text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error);

static void
_error_handler          (GMarkupParseContext *context,
                        GError              *error,
                        gpointer             user_data);

static int
_load_xml				(GMarkupParser *parser,
							const gchar *filename,
							const SYSDB_PARSE_PARAM *param );

/* sysdb xml data parser */
static const GMarkupParser def_parser = {
	_start_element_handler,
	_end_element_handler,
	NULL, //_text_handler,
	NULL, //_passthrough_handler,
	_error_handler
};


/* sysdb xml data parser */
static const GMarkupParser value_parser = {
	_start_value_element_handler,
	_end_element_handler,
	NULL, //_text_handler,
	NULL, //_passthrough_handler,
	_error_handler
};

static const GMarkupParser silent_parser = {
	NULL,
	NULL,
	NULL,
	NULL,
	_error_handler
};


GType
nf_sysdb_get_type (void)
{
	static GType nf_sysdb_type = 0;

	if (G_UNLIKELY (nf_sysdb_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfSysDbClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_sysdb_class_init,
			NULL,
			NULL,
			sizeof (NfSysDb),
			0,
			(GInstanceInitFunc) nf_sysdb_instance_init,
			NULL
		};

		nf_sysdb_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfSysDb", &object_info, 0);
	}
	return nf_sysdb_type;
}

static void
nf_sysdb_class_init (NfSysDbClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_sysdb_set_property;
	gobject_class->get_property = nf_sysdb_get_property;

	gobject_class->dispose = nf_sysdb_dispose;
	gobject_class->finalize = nf_sysdb_finalize;

	quark_property_value = g_quark_from_static_string ("property-value");
	quark_property_min = g_quark_from_static_string ("property-min");
	quark_property_max = g_quark_from_static_string ("property-max");
	quark_property_cb_validate = g_quark_from_static_string ("property-cb_validate");
}

static void
nf_sysdb_instance_init (GTypeInstance* instance, gpointer g_class)
{
	gint i;
	NfSysDb *self = NF_SYSDB (instance);

	self->spec_pool = g_param_spec_pool_new (FALSE);
	for(i=0; i<NF_SYSDB_CATE_NR; i++)
	{
		self->cate_lock[i] = g_mutex_new();
	}

}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_sysdb_dispose (GObject * object)
{
	// 1. GParamSpecPool ?? ???? ?ִ? ?ֵ? free ????????
	// 2. GParamSpecPool ??ü?? ��??
	// 3. cate_lock ��??~;;

	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_sysdb_finalize (GObject * object)
{
	parent_class->finalize (object);
}



static void
nf_sysdb_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
 	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_sysdb_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
  }
}


/*
 * sysdb db property
 */
void
nf_sysdb_db_install_property_value (NfSysDb   *self,
						GParamSpec			*pspec,
						GValue				*value)
{
	g_return_if_fail (NF_IS_SYSDB (self));
	g_return_if_fail (G_IS_PARAM_SPEC (pspec));
	g_return_if_fail (pspec->flags & G_PARAM_READABLE);
	g_return_if_fail (!(pspec->flags & (G_PARAM_CONSTRUCT_ONLY | G_PARAM_CONSTRUCT)));

	if (g_param_spec_pool_lookup (self->spec_pool, pspec->name, G_OBJECT_TYPE (self), FALSE))
	{
		g_warning (G_STRLOC ": class `%s' already contains a style property named `%s'",
		G_OBJECT_TYPE_NAME (self),
		pspec->name);

	 	g_param_spec_unref(pspec);

		return;
	}

	g_param_spec_ref_sink (pspec);
	g_param_spec_set_qdata (pspec, quark_property_value, (gpointer) value );
	g_param_spec_pool_insert (self->spec_pool, pspec, G_OBJECT_TYPE (self));

}

GParamSpec*
nf_sysdb_db_find (NfSysDb   *self,
				      const gchar    *property_name)
{
	g_return_val_if_fail (NF_IS_SYSDB (self), NULL);
	g_return_val_if_fail (property_name != NULL, NULL);

	return g_param_spec_pool_lookup (self->spec_pool,
				property_name,
				G_OBJECT_TYPE (self),
				TRUE);
}

GParamSpec**
nf_sysdb_db_list_property (NfSysDb   *self,
					guint          *n_properties)
{
	GParamSpec **pspecs;
	guint n;

	g_return_val_if_fail (NF_IS_SYSDB (self), NULL);

	pspecs = g_param_spec_pool_list (self->spec_pool,
				G_OBJECT_TYPE (self),
				&n);
	if (n_properties)
		*n_properties = n;

	return pspecs;
}

gboolean
nf_sysdb_db_get (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error)
{

	GParamSpec *pspec;
	gboolean ret = TRUE;

	g_return_val_if_fail (NF_IS_SYSDB (self), FALSE);
	g_return_val_if_fail (property_name != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_GET] )
		g_message("%s [%s]",__FUNCTION__, property_name);
#endif

	g_object_ref (self);
	pspec = g_param_spec_pool_lookup (self->spec_pool,
					property_name,
					G_OBJECT_TYPE (self),
					TRUE);
	if (!pspec)
	{
		g_warning ("%s: '%s' has no property named '%s'",
			G_STRLOC,
			G_OBJECT_TYPE_NAME (self),
			property_name);

		ret = FALSE;
	}
	else
	{
		const  GValue *peek_value = NULL;
		peek_value = g_param_spec_get_qdata (pspec, quark_property_value);

		g_assert ( peek_value != NULL );

		g_value_init(value, G_PARAM_SPEC_VALUE_TYPE(pspec));
		g_value_copy(peek_value, value);

#ifdef DEBUG_SYSDB_LOG
		if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_GET_VALUE] )
			g_message("%s [%s]",__FUNCTION__, property_name);
#endif

	}
	g_object_unref (self);

	return ret;
}


GValue *
nf_sysdb_db_get_gvalue (NfSysDb   *self,
						const gchar *property_name )
{
	GParamSpec *pspec;
	GValue *ret_val = NULL;

	g_return_val_if_fail (NF_IS_SYSDB (self), FALSE);
	g_return_val_if_fail (property_name != NULL, FALSE);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_GET] )
		g_message("%s [%s]",__FUNCTION__, property_name);
#endif

	g_object_ref (self);
	pspec = g_param_spec_pool_lookup (self->spec_pool,
					property_name,
					G_OBJECT_TYPE (self),
					TRUE);
	if (!pspec)
	{
		g_warning ("%s: '%s' has no property named '%s'",
			G_STRLOC,
			G_OBJECT_TYPE_NAME (self),
			property_name);
	}
	else
	{
		ret_val = g_param_spec_get_qdata (pspec, quark_property_value);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_GET_VALUE] )
		g_message("%s [%s]",__FUNCTION__, property_name);
#endif

		g_assert ( ret_val != NULL );
	}
	g_object_unref (self);

	return ret_val;
}


gboolean
nf_sysdb_db_set (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error)
{
	GParamSpec *pspec;
	gboolean ret = TRUE;

	g_return_val_if_fail ( NF_IS_SYSDB (self), FALSE);
	g_return_val_if_fail ( property_name, FALSE);
	g_return_val_if_fail ( G_IS_VALUE(value) , FALSE);

#ifdef DEBUG_SYSDB_SET_VALIDATE
	ret = nf_sysdb_validate_key0(property_name, value, NULL );
	if(!ret)
		return ret;
#endif

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_SET] )
		g_message("%s [%s]",__FUNCTION__, property_name);
#endif

	g_object_ref (self);
	pspec = g_param_spec_pool_lookup (self->spec_pool,
					property_name,
					G_OBJECT_TYPE (self),
					TRUE);
	if (!pspec)
	{
		g_warning ("%s: '%s' has no property named '%s'",
			G_STRLOC,
			G_OBJECT_TYPE_NAME (self),
			property_name);

		ret = FALSE;
	}
	else
	{
		GValue *peek_value = NULL;

		peek_value = g_param_spec_get_qdata (pspec, quark_property_value);
		g_assert ( peek_value != NULL );
/*
		1) Ÿ???? ?޶??? g_value_type_transformable ()??
		??ȯ ???ɼ? üũ ??  set ??????

		2) ?????ϱ? ???? üũ????
		readonly flag  AND  g_param_value_validate()

		readonly?? nf_sysdb_init???? ?ѹ? ?ʱ?ȭ ?ȴ?�� ???? ?????Ǵ? ????.
		?Ϻ? ?ʵ???�� min,max value?? ??��?Ƿ? üũ?? ??????
		??Ʈ???? ???�??ּұ???,?ִ????? üũ??��?? ????
*/
		if(G_VALUE_TYPE(peek_value) == G_VALUE_TYPE(value) )
		{
			g_value_copy(value, peek_value);
			/*
				g_value_copy ???ο? type???? copy ????�� ��???ؼ? ?????Ѵ?.
				string?? ????, dest?? ?Ҵ??? string�� free?ϰ? strdup��?? ??????
				???ο? ??��?? ??ü ?Ѵ?.

				1) nocopy_content ?ɼ?��?? ?��????? ???? ?? ??��
				2) ?????? ?ʿ? ?????? ????, ???? ??��?ϴ? ?߿? ?о�?? thread??
				��?? ?? ???? ?ִ?. ?�?��å�� ?Ӽ? get?? ?? ?????ؼ? ?ѱ??ϱ?
				Ȯ??�� ??��.
			*/

#ifdef DEBUG_SYSDB_LOG
			if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_SET_VALUE] )
				g_message("%s [%s]",__FUNCTION__, property_name);
#endif
		}
		else
		{
			g_warning ("%s: [%s][%s] type miss match %s,%s",
				G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
				G_VALUE_TYPE_NAME(peek_value), G_VALUE_TYPE_NAME(value) );

			ret = FALSE;
		}

	}
	g_object_unref (self);

	return ret;
}

gboolean
nf_sysdb_db_validate (NfSysDb   *self,
		      const gchar *property_name,
		      GValue *value,
		      GError **error)
{
	GParamSpec *pspec;
	gboolean ret = TRUE;

	g_return_val_if_fail ( NF_IS_SYSDB (self), FALSE);
	g_return_val_if_fail ( property_name, FALSE);
	g_return_val_if_fail ( G_IS_VALUE(value) , FALSE);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_VALIDATE] )
		g_message("%s [%s]",__FUNCTION__, property_name);
#endif

	g_object_ref (self);
	pspec = g_param_spec_pool_lookup (self->spec_pool,
					property_name,
					G_OBJECT_TYPE (self),
					TRUE);
	if (!pspec)
	{
		g_warning ("%s: '%s' has no property named '%s'",
			G_STRLOC,
			G_OBJECT_TYPE_NAME (self),
			property_name);

		ret = FALSE;
	}
	else
	{
		GValue *peek_value = NULL;
		peek_value = g_param_spec_get_qdata (pspec, quark_property_value);
		g_assert ( peek_value != NULL );

		if(G_VALUE_TYPE(peek_value) == G_VALUE_TYPE(value) )
		{
			switch ( G_VALUE_TYPE(peek_value) )
			{
				case G_TYPE_STRING:
				{
					guint param_min = 0, param_max = 0;
					guint value_strlen;

					param_min = (guint)g_param_spec_get_qdata (pspec, quark_property_min);
					param_max = (guint)g_param_spec_get_qdata (pspec, quark_property_max);

					if(param_min>0)
					{
						value_strlen = strnlen( g_value_get_string(value), param_min+1 );
						if( value_strlen < param_min )
						{
							g_warning ("%s: [%s][%s] strlen error MIN[%d] value_len[%d]",
								G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
								param_min, value_strlen );
							ret = FALSE;
						}
					}
					if(param_max>0)
					{
						value_strlen = strnlen( g_value_get_string(value), param_max+1 );
						if( value_strlen > param_max )
						{
							g_warning ("%s: [%s][%s] strlen error MAX[%d] value_len[%d]",
								G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
								param_max, value_strlen );
							ret = FALSE;
						}
					}
					break;
				}
				case G_TYPE_BOOLEAN:
				{
					gboolean tmp = g_value_get_boolean(value);
					if( tmp != 0 && tmp != 1 )
					{
						g_warning ("%s: [%s][%s] validate error value[%d]",
							G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name, tmp);
						ret = FALSE;
				 	}
					break;
				}
				case G_TYPE_UINT:
				{
					GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);
					guint tmp = g_value_get_uint(value);

					if( tmp < uspec->minimum || tmp > uspec->maximum )
					{
						g_warning ("%s: [%s][%s] validate error min[0x%x] max[0x%x] value[0x%x]",
							G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
							uspec->minimum, uspec->maximum, tmp);
						ret = FALSE;
					}
					break;
				}
				case G_TYPE_INT:
				{
					GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);
					gint tmp = g_value_get_int(value);

					if( tmp < ispec->minimum || tmp > ispec->maximum )
					{
						g_warning ("%s: [%s][%s] validate error min[%d] max[%d] value[%d]",
							G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
							ispec->minimum, ispec->maximum, tmp);
						ret = FALSE;
					}
					break;
				}
				default :
					g_warning ("%s: [%s][%s] validate error unknown type",
							G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name );

					ret = FALSE;
					break;
			}
		}
		else
		{
			g_warning ("%s: [%s][%s] type miss match %s,%s",
				G_STRLOC, G_OBJECT_TYPE_NAME (self), property_name,
				G_VALUE_TYPE_NAME(peek_value), G_VALUE_TYPE_NAME(value) );

			ret = FALSE;
		}

	}
	g_object_unref (self);

	return ret;
}

// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
void	check_sysid_hostname()
{
	char hostname[256], sysid[256];
	char buff[256];

	memset(hostname, 0x00, sizeof(char)*256);
	memset(sysid, 0x00, sizeof(char)*256);
	memset(buff, 0x00, sizeof(char)*256);

	sprintf(hostname, "%s", nf_sysdb_get_str_nocopy("net.ddns.hostname"));
	sprintf(sysid, "%s", nf_sysdb_get_str_nocopy("sys.info.sysid"));

	if( strncmp(hostname, " ", sizeof(char)*256-1) == 0 ){
		sprintf(buff, "%s-%s", nf_sysdb_get_str_nocopy("sys.info.model"), nf_sysman_get_serial_num());
		nf_sysdb_set_str("net.ddns.hostname", buff);
		nf_sysdb_save("net");
	}

	if( strncmp(sysid, " ", sizeof(char)*256-1) == 0 ){
		sprintf(buff, "%s-%s", nf_sysdb_get_str_nocopy("sys.info.model"), nf_sysman_get_serial_num());
		nf_sysdb_set_str("sys.info.sysid", buff);
		nf_sysdb_save("sys");
	}
}

void	check_hostname()
{
	FILE *fp = NULL;
	char buff[256], hostname[256];
	int change_hostname = 0;

	memset(buff, 0x00, sizeof(char)*256);
	memset(hostname, 0x00, sizeof(char)*256);

	check_sysid_hostname();

	fp = fopen("/etc/hostname", "r");
	if(!fp){
		printf("fopen fail!!\n");
		return;
	}

	fscanf(fp, "%s", buff);
	sprintf(hostname, "%s", nf_sysdb_get_str_nocopy("net.ddns.hostname"));
	fclose(fp);

	if( strncmp(buff, hostname, sizeof(char)*256-1) != 0 ){
		change_hostname = 1;
	}

	if( change_hostname ){
		fp = fopen("/etc/hostname", "w");
		if(!fp){
			printf("fopen fail!!\n");
			return;
		}

		fprintf(fp, "%s", hostname);
		fclose(fp);
		fflush(fp);
	}

	proxy_system("hostname -F /etc/hostname", 1, 3);

}


void nf_sysinfo_init(void)
{

#if defined(_NCX301)
	nf_sysdb_set_str("sys.info.model", "NCX-2003P");
#elif defined(_NVS2G16)
	nf_sysdb_set_str("sys.info.model", "NET5516");
#elif defined(_NVS2G08)
	nf_sysdb_set_str("sys.info.model", "NET5508");
#elif defined(_NVS2G04)
	nf_sysdb_set_str("sys.info.model", "NET5504");
#elif defined(_NVS2G01)
	nf_sysdb_set_str("sys.info.model", "NET5501");
#elif defined(_NVS2G01H)
	nf_sysdb_set_str("sys.info.model", "NET5501-XT");
#elif defined(_NVS2G01I)
	nf_sysdb_set_str("sys.info.model", "NET5501-I");
#elif defined(_IPX_1648P3ECO) || defined(_IPX_0824P3ECO)
	nf_sysdb_set_str("sys.info.model", "IPX-P3ECO");
#elif defined(_IPX_1648VE3) || defined(_IPX_0824VE3) || defined(_IPX_0412VE3)
	nf_sysdb_set_str("sys.info.model", "IPX-VE3");
#elif defined(_IPX_0824P3) || defined(_IPX_1648P3)
	nf_sysdb_set_str("sys.info.model", "IPX-P3");
#elif defined(_IPX_0824P4) || defined(_IPX_1648P4) 
	nf_sysdb_set_str("sys.info.model", "IPX-P4");
#elif defined(_IPX_0412L4) || defined(_IPX_0824L4) || defined(_IPX_1648L4) || defined(_IPX_0412M4) || defined(_IPX_0824M4) || \
	  defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
	nf_sysdb_set_str("sys.info.model", "IPX-L4");
#elif defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
	nf_sysdb_set_str("sys.info.model", "IPX-P4E");
#elif defined(_IPX_32M4E) || defined(_IPX_32P5)
	nf_sysdb_set_str("sys.info.model", "IPX-M4E");
//_IPX_32P5
#else
	nf_sysdb_set_str("sys.info.model", "NULL");
#endif

	check_hostname();
}
#endif
// onvif_porting

gboolean
nf_sysdb_remove_private(void)
{
	proxy_system("rm -f /NFDVR/data/nf_sysdb_private.conf",1,3);
}

/******************************************************************************/

/**
	@brief				sysdb ?ʱ?ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_init(int wait, int load_data, char *mtd_path)
{
	gboolean 	ret = TRUE;
	g_return_val_if_fail (_nf_sysdb == NULL, FALSE);
	gint i = 0;
	SYSDB_PARSE_PARAM	param;

	// 1. NfSysDb ?ν??Ͻ??? ??????


	// 2. NF_SYSDB_CONF_FILENAME ?ε? ( ???????? )
	// 3. nf_sysdb_*.conf ?ε? (??�� ?????? ????)

	g_message("%s wait[%d] load_data[%d]",__FUNCTION__, wait, load_data);

	_nf_sysdb = g_object_new ( NF_TYPE_SYSDB , NULL);

	nf_debug_category_add( "sysdb", _DEBUG_SYSDB_str, _DEBUG_SYSDB_log, DEBUG_SYSDB_IDX_NR);

	memset(&param, 0x00, sizeof(SYSDB_PARSE_PARAM));
	param.type = SYSDB_LOAD_INIT;

	ret = _load_xml( &def_parser, NF_SYSDB_CONF_VER_FILENAME, &param); // for version
	_load_xml( &def_parser, NF_SYSDB_CONF_PRIVATE_FILENAME, &param); // for private db
	ret = _load_xml( &def_parser, NF_SYSDB_CONF_FILENAME, &param);
	g_return_val_if_fail ( ret , FALSE);

	// nf_sysdb_save_all();
#ifndef DEBUG_SYSDB_ALWAYS_DEFAULT
	/* ??�� ??�� ?о? ?´?. */
	if(load_data == NF_SYSDB_INIT_LOAD_DATA )
	{
		i = 0;
		while( _sysdb_cate_list[i] )
		{
			nf_sysdb_load(_sysdb_cate_list[i]);
			i++;
		}
		g_message("%s load_data complete!!", __FUNCTION__);

	}else if(load_data == NF_SYSDB_INIT_FW_UPGRADE ){

		g_message("%s load_data start path[%s]", __FUNCTION__, mtd_path);
		nf_sysdb_import_fwup( mtd_path );
		g_message("%s load_data complete!!", __FUNCTION__);

	}

#endif
	/*
		1) ?ϵ????�?Ư�� ???Ϸ? ???? ??��?? ?о? ?;? ?ϴ? ?κе? ??�� ????
		2) readonly flag ???? ?׸? ?ʿ?
	*/

	//for test
	//nf_sysdb_save_all();

#ifdef	DEBUG_SYSDB_TEST
	nf_sysdb_test();
#endif

	_test_lic();

	return ret;
}


gboolean
_nf_sysdb_load_internal( const gchar *category, const gchar *filename, gint type )
{
	gboolean ret = FALSE;
	SYSDB_PARSE_PARAM	param;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_message("%s cate[%s] file[%s] type[%d]", __FUNCTION__, category, filename, type);

	strncpy(param.category, category, sizeof(param.category)-1 );
	param.type = type;

	ret = _load_xml( &value_parser, filename, &param);

	return ret;
}


/**
	@brief				sysdb ?ش? ī?װ���?? ??�� load ?Ѵ?.
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_load( const gchar *category )
{
	gchar buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_snprintf( buff, sizeof(buff), NF_SYSDB_CONF_PATH_STR, category );

	return _nf_sysdb_load_internal( category, buff, SYSDB_LOAD_FIRST );
}


/**
	@brief				sysdb ?ش? ī?װ���?? ????�� default?? ?ǵ?????.
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_default( const gchar *category )
{
	gboolean ret = FALSE;
	SYSDB_PARSE_PARAM	param;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_message("%s called [%s]", __FUNCTION__, category);
	// SYSDB_LOAD_FACTORY

	strncpy(param.category, category, sizeof(param.category)-1 );
	param.type = SYSDB_LOAD_FACTORY;

	ret = _load_xml( &value_parser, NF_SYSDB_CONF_FILENAME, &param );
	_load_xml( &value_parser, NF_SYSDB_CONF_PRIVATE_FILENAME, &param );

	return ret;
}

// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
/**
  @brief        sysdb ?ش? ī?װ���?? ????�� default?? ?ǵ????? ( onvif???? )
  @return       gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_default_onvif( const gchar *category )
{
	gboolean ret = FALSE;
	SYSDB_PARSE_PARAM     param;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_message("%s called [%s]", __FUNCTION__, category);
	// SYSDB_LOAD_FACTORY

	strncpy(param.category, category, sizeof(param.category)-1 );
	param.type = SYSDB_LOAD_FACTORY|SYSDB_LOAD_FACTORY_ONVIF;

	ret = _load_xml( &value_parser, NF_SYSDB_CONF_FILENAME, &param );

	//nf_sysdb_update_ntsc_pal_type();

	/* set system info */
	nf_sysinfo_init();

	return ret;
}
#endif
// onvif_porting

/**
  @brief        sysdb ?ش? ī?װ���?? ????�� default?? ?ǵ????? ( network ��?? )
  @return       gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_default_without_network( const gchar *category )
{
  gboolean ret = FALSE;
  SYSDB_PARSE_PARAM     param;

  g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
  g_return_val_if_fail (category != NULL, FALSE);

  g_message("%s called [%s]", __FUNCTION__, category);
  // SYSDB_LOAD_FACTORY

  strncpy(param.category, category, sizeof(param.category)-1 );
  param.type = SYSDB_LOAD_FACTORY|SYSDB_LOAD_FACTORY_WEBRA;

  ret = _load_xml( &value_parser, NF_SYSDB_CONF_FILENAME, &param );

  return ret;
}

static gint
_pspec_compare_func ( gconstpointer a, gconstpointer b)
{
	// ?????????Ͷ?~;;
	return strcmp( g_param_spec_get_nick( *(GParamSpec **)a ),
					g_param_spec_get_nick( *(GParamSpec **)b )  );
}

#ifdef ENABLE_SYSDB_CONVERT_TABLE

gboolean
_nf_sysdb_save_internal( const gchar *category, const gchar *filename, gint force_flush)
{
	FILE 		*f = NULL;
	int 		i,j, saved_count=0;
	int 		cate_len = 0;

	SYSDB_CONVERT_TABLE *cur_table = NULL;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_message("%s cate[%s] file[%s]", __FUNCTION__, category, filename);

	// file open
	f = fopen( filename,"w");

	g_return_val_if_fail (f != NULL, FALSE);

	cate_len = strlen(category);

	// sysdb xml body
	fprintf( f, "<nf_sysdb>\n<sys>\n");

	for(j=0; j<NF_SYSDB_CONVERT_TYPE_NR; j++){

		i = 0;
		while(1) {

			cur_table = &(_sysdb_convert_table[j][i++]);

			if(strcmp(cur_table->prop_name , "NULL") == 0)
				break;

			if( cate_len == 0 ||
					g_ascii_strncasecmp( cur_table->prop_name ,
								category, cate_len ) == 0 )
			{
				GParamSpec *pspec = NULL;
				GValue *tmp_value = NULL;
	 			gchar *markup = NULL;
				GValue tmp_value_str ;

				pspec = g_param_spec_pool_lookup ( _nf_sysdb->spec_pool,
													cur_table->prop_name,
													G_OBJECT_TYPE (_nf_sysdb), 0);

				if (!pspec)
				{
					g_warning ("%s: '%s' has no property named '%s'",
						G_STRLOC,
						G_OBJECT_TYPE_NAME (_nf_sysdb),
						cur_table->prop_name);

					continue;
				}

	 			memset (&tmp_value_str, 0, sizeof (GValue));
				tmp_value = g_param_spec_get_qdata (pspec, quark_property_value);
				g_value_init( &tmp_value_str, G_TYPE_STRING);
				g_value_transform( 	tmp_value, &tmp_value_str);

				markup =  g_markup_printf_escaped( "<item key=\"%s\" type=\"%s\" val=\"%s\" />\n",
								g_param_spec_get_nick(pspec),
								g_param_spec_get_blurb(pspec),
								g_value_get_string(&tmp_value_str) );

				g_assert(markup);
				fputs( markup, f); //fprintf(f, markup);
				g_free(markup);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_SAVE] )
			g_message( "%05d [%-16s][%-10s] name[%-32s] [%s]" , i,
							G_PARAM_SPEC_TYPE_NAME(pspec),
							g_param_spec_get_blurb(pspec),
							g_param_spec_get_nick(pspec),
							g_value_get_string(&tmp_value_str) );
#endif

				g_value_unset( &tmp_value_str );
				++saved_count;
			}
		}

	}

	/*
		?ΰ?��???? ?ʿ?
		??¥, CRC;
	*/
	fprintf( f, "</sys>\n</nf_sysdb>\n");
	fclose(f);

{
	struct stat f_stat;
    memset(&f_stat, 0x00, sizeof(struct stat  ) );
	stat(filename, &f_stat);

	g_message("%s cate[%s] file[%s] done size[%ld]", __FUNCTION__, category, filename,
					f_stat.st_size);
}

	if( force_flush )
	{
		nf_sysdb_save_flush();
	}

	return 1;
}

#else  // use nf_sysdb_db_list_property and qsort

gboolean
_nf_sysdb_save_internal( const gchar *category, const gchar *filename, gint force_flush)
{
	FILE 		*f = NULL;
	GParamSpec	**pspecs = NULL;
	int i, n_pspecs = 0, saved_count=0;
	int cate_len = 0;

	int str_len;

	gchar item[32*1024];
	int item_size = 0;

	item_size = sizeof(item);

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_message("%s cate[%s] file[%s]", __FUNCTION__, category, filename);

	// file open
	f = fopen( filename,"w");

	g_return_val_if_fail (f != NULL, FALSE);

	pspecs = nf_sysdb_db_list_property( _nf_sysdb, &n_pspecs);

	if(n_pspecs == 0){
		g_message("%s n_pspecs[%d]", __FUNCTION__, n_pspecs );

		fprintf( f, "<nf_sysdb>\n<sys>\n");
		fprintf( f, "</sys>\n</nf_sysdb>\n");

		fclose(f);
		return 1;
	}
/*
	hash table?ȿ? ???? ?ִ? ???̶? key ???? ?????? ?ȵǾ? ?ִ?.
	?????? ???? ???ҷ��? sort?? ?ʿ???
*/
 	qsort( pspecs, n_pspecs, sizeof(GParamSpec *), _pspec_compare_func);
/*
	output ??)

	<nf_sysdb><sys>
	<item key="sys.test.uint"				type="UINT" 	val="123" />
	<item key="sys.test.int"				type="INT"	 	val="-2" />
	<item key="sys.test.bool"				type="BOOL" 	val="1" />
	</sys></nf_sysdb>
*/

	cate_len = strlen(category);

	// sysdb xml body
	fprintf( f, "<nf_sysdb>\n<sys>\n");

	// file open ( nf_sysdb_sys.conf.tmp )
	// ?ӽ????Ͽ??? tmp ????ǥ ????
	for(i=0; i<n_pspecs; i++)
	{
		if( cate_len == 0 ||
				g_ascii_strncasecmp( g_param_spec_get_nick(pspecs[i]),
								category, cate_len ) == 0 )
		{

			GValue *tmp_value;
			GValue tmp_value_str;
 			memset (&tmp_value_str, 0, sizeof (GValue));

			gchar *type = NULL;
			gchar *ori_val = NULL;
			gchar *escaped_val = NULL;

			tmp_value = g_param_spec_get_qdata (pspecs[i], quark_property_value);
			g_value_init( &tmp_value_str, G_TYPE_STRING);
			g_value_transform( 	tmp_value, &tmp_value_str);

			type = g_param_spec_get_blurb(pspecs[i]);
			ori_val = g_value_get_string(&tmp_value_str);

			if(!strcmp( type, "STRING"))
			{
				escaped_val = g_markup_escape_text( ori_val, strlen(ori_val));
				g_assert(escaped_val);

				str_len = snprintf(item, item_size, "<item key=\"%s\" type=\"%s\" val=\"%s\" />\n", g_param_spec_get_nick(pspecs[i]), type, escaped_val);
				if( str_len >= item_size)
					g_error("\n[%s] item[%s] overflow !!!!!\n", __FUNCTION__, g_param_spec_get_nick(pspecs[i]));

				g_free(escaped_val);
			}
			else
			{
				str_len = snprintf(item, item_size, "<item key=\"%s\" type=\"%s\" val=\"%s\" />\n", g_param_spec_get_nick(pspecs[i]), type, ori_val);
			}

			fputs( item, f);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_SAVE] )
			g_message( "%05d [%-16s][%-10s] name[%-32s] [%s]" , i,
							G_PARAM_SPEC_TYPE_NAME(pspecs[i]),
							g_param_spec_get_blurb(pspecs[i]),
							g_param_spec_get_nick(pspecs[i]),
							g_value_get_string(&tmp_value_str) );
#endif
			g_value_unset( &tmp_value_str );
//			++saved_count;
		}
	}
	/*
		?ΰ?��???? ?ʿ?
		??¥, CRC;
	*/
	fprintf( f, "</sys>\n</nf_sysdb>\n");
	fclose(f);
	g_free(pspecs);

	if( force_flush )
	{
		nf_sysdb_save_flush();
	}

	return 1;
}
#endif

/**
	@brief				sysdb ?ش? ī?װ���?? ??�� save ?Ѵ?.
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_save( const gchar *category )
{
	char		buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	g_snprintf( buff, sizeof(buff), NF_SYSDB_CONF_PATH_STR , category );

	return _nf_sysdb_save_internal( category, buff, 1 );
}

/**
	@brief				sysdb category lock
	@return	void
*/
void
nf_sysdb_lock(NF_SYSDB_CATE_E cate)
{
	g_return_if_fail ( _nf_sysdb != NULL);
	g_return_if_fail ( cate < NF_SYSDB_CATE_NR  );

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_LOCK] )
		g_message("%s [%d]",__FUNCTION__, cate);
#endif

	g_mutex_lock( _nf_sysdb->cate_lock[cate] );

	return;
}

/**
	@brief				sysdb category trylock
	@return	void
*/
gboolean
nf_sysdb_trylock(NF_SYSDB_CATE_E cate)
{
	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( cate < NF_SYSDB_CATE_NR, FALSE );

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_LOCK] )
		g_message("%s [%d]",__FUNCTION__, cate);
#endif

	return g_mutex_trylock( _nf_sysdb->cate_lock[cate] );
}

/**
	@brief				sysdb category unlock
	@return	void
*/
void
nf_sysdb_unlock(NF_SYSDB_CATE_E cate)
{
	g_return_if_fail ( _nf_sysdb != NULL );
	g_return_if_fail ( cate < NF_SYSDB_CATE_NR  );

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_UNLCOK] )
		g_message("%s [%d]",__FUNCTION__, cate);
#endif

	g_mutex_unlock( _nf_sysdb->cate_lock[cate] );
	return;
}


/**
	@brief				sysdb save flush
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
void nf_sysdb_save_flush()
{
	sync();
	proxy_system("/bin/sync",1,3);
}

/**
	@brief				sysdb save all
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
void nf_sysdb_save_all()
{
	char		buff[256];
	int i=0;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);

	while( _sysdb_cate_list[i] )
	{
		g_snprintf( buff, sizeof(buff), NF_SYSDB_CONF_PATH_STR, _sysdb_cate_list[i] );
		_nf_sysdb_save_internal(  _sysdb_cate_list[i], buff, 0 );

		i++;
	}

	nf_sysdb_save_flush();

	return;
}


#define SYSDB_FILENAME_MAX	256
#define SYSDB_BUF_MAX		1024

static int _gz_compress(char *in_name, char *out_name)
{
    char destFile[SYSDB_FILENAME_MAX], buf[SYSDB_BUF_MAX];
    gzFile pDestFile;
    FILE* pOriFile;
    int curRead = 0;
    
    sprintf(destFile, "%s", out_name);
 
    if ((pOriFile = fopen(in_name, "rb")) < 0)
    {
        printf("original file open error\n");
        return -1;    
    }
 
    if ((pDestFile = gzopen(destFile, "wb")) == NULL)
    {
        printf("dest file open error\n");

		if(pOriFile)
			fclose(pOriFile);

		return -1;
    }
  
    while((curRead = fread(buf, sizeof(char), SYSDB_BUF_MAX, pOriFile)) > 0)
    {
        if (gzwrite(pDestFile, buf, curRead) < 0)
        {
            printf("error in the while loop!! \n");

		    gzclose(pDestFile);
		    fclose(pOriFile);

			return -1;
        }
    }
 
    gzclose(pDestFile);
    fclose(pOriFile);
 
    printf("%s - IN[%s] => OUT[%s]\n", __FUNCTION__, in_name, out_name);

	return 0;
}
 
static int _gz_uncompress(char *in_name, char *out_name)
{
    char destFile[SYSDB_FILENAME_MAX], buf[SYSDB_BUF_MAX];
    gzFile pOriFile;
    FILE* pDestFile;
    int curRead=0;
   
    if((pDestFile = fopen(out_name, "wb")) == NULL ){
        printf("dest file open Error! \n");
		return -1;
    }
 
    if ((pOriFile = gzopen(in_name, "rb")) == NULL){
        printf("original file open Error! \n");

		if(pDestFile)
			fclose(pDestFile);		
		
		return -1;
    }
    
    while( curRead = gzread(pOriFile, buf, SYSDB_BUF_MAX) )
    {
        if ( fwrite(buf , sizeof(char), curRead, pDestFile) < 0 ){

            printf("error in the while loop!! \n");
 
            gzclose(pOriFile);
            fclose(pDestFile);
 
			return -1;
        }
    }
    
    gzclose(pOriFile);
    fclose(pDestFile);

    printf("%s - IN[%s] => OUT[%s]\n", __FUNCTION__, in_name, out_name);

	return 0;
}

#define DB_IMPORT_COMPRESSED	"/tmp/db_import_compressed"
#define DB_IMPORT_TEXT			"/tmp/db_import_text"

static gboolean _nf_sysdb_import_encrypted_data(gchar *filename, gint type)
{
	gchar *contents;
	gsize  length;
	GError *error;

	int encryption_len;
	
	gchar *decrypt_buff = NULL;
	int decrypt_size;

	int ret;
	gboolean tmp_ret;

	error = NULL;
	if (!g_file_get_contents (filename, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);
		g_free(contents);
		return FALSE;
	}

	if( contents[0] == 'I' && contents[1] == 'T' && contents[2] == 'X' && contents[3] == 'V' && contents[4] == 1 )
	{
		FILE *decrypt_fp = NULL;	
		
		printf("%s - encrypted sysdb\n", __FUNCTION__);

		encryption_len = length - 6;

		decrypt_size = encryption_len + 64;
		decrypt_buff = g_malloc(decrypt_size);

		ret = DecodeData(decrypt_buff, &decrypt_size, &contents[6], &encryption_len);

		if( ret != CTR_SUCCESS )
		{
			g_message("%s DecodeData ERR => %d", __FUNCTION__, ret);

			g_free(contents);
			g_free(decrypt_buff);

			return FALSE;
		}	

		g_free(contents);

		decrypt_fp = fopen(DB_IMPORT_COMPRESSED, "w");

		if( fwrite(decrypt_buff, decrypt_size, 1, decrypt_fp) != 1 )
		{
			g_message("%s DecBuff flush fail", __FUNCTION__);

			fclose(decrypt_fp);
			g_free(decrypt_buff);
			
			return FALSE;
		}

		fclose(decrypt_fp);
		g_free(decrypt_buff);

		ret = _gz_uncompress(DB_IMPORT_COMPRESSED, DB_IMPORT_TEXT);

		if (access(DB_IMPORT_COMPRESSED, F_OK) == 0)
			remove(DB_IMPORT_COMPRESSED);

		if( ret < 0 )
		{			
			if (access(DB_IMPORT_TEXT, F_OK) == 0)
				remove(DB_IMPORT_TEXT);
			
			return FALSE;
		}

		tmp_ret = _nf_sysdb_load_internal( "", DB_IMPORT_TEXT, type );
		remove(DB_IMPORT_TEXT);

		return tmp_ret;
	}
	else
	{
		g_free(contents);

		printf("%s - normal sysdb\n", __FUNCTION__);

		return _nf_sysdb_load_internal( "", filename, type );	
	}
}

#define DB_EXPORT_TEXT			"/tmp/db_export_text"
#define DB_EXPORT_COMPRESSED	"/tmp/db_export_compressed"

static gboolean _convert_to_encrypt_format(gchar *in_name, gchar *out_name)
{
	int ret = 0;

	gchar *contents;
	gsize  length;
	GError *error;

	int enc_len;
	gchar *enc_buff;

	gchar enc_header[6] = {'I','T','X','V',1,0};
	FILE *sysdb_fp;

	ret = _gz_compress(in_name, DB_EXPORT_COMPRESSED);

	if (access(in_name, F_OK) == 0)
		remove(in_name);

	if( ret < 0 )
	{	
		if (access(DB_EXPORT_COMPRESSED, F_OK) == 0)	
			remove(DB_EXPORT_COMPRESSED);
		
		return FALSE;
	}

	error = NULL;
	if (!g_file_get_contents (DB_EXPORT_COMPRESSED, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);
		g_free(contents);
		return FALSE;
	}

	remove(DB_EXPORT_COMPRESSED);

	enc_len = length + 64;
	enc_buff = g_malloc(enc_len);

	ret = EncodeData(enc_buff, &enc_len, contents, &length);
	if( ret != CTR_SUCCESS )
	{
		g_message("%s EncodeData ERR => %d", __FUNCTION__, ret);

		g_free(contents);
		g_free(enc_buff);

		return FALSE;
	}

	g_free(contents);

	sysdb_fp = fopen(out_name, "w");

	if( sysdb_fp == NULL )
	{
		g_free(enc_buff);
		return FALSE;
	}

	if( fwrite(enc_header, 6, 1, sysdb_fp) != 1 )
	{
		g_message("%s EncHeader add fail", __FUNCTION__);

		fclose(sysdb_fp);
		g_free(enc_buff);

		return FALSE;
	}

	if( fwrite(enc_buff, enc_len, 1, sysdb_fp) != 1 )
	{
		g_message("%s EncData add fail", __FUNCTION__);	
		
		fclose(sysdb_fp);
		g_free(enc_buff);

		return FALSE;
	}

	fclose(sysdb_fp);
	g_free(enc_buff);

	return TRUE;
}

/**
	@brief				sysdb ??����?? ??��?��?
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_import_fwup( const gchar *path )
{
	int i = 0;
	char buff[256];
	char pre_private[100];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);

	while( _sysdb_cate_list[i] )
	{
		g_snprintf( buff, sizeof(buff), NF_SYSDB_CONF_IMPORT_PATH_STR, path, _sysdb_cate_list[i] );
		_nf_sysdb_load_internal( "", buff, SYSDB_LOAD_IMPORT_FWUP );
		i++;
	}

	if(nf_sysdb_get_bool("sys.init.func"))
	{
		g_snprintf( pre_private, sizeof(pre_private), NF_SYSDB_CONF_IMPORT_PATH_STR, path, "private");
		g_snprintf( buff, sizeof(buff), "cp -f %s %s", pre_private, NF_SYSDB_CONF_PRIVATE_FILENAME);
		proxy_system(buff,1,3);

		if(g_access(NF_SYSDB_CONF_PRIVATE_FILENAME, 0) == 0)
			nf_sysdb_set_bool("sys.init.run", FALSE);
		else
			nf_sysdb_set_bool("sys.init.run", TRUE);
	}
	else
		nf_sysdb_set_bool("sys.init.run", FALSE);

	g_message("%s load_data complete!!", __FUNCTION__);

	//nf_sysdb_save_all();

	return 1;
}


/**
	@brief				sysdb ??����?? ??��?��?
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_import( const gchar *filename )
{
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	return _nf_sysdb_import_encrypted_data(filename, SYSDB_LOAD_IMPORT_UESR);
}

static char sysdb_import_help[] = "sysdb_import";
static int sysdb_import(int argc, char **argv)
{
	gboolean ret;

	ret = nf_sysdb_import("/NFDVR/system_data.ndb");

	g_message("%s - RET:%d", __FUNCTION__, ret);	

	return 0;
}
__commandlist(sysdb_import, "sysdb_import", sysdb_import_help, sysdb_import_help);

/**
  @brief                                webra????sysdb ?????�?가??????
  @return       gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_import_webra( const gchar *filename )
{
  g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

	return _nf_sysdb_import_encrypted_data(filename, (SYSDB_LOAD_IMPORT_UESR|SYSDB_LOAD_IMPORT_WEBRA) ); 
}

/**
	@brief				sysdb ??����?? ????????
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_sysdb_export( const gchar *filename )
{
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	gboolean ret;
	int tmp_ret;

	if( _nf_sysdb_save_internal( "", DB_EXPORT_TEXT, 0) == FALSE )
		return FALSE;

	if( _convert_to_encrypt_format(DB_EXPORT_TEXT, filename) == FALSE )
		return FALSE;

	nf_sysdb_save_flush();	

	return TRUE;
}

static char sysdb_export_help[] = "sysdb_export";
static int sysdb_export(int argc, char **argv)
{
	gboolean ret;

	ret = nf_sysdb_export("/NFDVR/system_data.ndb");

	g_message("%s - RET:%d", __FUNCTION__, ret);	

	return 0;
}
__commandlist(sysdb_export, "sysdb_export", sysdb_export_help, sysdb_export_help);

/**
	@brief				sysdb ??����?? ??��?��? ?? ?׽?Ʈ ?غ???
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_import_test( const gchar *filename )
{
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	return TRUE;
}


/**
	@brief				sysdb ?ش? ī?װ���?? ??�� dump ?Ѵ?.
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_dump( const gchar *category )
{

	GParamSpec	**pspecs = NULL;
	int i, n_pspecs = 0;
	int cate_len = 0;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (category != NULL, FALSE);

	cate_len = strlen(category);

	pspecs = nf_sysdb_db_list_property( _nf_sysdb, &n_pspecs);

	if(n_pspecs == 0) return 1;

 	qsort( pspecs, n_pspecs, sizeof(GParamSpec *), _pspec_compare_func);

	for(i=0; i<n_pspecs; i++)
	{
		if( g_ascii_strncasecmp( g_param_spec_get_nick(pspecs[i]),
								category, cate_len ) == 0 )
		{
			GValue *tmp_value;
			GValue tmp_value_str;
 			memset (&tmp_value_str, 0, sizeof (GValue));

			tmp_value = g_param_spec_get_qdata (pspecs[i], quark_property_value);
			g_value_init( &tmp_value_str, G_TYPE_STRING);
			g_value_transform( 	tmp_value, &tmp_value_str);

			g_message( "%05d [%-16s][%-10s] name[%-32s] [%s]" , i,
							G_PARAM_SPEC_TYPE_NAME(pspecs[i]),
							g_param_spec_get_blurb(pspecs[i]),
							g_param_spec_get_nick(pspecs[i]),
							g_value_get_string(&tmp_value_str) );

			g_value_unset( &tmp_value_str );
		}
	}

	g_free(pspecs);
	return 1;
}

/**
	@brief				sysdb ???ϵ? ?׸?�� ???? ȭ?? dump
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_dump_all()
{
	int n,i;
	GParamSpec **pspecs;

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);

	pspecs = nf_sysdb_db_list_property( _nf_sysdb, &n);

	if(n == 0) return 1;
 	qsort( pspecs, n, sizeof(GParamSpec *), _pspec_compare_func);

	for(i=0;i<n;i++)
	{
		GValue *tmp_value;
		GValue tmp_value_str;
		memset (&tmp_value_str, 0, sizeof (GValue));

		tmp_value = g_param_spec_get_qdata (pspecs[i], quark_property_value);
		g_value_init( &tmp_value_str, G_TYPE_STRING);

		g_value_transform( 	tmp_value, &tmp_value_str);

		g_message( "%05d [%-16s][%-10s] name[%-32s] [%s]" , i,
						G_PARAM_SPEC_TYPE_NAME(pspecs[i]),
						g_param_spec_get_blurb(pspecs[i]),
						g_param_spec_get_nick(pspecs[i]),
						g_value_get_string(&tmp_value_str) );


		g_value_unset( &tmp_value_str );
	}
	g_free(pspecs);

	return 1;
}

/**
	@brief				sysdb ???? ?׽?Ʈ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_test()
{
	guint i = 0;
	gchar buff[128];

/*
<item key="sys.test.uint"				type="UINT" 	min="" max="" def="" val="33" />
<item key="sys.test.int"				type="INT"	 	min="" max="" def="" val="-2" />
<item key="sys.test.bool"				type="BOOL" 	min="" max="" def="" val="1" />
*/
	g_message("----------------------------------------");
	//while(1)
	{
		GValue 		ret_value;
		GValue 		set_value;

		memset (&ret_value, 0, sizeof (GValue));
		memset (&set_value, 0, sizeof (GValue));

		nf_sysdb_get_key0("sys.test.uint", &ret_value, NULL);
		g_message ( "NF_HOST sys.test.uint [%d]", g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);

		nf_sysdb_get_key0("sys.test.int", &ret_value, NULL);
		g_message ( "NF_HOST sys.test.int [%d]", g_value_get_int(&ret_value));
		g_value_unset(&ret_value);

		nf_sysdb_get_key0("sys.test.bool", &ret_value, NULL);
		g_message ( "NF_HOST sys.test.bool [%d]", g_value_get_boolean(&ret_value));
		g_value_unset(&ret_value);

		nf_sysdb_get_key0("sys.info.sysid", &ret_value, NULL);
		g_message ( "NF_HOST SYSID [%s]", g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

		nf_sysdb_get_key0("sys.info.swver", &ret_value, NULL);
		g_message ( "NF_HOST SWVER [%s]", g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

#if 0
<item key="sys.test.string"				type="STRING" 	min="4" max="8" val="ABCD" />
<item key="sys.test.uint"				type="UINT" 	min="0" max="9999" val="123" />
<item key="sys.test.int"				type="INT"	 	min="-9999" max="9999" val="-2" />
<item key="sys.test.bool"				type="BOOL" 	min="0" max="1" val="1" />
#endif
		nf_sysdb_validate_int("sys.test.int", -3);
		nf_sysdb_validate_int("sys.test.int", 3);
		nf_sysdb_validate_int("sys.test.int", -10000);
		nf_sysdb_validate_int("sys.test.int", 10000);

		nf_sysdb_validate_uint("sys.test.uint", 0);
		nf_sysdb_validate_uint("sys.test.uint", 1);
		nf_sysdb_validate_uint("sys.test.uint", 2);
		nf_sysdb_validate_uint("sys.test.uint", 100000);

		nf_sysdb_validate_bool("sys.test.bool", 0);
		nf_sysdb_validate_bool("sys.test.bool", 1);
		nf_sysdb_validate_bool("sys.test.bool", 2);

		nf_sysdb_validate_str("sys.test.string", "ABDD");
		nf_sysdb_validate_str("sys.test.string", "A");
		nf_sysdb_validate_str("sys.test.string", "AAAAAAAAAAAAAA");


		g_value_init( &set_value,  G_TYPE_STRING);
		g_snprintf(buff, sizeof(buff), "test software id_%d", ++i);
		g_value_set_string(&set_value, buff);
		nf_sysdb_set_key0("sys.info.swver", &set_value, NULL);
		g_value_unset(&set_value);

		nf_sysdb_get_key0("sys.info.swver", &ret_value, NULL);
		g_message ( "NF_HOST SWVER [%s]", g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

		//nf_sysdb_dump_all();
		usleep(10*1000);
	}
	g_message("----------------------------------------");

#if 0	// testset
	g_message("---- init");
	nf_sysdb_dump("sys");
	g_return_val_if_fail ( nf_sysdb_load("sys") , FALSE);

	g_message("---- load");
	nf_sysdb_dump("sys");

	nf_sysdb_default("sys.date");
	g_message("---- def");
	nf_sysdb_dump("sys");

#endif

	return 1;
}


/**
	@brief				sysdb ��ȸ
	@param[in]	key		��ȸ ?ϰ? ??�� sysdb key
	@param[out]	retval	��ȸ ???? (G_TYPE_STRING)
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_get_key0(	const gchar *key,
						GValue *retval, GError **error)
{
	gboolean ret = TRUE;

	/* ?Ķ????? ��?? */
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	ret = nf_sysdb_db_get( _nf_sysdb, key, retval, error);

	return ret; // api ȣ?? ???? ��??
}

/**
	@brief				sysdb ��ȸ
	@param[in]	key		��ȸ ?ϰ? ??�� sysdb key
	@param[in]	idx1	��ȸ ?ϰ? ??�� idx ??ȣ
	@param[out]	retval	��ȸ ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred

	@see				@nf_sysdb_set_cam_title
*/
gboolean
nf_sysdb_get_key1( 	const gchar *key,
						guint idx0 , 
						GValue *retval, GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	g_snprintf( key_buff,sizeof(key_buff), key, idx0);
	ret = nf_sysdb_db_get( _nf_sysdb, key_buff, retval, error);

	return ret;
}



/**
	@brief				sysdb ��ȸ
	@param[in]	key		��ȸ ?ϰ? ??�� sysdb key
	@param[in]	idx1	��ȸ ?ϰ? ??�� idx1 ??ȣ
	@param[in]	idx2	��ȸ ?ϰ? ??�� idx2 ??ȣ
	@param[out]	retval	��ȸ ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred

	@see				@nf_sysdb_set_cam_title
*/
gboolean
nf_sysdb_get_key2(	const gchar *key,
						guint idx0, const guint idx1 , 
						GValue *retval,	GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);
	
	g_snprintf( key_buff,sizeof(key_buff), key, idx0, idx1);
	ret = nf_sysdb_db_get( _nf_sysdb, key_buff, retval, error);

	return ret;
}

/**
	@brief				sysdb ��ȸ
	@param[in]	key		��ȸ ?ϰ? ??�� sysdb key
	@param[in]	idx1	��ȸ ?ϰ? ??�� idx1 ??ȣ
	@param[in]	idx2	��ȸ ?ϰ? ??�� idx2 ??ȣ
	@param[out]	retval	��ȸ ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred

	@see				@nf_sysdb_set_cam_title
*/
gboolean
nf_sysdb_get_key3(	const gchar *key,
						const guint idx0, const guint idx1 , const guint idx2 ,
						GValue *retval,	GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	g_snprintf( key_buff,sizeof(key_buff), key, idx0, idx1, idx2);
	ret = nf_sysdb_db_get( _nf_sysdb, key_buff, retval, error);

	return ret;
}


/**
	@brief				sysdb ?ش? Ű???? string�� ???? (???�?
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@return	gchar *		string pointer on success, NULL if an error occurred
*/
gchar *
nf_sysdb_get_str( const gchar *property_name )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, NULL );
	g_return_val_if_fail ( property_name, NULL );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	if(tmp != NULL &&  G_VALUE_HOLDS_STRING(tmp) )
		return g_value_dup_string(tmp);

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return NULL;
}

/**
	@brief				sysdb ??��?ϱ? string type
	@param[in]	key		get string key
	@param[need_base64_decode]   need to 1
	@param[file_path]			 file_descriptor
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean nf_sysdb_get_str_to_file( const gchar *property_name, int need_base64_decode, char *file_path )
{
	char *db_str = NULL;
	FILE *fp = NULL;
	int len = 0;
	int decode_len = 0;
	gboolean ret = FALSE;

	g_return_val_if_fail (property_name != NULL, FALSE);
	g_return_val_if_fail (file_path != NULL, FALSE);

	db_str = nf_sysdb_get_str_nocopy(property_name);

	if(db_str)
	{
		if( need_base64_decode )
		{
			len = strlen(db_str);

			if( len > 0 )
			{
				char *decoded = NULL;

				decoded = g_malloc(len);
				memset(decoded, 0x0, len);
				decode_len = base64_decode(db_str, len, decoded);

				fp = fopen(file_path,"w");
				if( fp )
				{
					fwrite(decoded, 1 ,decode_len, fp);
					fclose(fp);

					ret = TRUE;
				}

				g_free(decoded);
			}
		}
		else
		{
			fp = fopen(file_path,"w");
			if( fp )
			{
				fprintf(fp, "%s", db_str);
				fclose(fp);

				ret = TRUE;
			}
		}
	}

	return ret;
}

/**
	@brief				sysdb ?ش? Ű???? string�� ????(ī?? ????, ???? ?ϸ? ?ȵ?)
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@return	gchar *		string pointer on success, NULL if an error occurred
*/
gchar *
nf_sysdb_get_str_nocopy( const gchar *property_name )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, NULL );
	g_return_val_if_fail ( property_name, NULL );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	if(tmp != NULL &&  G_VALUE_HOLDS_STRING(tmp) )
		return g_value_get_string(tmp);

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return NULL;
}

/**
	@brief				sysdb ?ش? Ű???? str???·? ?????? map ???? ????
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@param[in]	idx		??Ʈ???? idx
	@return		gint	value on success, 0 if an error occurred

*/
gint
nf_sysdb_get_strmap( const gchar *property_name, gint idx )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	// ??Ʈ?????? ??ŭ idx ??ġ?? ???? ???? ó???? ?? ???ΰ??

	if(tmp != NULL && G_VALUE_HOLDS_STRING(tmp) )
		return g_value_get_string(tmp)[idx];

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return NULL;
}


/**
	@brief				sysdb ?ش? Ű???? bool ???? ?ޱ?
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@return		gint	value on success, 0 if an error occurred

*/
gboolean
nf_sysdb_get_bool( const gchar *property_name )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	if(tmp != NULL &&  G_VALUE_HOLDS_BOOLEAN(tmp) )
		return g_value_get_boolean(tmp);

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return 0;

}

/**
	@brief				sysdb ?ش? Ű???? guint ???? ?ޱ?
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@return		gint	value on success, 0 if an error occurred

*/
guint
nf_sysdb_get_uint ( const gchar *property_name )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	if(tmp != NULL &&  G_VALUE_HOLDS_UINT(tmp) )
		return g_value_get_uint(tmp);

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return 0;

}

/**
	@brief				sysdb ?ش? Ű???? gint ???? ?ޱ?
	@param[in]	property_name	��ȸ ?ϰ? ??�� sysdb key
	@return		gint	value on success, 0 if an error occurred

*/
gint
nf_sysdb_get_int ( const gchar *property_name )
{
	GValue *tmp = NULL;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	tmp = nf_sysdb_db_get_gvalue( _nf_sysdb, property_name );

	if(tmp != NULL &&  G_VALUE_HOLDS_INT(tmp) )
		return g_value_get_int(tmp);

	g_warning("%s prop[%s] Error type miss[%s]",  __FUNCTION__, property_name, tmp ? G_VALUE_TYPE_NAME(tmp) : "NULL" );
	return 0;

}

/**
	@brief				sysdb ????
	@param[in]	key		???? ?ϰ? ??�� sysdb key
	@param[out]	retval	???? ???? (G_TYPE_STRING)
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_validate_key0(	const gchar *key,
						GValue *retval, GError **error)
{
	gboolean ret = TRUE;

	/* ?Ķ????? ��?? */
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	ret = nf_sysdb_db_validate( _nf_sysdb, key, retval, error);

	return ret; // api ȣ?? ???? ��??
}


/**
	@brief				sysdb ??��
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@param[out]	retval	??�� ???? (G_TYPE_STRING)
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_set_key0(	const gchar *key,
						GValue *retval, GError **error)
{
	gboolean ret = TRUE;

	/* ?Ķ????? ��?? */
	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	ret = nf_sysdb_db_set( _nf_sysdb, key, retval, error);

	return ret; // api ȣ?? ???? ��??
}

/**
	@brief				sysdb ??��
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@param[in]	idx1	??�� ?ϰ? ??�� idx ??ȣ
	@param[out]	retval	??�� ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_set_key1( 	const gchar *key,
						guint idx0 , 
						GValue *retval, GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	g_snprintf( key_buff,sizeof(key_buff), key, idx0);
	ret = nf_sysdb_db_set( _nf_sysdb, key_buff, retval, error);

	return ret;
}

/**
	@brief				sysdb ??��
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@param[in]	idx1	??�� ?ϰ? ??�� idx1 ??ȣ
	@param[in]	idx2	??�� ?ϰ? ??�� idx2 ??ȣ
	@param[out]	retval	??�� ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_set_key2(	const gchar *key,
						guint idx0, const guint idx1 , 
						GValue *retval,	GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	g_snprintf( key_buff,sizeof(key_buff), key, idx0, idx1);
	ret = nf_sysdb_db_set( _nf_sysdb, key_buff, retval, error);

	return ret;
}

/**
	@brief				sysdb ??��
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@param[in]	idx1	??�� ?ϰ? ??�� idx1 ??ȣ
	@param[in]	idx2	??�� ?ϰ? ??�� idx2 ??ȣ
	@param[out]	retval	??�� ????
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_sysdb_set_key3(	const gchar *key,
						const guint idx0, const guint idx1 , const guint idx2 ,
						GValue *retval,	GError **error)
{
	gboolean ret = TRUE;
	gchar	key_buff[256];

	g_return_val_if_fail (_nf_sysdb != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (retval != NULL, FALSE);

	g_snprintf( key_buff,sizeof(key_buff), key, idx0, idx1, idx2);
	ret = nf_sysdb_db_set( _nf_sysdb, key_buff, retval, error);

	return ret;
}

/**
	@brief					sysdb ??��?ϱ?  int type
	@param[in]	property_name	??�� ?ϰ? ??�� sysdb key
	@param[in]	value
	@return		gboolean	TRUE on success, FALSE if an error occurred

*/
gboolean
nf_sysdb_set_int ( const gchar *property_name, gint value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_INT);
	g_value_set_int(&set_value, value);

	ret = nf_sysdb_set_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief				sysdb ??��?ϱ?  uint type
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_set_uint ( const gchar *property_name, guint value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_UINT);
	g_value_set_uint(&set_value, value);

	ret = nf_sysdb_set_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief				sysdb ??��?ϱ?  boolean type
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_set_bool ( const gchar *property_name, gboolean value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, value);

	ret = nf_sysdb_set_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}


/**
	@brief				sysdb ??��?ϱ? string type
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_set_str ( const gchar *property_name, gchar *value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_STRING);
	g_value_set_string(&set_value, value);

	ret = nf_sysdb_set_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief				sysdb ??��?ϱ? string type
	@param[in]	key		??�� ?ϰ? ??�� sysdb key
	@param[need_base64_encode]   need to 1
	@param[file_path]			 file_descriptor
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean nf_sysdb_set_str_from_file( const gchar *property_name, int need_base64_encode, char *file_path )
{
	gchar *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;
	int len, base64_len;
	gboolean ret = FALSE;

	g_return_val_if_fail (property_name != NULL, FALSE);
	g_return_val_if_fail (file_path != NULL, FALSE);

	if (!g_file_get_contents ( file_path , &contents, &length, &error))
	{
		g_warning("%s %s\n", __FUNCTION__,  error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return FALSE;
	}

	if(contents)
	{
		if( length < 4096 ){
			if( need_base64_encode )
			{
				char *base64 = NULL;

				len = base64_encode_len(length);

				base64 = g_malloc(len);
				memset(base64, 0x0, len);
				base64_len = base64_encode(contents, length, base64);

				ret = nf_sysdb_set_str(property_name, base64);

				g_free(base64);
			}
			else
			{
				ret = nf_sysdb_set_str(property_name, contents);
			}
		}
		else{
			g_message("%s - file size is too big", __FUNCTION__);
		}

		g_free(contents);
	}

	return ret;
}

/**
	@brief					sysdb ?????ϱ?  int type
	@param[in]	property_name	???? ?ϰ? ??�� sysdb key
	@param[in]	value
	@return		gboolean	TRUE on success, FALSE if an error occurred

*/
gboolean
nf_sysdb_validate_int ( const gchar *property_name, gint value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_INT);
	g_value_set_int(&set_value, value);

	ret = nf_sysdb_validate_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief				sysdb ?????ϱ?  uint type
	@param[in]	key		???? ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_validate_uint ( const gchar *property_name, guint value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_UINT);
	g_value_set_uint(&set_value, value);

	ret = nf_sysdb_validate_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief				sysdb ?????ϱ?  boolean type
	@param[in]	key		???? ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_validate_bool ( const gchar *property_name, gboolean value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, value);

	ret = nf_sysdb_validate_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}


/**
	@brief				sysdb ?????ϱ? string type
	@param[in]	key		???? ?ϰ? ??�� sysdb key
	@return		gboolean	TRUE on success, FALSE if an error occurred
*/
gboolean
nf_sysdb_validate_str ( const gchar *property_name, gchar *value )
{
	GValue set_value;
	gboolean ret;

	g_return_val_if_fail ( _nf_sysdb != NULL, FALSE );
	g_return_val_if_fail ( property_name, FALSE );

	memset (&set_value, 0, sizeof (GValue));

	g_value_init( &set_value,  G_TYPE_STRING);
	g_value_set_string(&set_value, value);

	ret = nf_sysdb_validate_key0(property_name, &set_value, NULL);
	g_value_unset(&set_value);

	return ret;
}


/******************************************************************************/
#ifdef DEBUG_SYSDB_JBSHELL

static char sysdb_set_help[] = "sysdb_dump [prop] [type:i,u,b,s] [value]";
static int sysdb_set(int argc, char **argv)
{
	gchar	*prop = NULL,*type = NULL,*value = NULL;

	if(argc < 4){
		printf("%s\n",sysdb_set_help);
		return -1;
	}

	if(argc > 1) prop = argv[1];
	if(argc > 2) type = argv[2];
	if(argc > 3) value = argv[3];

	if( type[0] == 'i')
		nf_sysdb_set_int( prop, strtol( value, NULL, 0));
	else if( type[0] == 'u')
		nf_sysdb_set_uint( prop, strtoul( value, NULL, 0));
	else if( type[0] == 'b')
		nf_sysdb_set_bool( prop, strtol( value, NULL, 0));
	else if( type[0] == 's')
		nf_sysdb_set_str( prop, value);
	else
		printf("unknown type[%s]\n", type);
	return 0;
}
__commandlist(sysdb_set,"sysdb_set",sysdb_set_help, sysdb_set_help);

static void sysdb_print_cate()
{
	printf("sys     0\n");// 0
	printf("net     1\n");
	printf("audio   2\n");
	printf("disk    3\n");
	printf("cam     4\n");
	printf("usr     5\n");// 5
	printf("alarm   6\n");
	printf("act     7\n");
	printf("disp    8\n");
	printf("rec     9\n");
}
static char sysdb_dump_help[] = "sysdb_dump [cate:all sys net audio disk cam usr alarm act disp rec]";
static int sysdb_dump(int argc, char **argv)
{
	if(argc < 2){
		printf("%s\n",sysdb_dump_help);
		return -1;
	}

	if( strncmp( argv[1], "all", 3) == 0 )
		nf_sysdb_dump_all( );
	else
		nf_sysdb_dump( argv[1] );

	return 0;
}
__commandlist(sysdb_dump,"sysdb_dump",sysdb_dump_help, sysdb_dump_help);


static char sysdb_save_help[] = "sysdb_save [cate:all sys net audio disk cam usr alarm act disp rec]";
static int sysdb_save(int argc, char **argv)
{
	if(argc < 2){
		printf("%s\n",sysdb_save_help);
		return -1;
	}

	if( strncmp( argv[1], "all", 3) == 0 )
		nf_sysdb_save_all( );
	else
		nf_sysdb_save( argv[1] );

	return 0;
}
__commandlist(sysdb_save,"sysdb_save",sysdb_save_help, sysdb_save_help);


static char sysdb_load_help[] = "sysdb_load [cate:all sys net audio disk cam usr alarm act disp rec] [factory:0]";
static int sysdb_load(int argc, char **argv)
{
	gint load_default = 0;

	if(argc < 2){
		printf("%s\n",sysdb_load_help);
		return -1;
	}

	if(argc > 2) load_default = strtol( argv[2], NULL, 0);

	if( load_default == 0 )
		nf_sysdb_load( argv[1] );
	else
		nf_sysdb_default( argv[1] );

	return 0;
}
__commandlist(sysdb_load,"sysdb_load",sysdb_load_help, sysdb_load_help);


static char sysdb_2buff_help[] = "sysdb_2buff [idx]";
static int sysdb_2buff(int argc, char **argv)
{
	gint idx = 0;
	char buff[64*1024];
	int  size = 0, ret;

	if(argc < 2){
		printf("%s\n",sysdb_2buff_help);
		sysdb_print_cate();
		return -1;
	}

	idx = strtol( argv[1], NULL, 0);

	if(idx >= NF_SYSDB_CONVERT_TYPE_NR)
	{
		printf("idx over[%d]\n", idx);
		sysdb_print_cate();
		return 0;
	}

	memset( buff, 0x00, sizeof(buff));

	ret = nf_sysdb_sysdb_to_buff( idx, buff, &size);
	if(ret)
	{
		printf("idx[%d][%s] ret[%d]\n", idx, _sysdb_convert_cate_str[idx], ret);
		nf_debug_hexdump(buff, size);
	}

	return 0;
}
__commandlist(sysdb_2buff,"sysdb_2buff",sysdb_2buff_help, sysdb_2buff_help);


static char sysdb_change_help[] = "sysdb_change [idx]";
static int sysdb_change(int argc, char **argv)
{
	guint idx = 0;
	int  size = 0, ret;

	if(argc < 2){
		printf("%s\n",sysdb_change_help);
		sysdb_print_cate();
		return -1;
	}

	idx = strtoul( argv[1], NULL, 0);

	if(idx >= NF_SYSDB_CONVERT_TYPE_NR)
	{
		printf("idx over[%d]\n", idx);
		sysdb_print_cate();
		return 0;
	}

	nf_notify_fire_params( "sysdb_change",idx,0,0,0);

	return 0;
}
__commandlist(sysdb_change,"sysdb_change",sysdb_change_help, sysdb_change_help);

#endif




/*
 * sysdb db property simple xml-parser
 */
static int 		depth = 0;

static void
indent (int extra)
{
	int i = 0;
	while (i < depth)
	{
    	fputs ("  ", stdout);
		++i;
	}
}

static void
_start_element_handler  (GMarkupParseContext *context,
                        const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values,
                        gpointer             user_data,
                        GError             **error)
{
	int i=0;
	const gchar *key=NULL,*type=NULL,*min=NULL,*max=NULL;
	const gchar *val=NULL;

	GValue 		*tmp_value = NULL;
	GParamSpec	*tmp_pspec = NULL;

	if( strcasecmp( element_name, "item") )
	{
		return;
	}

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_LOAD] )
	{
		indent (0);
		printf ("ELEMENT '%s'\n", element_name);
		++depth;
		i = 0;
		while (attribute_names[i] != NULL)
		{
			indent (1);
			printf ("%s=\"%s\"\n",
						attribute_names[i],
						attribute_values[i]);
			++i;
		}
		--depth;
		++depth;
	}
#endif


	i = 0;
	while (attribute_names[i] != NULL)
	{
		if( !strcasecmp( attribute_names[i], "key") )
			key = attribute_values[i];
		else if( !strcasecmp( attribute_names[i], "type") )
			type = attribute_values[i];
		else if( !strcasecmp( attribute_names[i], "min") )
			min = attribute_values[i];
		else if( !strcasecmp( attribute_names[i], "max") )
			max= attribute_values[i];
		else if( !strcasecmp( attribute_names[i], "val") )
			val = attribute_values[i];
		else{
			gint line_number, char_number;
			g_markup_parse_context_get_position (context, &line_number, &char_number);
			g_message("unknown attribute [%s][%s] on line %d char %d",
							element_name, attribute_names[i],
							line_number, char_number);
		}
		i++;
	}
	// UCHAR,UINT,STRING,INT

	if(  !key || !type || !min || !max || !val )
	{
		g_warning("null attr!! key=%s type=%s min=%s max=%s val=%s\n", key,type,min,max,val);
		return;
	}

	//printf("key=%s type=%s min=%s max=%s def=%s val=%s\n", key,type,min,max,def,val);
	#define g_value_new(type) g_value_init (g_new0(GValue, 1), type)

	if( !strcasecmp( type, "UINT" ) )
	{
		guint param_min = 0, param_max = G_MAXUINT;
		guint param_val = 0;

		if( strcmp(min,"") ) param_min = strtoul(min,NULL,0);
		if( strcmp(max,"") ) param_max = strtoul(max,NULL,0);
		if( strcmp(val,"") ) param_val = strtoul(val,NULL,0);

		tmp_pspec = g_param_spec_uint(key, key, type,
									param_min, param_max, param_val,
									G_PARAM_READWRITE);
		tmp_value = g_value_new(  G_PARAM_SPEC_VALUE_TYPE (tmp_pspec) );
		g_value_set_uint( tmp_value, param_val);

	}
	else if( !strcasecmp( type, "STRING" ) )
	{
		guint param_min = 0, param_max = G_MAXUINT;

		if( strcmp(min,"") ) param_min = strtol(min,NULL,0);
		if( strcmp(max,"") ) param_max = strtol(max,NULL,0);

		tmp_pspec = g_param_spec_string(key, key, type, val, G_PARAM_READWRITE);
		g_param_spec_set_qdata (tmp_pspec, quark_property_min, (gpointer) param_min );
		g_param_spec_set_qdata (tmp_pspec, quark_property_max, (gpointer) param_max );

		tmp_value = g_value_new(  G_PARAM_SPEC_VALUE_TYPE (tmp_pspec) );
		g_value_set_string( tmp_value, val);

	}
	else if( !strcasecmp( type, "BOOL" ) )
	{
		char *tmp = NULL;
		gint param_val = 0;

		if( strcmp(val,"") ) param_val = strtol(val,&tmp,0);
		if(tmp == val) // false/true ó?? ???ڷ? ?? ????
		{
			if( strcasecmp( val, "TRUE" ) == 0 )
				param_val = 1;
		}

		tmp_pspec = g_param_spec_boolean(key, key, type, param_val, G_PARAM_READWRITE);

		tmp_value = g_value_new(  G_PARAM_SPEC_VALUE_TYPE (tmp_pspec) );
		g_value_set_boolean( tmp_value, param_val);

	}
	else if( !strcasecmp( type, "INT" ) )
	{
		gint param_min = G_MININT, param_max = G_MAXINT;
		gint param_val = 0;

		if( strcmp(min,"") ) param_min = strtol(min,NULL,0);
		if( strcmp(max,"") ) param_max = strtol(max,NULL,0);
		if( strcmp(val,"") ) param_val = strtol(val,NULL,0);

		tmp_pspec = g_param_spec_int(key, key, type,
									param_min, param_max, param_val,
									G_PARAM_READWRITE);

		tmp_value = g_value_new(  G_PARAM_SPEC_VALUE_TYPE (tmp_pspec) );
		g_value_set_int( tmp_value, param_val);

	}
	else // unknown
	{
		gint line_number, char_number;
		g_markup_parse_context_get_position (context, &line_number, &char_number);
		g_message("unknown type [%s] on line %d char %d", type, line_number, char_number);
		return;
	}
	nf_sysdb_db_install_property_value ( _nf_sysdb, tmp_pspec, tmp_value );
}

static void
_start_value_element_handler  (GMarkupParseContext *context,
                        const gchar         *element_name,
                        const gchar        **attribute_names,
                        const gchar        **attribute_values,
                        gpointer             user_data,
                        GError             **error)
{
	int i = 0;
	const gchar *key=NULL,*type=NULL, *val=NULL;

	SYSDB_PARSE_PARAM *param = ( SYSDB_PARSE_PARAM *) user_data;
	const gchar *category = param->category;

	int			cate_len = 0;
	GValue 		*tmp_value = NULL;

	if( strcasecmp( element_name, "item") )
	{
		return;
	}

	if( category != NULL )
		cate_len = strlen( category);

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_LOAD] )
	{
		indent (0);
		printf ("ELEMENT '%s'\n", element_name);
		++depth;
		i = 0;
		while (attribute_names[i] != NULL)
		{
			indent (1);
			printf ("    %s=\"%s\"\n", attribute_names[i], attribute_values[i]);
			++i;
		}
		--depth;
		++depth;
	}
#endif

	i = 0;
	while (attribute_names[i] != NULL)
	{
		if( !strcasecmp( attribute_names[i], "key") )
		{
			key = attribute_values[i];
			if( !( cate_len == 0 || (cate_len >0 && strncmp( key, category, cate_len ) == 0))  )
			{
				// g_warning("different category[%s] key[%s]", category, key);
				return;
			}
		}
		else if( !strcasecmp( attribute_names[i], "type") )
			type = attribute_values[i];
		else if( !strcasecmp( attribute_names[i], "val") )
			val = attribute_values[i];

		i++;
	}


	if( !key || !type || !val )
	{
		g_warning("null attr!! key=%s type=%s val=%s\n", key,type,val);
		return;
	}

#if 0	// FIXME!!! choissi
	if( strncasecmp(key,"sys.",4)  == 0
		|| strncasecmp(key, "disp.osd", 8) == 0
		|| strncasecmp(key, "ipcam.", 6) == 0 )
#endif
	{
		for(i=0; i<_sysdb_skip_cnt; ++i)
		{
			if( (param->type & _sysdb_skip_list[i].skip_mask )
					&& strncasecmp(key,
							_sysdb_skip_list[i].key,
							_sysdb_skip_list[i].key_len ) == 0 )
			{

#ifdef DEBUG_SYSDB_LOG
	if( _DEBUG_SYSDB_log[DEBUG_SYSDB_IDX_LOAD_SKIP] )
		g_message("skip key[%s] [0x%08x]", key, _sysdb_skip_list[i].skip_mask);
#endif
				return;
			}
		}

	}
/*
	?̹? ???????? GParamSpecPool�� lookup?ؼ? ?ش? key?? GParamSpec�� ?????´?.

	1) nf_sysdb_*.conf ?? key?? value?? ?????? ?ְ?
	2) nf_sysdb.conf???? ?????? ??�� ???? sysdb default ??�� ???? ?? ??

	string ???? ?ش? ????��?? ??�� ??ȯ???? ?ؾ? ?Ѵ?.

	#define	G_PARAM_SPEC_VALUE_TYPE(pspec)	(G_PARAM_SPEC (pspec)->value_type)

*/

	if( !strcasecmp( type, "UINT" ) )
	{
		guint param_val = strtoul(val,NULL,0);

		tmp_value = g_value_new( G_TYPE_UINT  );
		g_value_set_uint( tmp_value, param_val);
	}
	else if( !strcasecmp( type, "STRING" ) )
	{
		tmp_value = g_value_new(G_TYPE_STRING );
		g_value_set_string( tmp_value, val);
	}
	else if( !strcasecmp( type, "BOOL" ) )
	{
		char *tmp = NULL;
		guint param_val = strtol(val,&tmp,0);

		if(tmp == val) // false/true ó?? ???ڷ? ?? ????
		{
			if( strcasecmp( val, "TRUE" ) == 0 )
				param_val = 1;
			else
				param_val = 0;
		}

		tmp_value = g_value_new(  G_TYPE_BOOLEAN );
		g_value_set_boolean( tmp_value, param_val);
	}
	else if( !strcasecmp( type, "INT" ) )
	{
		guint param_val = strtol(val,NULL,0);

		tmp_value = g_value_new(  G_TYPE_INT );
		g_value_set_int( tmp_value, param_val);
	}else{
		g_warning("unknown type!!  key[%s] type[%s]", type, key);
		return;
	}

	if(tmp_value) // value set ?ϱ?~;
	{
		nf_sysdb_db_set( _nf_sysdb, key, tmp_value, NULL);

		g_value_unset(tmp_value);
		g_free(tmp_value);
	}

	// g_param_value_validate ()
	return;

}

static void
_end_element_handler    (GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error)
{
#ifdef DEBUG_SYSDB_LOAD
	--depth;
	indent (0);
	//printf ("END '%s'\n", element_name);
#endif
}

// ????�� ?Ⱦ???
#if 0
static void
_text_handler                      (GMarkupParseContext *context,
                        const gchar         *text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error)
{
	indent (0);
	printf ("TEXT '%.*s'\n", (int)text_len, text);
}


static void
_passthrough_handler    (GMarkupParseContext *context,
                        const gchar         *passthrough_text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error)
{
	indent (0);
	printf ("PASS '%.*s'\n", (int)text_len, passthrough_text);
}
#endif

static void
_error_handler          (GMarkupParseContext *context,
                        GError              *error,
                        gpointer             user_data)
{
	fprintf (stderr, " %s\n", error->message);
}

static int
_load_xml(GMarkupParser *parser, const gchar *filename, const SYSDB_PARSE_PARAM *category)
{
	gchar *contents;
	gsize  length;
	GError *error;
	GMarkupParseContext *context;

	error = NULL;
	if (!g_file_get_contents (filename, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);
		g_free(contents);
		return -1;
	}
	g_message("fileopen name[%s] len[%d]", filename, length);
	context = g_markup_parse_context_new (parser, 0, (gpointer)category, NULL);

	if (!g_markup_parse_context_parse (context, contents, length, (gpointer)category))
    {
      g_markup_parse_context_free (context);
      g_free(contents);
      return -2;
    }

	if (!g_markup_parse_context_end_parse (context, NULL))
	{
		g_markup_parse_context_free (context);
		g_free(contents);
		return -3;
	}
    g_markup_parse_context_free (context);

    g_free(contents);
    return 1;
}


#ifdef NF_SYSDB_CONVERT

/**
		@brief						value validate check function
		@param[in]  teble_param		teble parameter
		@param[out] buff			buff having value
		@return		gboolean  		%TRUE on success, %FALSE if an error occurred
**/
gboolean
nf_sysdb_convert_value_validate_check(guint table_param, gchar* buff, gint size)
{
	gboolean ret = TRUE;
	gint i=0;
	SYSDB_CONVERT_TABLE *cur_table;

	g_message("%s called", __FUNCTION__);
	g_return_val_if_fail ( table_param < NF_SYSDB_CONVERT_TYPE_NR , FALSE );
	g_return_val_if_fail ( buff != NULL , FALSE );

	while(1)
	{
		cur_table = &(_sysdb_convert_table[table_param][i++]);


#ifdef DEBUG_SYSDB_CONVERT
		g_message("%s prop[%-32s] idx[%5d] type[%d] len[%d]", __FUNCTION__,
				cur_table->prop_name,
				cur_table->idx,
				cur_table->val_type,
				cur_table->strlen	 );
#endif

		if(strcmp(cur_table -> prop_name , "NULL") == 0)
			break;

		if(cur_table->val_type == SYSDB_TYPE_STRING)
		{
			gchar *tmp_val = NULL;

			tmp_val = (gchar *)g_malloc0(cur_table->strlen+1);
			g_return_val_if_fail( tmp_val, 0);

			strncpy( tmp_val, &buff[cur_table->idx], cur_table->strlen );

			ret = nf_sysdb_validate_str(cur_table->prop_name, tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_validate_str error.. ret[%d] prop_name[%s]",
							__FUNCTION__, ret, cur_table->prop_name);
				break;
			}
			g_free(tmp_val);
		}
		else if(cur_table->val_type == SYSDB_TYPE_INT)
		{
			gint tmp_val = *(gint *)&buff[cur_table->idx];
			ret = nf_sysdb_validate_int(cur_table->prop_name , tmp_val);

			if(!ret){
				g_warning("%s nf_sysdb_validate_int error.. ret[%d] prop_name[%s] tmp_val[%d]",
						__FUNCTION__, ret, cur_table->prop_name, tmp_val);
				break;
			}
		}
		else if(cur_table->val_type == SYSDB_TYPE_UINT)
		{
			guint tmp_val = *(guint *)&buff[cur_table->idx];

			ret = nf_sysdb_validate_uint(cur_table->prop_name , tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_validate_uint error.. ret[%d] prop_name[%s] tmp_val[%d]",
							__FUNCTION__, ret, cur_table->prop_name, tmp_val);
				break;
			}
		}
		else if(cur_table->val_type == SYSDB_TYPE_BOOL)
		{
			gboolean tmp_val = *(gboolean *)&buff[cur_table->idx];

			ret = nf_sysdb_validate_bool(cur_table->prop_name , tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_validate_bool error.. ret[%d] prop_name[%s] tmp_val[%d]\n",
						__FUNCTION__, ret, cur_table->prop_name, tmp_val);
				break;
			}
		}
		else
		{
			g_warning("%s SYSDB_TYPE Not exist type[%d]\n", __FUNCTION__,
						cur_table->val_type);

			ret = FALSE;
			break;
		}
	}

	return ret;
}

/**
		@brief						set value to sysdb
		@param[in]  teble_param		teble parameter
		@param[out]	buff 			buff having value
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
**/
gboolean nf_sysdb_buff_to_sysdb(guint table_param , gchar *buff, gint size)
{
	gint i=0;
	gboolean ret = TRUE;
	SYSDB_CONVERT_TABLE *cur_table;

	g_return_val_if_fail ( table_param < NF_SYSDB_CONVERT_TYPE_NR , FALSE );
	g_return_val_if_fail ( buff != NULL , FALSE );

	g_message("%s called", __FUNCTION__);

#if 0

	ret = nf_sysdb_convert_value_validate_check(table_param, buff, size);
	if(!ret)
	{
		g_warning("%s validate error ret[%d]", __FUNCTION__, ret);
		return ret;
	}

#endif

	i = 0;
	while(1)
	{
		cur_table = &(_sysdb_convert_table[table_param][i++]);


#ifdef DEBUG_SYSDB_CONVERT
		g_message("%s prop[%-32s] idx[%5d] type[%d] len[%d]", __FUNCTION__,
				cur_table->prop_name,
				cur_table->idx,
				cur_table->val_type,
				cur_table->strlen	 );
#endif

		if(strcmp(cur_table -> prop_name , "NULL") == 0)
			 break;

		if(cur_table->val_type == SYSDB_TYPE_STRING)
		{
			gchar *tmp_val = NULL;

			tmp_val = (gchar *)g_malloc0(cur_table->strlen+1);
			g_return_val_if_fail( tmp_val, 0);

			strncpy( tmp_val, &buff[cur_table->idx], cur_table->strlen );

			ret = nf_sysdb_set_str(cur_table->prop_name, tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_set_str error ret[%d] prop_name[%s]",
							__FUNCTION__, ret, cur_table->prop_name);
			}

			g_free(tmp_val);
		}
		else if(cur_table->val_type == SYSDB_TYPE_INT)
		{
			gint tmp_val = *(gint*)&buff[cur_table->idx];
			ret = nf_sysdb_set_int(cur_table->prop_name , tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_set_int error ret[%d] prop_name[%s] tmp_val[%d]",
							__FUNCTION__, ret, cur_table->prop_name, tmp_val);
			}
		}
		else if(cur_table->val_type == SYSDB_TYPE_UINT)
		{
			guint tmp_val = *(guint*)&buff[cur_table->idx];
			ret = nf_sysdb_set_uint(cur_table->prop_name , tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_set_uint error ret[%d] prop_name[%s] tmp_val[%d]",
							__FUNCTION__, ret, cur_table->prop_name, tmp_val);
			}
		}
		else if(cur_table->val_type == SYSDB_TYPE_BOOL)
		{
			gboolean tmp_val = *(gboolean*)&buff[cur_table->idx];
			ret = nf_sysdb_set_bool(cur_table->prop_name , tmp_val);
			if(!ret){
				g_warning("%s nf_sysdb_set_bool error ret[%d] prop_name[%s] tmp_val[%d]",
							__FUNCTION__, ret, cur_table->prop_name, tmp_val);
			}
		}
		else{
			//warning message
			g_warning("%s SYSDB_TYPE Not exist type[%d]", __FUNCTION__,
						cur_table->val_type);
			ret = FALSE;
			break;
		}
	}//end while

	return ret;
}

/**
		@brief						get value from sysdb and store to buff
		@param[in]  teble_param		teble parameter
		@param[out]	buff 			empty buff
		@return gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_sysdb_sysdb_to_buff(guint table_param , gchar *buff, gint *size)
{
	gint i=0;
	gboolean ret = TRUE;
	SYSDB_CONVERT_TABLE *cur_table;
	gint tmp_size = 0;

	g_return_val_if_fail ( table_param < NF_SYSDB_CONVERT_TYPE_NR , FALSE );
	g_return_val_if_fail ( buff != NULL , FALSE );

#if 0
	cur_table = &(_sysdb_convert_table[0][0]);
	printf("%s\n", cur_table->prop_name);
#endif

	i = 0;
	while(1)
	{
		cur_table = &(_sysdb_convert_table[table_param][i++]);

#ifdef DEBUG_SYSDB_CONVERT
		g_message("%s prop[%-32s] idx[%5d] type[%d] len[%d]", __FUNCTION__,
				cur_table->prop_name,
				cur_table->idx,
				cur_table->val_type,
				cur_table->strlen	 );
#endif

		if(strcmp(cur_table->prop_name , "NULL") == 0)
			break;

		if(cur_table->val_type == SYSDB_TYPE_STRING)
		{
			gchar *tmp_val = nf_sysdb_get_str_nocopy(cur_table->prop_name);
			if(!tmp_val)
			{
				g_warning("%s prop not found.. prop_name[%s]",
							 __FUNCTION__, cur_table->prop_name);
				ret = FALSE;
				break;
			}

			strncpy(&buff[cur_table->idx], tmp_val, cur_table->strlen);

			//memcpy(&buff[cur_table->idx], tmp_val, strlen(tmp_val) );
			//g_free(tmp_val);

			tmp_size += cur_table->strlen;
    	}
		else if(cur_table->val_type == SYSDB_TYPE_INT)
		{
			gint tmp_val = nf_sysdb_get_int(cur_table->prop_name);
			*(gint *)&buff[cur_table->idx] = tmp_val;

			tmp_size += sizeof(gint);
		}
		else if(cur_table->val_type == SYSDB_TYPE_UINT)
		{
			guint tmp_val = nf_sysdb_get_uint(cur_table->prop_name);
			*(guint *)&buff[cur_table->idx] = tmp_val;

			tmp_size += sizeof(guint);
		}
		else if(cur_table->val_type == SYSDB_TYPE_BOOL)
		{
			gboolean tmp_val = nf_sysdb_get_bool(cur_table->prop_name);
			*(guint *)&buff[cur_table->idx] = tmp_val;

			tmp_size += sizeof(guint);
		}
		else{
			//warning message
			g_warning("%s SYSDB_TYPE Not exist type[%d]", __FUNCTION__,
						cur_table->val_type);

			ret = FALSE;
			break;
		}
	}//end while

	if(ret == TRUE)
		*size= tmp_size;

	return ret;
}

#endif

//#ifdef ENABLE_USER_PASSWORD_ENCODING
#if 1

/* ***********************************  Encoding  ********************************* */
static RET_VAL SEED_EncInit(SEED_ALG_INFO *AlgInfo) {
	AlgInfo->BufLen = 0;
	if (AlgInfo->ModeID != AI_ECB) {
//		memcpy(AlgInfo->ChainVar, AlgInfo->IV, SEED_BLOCK_LEN);
		int i;
		for (i = 0; i < SEED_BLOCK_LEN; i++) {
			AlgInfo->ChainVar[i] = AlgInfo->IV[i];
		}
	}
	return CTR_SUCCESS;
}

static RET_VAL PaddSet(BYTE *pbOutBuffer, DWORD dRmdLen, DWORD dBlockLen,
		DWORD dPaddingType) {
	int i;
	DWORD dPadLen;

	switch (dPaddingType) {
	case AI_NO_PADDING:
		if (dRmdLen == 0)
			return 0;
		else
			return CTR_DATA_LEN_ERROR;

	case AI_PKCS_PADDING:
		dPadLen = dBlockLen - dRmdLen;
//			memset(pbOutBuffer+dRmdLen, (char)dPadLen, (int)dPadLen);
		for (i = 0; i < (int) (dPadLen); i++) {
			pbOutBuffer[i + dRmdLen] = (char) dPadLen;
		}
		return dPadLen;
	default:
		return CTR_FATAL_ERROR;
	}
}

static RET_VAL ECB_EncUpdate(SEED_ALG_INFO *AlgInfo,		//
		BYTE *PlainTxt,		//	?ԷµǴ? ?????? pointer
		DWORD PlainTxtLen,	//	?ԷµǴ? ?????? ????Ʈ ??
		BYTE *CipherTxt, 	//	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	int i;
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;

	//
	*CipherTxtLen = BufLen + PlainTxtLen;

	//	No one block
	if (*CipherTxtLen < BlockLen) {
//		memcpy(AlgInfo->Buffer+BufLen, PlainTxt, (int)PlainTxtLen);
		for (i = 0; i < (int) PlainTxtLen; i++) {
			AlgInfo->Buffer[BufLen + i] = PlainTxt[i];
		}
		AlgInfo->BufLen += PlainTxtLen;
		*CipherTxtLen = 0;
		return CTR_SUCCESS;
	}

	//	control the case that PlainTxt and CipherTxt are the same buffer
	if (PlainTxt == CipherTxt)
		return CTR_FATAL_ERROR;

	//	first block
//	memcpy(AlgInfo->Buffer+BufLen, PlainTxt, (int)(BlockLen - BufLen));
	for (i = 0; i < (int) (BlockLen - BufLen); i++) {
		AlgInfo->Buffer[BufLen + i] = PlainTxt[i];
	}
	PlainTxt += BlockLen - BufLen;
	PlainTxtLen -= BlockLen - BufLen;

	//	core part
	BlockCopy(CipherTxt, AlgInfo->Buffer);
	SEED_Encrypt(ScheduledKey, CipherTxt);
	CipherTxt += BlockLen;
	while (PlainTxtLen >= BlockLen) {
		BlockCopy(CipherTxt, PlainTxt);
		SEED_Encrypt(ScheduledKey, CipherTxt);
		PlainTxt += BlockLen;
		CipherTxt += BlockLen;
		PlainTxtLen -= BlockLen;
	}

	//	save remained data
//	memcpy(AlgInfo->Buffer, PlainTxt, (int)PlainTxtLen);
	for (i = 0; i < (int) (PlainTxtLen); i++) {
		AlgInfo->Buffer[i] = PlainTxt[i];
	}

	AlgInfo->BufLen = PlainTxtLen;
	*CipherTxtLen -= PlainTxtLen;

	//	control the case that PlainTxt and CipherTxt are the same buffer
	return CTR_SUCCESS;/* ******************************************************************************** */

}

static RET_VAL CBC_EncUpdate(SEED_ALG_INFO *AlgInfo, BYTE *PlainTxt,
		DWORD PlainTxtLen, BYTE *CipherTxt, DWORD *CipherTxtLen) {
	int i;
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;

	//
	*CipherTxtLen = BufLen + PlainTxtLen;

	//	No one block
	if (*CipherTxtLen < BlockLen) {
//		memcpy(AlgInfo->Buffer+BufLen, PlainTxt, (int)PlainTxtLen);
		for (i = 0; i < (int) PlainTxtLen; i++) {
			AlgInfo->Buffer[BufLen + i] = PlainTxt[i];
		}

		AlgInfo->BufLen += PlainTxtLen;
		*CipherTxtLen = 0;
		return CTR_SUCCESS;
	}

	//	control the case that PlainTxt and CipherTxt are the same buffer
	if (PlainTxt == CipherTxt)
		return CTR_FATAL_ERROR;

	//	first block
//	memcpy(AlgInfo->Buffer+BufLen, PlainTxt, (int)(BlockLen - BufLen));
	for (i = 0; i < (int) (BlockLen - BufLen); i++) {
		AlgInfo->Buffer[BufLen + i] = PlainTxt[i];
	}
	PlainTxt += BlockLen - BufLen;
	PlainTxtLen -= BlockLen - BufLen;

	//	core part
	BlockXor(CipherTxt, AlgInfo->ChainVar, AlgInfo->Buffer);
	SEED_Encrypt(ScheduledKey, CipherTxt);
	CipherTxt += BlockLen;
	while (PlainTxtLen >= BlockLen) {
		BlockXor(CipherTxt, CipherTxt-BlockLen, PlainTxt);
		SEED_Encrypt(ScheduledKey, CipherTxt);
		PlainTxt += BlockLen;
		CipherTxt += BlockLen;
		PlainTxtLen -= BlockLen;
	}
	BlockCopy(AlgInfo->ChainVar, CipherTxt-BlockLen);

	//	save remained data
//	memcpy(AlgInfo->Buffer, PlainTxt, (int)PlainTxtLen);
	for (i = 0; i < (int) (PlainTxtLen); i++) {
		AlgInfo->Buffer[i] = PlainTxt[i];
	}
	AlgInfo->BufLen = PlainTxtLen;
	*CipherTxtLen -= PlainTxtLen;

	//
	return CTR_SUCCESS;
}

static RET_VAL SEED_EncUpdate(SEED_ALG_INFO *AlgInfo, BYTE *PlainTxt,
		DWORD PlainTxtLen, BYTE *CipherTxt, DWORD *CipherTxtLen) {
	switch (AlgInfo->ModeID) {
	case AI_CBC:
		return CBC_EncUpdate(AlgInfo, PlainTxt, PlainTxtLen, CipherTxt,
				CipherTxtLen);
	default:
		return CTR_FATAL_ERROR;
	}
}

void SEED_Encrypt(void *CipherKey,		//	??/??ȣ?? Round Key
		BYTE *Data)			//	??????�� ��?? ????�� ????Ű?? pointer
{
	DWORD A, B, C, D, *K = (DWORD*) CipherKey;

	BIG_B2D( &(Data[8]), C);
	BIG_B2D( &(Data[12]), D);
	BIG_B2D( &(Data[0]), A);
	BIG_B2D( &(Data[4]), B);

	//
	SeedRound(A, B, C, D, K);
	SeedRound(C, D, A, B, K+ 2);
	SeedRound(A, B, C, D, K+ 4);
	SeedRound(C, D, A, B, K+ 6);
	SeedRound(A, B, C, D, K+ 8);
	SeedRound(C, D, A, B, K+10);
	SeedRound(A, B, C, D, K+12);
	SeedRound(C, D, A, B, K+14);
	SeedRound(A, B, C, D, K+16);
	SeedRound(C, D, A, B, K+18);
	SeedRound(A, B, C, D, K+20);
	SeedRound(C, D, A, B, K+22);
	SeedRound(A, B, C, D, K+24);
	SeedRound(C, D, A, B, K+26);
	SeedRound(A, B, C, D, K+28);
	SeedRound(C, D, A, B, K+30);

	//
	BIG_D2B(A, &(Data[8]));
	BIG_D2B(B, &(Data[12]));
	BIG_D2B(C, &(Data[0]));
	BIG_D2B(D, &(Data[4]));

	//	Remove sensitive data
	A = B = C = D = 0;
	K = NULL;
}

static RET_VAL SEED_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt, //	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	switch (AlgInfo->ModeID) {
	case AI_CBC:
		return CBC_EncFinal(AlgInfo, CipherTxt, CipherTxtLen);
	default:
		return CTR_FATAL_ERROR;
	}
}

static RET_VAL ECB_EncFinal(SEED_ALG_INFO *AlgInfo,		//
		BYTE *CipherTxt, 	//	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;
	DWORD PaddByte;

	//	Padding
	PaddByte = PaddSet(AlgInfo->Buffer, BufLen, BlockLen, AlgInfo->PadType);
	if (PaddByte > BlockLen)
		return PaddByte;

	if (PaddByte == 0) {
		*CipherTxtLen = 0;
		return CTR_SUCCESS;
	}

	//	core part
	BlockCopy(CipherTxt, AlgInfo->Buffer);
	SEED_Encrypt(ScheduledKey, CipherTxt);

	//
	*CipherTxtLen = BlockLen;

	//
	return CTR_SUCCESS;
}

static RET_VAL CBC_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt, //	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;
	DWORD PaddByte;

	//	Padding
	PaddByte = PaddSet(AlgInfo->Buffer, BufLen, BlockLen, AlgInfo->PadType);
	if (PaddByte > BlockLen)
		return PaddByte;

	if (PaddByte == 0) {
		*CipherTxtLen = 0;
		return CTR_SUCCESS;
	}

	//	core part
	BlockXor(CipherTxt, AlgInfo->Buffer, AlgInfo->ChainVar);
	SEED_Encrypt(ScheduledKey, CipherTxt);
	BlockCopy(AlgInfo->ChainVar, CipherTxt);

	//
	*CipherTxtLen = BlockLen;

	//
	return CTR_SUCCESS;
}

static RET_VAL OFB_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt, //	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN;
	DWORD BufLen = AlgInfo->BufLen;
	DWORD i;

	//	Check Output Memory Size
	*CipherTxtLen = BlockLen;

	//	core part
	SEED_Encrypt(ScheduledKey, AlgInfo->ChainVar);
	for (i = 0; i < BufLen; i++)
		CipherTxt[i] = (BYTE) (AlgInfo->Buffer[i] ^ AlgInfo->ChainVar[i]);

	//
	*CipherTxtLen = BufLen;

	//
	return CTR_SUCCESS;
}

static RET_VAL CFB_EncFinal(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt, //	??ȣ???? ???µ? pointer
		DWORD *CipherTxtLen)	//	???µǴ? ??ȣ???? ????Ʈ ??
{
	int i;
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BufLen = AlgInfo->BufLen;

	//	Check Output Memory Size
	*CipherTxtLen = BufLen;

	//	core part
	SEED_Encrypt(ScheduledKey, AlgInfo->ChainVar);
	BlockXor(AlgInfo->ChainVar, AlgInfo->ChainVar, AlgInfo->Buffer);
//	memcpy(CipherTxt, AlgInfo->ChainVar, BufLen);
	for (i = 0; i < (int) (BufLen); i++) {
		CipherTxt[i] = AlgInfo->ChainVar[i];
	}

	//
	*CipherTxtLen = BufLen;

	//
	return CTR_SUCCESS;
}

/* ******************************************************************************** */
/* ***********************************  Decoding   ******************************** */
static RET_VAL SEED_DecUpdate(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt,
		DWORD CipherTxtLen, BYTE *PlainTxt, DWORD *PlainTxtLen) {
	switch (AlgInfo->ModeID) {
	case AI_CBC:
		return CBC_DecUpdate(AlgInfo, CipherTxt, CipherTxtLen, PlainTxt,
				PlainTxtLen);
	default:
		return CTR_FATAL_ERROR;
	}
}

void SEED_Decrypt(void *CipherKey, BYTE *Data) {
	DWORD A, B, C, D, *K = (DWORD*) CipherKey;

	//
	BIG_B2D( &(Data[8]), C);
	BIG_B2D( &(Data[12]), D);
	BIG_B2D( &(Data[0]), A);
	BIG_B2D( &(Data[4]), B);

	//
	SeedRound(A, B, C, D, K+30);
	SeedRound(C, D, A, B, K+28);
	SeedRound(A, B, C, D, K+26);
	SeedRound(C, D, A, B, K+24);
	SeedRound(A, B, C, D, K+22);
	SeedRound(C, D, A, B, K+20);
	SeedRound(A, B, C, D, K+18);
	SeedRound(C, D, A, B, K+16);
	SeedRound(A, B, C, D, K+14);
	SeedRound(C, D, A, B, K+12);
	SeedRound(A, B, C, D, K+10);
	SeedRound(C, D, A, B, K+ 8);
	SeedRound(A, B, C, D, K+ 6);
	SeedRound(C, D, A, B, K+ 4);
	SeedRound(A, B, C, D, K+ 2);
	SeedRound(C, D, A, B, K+ 0);

	//
	BIG_D2B(A, &(Data[8]));
	BIG_D2B(B, &(Data[12]));
	BIG_D2B(C, &(Data[0]));
	BIG_D2B(D, &(Data[4]));

	//	Remove sensitive data
	A = B = C = D = 0;
	K = NULL;
}

static RET_VAL PaddCheck(BYTE *pbOutBuffer, DWORD dBlockLen, DWORD dPaddingType) {
	DWORD i, dPadLen;

	switch (dPaddingType) {
	case AI_NO_PADDING:
		return 0;			//	padding?? ????Ÿ?? 0????Ʈ??.

	case AI_PKCS_PADDING:
		dPadLen = pbOutBuffer[dBlockLen - 1];
		if (((int) dPadLen <= 0) || (dPadLen > (int) dBlockLen))
			return CTR_PAD_CHECK_ERROR;
		for (i = 1; i <= dPadLen; i++)
			if (pbOutBuffer[dBlockLen - i] != dPadLen)
				return CTR_PAD_CHECK_ERROR;
		return dPadLen;

	default:
		return CTR_FATAL_ERROR;
	}
}

static RET_VAL CBC_DecUpdate(SEED_ALG_INFO *AlgInfo, BYTE *CipherTxt,
		DWORD CipherTxtLen, BYTE *PlainTxt, DWORD *PlainTxtLen) {
	int i;
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;

	//	Check Output Memory Size
	*PlainTxtLen = BufLen + CipherTxtLen;

	//	No one block
	if (BufLen + CipherTxtLen <= BlockLen) {
//		memcpy(AlgInfo->Buffer+BufLen, CipherTxt, (int)CipherTxtLen);
		for (i = 0; i < (int) CipherTxtLen; i++) {
			AlgInfo->Buffer[BufLen + i] = CipherTxt[i];
		}
		AlgInfo->BufLen += CipherTxtLen;
		*PlainTxtLen = 0;
		return CTR_SUCCESS;
	}

	//	control the case that CipherTxt and PlainTxt are the same buffer
	if (CipherTxt == PlainTxt)
		return CTR_FATAL_ERROR;

	//	first block
	*PlainTxtLen = BufLen + CipherTxtLen;
//	memcpy(AlgInfo->Buffer+BufLen, CipherTxt, (int)(BlockLen - BufLen));
	for (i = 0; i < (int) (BlockLen - BufLen); i++) {
		AlgInfo->Buffer[BufLen + i] = CipherTxt[i];
	}
	CipherTxt += BlockLen - BufLen;
	CipherTxtLen -= BlockLen - BufLen;

	//	core part
	BlockCopy(PlainTxt, AlgInfo->Buffer);
	SEED_Decrypt(ScheduledKey, PlainTxt);
	BlockXor(PlainTxt, PlainTxt, AlgInfo->ChainVar);
	PlainTxt += BlockLen;
	if (CipherTxtLen > BlockLen) {
		BlockCopy(PlainTxt, CipherTxt);
		SEED_Decrypt(ScheduledKey, PlainTxt);
		BlockXor(PlainTxt, PlainTxt, AlgInfo->Buffer);
		CipherTxt += BlockLen;
		PlainTxt += BlockLen;
		CipherTxtLen -= BlockLen;
	}
	while (CipherTxtLen > BlockLen) {
		BlockCopy(PlainTxt, CipherTxt);
		SEED_Decrypt(ScheduledKey, PlainTxt);
		BlockXor(PlainTxt, PlainTxt, CipherTxt-BlockLen);
		CipherTxt += BlockLen;
		PlainTxt += BlockLen;
		CipherTxtLen -= BlockLen;
	}
	BlockCopy(AlgInfo->ChainVar, CipherTxt-BlockLen);

	//	save remained data
//	memcpy(AlgInfo->Buffer, CipherTxt, (int)CipherTxtLen);
	for (i = 0; i < (int) (CipherTxtLen); i++) {
		AlgInfo->Buffer[i] = CipherTxt[i];
	}
	AlgInfo->BufLen = (AlgInfo->BufLen & 0xF0000000) + CipherTxtLen;
	*PlainTxtLen -= CipherTxtLen;

	//
	return CTR_SUCCESS;
}

static RET_VAL CBC_DecFinal(SEED_ALG_INFO *AlgInfo, BYTE *PlainTxt,
		DWORD *PlainTxtLen) {
	DWORD *ScheduledKey = AlgInfo->RoundKey;
	DWORD BlockLen = SEED_BLOCK_LEN, BufLen = AlgInfo->BufLen;
	RET_VAL ret;

	//	Check Output Memory Size
	if (BufLen == 0) {
		*PlainTxtLen = 0;
		return CTR_SUCCESS;
	}
	*PlainTxtLen = BlockLen;

	if (BufLen != BlockLen)
		return CTR_CIPHER_LEN_ERROR;

	//	core part
	BlockCopy(PlainTxt, AlgInfo->Buffer);
	SEED_Decrypt(ScheduledKey, PlainTxt);
	BlockXor(PlainTxt, PlainTxt, AlgInfo->ChainVar);
	BlockCopy(AlgInfo->ChainVar, AlgInfo->Buffer);

	//	Padding Check
	ret = PaddCheck(PlainTxt, BlockLen, AlgInfo->PadType);
	if (ret == (DWORD) -3)
		return CTR_PAD_CHECK_ERROR;
	if (ret == (DWORD) -1)
		return CTR_FATAL_ERROR;

	*PlainTxtLen = BlockLen - ret;

	//
	return CTR_SUCCESS;
}


static RET_VAL SEED_DecFinal(SEED_ALG_INFO *AlgInfo, BYTE *PlainTxt,
		DWORD *PlainTxtLen) {
	switch (AlgInfo->ModeID) {
	case AI_CBC:
		return CBC_DecFinal(AlgInfo, PlainTxt, PlainTxtLen);
	default:
		return CTR_FATAL_ERROR;
	}
}

static RET_VAL SEED_DecInit(SEED_ALG_INFO *AlgInfo) {
	AlgInfo->BufLen = 0;
	if (AlgInfo->ModeID != AI_ECB) {
//		memcpy(AlgInfo->ChainVar, AlgInfo->IV, SEED_BLOCK_LEN);
		int i;
		for (i = 0; i < SEED_BLOCK_LEN; i++) {
			AlgInfo->ChainVar[i] = AlgInfo->IV[i];
		}
	}
	return CTR_SUCCESS;
}

static int SeedExecuteEX(unsigned char *UserKey, unsigned char *IV,
		DWORD EncType, DWORD ModeType, DWORD PadType,
		BYTE* lpSrc, DWORD *SLen, BYTE* lpDst, DWORD *DLen) {
		
	int i;

	DWORD seed_buffer_length;
	BYTE* SrcData;
	BYTE* DstData;

	DWORD UKLen = 16, IVLen = 16, SrcLen = 0, DstLen = 0;
	RET_VAL ret;
	SEED_ALG_INFO AlgInfo;

	DWORD dst_buffer_size;
	//modified_mspark
	SrcData = NULL;
	DstData = NULL;

	seed_buffer_length = (*SLen) + 64;
	dst_buffer_size = (*DLen);

	SrcData = malloc(seed_buffer_length);
	if (SrcData == NULL)
		return NULL_PTR_ERROR;
//	memset( SrcData, 0x00, seed_buffer_length );
	for (i = 0; i < seed_buffer_length; i++) {
		SrcData[i] = 0;
	}

	DstData = malloc(seed_buffer_length);
	if (DstData == NULL) {
		free(SrcData);
		return NULL_PTR_ERROR;
	}
//	memset( DstData, 0x00, seed_buffer_length );
	for (i = 0; i < seed_buffer_length; i++) {
		DstData[i] = 0;
	}

	(*DLen) = 0;
	//
	SEED_SetAlgInfo(ModeType, PadType, IV, &AlgInfo);
	ret = SEED_KeySchedule(UserKey, UKLen, &AlgInfo);
	if (ret != CTR_SUCCESS) {
		if (SrcData)
			free(SrcData);
		if (DstData)
			free(DstData);
		return ret;
	}

	if (EncType == 0)	//	Encryption
			{
		ret = SEED_EncInit(&AlgInfo);
		if (ret != CTR_SUCCESS) {
			if (SrcData)
				free(SrcData);
			if (DstData)
				free(DstData);
			return ret;
		}

		{
			SrcLen = (*SLen);
//			memcpy(SrcData, lpSrc, (*SLen));
			for (i = 0; i < (int) ((*SLen)); i++) {
				SrcData[i] = lpSrc[i];
			}

			DstLen = seed_buffer_length;
			ret = SEED_EncUpdate(&AlgInfo, SrcData, SrcLen, DstData, &DstLen);
			if (ret != CTR_SUCCESS) {
				//Error(ret, "SEED_EncUpdate() returns.");
				if (SrcData)
					free(SrcData);
				if (DstData)
					free(DstData);
				return ret;
			}
//			memcpy(lpDst, DstData, DstLen);
			for (i = 0; i < (int) (DstLen); i++) {
				lpDst[i] = DstData[i];
			}
		}

		(*DLen) += DstLen;

		DstLen = seed_buffer_length;
		ret = SEED_EncFinal(&AlgInfo, DstData, &DstLen);
		if (ret != CTR_SUCCESS) {
			//Error(ret, "SEED_EncFinal() returns.");
			if (SrcData)
				free(SrcData);
			if (DstData)
				free(DstData);
			return ret;
		}

		if (dst_buffer_size < DstLen) {
			if (SrcData)
				free(SrcData);
			if (DstData)
				free(DstData);
			return CTR_FATAL_ERROR;
		}

//		memcpy(lpDst+(*DLen), DstData, DstLen);
		for (i = 0; i < (int) (DstLen); i++) {
			lpDst[i + (*DLen)] = DstData[i];
		}
		(*DLen) += DstLen;
	} else {					//	Decryption
		ret = SEED_DecInit(&AlgInfo);
		if (ret != CTR_SUCCESS) {
			return ret;
		}
		{
			SrcLen = *SLen;
//			memcpy(SrcData, lpSrc, *SLen);
			for (i = 0; i < (int) ((*SLen)); i++) {
				SrcData[i] = lpSrc[i];
			}

			DstLen = seed_buffer_length;
			ret = SEED_DecUpdate(&AlgInfo, SrcData, SrcLen, DstData, &DstLen);
			if (ret != CTR_SUCCESS) {
				//Error(ret, "SEED_DecUpdate() returns.");
				if (SrcData)
					free(SrcData);
				if (DstData)
					free(DstData);
				return ret;
			}

//			memcpy(lpDst, DstData, DstLen);
			for (i = 0; i < (int) (DstLen); i++) {
				lpDst[i] = DstData[i];
			}
		}
		(*DLen) += DstLen;
		DstLen = seed_buffer_length;
		ret = SEED_DecFinal(&AlgInfo, DstData, &DstLen);
		if (ret != CTR_SUCCESS) {
			//Error(ret, "SEED_DecFinal() returns.");
			if (SrcData)
				free(SrcData);
			if (DstData)
				free(DstData);
			return ret;
		}

		if (dst_buffer_size < DstLen) {
			if (SrcData)
				free(SrcData);
			if (DstData)
				free(DstData);
			return CTR_FATAL_ERROR;
		}

//		memcpy(lpDst+(*DLen), DstData, DstLen);
		for (i = 0; i < (int) (DstLen); i++) {
			lpDst[i + (*DLen)] = DstData[i];
		}
		(*DLen) += DstLen;
	}
	if (SrcData)
		free(SrcData);
	if (DstData)
		free(DstData);
	return CTR_SUCCESS;
}

static int SeedExecute(DWORD EncType, DWORD ModeType, DWORD PadType,
		BYTE* lpSrc, DWORD *SLen, BYTE* lpDst, DWORD *DLen) {

	unsigned char _def_UserKey[SEED_USER_KEY_LEN] = { 0x10, 0x31, 0x45, 0x67, 0x89,
			0xAB, 0xCD, 0xE0, 0xCE, 0x34, 0x56, 0x68, 0x93, 0xBC, 0xDE, 0x07 };

	unsigned char _def_IV[SEED_BLOCK_LEN] = { 0x4E, 0xEC, 0xAA, 0x88, 0x66, 0x44,
			0x22, 0x11, 0xDE, 0xEC, 0xAA, 0x98, 0x76, 0x54, 0x32, 0x11 };
	
	return SeedExecuteEX( _def_UserKey,_def_IV,EncType,ModeType,PadType,lpSrc,SLen,lpDst,DLen);
	
}

static void SEED_SetAlgInfo(DWORD ModeID, DWORD PadType, BYTE *IV,
		SEED_ALG_INFO *AlgInfo) {
	int i;
	AlgInfo->ModeID = ModeID;
	AlgInfo->PadType = PadType;

	if (IV != NULL) {
//		memcpy(AlgInfo->IV, IV, SEED_BLOCK_LEN);
		for (i = 0; i < (int) (SEED_BLOCK_LEN); i++) {
			AlgInfo->IV[i] = IV[i];
		}
	} else {
//		memset(AlgInfo->IV, 0, SEED_BLOCK_LEN);
		for (i = 0; i < (int) (SEED_BLOCK_LEN); i++) {
			AlgInfo->IV[i] = 0;
		}
	}
}

static RET_VAL SEED_KeySchedule(BYTE *UserKey,		//	?????? ????Ű ?Է?
		DWORD UserKeyLen,		//	?????? ????Ű?? ????Ʈ ??
		SEED_ALG_INFO *AlgInfo)	//	??ȣ??/??ȣ?? Round Key ????/????
{
	DWORD A, B, C, D, T0, T1, *K = AlgInfo->RoundKey;

	////
	if (UserKeyLen != SEED_USER_KEY_LEN)
		return CTR_INVALID_USERKEYLEN;

	////
	BIG_B2D( &(UserKey[0]), A);
	BIG_B2D( &(UserKey[4]), B);
	BIG_B2D( &(UserKey[8]), C);
	BIG_B2D( &(UserKey[12]), D);

	T0 = A + C - KC0;
	T1 = B - D + KC0;
	K[0] = SEED_SL[0][(T0) & 0xFF] ^ SEED_SL[1][(T0 >> 8) & 0xFF]
			^ SEED_SL[2][(T0 >> 16) & 0xFF] ^ SEED_SL[3][(T0 >> 24) & 0xFF];
	K[1] = SEED_SL[0][(T1) & 0xFF] ^ SEED_SL[1][(T1 >> 8) & 0xFF]
			^ SEED_SL[2][(T1 >> 16) & 0xFF] ^ SEED_SL[3][(T1 >> 24) & 0xFF];
	;

	EncRoundKeyUpdate0(K+ 2, A, B, C, D, KC1);
	EncRoundKeyUpdate1(K+ 4, A, B, C, D, KC2);
	EncRoundKeyUpdate0(K+ 6, A, B, C, D, KC3);
	EncRoundKeyUpdate1(K+ 8, A, B, C, D, KC4);
	EncRoundKeyUpdate0(K+10, A, B, C, D, KC5);
	EncRoundKeyUpdate1(K+12, A, B, C, D, KC6);
	EncRoundKeyUpdate0(K+14, A, B, C, D, KC7);
	EncRoundKeyUpdate1(K+16, A, B, C, D, KC8);
	EncRoundKeyUpdate0(K+18, A, B, C, D, KC9);
	EncRoundKeyUpdate1(K+20, A, B, C, D, KC10);
	EncRoundKeyUpdate0(K+22, A, B, C, D, KC11);
	EncRoundKeyUpdate1(K+24, A, B, C, D, KC12);
	EncRoundKeyUpdate0(K+26, A, B, C, D, KC13);
	EncRoundKeyUpdate1(K+28, A, B, C, D, KC14);
	EncRoundKeyUpdate0(K+30, A, B, C, D, KC15);
	//	Remove sensitive data
	A = B = C = D = T0 = T1 = 0;
	K = NULL;
	//
	return CTR_SUCCESS;
}

static int EncodeData(BYTE* lpDst, DWORD *DLen, BYTE* lpSrc, DWORD *SLen) {
	if (!lpDst || (*DLen == 0) || !lpSrc || (*SLen == 0))
		return NULL_PTR_ERROR;

	return SeedExecute(ENC_TYPE, AI_CBC, AI_PKCS_PADDING, lpSrc, SLen, lpDst,
			DLen);
}

static int DecodeData(BYTE* lpDst, DWORD *DLen, BYTE* lpSrc, DWORD *SLen) {
	if (!lpDst || (*DLen == 0) || !lpSrc || (*SLen == 0))
		return NULL_PTR_ERROR;

	return SeedExecute(DEC_TYPE, AI_CBC, AI_PKCS_PADDING, lpSrc, SLen, lpDst,
			DLen);
}

#endif // ENABLE_USER_PASSWORD_ENCODING

/*
1) ?????? ?? ?̷? ��?? ???ڵ?�� ???????? ?Ѵ?.
	??Ÿ?ӿ??? ?̷? ???ڵ? ???? ?ٷ? ?????Ѵ?.

gchar*      g_strescape  (const gchar *source,
                          const gchar *exceptions);

Escapes the special characters '\b', '\f', '\n', '\r', '\t', '\' and '"' in the
string source by inserting a '\' before them. Additionally all characters in the
range 0x01-0x1F (everything below SPACE) and in the range 0x7F-0xFF (all
non-ASCII chars) are replaced with a '\' followed by their octal representation.
Characters supplied in exceptions are not escaped.

source : a string to escape.
exceptions : a string of characters not to escape in source.
Returns : a newly-allocated copy of source with certain characters escaped. See above.



gchar*      g_strcompress (const gchar *source);

Replaces all escaped characters with their one byte equivalent. It does the
reverse conversion of g_strescape().

source : a string to compress.
Returns : a newly-allocated copy of source with all escaped character compressed.


2) sysdb ???? ��???? sysdb.conf???? ??????, ??��?? ?????? ?????ʹ? ?? ?׷�??
	???? ????�� ?????Ѵ?.

	nf_sysdb_sys.conf
	nf_sysdb_net.conf
	nf_sysdb_audio.conf
	nf_sysdb_disk.conf
	nf_sysdb_cam.conf
	nf_sysdb_usr.conf
	nf_sysdb_alarm.conf
	nf_sysdb_act.conf
	nf_sysdb_disp.conf
	nf_sysdb_rec.conf

	nf_sysdb_save( char *category );
	{
		list = nf_sysdb_db_list_property
		list ?? free?????? ??.
	}

	nf_sysdb_load( char *category );
	{
		?ش? ��???? ???ʷ? ?ε?..
	}

3) strtol example /// strtoul, strtod

http://www.cplusplus.com/reference/clibrary/cstdlib/strtol.html

#include <stdio.h>
#include <stdlib.h>

int main ()
{
  char szNumbers[] = "2001 60c0c0 -1101110100110100100000 0x6fffff";
  char * pEnd;
  long int li1, li2, li3, li4;
  li1 = strtol (szNumbers,&pEnd,10);
  li2 = strtol (pEnd,&pEnd,16);
  li3 = strtol (pEnd,&pEnd,2);
  li4 = strtol (pEnd,NULL,0);
  printf ("The decimal equivalents are: %ld, %ld, %ld and %ld.\n", li1, li2, li3, li4);
  return 0;
}

Output:
The decimal equivalents are: 2001, 6340800, -3624224 and 7340031


4) sysdb.conf   attribute def?? ??��.
5) ???? ?ʱ?ȭ ??�� ?ε??ϴ? ?????? ?ʿ???.
6) sysdb version, crc üũ ?ʿ?
7) ?ۿ??? ??��?? ??ģ????~???

00e04c4d00c4

0x00 0xe0 0x4c 0x4d 

*/
#include "curl/curl.h"
#include <curl/easy.h>


#define LIC_CURL_RET_MAX_DATA_SIZE	(32*1024)

#define LIC_EVENT_SERVER_ID				"1nvm5kb3nx9r0ykv"
#define LIC_EVENT_SERVER_PASSWORD		"YBXcR7mg4FwD!u*A"
#define LIC_EVENT_SERVER_HOST_REQ_URL	"https://license.sequrinet.com:8443/api/devices/licenses/?mac="

/*
curl -u 1nvm5kb3nx9r0ykv:YBXcR7mg4FwD\!u*A --digest https://license.sequrinet.com:8443/api/devices/licenses/?mac=00115f000000,00115f000001,00115f000003 -k
*/

typedef struct _LIC_CURL_RET_T {
	unsigned int data_len;			
	char data[LIC_CURL_RET_MAX_DATA_SIZE];		
} LIC_CURL_RET;
                          

#ifdef LIC_DEBUG
static
void _curl_my_dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size)
{
  size_t i;
  size_t c;
  unsigned int width=0x10;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i=0; i<size; i+= width) {
    fprintf(stream, "%4.4lx: ", (long)i);
 
    /* show hex to the left */
    for(c = 0; c < width; c++) {
      if(i+c < size)
        fprintf(stream, "%02x ", ptr[i+c]);
      else
        fputs("   ", stream);
    }
 
    /* show data on the right */
    for(c = 0; (c < width) && (i+c < size); c++) {
      char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x80) ? ptr[i+c] : '.';
      fputc(x, stream);
    }
 
    fputc('\n', stream); /* newline */
  }
}
 
static
int _curl_my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  const char *text;
  (void)handle; /* prevent compiler warning */
  (void)userp;
 
  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }
 
  _curl_my_dump(text, stderr, (unsigned char *)data, size);
  return 0;
}
#endif 


static size_t _lic_request_write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
{
      
    int req_size = (size*nmemb);
    int writeble_size = 0;
    
    LIC_CURL_RET *curl_ret = (LIC_CURL_RET *)stream;
    
    if( curl_ret->data_len + req_size <= LIC_CURL_RET_MAX_DATA_SIZE){
    	writeble_size = req_size;
    }else {
    	writeble_size = curl_ret->data_len + req_size - LIC_CURL_RET_MAX_DATA_SIZE;
    }
    	   
   if( writeble_size >0 )  {
    	memcpy( &curl_ret->data[curl_ret->data_len] , ptr, writeble_size); 
    	curl_ret->data_len += writeble_size;    	    	
    } 
    
    //g_message("%s dump[%*.*s]", __FUNCTION__, req_size,req_size,ptr );
    
    return writeble_size;
}


static int _lic_request_to_server(char *req_string, LIC_CURL_RET *curl_ret)
{
    CURL *curl = NULL;	
		
	int ret = -1;	
    curl = curl_easy_init();
#if 0	
	g_message("%s req_url[%s]",__FUNCTION__, req_string);	
#endif		
    if (curl) {

	    CURLcode res;			

		struct curl_httppost *form_post = NULL;
		struct curl_httppost *last_ptr = NULL;	    
  		
		char header[256];	
	
		gchar err_str[CURL_ERROR_SIZE];

        curl_easy_setopt(curl, CURLOPT_URL, req_string);

		//memset( curl_ret, 0x00, sizeof(curl_ret));
		
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _lic_request_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_ret);

		curl_easy_setopt(curl, CURLOPT_USERNAME, LIC_EVENT_SERVER_ID);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, LIC_EVENT_SERVER_PASSWORD);
		
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);	

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);				

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);

#ifdef LIC_DEBUGx
		//curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, _curl_my_dump);
	    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif	    
		curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, 1024*1024);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);			

/*
CURLFORM_BUFFER
	CURLFORM_FILE �� ???????? ?ʰ? ?????? ��?? ???? ???ε? ?κп? ???? ?˴ϴ? . 
	libcurl?? ???? ?????? ?̹? ???ۿ? ??���� ?˸??ϴ?. ?Ű? ?????? ?????? ?????? 
	???? ?̸? ?ʵ? ?? ��???ϴ? ???ڿ??Դϴ? .

CURLFORM_BUFFERPTR
	CURLFORM_BUFFER ?? ?Բ? ?????˴ϴ? . 
	?Ű? ?????? ???ε? ?? ???ۿ? ???? ???????Դϴ?. 
	?? ???۴?, curl_easy_cleanup ?? ?ҷ? ?? ?????? ???????? ?ʽ��ϴ? . 
	???? ?????? ????Ʈ ???? ??�� ?Ϸ��? CURLFORM_BUFFERLENGTH ?? ?????ؾ??մϴ? .

CURLFORM_BUFFERLENGTH
	CURLFORM_BUFFER ?? ?Բ? ?????˴ϴ? . 
	?Ű? ?????? ?????? ???̸? ��???ϴ? long ???Դϴ?.
*/

#if 0
		if( file_path )
		{			
  			curl_formadd(&form_post, &last_ptr,
				CURLFORM_COPYNAME, "IMAGE",
				CURLFORM_FILE, file_path ,
				CURLFORM_FILENAME, "image.jpg",
				CURLFORM_CONTENTTYPE, "image/jpeg",
				CURLFORM_END);
								
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, form_post);
		}
#endif

        res = curl_easy_perform(curl);
		ret = res;
		
		curl_easy_cleanup(curl);
		
		if(form_post) {
			 curl_formfree(form_post);
		}
    }
	else{
		g_warning("%s - curl init error", __FUNCTION__);
	}	
	
	return ret;
}


#define uint8_t		unsigned char
#define uint16_t	unsigned short

static int _base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize);
static int _base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize);
static uint16_t  _crc16(uint16_t crc, const void *buf, size_t size);

static uint16_t crc16_tab[256] = {
	0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
	0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
	0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
	0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
	0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
	0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
	0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
	0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
	0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
	0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
	0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
	0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
	0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
	0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
	0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
	0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
	0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
	0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
	0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
	0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
	0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
	0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
	0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
	0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
	0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
	0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
	0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
	0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
	0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
	0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
	0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
	0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};


static uint16_t  _crc16(uint16_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = buf;

	while (size--)
		crc = crc16_tab[(crc ^ (*p++)) & 0xFF] ^ (crc >> 8);

    return crc;
}


int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize) {
  int buffer = 0;
  int bitsLeft = 0;
  int count = 0;

  const uint8_t *ptr = encoded;
	
  for (; count < bufSize && *ptr; ++ptr) {
    uint8_t ch = *ptr;
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
      continue;
    }
    buffer <<= 5;

    // Deal with commonly mistyped characters
    if (ch == '0') {
      ch = 'O';
    } else if (ch == '1') {
      ch = 'L';
    } else if (ch == '8') {
      ch = 'B';
    }

    // Look up one base32 digit
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      ch = (ch & 0x1F) - 1;
    } else if (ch >= '2' && ch <= '7') {
      ch -= '2' - 26;
    } else {
      return -1;
    }

    buffer |= ch;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      result[count++] = buffer >> (bitsLeft - 8);
      bitsLeft -= 8;
    }
  }
  if (count < bufSize) {
    result[count] = '\000';
  }
  return count;
}

int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize) {
  if (length < 0 || length > (1 << 28)) {
    return -1;
  }
  int count = 0;
  if (length > 0) {
    int buffer = data[0];
    int next = 1;
    int bitsLeft = 8;
    while (count < bufSize && (bitsLeft > 0 || next < length)) {
      if (bitsLeft < 5) {
        if (next < length) {
          buffer <<= 8;
          buffer |= data[next++] & 0xFF;
          bitsLeft += 8;
        } else {
          int pad = 5 - bitsLeft;
          buffer <<= pad;
          bitsLeft += pad;
        }
      }
      int index = 0x1F & (buffer >> (bitsLeft - 5));
      bitsLeft -= 5;
      result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
    }
  }
  if (count < bufSize) {
    result[count] = '\000';
  }
  return count;
}

//#define LIC_DEBUG
//#define ENABLE_LIC_ENCODING
#include "nf_util_netif.h"

gboolean nf_sysdb_license_set_key( NF_SYSDB_LICENSE_SECRET_KEY *out_key)
{

	NF_NETIF_MAC mac;				
	int len = 0;
	char buff[32];				

	g_return_if_fail( out_key != NULL);

	memset ( out_key, 0x00, sizeof(NF_SYSDB_LICENSE_SECRET_KEY));
	memset( &mac, 0x00, sizeof(NF_NETIF_MAC));
		
	nf_netif_get_mac( &mac );			
	memcpy( out_key->mac_addr, mac.mac_addr, NF_SYSDB_LICENSE_MAC_LEN);		

#ifdef LIC_DEBUG
	snprintf(buff, sizeof(buff), "%05d", 0 );	
#else 
	snprintf(buff, sizeof(buff), "%05d", nf_sysman_get_fwver_product() );		
#endif 		
	len = strnlen(buff, NF_SYSDB_LICENSE_MODEL_LEN);
	if(len) memcpy( out_key->model, buff, len);

#ifdef LIC_DEBUGx
	nf_debug_hexdump( out_key, sizeof( NF_SYSDB_LICENSE_SECRET_KEY ) );
#endif 
				
	return 1;	
}

// string to raw 
// OK74A-AEWSX-LVK2A-N6NQE-FA35E-IAAAA
// -> OK74AAEWSXLVK2AN6NQEFA35EIAAAA
//    012345678901234567890123456780

static void _lic_skey_to_rkey( char *in, char *out, int out_len)
{
	int in_len = 0;
	
	g_return_if_fail( in != NULL);
	g_return_if_fail( out != NULL);	
	
	in_len = strnlen(in, NF_SYSDB_LICENSE_KEY_STR_LEN+1);
	// g_message("in_len[%d]",in_len);	
	g_return_if_fail( in_len == NF_SYSDB_LICENSE_KEY_STR_LEN );

	// OK74A-AEWSX-LVK2A-N6NQE-FA35E-IAAAA
	// 012345678901234567890123456789012345	
	
	snprintf(out, out_len, "%5.5s%5.5s%5.5s%5.5s%5.5s%5.5s",
				&in[0],	&in[6],	&in[12], &in[18], &in[24], &in[30] );
	
}

static void _lic_rkey_to_skey( char *in, char *out, int out_len)
{
	int in_len = 0;
	g_return_if_fail( in != NULL);
	g_return_if_fail( out != NULL);	
	
	in_len = strnlen(in, NF_SYSDB_LICENSE_KEY_RAW_LEN+1);
	g_return_if_fail( in_len  == NF_SYSDB_LICENSE_KEY_RAW_LEN );	 

	// OK74A AEWSX LVK2A N6NQE FA35E IAAAA
	// 01234 56789 01234 56789 01234 56789
	
	snprintf(out, out_len, "%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s",
				&in[0],	&in[5],	&in[10], &in[15], &in[20], &in[25] );
	
}

// int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize)
// int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize)

// raw_key only
static int _lic_check_crc(char *in)
{		
	int in_len = 0;
	
	char enc_buff[64];
	char crc_buff[64];
	
	uint16_t crc_value = 0;
		
	in_len = strnlen(in, NF_SYSDB_LICENSE_KEY_RAW_LEN+1);	
	if( in_len != NF_SYSDB_LICENSE_KEY_RAW_LEN) return -1;	
	
	// _lic_skey_to_rkey ( in, rkey, sizeof(rkey) );		
	memset( enc_buff, 0x00, sizeof( enc_buff ));
	memset( crc_buff, 0x00, sizeof( crc_buff ));
	
	base32_decode( in, enc_buff,  NF_SYSDB_LICENSE_KEY_RAW_LEN );				
	crc_value = _crc16( 0 , enc_buff, 16);
	base32_encode( &crc_value, 2, crc_buff, sizeof( crc_buff ));

#ifdef LIC_DEBUG
	// https://www.lammertbies.nl/comm/info/crc-calculation.html
	//nf_debug_hexdump( enc_buff, 16 );		
	g_message("in crc_value [0x%04x]/[%4.4s] [%4.4s]", crc_value, crc_buff, &in[26]);
#endif 
	
	return memcmp( &in[26], crc_buff, 4) == 0 ? 0:-1;
}


// 0  ok
// -1 wrong format
// -2 wrong model 
// -3 wrong mac_address
// -4 null  param
// -5 wrong lic
gint nf_sysdb_license_decoding( NF_SYSDB_LICENSE_SECRET_KEY *secret_key,
									char *license_key,	NF_SYSDB_LICENSE_INFO *out_key )
{
	unsigned char UserKey[SEED_USER_KEY_LEN] = { 
				0xaa,0xec,0x82,0xb0,
				0x7d,0xe5,0xdd,0x43,
				0x46,0x93,0xd0,0x7c,
				0xc2,0x4c,0x0d,0xa5 };
				
	unsigned char IV[SEED_BLOCK_LEN] = { 0x00, };

	unsigned char tmp_rawkey[64];	
	unsigned char tmp_binkey[64];		
		
	uint16_t crc_value = 0;
	
	NF_SYSDB_LICENSE_SECRET_KEY local_key;
		
	g_return_val_if_fail( license_key != NULL, -4);
	g_return_val_if_fail( out_key != NULL, -4);

#ifdef LIC_DEBUG
	g_message("%s in key[%35.35s]",__FUNCTION__, license_key);
#endif
	
	_lic_skey_to_rkey( license_key, tmp_rawkey, sizeof(tmp_rawkey));
	if( _lic_check_crc( tmp_rawkey ) != 0 )
	{	
		return -1;
	}
		
	
	memset( tmp_binkey, 0x00, sizeof(tmp_binkey));	
	base32_decode( tmp_rawkey, tmp_binkey, 16 );

#ifdef LIC_DEBUGx
	g_message("%s bin key",__FUNCTION__);
	nf_debug_hexdump( tmp_binkey, 16 );	
#endif

	if(secret_key == NULL)
	{
		nf_sysdb_license_set_key(&local_key);
		secret_key = &local_key;		
	}

#ifdef LIC_DEBUGx
	g_message("%s iv",__FUNCTION__);
	nf_debug_hexdump( secret_key, sizeof(NF_SYSDB_LICENSE_SECRET_KEY) );
#endif

	memcpy( IV, secret_key, 16);		
	memset( out_key, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
	
	{
		int in_len = 16, out_len = 32;	
		int ret,i;
		char tmp_dec[64];	

		memset(tmp_dec, 0x00, sizeof(tmp_dec));
		
		memcpy( UserKey, &IV[2],4);
		ret = SeedExecuteEX( UserKey, IV, 
					DEC_TYPE, AI_CBC, AI_NO_PADDING,
					tmp_binkey, &in_len, tmp_dec , &out_len);								
				
#ifdef LIC_DEBUGx
		g_message("%s SeedExecuteEX dec ret[%d] len[%d][%d]",__FUNCTION__, ret, in_len, out_len );
		nf_debug_hexdump( out_key, 32 );
#endif		
		// for lic decoding test
		for(i=0;i<8;++i) {
			int c = tmp_dec[i];
			// isprint is a set of Alphanumeric characters, Punctuation characters and Space characters.
			// Punctuation characters is a set of 
			// 		! " # $ % & ' ( ) * + , - . / : ; 
			// 		< = > ? @ [ \ ] ^ _ ` { | } ~						
			if( isprint(c) || c == 0 ){

			}else{
				return -4;
			}
		}				
		memcpy(out_key, tmp_dec, sizeof (NF_SYSDB_LICENSE_INFO));
	}

	return 0;		
}

#ifdef ENABLE_LIC_ENCODING
gint nf_sysdb_license_encoding( NF_SYSDB_LICENSE_SECRET_KEY *secret_key,
									NF_SYSDB_LICENSE_INFO *lic,
									char *out_key, int out_key_len )
{
	unsigned char UserKey[SEED_USER_KEY_LEN] = { 
				0xaa,0xec,0x82,0xb0,
				0x7d,0xe5,0xdd,0x43,
				0x46,0x93,0xd0,0x7c,
				0xc2,0x4c,0x0d,0xa5 };
				
	unsigned char IV[SEED_BLOCK_LEN] = { 0x00, };

	unsigned char tmp_buff[64];		
	unsigned char tmp_base32[64];		
	uint16_t crc_value = 0;
	
	NF_SYSDB_LICENSE_SECRET_KEY local_key;
		
	g_return_val_if_fail( lic != NULL, -4);
	g_return_val_if_fail( out_key != NULL, -4);
		
	if(secret_key == NULL)
	{
		nf_sysdb_license_set_key(&local_key);
		secret_key = &local_key;
	}
	
#ifdef LIC_DEBUGx
	g_message("%s iv",__FUNCTION__);
	nf_debug_hexdump( secret_key, sizeof(NF_SYSDB_LICENSE_SECRET_KEY) );
#endif
#ifdef LIC_DEBUG
	g_message("%s lic",__FUNCTION__);
	nf_debug_hexdump( lic, sizeof(NF_SYSDB_LICENSE_INFO) );
#endif 
	
	memcpy( IV, secret_key, 16);	
	memset( tmp_buff, 0x00, sizeof(tmp_buff));
	memset( tmp_base32, 0x00, sizeof(tmp_base32));

	{					
		int in_len = 16, out_len = 32;		
		int ret;
		
		memcpy( UserKey, &IV[2],4);
		ret = SeedExecuteEX( UserKey, IV, 
					ENC_TYPE, AI_CBC, AI_NO_PADDING,
					lic->name, &in_len, tmp_buff, &out_len);								
#ifdef LIC_DEBUG			
		g_message("%s SeedExecuteEX enc ret[%d] len[%d][%d]",__FUNCTION__, ret, in_len, out_len );
		nf_debug_hexdump( tmp_buff, 32 );	
#endif
	}

#ifdef LIC_DEBUGx // for decoding test 
	{
		int in_len = 16, out_len = 32;	
		int ret;
		char tmp_dec[64];		
		
		memcpy( IV, secret_key, 16);		
		memset(tmp_dec, 0x00, sizeof(tmp_dec));		
		
		ret = SeedExecuteEX( UserKey, IV, 
					DEC_TYPE, AI_CBC, AI_NO_PADDING,
					tmp_buff, &in_len, tmp_dec, &out_len);								
				
		g_message("%s SeedExecuteEX dec ret[%d] len[%d][%d]",__FUNCTION__, ret, in_len, out_len );
		nf_debug_hexdump( tmp_dec, 32 );	
	}
#endif
												
	base32_encode( tmp_buff, 16, tmp_base32, sizeof(tmp_base32) );

#ifdef LIC_DEBUGx
	g_message("%s tmp_base32",__FUNCTION__);
	nf_debug_hexdump( tmp_base32, 30 );	
#endif 
		
	crc_value = _crc16( 0 , tmp_buff, 16);								
	base32_encode( &crc_value, 2, &tmp_base32[26], 5);
		
#ifdef LIC_DEBUGx
	g_message("%s tmp_base32 adding crc16[0x%04x]",__FUNCTION__, crc_value);
	nf_debug_hexdump( tmp_base32, 30 );	
#endif 
	
	_lic_rkey_to_skey( tmp_base32, out_key, out_key_len);																
}
#endif //ENABLE_LIC_ENCODING

// 0 ok
// -1 param error
// -2 comm error 
	
gint nf_sysdb_license_recive_from_svr( NF_SYSDB_LICENSE_SECRET_KEY *secret_key,
									NF_SYSDB_LICENSE_INFO *out_arr_keys, 
									int out_max_count)
{
	// http://222.112.8.34:8080/browse/SWIPXMFOUR-1124
	
	LIC_CURL_RET curl_ret;
	char req_string[1024];
	int ret=-1;
	
	char *key_string = NULL, *data_ptr = NULL;
	int  idx = 0;
	
	g_return_val_if_fail( secret_key != NULL, -1);
	g_return_val_if_fail( out_arr_keys != NULL, -1);

	memset( &curl_ret, 0x00, sizeof(curl_ret));
	memset( req_string, 0x00, sizeof(req_string));
	
	snprintf(req_string, sizeof(req_string),
			"%s%02x%02x%02x%02x%02x%02x",
			LIC_EVENT_SERVER_HOST_REQ_URL, 
			(unsigned char)secret_key->mac_addr[0],
			(unsigned char)secret_key->mac_addr[1],
			(unsigned char)secret_key->mac_addr[2],
			(unsigned char)secret_key->mac_addr[3],
			(unsigned char)secret_key->mac_addr[4],
			(unsigned char)secret_key->mac_addr[5] );


	ret = _lic_request_to_server( req_string, &curl_ret);	

#ifdef LIC_DEBUG
	g_message("%s req_string[%s] ret[%d]",__FUNCTION__, req_string, ret);		
	g_message("%s curl_ret[%s]",__FUNCTION__, curl_ret.data );
#endif
	
	if(ret != 0)
		return -2;
		
	memset(out_arr_keys, 0x00, sizeof(NF_SYSDB_LICENSE_INFO)*out_max_count);
	
	data_ptr = curl_ret.data;
	while( key_string = strstr(data_ptr, "key\":\"") ) {		
				
		strncpy( out_arr_keys[idx].key, key_string+6, NF_SYSDB_LICENSE_KEY_STR_LEN);
		data_ptr = key_string + 6;		

#ifdef LIC_DEBUG
		g_message("%s idx[%d] keys[%35.35s]",__FUNCTION__, idx, out_arr_keys[idx].key );
#endif
		++idx;
	}
	
	return idx;
}

	
/*

http://222.112.8.34:8080/browse/SWIPXMFOUR-1135

<?߰? DB>
Ű ???? : '<item key="sys.lic.key_count" type="UINT" min="0" max="16" val="0" />'
Ű : '<item key="sys.lic.L0~15.key"	type="STRING" min="0" max="64" val="" />'
?߱??? : '<item key="sys.lic.L0~15.acquired_date"	type="UINT" min="0" max="" val="" />'
?????? : '<item key="sys.lic.L0~15.expired_date"	type="UINT" min="0" max="" val="" />'

<item key="sys.lic.key_count" type="UINT" min="0" max="16" val="3" />
<item key="sys.lic.L0.key"	type="STRING" min="0" max="64" val="USRTL-5AU7A-ICEGK-YD42Y-IHV7C-Q2TSA" />
<item key="sys.lic.L1.key"	type="STRING" min="0" max="64" val="53RAK-T6NAB-I7P7D-5KMQZ-HUTDX-4FABA" />
<item key="sys.lic.L2.key"	type="STRING" min="0" max="64" val="4UEPF-NRHP2-5S2UD-3HCRU-QRECI-44JNQ" />
*/
/*
on success
	sysdb idx
on error
	-1 : param error
	-2 : not found
*/		
gint nf_sysdb_license_get_from_sysdb( char *lic_name, NF_SYSDB_LICENSE_INFO *out)
{
	
	int lic_max_count = 0,i;	
	char tmp_sysdb_key[128];
	char tmp_lic_key[128];
	char tmp_lic_name[64];
	
	NF_SYSDB_LICENSE_INFO  lic_info;
			
	g_return_val_if_fail( lic_name != NULL, -1);
	g_return_val_if_fail( out != NULL, -1);
	
	memset( tmp_sysdb_key, 0x00, sizeof(tmp_sysdb_key));
	memset( tmp_lic_key, 0x00, sizeof(tmp_lic_key));
	memset( tmp_lic_name, 0x00, sizeof(tmp_lic_name));	

#if _SKIP_CHECK_LICENSE
		if (nf_sysman_hotkey_is_nfs()) return 1;
#endif
		
	lic_max_count = nf_sysdb_get_uint("sys.lic.key_count");
#ifdef LIC_DEBUG
	g_message("%s lic_count[%d]",__FUNCTION__, lic_max_count);
#endif 
	if( lic_max_count == 0) {
		return -2;
	}
	
	memcpy( tmp_lic_name, lic_name, strnlen(lic_name, 8) );
			
	for(i=0; i<lic_max_count; ++i)
	{		
		snprintf( tmp_sysdb_key, sizeof(tmp_sysdb_key), "sys.lic.L%d.key",i);					
		
		memset( &lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));		
		nf_sysdb_license_decoding( NULL, nf_sysdb_get_str_nocopy(tmp_sysdb_key), &lic_info);

#ifdef LIC_DEBUG
		g_message("%s idx[%d] key[%s]",__FUNCTION__, lic_max_count, nf_sysdb_get_str_nocopy(tmp_sysdb_key) );
		nf_debug_hexdump( &lic_info, 16);
#endif

		if(memcmp(lic_info.name,tmp_lic_name,8) == 0){
			memcpy(out,  &lic_info, sizeof(NF_SYSDB_LICENSE_INFO));
			return i;
		}
	}		
			
	return -2;	
}

//#define LIC_DEBUG

void _test_lic()
{
	
#ifdef LIC_DEBUG
	NF_SYSDB_LICENSE_INFO in_lic, out_lic;
	NF_SYSDB_LICENSE_SECRET_KEY   skey;
	char out_key[64];
				
	memset ( &in_lic, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
	memset ( &out_lic, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
	memset ( &skey, 0x00, sizeof(NF_SYSDB_LICENSE_SECRET_KEY));
	
	memset ( out_key, 0x00, sizeof(out_key));

	skey.mac_addr[0] = 0x00;
	skey.mac_addr[1] = 0x11;
	skey.mac_addr[2] = 0x6f;
	skey.mac_addr[3] = 0x00;
	skey.mac_addr[4] = 0x02;
	skey.mac_addr[5] = 0x3d;
	
	snprintf(skey.model, sizeof(skey.model), "%05d", 0 );	

#ifdef ENABLE_LIC_ENCODING	
	memcpy(in_lic.name, "DLVA", 4);
	in_lic.param1 = 0x12345678;
	in_lic.param2 = 0x87654321;
	// for encoding test
	nf_sysdb_license_encoding( &skey, &in_lic, out_key, sizeof(NF_SYSDB_LICENSE_INFO) );			
	g_message("%s out_key[%35.35s]", __FUNCTION__, out_key);
#else

	snprintf(out_key, sizeof(out_key), "USRTL-5AU7A-ICEGK-YD42Y-IHV7C-Q2TSA");
	
#endif 
														
	// for decoding test
	g_message("%s decoding test [%s]",__FUNCTION__, out_key);
	nf_sysdb_license_decoding( &skey, out_key, &out_lic);
	nf_debug_hexdump( &out_lic, sizeof(NF_SYSDB_LICENSE_INFO) );

#if 1
	{		
		int ret = 0, i=0;		
		#define MAX_OUT_ARR 16		
		NF_SYSDB_LICENSE_INFO out_arr_keys[MAX_OUT_ARR];		

#if 0
		skey.mac_addr[0] = 0x00;
		skey.mac_addr[1] = 0x11;
		skey.mac_addr[2] = 0x6f;
		skey.mac_addr[3] = 0x00;
		skey.mac_addr[4] = 0x02;
		skey.mac_addr[5] = 0x3d;
#else
		skey.mac_addr[0] = 0x00;
		skey.mac_addr[1] = 0x11;
		skey.mac_addr[2] = 0x5f;
		skey.mac_addr[3] = 0x30;
		skey.mac_addr[4] = 0x04;
		skey.mac_addr[5] = 0xf5;
#endif
		snprintf(skey.model, sizeof(skey.model), "%05d", 0 );
		ret = nf_sysdb_license_recive_from_svr( &skey, out_arr_keys, MAX_OUT_ARR);
		g_message("%s nf_sysdb_license_recive_from_svr ret[%d]",__FUNCTION__, ret);
		for (; i<ret; i++)
		{
			char tmp_key[128];
			snprintf(tmp_key, sizeof(tmp_key), "%35.35s", out_arr_keys[i].key);
						
			memset( &out_lic, 0x00, sizeof(out_lic));
			nf_sysdb_license_decoding( &skey, tmp_key, &out_lic);
			
			g_message("%s idx[%d] key[%s]",__FUNCTION__, i, tmp_key);
			nf_debug_hexdump( &out_lic, 16 );
		}
	}	
#endif
	
	{
		int ret;
		memset( &out_lic, 0x00, sizeof(out_lic));
		ret = nf_sysdb_license_get_from_sysdb("VA", &out_lic);	
		g_message("%s nf_sysdb_license_get_from_sysdb ret[%d]",__FUNCTION__, ret);
		nf_debug_hexdump( &out_lic, 16 );
	}
	exit(0);	

#endif // LIC_DEBUG
		
}

/******************************************************************************/
/* end of file                                                                */
/******************************************************************************/
