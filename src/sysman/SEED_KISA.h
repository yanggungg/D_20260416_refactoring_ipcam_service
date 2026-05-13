/*******************************************************************************
*
* FILE:         SEED_KISA.h
*
* DESCRIPTION:  header file for SEED_KISA.c
*
*******************************************************************************/

#ifndef SEED_H
#define SEED_H


/******************************* Include files ********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************************* Type Definitions *****************************/

typedef unsigned int        DWORD;		// unsigned 4-byte data type
typedef unsigned short      WORD;		// unsigned 2-byte data type
typedef unsigned char       BYTE;		// unsigned 1-byte data type


/***************************** Endianness Define ******************************/
// If endianness is not defined correctly, you must modify here.
// SEED uses the Little endian as a defalut order

#define LITTLE_ENDIAN
#undef BIG_ENDIAN

/**************************** Constant Definitions ****************************/

#define NoRounds         16						// the number of rounds
#define NoRoundKeys      (NoRounds*2)			// the number of round-keys
#define SeedBlockSize    16    					// block length in bytes
#define SeedBlockLen     128   					// block length in bits
#define DVRSeedBlockSize (16*8)

/******************************** Common Macros *******************************/

// macroses for left or right rotations
//    #define ROTL(x, n)     (_lrotl((x), (n)))		// left rotation
//    #define ROTR(x, n)     (_lrotr((x), (n)))		// right rotation
#define ROTL(x, n)     (((x) << (n)) | ((x) >> (32-(n))))		// left rotation
#define ROTR(x, n)     (((x) >> (n)) | ((x) << (32-(n))))		// right rotation

// macroses for converting endianess
#define EndianChange(dwS)                       \
    ( (ROTL((dwS),  8) & (DWORD)0x00ff00ff) |   \
      (ROTL((dwS), 24) & (DWORD)0xff00ff00) )


/*************************** Function Declarations ****************************/

void SeedEncrypt(		/* encryption function */
		BYTE *pbData, 				// [in,out]	data to be encrypted
		DWORD *pdwRoundKey			// [in]			round keys for encryption
		);
    
void SeedDecrypt(		/* decryption function */
		BYTE *pbData, 				// [in,out]	data to be decrypted
		DWORD *pdwRoundKey			// [in]			round keys for decryption
		);
    
void SeedRoundKey(		/* key scheduling function */
		DWORD *pdwRoundKey, 		// [out]		round keys for encryption or decryption
		BYTE *pbUserKey				// [in]			secret user key 
		);


/*************************** END OF FILE **************************************/
#endif
