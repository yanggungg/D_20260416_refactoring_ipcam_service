/*
 * scm_openssl.c
 *  - scm openssl
 *  - dependency :
 *
 * Written by Jeong-Ho Yang. <yanggungg@itxsecurity.com>
 * Copyright (c) ITX security, Jan 14, 2022
 *
 */

#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/err.h>		  /* ERR_() */
#include <time.h>					  /* asctime() */
#include <string.h>				  /* strncpy() */
#include "nf_sysdb.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "scm.h"
#include "scm_internal.h"

#define DBG_LEVEL 9
#define DBG_MODULE "SCM_OPENSSL"

#define SSL_SELF_SIGNED_PATH "/NFDVR/webra/conf/sslcertification.pem"
#define SSL_PUB_KEY_PATH "/NFDVR/webra/conf/ssl_public_key.pem"
#define SSL_PRI_KEY_PATH "/NFDVR/webra/conf/ssl_private_key.key"
#define SSL_DCV_FILE_PATH "/NFDVR/webra/htdocs/.well-known/pki-validation"
#define SSL_PUB_KEY_LINK "/tmp/webra/conf/ssl_pub_link"
#define SSL_PRI_KEY_LINK "/tmp/webra/conf/ssl_pri_link"

#define NAC_CA_CERT_PATH "/NFDVR/data/IPKI/nac/ca"
#define NAC_CL_CERT_PATH "/NFDVR/data/IPKI/nac/client"
#define NAC_CL_KEY_PATH "/NFDVR/data/IPKI/nac/client_key"

#define NAC_TMP_PATH "/NFDVR/data/IPKI/tmp_nac"
#define NAC_TMP_CA_CERT_PATH "/NFDVR/data/IPKI/tmp_nac/ca"
#define NAC_TMP_CL_CERT_PATH "/NFDVR/data/IPKI/tmp_nac/client"
#define NAC_TMP_CL_KEY_PATH "/NFDVR/data/IPKI/tmp_nac/client_key"

#define NAC_BACKUP_PATH "/NFDVR/data/IPKI/backup_nac"
#define NAC_BACKUP_CA_CERT_PATH "/NFDVR/data/IPKI/backup_nac/ca"
#define NAC_BACKUP_CL_CERT_PATH "/NFDVR/data/IPKI/backup_nac/client"
#define NAC_BACKUP_CL_KEY_PATH "/NFDVR/data/IPKI/backup_nac/client_key"

#define NAC_CA_CERT_LINK "/NFDVR/data/IPKI/nac/ca_cert_link"
#define NAC_CL_CERT_LINK "/NFDVR/data/IPKI/nac/client_cert_link"
#define NAC_CL_KEY_LINK "/NFDVR/data/IPKI/nac/client_key_link"

static FM_DATE_E _get_data_form()
{
	FM_DATE_E ret;
    int db_index = 0;
    
    db_index = nf_sysdb_get_uint("sys.date.dateform");

	if(db_index == 0)			ret = YYYYMMDD;
	else if(db_index == 1)		ret = MMDDYYYY;
	else if(db_index == 2)		ret = DDMMYYYY;
	else	ret = MMDDYYYY;

	return ret;
}

static int _is_exist_certificate()
{
    int ret = 0;
    
    if (ifn_is_file_exist(SSL_PUB_KEY_LINK) && ifn_is_file_exist(SSL_PRI_KEY_LINK)) ret = 1;
    
    return ret;
}

static char* ASN1_TIME_to_string(const ASN1_TIME* time, char *out)
{
    struct tm t;
    time_t tt;
    const char* str = (const char*) time->data;
    size_t i = 0;

    memset(&t, 0, sizeof(t));

    if (time->type == V_ASN1_UTCTIME) {/* two digit year */
        t.tm_year = (str[i++] - '0') * 10;
        t.tm_year += (str[i++] - '0');
        if (t.tm_year < 70)
            t.tm_year += 100;
    } else if (time->type == V_ASN1_GENERALIZEDTIME) {/* four digit year */
        t.tm_year = (str[i++] - '0') * 1000;
        t.tm_year+= (str[i++] - '0') * 100;
        t.tm_year+= (str[i++] - '0') * 10;
        t.tm_year+= (str[i++] - '0');
        t.tm_year -= 1900;
    }
    t.tm_mon  = (str[i++] - '0') * 10;
    t.tm_mon += (str[i++] - '0') - 1; // -1 since January is 0 not 1.
    t.tm_mday = (str[i++] - '0') * 10;
    t.tm_mday+= (str[i++] - '0');
    t.tm_hour = (str[i++] - '0') * 10;
    t.tm_hour+= (str[i++] - '0');
    t.tm_min  = (str[i++] - '0') * 10;
    t.tm_min += (str[i++] - '0');
    t.tm_sec  = (str[i++] - '0') * 10;
    t.tm_sec += (str[i++] - '0');

    tt = timegm(&t);
    if (tt > NF_UPPER_TIMELIMIT || tt < 0) {
        tt = NF_UPPER_TIMELIMIT;
    }
    ifn_get_local_day_text(tt, _get_data_form(), out);

    return out;
}

