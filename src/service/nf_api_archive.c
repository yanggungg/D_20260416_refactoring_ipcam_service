#include "libsst.h"
#include "libarch.h"
#include "nf_common.h"
#include "nf_api_archive.h"
#include "nf_debug.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_arch"

#define DEBUG_ARCHIVE_FUNC_CALL
#define ENABLE_ARCHIVE_MEDIA_CHECK
//#define ENABLE_ARCHIVE_SYSCAM_NAME
#define ENABLE_ARCHIVE_FTP

static GQuark 
_nf_api_archive_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_API_ARCHIVE_ERROR 	_nf_api_archive_error_quark()

#if ( defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) )

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_type.h"
#include "hi_unf_cipher.h"
#include "hi_common.h"

#define HI_ERR_CIPHER(format, arg...)     HI_PRINT( "\033[0;1;31m" format "\033[0m", ## arg)
#define HI_INFO_CIPHER(format, arg...)    HI_PRINT( "\033[0;1;32m" format "\033[0m", ## arg)
#define TEST_END_PASS()				  	  HI_INFO_CIPHER("****************** %s test PASS !!! ******************\n", __FUNCTION__)
#define TEST_END_FAIL()				  	  HI_ERR_CIPHER("****************** %s test FAIL !!! ******************\n", __FUNCTION__)
#define TEST_RESULT_PRINT()				  { if (ret) TEST_END_FAIL(); else TEST_END_PASS();}
#define U32_TO_POINT(addr)  ((HI_VOID*)((HI_SIZE_T)(addr)))
#define POINT_TO_U32(addr)  ((HI_U32)((HI_SIZE_T)(addr)))

typedef HI_S32 (*list_func)();

/*
static HI_U8 aes_128_cbc_IV[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static HI_U8 aes_128_cbc_key[16]= {0x11, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
static HI_U8 aes_128_src_buf[16] = {0x11, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static HI_U8 aes_128_dst_buf[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
*/
static GStaticMutex _nf_cipher_mutex = G_STATIC_MUTEX_INIT;

static HI_S32 printBuffer(HI_CHAR *string, HI_U8 *pu8Input, HI_U32 u32Length)
{
    HI_U32 i = 0;

    if ( NULL != string )
    {
        printf("%s\n", string);
    }

    for ( i = 0 ; i < u32Length; i++ )
    {
        if( (i % 16 == 0) && (i != 0)) printf("\n");
        printf("0x%02x ", pu8Input[i]);
    }
    printf("\n");

    return HI_SUCCESS;
}

HI_S32 Setconfiginfo(HI_HANDLE chnHandle, HI_BOOL bKeyByCA, HI_UNF_CIPHER_ALG_E alg, HI_UNF_CIPHER_WORK_MODE_E mode, HI_UNF_CIPHER_KEY_LENGTH_E keyLen,
                                                const HI_U8 u8KeyBuf[16], const HI_U8 u8IVBuf[16])
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_UNF_CIPHER_CTRL_S CipherCtrl;

    memset(&CipherCtrl, 0, sizeof(HI_UNF_CIPHER_CTRL_S));
    CipherCtrl.enAlg = alg;
    CipherCtrl.enWorkMode = mode;
    CipherCtrl.enBitWidth = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
    CipherCtrl.enKeyLen = keyLen;
    CipherCtrl.bKeyByCA = bKeyByCA;
    if(CipherCtrl.enWorkMode != HI_UNF_CIPHER_WORK_MODE_ECB)
    {
        CipherCtrl.stChangeFlags.bit1IV = 1;  //must set for CBC , CFB mode
        memcpy(CipherCtrl.u32IV, u8IVBuf, 16);
    }

    memcpy(CipherCtrl.u32Key, u8KeyBuf, 16);

    s32Ret = HI_UNF_CIPHER_ConfigHandle(chnHandle, &CipherCtrl);
    if(HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


#define NF_CIPHER_MAX_PAYLOAD (1024*512)
#define NF_CIPHER_MIN_PAYLOAD (16)


static gboolean
nf_cipher_execute( int mode, int algo, unsigned char *key,  unsigned char *iv,  
					unsigned char *inbuf, unsigned char *outbuf, unsigned int len)
{

	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32InputAddrPhy = 0;
	HI_U32 u32OutPutAddrPhy = 0;
	HI_U32 u32Testcached = 0;
	HI_U8 *pInputAddrVir = HI_NULL;
	HI_U8 *pOutputAddrVir = HI_NULL;
	HI_HANDLE hTestchnid = 0;
	HI_UNF_CIPHER_ATTS_S stCipherAttr;

	printf("nf_cipher_execute  len %d \n",len);
	s32Ret = HI_UNF_CIPHER_Init();
	if(HI_SUCCESS != s32Ret)
	{
	    return ;
	}

	stCipherAttr.enCipherType = HI_UNF_CIPHER_TYPE_NORMAL;
	s32Ret = HI_UNF_CIPHER_CreateHandle(&hTestchnid, &stCipherAttr);
	if(HI_SUCCESS != s32Ret)
	{
	    HI_UNF_CIPHER_DeInit();
	    return ;
	}

	u32InputAddrPhy = POINT_TO_U32(HI_MMZ_New(len, 0, NULL, "CIPHER_BufIn"));
	if (0 == u32InputAddrPhy)
	{
	    HI_ERR_CIPHER("Error: Get phyaddr for input failed!\n");
	    goto __CIPHER_EXIT__;
	}
	pInputAddrVir = HI_MMZ_Map(u32InputAddrPhy, u32Testcached);
	u32OutPutAddrPhy = POINT_TO_U32(HI_MMZ_New(len, 0, NULL, "CIPHER_BufOut"));
	if (0 == u32OutPutAddrPhy)
	{
	    HI_ERR_CIPHER("Error: Get phyaddr for outPut failed!\n");
	    goto __CIPHER_EXIT__;
	}
	
	pOutputAddrVir = HI_MMZ_Map(u32OutPutAddrPhy, u32Testcached);

	/* For encrypt */
	s32Ret = Setconfiginfo(hTestchnid,
	                        HI_FALSE,
	                        HI_UNF_CIPHER_ALG_AES,
	                        HI_UNF_CIPHER_WORK_MODE_ECB,
	                        HI_UNF_CIPHER_KEY_AES_128BIT,
	                        key,
	                        iv);
	if(HI_SUCCESS != s32Ret)
	{
	    HI_ERR_CIPHER("Set config info failed.\n");
	    goto __CIPHER_EXIT__;
	}

	memset(pInputAddrVir, 0x0, len);
	memcpy(pInputAddrVir, inbuf, len);

	memset(pOutputAddrVir, 0x0, len);

	s32Ret = HI_UNF_CIPHER_Encrypt(hTestchnid, u32InputAddrPhy, u32OutPutAddrPhy, len);
	if(HI_SUCCESS != s32Ret)
	{
	    HI_ERR_CIPHER("Cipher encrypt failed.\n");
	    s32Ret = HI_FAILURE;
	    goto __CIPHER_EXIT__;
	}

	memcpy(outbuf , pOutputAddrVir , len);

	__CIPHER_EXIT__:

	if (u32InputAddrPhy> 0)
	{
	    HI_MMZ_Unmap(u32InputAddrPhy);
	    HI_MMZ_Delete(u32InputAddrPhy);
	}
	if (u32OutPutAddrPhy > 0)
	{
	    HI_MMZ_Unmap(u32OutPutAddrPhy);
	    HI_MMZ_Delete(u32OutPutAddrPhy);
	}

	HI_UNF_CIPHER_DestroyHandle(hTestchnid);
	HI_UNF_CIPHER_DeInit();

	return ;
	
}


#else

/*
	M4 16CH
*/

/*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>

// #include "hi35XX/hi_type.h"
// #include "hi35XX/hi_error_mpi.h"
// #include "hi35XX/hi_unf_cipher.h"

// #include "hi35XX/mmz-userdev.h"

#define HIAPI_RUN_RETURN(ret) \
do {\
    if ( ret != 0)\
    {\
        printf("\033[0;31m<%s>not pass at line:%d err:%x\033[0;39m\n",\
            __FUNCTION__,__LINE__,ret);\
        return FALSE;\
     }\
}while(0)

/* must load MMZ first */
static gint g_s32MmzFd = -1;
static GStaticMutex _nf_cipher_mutex = G_STATIC_MUTEX_INIT;

    
#define _CI_CHECK_MMZ_OPEN() do{\
if(g_s32MmzFd < 0) \
{\
    g_s32MmzFd = open("/dev/mmz_userdev", O_RDWR);\
}\
}while(0)

