#include <sys/types.h>                                                   
#include <sys/stat.h>                                                    
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <glib.h>

#include <net/if.h>
#include <sys/socket.h>
#include <linux/mii.h>

#include "nf_common.h"
#include "nf_sysdb.h"
#include "nf_api_param_hw_enc.h"
#include "nf_util_device.h"
#include "nf_qc.h"
#include "nf_api_param_hw.h"
#include "nf_sysman.h"
// #include "nf_event_vloss.h"
// #include "nf_event_video_type_analog.h"
// #include "nf_video_type.h"

#include "unp.h"

//#define DEBUG_DISABLE_DRIVER   //FIXME choissi

#define DEBUG_JBSHELL_UTIL_DEVICE
#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
	#include "jbshell.h"
#endif

#if  defined(USE_DEV_DECODER)
	#include "decoder.h"
#endif /* defined(USE_DEV_DECODER) */


#if defined(USE_DEV_TWAUDIO)
#include "tw286x.h"
#endif /* defined(USE_DEV_TWAUDIO) */


#if defined(USE_DEV_AM8816)
#include "am8816_ioctl.h"
#endif /* defined(USE_DEV_AM8816) */


#if defined(USE_DEV_KEYPAD)
#include "keypad.h"
#include "itx_key_table_info.h"
#endif /* defined(USE_DEV_KEYPAD) */


#if defined(USE_DEV_JOG)
#include "jog.h"
#endif /* defined(USE_DEV_JOG) */


#if defined(USE_DEV_SHUTTLE)
#include "shuttle.h"
#endif /* defined(USE_DEV_SHUTTLE) */


#if defined(USE_DEV_REMOCON)
#include "keypad.h"
#endif /* defined(USE_DEV_REMOCON) */


#if defined(USE_DEV_ALARM_IN)
#include "alarm_in.h"
#endif /* defined(USE_DEV_ALARM_IN) */


#if defined(USE_DEV_ALARM_OUT)
#include "alarm_out.h"
	#if defined(ENABLE_RELAY_IPCAM)
		#include "nf_api_ipcam.h"
	#endif
#endif /* defined(USE_DEV_ALARM_OUT) */

#if defined(USE_DEV_MICOM)
#include "drv_micom.h"
#endif /* defined(USE_DEV_MICOM) */

#if defined(USE_DEV_BOARD_PP)
#include "board_pp.h"
#endif /* defined(USE_DEV_BOARD_PP) */


#if defined(USE_DEV_WATCHDOG)
#include "watchdog.h"
#endif /* defined(USE_DEV_WATCHDOG) */


#if defined(USE_DEV_RS485)
#include "fpga_rs485_ioctl.h"
#endif /* defined(USE_DEV_RS485) */


#if defined(USE_DEV_FS1648)
#include "fs1648.h"
#endif /* defined(USE_DEV_FS1648) */

#if defined(USE_DEV_CD22M3492)
#include "spot_anf_ioctl.h"
#endif /* defined(USE_DEV_ANALOG_SPOT) */

#if defined(USE_DEV_GENNUM)
#include "gennum.h"
#endif /* defined(USE_DEV_GENNUM) */

#if defined(USE_DEV_TPS2384)
#include "tps2384.h"
#endif /* defined(USE_DEV_GENNUM) */

#if defined(USE_DEV_TLV320AIC23)
#include "tlv320aic23.h"
#endif

#if defined(USE_DEV_EDID)
#include "itx_edid.h"
#endif

#if defined(USE_DEV_DRV_EXT)
#include "drv_ext.h"
#endif

#if defined(USE_DEV_IP804)
#include "ip804.h"
#endif

#if defined(USE_DEV_IP804)
#include "ip1819_switch.h"
#endif

//#define DEBUG_DECODER_API
//#define DEBUG_RELAY_API
//#define DEBUG_BUZZER_API
//#define DEBUG_BOARDPP_API
//#define DEBUG_LED_ONOFF_MSG

#if 0
#if defined(USE_DEV_IVR_HF)
	#include "ivr_hf.h"
#endif
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_dev"

// XXX
#define DEBUG_IOCTL		/** 20100702 added by pakkhman **/
static gint _Fd_Arr[DEV_FD_IDX_NR];

#if defined(USE_DEV_SWITCH)

// ksi_test
struct itx_ip1819_switch_data {
	struct mii_ioctl_data mii;
	unsigned char page;
};
	static GStaticMutex _nf_dev_switch_mutex = G_STATIC_MUTEX_INIT;
	static struct ifreq ifr;
#endif

extern int nf_hw_get_alarm_out_nr(void);

/************************* Device Open  ****************************/
/**
		@brief						Open device driver
		@return		fd				return device ID Number
*/

gboolean nf_dev_init(void)
{
	int i;
	
	for(i=0;i<DEV_FD_IDX_NR; ++i)
	{
		_Fd_Arr[i] = -1;
	}
		
#if defined(USE_DEV_DECODER)
	nf_dev_open_decoder();	
#endif /* defined(USE_DEV_DECODER) */

#if defined(USE_DEV_DECODER_MINOR1)
	nf_dev_open_decoder_minor1();
#endif

#if defined(USE_DEV_DECODER_MINOR2)
	nf_dev_open_decoder_minor2();
#endif

#if defined(USE_DEV_DECODER_MINOR3)
	nf_dev_open_decoder_minor3();
#endif

#if defined(USE_DEV_AM8816)
	nf_dev_open_am8816();
#endif /* defined(USE_DEV_AM8816) */

#if defined(USE_DEV_KEYPAD)
	nf_dev_open_keypad();
#endif /* defined(USE_DEV_KEYPAD) */

#if defined(USE_DEV_JOG)
	nf_dev_open_jog();
#endif /* defined(USE_DEV_JOG) */

#if defined(USE_DEV_SHUTTLE)
	nf_dev_open_shuttle();
#endif /* defined(USE_DEV_SHUTTLE) */

#if defined(USE_DEV_REMOCON)
	nf_dev_open_remocon();
#endif /* defined(USE_DEV_ALARM_IN) */

#if defined(USE_DEV_ALARM_IN)
	nf_dev_open_sensor();
#endif /* defined(USE_DEV_ALARM_IN) */

#if defined(USE_DEV_ALARM_OUT)
	nf_dev_open_relay();
#endif /* defined(USE_DEV_ALARM_OUT) */

#if defined(USE_DEV_MICOM)
	nf_dev_open_micom();
#endif /* defined(USE_DEV_MICOM) */

#if defined(USE_DEV_BOARD_PP)
	nf_dev_open_board_pp();
#endif /* defined(USE_DEV_BOARD_PP) */

#if defined(USE_DEV_WATCHDOG)
	nf_dev_open_watchdog();
#endif /* defined(USE_DEV_WATCHDOG) */

#if defined(USE_DEV_RS485)
    nf_dev_open_rs485();
#endif /* defined(USE_DEV_FS1648) */

#if defined(USE_DEV_FS1648)
	nf_dev_open_fs1648();
#endif /* defined(USE_DEV_FS1648) */

#if defined(USE_DEV_CD22M3492)
	nf_dev_open_spot_anf();
#endif /* defined(USE_DEV_ANALOG_SPOT) */

#if defined(USE_DEV_SOLO6x10)
	nf_dev_open_solo_vin();
#endif /* defined(USE_DEV_SOLO6x10) */

#if defined(USE_DEV_GENNUM) 
	nf_dev_open_gennum();

	nf_dev_open_gennum_minor1();
#endif /* defined(USE_DEV_GENNUM) */

#if defined(USE_DEV_TPS2384) 
	nf_dev_open_tps2384();
#endif /* defined(USE_DEV_TPS2384) */

#if defined(USE_DEV_TLV320AIC23)
	nf_dev_open_tlv320aic23();
#endif	/* defined(USE_DEV_TLV320AIC23) */

#if defined(USE_DEV_EDID)
	nf_dev_open_edid();
#endif  /* defined(USE_DEV_EDID) */

#if defined(USE_DEV_TW2828)
	nf_dev_open_tw2828();
#endif
 // KJH
/*
#if defined(USE_DEV_DSP)
	nf_dev_open_dsp_read();
	nf_dev_open_dsp_write();
#endif
*/

#if defined(USE_DEV_IVR_HF)
	nf_dev_open_ivr_hf();
	nf_dev_open_ivr_hf_minor1();
#endif

#if defined(USE_DEV_DRV_EXT)
	nf_dev_open_drv_ext();
#endif

#if defined(USE_DEV_POE)
	nf_dev_open_poe();
#endif

#if defined(USE_DEV_SWITCH)
	nf_dev_open_switch();
#endif

	return 1;
}


#if defined(USE_DEV_IVR_HF)
gint nf_dev_open_ivr_hf(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_IVR_HF];
	
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
		
	fd = Open(DEVNAME_IVR_HF, O_RDWR, 0);

	if(fd < 0){
		printf("IVNET DEVICE OPEN Error... fd[%d]", fd);
	}else{
		printf("IVNET DEVICE OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_IVR_HF] = fd;
	}
	
	return fd;
}


gint nf_dev_open_ivr_hf_minor1(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_IVR_HF_MINOR1];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
		printf("DEV_FD_IDX_IVR_HF_MINOR1 @@@@@@@@@");
	fd = Open(DEVNAME_IVR_HF_MINOR1, O_RDWR, 0);
				
	if(fd < 0){
		printf("IVNET DEVICE MINOR1 OPEN Error... fd[%d]", fd);
	}else{
		printf("IVNET DEVICE MINOR1 OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_IVR_HF_MINOR1] = fd;
	}
	
	return fd;
}
#endif

#if  defined(USE_DEV_DECODER)
gint nf_dev_open_decoder(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DECODER];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DECODER, O_RDWR, 0);

	if(fd < 0){
		g_warning("DECODER DEVICE OPEN Error... fd[%d]", fd);
	}else{
		g_message("DECODER DEVICE OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DECODER] = fd;
	}
	
	return fd;
}
#endif

#if defined(USE_DEV_DECODER_MINOR1)
gint nf_dev_open_decoder_minor1(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DECODER_MINOR1, O_RDWR, 0);

	if(fd < 0){
		g_warning("DECODER DEVICE MINOR1 OPEN Error... fd[%d]", fd);
	}else{
		g_message("DECODER DEVICE MINOR1 OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1] = fd;
	}
	
	return fd;
}
#endif

#if defined(USE_DEV_DECODER_MINOR2)
gint nf_dev_open_decoder_minor2(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DECODER_MINOR2];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DECODER_MINOR2, O_RDWR, 0);

	if(fd < 0){
		g_warning("DECODER DEVICE MINOR2 OPEN Error... fd[%d]", fd);
	}else{
		g_message("DECODER DEVICE MINOR2 OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2] = fd;
	}
	
	return fd;
}
#endif

#if defined(USE_DEV_DECODER_MINOR3)
gint nf_dev_open_decoder_minor3(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DECODER_MINOR3];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DECODER_MINOR3, O_RDWR, 0);

	if(fd < 0){
		g_warning("DECODER DEVICE MINOR3 OPEN Error... fd[%d]", fd);
	}else{
		g_message("DECODER DEVICE MINOR3 OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3] = fd;
	}
	
	return fd;
}
#endif

gint nf_dev_open_dsp_read(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DSP_READ];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DSP, O_RDONLY, 0);

	if(fd < 0){
		g_warning("DSP Read DEVICE OPEN Error... fd[%d]", fd);
	}else{
		g_message("DSP Read DEVICE OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DSP_READ] = fd;
	}
	
	return fd;
}

gint nf_dev_open_dsp_write(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DSP_WRITE];
			
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
				
	fd = Open(DEVNAME_DSP, O_WRONLY | O_NONBLOCK , 0);

	if(fd < 0){
		g_warning("DSP WRITE DEVICE OPEN Error... fd[%d]", fd);
	}else{
		g_message("DSP WRITE DEVICE OPEN...	   fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DSP_WRITE] = fd;
	}
	
	return fd;
}

#if 0	// choissinf 2008-11-21  "/dev/dsp"�� nmf���� ���� ����
gint nf_dev_open_twaudio(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_TWAUDIO];

	g_return_val_if_fail( fd<= 0 , -1);
	
	fd = Open(DEVNAME_TWAUDIO, O_RDWR, 0);

	if(fd < 0) 
		g_warning("TWAUDIO DEVICE OPEN Error... fd[%d]", fd);
    else{
		g_message("TWAUDIO DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_TWAUDIO] = fd;
	}

    return fd;
}
#endif

gint nf_dev_open_am8816(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_AM8816];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_AM8816, O_RDWR, 0);

	if(fd < 0){
		g_warning("AM8816 DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("AM8816 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_AM8816] = fd;
	}

	return fd;
}

gint nf_dev_open_keypad(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_KEYPAD];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_KEYPAD, O_RDWR, 0);

	if(fd < 0){
		g_warning("KEYPAD DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("KEYPAD DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_KEYPAD] = fd;
	}

	return fd;
}

gint nf_dev_open_jog(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_JOG];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_JOG, O_RDWR, 0);
	if(fd < 0){
		g_warning("JOG DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("JOG DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_JOG] = fd;
	}

	return fd;
}
gint nf_dev_open_shuttle(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_SHUTTLE];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_SHUTTLE, O_RDWR, 0);

	if(fd < 0){
		g_warning("SHUTTLE DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("SHUTTLE DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_SHUTTLE] = fd;
	}

	return fd;
}

gint nf_dev_open_remocon(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_REMOCON];

	g_return_val_if_fail( fd == -1 , fd);

	if (fd != -1) 
	{
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	#if defined(CHIP_NVT)
		#if defined(CHIP_NVT_NT9833x)	// after.. change NF51090 .. this ueses only nt9833x
			// code/sample/ir_rc_tes/ir_rc_test.c
			gint fd_tmp=0, ret=0;
			char protocol[] = "nec";

			/* Configure as NEC protocol and enable */
			fd_tmp = open("/sys/class/rc/rc0/protocols", O_WRONLY);
			if (fd_tmp < 0 ) {
				printf("failed to open /sys/class/rc/rc0/protocols\n");
				return fd;
			}

			ret = write(fd_tmp, protocol, sizeof(protocol));
			if (ret < 0) {
				printf("failed to write protocols\n", ret);
			}

			close(fd_tmp);
		#endif

		/* Waiting for input events sent by IR */
		fd = Open(DEVNAME_REMOCON, O_RDONLY, 0);
	#else
		fd = Open(DEVNAME_REMOCON, O_RDWR, 0);
	#endif

	if (fd < 0)
	{
		g_warning("REMOCON DEVICE OPEN Error... fd[%d]", fd);
	}
	else
	{
		g_message("REMOCON DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_REMOCON] = fd;
	}

	return fd;
}

gint nf_dev_open_sensor(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_ALARM_IN];
	
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_ALARM_IN, O_RDWR, 0);

    if(fd < 0){
        g_warning("SENSOR DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("SENSOR DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_ALARM_IN] = fd;
	}

	return fd;
}

gint nf_dev_open_relay( void )
{
	gint fd = _Fd_Arr[DEV_FD_IDX_ALARM_OUT];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_ALARM_OUT, O_RDWR, 0);

    if(fd < 0){
        g_warning("Alarm Out Device open error... fd[%d]", fd);
	}
    else{
		g_message("Alarm Out DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_ALARM_OUT] = fd;
	}
	
	return fd;
}

#if defined(USE_DEV_MICOM)
gint nf_dev_open_micom( void )
{
	gint fd = _Fd_Arr[DEV_FD_IDX_MICOM];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_MICOM, O_RDWR, 0);

	if(fd < 0){
		g_warning("RELAY Device open error... fd[%d]", fd);
	}
	else{
		g_message("RELAY DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_MICOM] = fd;
	}

	return fd;
}
#endif

gint nf_dev_open_board_pp(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_BOARD_PP];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_BOARD_PP, O_RDWR, 0);
	if(fd < 0){
		g_warning("BOARD_PP device open error... fd[%d]", fd);
	}
    else{
		g_message("BOARD_PP DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_BOARD_PP] = fd;
	}

	return fd;
}

gint nf_dev_open_watchdog(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_WATCHDOG];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_WATCHDOG, O_RDWR,0);

	if(fd < 0){
		g_warning("WATCHDOG device open error... fd[%d]", fd);
	}
    else{
		g_message("WATCHDOG DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_WATCHDOG] = fd;
	}
	return fd;
}

#if 1	// choissinf 2008-12-11 ptz, keyctrl���� ���� ����
gint nf_dev_open_rs485(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_RS485];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}
	
	fd = Open(DEVNAME_RS485, O_RDWR,0);

	if(fd < 0){
		g_warning("RS485 device open error... fd[%d]", fd);
	}
    else{
		g_message("RS485 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_RS485] = fd;
	}
	return fd;
}
#endif


gint nf_dev_open_fs1648(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_FS1648];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_FS1648, O_RDWR, 0);

	if(fd < 0){
		g_warning("FS1648 DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("FS1648 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_FS1648] = fd;
	}

	return fd;
}

gint nf_dev_open_spot_anf(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_SPOT_ANF];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_SPOT_ANF, O_RDWR, 0);

	if(fd < 0){
		g_warning("SPOT_ANF DEVICE OPEN Error... fd[%d]", fd);
	}
    else{
		g_message("SPOT_ANF DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_SPOT_ANF] = fd;
	}

	return fd;
}


gint nf_dev_open_solo_vin(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_SOLO_VIN];

	g_message("%s called",__FUNCTION__);

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_SOLO_VIN, O_RDWR, 0);

	if(fd < 0){
		g_warning("SOLO_VIN DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("SOLO_VIN DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_SOLO_VIN] = fd;
	}

	return fd;
}

#if defined(USE_DEV_GENNUM)
gint nf_dev_open_gennum(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_GENNUM];

	g_message("%s called",__FUNCTION__);

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_GENNUM, O_RDWR, 0);

	if(fd < 0){
		g_warning("GENNUM DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("GENNUM DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_GENNUM] = fd;
	}

	return fd;
}

gint nf_dev_open_gennum_minor1(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_GENNUM_MINOR1];

	g_message("%s called fd",__FUNCTION__);

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_GENNUM_MINOR1, O_RDWR, 0);

	if(fd < 0){
		g_warning("GENNUM DEVICE MINOR1 OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("GENNUM DEVICE MINOR1 OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_GENNUM_MINOR1] = fd;
	}

	return fd;
}
#endif	/* defined(USE_DEV_GENNUM) */

#if defined(USE_DEV_TPS2384)
gint nf_dev_open_tps2384(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_TPS2384];

	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_TPS2384, O_RDWR, 0);

	if(fd < 0){
		g_warning("TPS2384 DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("TPS2384 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_TPS2384] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_TLV320AIC23)
gint nf_dev_open_tlv320aic23(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_TLV320AIC23];

	g_message("%s called\n", __FUNCTION__);
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_TLV320AIC23, O_RDWR, 0);

	if(fd < 0){
		g_warning("TLV320AIC23 DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("TLV320AIC23 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_TLV320AIC23] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_EDID)
gint nf_dev_open_edid(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_EDID];

	g_message("%s called\n", __FUNCTION__);
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_EDID, O_RDWR, 0);

	if(fd < 0){
		g_warning("EDID DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("EDID DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_EDID] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_TW2828)
gint nf_dev_open_tw2828(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_TW2828];
	
	g_message("%s called\n", __FUNCTION__);
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_TW2828, O_RDWR, 0);

	if(fd < 0){
		g_warning("TW2828 DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("TW2828 DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_TW2828] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_DRV_EXT)
gint nf_dev_open_drv_ext(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_DRV_EXT];
	
	g_message("%s called\n", __FUNCTION__);
	g_return_val_if_fail( fd == -1 , fd);
	if( fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_DRV_EXT, O_RDWR, 0);

	if(fd < 0){
		g_warning("DRV_EXT DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("DRV_EXT DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_DRV_EXT] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_POE)
gint nf_dev_open_poe(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_POE];

	g_return_val_if_fail(fd == -1, fd);

	if (fd != -1) {
		g_message("%s already opened fd[%d]",__FUNCTION__, fd);
		return fd;
	}

	fd = Open(DEVNAME_POE, O_RDWR, 0);

	if(fd < 0){
		g_warning("POE DEVICE OPEN Error... fd[%d]", fd);
	}
	else {
		g_message("POE DEVICE OPEN...       fd[%d]", fd);
		_Fd_Arr[DEV_FD_IDX_POE] = fd;
	}

	return fd;
}
#endif

#if defined(USE_DEV_SWITCH)
gint nf_dev_open_switch(void)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_SWITCH];
	// #if defined(_IPX_0412M4) || defined(_IPX_0412M4E)
	// 	char name[]="ip175d";
	// #else
	char name[]="ip1819";
	// #endif

	g_return_val_if_fail(fd == -1, fd);

	if (fd != -1) {
		printf("[%s] already opened fd[%d]\n", __FUNCTION__, fd);
		return fd;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0) {
		printf("[%s] SWITCH DEVICE OPEN Error... fd[%d]\n", __FUNCTION__, fd);
	} else {
		printf("[%s] SWITCH DEVICE OPEN...       fd[%d]\n", __FUNCTION__, fd);
		_Fd_Arr[DEV_FD_IDX_SWITCH] = fd;
	}

	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

	return fd;
}
#endif