static int _filter_dcv_file(char *file_name, void *filter_param)
{
    int len;
    len = strlen(file_name);
    FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T *)filter_param;

    if (len < 5)
        return 0;
    if (g_strncasecmp(&(file_name[len - 4]), fparam->ext, 4))
        return 0;

    return 1;
}

static int _filter_icrt_file(char *file_name, void *filter_param)
{
    int len;
    len = strlen(file_name);
    FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T *)filter_param;

    if (len < 6)
        return 0;
    if (g_strncasecmp(&(file_name[len - 5]), fparam->ext, 5))
        return 0;

    return 1;
}

static int _filter_ssl_file(char *file_name, void *filter_param)
{
    int len;
    len = strlen(file_name);
    FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T *)filter_param;

    if (len < 5)
        return 0;
    if (g_strncasecmp(&(file_name[len - 4]), fparam->ext, 4))
        return 0;

    return 1;
}

static char **_new_ssl_dcv_list(MEDIA_ID id, int *ret_cnt)
{
    char **flist;
    char mnt_path[MAX_PATH_LEN + 1];
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1)
        return 0;

    strcpy(filter_param.ext, ".txt");
    filter_param.condition = FF_NOFILTER;
    flist = ifn_new_filelist(mnt_path, _filter_dcv_file, &filter_param, ret_cnt);

    return flist;
}

static char **_new_ssl_icrt_list(MEDIA_ID id, int *ret_cnt)
{
    char **flist;
    char mnt_path[MAX_PATH_LEN + 1];
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1)
        return 0;

    strcpy(filter_param.ext, ".icrt");
    filter_param.condition = FF_NOFILTER;
    flist = ifn_new_filelist(mnt_path, _filter_icrt_file, &filter_param, ret_cnt);

    return flist;
}

static char **_new_ssl_pem_list(MEDIA_ID id, int *ret_cnt)
{
    char **flist;
    char mnt_path[MAX_PATH_LEN + 1];
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1)
        return 0;

    strcpy(filter_param.ext, ".pem");
    filter_param.condition = FF_NOFILTER;
    flist = ifn_new_filelist(mnt_path, _filter_ssl_file, &filter_param, ret_cnt);

    return flist;
}

static char **_new_ssl_crt_list(MEDIA_ID id, int *ret_cnt)
{
    char **flist;
    char mnt_path[MAX_PATH_LEN + 1];
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1)
        return 0;

    strcpy(filter_param.ext, ".crt");
    filter_param.condition = FF_NOFILTER;
    flist = ifn_new_filelist(mnt_path, _filter_ssl_file, &filter_param, ret_cnt);

    return flist;
}

static char **_new_ssl_key_list(MEDIA_ID id, int *ret_cnt)
{
    char **flist;
    char mnt_path[MAX_PATH_LEN + 1];
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1)
        return 0;

    strcpy(filter_param.ext, ".key");
    filter_param.condition = FF_NOFILTER;
    flist = ifn_new_filelist(mnt_path, _filter_ssl_file, &filter_param, ret_cnt);

    return flist;
}

static char **_get_file_list(char *path, int *ret_cnt)
{
    char **flist;
    FNAME_FILTER_PARAM_T filter_param;

    memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));
    filter_param.condition = FF_NOFILTER;
    // filter_param.opt |= IFO_INCLUDE_FULLPATH;
    flist = ifn_new_filelist(path, NULL, &filter_param, ret_cnt);
    
    return flist;
}

static int _free_ssl_list(char **ssl_list)
{
    return ifn_free_filelist(ssl_list);
}

static int _remove_pri_key_password(char *src, char *desc, char *pri_pass)
{
    RSA *prsa = NULL;
    FILE *fp;
    char cmd[1024];
    char *ptr = NULL;

    memset(cmd, 0x00, sizeof(cmd));
    g_sprintf(cmd, "openssl rsa -in %s -out %s -passin pass:%s", src, "/tmp/ssl_private_key_nopass.pem", pri_pass);
    proxy_system(cmd, 1, 3);

    fp = fopen("/tmp/ssl_private_key_nopass.pem", "r");
    if (!fp)
    {
        DMSG(9, "ssl_private_key_nopass.pem open fail!");
        return 0;
    }

    prsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (!prsa)
    {
        DMSG(9, "Read private key fail!");
        fclose(fp);
        return 0;
    }
    DMSG(9, "prsa : %p", prsa);

    strcpy(desc, "/tmp/ssl_private_key_nopass.pem");
    fclose(fp);
    RSA_free(prsa);

    return 1;
}

static int _check_pri_key_password(char *file_path)
{
    RSA *prsa = NULL;
    FILE *fp;
    char strbuf[1024];

    fp = fopen(file_path, "r");
    if (!fp)
    {
        DMSG(9, "File open fail!");
        return -1;
    }

    prsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (!prsa)
    {
        fclose(fp);
        return 1;
    }

    fclose(fp);
    RSA_free(prsa);

    return 0;
}