/* user can call HI_MPI_SYS_MmzAlloc to replace*/
static gint _CI_MmzAlloc(guint *pu32PhyAddr, void **ppVitAddr, const gchar *pstrMmb, const gchar *pstrZone, guint u32Len)
{
#if 0 /*hisilicon*/
    struct mmb_info mmi = {0};

    _CI_CHECK_MMZ_OPEN();

    mmi.size = u32Len;
    mmi.prot = PROT_READ | PROT_WRITE;
    mmi.flags = MAP_SHARED;
#if 1
    if (NULL != pstrMmb)
    {
        strncpy(mmi.mmb_name, pstrMmb, sizeof(mmi.mmb_name));
    }
    if (NULL != pstrZone)
    {
        strncpy(mmi.mmz_name, pstrZone, sizeof(mmi.mmz_name));
    }
#endif
    if (ioctl(g_s32MmzFd, IOC_MMB_ALLOC, &mmi) != HI_SUCCESS)
    {
    	printf("System alloc mmz memory failed!\n");
        return HI_FAILURE;
    }
    
    if (ioctl(g_s32MmzFd, IOC_MMB_USER_REMAP, &mmi) != HI_SUCCESS)
    {
    	printf("System remap mmz memory failed!\n");
        return HI_FAILURE;
    }
    
    *pu32PhyAddr = mmi.phys_addr;
    *ppVitAddr = mmi.mapped;
#endif    
    return 0;
}    

/* user can call HI_MPI_SYS_MmzFree to replace*/
static gint _CI_MmzFree(guint u32PhyAddr)
{
#if 0 /*hisilicon*/
    struct mmb_info mmi = {0};

    _CI_CHECK_MMZ_OPEN();

    mmi.phys_addr = u32PhyAddr;
    
    if (ioctl(g_s32MmzFd, IOC_MMB_USER_UNMAP, &mmi) != HI_SUCCESS)
    {
    	printf("System unmap mmz memory failed!\n");
        return HI_FAILURE;
    }
    
    if (ioctl(g_s32MmzFd, IOC_MMB_FREE, &mmi) != HI_SUCCESS)
    {
    	printf("System free mmz memory failed!\n");
        return HI_FAILURE;
    }
#endif
    return 0;
}


static gint _CI_Setconfiginfo(guint chnHandle, int algo, 
			unsigned char *key,  unsigned char *iv)
{
#if 0 /*hisilicon*/
    HI_U32 i;
    HI_U32 ckey = 'a';
    HI_UNF_CIPHER_CTRL_S CipherCtrl;

    //CipherCtrl.bKeyByCA = HI_FALSE;
    CipherCtrl.enAlg = HI_UNF_CIPHER_ALG_AES;
    CipherCtrl.enWorkMode = HI_UNF_CIPHER_WORK_MODE_ECB;
    CipherCtrl.enBitWidth = HI_UNF_CIPHER_BIT_WIDTH_8BIT;
    CipherCtrl.enKeyLen = HI_UNF_CIPHER_KEY_AES_128BIT;

	
	if( key == NULL) {
	    for (i = 0; i < 8; i++)
	    {
	        CipherCtrl.u32Key[i] = ckey++;
	    }
	}else{		
		memset(CipherCtrl.u32Key, 0x00, sizeof(CipherCtrl.u32Key));	
		memcpy(CipherCtrl.u32Key, key, strnlen(key,sizeof(CipherCtrl.u32Key) ));		
		
		//nf_debug_hexdump(CipherCtrl.u32Key, sizeof(CipherCtrl.u32Key));   		
	}
	
	if( iv == NULL) {
	    for (i = 0; i < 4; i++)
	    {
	        CipherCtrl.u32IV[i] = ckey++;
	    }
	}else{
		memset(CipherCtrl.u32IV, 0x00, sizeof(CipherCtrl.u32IV));	
		memcpy(CipherCtrl.u32IV, iv, strnlen(iv,sizeof(CipherCtrl.u32IV) ));		
		
		//nf_debug_hexdump(CipherCtrl.u32IV, sizeof(CipherCtrl.u32IV));   
	}
	
    HIAPI_RUN_RETURN(HI_UNF_CIPHER_ConfigHandle(chnHandle, &CipherCtrl));

    return HI_SUCCESS;
#endif
	return 0;
}

//#define NF_CIPHER_MAX_PAYLOAD ((1024*1024)-1)

#define NF_CIPHER_MAX_PAYLOAD (1024*512)
#define NF_CIPHER_MIN_PAYLOAD (16)

#include <openssl/aes.h>
  
static const unsigned char key16[16]= {
	0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x12
};

#define KEY_BIT 128
#define IV_SIZE 16
#define RW_SIZE 1
#define SUCC 0
#define FAIL -1

AES_KEY aes_ks3;

int nf_cipher_execute_test()
{
    int i=0;
    int len=0;
    int padding_len=0;
	unsigned char iv[IV_SIZE];
    char in[16] = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x12};
    char buf[16];

	printf("nf_cipher_execute_test !!!\n");
    memset(iv,0,sizeof(iv)); // init iv
    AES_set_encrypt_key(key16 ,KEY_BIT ,&aes_ks3);

	for(i=0; i < 1000000; i++){
    	AES_ecb_encrypt(in ,buf ,&aes_ks3 ,AES_ENCRYPT);
		in[i%16]++;
		if((i%10000)== 0)
			printf("AES_ecb_encrypt process cnt %d \n",i);
	}
	
    AES_ecb_encrypt(in ,buf ,&aes_ks3 ,AES_ENCRYPT);