#if defined(USE_DEV_KEYPAD)
/******************************    KEYPAD   ****************************/
/**
		@brief					keypad device enable
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_dev_enable(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_ENABLE);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif
	return (ret ==0) ? 1:0;
}

/**
		@brief					keypad device disable
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_dev_disable(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_DISABLE);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret ==0) ? 1:0;
}

gboolean nf_dev_keypad_is_valid_led(guint led)
{
	gboolean ret = FALSE;
	
	switch(led)
	{
		case ITX_LED_POWER:
		case ITX_LED_REC:			
		case ITX_LED_NET:
		case ITX_LED_ALARM:
		case ITX_LED_HDD1: 
		case ITX_LED_HDD2:
		case ITX_LED_HDD3: 
		case ITX_LED_HDD4: 
		case ITX_LED_HDD5: 
		case ITX_LED_ESATA:
			ret = TRUE;
			break;
		default:
			ret = FALSE;
			break;
	}

	return ret;
}

/**
		@brief					led on
		@param[in]	led			led status range 0~39
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_led_on(guint led)
{
	gint ret=0;
	unsigned long long led_mask = 0;
	unsigned int lo = 0;
	unsigned int hi = 0;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);
	g_return_val_if_fail( led <=LED_INDEX_MAX_NUM, 0);

	if( nf_dev_keypad_is_valid_led(led) == FALSE )	return 0;

	#ifdef OLD_LED_ONOFF
		{
			if(led < 32){
				lo = (1<<led);
			}
			else{
				led = led %32;
				hi = (1<<led);	
			}

			((unsigned int*)&led_mask)[0] = lo;
			((unsigned int*)&led_mask)[1] = hi;

			ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_ON, &led_mask);

			if(ret <0) {
				#if defined(DEBUG_IOCTL)
					g_warning("%s Ioctl Error ret[%d] led[%ld]", __FUNCTION__, ret, led);
				#else
					;
				#endif
			}else {
				#ifdef	DEBUG_LED_ONOFF_MSG		
					g_message("%s LED ON !!! led[%ld]", __FUNCTION__, led);
				#endif		
			}
		}
	#else // #ifdef OLD_LED_ONOFF

		ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_ON, &led);

		if(ret <0) {
			#if defined(DEBUG_IOCTL)
				g_warning("%s Ioctl Error ret[%d] led[%d]", __FUNCTION__, ret, led);
			#endif
		}else {
			#ifdef	DEBUG_LED_ONOFF_MSG
				g_message("%s LED ON !!! led[%d]", __FUNCTION__, led);
			#endif
		}

	#endif //#ifdef OLD_LED_ONOFF

	return (ret ==0) ? 1:0;
}

/**
		@brief					led on and off
		@param[in]	led			bit mask value
		@param[in]	is_on		1 : on , 0 : off
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_led_on_off(unsigned long long led, gint is_on)
{
	gint ret=0;
	unsigned long long val=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);
	
	val = led;
	
	if(is_on)
		ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_ON, &val);
	else
		ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_OFF, &val);
	
	#if defined(DEBUG_IOCTL)
		if(ret <0)
			g_warning("%s Ioctl Error ret[%d] led[%llx]", __FUNCTION__, ret, val);
	#endif

	return (ret ==0) ? 1:0;
}

/**
		@brief					led off
		@param[in]	led			led status range 0~39
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_led_off(guint led)
{
	if( nf_dev_keypad_is_valid_led(led) == FALSE )	return 0;
	
	#ifdef OLD_LED_ONOFF
		gint ret=0;
		unsigned long long led_mask = 0;
		unsigned int lo = 0;
		unsigned int hi = 0;

		g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);
		g_return_val_if_fail( led <= LED_INDEX_MAX_NUM, 0);

		if(led < 32){
			lo = (1<<led);
		}
		else{
			led = led %32;
			hi = (1<<led);	
		}
		
		((unsigned int*)&led_mask)[0] = lo;
		((unsigned int*)&led_mask)[1] = hi;

		ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_OFF, &led_mask);

		if(ret <0) {
			#if defined(DEBUG_IOCTL)
				g_warning("%s Ioctl Error ret[%d] led[%d]", __FUNCTION__, ret, led);
			#endif
		} else {
			#ifdef	DEBUG_LED_ONOFF_MSG
				g_message("%s LED OFF !!! led[%d]", __FUNCTION__, led);
			#endif
		}
	#else // #ifdef OLD_LED_ONOFF
		gint ret=0;

		g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);
		// g_return_val_if_fail( led <=40, 0);
		
		ret = Ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_LED_OFF, &led);

		if(ret <0) {
			#if defined(DEBUG_IOCTL)
				g_warning("%s Ioctl Error ret[%d] led[%d]", __FUNCTION__, ret, led);
			#endif
		} else {
			#ifdef	DEBUG_LED_ONOFF_MSG
				g_message("%s LED OFF !!! led[%d]", __FUNCTION__, led);
			#endif
		}
	#endif //#ifdef OLD_LED_ONOFF

	return (ret ==0) ? 1:0;
}
#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_led_cmd_help[] = "on  val\n"
											"    off val\n";
static int nf_dev_jbshell_led_cmd(int argc, char **argv)
{
	guint val=0;

	if(argc < 3)
		goto nf_dev_led_msg;

	if(strcmp(argv[1], "on") == 0)
	{
		val=(guint)strtoul(argv[2], NULL, 10);
		nf_dev_keypad_led_on(val);
	}
	else if(strcmp(argv[1], "off") == 0)
	{
		val=(guint)strtoul(argv[2], NULL, 10);
		nf_dev_keypad_led_off(val);
	}
	else
		goto nf_dev_led_msg;
	
	return 0;

nf_dev_led_msg:
	printf("Invalid arguments\n%s\n", nf_dev_jbshell_led_cmd_help);
	return -1;
}
__commandlist(nf_dev_jbshell_led_cmd, "led", nf_dev_jbshell_led_cmd_help, nf_dev_jbshell_led_cmd_help);
#endif

/**
		@brief					led all off
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_led_all_off(void)
{
	nf_dev_keypad_led_off(ITX_LED_POWER);
	nf_dev_keypad_led_off(ITX_LED_REC);
	nf_dev_keypad_led_off(ITX_LED_NET);
	nf_dev_keypad_led_off(ITX_LED_ALARM);
	nf_dev_keypad_led_off(ITX_LED_HDD1);
	nf_dev_keypad_led_off(ITX_LED_HDD2);
	nf_dev_keypad_led_off(ITX_LED_HDD3);
	nf_dev_keypad_led_off(ITX_LED_HDD4);
	nf_dev_keypad_led_off(ITX_LED_HDD5);
	nf_dev_keypad_led_off(ITX_LED_ESATA);

	return TRUE;
}

gboolean
nf_dev_keypad_led_all_on(void)
{
	nf_dev_keypad_led_on(ITX_LED_POWER);	g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_REC);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_NET);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_ALARM);	g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_HDD1);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_HDD2);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_HDD3);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_HDD4);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_HDD5);		g_usleep(1000*250);
	nf_dev_keypad_led_on(ITX_LED_ESATA);

	return TRUE;
}
/**
		@brief					KEYPAD BUZZER ON
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_beep_on(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_BUZZER_ON);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	} else {
		g_message("%s Keypad buzzer on", __FUNCTION__);
	}
#endif

	return (ret ==0) ? 1:0;
}

/**
		@brief					KEYPAD BUZZER OFF
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_beep_off(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_BUZZER_OFF);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	} else {
		g_message("%s Keypad buzzer off", __FUNCTION__);
	}
#endif

	return (ret ==0) ? 1:0;
}

/**
		@brief					KEYPAD Input Count
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_keypad_input_cnt(unsigned int *input_cnt)
{
	gint ret=0;

#ifdef	DEBUG_DISABLE_DRIVER
	return 1;
#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);
	g_return_val_if_fail(input_cnt , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD],  KEYPAD_INPUT_COUNT, input_cnt);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	} 
#endif

	return (ret ==0) ? 1:0;
}

/**
		@brief					keypad set pt_type 
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_dev_keypad_set_pt_type(struct keypad_name *_pt_type)
{
	gint ret=0;
	NF_PARAM_HW hwparam;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_SET_PT_TYPE, _pt_type);
	
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	else
	{
		memset(&hwparam, 0x00, sizeof(NF_PARAM_HW));
		ret = nf_api_param_hw_get_protect(&hwparam, 0);
		if(!ret)
		{
			g_warning("%s HW_PARAM GET ERROR", __FUNCTION__);
			return FALSE;
		}
		memset(hwparam.pannel_type, 0x00, NF_HW_PARAM_STR_32);
		strncpy(hwparam.pannel_type, (gchar *)_pt_type->name, NF_HW_PARAM_STR_32);
		ret = nf_api_param_hw_set(&hwparam);
		if(!ret)
		{
			g_warning("%s HW_PARAM SET ERROR", __FUNCTION__);
			return FALSE;
		}
		return TRUE;
	}

	return (ret ==0) ? 1:0;
}

/**
		@brief					keypad pt_type list info get
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean 
nf_dev_keypad_get_pt_info(struct keypad_pt_list *_pt_info)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_KEYPAD]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_GET_PT_LIST, _pt_info);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret ==0) ? 1:0;
}

gboolean
nf_dev_keypad_get_is_keypower(unsigned int *is_keypower)
{
	gint ret=0;
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_GET_PT_IS_KEYPOWER, is_keypower);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret ==0) ? 1:0;
}

gboolean
nf_dev_keypad_get_pt_is_key(unsigned int *is_key)
{
	gint ret=0;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_GET_PT_IS_KEY, is_key);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret ==0) ? 1:0;
}

guint
nf_dev_keypad_check_pt_key(unsigned int key_idx)
{
	gint ret = 0;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_KEYPAD], KEYPAD_CHECK_PT_KEY, &key_idx);

#if defined(DEBUG_IOCTL)
	if(ret < 0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return key_idx;
}
#endif /* defined(USE_DEV_KEYPAD) */



#if defined(USE_DEV_JOG)
/******************************   jog   ****************************/
/**
		@brief					jog device enable/disable
		@param[in]	is_enable	1 : on , 0 : 0ff
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_jog_enable(gint is_enable)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_JOG]>0 , 0);
	g_return_val_if_fail(is_enable == 1 || is_enable ==0, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_JOG], JOG_ONOFF, &is_enable);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] is_enable [%d]",
					__FUNCTION__, ret, is_enable);
#endif

	return (ret ==0) ? 1:0;
}
#endif /* defined(USE_DEV_JOG) */





#if defined(USE_DEV_SHUTTLE)
/***************************  shuttle   ****************************/
/**
		@brief					shuttle device enable/disable
		@param[in]	is_enable	1 : on , 0 : 0ff
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_shuttle_enable(gint is_enable)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_SHUTTLE]>0 , 0);
	g_return_val_if_fail(is_enable == 1 || is_enable ==0, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_SHUTTLE], SHUTTLE_ONOFF, &is_enable);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] is_enable [%d]",
					__FUNCTION__, ret, is_enable);
#endif

	return (ret ==0) ? 1:0;
}
#endif /* defined(USE_DEV_SHUTTLE) */



#if defined(USE_DEV_REMOCON)
/***************************    REMOCON   ****************************/
/**
		@brief					remocon device enable/disable
		@param[in]	is_enable	1 : on , 0 : 0ff
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_remocon_enable(gint is_enable)
{
	gint ret=0;
	g_print("is_enable [%d]\n", is_enable);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_REMOCON]>0 , 0);
	g_return_val_if_fail(is_enable == 1 || is_enable ==0, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_REMOCON], RC_ONOFF, &is_enable);
	
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] is_enable [%d]",
					__FUNCTION__, ret, is_enable);
#endif

	return (ret ==0) ? 1:0;
}


/**
		@brief					REMOCON BUZZER ON
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_remocon_beep_on(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_REMOCON]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_REMOCON],  RC_BUZZER_ON);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	} else {
		g_message("%s Remocon buzzer on", __FUNCTION__);
	}
#endif

	return (ret ==0) ? 1:0;
}

/**
		@brief					REMOCON BUZZER OFF
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_remocon_beep_off(void)
{
	gint ret=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_REMOCON]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_REMOCON],  RC_BUZZER_OFF);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	} else {
		g_message("%s Remocon buzzer off", __FUNCTION__);
	}
#endif

	return (ret ==0) ? 1:0;
}

/**
		add by pakkhman 091021
		@brief                  Remocon Input Count
		@return     gboolean    %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_remocon_input_cnt(unsigned int *input_cnt)
{
	gint ret=0;

#ifdef	DEBUG_DISABLE_DRIVER
	return 1;
#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_REMOCON]>0 , 0);
	g_return_val_if_fail(input_cnt , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_REMOCON],  RC_INPUT_COUNT, input_cnt);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}
#endif

	return (ret ==0) ? 1:0;
}


#endif /* defined(USE_DEV_REMOCON) */

static GStaticMutex remocon_repeat_lock = G_STATIC_MUTEX_INIT;
static unsigned int remocon_repeat_flag = 0;
static struct timeval rc_update_time;

unsigned int nf_dev_remocon_get_repeat_flag(void)
{
	unsigned int is_repeat = 0;

    g_static_mutex_lock (&remocon_repeat_lock);
	is_repeat = remocon_repeat_flag;
	g_static_mutex_unlock (&remocon_repeat_lock);

	return is_repeat;
}

void nf_dev_remocon_set_repeat_flag(unsigned int is_repeat)
{
    g_static_mutex_lock (&remocon_repeat_lock);
	remocon_repeat_flag = is_repeat;
	g_static_mutex_unlock (&remocon_repeat_lock);
}

void nf_dev_remocon_set_repeat_time(struct timeval *tv) 
{
    g_static_mutex_lock (&remocon_repeat_lock);
	rc_update_time = *tv;
	g_static_mutex_unlock (&remocon_repeat_lock);	
}

void nf_dev_remocon_get_repeat_time(struct timeval *tv)
{
    g_static_mutex_lock (&remocon_repeat_lock);
	*tv = rc_update_time;
	g_static_mutex_unlock (&remocon_repeat_lock);
}

#if defined(USE_DEV_MIC)

#define DEBUG_MIC_API

volatile static guint _mic_out_mask = 0;
volatile static gboolean _mic_out_onoff = 0;

/**
		@brief						get mic bind
		@param[out]	ch				
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
guint
nf_dev_mic_get_output_mask( )
{	
	return _mic_out_mask;
}

gboolean
nf_dev_mic_get_onoff( )
{		
	return _mic_out_onoff;
}


/**
		@brief						set mic onoff
		@param[in]	onoff						
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_mic_set_output_mask( guint mask )
{
	gboolean ret = TRUE;

#ifdef DEBUG_MIC_API
	g_message("%s mask[%08x]", __FUNCTION__, mask);
#endif
	
	_mic_out_mask = mask;
	
	return 1;
}

gboolean
nf_dev_mic_set_onoff( gboolean onoff )
{
	gboolean ret = TRUE;

#ifdef DEBUG_MIC_API
	g_message("%s on[%d]", __FUNCTION__, onoff);
#endif

	_mic_out_onoff =  onoff;

#if defined(USE_TLV320AIC23)
	return nf_dev_audio_live_onoff_ctrl(onoff);
#else 
	return 1;
#endif	
	
}

#endif



#if defined(USE_DEV_AUDIO)

volatile static guint _audio_out_ch =  NF_AUDIO_DAC_PLAYBACK;
volatile static guint _audio_out_is_netrx = 0;
extern int nf_ipcam_set_live_audio_ch(gint ch);

guint nf_dev_audio_get_ch(void)
{
	if( _audio_out_is_netrx )
		return NF_AUDIO_DAC_PLAYBACK;
	else 		
	return 	_audio_out_ch;
}

/**
		@brief						set live audio
		@param[in]	ch				select channel (if val 0xff, playback)
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_audio_set_dac( guint ch)
{
	gboolean ret = TRUE;
	guint mode = 0;

#ifdef DEBUG_AUDIO_API
	g_message("%s ch[%d]", __FUNCTION__, ch);
#endif

	_audio_out_ch = ch;	

	if( _audio_out_is_netrx ){
		nf_ipcam_set_live_audio_ch( 0xff );
	} else {
		nf_ipcam_set_live_audio_ch( (gint)_audio_out_ch );
	}

	#if defined(USE_DEV_DECODER)
		if( _audio_out_is_netrx ) {
			if(nf_dev_decoder_get_dac() != NF_DEV_DECODER_DAC_PLAYBACK)
			{
				nf_dev_decoder_set_dac(NF_DEV_DECODER_DAC_PLAYBACK);
				#if defined (USE_DEV_DSP)
					nf_dev_audio_dsp_reset(0); // WRITE FD RESET
				#endif
			}
		} else {
			nf_dev_decoder_set_dac(_audio_out_ch);
			#if defined (USE_DEV_DSP)
				nf_dev_audio_dsp_reset(0); // WRITE FD RESET
			#endif
		}
	#endif

	return 1;
}


gboolean
nf_dev_audio_set_netrx( gboolean on_off)
{	
	gboolean ret = TRUE;
			
	guint prev_tx = _audio_out_is_netrx;
	
	_audio_out_is_netrx = (guint)on_off;	
	

	if( on_off ) {
		nf_ipcam_set_live_audio_ch( 0xff );
	} else {
		nf_ipcam_set_live_audio_ch( (gint)_audio_out_ch );
	}

	#if defined(USE_DEV_DECODER)
		if( on_off ) {
			if(nf_dev_decoder_get_dac() != NF_DEV_DECODER_DAC_PLAYBACK)
				nf_dev_decoder_set_dac(NF_DEV_DECODER_DAC_PLAYBACK);
		} else {
			nf_dev_decoder_set_dac(_audio_out_ch);
		}
	#endif

	#if defined (USE_DEV_DSP)
		if( prev_tx == 0 )
			nf_dev_audio_dsp_reset(0); // WRITE FD RESET
	#endif 		
	
	return 1;
}

#endif



#if defined(USE_DEV_DECODER)
/******************************    TW2864   ****************************/
/**
		@brief					TW2864 Driver Vloss On & Off Init
		@param[in]	on			if on is 1, set 16 channel
								and	if on is 0 , not set all channel
		@return		gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_vloss_enable(gboolean is_enable)
{
	gint ret=0;
	gint val;
   	
	#ifdef DEBUG_DECODER_API
		g_message("%s on[%d]", __FUNCTION__, on);
	#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( is_enable==1 || is_enable==0, 0);        
    
	if(is_enable){
		val = 0xffff;
	}
    else{
		val = 0x0;
	}

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_VL_ONOFF, &val);

	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d] on[%d]", __FUNCTION__, ret, is_enable);
	} else {
		g_message("%s INIT OK !!! on[%d]", __FUNCTION__, is_enable);
	}

	return (ret == 0 ) ? 1:0;
}


static guint _decoder_dac_ch = NF_DEV_DECODER_DAC_PLAYBACK;
guint nf_dev_decoder_get_dac(void)
{
	return 	_decoder_dac_ch;
}

/**
		@brief						Set Live Audio
		@param[in]	ch				Select Channel (if val 0xff, playback)
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_set_dac(guint ch)
{
	gboolean ret = TRUE;
	guint mode = 0;

	#ifdef DEBUG_DECODER_API
		g_message("%s ch[%d]", __FUNCTION__, ch);
	#endif
    
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	g_return_val_if_fail( (ch < NUM_AUDIO_MAX || ch == NF_DEV_DECODER_DAC_PLAYBACK || ch == NF_DEV_DECODER_DAC_NET_AUDIO) , 0);

	if(ch == NF_DEV_DECODER_DAC_PLAYBACK){

		ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_DAC_PLAYBACK);

	}	else{

		ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_DAC_CH, &ch);
	}

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] ch[%d]", __FUNCTION__, ret, ch);

	_decoder_dac_ch = ch;

	return (ret == 0) ? 1:0;
}

/**
		@brief						Audio Dac Mute
		@param[in]	void
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_set_dac_mute(void)
{
	gboolean ret = TRUE;

	#ifdef DEBUG_DECODER_API
		g_message("%s", __FUNCTION__);
	#endif 

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_DAC_MUTE);
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	
	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_decoder_set_gain(int volume)
{
	gboolean ret = TRUE;

	#ifdef DEBUG_DECODER_API
		g_message("%s", __FUNCTION__);
	#endif 

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

#if 0
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_GAIN, &volume);
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						nf_dev_decoder_set_picture
		@param[in]	ch				ch0 ~ ch 15
		@param[in]	brightness		1~100(range)	
		@param[in]	hue				1~100		
		@param[in]	colour			1~100		
		@param[in]	contrast		1~100		
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_decoder_set_picture(guint ch, guint brightness, guint hue, 
										guint colour, guint contrast, guint sharpness)
{
#if defined(USE_DEV_DECODER_CX26828)
	gboolean ret = TRUE;
	gint chip=0, ch_real=0;
	cx26828_image_adjust set_picture;
	
	g_return_val_if_fail(ch < NUM_ACTIVE_CH, 0);

	if(ch < 8)
	{
		chip=0;
		ch_real=(gint)ch;
	}
	else
	{
		chip=1;
		ch_real=(gint)(ch - 8);
	}

	#if 0
		g_message("Chip[%d] CH[%d -> %d] Brightness[%d] Hue[%d] colour[%d] Contrast[%d]", 
				chip, ch, ch_real, brightness, hue, colour, contrast);
	#endif

	// Need Turning!!!!
	set_picture.chip = (guchar)chip;
	set_picture.chn = (guchar)ch_real;
	set_picture.hue = (guchar)hue;
	set_picture.contrast = (guchar)contrast;
	set_picture.brightness = (guchar)brightness;
	set_picture.saturation = (guchar)colour;
	set_picture.sharpness = 50;
	set_picture.item_sel = 0;/* use such as "CX26828_SET_HUE|CX26828_SET_CONTRAST" */

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_PICTURE, &set_picture);

#else
	gboolean ret = TRUE;
	struct decoder_video_picture set_picture;
	gchar tmp_key[256]={0, };

	g_return_val_if_fail(ch < NUM_ACTIVE_CH, 0);
	#ifdef DEBUG_DECODER_API
		g_message("%s ch[%d]  br[%d]hue[%d]co[%d]ct[%d]shp[%d]", 
					__FUNCTION__, ch,  brightness, hue, colour, contrast, sharpness);
	#endif
    
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( ch<16, 0);
	g_return_val_if_fail( brightness<=100, 0);
	g_return_val_if_fail( hue<=100, 0);
	g_return_val_if_fail( colour<=100, 0);
	g_return_val_if_fail( contrast<=100, 0);
	g_return_val_if_fail( sharpness<=8, 0);

	set_picture.channel = ch;
	set_picture.brightness = brightness;
	set_picture.hue = hue;
	set_picture.colour = colour;
	set_picture.contrast = contrast;
	set_picture.sharpness = sharpness;

	set_picture.idx_decoder=nf_event_d_get_decoder_idx();

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_PICTURE, &set_picture);
#endif

	if(ret <0)
    	g_warning("%s Ioctl Error ret[%d]  ch[%d]  br[%d]hue[%d]cr[%d]co[%d]",
    		__FUNCTION__, ret, ch,  brightness, hue, colour, contrast);

	return (ret == 1) ? 1:0;
}