static int _check_ssl_prikey_validate(char *file_path)
{
    RSA *prsa = NULL;
    FILE *fp;
    int ret = 0;

    DMSG(9, "file : %s", file_path);

    fp = fopen(file_path, "r");
    if (!fp)
    {
        DMSG(9, "File open fail!");
        return -1;
    }

    prsa = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (!prsa)
    {
        fclose(fp);
        return -1;
    }

    ret = RSA_check_key(prsa);

    fclose(fp);
    RSA_free(prsa);

    DMSG(9, "ret : %d", ret);

    return ret;
}

static int _check_ssl_source_file(char cert_path[][512])
{
    if (!ifn_is_file_exist(cert_path[0]))
    {
        DMSG(9, "Can't find PRIVATE KEY");
        return -1;
    }
    if (!ifn_is_file_exist(cert_path[1]))
    {
        DMSG(9, "Can't find PUBLIC KEY");
        return -1;
    }

    return 0;
}

static int _copy_ssl_key_file(char *pri_path, char *pub_path)
{
    char cmd[1024];

    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "cp %s %s", pri_path, SSL_PRI_KEY_PATH);
    proxy_system(cmd, 1, 3);

    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "cp %s %s", pub_path, SSL_PUB_KEY_PATH);
    proxy_system(cmd, 1, 3);

    if (!ifn_is_file_exist(SSL_PRI_KEY_PATH))
    {
        DMSG(9, "Can't find SSL PRIVATE KEY");
        return -1;
    }
    if (!ifn_is_file_exist(SSL_PUB_KEY_PATH))
    {
        DMSG(9, "Can't find SSL PUBLIC KEY");
        return -1;
    }

    return 0;
}

static int _get_ssl_key_content(char *pri_buf, char *pub_buf)
{
    gchar *pri_key = NULL, *pub_key = NULL;
    gsize pri_len = 0, pub_len = 0;
    GError *error = NULL;

    if (!g_file_get_contents(SSL_PRI_KEY_LINK, &pri_key, &pri_len, &error))
    {
        g_error_free(error);
        g_free(pri_key);
        return -1;
    }

    DMSG(9, "pri_len : %d", pri_len);

    if (!g_file_get_contents(SSL_PUB_KEY_LINK, &pub_key, &pub_len, &error))
    {
        g_error_free(error);
        g_free(pri_key);
        g_free(pub_key);
        return -2;
    }

    DMSG(9, "pub_len : %d", pub_len);

    strncpy(pri_buf, pri_key, pri_len);
    strncpy(pub_buf, pub_key, pub_len);

    g_free(pri_key);
    g_free(pub_key);

    return 0;
}

static int _decrypt_icrt(char *cipher_path, char *des_path)
{
    int rsz = 0, wsz = 0, tsz = 0;
    int ret = -1;
    int ctx_inited = 0;
    FILE *fin = NULL, *fout = NULL;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX ctx_buf;
    EVP_CIPHER_CTX *ctx = &ctx_buf;
#else
    EVP_CIPHER_CTX *ctx = NULL;
#endif
    unsigned char rbuf[1024];
    unsigned char wbuf[1024 + EVP_MAX_BLOCK_LENGTH];
    unsigned char key[32] = {
        0,
    };
    unsigned char iv[16] = {
        0,
    };

    memset(rbuf, 0x00, 1024);
    memset(wbuf, 0x00, 1024 + EVP_MAX_BLOCK_LENGTH);
    
    if (!ifn_is_file_exist(cipher_path))
    {
        DMSG(9, "Can't find cipher_path");
        return -1;
    }

    fin = fopen(cipher_path, "rb");
    if (fin == NULL)
    {
        DMSG(9, "read file open fail");
        return -1;
    }

    fout = fopen(des_path, "wb");
    if (fout == NULL)
    {
        DMSG(9, "wirte file open fail");
        goto done;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_init(ctx);
    ctx_inited = 1;
#else
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        DMSG(9, "EVP_CIPHER_CTX_new fail");
        goto done;
    }
    ctx_inited = 1;
#endif

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        DMSG(9, "EVP_DecryptInit_ex fail");
        goto done;
    }

    while ((rsz = fread(rbuf, 1, 1024, fin)) > 0)
    {
        if (1 != EVP_DecryptUpdate(ctx, wbuf, &wsz, rbuf, rsz))
        {
            DMSG(9, "EVP_DecryptUpdate fail");
            goto done;
        }

        fwrite(wbuf, 1, wsz, fout);
    }

    if (1 != EVP_DecryptFinal_ex(ctx, wbuf + wsz, &tsz))
    {
        DMSG(9, "EVP_DecryptFinal_ex fail");
        goto done;
    }

    fwrite(wbuf + wsz, 1, tsz, fout);

    ret = 0;

done:
    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);
    if (ctx_inited)
    {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        EVP_CIPHER_CTX_cleanup(ctx);
#else
        EVP_CIPHER_CTX_free(ctx);
#endif
    }
    
    if (ret == 0)
        DMSG(9, "");

    return ret;
}

static int _uncompress_icrt(char *src_path)
{
    char cmd[256];
    
    if (!ifn_is_file_exist(src_path))
    {
        DMSG(9, "Can't find src_path");
        return -1;
    }
    
    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "tar -zxvf %s -C /NFDVR/webra/conf/", src_path);
    if (system(cmd) != 0)
    {
        DMSG(9, "tar error");
        return -1;
    }
    
    if (remove(src_path) != 0)
    {
        DMSG(9, "icrt file remove fail");
    }
    
    return 0;
}