//	  for (i = 0; i < 16; i++) {
//	    printf("%02x", (unsigned char)buf[i]);
//	  }

    return SUCC;
}

static gboolean
nf_cipher_execute( int mode, int algo, 
								unsigned char *key,  unsigned char *iv,  
								unsigned char *inbuf, unsigned char *outbuf, 
								unsigned int len)
{

    static guint inputAddrPhy = 0;
    static guint outPutAddrPhy = 0;

    guchar inputAddrVir[NF_CIPHER_MIN_PAYLOAD];
	guchar outputAddrVir[NF_CIPHER_MIN_PAYLOAD];

    static guint TestchnidEnc = 0;
    static guint TestchnidDec = 0;

	static int init = 0;
	int ret;

	unsigned int nleft;
	char    *in_ptr, *out_ptr;
    char buf[16];
	int i;
    	
	g_return_val_if_fail( key != NULL, 0);
	g_return_val_if_fail( inbuf != NULL, 0);
	g_return_val_if_fail( outbuf != NULL, 0);

    in_ptr = inbuf;
    out_ptr = outbuf;
    nleft = len;
	
	//g_static_mutex_lock (&_nf_cipher_mutex);
	
	g_message("%s start!! mode[%d] size[%d]",__FUNCTION__, mode, nleft);
		
	while( nleft >= NF_CIPHER_MIN_PAYLOAD ) {

		int tmp_len = (nleft > NF_CIPHER_MIN_PAYLOAD) ? NF_CIPHER_MIN_PAYLOAD : nleft;	
			
		memcpy(inputAddrVir, in_ptr, tmp_len);			

		//nf_debug_hexdump(inputAddrVir, 48);

		if( mode == 0  ) {	// Encrypt

    			AES_set_encrypt_key(key ,KEY_BIT ,&aes_ks3);
				
    			AES_ecb_encrypt(inputAddrVir ,outputAddrVir ,&aes_ks3 ,AES_ENCRYPT);
			
		} else {	// Decrypt
				
		}
		
		//nf_debug_hexdump(outputAddrVir, 48);
		
		memcpy(out_ptr, outputAddrVir, tmp_len);	
		
		nleft -= tmp_len;		
		in_ptr += tmp_len;
		out_ptr += tmp_len;
	}


	//g_static_mutex_unlock (&_nf_cipher_mutex);
	
	if(nleft >0) {
		g_message("%s ---> last nleft[%d]",__FUNCTION__, nleft);
	}
			
err_ret_with_unlock:
	
	//g_warning("%s err[%d]",__FUNCTION__, ret);	
	//g_static_mutex_unlock (&_nf_cipher_mutex);
	
	return 0;		
}

#if 0
static int _cipher_unit_test1()
{
    HI_U32 Testlen = 1024*1023;

    static HI_U32 inputAddrPhy = 0;
    static HI_U32 outPutAddrPhy = 0;
    static HI_U32 resultAddrPhy = 0;

    static HI_U8* inputAddrVir = NULL;
	static HI_U8* outputAddrVir = NULL;
    static HI_U8* resultAddrVir = NULL;

    static HI_HANDLE TestchnidEnc = 0;
    static HI_HANDLE TestchnidDec = 0;

	static int init = 0;
	
	if(!init) 
	{		
	    HIAPI_RUN_RETURN(HI_UNF_CIPHER_Open());
	    
	    HIAPI_RUN_RETURN(HI_UNF_CIPHER_CreateHandle(&TestchnidEnc));
	    HIAPI_RUN_RETURN(HI_UNF_CIPHER_CreateHandle(&TestchnidDec));	

    	HIAPI_RUN_RETURN(_CI_MmzAlloc(&inputAddrPhy,(HI_VOID **)&inputAddrVir,
    	  	"cipherIn",NULL,Testlen));
	
    	HIAPI_RUN_RETURN(_CI_MmzAlloc(&outPutAddrPhy,(HI_VOID **)&outputAddrVir,
	       	"cipherOut",NULL,Testlen));
	
    	HIAPI_RUN_RETURN(_CI_MmzAlloc(&resultAddrPhy,(HI_VOID **)&resultAddrVir,
			"cipherResult",NULL,Testlen));
			
	    init = 1;
	}
	
    memset(inputAddrVir, 'I', Testlen);
    
#if 1
    memset(outputAddrVir, 'O', 1024);
    memset(resultAddrVir, 'R', 1024);
#endif 

    HIAPI_RUN_RETURN(_CI_Setconfiginfo(TestchnidEnc, 0, NULL, NULL));
    HIAPI_RUN_RETURN(HI_UNF_CIPHER_Encrypt(TestchnidEnc, inputAddrPhy, outPutAddrPhy, Testlen));

#if 1
    HIAPI_RUN_RETURN(_CI_Setconfiginfo(TestchnidDec, 0, NULL, NULL));
    HIAPI_RUN_RETURN(HI_UNF_CIPHER_Decrypt(TestchnidDec, outPutAddrPhy, resultAddrPhy, Testlen));
#endif 

/*
	nf_debug_hexdump(inputAddrVir, 16);        
	nf_debug_hexdump(outputAddrVir, 16);        
	nf_debug_hexdump(resultAddrVir, 16);        
*/
    if (0 != memcmp(resultAddrVir, inputAddrVir, 1024))   {
        printf("encrypt and decrypt test fail!\n");
    }else{
    	printf("encrypt and decrypt test OK!\n");
    }
        
    return HI_SUCCESS;
}

#define UNIT_TEST_SIZE  (NF_CIPHER_MAX_PAYLOAD + 1024)
static int _cipher_unit_test2( int size )
{
    char inbuf[UNIT_TEST_SIZE];
    char outbuf[UNIT_TEST_SIZE];
    char retbuf[UNIT_TEST_SIZE];
    int ret = 0;

    memset(inbuf, 0x00, UNIT_TEST_SIZE);
    memset(outbuf, 0x00, UNIT_TEST_SIZE);
    memset(retbuf, 0x00, UNIT_TEST_SIZE);    
	
    memset(inbuf, 'I', size);
    memset(outbuf, 'O', size);
    memset(retbuf, 'R', size);    
    
    memcpy(inbuf, &size, 4);
    
    // Encrypt       
    nf_cipher_execute(0, 0,"0123456789abcdef","0123456789abcdef", inbuf, outbuf, size);
    
    // Decrypt
    nf_cipher_execute(1, 0,"0123456789abcdef","0123456789abcdef", outbuf, retbuf, size);

	nf_debug_hexdump(inbuf, 48);        
	nf_debug_hexdump(outbuf, 48);        
	nf_debug_hexdump(retbuf, 48);        
    
    if( memcmp( inbuf, retbuf, size) == 0)
    	g_message("cipher test ok    size[%d]", size);
    else
    	g_message("cipher test fail! size[%d]", size);
    
    return HI_SUCCESS;
}