/**
		@brief						get video loss current value
		@return		status			return video loss value
*/
gint
nf_dev_decoder_get_vloss_status(void)
{
	gboolean ret = TRUE;
	gint status;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif
    
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
    
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_GET_VIDEOLOSS, &status);
	if(ret <0) 
	    g_warning("%s Ioctl Error... status[0x%08x]", __FUNCTION__, status);

	#ifdef DEBUG_DECODER_API
	    g_message("%s [0x%08x]",__FUNCTION__, status);
	#endif

	return status;
}

/**
		@brief						Initialize Table!!
		@param[in]	is_pal			if is_pal is 1, PAL mode and if is_pal is 0 , 
									NTSC mode
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_init_tbl(gboolean is_pal)
{
	gboolean ret = TRUE;
	guint mode;

	#ifdef DEBUG_DECODER_API
		g_message("%s is_pal[%d]", __FUNCTION__, is_pal);
	#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( is_pal==1 || is_pal==0, 0);

	if(is_pal){
		mode = DECODER_SET_PAL;
	}
	else{
		mode = DECODER_SET_NTSC;
	}

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], mode);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] sig[%d] ", __FUNCTION__, ret, is_pal);

	#ifdef DEBUG_DECODER_API
		g_message("%s signal_type[%s]", __FUNCTION__, is_pal ? "PAL":"NTSC");
	#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Decoder Raster cntl
		@param[in]  chip            decoder chip is 0 or 1 or 2 or 3.
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_cntl_raster(gint idx_decoder, gint ch, gint is_enable)
{
	gboolean ret = TRUE;
	struct decoder_raster_cntl info;

	#ifdef DEBUG_DECODER_API
		g_message("%s chip[%d]", __FUNCTION__, chip);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0, 0);
	g_return_val_if_fail(ch < NUM_ACTIVE_CH, 0);
	g_return_val_if_fail(is_enable==1 || is_enable==0, 0);

	info.idx=idx_decoder;
	info.ch=ch;
	info.is_enable=is_enable;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_CNTL_RASTER, &info);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_decoder_cntl_raster_once(guint mask_xor, guint mask)
{
	gboolean ret=TRUE;
	struct decoder_raster_cntl_once info;

	#ifdef DEBUG_DECODER_API
		g_message("%s fd %d", __FUNCTION__, _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]);
	#endif

	info.mask_xor=mask_xor;
	info.mask=mask;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_CNTL_RASTER_ONCE, &info);
	if(ret <0) 
	    g_warning("%s Decoder Cntl Offset Ioctl Error...", __FUNCTION__);

	return ret;
}


/**
		@brief                      Decoder Offser Cntl
		@param[in]  
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_cntl_offset(gint idx_decoder, gint ch, gint x, gint y)
{
	gboolean ret = TRUE;

	#ifdef DEBUG_DECODER_API
		g_message("%s chip[%d]", __FUNCTION__, chip);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0, 0);
	g_return_val_if_fail(ch < NUM_ACTIVE_CH, 0);
#if 0
	info.idx=idx_decoder;
	info.ch=ch;
	info.is_enable=is_enable;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_CNTL_RASTER, &info);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						Decoder Register Dump
		@param[in]	chip			decoder chip is 0 or 1 or 2 or 3.
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_reg_dump(gint chip)
{
	gboolean ret = TRUE;

	#ifdef DEBUG_DECODER_API
		g_message("%s chip[%d]", __FUNCTION__, chip);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( chip>=0 || chip <4, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_DUMP, &chip);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] no[%d]", __FUNCTION__, ret, chip);

	#ifdef DEBUG_DECODER_API
		g_message("%s Chip Number[%d]", __FUNCTION__, chip);
	#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Decoder Register Dump
		@param[in]  chip            decoder chip is 0 or 1 or 2 or 3.
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_dump_new(gint is_read, gint idx_decoder, gint chip, gushort addr, guchar data_write, guchar *data_read)
{
	gboolean ret = TRUE;
	struct decoder_cntl_dump info;

	#ifdef DEBUG_DECODER_API
		g_message("%s line%d", __FUNCTION__, __LINE__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( chip>=0 || chip <4, 0);

	memset(&info, 0x0, sizeof(info));

	info.idx = idx_decoder;
	info.is_read = is_read;
	info.chip = chip;
	info.addr = addr;
	info.data_write = data_write;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_DUMP_NEW, &info);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] idx %d chip %d", __FUNCTION__, ret, idx_decoder, chip);

	if(is_read)
	{
		*data_read=info.data_read;
		g_message("%s data 0x%02x", __FUNCTION__, *data_read);
	}

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Decoder Control Delay (V / H)
		@param[in]  chip            decoder chip is 0 or 1 or 2 or 3.
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_cntl_delay(gint idx_decoder, gint ch, gint type_camera)
{
	gboolean ret = TRUE;
	struct decoder_cntl_delay info;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);
	g_return_val_if_fail( ch>=0 || ch <NUM_ACTIVE_CH, 0);

	memset(&info, 0x0, sizeof(info));

	info.idx = idx_decoder;
	info.ch = ch;
	info.type_camera = type_camera;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_CNTL_DELAY, &info);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d] idx %d ch %d", __FUNCTION__, ret, idx_decoder, ch);

	return (ret == 0) ? 1:0;
}

/**
		@brief						Get Current Signal Type
		@return		status			if status is 0, NTSC  if status is 1 , PAL
*/
gint
nf_dev_decoder_get_signal_type(void)
{
	gboolean ret = TRUE;
	gint status=0;

	#ifdef DEBUG_DECODER_API
		g_message("%s called", __FUNCTION__);
	#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_GET_SIGTYPE, &status);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	#ifdef DEBUG_DECODER_API
		g_message("%s signal_type[%s]", __FUNCTION__, status ? "PAL" : "NTSC");
	#endif

	return status;
}

#if defined(USE_DEV_DECODER_NVP1108B)
/**
		@brief							NVP1108B.. Register Write Byte For Debug
		@return
*/
gint
nf_dev_decoder_dbg_write_byte(guchar reg, guchar val, guchar dev, guint bank)
{
	struct decoder_debug_reg dbg;
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	dbg.dev = dev;
	dbg.addr = reg;
	dbg.data = val;
	dbg.bank = bank;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_WR_BYTE, &dbg);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#else
/**
		@brief							Write Byte For Debug
		@return
*/
gint
nf_dev_decoder_dbg_write_byte(guchar reg, guchar val)
{
	struct decoder_debug_reg dbg;
	gboolean ret = TRUE;
	guchar i=0;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	for (i=0; i<4; i++) {
		dbg.dev = i;
		dbg.addr = reg;
		dbg.data = val;

		ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_STATUS, &dbg);
	}

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#endif

/**
		@brief		Coaxial PTZ Send Function
		@param[in]	ch   : channel
					cmd  : command
					mode : send mode (BIT16_MODE : 0, BIT32_MODE : 1)
					wait : delay us
		@return
*/
gboolean
nf_dev_decoder_set_ptz_cmd(guchar ch, guchar cmd, guchar mode, guint param)
{
	gboolean ret = TRUE;
	PTZ_DATA data;
	u_int pt_speed_tmp;
	u_int proto_idx=0;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER] > 0, 0);

//	g_message("%s ch[%d] cmd[%d] mode[%d]", __FUNCTION__, ch, cmd, mode);
	if(mode == 24) // 24 : TVI PELCO-C
	{
		proto_idx = DECODER_MODE_STATE_TVI_PELCO_C;
	}

#if defined(USE_DEV_DECODER_IDEN1100)
	data.channel=ch;
	data.command=cmd;
	data.mode=mode;
	data.wait=100;

	switch(cmd)
	{
		case PTZ_PAN_LEFT	:   
		case PTZ_PAN_RIGHT	:
		case PTZ_TILT_UP	:             
		case PTZ_TILT_DOWN	:	           
		case PTZ_LEFTUP		:       
		case PTZ_LEFTDOWN	:        
		case PTZ_RIGHTUP	:	        
		case PTZ_RIGHTDOWN	:      
			pt_speed_tmp = 0xff & ((0x3f * param) / 100);
			data.param = pt_speed_tmp | (pt_speed_tmp << 8);
		break;		
		case PTZ_AUTO_FOCUS :
		case PTZ_AUTO_IRIS	:
			if(param)
				data.param = 0x00;
			else
				data.param = 0x01;
		break;
		default :
			data.param = param;
		break;
	}
#elif defined(USE_DEV_DECODER_NVP191X) || defined(USE_DEV_DECODER_NVP6124)
	gchar tmp_key[256]={0,};

	data.channel=ch;
	data.command=cmd;
	data.param=param;

	sprintf(tmp_key, "cam.C%d.analog_type", ch);

	if(nf_sysdb_get_uint(tmp_key) == NF_EVENT_ANALOG_TYPE_TVI_G)
		data.mode=DECODER_IDX_TP2802;
	else if(nf_sysdb_get_uint(tmp_key) == NF_EVENT_ANALOG_TYPE_TVI_H)
		data.mode=DECODER_IDX_TP2802;
	else if(nf_sysdb_get_uint(tmp_key) == NF_EVENT_ANALOG_TYPE_AHD)
		data.mode=DECODER_IDX_NVP6124;
	else if(nf_sysdb_get_uint(tmp_key) == NF_EVENT_ANALOG_TYPE_HDSDI)
		data.mode=DECODER_IDX_HDSDI;

#elif defined(USE_DEV_DECODER_TP2834)
	gchar tmp_key[256] = {0,};
	uint video_type = 0;
	#if defined(ENABLE_VIDEO_AUTO_DETECTION)
		NF_VIDEO_TYPE info;
	#endif
	
	data.channel = ch;
	data.command = cmd;
	data.param = param;

	#if defined(ENABLE_VIDEO_AUTO_DETECTION)
		nf_event_v_get_video_type(&info);
		
		sprintf(tmp_key, "cam.C%d.analog_type", ch);
		video_type = nf_sysdb_get_uint(tmp_key);

		if(video_type == NF_EVENT_ANALOG_TYPE_EXSDI)
		{
			g_message("%s line%d Coaxitorn ex-sdi mode", __FUNCTION__, __LINE__);
			data.mode=DECODER_MODE_STATE_HDSDI;
		}
		else
		{
			video_type = info.type_camera[ch];

			#if 0
				g_message("%s line%d ch%d video_type %d tyype %d fps %d", 
							__FUNCTION__, __LINE__, ch, video_type, info.type[ch], info.fps[ch]);
			#endif
			if((info.type[ch] == VIDEO_IN_TYPE_3M) || (info.type[ch] == VIDEO_IN_TYPE_1024x1536p))
			{
				if(video_type == NF_EVENT_VIDEO_TYPE_TVI) {
					data.mode = DECODER_MODE_STATE_TVI;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDA)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT)) {
					if(info.fps[ch] == NF_EVENT_FPS18)
						data.mode = DECODER_MODE_STATE_AHD_3M18;
					else if((info.fps[ch] == NF_EVENT_FPS25) || (info.fps[ch] == NF_EVENT_FPS30))
						data.mode = DECODER_MODE_STATE_AHD_3M25;
					else
						data.mode = DECODER_MODE_STATE_AHD;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDC)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDC_DEFAULT)) {
					data.mode = DECODER_MODE_STATE_HDC;
				}
				else {
					g_warning("%s 3M PTZ Send Unknown Type", __FUNCTION__);
				}
			}
			else if((info.type[ch] == VIDEO_IN_TYPE_4M) || (info.type[ch] == VIDEO_IN_TYPE_1344x1520P)
					|| (info.type[ch] == VIDEO_IN_TYPE_4M_QHD) || (info.type[ch] == VIDEO_IN_TYPE_1280x1440p))
			{
				if(video_type == NF_EVENT_VIDEO_TYPE_TVI) {
					data.mode = DECODER_MODE_STATE_TVI;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDA)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT)) {
					if((info.fps[ch] == NF_EVENT_FPS25) || (info.fps[ch] == NF_EVENT_FPS30))
						data.mode = DECODER_MODE_STATE_AHD_4M25;
					else if(info.fps[ch] == NF_EVENT_FPS15)
						data.mode = DECODER_MODE_STATE_AHD_4M15;
					else
						data.mode = DECODER_MODE_STATE_AHD;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDC)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDC_DEFAULT)) {
					data.mode = DECODER_MODE_STATE_HDC;
				}
				else {
					g_warning("%s 4M PTZ Send Unknown Type", __FUNCTION__);
				}
			}
			else if((info.type[ch] == VIDEO_IN_TYPE_5M) || (info.type[ch] == VIDEO_IN_TYPE_1296x1944p))
			{
				if(video_type == NF_EVENT_VIDEO_TYPE_TVI) {
					data.mode = DECODER_MODE_STATE_TVI;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDA)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT)) {
					if(info.fps[ch] == NF_EVENT_FPS20) {
						data.mode = DECODER_MODE_STATE_AHD_4M25;
					}
					else if(info.fps[ch] == NF_EVENT_FPS12) {
						data.mode = DECODER_MODE_STATE_AHD_4M15;
					}
					else {
						data.mode = DECODER_MODE_STATE_AHD;
					}
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDC)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDC_DEFAULT)) {
					data.mode = DECODER_MODE_STATE_HDC;
				}
				else {
					g_warning("%s 5M PTZ Send Unknown Type", __FUNCTION__);
				}
			}
			else if((info.type[ch] == VIDEO_IN_TYPE_8M) || (info.type[ch] == VIDEO_IN_TYPE_8M_HALF))
			{
				if(video_type == NF_EVENT_VIDEO_TYPE_TVI)
					data.mode = DECODER_MODE_STATE_TVI;
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDA)
					|| (video_type == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
					data.mode = DECODER_MODE_STATE_AHD_4M25;
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDC)
					|| (video_type == NF_EVENT_VIDEO_TYPE_HDC_DEFAULT))
					data.mode = DECODER_MODE_STATE_HDC_QHD;
				else
					g_warning("%s 8M PTZ Send Unknown Type", __FUNCTION__);
			}
			else if(info.type[ch] == VIDEO_IN_TYPE_SD)
			{
				data.mode = DECODER_MODE_STATE_CVBS;
			}
			else
			{
				if(video_type == NF_EVENT_VIDEO_TYPE_TVI) {
					data.mode = DECODER_MODE_STATE_TVI;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDA)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT)) {
					data.mode = DECODER_MODE_STATE_AHD;
				}
				else if((video_type == NF_EVENT_VIDEO_TYPE_HDC)
						|| (video_type == NF_EVENT_VIDEO_TYPE_HDC_DEFAULT)) {
					data.mode = DECODER_MODE_STATE_HDC;
				}
				else {
					g_warning("%s Default PTZ Send Unknown Type", __FUNCTION__);
				}
			}
		}

		if(data.mode == DECODER_MODE_STATE_TVI)
		{
			if(proto_idx == DECODER_MODE_STATE_TVI_PELCO_C)
				data.mode = DECODER_MODE_STATE_TVI_PELCO_C;
			else
				data.mode = DECODER_MODE_STATE_TVI;
		}
	#else
		sprintf(tmp_key, "cam.C%d.analog_type", ch);
		video_type = nf_sysdb_get_uint(tmp_key);
	#endif

	printf("[%s] video_type %d data.channel = %d, data.command = %d, data.param = %d, data.mode = %d\n", 
				__FUNCTION__, video_type, data.channel, data.command, data.param, data.mode);
#endif

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_PTZ_CMD, &data);

	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief							Decoder Set PlayBack
		@return
*/
gint
nf_dev_decoder_set_playback(void)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_SET_DAC_PLAYBACK);
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_decoder_cntl_eq(gint ch, gboolean is_enable, gint idx)
{
	gboolean ret = TRUE;
	struct decoder_cntl_eq info;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER]>0 , 0);

	memset(&info, 0x0, sizeof(info));

	info.idx=idx;
	info.ch=ch;
	info.is_enable=is_enable;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER], DECODER_CNTL_EQ, &info);
	if(ret <0)
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_decoder_cntl_scanmode(gint ch, gint mode)
{
	gboolean ret = TRUE;
	DECODER_SCANMODE info;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

   info.ch=ch;
	info.mode=mode;   

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_SET_SCANMODE, &info);
	if(ret <0) 
	    g_warning("%s Decoder Set Scanmode Ioctl Error...", __FUNCTION__);

	return ret;
}

gboolean
nf_dev_decoder_cntl_scanmode_host(gint ch, gint mode)
{
	gboolean ret = TRUE;
	DECODER_SCANMODE info;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	info.ch=ch;
	info.mode=mode;   

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_SET_SCANMODE_HOST, &info);
	if(ret <0) 
		g_warning("%s Decoder Set Scanmode Host Ioctl Error...", __FUNCTION__);

	return ret;
}

gboolean
#if defined(ENABLE_VIDEO_AUTO_DETECTION)
nf_dev_decoder_cntl_video_rescan(gint ch, gint mode, guint analog_type)
#else
nf_dev_decoder_cntl_video_rescan(gint ch, gint mode)
#endif
{
	gboolean ret = TRUE;
	DECODER_SCANMODE info;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	info.ch=ch;
	info.mode=mode;
	#if defined(ENABLE_VIDEO_AUTO_DETECTION)
		info.rsvd0=analog_type;
	#endif

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_VIDEO_RESCAN, &info);
	if(ret <0)
		g_warning("%s Decoder Set Scanmode Ioctl Error...", __FUNCTION__);

	return ret;
}

gboolean
nf_dev_decoder_cntl_get_std(gint ch, gint *resol)
{
	gboolean ret = TRUE;
	struct decoder_std info;

	#ifdef DEBUG_DECODER_API
		g_message("%s fd %d", __FUNCTION__, _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

   info.ch=ch;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_CNTL_GET_STD, &info);
	if(ret <0) 
	    g_warning("%s Decoder Get Resolution Ioctl Error...", __FUNCTION__);
	else
		*resol=info.std;

	return ret;
}

gboolean
nf_dev_decoder_cntl_get_mode(gint ch, gint *mode, gint *std)
{
	gboolean ret = TRUE;
	struct decoder_mode info;

	#ifdef DEBUG_DECODER_API
		g_message("%s fd %d", __FUNCTION__, _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	info.ch=ch;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_CNTL_GET_MODE, &info);
	if(ret <0) 
	    g_warning("%s Decoder Get Resolution Ioctl Error...", __FUNCTION__);
	else
	{
		*mode=info.mode;
		*std=info.std;
	}

	return ret;
}

#if defined(ENABLE_VIDEO_AUTO_DETECTION)
gboolean
nf_dev_decoder_cntl_video_resol(gint *mode, gchar *resol)
{
	gboolean ret = TRUE;
	struct decoder_resol info;
	gint ch=0;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	for(ch=0; ch<NUM_ACTIVE_CH; ch++)
	{
		info.mode[ch]=mode[ch];
		info.resol[ch]=resol[ch];
	}

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_VIDEO_INIT, &info);
	if(ret <0)
		g_warning("%s Decoder Set Scanmode Ioctl Error...", __FUNCTION__);

	return ret;
}
#endif

gboolean
nf_dev_decoder_cntl_disp_location(gint ch, gint x, gint y)
{
	gboolean ret = TRUE;
	struct decoder_cntl_offset info;

	#ifdef DEBUG_DECODER_API
		g_message("%s fd %d", __FUNCTION__, _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	info.ch=ch;
	info.x=x;
	info.y=y;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_CNTL_DISP_OFFSET, &info);
	if(ret <0) 
	    g_warning("%s Decoder Cntl Offset Ioctl Error...", __FUNCTION__);

	return ret;
}

gboolean
nf_dev_decoder_cntl_stop_timer(gboolean is_stop)
{
	gboolean ret = TRUE;
	struct decoder_dbg info;

	#ifdef DEBUG_DECODER_API
		g_message("%s fd %d", __FUNCTION__, _Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	info.idx_dbg=DECODER_IDX_DBG_TIMER;
	info.dbg_timer.is_stop=is_stop;
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_DEBUG, &info);
	if(ret <0) 
	    g_warning("%s Decoder Cntl Stop Timer Ioctl Error...", __FUNCTION__);

	return ret;
}

#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_decoder_cmd_help[] = "decoder live on ch(1~16)\n"
												"        live off\n"
												"        pb ch(1~16)\n";
											#if 0		// Hang!!!
												"        set_pic ch bri hue colour contrast\n";
											#endif