static int _remove_ca_certificate()
{
    char cmd[1024];
    int ret = 0;
    int res = 0;

    if (ifn_is_file_exist(SSL_PRI_KEY_PATH))
    {
        DMSG(9, "Delete PRIVATE KEY");
        res = remove(SSL_PRI_KEY_PATH);
        DMSG(9, "res : %d", res);
        ret = 1;
    }

    if (ifn_is_file_exist(SSL_PUB_KEY_PATH))
    {
        DMSG(9, "Delete PUBLIC KEY");
        res = remove(SSL_PUB_KEY_PATH);
        DMSG(9, "res : %d", res);
        ret = 1;
    }
    
    return ret;
}

static int _remove_self_signed_certificate()
{
    char cmd[1024];
    int ret = 0;
    int res = 0;

    if (ifn_is_file_exist(SSL_SELF_SIGNED_PATH))
    {
        DMSG(9, "Remove SSL_SELF_SIGNED_PATH");
        res = remove(SSL_SELF_SIGNED_PATH);
        DMSG(9, "res : %d", res);
        ret = 1;
    }
    
    return ret;
}

static int _ssl_save_db(char *pri_key, char *pub_key)
{
    GValue set_value = {
        0,
    };

    nf_sysdb_lock(NF_SYSDB_CATE_NET);
    
    nf_sysdb_set_str("net.http.cert", pri_key);
    // nf_sysdb_set_str("net.http.ca_cert", pub_key);

    nf_sysdb_save("net");
    nf_sysdb_unlock(NF_SYSDB_CATE_NET);
    
    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

    return 0;
}

static int _get_issuer(X509 *x509, char *buf)
{
    X509_NAME *cn;
    int lastpos = -1;
    
    if (!x509) return -1;
    
    cn = X509_get_issuer_name(x509);
    if (!cn) return -1;
    
    for (;;)
    {
        lastpos = X509_NAME_get_index_by_NID(cn, NID_commonName, lastpos);
        if (lastpos == -1)
            break;
        X509_NAME_ENTRY *e = X509_NAME_get_entry(cn, lastpos);
        ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
	    char *str = ASN1_STRING_data(d);
        strcpy(buf, str);
    }

    return 0;
}

static int _get_subject(X509 *x509, char *buf)
{
    X509_NAME *cn;
    int lastpos = -1;
    
    if (!x509) return -1;
    
    cn = X509_get_subject_name(x509);
    if (!cn) return -1;
    
    for (;;)
    {
        lastpos = X509_NAME_get_index_by_NID(cn, NID_commonName, lastpos);
        if (lastpos == -1)
            break;
        X509_NAME_ENTRY *e = X509_NAME_get_entry(cn, lastpos);
        ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
	    char *str = ASN1_STRING_data(d);
        strcpy(buf, str);
    }

    return 0;
}

static int _make_link(int type)
{
    char cmd[512];
    char cmd2[512];
    int res = 0;
    
    memset(cmd, 0, sizeof(cmd));
    memset(cmd2, 0, sizeof(cmd2));
    
    if (type == 0)
    {
        sprintf(cmd, "ln -fs %s %s", SSL_PUB_KEY_PATH, SSL_PUB_KEY_LINK);
        sprintf(cmd2, "ln -fs %s %s", SSL_PRI_KEY_PATH, SSL_PRI_KEY_LINK);
    }
    else
    {
        sprintf(cmd, "ln -fs %s %s", SSL_SELF_SIGNED_PATH, SSL_PUB_KEY_LINK);
        sprintf(cmd2, "ln -fs %s %s", SSL_SELF_SIGNED_PATH, SSL_PRI_KEY_LINK);
    }
        
    if (system(cmd) || system(cmd2))
    {
        res = -1;
    }
    

    return res;
}

static int _get_cert_info(FILE *fp, SSL_CERT_INFO_T *info)
{
    X509 *x509 = NULL;
    char *name = NULL;
	char notBefore[64];
	char notAfter[64];
    
    x509 = PEM_read_X509(fp, NULL, NULL, NULL);
    if (!x509)
    {
        DMSG(9, "x509 read open fail!");
        return -1;
    }
    
    _get_issuer(x509, info->issuer);
    _get_subject(x509, info->subject);
    ASN1_TIME_to_string(X509_get_notBefore(x509), info->from);
    ASN1_TIME_to_string(X509_get_notAfter(x509), info->to);
                              
    DMSG(9, "issuer : %s", info->issuer);
    DMSG(9, "subject : %s", info->subject);
    DMSG(9, "from : %s", info->from);
    DMSG(9, "to : %s", info->to);
    
    X509_free(x509);
    
    return 0;
}

