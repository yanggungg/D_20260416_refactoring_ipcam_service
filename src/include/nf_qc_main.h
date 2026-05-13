#define PORT 5000
#define MAXBUF 50000

//------------------------------------------------------------------------------
#define BCD_BIN(val)        (((val) & 0x0f) + ((val)>>4)*10)
#define BIN_BCD(val)        ((((val)/10)<<4) + (val)%10)

//===============================================
// QC load/set data
//===============================================
typedef struct _QC_LOADSET_INFO
{
    char hw_ver[32];
    char front_type[32];
    char rc_type[32];
    u_char rtc_info[7];
    char mac_info[32];

}QC_LOADSET_INFO;

enum
{
	NET = 1,
	COM,
	READ,
	WRITE,
	PROTOCOL,
	PROTOCOL_PARAM,
};
typedef struct TRANSINFO
{
	int mode;
	int sock;
}trans_info;

/**********************
	QC FUNC
**********************/
void nf_qc_init(void);