static int nf_dev_jbshell_decoder_cmd(int argc, char **argv)
{
	if(argc < 2)
		goto nf_dev_decoder_msg;

	if(strcmp(argv[1], "live") == 0)
	{
		if(strcmp(argv[2], "on") == 0)
		{
			guint ch=0;

			if(argc < 4)
				goto nf_dev_decoder_msg;

			ch = (guint)strtoul(argv[3], NULL, 0);
			nf_dev_decoder_set_dac(ch);
		}
		else if(strcmp(argv[2], "off") == 0)
		{
			if(argc < 3)
				goto nf_dev_decoder_msg;
		
			#if defined(USE_DEV_DECODER_IDEN1100) || defined(USE_DEV_DECODER_CX26828) || defined(USE_DEV_DECODER_NVP191X) \
				|| defined(USE_DEV_DECODER_NVP6124)
				nf_dev_decoder_set_dac_mute(TRUE);
			#else
				nf_dev_decoder_set_dac_mute();
			#endif
		}
		else
			goto nf_dev_decoder_msg;
	}
	else if(strcmp(argv[1], "pb") == 0)		// PlayBack
		nf_dev_decoder_set_dac(NF_DEV_DECODER_DAC_PLAYBACK);
	else if(strcmp(argv[1], "set_pic") == 0)
	{
		guint ch=0, bri=0, hue=0, colour=0, contrast=0, sharpness=0;
		
		if(argc < 8)
			goto nf_dev_decoder_msg;
		
		ch=(guint)strtoul(argv[2], NULL, 10);
		bri=(guint)strtoul(argv[3], NULL, 10);
		hue=(guint)strtoul(argv[4], NULL, 10);
		colour=(guint)strtoul(argv[5], NULL, 10);
		contrast=(guint)strtoul(argv[6], NULL, 10);
		sharpness=(guint)strtoul(argv[7], NULL, 10);

		g_message("CH[%d] Brightness[%d] Hue[%d] colour[%d] Contrast[%d] Sharpness[%d]", 
						ch, bri, hue, colour, contrast, sharpness);

		nf_dev_decoder_set_picture(ch, bri, hue, colour, contrast, sharpness);
	}
	else if(strcmp(argv[1], "raster") == 0)
	{
		/*
			DECODER_IDX_CX25930     = 0,
			DECODER_IDX_NVP191X     = 1,
			DECODER_IDX_NVP6124     = 2,
			DECODER_IDX_TP2802      = 3,
		*/
		gint idx=0, ch=0, is_enable=0;

		if(argc < 5)
			goto nf_dev_decoder_msg;

		idx=(gint)strtoul(argv[2], NULL, 10);
		ch=(gint)strtoul(argv[3], NULL, 10);
		is_enable=(gint)strtoul(argv[4], NULL, 10);

		nf_dev_decoder_cntl_raster(idx, ch, is_enable);
	}
	else if(strcmp(argv[1], "dump") == 0)
	{
		gint is_read=0, idx_decoder=0, chip=0;
		gushort addr=0;
		guchar data_write=0, data_read=0;
		
		if(argc < 7)
			goto nf_dev_decoder_msg;
		
		is_read=(gint)strtoul(argv[2], NULL, 10);
		idx_decoder=(gint)strtoul(argv[3], NULL, 10);
		chip=(gint)strtoul(argv[4], NULL, 10);
		addr=(gushort)strtoul(argv[5], NULL, 16);
		data_write=(guchar)strtoul(argv[6], NULL, 16);
		data_read=(guchar)strtoul(argv[7], NULL, 16);

		g_message("%s line%d is_read %d decoder_idx %d chip %d address 0x%04x data_write 0x%02x\n",
					__FUNCTION__, __LINE__, is_read, idx_decoder, chip, addr, data_write);

		nf_dev_decoder_dump_new(is_read, idx_decoder, chip, addr, data_write, &data_read);

		if(is_read)
			g_message("%s line%d [0x%x] READ DATA [0x%x]\n", __FUNCTION__, __LINE__, addr, data_read);

	}
	else if(strcmp(argv[1], "delay") == 0)
	{
		gint idx_decoder=0, ch=0, type_camera=0;
		gushort addr=0;

		if(argc < 5)
			goto nf_dev_decoder_msg;

		idx_decoder=(gint)strtoul(argv[2], NULL, 10);
		ch=(gint)strtoul(argv[3], NULL, 10);
		type_camera=(guchar)strtoul(argv[4], NULL, 10);

		g_message("%s line%d decoder_idx %d ch%d", __FUNCTION__, __LINE__, idx_decoder, ch);

		nf_dev_decoder_cntl_delay(idx_decoder, ch, type_camera);
	}
	#if defined(USE_DEV_DECODER_IDEN1100)
	else if(strcmp(argv[1], "ptz") == 0)
	{
		guchar ch=0, cmd=0, mode=0;
		
		if(argc < 5)
			goto nf_dev_decoder_msg;

		ch=(guchar)strtoul(argv[2], NULL, 16);
		cmd=(guchar)strtoul(argv[3], NULL, 16);
		mode=(guchar)strtoul(argv[4], NULL, 16);

		g_message("ptz command test -> ch[0x%02x] cmd[0x%02x] mode[0x%02x]", ch, cmd, mode);

		// Mode -> BIT16_MODE : 0 BIT32_MODE : 1
		/** PTZ Command
			PTZ_ZOOM_WIDE           (0x00)
			PTZ_ZOOM_TELE           (0x01)
			PTZ_PAN_LEFT            (0x02)
			PTZ_PAN_RIGHT           (0x03)
			PTZ_TILT_UP             (0x04)
			PTZ_TILT_DOWN           (0x05)
			PTZ_IRIS_OPEN           (0x06)
			PTZ_IRIS_CLOSE          (0x07)
		**/
		nf_dev_decoder_set_ptz_cmd(ch, cmd, mode, 0);
	}
	#endif
	else
		goto nf_dev_decoder_msg;
	
	return 0;

nf_dev_decoder_msg:
	printf("Invalid arguments\n%s\n", nf_dev_jbshell_decoder_cmd_help);
	return -1;
}
__commandlist(nf_dev_jbshell_decoder_cmd, "decoder", nf_dev_jbshell_decoder_cmd_help, nf_dev_jbshell_decoder_cmd_help);
#endif  /* defined(DEBUG_JBSHELL_UTIL_DEVICE) */
#endif	/* defined(USE_DEV_DECODER) */

#if defined(USE_DEV_DECODER_MINOR1)
#if defined(USE_DEV_DECODER_NVP1108B) || defined(USE_DEV_DECODER_IDEN1100) || defined(USE_DEV_DECODER_NVP191X) \
		|| defined(USE_DEV_DECODER_NVP6124)
/**
		@brief						Set Signal Type Check Debounce
		@param[in]	debounce		Decoder Signal Type Event Occur Debounce
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_decoder_set_signal_debounce(guint debounce)
{
	gboolean ret = TRUE;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_SET_DEBOUNCE_SIGNAL, &debounce);
	if(ret <0) 
	    g_warning("%s Ioctl Error... Debounce[%d]", __FUNCTION__, debounce);

	#ifdef DEBUG_DECODER_API
			g_message("%s debounce[%d]",__FUNCTION__, debounce);
	#endif

	return (ret == 0) ? 1:0;
}
#endif	/* defined(USE_DEV_DECODER_NVP1108B) */

/**
		@brief						Set Signal Type Check Debounce
		@return		gint			Current Signal Type Value
*/
guint
nf_dev_decoder_get_signal_status(void)
{
	gboolean ret = TRUE;
	guint status=0;

	#ifdef DEBUG_DECODER_API
		g_message("%s ", __FUNCTION__);
	#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1]>0 , 0);
 
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR1], DECODER_GET_CAMERA_SIGNAL_STATUS, &status);
	if(ret <0) 
	    g_warning("%s Ioctl Error... Signal Status[0x%08x]", __FUNCTION__, status);

	#ifdef DEBUG_DECODER_API
			g_message("%s Signal Status[0x%08x]",__FUNCTION__, status);
	#endif

	return status;
}
#endif	/* defined(USE_DEV_DECODER_MINOR1) */

#if defined(USE_DEV_DECODER_MINOR2)

#if defined(USE_DEV_DECODER_IDEN1100)
gboolean nf_dev_decoder_motion_init(gint is_on, gint ch, guint is_960, guint is_pal, 
					guint md_velocity, guint md_lvsens, guint md_spsens, guint md_tmpsens, guchar *mask_md_area)

{
	struct decoder_conf_md conf_md;
	gboolean ret = TRUE;
	
	conf_md.is_on=is_on;
	conf_md.ch=ch;
	conf_md.is_960=is_960;
	conf_md.is_pal=is_pal;
	conf_md.md_velocity=md_velocity;
	conf_md.md_lvsens=md_lvsens;
	conf_md.md_spsens=md_spsens;
	conf_md.md_tmpsens=md_tmpsens;
	
	memcpy(conf_md.mask_md_area, mask_md_area, sizeof(conf_md.mask_md_area));

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2], DECODER_SET_MD, &conf_md);
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1:0;
}
#elif defined(USE_DEV_DECODER_CX26828) || defined(USE_DEV_DECODER_NVP191X)
gboolean nf_dev_decoder_motion_init(guint is_on, gint ch, guint is_pal, guchar *mask_md_area, guint md_sense)
{
	struct decoder_conf_md conf_md;
	gboolean ret = TRUE;

	conf_md.is_on=(gint)is_on;
	conf_md.ch=ch;
	conf_md.is_pal=is_pal;
	conf_md.sens_thresh=md_sense;

	memcpy(conf_md.mask_md_area, mask_md_area, sizeof(conf_md.mask_md_area));

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2], DECODER_SET_MD, &conf_md);
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1:0;
}
#else
gboolean nf_dev_decoder_motion_init(guint is_on, gint ch, guint is_pal, guchar *mask_md_area, guint md_sense)
{
	struct decoder_conf_md conf_md;
	gboolean ret = TRUE;

	conf_md.is_on=(gint)is_on;
	conf_md.ch=ch;
	conf_md.is_pal=is_pal;
	conf_md.sens_thresh=md_sense;

	memcpy(conf_md.mask_md_area, mask_md_area, sizeof(conf_md.mask_md_area));

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2], DECODER_SET_MD, &conf_md);
	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1:0;
}
#endif	/* defined(USE_DEV_DECODER_IDEN1100) */

gboolean nf_dev_decoder_motion_enable_flag(guint flag)
{
	gboolean ret=TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2]>0, 0);
    
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR2], DECODER_SET_MOTION_FLAG_ONOFF, &flag);

	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d] Flag[0x%08x]", __FUNCTION__, ret, flag);
	} else {
		g_message("%s Enable Flag[0x%08x]", __FUNCTION__, flag);
	}

	return (ret == 0) ? 1:0;
}
#endif	/* defined(USE_DEV_DECODER_MINOR2) */



#if defined(USE_DEV_DECODER_MINOR3)

#if defined(USE_DEV_DECODER_IDEN1100)
gboolean nf_dev_decoder_tamper_init_rebl(struct decoder_conf_tamper_rebl_user *tamper_user)
{
	gboolean ret=TRUE;
	struct decoder_conf_tamper_rebl tamper_drv;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3]>0, 0);
	g_return_val_if_fail(tamper_user->ch < NUM_ACTIVE_CH, 0);

	tamper_drv.ch=tamper_user->ch;
	tamper_drv.is_on=tamper_user->is_on;

	tamper_drv.rd_lvsens=tamper_user->rd_lvsens;
	tamper_drv.rd_spsens=tamper_user->rd_spsens;
	tamper_drv.rd_tmpsens=tamper_user->rd_tmpsens;

	tamper_drv.blockage_lvsens=tamper_user->blockage_lvsens;
	tamper_drv.blockage_spsens=tamper_user->blockage_spsens;
	tamper_drv.blockage_tmpsens=tamper_user->blockage_tmpsens;

	memcpy(tamper_drv.mask_rebl, tamper_user->mask_rebl, sizeof(tamper_drv.mask_rebl));

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3], DECODER_SET_TAMPER_REBL, &tamper_drv);

	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_decoder_tamper_init_defocus(struct decoder_conf_tamper_defocus_user *tamper_user)
{
	gboolean ret=TRUE;
	struct decoder_conf_tamper_defocus tamper_drv;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3]>0, 0);
	g_return_val_if_fail(tamper_user->ch < NUM_ACTIVE_CH, 0);

	tamper_drv.ch=tamper_user->ch;
	tamper_drv.is_on=tamper_user->is_on;

	tamper_drv.defocus_lvsens=tamper_user->defocus_lvsens;
	tamper_drv.defocus_spsens=tamper_user->defocus_spsens;
	tamper_drv.defocus_tmpsens=tamper_user->defocus_tmpsens;


	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3], DECODER_SET_TAMPER_DEFOCUS, &tamper_drv);

	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_decoder_tamper_enable_flag(gboolean is_on)
{
	gboolean ret=TRUE;
	guint flag=0;
	gint ch=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3]>0, 0);
    
	if(is_on)
	{
		for(ch=0; ch<NUM_ACTIVE_CH; ch++)
			flag |= (guint)(1 << ch);
	}
	else
		flag = 0x0;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_DECODER_MINOR3], DECODER_SET_TAMPER_FLAG_ONOFF, &flag);

	if(ret <0) {
		g_warning("%s Ioctl Error ret[%d] on[%d]", __FUNCTION__, ret, flag);
	} else {
		g_message("%s Enable Flag[0x%08x]", __FUNCTION__, flag);
	}

	return (ret == 0) ? 1:0;
}

#endif

#endif	/* defined(USE_DEV_DECODER_MINOR3) */

#if defined(USE_DEV_MICOM)
gint nf_dev_micom_upgrade_flag_get(void)
{
	gint get_flag=0;
	gboolean ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_UP_FLAG_GET, &get_flag);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
		;
		#endif
	}

	return get_flag;
}
gboolean nf_dev_micom_upgrade_flag_set(gint set_flag)
{
	gboolean ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_UP_FLAG_SET, &set_flag);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d]", __FUNCTION__ , ret);
		#else
		;
		#endif
	}

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_micom_write(unsigned char addr, char *data, int length, int idx)
{
	struct micom_info info;
	gboolean ret=TRUE;
	int cnt=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	if(length > MICOM_INFO_SIZE_BUFFER) {
		return FALSE;
	}

	memset(&info, 0x0, sizeof(struct micom_info));
	info.addr=addr;
	info.length=length;
	memcpy(info.data_wr, data, length);
	info.idx=idx;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_WRITE, &info);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
			;
		#endif
	}

	return (ret == 0) ? 1:0;
}

#if 0
gboolean nf_dev_micom_write_for_spot(char *data, int length)
{
	struct micom_info info;
	gboolean ret=TRUE;
	int cnt=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	if(length > MICOM_INFO_SIZE_BUFFER) {
		return FALSE;
	}

	#if 1
		printf("======= Micom Debug Char For Sopt =======\n");
		printf("length %d data : ", length);
		for(cnt=0; cnt<length; cnt++) {
			printf("%x ", data[cnt]);
		}
		printf("\n");
	#endif

	memset(&info, 0x0, sizeof(struct micom_info));
	info.addr=MICOM_CMD_SET_SPOT;
	info.length=length;
	memcpy(info.data_wr, data, length);
	info.idx=MICOM_IDX_SPOT;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_WRITE, &info);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
			;
		#endif
	}

	return (ret == 0) ? 1:0;
}
#endif

unsigned char nf_dev_micom_getc(void)
{
	unsigned char data=0;
	gboolean ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_GETC, &data);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
			;
		#endif
	}

	return data;
}

gboolean nf_dev_micom_puts(guchar *data)
{
	gboolean ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_PUTS, data);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d]", __FUNCTION__ , ret);
		#else
		;
		#endif
	}

	return (ret == 0) ? 1:0;
}
gint nf_dev_micom_check_aprom(void)
{
	int chk=0;
	gboolean ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_APROM_CHECK, &chk);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
			 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
		;
		#endif
	}

	return chk;
}

gint nf_dev_micom_reset(void)
{
	gint ret=TRUE;
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_MICOM]>0 , 0);

	ret	= ioctl(_Fd_Arr[DEV_FD_IDX_MICOM], MICOM_RESET_GPIO);
	if(ret <0){
		#if defined(DEBUG_IOCTL)
       		 g_warning("%s Ioctl Error ret[%d] ", __FUNCTION__ , ret);
		#else
		;
		#endif
	}

	return ret;
}
#endif


#if defined(USE_DEV_ALARM_IN)
/**************************************    SENSOR   ****************************/
/**
		@brief						sensor device enable/disable
		@param[in]	on				1 : ON   0 : OFF
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_sensor_all_enable(gint is_enable)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN]>0 , 0);
	g_return_val_if_fail( is_enable==1 || is_enable==0, 0);	

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], ALARM_IN_ONOFF, &is_enable);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s INIT Error ret[%d] is_enablis_enable%d]", __FUNCTION__, ret, is_enable);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						sensor device init	
		@param[in]	on				on is 1 or 0.... 1 : ALL ON    0 : ALL OFF
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_sensor_init(gint on)
{
	gboolean ret = TRUE;
	guint mode;	
	
#ifdef DEBUG_RELAY_API
	g_message("%s on[%d]", __FUNCTION__, on);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN]>0 , 0);
	g_return_val_if_fail( on==1 || on==0, 0);	

	if(on){
		mode = ALARM_IN_SCAN_ALL_ON;
	}
	else{
		mode = ALARM_IN_SCAN_ALL_OFF;
	}

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], mode);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s INIT Error ret[%d] on[%d]", __FUNCTION__, ret, on);
#endif

#ifdef DEBUG_RELAY_API
		g_message("%s INIT OK!!! on[%d]", __FUNCTION__, on);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						set sensor channel
		@param[in]	ch				channel number
		@param[in]	on_off			1 : channel on    0 :     channel off
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_sensor_ch_onoff(gint ch, gint on_off)
{
	gboolean ret = TRUE;
	guint mode;
	char tmp_key[256];

#ifdef DEBUG_RELAY_API
	g_message("%s ch[%d] on_off[%d]", __FUNCTION__, ch, on_off);
#endif
 
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN]>0 , 0);

	g_return_val_if_fail(ch>=0 && ch<18, 0);
	
	if(on_off){
		mode = ALARM_IN_SCAN_ON;
	}
	else{
		mode = ALARM_IN_SCAN_OFF;
	}

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], mode, &ch);
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error ch[%d] on_off[%d]", __FUNCTION__, ch, on_off);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						change mode type
		@param[in]	ch_bit			channel bitfiled
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_sensor_set_type(guint ch_bit)
{
	gboolean ret = TRUE;

#ifdef DEBUG_RELAY_API
	g_message("%s ch_bit[0x%08x]", __FUNCTION__,  ch_bit);
#endif
    
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN]>0 , 0);
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], ALARM_IN_CHG_MODE , &ch_bit);

#if defined(DEBUG_IOCTL)
	if(ret <0) {
		g_warning("%s Ioctl error ret[%d] ch_bit[0x%08x]\n", 
			__FUNCTION__ , ret, ch_bit);
	}
#endif
	
	return (ret == 0) ? 1:0;
}
#if 0 
gboolean nf_dev_sensor_get_ari_panic_bit(guchar *val)
{
	gboolean ret = TRUE;
    
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN]>0 , 0);
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], ALARM_IN_SCAN_ARI_PANIC_BIT, val);
	
	if(ret <0) {
		g_warning("%s Ioctl error ret[%d]\n", 
			__FUNCTION__ , ret);
	}
	
	return (ret == 0) ? 1:0;

}
#endif
#endif /* defined(USE_DEV_ALARM_IN) */

/*
 *		@brief						Get Current Sensor Value
 *		@param[in]	curr_sensor		current sensor value
 *		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_sensor_get_curr_value(guint *curr_sensor)
{
	gboolean ret = TRUE;
	guint val = 0;

#ifdef DEBUG_RELAY_API
	g_message("%s called", __FUNCTION__);
#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_IN] > 0, 0);
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_IN], ALARM_IN_GET_CURR_VAL, &val);

#if defined (DEBUG_IOCTL)
	if (ret < 0)
	{
		g_warning("%s Ioctl error ret[%d] val[0x%08x]\n", 
				__FUNCTION__, ret, val);
	}
#endif

	*curr_sensor = val;

	return (ret == 0) ? 1 : 0;
}




#if defined(USE_DEV_ALARM_OUT)
/**************************************    RELAY   *****************************/
static GStaticMutex _nf_relay_mutex = G_STATIC_MUTEX_INIT;
static guint        _relay_onoff_bit = 0;
static guint        _relay_onoff_bit_cam = 0;

/**
		@brief						relay device init
		@param[in]	on				all on : 1    all off : 0
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_relay_init(gint on)
{
	gboolean ret=TRUE;
	guint val=0;

#ifdef DEBUG_RELAY_API
	g_message("%s on[%d]", __FUNCTION__,  on);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
	g_return_val_if_fail(on==1 || on==0, 0);

	if(on){
		val = 0xffff;
	}
    else{
		val = 0x0;
	}
	
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_SET_ALL , &val);
	
	if(ret <0)
#if defined(DEBUG_IOCTL)
		g_warning("%s Ioctl Error... ret[%d] on[%d]", __FUNCTION__ , ret, on);
#else
		;
#endif
	else
	{
		g_static_mutex_lock (&_nf_relay_mutex);
		_relay_onoff_bit = val;
		g_static_mutex_unlock (&_nf_relay_mutex);

		nf_notify_fire_params( "alarm", _relay_onoff_bit,0,0,0);
#ifdef DEBUG_RELAY_API
		g_message("%s OK!!! on[%d] val[0x%04x]", __FUNCTION__, on, val);
#endif
	}
	
	return (ret == 0) ? 1:0;
}

/**
		@brief						relay channel on
		@param[in]	ch				channel number		range : 0~15
		@param[in]	dwell_time		lasting time
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_relay_on(gint ch ,gint dwell_time)
{
	gboolean ret=TRUE;
	struct alarm_out_cmd val;

#ifdef DEBUG_RELAY_API
	g_message("%s ch[%d] dwell_time[%d]", __FUNCTION__, ch, dwell_time);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
	g_return_val_if_fail( ch>=0 && ch<NUM_RELAY, 0);

	val.ch = ch;
	val.period = dwell_time;

	ret	= Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_ON, &val);
	
	if(ret <0){
#if defined(DEBUG_IOCTL)
        g_warning("%s Ioctl Error ret[%d] ch[%d] dwell_time[%d]", 
        	__FUNCTION__ , ret, ch, dwell_time);
#else
		;
#endif
	}
	else{
		
		guint save_flag;
				
		g_static_mutex_lock (&_nf_relay_mutex);
		
		save_flag = _relay_onoff_bit;
		_relay_onoff_bit |= (guint)(1<<ch);
		save_flag ^= _relay_onoff_bit;

		g_static_mutex_unlock (&_nf_relay_mutex);
		
		if(save_flag)
			nf_notify_fire_params( "alarm", _relay_onoff_bit,0,0,0);					
	}
			
	return (ret == 0) ? 1:0;
}

/**
		@brief                      relay channel on (just one channel)
		@param[in]  ch              channel number      range : 0~15
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_relay_ch_on(gint ch)
{
	gboolean ret=TRUE;
	struct alarm_out_cmd val;

#ifdef DEBUG_RELAY_API
	g_message("%s ch[%d]", __FUNCTION__, ch);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
#if defined(ENABLE_RELAY_IPCAM)
	g_return_val_if_fail( ch>=0 && ch<NUM_RELAY_DVR, 0);
#else
	g_return_val_if_fail( ch>=0 && ch<NUM_RELAY, 0);
#endif

	val.ch = ch;
	val.period = 0x7fffffff;
	
	ret	= Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_ON, &val);

	if(ret <0){
#if defined(DEBUG_IOCTL)
        g_warning("%s Ioctl Error ret[%d] ch[%d]", 
        	__FUNCTION__ , ret, ch);
#else
		;
#endif
	}
	else{
		guint save_flag;
				
		g_static_mutex_lock (&_nf_relay_mutex);
		
		save_flag = _relay_onoff_bit;
		#if defined(ENABLE_RELAY_IPCAM)
			_relay_onoff_bit |= (guint)(1<<(ch/*+NUM_RELAY_IPCAM*/)); //0~15 ch dvr alarm(P4E32ch only) -> P4E 8/16ch = cam alarm
		#else
			_relay_onoff_bit |= (1<<ch);
		#endif
		save_flag ^= _relay_onoff_bit;

		g_static_mutex_unlock (&_nf_relay_mutex);

		if(save_flag)
			nf_notify_fire_params( "alarm", _relay_onoff_bit, _relay_onoff_bit_cam ,0 ,0);					
	}
			
	return (ret == 0) ? 1:0;
}