static int _get_realpath_filename(const char* link_path, char* filename, size_t max_len) 
{
    char resolved_path[PATH_MAX];
    char* pos = NULL;
    
    if (realpath(link_path, resolved_path) == NULL) {
        perror("realpath");
        return 0;
    }
    
    pos = strrchr(resolved_path, '/');
    
    if (pos == NULL) {
        strncpy(filename, resolved_path, max_len - 1);
    } else {
        strncpy(filename, pos + 1, max_len - 1);
    }
    
    filename[max_len - 1] = '\0';
    
    return strlen(filename);
}

static int _check_self_signed_cert()
{
    char fname[128];
    int ret = 0;
    int len = 0;
    
    memset(fname, 0x00, sizeof(fname));
    
    len = _get_realpath_filename(SSL_PUB_KEY_LINK, fname, 128);
    DMSG(9, "cert name : %s", fname);
    
    if (strcmp(fname, "sslcertification.pem") == 0) ret = 1;
    
    return ret;
}

static int _upload_8021x_cert_temp(char *src_path, char *dest_path)
{
    char cmd[512];
    int ret = 0;
    
    memset(cmd, 0x00, sizeof(cmd));
    
    if (!ifn_is_file_exist(dest_path)) {
        DMSG(9, "dest_path fail!");
        ret = mkdir(dest_path, 0777);
        DMSG(9, "mkdir dest_path ret : %d", ret);
        if (ret == -1) return -1;
    }
    
    sprintf(cmd, "cp %s %s", src_path, dest_path);
    proxy_system(cmd, 1, 3);
    
    return 0;
}

static int _delete_8021x_cert_temp(CERT_TYPE_E type)
{
    int ret = 0;
    
    if (type == CERT_TYPE_CA)
    {
        if (ifn_is_file_exist(NAC_TMP_CA_CERT_PATH)) {
            ret = ifn_remove_dir_rf(NAC_TMP_CA_CERT_PATH);
        }
    }
    else if (type == CERT_TYPE_CLIENT)
    {
        if (ifn_is_file_exist(NAC_TMP_CL_CERT_PATH)) {
            ret = ifn_remove_dir_rf(NAC_TMP_CL_CERT_PATH);
        }
    }
    else
    {
        if (ifn_is_file_exist(NAC_TMP_CL_KEY_PATH)) {
            ret = ifn_remove_dir_rf(NAC_TMP_CL_KEY_PATH);
        }        
    }
    
    return ret;
}

static int _delete_8021x_cert_temp_all()
{
    char cmd[512];
    int ret = 0;
    
    memset(cmd, 0x00, sizeof(cmd));
    
    if (ifn_is_file_exist(NAC_TMP_PATH)) {
        ret = ifn_remove_dir_rf(NAC_TMP_PATH);
    }
    
    return ret;
}

static int _delete_8021x_cert_del_all()
{
    char cmd[512];
    int ret = 0;
    
    memset(cmd, 0x00, sizeof(cmd));
    
    if (ifn_is_file_exist(NAC_BACKUP_PATH)) {
        ret = ifn_remove_dir_rf(NAC_BACKUP_PATH);
    }
    
    return ret;
}

static int _delete_8021x_cert_link(CERT_TYPE_E type)
{
    int ret = 0;
    
    if (type == CERT_TYPE_CA) {
        remove(NAC_CA_CERT_LINK);
    }
    else if (type == CERT_TYPE_CLIENT) {
        remove(NAC_CL_CERT_LINK);
    }
    else {
        remove(NAC_CL_KEY_LINK);
    }
    
    return ret;

}

static int _move_8021x_cert_dir(char *target_dir, char *dest_dir)
{
    char cmd[512];
    
    memset(cmd, 0x00, sizeof(cmd));
    
    ifn_remove_dir_rf(dest_dir);
    sprintf(cmd, "mv %s %s", target_dir, dest_dir);
    proxy_system(cmd, 1, 3);
    
    DMSG(9, "move %s to %s", target_dir, dest_dir);
    
    return 0;
}

static int _make_8021x_cert_link(char *target_path, char *dest_link)
{
    char cmd[512];
    int ret = 0;
    int file_cnt = 0;
    char **filelist = NULL;
    int i;
    
    filelist = _get_file_list(target_path, &file_cnt);
    if (!filelist || file_cnt <= 0 || !filelist[0]) {
        DMSG(9, "empty cert dir : %s", target_path);
        if (filelist) {
            ifn_free_filelist(filelist);
        }
        return -1;
    }

    for (i = 0; i < file_cnt; i++) {
        DMSG(9, "file[%d] : %s", i, filelist[i]);
    }
    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "ln -fs %s/%s %s", target_path, filelist[0], dest_link);
    ret = proxy_system(cmd, 1, 3);
    
    ifn_free_filelist(filelist);
    
    return ret;
}