static int _cipher_unit_test_thread_func(void *arg)
{
	int size = 0;
	while(1)
	{		
		_cipher_unit_test2( (++size%128)+ NF_CIPHER_MAX_PAYLOAD );		
		g_usleep(100000);
	}
}

#endif

#endif

/*******************************************************************************/
static int 
_is_valid_dirfilename( unsigned char uctmp )
{
	// https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words	
	if( uctmp == '/'
		||  uctmp == '\\'
		||  uctmp == '?'
		||  uctmp == '%'
		||  uctmp == '*'
		||  uctmp == ':'
		||  uctmp == '"'
		||  uctmp == '<'
		||  uctmp == '>'
		||  uctmp == '.' 
		||  uctmp < ' '  // ascii value 0~31 
		)	
		return 0;
	else 
		return 1;
}



#ifdef ENABLE_ARCHIVE_SYSCAM_NAME

static volatile int  _g_cam_changed = 1;
static volatile int  _g_sys_changed = 1;

static void
_archive_sysdb_reload_syscam_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_CAM) {	// cam.ptz.Px.*				
		++_g_cam_changed;
	}else if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS) {	// cam.ptz.Px.*			
		++_g_sys_changed;
	}
}

static void 
_archive_set_syscam_info()
{
	if(	_g_sys_changed )
	{						
		char tmp[256];
		gint len,i;
		
		memset( tmp, 0x00, sizeof(tmp));
				
		len = snprintf( tmp, sizeof(tmp)-1 , "%s", nf_sysdb_get_str_nocopy("sys.info.sysid") );

		for(i=0;i<len;++i)
			if( !_is_valid_dirfilename(tmp[i]) )
				tmp[i] = '_';
				
		arch_set_system_name( tmp );
		_g_sys_changed = 0;
	}	
	
	if( _g_cam_changed )
	{
		struct cam_name_t cam_title[32];

		gint len,i,idx;
		char tmp[128], sys[32];
		
		memset( cam_title, 0x00, sizeof(cam_title));		
		
		for(idx=0; idx < NUM_ACTIVE_CH; ++idx)
		{			
			sprintf(sys, "cam.C%d.title", idx);
			len = snprintf( tmp, sizeof(tmp)-1 , "%s", nf_sysdb_get_str_nocopy(sys) );

			for(i=0;i<len;++i)
				if( !_is_valid_dirfilename(tmp[i]) )
					tmp[i] = '_';
												
			strcpy( cam_title[idx].name, tmp);						
		}
		arch_set_cam_title( cam_title );
		_g_cam_changed = 0;		
	}
}

#endif

static void 
_archive_set_cam_title()
{
	struct cam_name_t cam_title[32];
	gint len,i,idx;
	char tmp[128], sys[32];
		
	memset( cam_title, 0x00, sizeof(cam_title));			
	for(idx=0; idx < NUM_ACTIVE_CH; ++idx)
	{			
		sprintf(sys, "cam.C%d.title", idx);
		len = snprintf( tmp, sizeof(tmp)-1 , "%s", nf_sysdb_get_str_nocopy(sys) );

		for(i=0;i<len;++i)
			if( !_is_valid_dirfilename(tmp[i]) )
				tmp[i] = '_';
											
		strcpy( cam_title[idx].name, tmp);						
	}
	arch_set_cam_title( cam_title );	
}

static void
_archive_set_mac_addr()
{

	gint len,i;
	char tmp[128], sys[32];
	
	sprintf(sys, "sys.info.mac");
	len = snprintf( tmp, sizeof(tmp)-1 , "%s", nf_sysdb_get_str_nocopy(sys) );

	for(i=0;i<len;++i)
		if( !_is_valid_dirfilename(tmp[i]) )
			tmp[i] = '_';
									
	arch_set_mac_addr( tmp );
}
#if  defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined( __DIGI_SUPPORT__)

typedef  struct ftp_driver_t  FTP_DRIVER;
static volatile int  _g_ftp_changed = 1;
static FTP_DRIVER _g_ftp_driver;

static void
_archive_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{		
	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_NET) {	// cam.ptz.Px.*	
		++_g_ftp_changed;
	}
}

static void 
_archive_set_ftp_info()
{
	if(	_g_ftp_changed )
	{		
		snprintf( _g_ftp_driver.server, sizeof(_g_ftp_driver.server)-1,
						"%s" , nf_sysdb_get_str_nocopy( "net.ftp.host") );
						
		_g_ftp_driver.port = nf_sysdb_get_uint("net.ftp.port");
		_g_ftp_driver.is_anon = nf_sysdb_get_bool("net.ftp.is_anon");
		_g_ftp_driver.is_pasv = 1;		// force set
		
		if( _g_ftp_driver.is_anon )
		{
			strcpy( _g_ftp_driver.username, "anonymous");
			strcpy( _g_ftp_driver.password, "anonymous@dvr.net");
			
		}else{
			snprintf( _g_ftp_driver.username, sizeof(_g_ftp_driver.username)-1,
							"%s" , nf_sysdb_get_str_nocopy( "net.ftp.username") );
			
			snprintf( _g_ftp_driver.password, sizeof(_g_ftp_driver.password)-1, 
							"%s" , nf_sysdb_get_str_nocopy( "net.ftp.passwd") );
		}
		
		snprintf( _g_ftp_driver.dir_name, sizeof(_g_ftp_driver.dir_name)-1,
						"%s" , nf_sysdb_get_str_nocopy( "net.ftp.dir_path") );

		_g_ftp_driver.timeout_conn_sec = 5;	// force set
		_g_ftp_driver.conn_retry_cnt = 3;	// force set
		_g_ftp_driver.timeout_rx_sec = 3;	// force set
		_g_ftp_driver.timeout_tx_sec = 3;	// force set

#if 1
		g_message("%s  server[%s] port[%d]", __FUNCTION__, _g_ftp_driver.server, _g_ftp_driver.port);
		g_message("%s  username[%s]", __FUNCTION__, _g_ftp_driver.username);
		g_message("%s  password[%s]", __FUNCTION__, _g_ftp_driver.password);
		g_message("%s  dir_name[%s]", __FUNCTION__, _g_ftp_driver.dir_name);
		
		g_message("%s  is_anon[%d]", __FUNCTION__, _g_ftp_driver.is_anon);
		g_message("%s  is_pasv[%d]", __FUNCTION__, _g_ftp_driver.is_pasv);
		
#endif	
	
		arch_set_ftp_info( &_g_ftp_driver);			
		_g_ftp_changed = 0;
	}			
}