#if defined(ENABLE_RELAY_IPCAM)
gboolean nf_dev_relay_ch_on_ipcam(gint ch)
{
	GError **error=NULL;
	guint save_flag;

	//g_message("%s IPCAM Ch[%d] On!!", __FUNCTION__, ch);
	nf_ipcam_set_relay(ch, 1, NULL, NULL, error);

	g_static_mutex_lock (&_nf_relay_mutex);
	
	//save_flag = _relay_onoff_bit;
	//_relay_onoff_bit |= (1<<ch);
	//save_flag ^= _relay_onoff_bit;
	save_flag = _relay_onoff_bit_cam;
	_relay_onoff_bit_cam |= (guint)(1<<ch);
	save_flag ^= _relay_onoff_bit_cam;
	g_static_mutex_unlock (&_nf_relay_mutex);

	if(save_flag)
		nf_notify_fire_params( "alarm", _relay_onoff_bit, _relay_onoff_bit_cam, 0, 0);
		//nf_notify_fire_params( "alarm", _relay_onoff_bit, 0, 0, 0);
	

	return TRUE;
}

gboolean nf_dev_relay_ch_off_ipcam(gint ch)
{
	GError **error=NULL;
	guint save_flag;

	//g_message("%s IPCAM Ch[%d] Off!!", __FUNCTION__, ch);
	nf_ipcam_set_relay(ch, 0, NULL, NULL, error);

	g_static_mutex_lock (&_nf_relay_mutex);
	
	//save_flag = _relay_onoff_bit;
	//_relay_onoff_bit &= ~(1<<ch);
	//save_flag ^= _relay_onoff_bit;
	save_flag = _relay_onoff_bit_cam;
	_relay_onoff_bit_cam &= (guint)~(1<<ch);
	save_flag ^= _relay_onoff_bit_cam;
	g_static_mutex_unlock (&_nf_relay_mutex);

	if(save_flag)
		nf_notify_fire_params( "alarm", _relay_onoff_bit, _relay_onoff_bit_cam, 0, 0);
		//nf_notify_fire_params( "alarm", _relay_onoff_bit, 0, 0, 0);


	return TRUE;
}
#endif

gboolean nf_dev_relay_ari_on(void)
{
	gboolean ret=TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
	
	g_message("%s called\n", __FUNCTION__);
	ret	= ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_ARI_FLAG_ON);

#if defined(DEBUG_IOCTL)
	if(ret <0){
        g_warning("%s Ioctl Error ret[%d]", __FUNCTION__ , ret);
	}
#endif

	return (ret == 0) ? 1:0;
}
	
gboolean nf_dev_relay_ari_off(void)
{
	gboolean ret=TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
	
	g_message("%s called\n", __FUNCTION__);
	ret	= ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_ARI_FLAG_OFF);

#if defined(DEBUG_IOCTL)
	if(ret <0){
        g_warning("%s Ioctl Error ret[%d]", __FUNCTION__ , ret);
	}
#endif

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_relay_enable(void)
{
	gboolean ret=TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
	ret	= ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], DEV_RELAY_ENABLE);

#if defined(DEBUG_IOCTL)
	if(ret <0){
        g_warning("%s Ioctl Error ret[%d]", __FUNCTION__ , ret);
	}
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						relay off
		@param[in]	ch				channel number		range : 0~15
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_relay_off(gint ch)
{
	gboolean ret=TRUE;

#ifdef DEBUG_RELAY_API
	g_message("%s ch[%d]", __FUNCTION__, ch);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);
#if defined(ENABLE_RELAY_IPCAM)
	g_return_val_if_fail( ch>=0 && ch<NUM_RELAY_DVR, 0);
#else
	g_return_val_if_fail( ch>=0 && ch<NUM_RELAY, 0);
#endif

	ret	= Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_OFF, &ch);
	
	if(ret <0)
#if defined(DEBUG_IOCTL)
		g_warning("%s Ioctl Error ret[%d] ch[%d]", __FUNCTION__ , ret, ch);
#else
		;
#endif
	else
	{
		guint save_flag;
				
		g_static_mutex_lock (&_nf_relay_mutex);
		
		save_flag = _relay_onoff_bit;
		#if defined(ENABLE_RELAY_IPCAM)
			_relay_onoff_bit &= ~(1<<(ch/*+NUM_RELAY_IPCAM*/));	//0~15 ch dvr alarm(P4E32ch only) -> P4E 8/16ch = cam alarm
		#else
			_relay_onoff_bit &= ~(1<<ch);
		#endif
		save_flag ^= _relay_onoff_bit;

		g_static_mutex_unlock (&_nf_relay_mutex);
		
		if(save_flag)
			nf_notify_fire_params( "alarm", _relay_onoff_bit ,_relay_onoff_bit_cam ,0 ,0);			

	}

	return (ret == 0) ? 1:0;
}

/**
		@brief						change mode type
		@param[in]	ch_bit			channel bitfield
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_relay_set_type(guint ch_bit)
{
	gboolean ret = TRUE;

#ifdef DEBUG_RELAY_API
	g_message("%s ch_bit[0x%08x]", __FUNCTION__,  ch_bit);
#endif
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], ALARM_OUT_CHG_MODE, &ch_bit);
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] ch_bit[0x%08x]",
				__FUNCTION__ , ret, ch_bit);
#endif
	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_relay_get_type(guint *ch_bit)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_ALARM_OUT]>0 , 0);

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_ALARM_OUT], (gint)ALARM_OUT_GET_MODE, ch_bit);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] ch_bit[0x%08x]",
				__FUNCTION__ , ret, *ch_bit);
#endif
	return (ret == 0) ? 1:0;
}

#endif /* defined(USE_DEV_ALARM_OUT) */

 
#if defined(USE_DEV_BOARD_PP)
/********************************    Board_pp   *********************************/
/**
		@brief						buzzer on about 
		@param[in]
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
static GStaticMutex _nf_buzzer_mutex = G_STATIC_MUTEX_INIT;
static guint        _is_buzzer_on = FALSE;
gboolean
nf_dev_buzzer_on(void)
{
	gboolean ret = TRUE;
	struct _BOARD_PP_BUZZER_T board_pp_buzzer;	
#ifdef DEBUG_BUZZER_API
	g_message("%s called", __FUNCTION__);
#endif

#if 0
	g_warning("%s called FIXME!!!! skip ioctl call", __FUNCTION__);
	return 1;
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_BUZZER_ON);

	if(ret <0)
	{
#if defined(DEBUG_IOCTL)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
		#else
			;
#endif
	}
	else
	{
		g_static_mutex_lock (&_nf_buzzer_mutex);
		
		if(!_is_buzzer_on)
			nf_notify_fire_params("buzzer", 1, 0, 0, 0);
		_is_buzzer_on=TRUE;

		g_static_mutex_unlock (&_nf_buzzer_mutex);
	}

	return (ret == 0) ? 1:0;
}

/**
		@brief						buzzer on about type
		@param[in]					type		:	0 is a single sound.
													1 is a double sound.
		@param[in]					dwell_time	:	buzzer on duration time
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_buzzer_on_type(guint type, guint dwell_time)
{
	gboolean ret = TRUE;
	struct _BOARD_PP_BUZZER_T board_pp_buzzer;	
#ifdef DEBUG_BUZZER_API
	g_message("%s called", __FUNCTION__);
#endif
    
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
	g_return_val_if_fail(type==1 || type==0, 0);
	g_return_val_if_fail(dwell_time >0 , 0);

	board_pp_buzzer.type = type;
	board_pp_buzzer.dwell_time = dwell_time;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_BUZZER_ON_TYPE, &board_pp_buzzer);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief						buzzer off
		@param[in]
		@return		gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_buzzer_off(void)
{
	gboolean ret = TRUE;

#ifdef DEBUG_BUZZER_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_BUZZER_OFF);
	
	if(ret <0)
	{
#if defined(DEBUG_IOCTL)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
		#else
			;
#endif
	}
	else
	{
		g_static_mutex_lock (&_nf_buzzer_mutex);
		
		if(_is_buzzer_on)
			nf_notify_fire_params("buzzer", 0, 0, 0, 0);
		_is_buzzer_on=FALSE;

		g_static_mutex_unlock (&_nf_buzzer_mutex);
	}

	return (ret == 0) ? 1:0;
}

/**
		@brief                      board reset	
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_board_reset(void)
{
	gboolean ret = TRUE;
	guint on_off = 0;
#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

	// system reboot 후 micom led 초기화 위해 추가 함.
	nf_dev_micom_reset();

	#if 0
		if (nf_dev_board_reset_watchdog()) {
			return TRUE;
		} else {
			return FALSE;
		}
	#endif
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
	
	on_off=1;	
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_BOARD_RESET, &on_off);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] on_off[%d]", __FUNCTION__ , ret, on_off);
#endif

#ifdef DEBUG_BOARDPP_API
		g_message("%s Ioctl Ok... on_off[%d]", __FUNCTION__ , on_off);
#endif

	return (ret == 0) ? 1:0;
}

gboolean
nf_dev_board_reset_watchdog(void)
{
	gboolean ret = TRUE;
	gint margine=0;

	g_message("%s line%d Watchdog Reset!!!", __FUNCTION__, __LINE__);
	margine=1;
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_WATCHDOG], WDIOC_SETTIMEOUT, &margine);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return TRUE;
}

/**
		@brief                      fan on and off
		@param[in]  on_off			on_off : 0 is fan off		1 is fan on
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_board_fan_ctl(guint on_off)
{
	gboolean ret = TRUE;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	
	g_return_val_if_fail(on_off==1 || on_off==0, 0);


	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_FAN_CTL_ON_OFF, &on_off);
	   
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] on_off[%d]", __FUNCTION__ , ret, on_off);
#endif

#ifdef DEBUG_BOARDPP_API
		g_message("%s Ioctl Ok... on_off[%d]", __FUNCTION__ , on_off);
#endif
		
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      get pal or ntsc infomation
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_dev_board_pp_is_pal(void)
{
	guint is_pal = 0;
	gboolean ret = TRUE;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], (gint)BOARD_PP_IS_GPIO_PAL, &is_pal);

#ifdef DEBUG_BOARDPP_API
	g_message("is_pal [%d]", is_pal);
#endif

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] is_pal[%d]", __FUNCTION__ , ret, is_pal);
#endif

	return (ret == 0) ?  (gboolean)is_pal:0;	
}

/**
		@brief                      get is_d1? or is_4d1? infomation
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_is_d1(void)
{
	guint is_d1 = 0;
	gboolean ret = TRUE;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], (gint)BOARD_PP_IS_GPIO_D1, &is_d1);

#ifdef DEBUG_BOARDPP_API
	g_message("is_d1 [%d]", is_d1);
#endif
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] is_d1[%d]", __FUNCTION__ , ret, is_d1);
#endif

	return (ret == 0) ?  (gboolean)is_d1:0;	
}

gboolean nf_dev_board_pp_fan_get_info(NF_UTIL_FAN_INFO *fan_info)
{
	gint ret = TRUE;
	BOARD_PP_FAN_INFO info;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	memset(&info, 0x0, sizeof(BOARD_PP_FAN_INFO));

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], (gint)BOARD_PP_GET_FAN_INFO, &info);

#ifdef DEBUG_BOARDPP_API
	g_message("CPU Fan Speed       [%d]", info.speed_cpu_fan);
	g_message("SYS Fan1 Speed      [%d]", info.speed_sys_fan1);
	g_message("SYS Fan2 Speed      [%d]", info.speed_sys_fan2);
	g_message("CPU Fan TEMP        [%d]", info.temper_cpu);
	g_message("SYS Fan TEMP        [%d]", info.temper_sys);
	g_message("Fan DUTY            CPU[%d] SYS[%d]", info.duty_cpu_fan, info.duty_sys_fan);
	g_message("Fan MODE            CPU[%d] SYS[%d]", info.cpufan_mode, info.sysfan_mode);
	g_message("CPU Fan Threshold   [%d]", info.thresh_cpu);
	g_message("SYS Fan Threshold   [%d]", info.thresh_sys);
#endif

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif
	
	memcpy(fan_info, &info, sizeof(BOARD_PP_FAN_INFO));
		
	return (ret == 0) ? 1:0;
}

/**
		@brief                      Get Fan Duty
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_fan_get_duty(gint is_cpu, guchar *fan_duty)
{
	gint ret = TRUE, mode=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	
	
	if(is_cpu)
		mode=(gint)BOARD_PP_GET_CPUFAN_DUTY;
	else
		mode=(gint)BOARD_PP_GET_SYSFAN_DUTY;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], mode, fan_duty);
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Set Fan Duty
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_fan_set_duty(gint is_cpu, guchar fan_duty)
{
	gint ret = TRUE, mode=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	
	
	if(is_cpu)
		mode=BOARD_PP_SET_CPUFAN_DUTY;
	else
		mode=BOARD_PP_SET_SYSFAN_DUTY;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], mode, &fan_duty);
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Get Fan Mode 
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_fan_get_mode(gint is_cpu, guchar *fan_mode)
{
	gint ret = TRUE, mode=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	
	
	if(is_cpu)
		mode=(gint)BOARD_PP_GET_CPUFAN_MODE;
	else
		mode=(gint)BOARD_PP_GET_SYSFAN_MODE;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], mode, fan_mode);
#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
		@brief                      Set Fan Mode
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_fan_set_mode(gint is_cpu, guchar fan_mode)
{
	gint ret = TRUE, mode=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	
	
	if(is_cpu)
		mode=BOARD_PP_SET_CPUFAN_MODE;
	else
		mode=BOARD_PP_SET_SYSFAN_MODE;
	
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], mode, &fan_mode);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}


/**
		@brief                      Set Fan Threshold
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_board_pp_fan_set_thresh(gint is_cpu, gchar thresh)
{
	gint ret = TRUE, mode=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);	

	if(is_cpu)
		mode=BOARD_PP_SET_CPUFAN_THRESH;
	else
		mode=BOARD_PP_SET_SYSFAN_THRESH;
	
	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], mode, &thresh);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	return (ret == 0) ? 1:0;
}

/**
  		@brief                      Set loop out enable, Only ATM3GH (use Y-cable)
  		@return     gboolean        %TRUE on success, %FALSE if an error occurred
 **/
gboolean nf_board_pp_set_loop_out(guint enable_flag)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SET_LOOP_OUT, &enable_flag);

	if(ret <0)
			g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_board_pp_set_rs485_rtsn(guint is_tx)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SET_RS485_RTSN, &is_tx);

	if (ret < 0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__, ret);

		return (ret == 0) ? 1: 0;
}

#if !defined(_UTM7G_1648D) && !defined(_UTM7G_0824D) && !defined(_UTM7G_0412D)
gboolean nf_board_pp_set_eeprom(int bank, u_char addr, u_char len, u_char *data)
{
	gboolean ret = TRUE;
	BOARD_PP_EEPROM_INFO eeprom_info;
	int i;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	memset(&eeprom_info, 0x00, sizeof(eeprom_info));

	eeprom_info.bank = bank;
	eeprom_info.addr = addr;
	eeprom_info.length = len;
	memcpy(eeprom_info.data, data, len);

#if 0
	g_print("============= EEPROM DATA =============\n");
	for(i=0; i< eeprom_info.length; i++)
	{
		if(i%16 == 0)	
			g_print("\n");
		g_print("%02x ", eeprom_info.data[i]);
	}
#endif
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SET_EEPROM_DATA, &eeprom_info);

	if(ret <0)
			g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}
	
gboolean nf_board_pp_get_eeprom(int bank, u_char addr, u_char len, u_char *data)
{
	gboolean ret = TRUE;
	BOARD_PP_EEPROM_INFO eeprom_info;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	memset(&eeprom_info, 0x00, sizeof(eeprom_info));

	eeprom_info.bank = bank;
	eeprom_info.addr = addr;
	eeprom_info.length = len;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_GET_EEPROM_DATA, &eeprom_info);

	if(ret <0)
			g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	memcpy(data, eeprom_info.data, len);
	return (ret == 0) ? 1:0;
}
#endif
gboolean nf_dev_board_pp_get_gpio(guchar gpio, guchar pin, guchar *data)
{
	int ret = 0;	
	BOARD_PP_GPIO_INFO gpio_info;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

	gpio_info.gpio = gpio;
	gpio_info.pin = pin;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_GET_GPIO, &gpio_info);

#if defined(DEBUG_IOCTL)
	if (ret < 0) {
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	}
#endif

	*data = gpio_info.data;

	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_board_pp_set_gpio(guchar gpio, guchar pin, guchar data)
{
	int ret = 0;	
	BOARD_PP_GPIO_INFO gpio_info;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	gpio_info.gpio = gpio;
	gpio_info.pin = pin;
	gpio_info.data = data;

	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], (gint)BOARD_PP_SET_GPIO, &gpio_info);

#if defined(DEBUG_IOCTL)
	if (ret < 0) {
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	}
#endif
	
	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_board_pp_cntl_buzzer_irda(void)
{
	int ret = 0;	

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_CNTL_BUZZER_IRDA);

	if (ret < 0) {
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	}
	
	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_board_pp_poe_force_reset(void)
{
	int ret = 0;	

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

	// #if defined(_IPX_0412M4) || defined(_IPX_0824M4) || \
	// 	defined(_IPX_0824M4E) || defined(_IPX_0412M4E)
	// 	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_POE_FORCE_RESET);
	// 	#if defined(DEBUG_IOCTL)
	// 		if (ret < 0) {
	// 			g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	// 		}
	// 	#endif
	// #endif
	g_warning("%s ### NOT SUPPOTED ###", __FUNCTION__);
	return (ret == 0) ? 1 : 0;
}