static int _backup_8021x_cert(CERT_TYPE_E type)
{
    int ret = 0;
    
    if (( type == CERT_TYPE_CA && !ifn_is_file_exist(NAC_CA_CERT_PATH) ) ||
        ( type == CERT_TYPE_CLIENT && !ifn_is_file_exist(NAC_CL_CERT_PATH) ) ||
        ( type == CERT_TYPE_CLIENT_KEY && !ifn_is_file_exist(NAC_CL_KEY_PATH) ))
    {
        DMSG(9, "No original certificate!");
        return -1;
    }
    
    if (!ifn_is_file_exist(NAC_BACKUP_PATH)) {
        ret = mkdir(NAC_BACKUP_PATH, 0777);
        if (ret == -1) return -2;
    }
    
    if (type == CERT_TYPE_CA)
    {
        _move_8021x_cert_dir(NAC_CA_CERT_PATH, NAC_BACKUP_CA_CERT_PATH);
        ret = 1;
    }
    else if (type == CERT_TYPE_CLIENT)
    {
        _move_8021x_cert_dir(NAC_CL_CERT_PATH, NAC_BACKUP_CL_CERT_PATH);
        ret = 1;
    }
    else
    {
        _move_8021x_cert_dir(NAC_CL_KEY_PATH, NAC_BACKUP_CL_KEY_PATH);
        ret = 1;
    }

    return ret;
}

static void _restore_8021x_cert()
{
    if (ifn_is_file_exist(NAC_BACKUP_PATH)) {
        if (ifn_is_file_exist(NAC_BACKUP_CA_CERT_PATH)) {
            _move_8021x_cert_dir(NAC_BACKUP_CA_CERT_PATH, NAC_CA_CERT_PATH);
        }
        if (ifn_is_file_exist(NAC_BACKUP_CL_CERT_PATH)) {
            _move_8021x_cert_dir(NAC_BACKUP_CL_CERT_PATH, NAC_CL_CERT_PATH);
        }
        if (ifn_is_file_exist(NAC_BACKUP_CL_KEY_PATH)) {
            _move_8021x_cert_dir(NAC_BACKUP_CL_KEY_PATH, NAC_CL_KEY_PATH);
        }
        
        ifn_remove_dir_rf(NAC_BACKUP_PATH);
    }
    
    if (ifn_is_file_exist(NAC_CA_CERT_PATH)) {
        _make_8021x_cert_link(NAC_CA_CERT_PATH, NAC_CA_CERT_LINK);
    }
    if (ifn_is_file_exist(NAC_CL_CERT_PATH)) {
        _make_8021x_cert_link(NAC_CL_CERT_PATH, NAC_CL_CERT_LINK);
    }
    if (ifn_is_file_exist(NAC_CL_KEY_PATH)) {
        _make_8021x_cert_link(NAC_CL_KEY_PATH, NAC_CL_KEY_LINK);
    }
}

static int _delete_8021x_cert()
{
    int ret = 0;
    
    ret = ifn_delete_file_in_dir("/NFDVR/data/IPKI/nac");
    
    return ret;
}

////////////////////////////////////////////
// public
//

int _scm_work_ssl_install(SCM_T *piscm, TRANSACTION_E tra)
{
    char pri_buf[8192];
    char pub_buf[8192];
    char cmd[1024];
    int ret = 0;

    DMSG(9, "");

    system("/NFDVR/webra/sbin/httpd.sh restart");
    
    _scm_work_net_start(piscm, tra);

    return 0;
}

int _scm_work_ssl_delete(SCM_T *piscm, TRANSACTION_E tra)
{
    char pri_buf[8192];
    char pub_buf[8192];
    char cmd[1024];
    int ret = 0;

    DMSG(9, "");

    system("/NFDVR/webra/sbin/httpd.sh restart");
    
    _scm_work_net_start(piscm, tra);

    return 0;
}

int _scm_openssl_delete_cert()
{
    _remove_ca_certificate();
    _delete_8021x_cert();
    
    return 0;
}

int _scm_init_8021x_cert()
{
    _restore_8021x_cert();
    _delete_8021x_cert_temp_all();

    return 0;
}

char **scm_new_ssl_dcv_list(MEDIA_ID id, int *ret_cnt)
{
    return _new_ssl_dcv_list(id, ret_cnt);
}

char **scm_new_ssl_icrt_list(MEDIA_ID id, int *ret_cnt)
{
    return _new_ssl_icrt_list(id, ret_cnt);
}

char **scm_new_ssl_pem_list(MEDIA_ID id, int *ret_cnt)
{
    return _new_ssl_pem_list(id, ret_cnt);
}

char **scm_new_ssl_crt_list(MEDIA_ID id, int *ret_cnt)
{
    return _new_ssl_crt_list(id, ret_cnt);
}

char **scm_new_ssl_key_list(MEDIA_ID id, int *ret_cnt)
{
    return _new_ssl_key_list(id, ret_cnt);
}

int scm_free_ssl_list(char **ssl_list)
{
    return _free_ssl_list(ssl_list);
}

int scm_is_private_key(char *key_path)
{
    gchar *content = NULL;
    gsize len = 0;
    GError *error = NULL;
    int ret = 0;

    if (!g_file_get_contents(key_path, &content, &len, &error))
    {
        g_error_free(error);
        g_free(content);
        return -1;
    }

    DMSG(9, "len : %d", len);

    if (strstr(content, "PRIVATE"))
        ret = 1;

    g_free(content);

    return ret;
}