/**
	@brief				set cipher params
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_set_cipher_param( NF_ARCH_CIPHER_PARAM *param,  GError **error )
{
	
	g_return_val_if_fail ( param != NULL, FALSE);

	arch_set_cipher_param(  param->enable, 
							param->algo, 
							param->passwd,
							param->iv );							

	return 1;						
}

/**
	@brief				set FTP inf
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_set_ftp_info( NF_FTP_CLIENT_REQ *req, GError **error )
{
	gint result = 0;


	g_return_val_if_fail (req != NULL, FALSE);
	g_return_val_if_fail ( req->server[0]  != 0 , FALSE);

	if ( !req->is_anon )
	{	
		g_return_val_if_fail ( req->user[0]  != 0 , FALSE);
//		g_return_val_if_fail ( req->passwd[0]  != 0 , FALSE);
	}

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	snprintf( _g_ftp_driver.server, sizeof(_g_ftp_driver.server)-1,	"%s" , req->server );
	_g_ftp_driver.port = req->port;

	_g_ftp_driver.is_anon = req->is_anon;
	if( _g_ftp_driver.is_anon )
	{
		strcpy( _g_ftp_driver.username, "anonymous");
		strcpy( _g_ftp_driver.password, "anonymous@dvr.net");

	}else{
		snprintf( _g_ftp_driver.username, sizeof(_g_ftp_driver.username)-1, "%s" , req->user);
		snprintf( _g_ftp_driver.password, sizeof(_g_ftp_driver.password)-1, "%s" , req->passwd );
	}

	snprintf( _g_ftp_driver.dir_name, sizeof(_g_ftp_driver.dir_name)-1,
			"%s" , nf_sysdb_get_str_nocopy( "net.ftp.dir_path") );

	_g_ftp_driver.is_pasv = 1;		// force set
	_g_ftp_driver.timeout_conn_sec = 5;	// force set
	_g_ftp_driver.conn_retry_cnt = 3;	// force set
	_g_ftp_driver.timeout_rx_sec = 3;	// force set
	_g_ftp_driver.timeout_tx_sec = 3;	// force set

#if 1
	g_message("%s  server[%s] port[%d]", __FUNCTION__, _g_ftp_driver.server, _g_ftp_driver.port);
	g_message("%s  username[%s]", __FUNCTION__, _g_ftp_driver.username);
	g_message("%s  password[%s]", __FUNCTION__, _g_ftp_driver.password);
	g_message("%s  dir_name[%s]", __FUNCTION__, _g_ftp_driver.dir_name);

	g_message("%s  is_anon[%d]", __FUNCTION__, _g_ftp_driver.is_anon);
	g_message("%s  is_pasv[%d]", __FUNCTION__, _g_ftp_driver.is_pasv);

#endif	

	arch_set_ftp_info( &_g_ftp_driver);
	return 1;
	
err_param:
	result = -ARCH_ERR_INVMODE;
	g_set_error( error, NF_API_ARCHIVE_ERROR, result, nf_arch_get_error_string(result) );
	return 0;
}
					
/*   
	@brief		 ftp connection test = creating directory, changing directory, writing ftp_test file
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_ftp_test(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = arch_ftp_test(fxn_cb, ctx_cb);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}	
	return ( result == 0 ) ? 1:0;
}
#endif


/**
	@brief				��ī�̺� �Ŵ��� ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_manager_start()
{
	gint result = 0;

#if defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined( __DIGI_SUPPORT__)
	static int _ftp_driver_init = 0;
#endif 

#ifdef ENABLE_ARCHIVE_SYSCAM_NAME
	static int _syscam_name_init = 0;
#endif

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif


#if defined(_SNF_MODEL)
  #define ARCH_SIZE_LIMIT_100GB  	100
  #define ARCH_SIZE_LIMIT_20GB  	20

  arch_set_size_limit( ARCH_SIZE_LIMIT_20GB ); // 20 GB size limit
  g_warning("%s arch_set_size_limit %d GB", __FUNCTION__, ARCH_SIZE_LIMIT_100GB );
#endif

	arch_set_player(MULTI_PLAYER);
	g_message("%s arch_set_player %d", __FUNCTION__, MULTI_PLAYER);
  
#if defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined(__DIGI_SUPPORT__)
	if( _ftp_driver_init == 0 )
	{
		gulong cb_handle;
		
		_ftp_driver_init = 1;
		
		//register sysdb reload callbock function
		cb_handle= nf_notify_connect_cb( "sysdb_change", _archive_sysdb_reload_cb_func, NULL);
		g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
		g_assert(cb_handle >0);
		
		memset( &_g_ftp_driver, 0x00, sizeof(_g_ftp_driver) );
		
		_g_ftp_changed = 1;
		arch_set_ftp_en( TRUE );
		
		_archive_set_ftp_info();				
	}			
#endif  /*__LOREX_SUPPORT__*/

#ifdef ENABLE_ARCHIVE_SYSCAM_NAME
	if( _syscam_name_init == 0 )
	{
		gulong cb_handle;

		_syscam_name_init = 1;

		cb_handle= nf_notify_connect_cb( "sysdb_change", _archive_sysdb_reload_syscam_cb_func, NULL);
		g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
		g_assert(cb_handle >0);

		_g_cam_changed = 1;
		_g_sys_changed = 1;
  
		_archive_set_syscam_info();
	}
#endif

	arch_set_dvr_type(ARCH_DVR_TYPE_IPX);

	result = arch_manager_start();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}
	
#if 0
	{		
		static int _cipher_init = 0;	
		if(_cipher_init == 0) {
			g_thread_create((GThreadFunc)_cipher_unit_test_thread_func,
						NULL, FALSE, NULL);
						
			_cipher_init = 1;
		}		
	}
#endif



	arch_set_cipher_fxn( nf_cipher_execute );	

#if 0
	arch_set_cipher_param(  /* en */ 1, 
							/* algo */ 0, 
							/* key  */ "1234567890abcdef",
							/* iv   */ "1234567890abcdef");
	g_message("%s called!! line[%d]", __FUNCTION__, __LINE__);							
#endif
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				��ī�̺� �Ŵ��� ����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_arch_manager_stop()
{
	gint result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_manager_stop();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}
	
	return ( result == 0 ) ? 1:0;
		
}