// For QC IPXM4 16CH Lan Status Check
gboolean nf_dev_board_pp_get_net_link_state(int is_wan, int *link_state)
{
	int ret=0, val=0;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

	val=is_wan;
	// #if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_GET_NET_LINK_STATUS, &val);
	#if defined(DEBUG_IOCTL)
		if (ret < 0) {
			g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
		}
	#endif

	*link_state=val;
	// #endif

	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_port_purge_mac_addr(struct mac_load_info p_mac_load_info)
{
	gboolean ret=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	// ksi_tst ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SMI_PURGE_PORT_MAC, &p_mac_load_info);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	g_warning("%s ### NOT SUPPOTED ###", __FUNCTION__);
	return (ret == 0) ? 1:0;
}

/**
		@brief						purge target port's mac address table.
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_port_purge_all_mac_addr(struct mac_load_info p_mac_load_info)
{
	gboolean ret=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	// ksi_test ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SMI_PURGE_ALL_PORT_MAC, &p_mac_load_info);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif
	g_warning("%s ### NOT SUPPOTED ###", __FUNCTION__);
	return (ret == 0) ? 1:0;
}

/**
		@brief						insert mac address to port's mac address table.
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_port_insert_mac_addr(struct mac_load_info p_mac_load_info)
{
	gboolean ret=0;

#ifdef DEBUG_BOARDPP_API
	g_message("%s called", __FUNCTION__);
#endif

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);

	// ksi_test ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SMI_LOAD_PORT_MAC, &p_mac_load_info);

#if defined(DEBUG_IOCTL)
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#endif

	g_warning("%s ### NOT SUPPOTED ###", __FUNCTION__);

	return (ret == 0) ? 1:0;
}

// #if defined(_ANF5HG_1648D) || defined(_ANF5HG_0824D) \
//  || defined(_ANF6HG_1648D) || defined(_ANF6HG_0824D)
// gboolean nf_dev_board_pp_switch_configuration(int mode)
// {
// #if 0
// 	if(mode == SWITCH_MODE_SEPERATE)
// 	{
// 		nf_dev_board_pp_set_switch_reg(0x15, 0x6, 0x18);		// CPU
// 		nf_dev_board_pp_set_switch_reg(0x13, 0x6, 0x20);		// WAN
// 		nf_dev_board_pp_set_switch_reg(0x14, 0x6, 0x20);		// LAN
// 	}
// 	else if(mode == SWITCH_MODE_COMBINE)
// 	{
// 		nf_dev_board_pp_set_switch_reg(0x15, 0x6, 0x18);		// CPU
// 		nf_dev_board_pp_set_switch_reg(0x13, 0x6, 0x28);		// WAN
// 		nf_dev_board_pp_set_switch_reg(0x14, 0x6, 0x30);		// LAN
// 	}
// #else
// 	int ret=0, mode_switch=0;

// 	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

// 	mode_switch=mode;
// 	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_CNTL_SWITCH_MODE, &mode_switch);

// 	if (ret < 0) {
// 		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
// 	}

// 	return (ret == 0) ? 1 : 0;
// #endif
// }

// gboolean nf_dev_board_pp_get_switch_reg(guint dev_addr, guint reg_addr, guint *data)
// {
// 	int ret = 0;	
// 	BOARD_PP_SWITCH_INFO info;

//     g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

// 	info.dev_addr = dev_addr;
// 	info.reg_addr = reg_addr;

// 	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_GET_SWITCH_REG, &info);

// 	if (ret < 0) {
// 		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
// 	}

// 	*data = info.data;

// 	return (ret == 0) ? 1 : 0;
// }

// gboolean nf_dev_board_pp_set_switch_reg(guint dev_addr, guint reg_addr, guint data)
// {
// 	int ret = 0;	
// 	BOARD_PP_SWITCH_INFO info;

//     g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);

// 	info.dev_addr = dev_addr;
// 	info.reg_addr = reg_addr;
// 	info.data = data;

// 	ret = Ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_SET_SWITCH_REG, &info);

// 	if (ret < 0) {
// 		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
// 	}
	
// 	return (ret == 0) ? 1 : 0;
// }
// #endif

#if defined(SCHIP_COPY_PROTECTION)
gboolean nf_dev_board_pp_dec_data(NF_HW_PARAM_ENC_DATA *data)
{
	int ret = 0;	

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP] > 0, 0);	

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_DEC_DATA, data);

	return (ret == 0) ? 1 : 0;
}
#endif
#endif /* defined(USE_DEV_BOARD_PP) */




#if defined(USE_DEV_AM8816)
/********************************    AM8816   *********************************/
/**
		@brief                      set_sequence : sequence display
		@param[in]  spot_info		struct infomation about AM8816_SPOT_INFO
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_sequence(AM8816_SPOT_INFO *spot_info)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_SET_SEQUENCE, spot_info);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      set border color
		@param[in]  color_num		color_num is 0~7
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_border_color(guint color_num)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(color_num < 8 , 0);
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_SET_BORDER_COLOR, &color_num);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] color_num[%d]", __FUNCTION__ , ret, color_num);
	
	return (ret == 0) ? 1:0;
}

/**
		@brief                      camera title on
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_camera_title_on(void)
{
	gboolean ret = TRUE;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_CAMERA_TITLE_ON);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      camera title off
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_camera_title_off(void)
{
	gboolean ret = TRUE;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_CAMERA_TITLE_OFF);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      date on
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_date_on(void)
{
	gboolean ret = TRUE;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_DATE_ON);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      date off
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_date_off(void)
{
	gboolean ret = TRUE;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_DATE_OFF);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief						set date_time format
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_date_format(guint date_format, guint am_pm)
{
	gboolean ret;
	guchar  value = 0;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
    g_return_val_if_fail( date_format < 3 , 0);
    g_return_val_if_fail( am_pm <2 , 0);
        
	value = ((0xf & date_format) << 4)  | (0xf & am_pm);
	
	g_message("%s date_format[%d] am_pm[%d] val[%02x]",__FUNCTION__, 
		date_format, am_pm, value);
		
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_DATE_FORMAT, &value);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}


/**
		@brief						set gmt offset
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_gmt_offset(gint offset)
{
	gboolean ret;
		
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
        	
	g_message("%s gmt_offset[%d]",__FUNCTION__, offset);
		
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_GMT_OFFSET, &offset);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}


/**
		@brief                      set border size
		@param[in]  size			size is 0~3
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_border_size(guint size)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
    g_return_val_if_fail(size <4 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SPOT_SET_BORDER_SIZE, &size);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] size[%d]", __FUNCTION__ , ret, size);
	
	return (ret == 0) ? 1:0;	
}


/**
		@brief                     	set ntsc
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_ntsc(void)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_NTSC);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      set pal
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_pal(void)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_PAL);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      set spot title name
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_spot_title(AM8816_SPOT_TIELE_NAME *am_title_name)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_TITLE_NAME, am_title_name);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief                      set covert
		@param[in]  ch_mask			covert channel mask
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_covert(guint ch_mask)
{
	gboolean ret = TRUE;
	gushort	value = 0xffff & ch_mask;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_COVERT, &value);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] ch_mask[0x%08]", __FUNCTION__ , ret, ch_mask );
	
	return (ret == 0) ? 1:0;
}

/**
		@brief                      set covert
		@param[in]  ch_mask			vloss channel mask
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_am8816_set_vloss(guint ch_mask)
{
	gboolean ret = TRUE;
	gushort	value = 0xffff & ch_mask;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_AM8816]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_AM8816], AM8816_SET_VIDEO_LOSS_CH, &value);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d] ch_mask[0x%08]", __FUNCTION__ , ret, ch_mask );
	
	return (ret == 0) ? 1:0;
}

#endif /* defined(USE_DEV_AM8816) */






#if defined(USE_DEV_WATCHDOG)
/**
		@brief      				change margin(hardware reset interval)
		@param[in]	margin			interval time
		@return     gboolean        %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_watchdog_chg_margin(guint margin)
{
	gboolean ret = TRUE;

#ifdef ENABLE_FPGA_WATCHDOG
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
	g_return_val_if_fail(margin<=10, 0);
	
	margin *= 6500; // 1tick 167usec , 0xffff tick   11sec; 10,944,345 usec
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_WATCHDOG_TIMER_SET, &margin);	
#else
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_WATCHDOG]>0 , 0);
	g_return_val_if_fail(margin<178, 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_WATCHDOG], WDIOC_SETTIMEOUT, &margin);
#endif

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]	margin[%d]", __FUNCTION__ , ret, margin);
	else
		g_message("%s Ioctl OK  sec[%d]", __FUNCTION__, margin);

	return (ret == 0) ? 1:0;	
}

/**
		@brief						keep alive
		@return     gboolean		%TRUE on success, %FALSE if an error occurred 
*/
gboolean nf_dev_watchdog_alive(void)
{
	gboolean ret = TRUE;

#ifdef ENABLE_FPGA_WATCHDOG
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_WATCHDOG_KICK);		
#else
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_WATCHDOG]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_WATCHDOG], WDIOC_KEEPALIVE);
#endif

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
#if 0
	else
		g_message("%s Ioctl ok", __FUNCTION__);
#endif
	return (ret == 0) ? 1:0;	
}

/**
		@brief						hisilicon watchdog thread stop / enable
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_watchdog_cntl_thread(gint is_enable)
{
	gboolean ret = TRUE;

	#if defined(_UTM7G_1648D) || defined(_UTM7G_0824D) || defined(_UTM7G_0412D)
		#ifdef ENABLE_FPGA_WATCHDOG
			g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_BOARD_PP]>0 , 0);
			ret = ioctl(_Fd_Arr[DEV_FD_IDX_BOARD_PP], BOARD_PP_WATCHDOG_KICK);
		#else
			g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_WATCHDOG]>0 , 0);

			ret = ioctl(_Fd_Arr[DEV_FD_IDX_WATCHDOG], WDIOC_KEEPCNTL);
		#endif
	#endif
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	#if 0
	else
		g_message("%s Ioctl ok", __FUNCTION__);
	#endif
	return (ret == 0) ? 1:0;
}

#if 0
/**
		@brief						register dump
		@return     gboolean		%TRUE on success, %FALSE if an error occurred 
*/
gboolean nf_dev_watchdog_reg_dump(void)
{
	gboolean ret = TRUE;
	wdtregs regs_info;
	
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_WATCHDOG]>0 , 0);

	memset( &regs_info, 0x00, sizeof(wdtregs));
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_WATCHDOG], WDIOC_GET_REG_INFO, &regs_info);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	{
		int i;
		g_message("pid12	[%08x]", regs_info.pid12);
		g_message("emu_clk	[%04x]", regs_info.emu_clk);
		for(i=0;i<10;i++)
			g_message("rsvd0[%d]	[%02x]", i,regs_info.rsvd0[i]);
		g_message("tim12	[%08x]", regs_info.tim12);
		g_message("tim34	[%08x]", regs_info.tim34);
		g_message("prd12	[%08x]", regs_info.prd12);
		g_message("prd34	[%08x]", regs_info.prd34);
		g_message("tcr 	[%08x]", regs_info.tcr);
		g_message("tgcr	[%04x]", regs_info.tgcr);
		for(i=0;i<2;i++)
			g_message("rsvd1[%d]	[%02x]", i, regs_info.rsvd1[i]);
		g_message("wdtcr	[%04x]", regs_info.wdtcr);
		g_message("wdkey	[%04x]", regs_info.wdkey);
	}
	
	return (ret == 0) ? 1:0;	
}
#endif

#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_wtd_help[] = "nf_dev_wtd set [time_sec]";
static int jbshell_nf_dev_wtd_test(int argc, char **argv)
{
	guint margin=0;
	
	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", nf_dev_wtd_help);
		return -1;
	}

	if(strcmp(argv[1], "set") == 0)
	{
		if ( argc < 3 ) {
			printf("Invalid arguments\n%s\n", nf_dev_wtd_help);
			return -1;
		}

		margin = (guint)strtoul(argv[2], NULL, 0);
		nf_dev_watchdog_chg_margin(margin);
	}
	else if(strcmp(argv[1], "alive") == 0)
		nf_dev_watchdog_alive();
	else
	{
		printf("Invalid arguments\n%s\n", nf_dev_wtd_help);
		return -1;
	}


	return 0;
}
__commandlist(jbshell_nf_dev_wtd_test,"nf_dev_wtd", nf_dev_wtd_help, nf_dev_wtd_help);
#endif		// defined(DEBUG_JBSHELL_UTIL_DEVICE)

#endif /* defined(USE_DEV_WATCHDOG) */







#if defined(USE_DEV_RS485)
/****************************    RS485   ******************************/
/**
		@brief						Read Queue free
		@return     gboolean		%TRUE on success, %FALSE if an error occurred 
*/
gboolean nf_dev_rs485_readq_free(void)
{
	gboolean ret = TRUE;
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_RS485]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_RS485], FPGA_RS485_IOCRL_READQ_FREE);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}

/**
		@brief						Register init( == Uart init)
		@return     gboolean		%TRUE on success, %FALSE if an error occurred 
*/
gboolean nf_dev_rs485_reg_init(void)
{
	gboolean ret = TRUE;
    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_RS485]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_RS485], FPGA_RS485_IOCRL_REG_INIT);
	
	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);
	
	return (ret == 0) ? 1:0;	
}
#endif /* defined(USE_DEV_RS485) */




#if defined(USE_DEV_FS1648)
/****************************    FS1648   ******************************/
/**
		@brief
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_rip_change_req(struct fs1648_chg_rip_reg *rip_context)
{
	gboolean ret = TRUE;

#if 0
	int i,j;

//	g_message("RIV Setting Table change_rip=0x%02x",rip_context->chg_flag);
//
//	for ( i = 0 ; i < 4 ; i++)
//	{
//		printf("RIP[%d] ",i);
//
//		for ( j = 0 ; j < 64 ; j++ )
//		{
//			printf("%02x ",rip_context->rip[i][j]);
//		}
//		printf("\n");
//	}

//	unsigned char rip[4][64];
#endif

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);
	g_return_val_if_fail( rip_context != NULL, 0 );

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_RIP_CHANGE_REQ, rip_context);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief						FS1648 live change
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_live_change(struct fs1648_anf_livedisp_fmt *livedisp_fmt)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);
	g_return_val_if_fail( livedisp_fmt != NULL, 0 );

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_ANF_CHG_LIVE, livedisp_fmt);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_rip_change(struct fs1648_chg_rip_reg *rip_context)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);
	g_return_val_if_fail( rip_context != NULL, 0 );

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_LOAD_REC_INFO, rip_context);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_covert(int t_covert)
{
	gboolean ret = TRUE;
	gint covert_data = t_covert;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_CHG_MASK, &covert_data);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		zoom start
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_start( gint ch_num )
{
	gboolean ret = TRUE;
	struct fs1648_zoom_data val;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	val.cam			= ch_num;
	val.x			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.y			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.zoom_inout	= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_CHG_ZOOM, &val);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		zoom in, out
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_inout( gint inout )
{
	gboolean ret = TRUE;
	struct fs1648_zoom_data val;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	val.cam			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.x			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.y			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.zoom_inout	= inout;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_INOUT_ZOOM, &val);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		zoom move
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_move( int zoom_x, int zoom_y )
{
	gboolean ret = TRUE;
	struct fs1648_zoom_data val;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	val.cam			= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����
	val.x			= zoom_x;
	val.y			= zoom_y;
	val.zoom_inout	= 0;		//�̰� ����̹����� ���⼭ ��� ���� ����

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_MOVE_ZOOM, &val);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		zoom stop
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_stop( void )
{
	gboolean ret = TRUE;
	int temp_t = 0;				//ioctl ������ ���߱� ���ؼ� �־� �� ����

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_STOP_ZOOM, &temp_t);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		motion on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_onoff( unsigned int t_onoff )						//hosik_motion
{
	gboolean ret = TRUE;
	unsigned int temp_t;

	temp_t = t_onoff;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_ONOFF, &temp_t);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		display on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_disp_onoff( unsigned int t_onoff )
{
	gboolean ret = TRUE;
	unsigned int temp_t;

	temp_t = t_onoff;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_DISP_ON, &temp_t);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		sensor sleep display on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_slp_on( struct fs1648_md_sensor_color *s_color )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_SLP_ON, s_color);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		sensor actor display on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_act_on( struct fs1648_md_sensor_color *s_color )		//ok
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_ACT_ON, s_color);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		set sensor sens
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_sens( struct fs1648_md_sensor_val *t_sens )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_SENS, t_sens);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		set detect sensor counts
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_cnt( struct fs1648_md_sensor_val *t_cnt )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_CNT, t_cnt);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		set motion area
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_area( struct fs1648_md_sensor_area *t_area )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_SET_MD_AREA, t_area);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

/**
		@brief		set pause data
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_pause( unsigned short t_data )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_FS1648]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_FS1648], FS1648_PAUSE, &t_data);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

#else   /*cfac, just temp coding for ATM0424*/
/****************************    FS1648   ******************************/

/**
		@brief
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_covert(int t_covert)
{
    return TRUE;
}


/**
		@brief		zoom start
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_start( gint ch_num )
{
    return TRUE;
}


/**
		@brief		zoom in, out
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_inout( gint inout )
{
    return TRUE;
}


/**
		@brief		zoom move
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_move( int zoom_x, int zoom_y )
{
    return TRUE;
}


/**
		@brief		zoom stop
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_zoom_stop( void )
{
    return TRUE;
}


/**
		@brief		motion on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_onoff( unsigned int t_onoff )						//hosik_motion
{
    return TRUE;
}


/**
		@brief		display on/off
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean anf_dev_fs1648_md_disp_onoff( unsigned int t_onoff )
{
    return TRUE;
}
#endif /* defined(USE_DEV_FS1648) */






#if defined(USE_DEV_CD22M3492)
/****************************    SPOT_ANF    ******************************/
/**
		@brief
		@return     gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_dev_spot_set_sequence(struct spot_status_user *spot_info)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_SPOT_ANF]>0 , 0);
	g_return_val_if_fail( spot_info != NULL, 0 );

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_SPOT_ANF], DRV_SPOT_SET_SEQUENCE, spot_info);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_spot_set_vloss(unsigned short t_data)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_SPOT_ANF]>0 , 0);

	g_message("%s, %d", __FUNCTION__, __LINE__);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_SPOT_ANF], DRV_SPOT_SET_VIDEO_LOSS_CH, &t_data);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_spot_get_vloss(unsigned short *t_data)
{
	gboolean ret = TRUE;

    g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_SPOT_ANF]>0 , 0);

	g_message("%s, %d", __FUNCTION__, __LINE__);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_SPOT_ANF], DRV_SPOT_GET_VIDEO_LOSS_CH, t_data);

	if(ret <0)
		g_warning("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	return (ret == 0) ? 1:0;
}
#endif /* defined(USE_DEV_ANALOG_SPOT) */



#if defined(USE_DEV_SOLO6x10) 
gboolean nf_dev_solo_send_md( struct SOLO6x10_SET_MD_CFG *solo_mdcfg )
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_SOLO_VIN]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_SOLO_VIN], IOCTL_VIN_SET_MD_AREA, solo_mdcfg);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#endif

#if defined(USE_DEV_GENNUM)
gboolean nf_dev_gennum_rx_set_vloss(guint mask)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_RX_SET_VLOSS_ONOFF, &mask);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_get_video_stardard(gint ch, guchar *val)
{
	gboolean ret = TRUE;
	GENNUM_STD_DATA std_data;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);
	g_return_val_if_fail(ch < NUM_ACTIVE_CH , 0);

	memset(&std_data, 0x0, sizeof(GENNUM_STD_DATA));
	
	std_data.ch=ch;
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_RX_GET_DETECTED_VIDEO_STD, &std_data);
	
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	else
		*val=std_data.data;

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_rx_set_vloss_interval(guint interval)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_RX_SET_VLOSS_POLLING_INTERVAL, &interval);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_rx_reg_dump(gushort reg, gint ch)
{
	gboolean ret = TRUE;
	Gennum_rx_r g_rx_r;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);
	
	g_rx_r.ch=ch;
	g_rx_r.reg=reg;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_RX_REG_DUMP, &g_rx_r);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_rx_write(gushort reg, gint ch, guchar *data)
{
	gboolean ret = TRUE;
	Gennum_rx_w	g_rx_w;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);
	
	g_rx_w.ch=ch;
	g_rx_w.reg=reg;
	memcpy(&g_rx_w.data, data, sizeof(g_rx_w.data));

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_RX_REG_WRITE, &g_rx_w);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_tx_reg_dump(gushort reg)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_TX_REG_DUMP, &reg);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_tx_wrte(gushort reg, guchar *data)
{
	gboolean ret = TRUE;
	Gennum_tx_w	g_tx_w;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);

	g_tx_w.reg=reg;
	memcpy(&g_tx_w.data, data, sizeof(g_tx_w.data));

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_TX_REG_WRITE, &reg);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_get_vloss(guint *vloss)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_GET_CURR_VLOSS_STATUS, vloss);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_gennum_get_tx_loop_status(gint *is_detected)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_GENNUM]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_GENNUM], GENNUM_GET_TX_LOOP_STATUS, is_detected);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_gennum_cmd_help[] = "gennum rx_r [ch] [reg]\n"
												"gennum rx_w [ch] [reg] [data]\n"
												"gennum tx_r [reg]\n"
												"gennum tx_w [reg] [data]\n"
												"gennum tx_loop\n";
static int nf_dev_jbshell_gennum_cmd(int argc, char **argv)
{
	gint ch=0;
	gushort reg=0, tmp=0;
	guchar data[2]={0, };

	if(argc < 2)
		goto nf_dev_dsp_msg;

	if(strcmp(argv[1], "rx_r") == 0)
	{
		if(argc < 4)
			goto nf_dev_dsp_msg;

		ch=(gint)strtoul(argv[2], NULL, 16);
		reg=(gushort)strtoul(argv[3], NULL, 16);
		
		if(!nf_dev_gennum_rx_reg_dump(reg, ch))
			g_warning("%s gennum rx ch[%d] dump error!!", __FUNCTION__, ch);
	}
	else if(strcmp(argv[1], "rx_w") == 0)
	{
		if(argc < 5)
			goto nf_dev_dsp_msg;

		ch=(gint)strtoul(argv[2], NULL, 16);
		reg=(gushort)strtoul(argv[3], NULL, 16);
		tmp=(gushort)strtoul(argv[4], NULL, 16);
		
		data[0]=(guchar)(tmp & 0xff);
		data[1]=(guchar)((tmp >> 8) & 0xff);

		if(!nf_dev_gennum_rx_write(reg, ch, data))
			g_warning("%s gennum rx ch[%d] write error!!", __FUNCTION__, ch);
	}
	else if(strcmp(argv[1], "tx_r") == 0)
	{
		if(argc < 3)
			goto nf_dev_dsp_msg;

		reg=(gushort)strtoul(argv[2], NULL, 16);

		if(!nf_dev_gennum_tx_reg_dump(reg))
			g_warning("%s gennum tx dump error!!", __FUNCTION__);
	}
	else if(strcmp(argv[1], "tx_w") == 0)
	{
		if(argc < 4)
			goto nf_dev_dsp_msg;

		reg=(gushort)strtoul(argv[2], NULL, 16);
		tmp=(gushort)strtoul(argv[3], NULL, 16);	
		
		data[0]=(guchar)(tmp & 0xff);
		data[1]=(guchar)((tmp >> 8) & 0xff);

		if(!nf_dev_gennum_tx_wrte(reg, data))
			g_warning("%s gennum tx write error!!", __FUNCTION__);
	}
	else if(strcmp(argv[1], "std") == 0)
	{
		guchar data=0;
		gint ch=0;
		
		if(argc < 3)
			goto nf_dev_dsp_msg;

		ch=(gint)strtoul(argv[2], NULL, 16);

		nf_dev_gennum_get_video_stardard(ch, &data);
		g_message("Video STD --> [0x%02x]", data);
	}
	else if(strcmp(argv[1], "tx_loop") == 0)
	{
		gint is_detected=0;
		
		if(argc < 2)
			goto nf_dev_dsp_msg;

		nf_dev_gennum_get_tx_loop_status(&is_detected);

		g_message("TX Loop is %s", (is_detected == 1) ? "Connected" : "Not Connected");
	}
	else
		g_print("Invalid arguments\n%s\n", nf_dev_jbshell_gennum_cmd_help);

	return TRUE;

nf_dev_dsp_msg:
	g_print("Invalid arguments\n%s\n", nf_dev_jbshell_gennum_cmd_help);
	return FALSE;
}
__commandlist(nf_dev_jbshell_gennum_cmd, "gennum", nf_dev_jbshell_gennum_cmd_help, nf_dev_jbshell_gennum_cmd_help);
#endif	/* defined(DEBUG_JBSHELL_UTIL_DEVICE) */ 
#endif	/* defined(USE_DEV_GENNUM) */


// #define SWITCH_MV_88E6095F (0)

// #if (SWITCH_MV_88E6095F)
// #define SWITCH_SYSCALL (370)
// #define MVSW_CMD_WRITE_REG (5)
// extern int nf_dev_switch_ingress_limit(int port, int in_bps)
// {
// 	int syscall_rtn = 0;

// 	int pri0rate = 0;
// 	double result = 0;
// 	double mo = 0;


// 	printf("[%s] port(%d) ingress rate (%d bits/sec)\n", __FUNCTION__, port, in_bps);

// 	g_return_val_if_fail((port >= 0 && port <= 10), 0);
// 	g_return_val_if_fail((in_bps <= 256000000 && in_bps >= 64000), 0);

// 	mo = 0.000000032 * in_bps;
// 	result = 8 / mo;
// 	pri0rate = (int)result;
// 	pri0rate++;
// 	pri0rate += 0x7000;

// 	syscall_rtn = syscall(SWITCH_SYSCALL, MVSW_CMD_WRITE_REG, port+0x10, 0x09, pri0rate);

// 	return (1);
// }
// #endif /* SWITCH_MV_88E6095F */