int scm_is_public_key(char *key_path)
{
    gchar *content = NULL;
    gsize len = 0;
    GError *error = NULL;
    int ret = 0;

    if (!g_file_get_contents(key_path, &content, &len, &error))
    {
        g_error_free(error);
        g_free(content);
        return -1;
    }

    DMSG(9, "len : %d", len);

    if (strstr(content, "CERTIFICATE"))
        ret = 1;

    g_free(content);

    return ret;
}

int scm_check_private_key_pass(char *key_path)
{
    return _check_pri_key_password(key_path);
}

int scm_ssl_icrt_install(char *icrt_path, IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_SSL_INSTALL;
    
    if (_decrypt_icrt(icrt_path, "/tmp/itxssl.tar.gz") == -1)
    {
        DMSG(9, "_decrypt_icrt fail");
        return -1;
    }
    
    if (_uncompress_icrt("/tmp/itxssl.tar.gz") == -1)
    {
        DMSG(9, "_uncompress_icrt fail");
        return -2;
    }
    
    if (_make_link(0) != 0)
    {
        DMSG(9, "_make_link fail");
        return -3;
    }
    
    // check pri key validate
    if (_check_ssl_prikey_validate(SSL_PRI_KEY_LINK) != 1)
        return -4;
        
    _remove_self_signed_certificate();
    
    // reload webserver
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    _scm_push_notification(INFY_NETCHANGE_API_BEGIN, tra);
    _scm_work_service_stop(&iscm, tra, 0);

    return 0;
}

int scm_ssl_install_certificate(char cert_path[][512], char *pri_pass, IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_SSL_INSTALL;
    char tmp[512];
    char pri_key[512];
    char pub_key[512];

    memset(tmp, 0x00, sizeof(tmp));
    memset(pri_key, 0x00, sizeof(pri_key));
    memset(pub_key, 0x00, sizeof(pub_key));

    // check file
    if (_check_ssl_source_file(cert_path) == -1)
        return -1;

    // remove private pass
    if (strlen(pri_pass) > 0)
    {
        if (!_remove_pri_key_password(cert_path[0], tmp, pri_pass))
        {
            return -3;
        }
        strcpy(pri_key, tmp);
    }
    else
    {
        strcpy(pri_key, cert_path[0]);
    }
    strcpy(pub_key, cert_path[1]);

    // check pri key validate
    if (_check_ssl_prikey_validate(pri_key) != 1)
        return -2;

    // file copy
    if (_copy_ssl_key_file(pri_key, pub_key) != 0)
        return -4;
    
    if (_make_link(0) != 0)
    {
        DMSG(9, "_make_link fail");
        return -5;
    }

    // reload webserver
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    _scm_push_notification(INFY_NETCHANGE_API_BEGIN, tra);
    _scm_work_service_stop(&iscm, tra, 0);

    return 0;
}

int scm_ssl_self_signed_install(IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_SSL_INSTALL;

    _remove_ca_certificate();
    
    if (!ifn_is_file_exist(SSL_SELF_SIGNED_PATH)) {
        system("/bin/sh /NFDVR/webra/sbin/make_ssl_cert.sh");
    }
    
    if (_make_link(1) != 0)
    {
        DMSG(9, "_make_link fail");
        return -1;
    }

    // reload webserver
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    _scm_push_notification(INFY_NETCHANGE_API_BEGIN, tra);
    _scm_work_service_stop(&iscm, tra, 0);

    return 0;
}

int scm_ssl_delete_certificate(IMSG ret_msg)
{
    TRANSACTION_E tra = TRA_SSL_DELETE;
    int need_restart = 0;
    
    if (_check_self_signed_cert()) return -1;

    if (_remove_ca_certificate())
    {
        need_restart = 1;
    }

    // reload webserver
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

    _scm_push_notification(INFY_NETCHANGE_API_BEGIN, tra);
    _scm_work_service_stop(&iscm, tra, 0);

    return 0;
}

int scm_ssl_get_cert_info(SSL_CERT_INFO_T *info)
{
    FILE *fp = NULL;
    int ret = 0;
        
    fp = fopen(SSL_PUB_KEY_LINK, "r");
    if (!fp)
    {
        DMSG(9, "Public key open error");
        return -1;
    }
    
    ret = _get_cert_info(fp, info);
    
    fclose(fp);
    
    return ret;
}

int scm_ssl_install_dcv_file(char *path)
{
    char cmd[512];
    int res = 0;
    
    if (ifn_make_dir_p(SSL_DCV_FILE_PATH) == -1)
    {
        DMSG(9, "Make DCV directory fail");
        return -1;
    }
    
    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "cp %s %s", path, SSL_DCV_FILE_PATH);
    res = proxy_system(cmd, 1, 3);
    DMSG(9, "res : %d", res);
    
    return res;
}

int scm_ssl_is_exist_certificate()
{
    return _is_exist_certificate();
}