/**
	@brief				������ ����	
	@param[in]	param	�Ķ����
	@param[out]	info	������ ���(api ����ڰ� �̸� buffer �Ҵ��ؼ� ȣ��)	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_query_snap( NF_ARCH_SNAP_PARAM *param, 
						NF_ARCH_SNAP_INFO *info, GError **error)
{
	gint result = 0;
	
	g_return_val_if_fail (param != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);
		
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
	nf_debug_hexdump( param, sizeof(NF_ARCH_SNAP_PARAM));
#endif

	g_return_val_if_fail (param->ch < NUM_TOTAL_CHANNEL, FALSE);
	g_return_val_if_fail (param->image_size > 0, FALSE);
	g_return_val_if_fail (param->image != NULL, FALSE);
		
	result = arch_query_snap( param->ch,
							  GTIMEVAL_TO_GUINT64(param->snap_time),
							  param->image_size,
							  param->image,
							  param->tag,
							  param->user,
							  param->memo,
							  (arch_snap_info_t *)info ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 		

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
		
	return ( result == 0 ) ? 1:0;
}
						
/**
	@brief				AVI ����	
	@param[in]	param	�Ķ����
	@param[out]	info	������ ���(api ����ڰ� �̸� buffer �Ҵ��ؼ� ȣ��)	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_query_avi( NF_ARCH_AVI_PARAM *param, NF_ARCH_AVI_INFO *info, GError **error)
{
	guint64		ch_mask =0; 
	guchar		emask =0;
	gint 		result = 0;
	
	g_return_val_if_fail (param != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
	nf_debug_hexdump( param, sizeof(NF_ARCH_AVI_PARAM));
#endif

	ch_mask = (guint64)param->ch_mask;
	ch_mask |= ((guint64)param->audio_mask << 32);
	
	if(param->inc_log)		emask |= NF_ARCH_EMASK_LOG;
	if(param->inc_text)		emask |= NF_ARCH_EMASK_TEXT;
	if(param->inc_ri)		emask |= NF_ARCH_EMASK_RI;
	if(param->inc_codec)	emask |= NF_ARCH_EMASK_CODEC;
	if(param->inc_player)	emask |= NF_ARCH_EMASK_PLAYER;
	if(param->no_audio_text) emask |= NF_ARCH_EMASK_NO_AUDIO_TEXT;
	if(param->inc_pos_log)  emask |= NF_ARCH_EMASK_POS_LOG;
								  
	result = arch_query_avi( 0, GTIMEVAL_TO_GUINT64(param->start_time),
								GTIMEVAL_TO_GUINT64(param->end_time),
								ch_mask, emask,							  	
							  	param->tag, param->user, param->memo,
							  	(arch_avi_info_t *)info,
							  	param->cb_event,
							  	param->cb_context ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				AVI ����	
	@param[in]	param	�Ķ����
	@param[out]	info	������ ���(api ����ڰ� �̸� buffer �Ҵ��ؼ� ȣ��)	
	@param[out]	error	return location for a #GError, or %NULL
	@param[in] limit : size limit in MB
      @param[in] option : 0(nothing), 1(start time fix & calculate to limit time), 2(end time fix & calculate to limit time)
					 3(end time will be changed to size limit(start time fix)), 4(start time will be changed to size limit(end time fix))
			              2, 4 options are not yet supported.
	@param[in] whole_check : if set to one, query start time to end time without stopping at limitMb
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_query_avi_ex( NF_ARCH_AVI_PARAM *param, NF_ARCH_AVI_INFO *info, GError **error, 
		guint limitMB, guint option, guint whole_check )
{
	guint64		ch_mask =0; 
	guchar		emask =0;
	gint 		result = 0;
	
	g_return_val_if_fail (param != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
	nf_debug_hexdump( param, sizeof(NF_ARCH_AVI_PARAM));
#endif

	ch_mask = (guint64)param->ch_mask;
	ch_mask |= ((guint64)param->audio_mask << 32);
	
	if(param->inc_log)		emask |= NF_ARCH_EMASK_LOG;
	if(param->inc_text)		emask |= NF_ARCH_EMASK_TEXT;
	if(param->inc_ri)		emask |= NF_ARCH_EMASK_RI;
	if(param->inc_codec)	emask |= NF_ARCH_EMASK_CODEC;
	if(param->inc_player)	emask |= NF_ARCH_EMASK_PLAYER;
	if(param->no_audio_text) emask |= NF_ARCH_EMASK_NO_AUDIO_TEXT;
	if(param->inc_pos_log)  emask |= NF_ARCH_EMASK_POS_LOG;
								  
	result = arch_query_avi_ex( 0, GTIMEVAL_TO_GUINT64(param->start_time),
								GTIMEVAL_TO_GUINT64(param->end_time),
								ch_mask, emask,							  	
							  	param->tag, param->user, param->memo,
							  	(arch_avi_info_t *)info,
							  	param->cb_event,
							  	param->cb_context,
							  	limitMB,
							  	option,
							  	whole_check ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;
}

/**
	@brief					
	@param[in]	param	�Ķ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_start_avi( NF_ARCH_PB_AVI_PARAM *param, GError **error)
{
	guint64		ch_mask =0; 
	guchar		emask =0;
	gint 		result = 0;
	
	g_return_val_if_fail (param != NULL, FALSE);

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
	nf_debug_hexdump( param, sizeof(NF_ARCH_PB_AVI_PARAM));
#endif

	ch_mask = (guint64)param->ch_mask;
	ch_mask |= ((guint64)param->audio_mask << 32);
	
	if(param->inc_log)		emask |= NF_ARCH_EMASK_LOG;
	if(param->inc_text)		emask |= NF_ARCH_EMASK_TEXT;
	if(param->inc_ri)		emask |= NF_ARCH_EMASK_RI;
	if(param->inc_codec)	emask |= NF_ARCH_EMASK_CODEC;
	if(param->inc_player)	emask |= NF_ARCH_EMASK_PLAYER;
	if(param->no_audio_text) emask |= NF_ARCH_EMASK_NO_AUDIO_TEXT;
	if(param->inc_pos_log)  emask |= NF_ARCH_EMASK_POS_LOG;
		
	result = arch_pb_start_avi( ch_mask, emask, param->tag, param->user, param->memo ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;

}

/**
	@brief					
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_stop_avi( GError **error  )
{
	gint 		result = 0;
	
	result = arch_pb_stop_avi( ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;	
}
	
/**
	@brief				AVI ����	
	@param[out]	info	������ ���(api ����ڰ� �̸� buffer �Ҵ�( or ���ú���) �ؼ� ȣ��)	
	@param[in]	cb_event	callback function
	@param[in]	cb_context	callback context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_query_avi( NF_ARCH_AVI_INFO *info, cb_arch_fxn_t cb_event, gpointer cb_context, GError **error)
{
	gint 		result = 0;
	
	g_return_val_if_fail (info != NULL, FALSE);
		
	result = arch_pb_query_avi( (arch_snap_info_t *)info , cb_event, cb_context ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;		
}

/**
	@brief				AVI ����	
	@param[out]	info	������ ���(api ����ڰ� �̸� buffer �Ҵ�( or ���ú���) �ؼ� ȣ��)	
	@param[in]	cb_event	callback function
	@param[in]	cb_context	callback context
	@param[in] limit : size limit in MB
      @param[in] option : 0(nothing), 1(start time fix & calculate to limit time), 2(end time fix & calculate to limit time)
					 3(end time will be changed to size limit(start time fix)), 4(start time will be changed to size limit(end time fix))
			              2, 4 options are not yet supported.
	@param[in] whole_check : if set to one, query start time to end time without stopping at limitMb
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean	
nf_arch_pb_query_avi_ex( NF_ARCH_AVI_INFO *info, cb_arch_fxn_t cb_event, gpointer cb_context, GError **error, 
		guint limitMB, guint option, guint whole_check )
{
	gint 		result = 0;
	
	g_return_val_if_fail (info != NULL, FALSE);
		
	result = arch_pb_query_avi_ex( (arch_snap_info_t *)info , cb_event, cb_context , limitMB, option, whole_check); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		
		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				������ ������ info ����
	@param[in]	tag		������ Tag
	@param[in]	user	������ User
	@param[in]	memo	������ Memo
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_modify(gchar *tag, gchar *user, gchar *memo)
{
	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = arch_info_modify( tag, user, memo ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 		
	}
		
	return ( result == 0 ) ? 1:0;
}


/**
	@brief				������ ������ info ����
	@param[in]	fxn_cb	�ݹ� function pointer (if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_destroy(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = arch_info_destroy( fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
		
	return ( result == 0 ) ? 1:0;			
}


/**
	@brief				������ ������ sst�� �߰�
	@param[in]	fxn_cb	�ݹ� function pointer (if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_info_add(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_info_add( fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}
		
	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				��ī�̺� ����Ʈ ����� ������ �����´�.
	@param[in]	fxn_cb	�ݹ� function pointer (if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@return	gint		negative if an error occurred,  
						non negative total entry count
*/
gint
nf_arch_list_get_count(NF_ARCH_TYPE_E type, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb)
{
	gint 		result = 0;
	
	g_return_val_if_fail (type <= NF_ARCH_TYPE_RAW , -1);
		
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = arch_list_get_count( type, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}	
	return result;
}