#if defined (USE_DEV_TPS2384)
gboolean nf_dev_tps2384_port_onoff(gint port, gboolean is_on, gint *is_fail)
{
	gboolean ret = TRUE;
	guint mode=0;
	NF_UTIL_TPS2384_PORT_ONOFF port_onoff;

	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TPS2384]>0 , 0);

	if(is_on)
		mode=TPS2384_PORT_ON;
	else
		mode=TPS2384_PORT_OFF;

	port_onoff.port=port;
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TPS2384], mode, &port_onoff);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	*is_fail=port_onoff.is_fail;

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_tps2384_get_info(NF_UTIL_TPS2384_PORT_INFO *info)
{
	gboolean ret = TRUE;

	//g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TPS2384]>0 , 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TPS2384], TPS2384_GET_INFO, info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_tps2384_get_info_single(int port, NF_UTIL_TPS2384_INFO *info)
{
	gboolean ret = TRUE;

	//g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TPS2384]>0 , 0);
	g_return_val_if_fail(port< NUM_ACTIVE_CH , 0);
	
	info->reserved = port;	
		
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TPS2384], TPS2384_GET_INFO_PORT, info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_tps2384_port_verify(guchar port, gboolean *is_on)
{
	gboolean ret = TRUE;
	TPS2384_WRITE_INFO winfo;

	winfo.port=port;
	winfo.command=TPS2384_PORT_CTRL_WRITE2;
	winfo.data=0;
	
	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TPS2384]>0, 0);
	
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TPS2384], TPS2384_REG_READ, &winfo);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	
	if((winfo.data >> 4) & 0x1)
		*is_on=FALSE;
	else
		*is_on=TRUE;

#if 0	
	g_message("%s winfo data[0x%02x]", __FUNCTION__, winfo.data);
#endif

	return (ret == 0) ? 1:0;
}
#if 0
gboolean nf_dev_tps2384_read_status(void)
{
	gboolean ret = TRUE;
	
	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TPS2384]>0 , 0);
	
//	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TPS2384], TPS2384_PORT_STATUS_READ);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#endif
#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_tps2384_cmd_help[] = "        info\n"
												"        port on  port_num\n"
												"        port off port_num\n";
static int nf_dev_jbshell_tps2384_cmd(int argc, char **argv)
{
	NF_UTIL_TPS2384_PORT_INFO  info;

	if(argc < 2)
		goto nf_dev_tps2384_msg;

	if(strcmp(argv[1], "info") == 0)
	{
		gint i=0;

		if(argc < 2)
			goto nf_dev_tps2384_msg;
	
		memset(&info, 0x0, sizeof(NF_UTIL_TPS2384_PORT_INFO));
		nf_dev_tps2384_get_info(&info);

		for(i=0; i<NUM_ACTIVE_CH; i++)
	{
			g_print("======================================\n");
			g_print("CH%d Discovery          --> %d\n", i, info.info[i].is_discovery);
			g_print("CH%d Active             --> %d\n", i, info.info[i].is_active);
			g_print("CH%d Port Class         --> %d\n", i, info.info[i].port_class);
			g_print("CH%d Func Status        --> %d\n", i, info.info[i].func_status);
			g_print("CH%d Voltage            --> %dV\n", i, info.info[i].voltage);
			g_print("CH%d Current            --> %dmA\n", i,info.info[i].current_mA);
			g_print("CH%d Power Comsumption  --> %dmW\n", i,info.info[i].consumption);
			g_print("\n\n");
			g_print("======================================\n");
		}
	}
	else if(strcmp(argv[1], "port") == 0)
	{
		gint port=0, is_fail;
		gboolean is_on=0;

		if(argc < 4)
			goto nf_dev_tps2384_msg;

		if(strcmp(argv[2], "on") == 0)
			is_on=TRUE;
		else if(strcmp(argv[2], "off") == 0)
			is_on=FALSE;
		else
			goto nf_dev_tps2384_msg;

		port=(gint)strtoul(argv[3], NULL, 10);

		nf_dev_tps2384_port_onoff(port, is_on, &is_fail);

		g_message("%s is_fail[%d]", __FUNCTION__, is_fail);
	}
	else
		goto nf_dev_tps2384_msg;

	return 0;

nf_dev_tps2384_msg:
		printf("Invalid arguments\n%s\n", nf_dev_jbshell_tps2384_cmd_help);
		return -1;
	}
__commandlist(nf_dev_jbshell_tps2384_cmd, "tps2384", nf_dev_jbshell_tps2384_cmd_help, nf_dev_jbshell_tps2384_cmd_help);
#endif	/* defined(DEBUG_JBSHELL_UTIL_DEVICE) */ 
#endif	/* defined(USE_DEV_TPS2384) */

#if defined(USE_DEV_TLV320AIC23)
gboolean nf_dev_audio_live_onoff_ctrl(gboolean is_on)
{
	gboolean ret = TRUE;
	guint mode=0;

//	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	if(is_on)
		mode=TLV320AIC23_LIVE_ON;
	else
		mode=TLV320AIC23_LIVE_OFF;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], mode);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

/**
  TLV320AIC23 MIC Boot
**/
gboolean nf_dev_audio_micb_ctrl(gboolean is_on)
{
	gboolean ret = TRUE;
	guint mode=0;

//	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	if(is_on)
		mode=TLV320AIC23_MICB_ON;
	else
		mode=TLV320AIC23_MICB_OFF;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], mode);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

/**
	Line Input Volume Cntl
**/
gboolean nf_dev_audio_line_input_vol_ctrl(gint vol)
{
	gboolean ret = TRUE;
	gint dB=0;

//	g_message("%s called", __FUNCTION__);
	
	g_return_val_if_fail((vol >= 0 && vol <= 100), 0);
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	dB=(((vol*31)/100) - 23);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], TLV320AIC23_LINE_INPUT_VOL_CTRL, &dB);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

/**
	Heaphone Volume Cntl
**/
gboolean nf_dev_audio_headphone_vol_ctrl(gint vol, gint is_left)
{
	gboolean ret = TRUE;
	gint dB=0;
	guint mode=0;

//	g_message("%s called", __FUNCTION__);
	
	g_return_val_if_fail((vol >= 0 && vol <= 100), 0);
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	dB=(((vol*79)/100) - 73);
	
	if(is_left)
		mode=TLV320AIC23_HEADPHONE_VOL_CTRL_LEFT;
	else
		mode=TLV320AIC23_HEADPHONE_VOL_CTRL_RIGHT;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], mode, &dB);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}


/**
	Line Input Mute
**/
gboolean nf_dev_audio_line_input_mute(gint is_mute)
{
	gboolean ret = TRUE;

//	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], TLV320AIC23_LINE_INPUT_MUTE, &is_mute);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

/**
	DAC Mute CTRL
**/
gboolean nf_dev_audio_dac_mute(gint is_mute)
{
	gboolean ret = TRUE;

//	g_message("%s called", __FUNCTION__);

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TLV320AIC23]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], TLV320AIC23_DAC_MUTE, &is_mute);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

/**
	DAC REG WRITE 
**/
gboolean nf_dev_audio_reg_write(gushort reg, guint mask, guint val)
{
	gboolean ret = TRUE;
	struct tlv320aic23_reg info;

	info.reg=reg;
	info.mask=mask;
	info.value=val;	

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TLV320AIC23], TLV320AIC23_REG_WRITE, &info);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}


#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_tlv320aic23_cmd_help[] = "live [on or off]\n"
													"            micb [on or off]\n"
													"            lmute [0 or 1]\n";
static int nf_dev_jbshell_tlv320aic23_cmd(int argc, char **argv)
{
	if(argc < 3)
		goto nf_dev_tlv320aic23_msg;

	if(strcmp(argv[1], "live") == 0)
	{
		if(strcmp(argv[2], "on") == 0)
		nf_dev_audio_live_onoff_ctrl(TRUE);
		else if(strcmp(argv[2], "off") == 0)
		nf_dev_audio_live_onoff_ctrl(FALSE);
		else
			goto nf_dev_tlv320aic23_msg;
	}
	else if(strcmp(argv[1], "micb") == 0)
	{
		if(strcmp(argv[2], "on") == 0)
			nf_dev_audio_micb_ctrl(TRUE);
		else if(strcmp(argv[2], "off") == 0)
			nf_dev_audio_micb_ctrl(FALSE);
		else
			goto nf_dev_tlv320aic23_msg;
	}
	else if(strcmp(argv[1], "vol") == 0)
	{
		gint vol=0, is_head=0;
		gboolean is_left=0;

		if(argc < 4)
			goto nf_dev_tlv320aic23_msg;
			
		if(strcmp(argv[2], "line") == 0)
		{
			is_head=FALSE;
		
			vol=(gint)strtoul(argv[3], NULL, 10);
		}
		else if(strcmp(argv[2], "head") == 0)
		{
			if(argc < 5)
				goto nf_dev_tlv320aic23_msg;
			
			is_head=TRUE;
			is_left=(gboolean)strtoul(argv[3], NULL, 10);
			vol=(gint)strtoul(argv[4], NULL, 10);
		}
		else
			goto nf_dev_tlv320aic23_msg;
			
		if(is_head)
			nf_dev_audio_headphone_vol_ctrl(vol, (gint)is_left);
		else
			nf_dev_audio_line_input_vol_ctrl(vol);
	}
	else if(strcmp(argv[1], "lmute") == 0)		// Line Mute Cntl
	{
		gboolean is_mute=FALSE;

		if(argc < 3)
			goto nf_dev_tlv320aic23_msg;
		is_mute=(gboolean)strtoul(argv[2], NULL, 10);

		nf_dev_audio_line_input_mute(is_mute);
	}
	else if(strcmp(argv[1], "dmute") == 0)		// Dac Mute Cntl
	{
		gboolean is_mute=FALSE;

		if(argc < 3)
			goto nf_dev_tlv320aic23_msg;
		is_mute=(gboolean)strtoul(argv[2], NULL, 10);

		nf_dev_audio_dac_mute(is_mute);
	}
	else if(strcmp(argv[1], "write") == 0)
	{
		gushort reg=0;
		guint mask=0, val=0;

		if(argc < 5)
			goto nf_dev_tlv320aic23_msg;

		reg=(gushort)strtoul(argv[2], NULL, 16);
		mask=(guint)strtoul(argv[3], NULL, 16);
		val=(guint)strtoul(argv[4], NULL, 16);

		g_message("%s reg[0x%04x] mask[0x%08x] val[0x%08x]", __FUNCTION__, reg, mask, val);
		nf_dev_audio_reg_write(reg, mask, val);
	}
	else
		goto nf_dev_tlv320aic23_msg;
	
	return 0;

nf_dev_tlv320aic23_msg:
	printf("Invalid arguments\n%s\n", nf_dev_jbshell_tlv320aic23_cmd_help);
	return -1;
}
__commandlist(nf_dev_jbshell_tlv320aic23_cmd, "tlv320aic23", nf_dev_jbshell_tlv320aic23_cmd_help, nf_dev_jbshell_tlv320aic23_cmd_help);
#endif	/* defined(DEBUG_JBSHELL_UTIL_DEVICE) */ 
#endif

#if defined(USE_DEV_DSP)
gboolean nf_dev_audio_input_ch_cntl(gint input_ch_num, gint fd)
{
	gboolean ret = TRUE;

//	g_message("%s called", __FUNCTION__);
	ret = ioctl(fd, SNDCTL_DSP_CHANNELS, &input_ch_num);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);


	return (ret == 0) ? 1:0;
}

gboolean nf_dev_audio_dsp_reset(gboolean is_fd_read)
{
	gboolean ret = TRUE;
	gint fd=0;

	if(is_fd_read)
		fd=_Fd_Arr[DEV_FD_IDX_DSP_READ];
	else
		fd=_Fd_Arr[DEV_FD_IDX_DSP_WRITE];

	g_message("%s called", __FUNCTION__);

//	nf_dev_audio_dac_mute(TRUE);

	ret = ioctl(fd, SNDCTL_DSP_RESET);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

//	nf_dev_audio_dac_mute(FALSE);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_audio_dsp_get_odelay(gboolean is_fd_read, int *delay)
{
	gboolean ret = TRUE;
	gint fd=0;
	
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_READ]>0 , 0);
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_WRITE]>0 , 0);

//	g_message("%s called", __FUNCTION__);
	if(is_fd_read)
		fd=_Fd_Arr[DEV_FD_IDX_DSP_READ];
	else
		fd=_Fd_Arr[DEV_FD_IDX_DSP_WRITE];

	ret = ioctl(fd, SNDCTL_DSP_GETODELAY, delay);

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_audio_dsp_get_optr(gboolean is_fd_read, void *data)
{
	gboolean ret = TRUE;
	gint fd=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_READ]>0 , 0);
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_WRITE]>0 , 0);

//	g_message("%s called", __FUNCTION__);
	if(is_fd_read)
		fd=_Fd_Arr[DEV_FD_IDX_DSP_READ];
	else
		fd=_Fd_Arr[DEV_FD_IDX_DSP_WRITE];

	ret = ioctl(fd, SNDCTL_DSP_GETOPTR, data);
	
	#if 0
		// ex)
		{
			struct count_info *info;
			info=(struct count_info *)data;
			g_message("bytes[%d]  blocks[%d]  ptr[%d]", info->bytes, info->blocks, info->ptr);
		}
	#endif

	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

/*      
	Buffer status queries.
	typedef struct audio_buf_info {
		int fragments;  # of available fragments (partially usend ones not counted)
		int fragstotal; Total # of fragments allocated
		int fragsize;   Size of a fragment in bytes
		
		int bytes;  Available space in bytes (includes partially used fragments)
		Note! 'bytes' could be more than fragments*fragsize 
	} audio_buf_info;
*/ 
gint nf_dev_audio_dsp_get_space(gboolean is_fd_read, gboolean is_ispace, audio_buf_info *info)
{
	gboolean ret = TRUE;
	gint fd=0;
	guint cmd=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_READ]>0 , 0);
	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DSP_WRITE]>0 , 0);
	
	if(is_fd_read)
		fd=_Fd_Arr[DEV_FD_IDX_DSP_READ];
	else
		fd=_Fd_Arr[DEV_FD_IDX_DSP_WRITE];

	if(is_ispace)
		cmd=SNDCTL_DSP_GETISPACE;
	else
		cmd=SNDCTL_DSP_GETOSPACE;

	ret = ioctl(fd, cmd, info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
typedef struct _NF_DEV_AUDIO_READ_TEST_T
{
	gint fd;
	gint ch;
	gint min;
	gchar filename[32];
} NF_DEV_AUDIO_READ_TEST;

typedef struct _NF_DEV_AUDIO_WRITE_TEST_T
{
	gint fd;
	gint ch;
	gchar filename[32];
} NF_DEV_AUDIO_WRITE_TEST;

NF_DEV_AUDIO_READ_TEST audio_read;
NF_DEV_AUDIO_WRITE_TEST audio_write;

static gint nf_dev_audio_read_thread_func(NF_DEV_AUDIO_READ_TEST *audio);
static gint nf_dev_audio_write_thread_func(NF_DEV_AUDIO_WRITE_TEST *audio);

static gint nf_dev_audio_read_thread_func(NF_DEV_AUDIO_READ_TEST *audio)
{
	FILE *fp=NULL;
	gint fd=0, i=0, min=0, ch=0;
	GTimeVal curr_timeval;
	glong out_sec=0;
	gchar *buf=NULL;

	g_message("%s fd[%d] ch[%d] min[%d] filename[%s]",
					__FUNCTION__, audio->fd, audio->ch, audio->min, audio->filename);

	fp=fopen(audio->filename, "w");
	if(fp == NULL)
	{
		printf("%s File Open Fail!! filename[%s]\n", __FUNCTION__, audio->filename);
		return -1;
	}
	else
		printf("%s File Open Success!! filename[%s]\n", __FUNCTION__, audio->filename);

	min=audio->min;

	gettimeofday((struct timeval *)&curr_timeval, NULL);

	out_sec=curr_timeval.tv_sec+(60*min);

	fd=audio->fd;
	ch=audio->ch;

	if(!nf_dev_audio_input_ch_cntl(ch, fd))
	{
		g_warning("%s Set Input Channel Fail!! Input Ch[%d]", __FUNCTION__, ch);
		fclose(fp);
		return FALSE;
	}
	else
		g_message("%s Set Input Channel [%d]", __FUNCTION__, ch);

	buf = (gchar *)g_malloc0(sizeof(gchar) * (guint)(ch+1));

	g_message("\n\n%s Audio Data Read Start!!! During [%d]Min!! Out Sec [%ld]-->[%ld]\n\n",
							__FUNCTION__, min, curr_timeval.tv_sec, out_sec);

	g_message("%s start!!", __FUNCTION__);
	while(1)
	{
		gettimeofday((struct timeval *)&curr_timeval, NULL);
		
		if(out_sec <= curr_timeval.tv_sec)
			break;
		
		if(read(fd, buf, (size_t)ch) != ch)
		{
			g_warning("%s Read Error!!!", __FUNCTION__);
			break;
		}

		if(fwrite(buf, (size_t)ch, 1, fp) != 1)
		{
			g_warning("%s File Write Error!!\n", __FUNCTION__);
			break;
		}
//		g_usleep( 1000*100 );
	}
	g_message("\n\n%s Audio Data Read Finish!!! Curr Time[%ld] \n\n", __FUNCTION__, curr_timeval.tv_sec);
	g_message("%s end!!", __FUNCTION__);

	g_free(buf);	
	fclose(fp);

	return TRUE;
}

static gint nf_dev_audio_write_thread_func(NF_DEV_AUDIO_WRITE_TEST *audio)
{
	FILE *fp=NULL;
	gint fd=0, ch=0;
	guint filesize=0, remain=0;
	guchar tmp[8]={0, };
	struct stat f_stat;
	
	ch=audio->ch;
	fd=audio->fd;

	if(stat(audio->filename, &f_stat) == -1)
	{
		g_warning("%s Cannot Find Filename [%s]", __FUNCTION__, audio->filename);
		return FALSE;
	}
	filesize=(guint)f_stat.st_size;

	fp=fopen(audio->filename, "r");
	if(fp == NULL)
	{
		g_message("%s File Open Fail!! filename[%s]", __FUNCTION__, audio->filename);
		return FALSE;
	}
	else
		g_message("%s File Open Success!! filename[%s] filesize[%d]", __FUNCTION__, audio->filename, filesize);


	g_message("%s start", __FUNCTION__);

	while(1)
	{
		if(fread(tmp, sizeof(tmp), 1, fp) != 1)
		{
			g_warning("%s File Read Error!!\n", __FUNCTION__);
			break;
		}
		write(fd, &tmp[ch], 1);
		
		filesize-=sizeof(tmp);
		if(filesize < sizeof(tmp))
		{
			g_message("%s Audio File Read Finish.. filesize[%d]", __FUNCTION__, filesize);
			break;
		}
	}
	
	g_message("%s end", __FUNCTION__);
	
	fclose(fp);
	return TRUE;
}

gboolean nf_dev_audio_rw_test(NF_DEV_AUDIO_READ_TEST *audio_read, NF_DEV_AUDIO_WRITE_TEST *audio_write)
{
	GThread         *thread0;
	GThread         *thread1;

#if 0
	g_message("%s Read Filename[%s] fd[%d] ch[%d] min[%d]", 
					__FUNCTION__, audio_read->filename, audio_read->fd, audio_read->ch, audio_read->min);
	g_message("%s Write Filename[%s] fd[%d] ch[%d]", 
					__FUNCTION__, audio_write->filename, audio_write->fd, audio_write->ch);
#endif
	thread0 = g_thread_create((GThreadFunc)nf_dev_audio_read_thread_func, audio_read, FALSE, NULL);
	if(thread0 == NULL)
	{
		g_warning("%s Audio Read Thread Create Fail!!", __FUNCTION__);
		return FALSE;
	}

	thread1 = g_thread_create((GThreadFunc)nf_dev_audio_write_thread_func, audio_write, FALSE, NULL);
	if(thread1 == NULL)
	{
		g_warning("%s Audio Write Thread Create Fail!!", __FUNCTION__);
		return FALSE;
	}

	g_message("%s Write1111 Filename[%s] fd[%d] ch[%d]", 
					__FUNCTION__, audio_write->filename, audio_write->fd, audio_write->ch);
	
	return TRUE;
}

static char nf_dev_jbshell_dsp_cmd_help[] = "dsp  input [ch_num]\n"
											"     read [filename] [min]\n";
static int nf_dev_jbshell_dsp_cmd(int argc, char **argv)
{
	gint fd=0;
	gint fd_read=0, fd_write=0;

	if(argc < 3)
		goto nf_dev_dsp_msg;

#if 0
	fd_read=open(DEVNAME_DSP, O_RDONLY, S_IREAD);
	fd_write=open(DEVNAME_DSP, O_WRONLY, S_IWRITE);
#else
	fd_read=_Fd_Arr[DEV_FD_IDX_DSP_READ];
	fd_write=_Fd_Arr[DEV_FD_IDX_DSP_WRITE];
#endif

	if(strcmp(argv[1], "input") == 0)
	{
		gint ch=0;

		ch=(gint)strtoul(argv[2], NULL, 10);
		
		if(!nf_dev_audio_input_ch_cntl(ch, fd_write))
			g_warning("%s Set Input Channel Fail!! Input Ch[%d]", __FUNCTION__, ch);
		else
			g_message("%s Set Input Channel [%d]", __FUNCTION__, ch);
	}
	else if(strcmp(argv[1], "read") == 0)
	{
		FILE *fp=NULL;
		char tmp[8]={0, };
		gint dsp_fd=0, i=0, min=0, ch=0;
		GTimeVal curr_timeval;
		glong out_sec=0;

		if(argc < 4)
			goto nf_dev_dsp_msg;

		fp=fopen(argv[2], "w");
		if(fp == NULL)
		{
			printf("File Open Fail!! filename[%s]\n", argv[2]);
			return -1;
		}
		else
			printf("File Open Success!! filename[%s]\n", argv[2]);

		min=(gint)strtoul(argv[3], NULL, 10);

		gettimeofday((struct timeval *)&curr_timeval, NULL);

		out_sec=curr_timeval.tv_sec+(60*min);

		g_message("\n\n%s Audio Data Read Start!!! During [%d]Min!! Out Sec [%ld]-->[%ld]\n\n",
			   					__FUNCTION__, min, curr_timeval.tv_sec, out_sec);

		dsp_fd=fd_read;

		ch=1;

		if(!nf_dev_audio_input_ch_cntl(ch, dsp_fd))
			g_warning("%s Set Input Channel Fail!! Input Ch[%d]", __FUNCTION__, ch);
		else
			g_message("%s Set Input Channel [%d]", __FUNCTION__, ch);

		while(1)
		{
			gettimeofday((struct timeval *)&curr_timeval, NULL);
			
			if(out_sec <= curr_timeval.tv_sec)
				break;
			
			if(read(dsp_fd, tmp, sizeof(tmp)) != sizeof(tmp))
			{
				g_warning("%s Read Error!!!", __FUNCTION__);
				break;
			}

			if(fwrite(tmp, sizeof(tmp), 1, fp) != 1)
			{
				g_warning("%s File Write Error!!\n", __FUNCTION__);
				break;
			}
		}
		g_message("\n\n%s Audio Data Read Finish!!! Curr Time[%ld] \n\n", __FUNCTION__, curr_timeval.tv_sec);
	}
	else if(strcmp(argv[1], "write") == 0)
	{
		FILE *fp=NULL;
		gint ch=0;
		guint filesize=0, remain=0;
		guchar tmp[8]={0, };
		struct stat f_stat;

		if(argc < 4)
			goto nf_dev_dsp_msg;

		fp=fopen(argv[2], "r");
		if(fp == NULL)
		{
			g_message("File Open Fail!! filename[%s]", argv[2]);
			return -1;
		}
		else
			g_message("File Open Success!! filename[%s]", argv[2]);

		if(stat(argv[2], &f_stat) == -1)
		{
			g_warning("%s Cannot Find Filename [%s]", __FUNCTION__, argv[2]);
			return -1;
		}
		filesize=(guint)f_stat.st_size;
		g_message("FileSize[%d]\n", filesize);

		ch=(gint)strtoul(argv[3], NULL, 10);

		if(ch > 1)
		{
			g_warning("%s CH is Wrong.. ch[%d]", __FUNCTION__, ch);
			return -1;
		}
		
		g_message("%s DSP File Write Start!!!", __FUNCTION__);	
		while(1)
		{
			if(fread(tmp, sizeof(tmp), 1, fp) != 1)
			{
				g_warning("%s File Read Error!!\n", __FUNCTION__);
				break;
			}
			
			write(fd_write, &tmp[ch], 1);
			
			filesize-=sizeof(tmp);
			if(filesize < sizeof(tmp))
			{
				g_message("Audio File Read Finish.. filesize[%d]", filesize);
				break;
			}
		}
		g_message("%s DSP File Read Start!!!", __FUNCTION__);	
	}
	else if(strcmp(argv[1], "rw") == 0)
	{
		gint ch_rd=0, ch_wr=0, min=0;

		if(argc < 7)
			goto nf_dev_dsp_msg;
		
		strcpy(audio_read.filename, argv[2]);
		audio_read.fd=fd_read;
		ch_rd=(gint)strtoul(argv[3], NULL, 10);
		audio_read.ch=ch_rd;
		min=(gint)strtoul(argv[4], NULL, 10);
		audio_read.min=min;

		strcpy(audio_write.filename, argv[5]);
		audio_write.fd=fd_write;
		ch_wr=(gint)strtoul(argv[6], NULL, 10);
		audio_write.ch=ch_wr;
		
		nf_dev_audio_rw_test(&audio_read, &audio_write);
	}
	else
		goto nf_dev_dsp_msg;
	
	return 0;

nf_dev_dsp_msg:
	printf("Invalid arguments\n%s\n", nf_dev_jbshell_dsp_cmd_help);
	return -1;
}
__commandlist(nf_dev_jbshell_dsp_cmd, "dsp", nf_dev_jbshell_dsp_cmd_help, nf_dev_jbshell_dsp_cmd_help);
#endif	/* defined(DEBUG_JBSHELL_UTIL_DEVICE) */ 
#endif



#if defined(USE_DEV_IVR_HF)

gint nf_dev_ivr_hf_init(ivr_hf_init_data *data)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_IVR_HF];
	gboolean ret = TRUE;
		
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_INIT, data);

	if(ret <0)
		printf("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	
	return ret;
}