int scm_8021x_upload_cert_temp(char *fpath, CERT_TYPE_E cert_type)
{
    char cmd[512];
    int ret = 0;
    
    memset(cmd, 0x00, sizeof(cmd));
    
    if (!ifn_is_file_exist(fpath)) {
        DMSG(9, "fail : %s", fpath);
        return -1;
    }
    
    if (!ifn_is_file_exist(NAC_TMP_PATH)) {
        ret = mkdir(NAC_TMP_PATH, 0777);
        if (ret == -1) return -2;
    }
    
    if (cert_type == CERT_TYPE_CA) {
        _upload_8021x_cert_temp(fpath, NAC_TMP_CA_CERT_PATH);
        _make_8021x_cert_link(NAC_TMP_CA_CERT_PATH, NAC_CA_CERT_LINK);
    }
    else if (cert_type == CERT_TYPE_CLIENT) {
        _upload_8021x_cert_temp(fpath, NAC_TMP_CL_CERT_PATH);
        _make_8021x_cert_link(NAC_TMP_CL_CERT_PATH, NAC_CL_CERT_LINK);
    }
    else {
        _upload_8021x_cert_temp(fpath, NAC_TMP_CL_KEY_PATH);
        _make_8021x_cert_link(NAC_TMP_CL_KEY_PATH, NAC_CL_KEY_LINK);
    }
    
    return 0; 
}

int scm_8021x_delete_cert(CERT_TYPE_E cert_type)
{
    int ret = 0;
    
    ret = _backup_8021x_cert(cert_type);
    _delete_8021x_cert_temp(cert_type);
    _delete_8021x_cert_link(cert_type);
    
    if (!ifn_is_file_exist(NAC_TMP_CA_CERT_PATH) &&
        !ifn_is_file_exist(NAC_TMP_CL_CERT_PATH) &&
        !ifn_is_file_exist(NAC_TMP_CL_KEY_PATH))
    {
        _delete_8021x_cert_temp_all();
    }
    
    return ret;
}

int scm_8021x_apply_cert()
{
    int ret = 0;
    
    if (ifn_is_file_exist(NAC_TMP_CA_CERT_PATH)) {
        _move_8021x_cert_dir(NAC_TMP_CA_CERT_PATH, NAC_CA_CERT_PATH);    
        _make_8021x_cert_link(NAC_CA_CERT_PATH, NAC_CA_CERT_LINK);
    }
    
    if (ifn_is_file_exist(NAC_TMP_CL_CERT_PATH)) {
        _move_8021x_cert_dir(NAC_TMP_CL_CERT_PATH, NAC_CL_CERT_PATH);    
        _make_8021x_cert_link(NAC_CL_CERT_PATH, NAC_CL_CERT_LINK);
    }
    
    if (ifn_is_file_exist(NAC_TMP_CL_KEY_PATH)) {
        _move_8021x_cert_dir(NAC_TMP_CL_KEY_PATH, NAC_CL_KEY_PATH);    
        _make_8021x_cert_link(NAC_CL_KEY_PATH, NAC_CL_KEY_LINK);
    }
    
    if (ifn_is_file_exist(NAC_BACKUP_PATH)) {
        _delete_8021x_cert_del_all();
    }
    
    _delete_8021x_cert_temp_all();
    
    return ret;
}

int scm_8021x_cancel_cert()
{
    int ret = 1;
    
    _restore_8021x_cert();
    _delete_8021x_cert_temp_all();
    
    return ret;
}

int scm_8021x_check_changed()
{
    if (ifn_is_file_exist(NAC_TMP_PATH) || 
        ifn_is_file_exist(NAC_BACKUP_PATH))
    {
        return 1;
    }
    
    return 0;
}

int scm_8021x_get_cert_filename(CERT_TYPE_E cert_type, char *fname, int buf_size)
{
    size_t len = 0;
    
    memset(fname, 0x00, buf_size);
    
    if (cert_type == CERT_TYPE_CA) {
        if (!ifn_is_file_exist(NAC_CA_CERT_LINK)) return 0;
        len = _get_realpath_filename(NAC_CA_CERT_LINK, fname, buf_size);
    }
    else if (cert_type == CERT_TYPE_CLIENT) {
        if (!ifn_is_file_exist(NAC_CL_CERT_LINK)) return 0;
        len = _get_realpath_filename(NAC_CL_CERT_LINK, fname, buf_size);
    }
    else {
        if (!ifn_is_file_exist(NAC_CL_KEY_LINK)) return 0;
        len = _get_realpath_filename(NAC_CL_KEY_LINK, fname, buf_size);
    }
    
    return len;
}

int scm_8021x_get_cert_info(CERT_TYPE_E cert_type, SSL_CERT_INFO_T *info)
{
    FILE *fp = NULL;
    int ret = 0;
        
    if (cert_type == CERT_TYPE_CA) {
        if (!ifn_is_file_exist(NAC_CA_CERT_LINK)) return -2;
        
        fp = fopen(NAC_CA_CERT_LINK, "r");
        if (!fp)
        {
            DMSG(9, "NAC_CA_CERT_LINK open error");
            return -1;
        }
    }
    else if (cert_type == CERT_TYPE_CLIENT) {
        if (!ifn_is_file_exist(NAC_CL_CERT_LINK)) return -2;
        
        fp = fopen(NAC_CL_CERT_LINK, "r");
        if (!fp)
        {
            DMSG(9, "NAC_CL_CERT_LINK open error");
            return -1;
        }
    }
    
    if (fp)
    {
        ret = _get_cert_info(fp, info);    
        fclose(fp);
    }
    
    return ret;
}