/**
	@brief				��ī�̺� ����Ʈ���� snapshot ��� ��������
	@param[in]	dir		��ȸ�� ����� ����(NF_ARCH_DIR_E)
	@param[in]	arch_id	��ȸ�� ��ī�̺� ���̵� (0:ó������, 0xffff:������)
	@param[in]	rcount	��û�� ����
	@param[in]	get_image	�̹��� ������ ����
	@param[out]	list	��ȸ ������� ��� ����(����ڰ� �Ҵ��ؼ� ����)
						sizeof(arch_snap_info_t)*rcount
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@return	gint		negative if an error occurred,
						non negative total entry count
*/
gint 
nf_arch_list_get_snap(NF_ARCH_DIR_E dir, guint16 arch_id, guint16 rcount, 
					gboolean get_image, NF_ARCH_SNAP_INFO *list, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb)
{

	gint 		result = 0;

	g_return_val_if_fail (dir == NF_ARCH_DIR_FORWARD || dir == NF_ARCH_DIR_BACKWARD , -1);
	g_return_val_if_fail (rcount <= NF_ARCH_MAX_RCOUNT , -2);
	g_return_val_if_fail (get_image == 0 || get_image == 1, -3);
	g_return_val_if_fail (list != NULL, -4);
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	

	result = arch_list_get_snap( dir, arch_id, rcount, get_image, 
								(arch_snap_info_t *)list, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}	
	return ( result == 0 ) ? 1:0;	
}
					

/**
	@brief				��ī�̺� ����Ʈ���� avi ��� ��������
	@param[in]	dir		��ȸ�� ����� ����(NF_ARCH_DIR_E)
	@param[in]	arch_id	��ȸ�� ��ī�̺� ���̵� (0:ó������, 0xffff:������)
	@param[in]	rcount	��û�� ����
	@param[out]	list	��ȸ ������� ��� ����(����ڰ� �Ҵ��ؼ� ����)
						sizeof(arch_avi_info_t)*rcount
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@return	gint		negative if an error occurred,
						non negative total entry count
*/
gint 
nf_arch_list_get_avi(NF_ARCH_DIR_E dir, guint16 arch_id, guint16 rcount, 
					NF_ARCH_AVI_INFO *list, cb_arch_fxn_t fxn_cb, gpointer ctx_cb)
{
	gint 		result = 0;

	g_return_val_if_fail (dir == NF_ARCH_DIR_FORWARD || dir == NF_ARCH_DIR_BACKWARD , -1);
	g_return_val_if_fail (rcount <= NF_ARCH_MAX_RCOUNT , -2);
	g_return_val_if_fail (list != NULL, -3);

	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = arch_list_get_avi( dir, arch_id, rcount, 
								(arch_avi_info_t *)list, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
		return 0;
	}	
	return 1;
}
					

/**
	@brief				��ī�̺� ����Ʈ���� entry ����
	@param[in]	type	��ī�̺� Ÿ�� (NF_ARCH_TYPE_E)
	@param[in]	arch_id	��ī�̺� ���̵�
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_list_delete(NF_ARCH_TYPE_E type, guint16 arch_id, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;

	g_return_val_if_fail ( type <= NF_ARCH_TYPE_RAW , FALSE);
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_list_delete( type, arch_id, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}	
	return ( result == 0 ) ? 1:0;		
}

/*   
	@brief		set archive data format
	@prarm[in]	data_format	1:multi archiving, 0:avi archiving
	@return	gboolean	always return 0;
*/
gboolean
nf_arch_set_multi(guint data_format)
{
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	g_return_val_if_fail (data_format == 0 || data_format == 1 || data_format == 2, FALSE);
	
	if( data_format == 2) // mul(saf) to mul mode
		data_format = 1;

	arch_set_multi(data_format);
	
	return 1;
}