gint nf_dev_ivr_hf_live_reset(ivr_hf_init_data *data)
{
	gint fd = _Fd_Arr[DEV_FD_IDX_IVR_HF];
	gboolean ret = TRUE;
		
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_LIVE_RESET, data);

	if(ret <0)
		printf("%s Ioctl error...ret[%d]", __FUNCTION__ , ret);

	
	return ret;
}

gint nf_dev_ivr_hf_set_display(ivr_hf_display *display)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_DISPLAY, display);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_set_hdrec(ivr_hf_hdrec* data)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_HDREC, data);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_set_sdrec(ivr_hf_sdrec* data)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_SDREC, data);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_get_vloss(guint *vloss)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_GET_VLOSS, vloss);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_set_md(ivr_hf_md *md)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_MD, md);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_set_md_display(int data)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_MD_DISPLAY, &data);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_get_md(guint *md_res)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_GET_MD_STATUS, md_res);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_get_hd_video_type(guint *type)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_GET_HD_VIDEO_TYPE, type);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gint nf_dev_ivr_hf_wr_reg_byte(gint reg, gint data)
{
	gboolean ret = TRUE;
	ivr_hf_wr wr_data;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	wr_data.reg=reg;
	wr_data.data=data;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_WRITE, &wr_data);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d] reg[0x%08x] data[0x%08x]", __FUNCTION__, ret, reg, data);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_ivr_hf_se_vloss_color(guint vloss)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_SET_VLOSS_COLOR, &vloss);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

#if 0
	gint nf_dev_ivr_hf_set_audio_ch(guint ch)
	{
		gboolean ret = TRUE;

		g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_IVR_HF]>0 , 0);

		ret = ioctl(_Fd_Arr[DEV_FD_IDX_IVR_HF], IVR_HF_GET_HD_VIDEO_TYPE, type);
		if(ret < 0)
			g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

		return (ret == 0) ? 1:0;
	}
#endif

#endif

#if defined(USE_DEV_TW2828)
gboolean nf_dev_tw2828_set_spot(TW2828_SPOT_INFO spot_seq_info)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TW2828]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TW2828], TW2828_SET_SPOT,  &spot_seq_info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_tw2828_set_osd(TW2828_OSD_INFO osd_info)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_TW2828]>0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_TW2828], TW2828_SET_OSD,  &osd_info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}

#endif

#if defined(USE_DEV_EDID)
gboolean nf_dev_edid_get_raw_data(struct edid_data *info, gboolean is_vga)
{
	gboolean ret = TRUE;
	guint mode=0;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_EDID]>0 , 0);

	if(is_vga)
		mode=EDID_GET_RAW_DATA_VGA;
	else
		mode=EDID_GET_RAW_DATA_HDMI;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_EDID], mode, info);
	if(ret < 0)
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);

	return (ret == 0) ? 1:0;
}
#endif

/*EOF*/


#if defined(USE_DEV_DRV_EXT)
gboolean nf_dev_drv_ext_cntl_loopout(gint ch, guint is_enable)
{
	gboolean ret = TRUE;
	struct drv_cntl_loopout info;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DRV_EXT]>0 , 0);
	
	#if defined(ENABLE_LOOPOUT)
		info.ch=ch;
		info.is_enable=is_enable;

		ret = ioctl(_Fd_Arr[DEV_FD_IDX_DRV_EXT], DRV_EXT_CNTL_LOOPOUT, &info);
		if(ret < 0)
			g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	#else
		ret=FALSE;
	#endif

	return (ret == 0) ? 1:0;
}

gboolean nf_dev_drv_ext_cntl_loopout_chk_chip(gboolean *is_chip)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_DRV_EXT]>0 , 0);
	
	#if defined(ENABLE_LOOPOUT)
		ret = ioctl(_Fd_Arr[DEV_FD_IDX_DRV_EXT], DRV_EXT_CNTL_LOOPOUT_CHK_CHIP, is_chip);
		if(ret < 0)
			g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	#else
		ret=FALSE;
	#endif

	return (ret == 0) ? 1:0;
}
#endif

#if defined (USE_DEV_POE)
gboolean nf_dev_poe_port_onoff(gint port, gboolean is_on, gint *is_fail)
{
	gboolean ret = TRUE;
	guint mode=0;
	NF_UTIL_POE_PORT_ONOFF port_onoff;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_POE] > 0 , 0);

	if (is_on == 1) {
		mode = POE_PORT_ON;
	} else {
		mode = POE_PORT_OFF;
	}

	port_onoff.port = port;
	ret = ioctl(_Fd_Arr[DEV_FD_IDX_POE], mode, &port_onoff);

	if (ret < 0) {
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	}

	*is_fail=port_onoff.is_fail;

	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_poe_get_info(NF_UTIL_POE_PORT_INFO *info)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_POE] > 0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_POE], POE_GET_INFO, info);
	if (ret < 0) {
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_poe_get_chip_infomation(NF_UTIL_POE_CHIP_INFO *info)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_POE] > 0 , 0);

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_POE], POE_GET_CHIP_INFORMATION, info);
	if (ret < 0) {
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1 : 0;
}

gboolean nf_dev_poe_get_info_single(int port, NF_UTIL_POE_INFO *info)
{
	gboolean ret = TRUE;

	g_return_val_if_fail(_Fd_Arr[DEV_FD_IDX_POE] > 0 , 0);
	g_return_val_if_fail(port < NUM_ACTIVE_CH, 0);

	info->reserved = port;

	ret = ioctl(_Fd_Arr[DEV_FD_IDX_POE], POE_GET_INFO_PORT, info);
	if (ret < 0) {
		g_warning("%s Ioctl error... ret[%d]", __FUNCTION__, ret);
	}

	return (ret == 0) ? 1 : 0;
}
static void _pirnt_mac(unsigned char *macaddr)
{
	int i = 0;
	printf("\033[0;36m %s [\033[0;39m", __FUNCTION__);
	for(i = 0; i < 6; i++)
		printf("\033[0;36m %02x\033[0;39m",macaddr[i]);
	printf("\033[0;36m ]\033[0;39m\n");
}

#if defined(DEBUG_JBSHELL_UTIL_DEVICE)
static char nf_dev_jbshell_poe_cmd_help[] = "info\n"
												"port on  port_num\n"
												"port off port_num\n";
static int nf_dev_jbshell_poe_cmd(int argc, char **argv)
{
	NF_UTIL_POE_PORT_INFO  info;

	if(argc < 2)
		goto nf_dev_poe_msg;

	if(strcmp(argv[1], "info") == 0)
	{
		gint i=0;

		if(argc < 2)
			goto nf_dev_poe_msg;

		memset(&info, 0x0, sizeof(NF_UTIL_POE_PORT_INFO));
		nf_dev_poe_get_info(&info);

		for(i=0; i<NUM_ACTIVE_CH; i++)
	{
			g_print("======================================\n");
			g_print("CH%d Discovery          --> %d\n", i, info.info[i].is_discovery);
			g_print("CH%d Active             --> %d\n", i, info.info[i].is_active);
			g_print("CH%d Port Class         --> %d\n", i, info.info[i].port_class);
			g_print("CH%d Func Status        --> %d\n", i, info.info[i].func_status);
			g_print("CH%d Voltage            --> %dV\n", i, info.info[i].voltage);
			g_print("CH%d Current            --> %dmA\n", i,info.info[i].current_mA);
			g_print("CH%d Power Comsumption  --> %dmW\n", i,info.info[i].consumption);
			g_print("\n\n");
			g_print("======================================\n");
		}
	}
	else if(strcmp(argv[1], "port") == 0)
	{
		gint port=0, is_fail;
		gboolean is_on=0;

		if(argc < 4)
			goto nf_dev_poe_msg;

		if(strcmp(argv[2], "on") == 0)
			is_on=TRUE;
		else if(strcmp(argv[2], "off") == 0)
			is_on=FALSE;
	else
			goto nf_dev_poe_msg;

		port=(gint)strtoul(argv[3], NULL, 10);

		nf_dev_poe_port_onoff(port, is_on, &is_fail);

		g_message("%s is_fail[%d]", __FUNCTION__, is_fail);
	}
	else if(strcmp(argv[1], "mac") == 0)
	{
		int i = 0;
		guchar mac_addr[16][6]={0,};
		nf_dev_switch_get_port_mac(mac_addr);
		for(i = 0; i < 16; i++)
			_pirnt_mac(mac_addr[i]);
	}
	else
		goto nf_dev_poe_msg;

	return 0;

nf_dev_poe_msg:
	printf("Invalid arguments\n%s\n", nf_dev_jbshell_poe_cmd_help);
	return -1;
			}
__commandlist(nf_dev_jbshell_poe_cmd, "poe", nf_dev_jbshell_poe_cmd_help, nf_dev_jbshell_poe_cmd_help);
#endif	/* defined(DEBUG_JBSHELL_UTIL_DEVICE) */
#endif	/* defined(USE_DEV_POE) */

#if defined(USE_DEV_SWITCH)
gboolean nf_dev_switch_init(int mode) 
{
	if (mode == NF_UTIL_SWITCH_CCTV_MODE) {
		// Set Vlan CCTV MODE. 
		nf_dev_switch_set_vlan(mode);
		// Set macaddr aging time to 5 min
		nf_dev_switch_set_aging_time(5);
		if (nf_sysman_qcmode_is_enable() == 1) {
			// #if defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824M4) || defined(_IPX_1648P4E) \
			//  || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
			// 	// Don't Recieve Packets From CAM Port
			// 	// nf_dev_switch_write(0, 0x1e, 0);
			// #endif
		}
	} else {
		// Set Vlan OPEN MODE. 
		nf_dev_switch_set_vlan(mode);
	}
}

gboolean nf_dev_switch_read(int phyaddr, unsigned char page, int regaddr, int *data)
{
	struct itx_ip1819_switch_data ip1819;

	memset(&ip1819, 0x00, sizeof(struct itx_ip1819_switch_data));

	ip1819.page = page;
	ip1819.mii.phy_id = phyaddr;
	ip1819.mii.reg_num= regaddr;
	g_static_mutex_lock (&_nf_dev_switch_mutex);
	ifr.ifr_data = &ip1819;
	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCGMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		g_static_mutex_unlock (&_nf_dev_switch_mutex);
		return FALSE;
	}
	*data = ip1819.mii.val_out;
	g_static_mutex_unlock (&_nf_dev_switch_mutex);
	return TRUE;
}

gboolean nf_dev_switch_write(int phyaddr, unsigned char page, int regaddr, int data)
{
	struct itx_ip1819_switch_data ip1819;
	
	memset(&ip1819, 0x00, sizeof(struct itx_ip1819_switch_data));
	ip1819.page = page;
	ip1819.mii.phy_id = phyaddr;
	ip1819.mii.reg_num= regaddr;
	ip1819.mii.val_in = data;
	g_static_mutex_lock (&_nf_dev_switch_mutex);
	ifr.ifr_data = &ip1819;
	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCSMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
				strerror(errno));
		g_static_mutex_unlock (&_nf_dev_switch_mutex);
		return FALSE;
	}
	g_static_mutex_unlock (&_nf_dev_switch_mutex);

	return TRUE;
}

/* ksi_test
	ARL Control Register Page 1
	addr : 0x01, default value : 0x0005
	bit[15] : Aging timer disable
	bit[14:0] : Aging timer (mg_aging_time + 1) *55.3 sec. +- 3.8%
*/
gboolean nf_dev_switch_set_aging_time(int sec)
{
	// sec |= 0x20; // Set time unit to seconds
	if(!nf_dev_switch_write(0, 0x01, P1REG_LUTAGINGTIME, sec))
	{
		g_warning("%s ERROR!!", __FUNCTION__); 
		return FALSE;
	}

	return TRUE;
}

int nf_dev_get_link_status(char *ifname) {
	int state = -1;
	int ret = 0;
	int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (socId < 0) 
	{
		g_warning("%s Socket failed...", __FUNCTION__);
		return -1;
	}
	
	struct ifreq if_req;
	(void) strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
	int rv = ioctl(socId, SIOCGIFFLAGS, &if_req);
	close(socId);

	if ( rv == -1)
	{
		g_warning("%s ioctl failed...", __FUNCTION__);
		return -1;
	}

	if(if_req.ifr_flags & IFF_UP)
	{
		ret |= (1<<0);
	}
	if(if_req.ifr_flags & IFF_RUNNING)
	{
		ret |= (1<<1);
	}
	return ret;
}

#define SWITCH_STATUS_REG	0x20 // page 3, SMI Port status for port 19
#define SWITCH_LINK_BIT		0x4
// #endif
gboolean nf_dev_switch_get_link_status(int *lport_mask)
{
	int port=0;
	int is_link = 0;

	if (*lport_mask < MAX_SWITCH_PORT_NUM) {
		port = *lport_mask;
		*lport_mask = 0;
		if (nf_dev_switch_read(port, 0x03, SWITCH_STATUS_REG, &is_link) == 0) {
			printf("[%s] ERROR!!\n", __FUNCTION__); 
			return FALSE;
		}

		if (is_link & SWITCH_LINK_BIT) {
			*lport_mask = 1;
		}
	} else {
		*lport_mask = 0;
		for (port = 0; port < MAX_SWITCH_PORT_NUM; port++) {
			if (nf_dev_switch_read(port, 0x03, SWITCH_STATUS_REG, &is_link) == 0) {
				printf("[%s] ERROR!!\n", __FUNCTION__); 
				return FALSE;
			}

			if (is_link & SWITCH_LINK_BIT) {
				*lport_mask |= (1 << port);
			}
		}
	}

	return TRUE;
}

// gboolean nf_dev_switch_get_port_mac(guchar mac_addr[8][6])
gboolean nf_dev_switch_get_port_mac(guchar mac_addr[16][6])
{
	g_static_mutex_lock (&_nf_dev_switch_mutex);
	ifr.ifr_data=mac_addr;
	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCLUTGET, &ifr) < 0)
	{
		printf("ioctl error [%s]\n", __FUNCTION__);
		g_static_mutex_unlock (&_nf_dev_switch_mutex);
		return FALSE;
	}
	g_static_mutex_unlock (&_nf_dev_switch_mutex);
#if 0
	else
	{
		for(i=0; i<5; i++)
		{
			printf("[%d] [", i);
			for(j=0; j<6; j++)
				printf("%02x:", mac_addr[i][j]);
			printf("\b]\n");
		}
	}
#endif
	return TRUE;
}

gboolean nf_dev_switch_get_mac_portnum(guchar mac_addr[6], int *port)
{
	guchar mac_info[7] = {0, };
	int i = 0;

	for (i = 0; i < 6; i++) {
		mac_info[i + 1] = mac_addr[i];
	}

	g_static_mutex_lock(&_nf_dev_switch_mutex);
	ifr.ifr_data = mac_info;

	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCLUTPORT, &ifr) < 0) {
		printf("[%s] Ioctl Error\n", __FUNCTION__);
		g_static_mutex_unlock(&_nf_dev_switch_mutex);
		return FALSE;
	}

	if (mac_info[0] == 0) {
		g_static_mutex_unlock(&_nf_dev_switch_mutex);
		return FALSE;
	} else {
		if (mac_info[0] >= NUM_ACTIVE_CH + 1) {
			// #if defined(_IPX_1648M4E) || defined(_IPX_1648P4E)|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
			*port = NUM_ACTIVE_CH_DVR ;
			// #else
			// 	*port = 8;
			// #endif
		} else {
			*port = mac_info[0] - 1;
		}
	}

	g_static_mutex_unlock(&_nf_dev_switch_mutex);

	return TRUE;
}

gboolean nf_dev_switch_set_vlan(int vlan_mode)
{
	struct _VLAN_setting VLAN_setting;
	int i, temp_vlan = 0;

	guint member[MAX_SWITCH_PORT_NUM] = {0,};

	if (vlan_mode == NF_UTIL_SWITCH_OPEN_MODE) {
		printf("[%s] Set VLAN OPEN MODE\n", __FUNCTION__);
		for (i = 0; i < MAX_SWITCH_PORT_NUM; i++) {
			member[i] = 0x1fffffff;
		}
	} else {
		if (nf_sysman_qcmode_is_enable() == 1){
			guint port_data = 0x30000000;
			printf("[%s] Set QC VLAN CCTV MODE\n", __FUNCTION__);

			for (i = 0; i < 12; i++) {
				member[i] = port_data | (1 << i);
			}

			for (i = 0; i < 6; i++) {
				member[i+12] = port_data | (1 << (i+20));
			}
			member[NF_UTIL_SWITCH_PORT_CPU] = 0x3fffffff;
		} else {
			guint port_data = 0x21000000;
			printf("[%s] Set VLAN CCTV MODE\n", __FUNCTION__);
			for (i = 0; i < MAX_SWITCH_PORT_NUM; i++) { // PORT 1 ~ PORT 12 설정
				member[i] = (port_data | (1 << i));
			}

			member[17] = 0x1fffffff;
			member[19] = 0x31000000;
		}
	}

	memset(&VLAN_setting, 0x00, sizeof(VLAN_setting));

	VLAN_setting.cmd |= CMD_READ;

	g_static_mutex_lock (&_nf_dev_switch_mutex);
	ifr.ifr_data = &VLAN_setting.cmd;
	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCVLAN, &ifr) < 0) {
		printf("No Support Read PVLAN...");
		g_static_mutex_unlock(&_nf_dev_switch_mutex);
		return FALSE;
	}

	VLAN_setting.cmd |= CMD_WRITE;  //bit4=1 --> Write
	VLAN_setting.vmode = VLAN_MODE_ALL_PBASE;

	for (i = 0; i < PBASE_VLAN_NUM; i++) {
		VLAN_setting.VLAN_entry[i].member = member[i];    // set port 5~0
	}

	if (ioctl(_Fd_Arr[DEV_FD_IDX_SWITCH], SIOCVLAN, &ifr) < 0) { 
		printf("No Support Write PVLAN...");
		g_static_mutex_unlock(&_nf_dev_switch_mutex);
		return FALSE;
	}

	g_static_mutex_unlock(&_nf_dev_switch_mutex);

	return TRUE;
}
#endif