/**
	@brief				���� ����
	@param[in]	type	��ī�̺� Ÿ�� (NF_ARCH_TYPE_E)
	@param[in]	arch_id	��ī�̺� ���̵�
	@param[in]	dev_id	��ī�̺��� ����̽� ���̵�
	@param[in]	erase	erase and burn
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
#define _DP_	fprintf(stderr, "%s() : %d \n",__func__,__LINE__)

void (*burn_fxn_cb) (gint result, gpointer context);
void arch_burn_start_callback(int result, void *context)
{
	burn_fxn_cb(result, context);
	nf_set_using_usb(FALSE);
}

extern void nf_swt_ctl_wan_insert_mac(unsigned char *p_ipaddr);
gboolean 
nf_arch_burn_start(NF_ARCH_TYPE_E type, guint16 arch_id, guchar dev_id, 
					gboolean erase,	cb_arch_fxn_t fxn_cb, gpointer ctx_cb, 
					GError **error)
{
	gint 		result = 0;
	guint cam_type = 0; //defacult : NTSC

	g_return_val_if_fail ( type <= NF_ARCH_TYPE_RAW , FALSE);
	g_return_val_if_fail (erase == 0 || erase == 1, FALSE);
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
#if defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined( __DIGI_SUPPORT__)
	_archive_set_ftp_info();
#endif 
#ifdef ENABLE_ARCHIVE_SYSCAM_NAME
	_archive_set_syscam_info();
#endif

	_archive_set_cam_title();
	_archive_set_mac_addr();
	//NTSC(0)/PAL(1)
	if ( nf_sysdb_get_bool("sys.info.sig_type") )
		cam_type = 0xffffffff;//bit mask
	else
		cam_type = 0;
	arch_set_camera_type(cam_type);
	
	{
		unsigned char ipaddr[32] = { 0, };
		GValue ret_value = { 0, };

		if (nf_sysdb_get_key0("net.ftp.host", &ret_value, NULL))
		{
			g_stpcpy(ipaddr, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);

			nf_swt_ctl_wan_insert_mac(ipaddr);
		}
	}

	nf_set_using_usb(TRUE);
	if ( fxn_cb )
	{
		burn_fxn_cb = fxn_cb;
		result = arch_burn_start( type, arch_id, dev_id, erase, arch_burn_start_callback, ctx_cb); 
	}
	else
	{
		result = arch_burn_start( type, arch_id, dev_id, erase, fxn_cb, ctx_cb); 
		nf_set_using_usb(FALSE);
	}

	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
	}
	return ( result == 0 ) ? 1:0;
}
					

/**
	@brief				���� ����
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_burn_end(GError **error)
{
	gint 		result = 0;

#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_burn_end(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}	
	return ( result == 0 ) ? 1:0;
}


/**
	@brief				���� ���
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_burn_cancel(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{

	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_burn_cancel(fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}	
	return ( result == 0 ) ? 1:0;
		
}


/**
	@brief				��ī�̺�� ����̽� ��� ��� (�ִ� 4��)
	@param[out]	dev_table	����̽� ��� ����(����ڰ� �Ҵ� arch_dev_info_t[4])
	@param[out]	dev_cnt		����̽� ����
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_get_list(NF_ARCH_DEV_INFO *dev_table, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;
	
	g_return_val_if_fail (dev_table != NULL, FALSE);
		
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

#if defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined( __DIGI_SUPPORT__)
	_archive_set_ftp_info();
#endif 

	result = arch_dev_get_list(dev_table, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}
	return ( result > 0 ) ? 1:0;					
}
					

/**
	@brief				��ī�̺�� ����̽� �̵�� ��� &  ũ�� ��� (�ִ� 4��)
	@param[out]	dev_table	����̽� ��� ����(����ڰ� �Ҵ� arch_dev_info_t[4])
	@param[out]	dev_cnt		����̽� ����
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_get_size(NF_ARCH_DEV_INFO *dev_table, 
					cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;
	
	g_return_val_if_fail (dev_table != NULL, FALSE);
		
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

#if defined( ENABLE_ARCHIVE_FTP ) || defined(__LOREX_SUPPORT__) || defined( __DIGI_SUPPORT__)
	_archive_set_ftp_info();
#endif 

	result = arch_dev_get_size(dev_table, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}
	
	return ( result > 0 ) ? 1:0;					
}	
							
/**
	@brief				
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/

static int xxxx = 0;
gboolean 
nf_arch_dev_set_notify(cb_arch_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

if (xxxx == 0) {
	xxxx = 1;
}
else {
	g_assert(0);
}

	result = arch_dev_set_notify(fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 

		g_set_error( error, NF_API_ARCHIVE_ERROR, result, 
						nf_arch_get_error_string(result) );
		
	}	
	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_dev_unset_notify()
{
	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_dev_unset_notify(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 		
	}	
	return ( result == 0 ) ? 1:0;			
}




/**		
	@brief				��ī�̺� ���� ���� ��ȸ
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_get_progress(NF_ARCH_PROGRESS *progress)
{
	gint 		result = 0;
	
	g_return_val_if_fail (progress != NULL, FALSE);
		
	result = arch_get_progress(progress); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}	
	return ( result == 0 ) ? 1:0;			

}


/**
	@brief				
	@return				
*/
const gchar*
nf_arch_get_error_string(gint errorno)
{	
	switch (errorno)
	{
		case ARCH_ERR_NONE					: return "ARCH_ERR_NONE";
		case ARCH_ERR_INVMODE				: return "ARCH_ERR_INVMODE";
		case ARCH_ERR_INVDEV				: return "ARCH_ERR_INVDEV";	
		case ARCH_ERR_INVPARAM				: return "ARCH_ERR_INVPARAM";
		case ARCH_ERR_INVMEDIA				: return "ARCH_ERR_INVMEDIA";
		case ARCH_ERR_MEDIA_FULL			: return "ARCH_ERR_MEDIA_FULL";	
		case ARCH_ERR_MEDIA_NOTERASABLE		: return "ARCH_ERR_MEDIA_NOTERASABLE";	
		case ARCH_ERR_FAIL					: return "ARCH_ERR_FAIL";
		case ARCH_ERR_FAIL_DEVERR			: return "ARCH_ERR_FAIL_DEVERR";
		case ARCH_ERR_REQMORE				: return "ARCH_ERR_REQMORE";
		case ARCH_ERR_LISTEMPTY				: return "ARCH_ERR_LISTEMPTY";	
		case ARCH_ERR_LISTFULL				: return "ARCH_ERR_LISTFULL";
		case ARCH_ERR_LISTADDED				: return "ARCH_ERR_LISTADDED";	
		case ARCH_ERR_CANCELED				: return "ARCH_ERR_CANCELED";		
		case ARCH_ERR_DATALOCKED			: return "ARCH_ERR_DATALOCKED";
		case ARCH_ERR_MAX_AVAILABLE_SIZE	: return "ARCH_ERR_MAX_AVAILABLE_SIZE";
		case ARCH_ERR_FTP_CONN				: return "ARCH_ERR_FTP_CONN";
		case ARCH_ERR_FTP_AUTH				: return "ARCH_ERR_FTP_AUTH";
		case ARCH_ERR_FTP_FAIL				: return "ARCH_ERR_FTP_FAIL";
		case ARCH_ERR_MULTISESSION_ERROR	: return "ARCH_ERR_MULTISESSION_ERROR";
		case ARCH_ERR_DISABLE_MULTISESSION	: return "ARCH_ERR_DISABLE_MULTISESSION";
		case ARCH_ERR_MAX_TIME_ADJUSTED	: return "ARCH_ERR_MAX_TIME_ADJUSTED";
	}	
	return "NULL";
}


/**		
	@brief					��ī�̺� ���� �߻� ��ġ
	@param[out]	err_info	���ο��� �Ҵ� �� �������� ����ü ����
	@return	int				���� �׸� ����, �����̸� ����
*/
int nf_arch_get_error_info(NF_ARCH_ERROR_INFO **err_info)
{
	return arch_get_error_info( (struct error_info **)err_info);
}

/**
	@brief				
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_arch_scanODD(gchar *odd_name)
{
	gint 		result = 0;
	
#ifdef	DEBUG_ARCHIVE_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = arch_scanODD(odd_name); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 		
	}	
	return result;			
}
