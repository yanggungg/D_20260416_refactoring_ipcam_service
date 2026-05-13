#include <sys/types.h>

#include "nf_common.h"
#include "nf_util_fw_single.h"
#include "nf_sysman.h"
#include "nf_util_time.h"
#include "nf_util_fw.h"
#include "nf_util_fw_crc.h"
#include "nf_util_device.h"
#include "nf_qc.h"
#include "nf_qc_app.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_app.h"
#include "nf_micom_upgrade.h"
#define DEBUG_JBSHELL_MICOM
#if defined(DEBUG_JBSHELL_MICOM)
	#include "jbshell.h"
#endif

static int micom_trd_end=0;
static int micom_err_flag=0;
static int micom_getc_cnt=0;
static const char MICOM_HEX[] = "0123456789ABCDEF";

//#define MICOM_DEBUG 1
#define NF_MICOM_CRC_CHECK

int micom_is_upgrade(image_header_t *micom_img_header);
int micom_upgrade_from_nand(int is_forceup);

static int micom_fw_write(char *psrc, unsigned int size);
static int micom_fw_erase();
static int micom_fw_verify(char *psrc, unsigned int size);
static unsigned char micom_chartoint(unsigned char data);
void print_micom_fw_data(u_char *psrc);
static void micom_print_header(image_header_t *info);

int micom_check_ldrom();
void micom_boot_aprom();
void micom_boot_ldrom();
void micom_clear_data_register();

void micom_upgrade_led_blink(int event)
{
	NF_NOTIFY_INFO *led_network = NULL;
	NF_NOTIFY_INFO *led_rec=0;
	u_int led_alarm=0;
	u_int rec_chk=0;
	u_int ch = 0;
	u_int try_cnt = 0;

	if(event)
	{
		// led_alarm = _event_get_physical_alarm_mask() | _event_get_virtual_alarm_mask();
		led_network = nf_notify_get("net_status");
		led_rec = nf_notify_get("analog_rec");

		// printf("\033[0;36m %s ALARM %x\033[0;39m\n", __FUNCTION__, led_alarm);
		// if(led_alarm != 0)
		// 	nf_dev_keypad_led_on(ALARM);
		// else
		// 	nf_dev_keypad_led_off(ALARM);
	
		if(led_network != NULL)
		{
			printf("\033[0;36m %s NETWORK conn[%d] live[%d] play[%d]\033[0;39m\n", __FUNCTION__,
			led_network->d.params[0], led_network->d.params[1], led_network->d.params[2]);

			if(led_network->d.params[0])
				nf_dev_keypad_led_on(NETWORK);
			else
				nf_dev_keypad_led_off(NETWORK);
		}

		if(led_rec != NULL)
		{
			for (ch = 0; ch < NUM_CHANNEL; ch++)
			{
				printf("\033[0;36m %s REC %c\033[0;39m\n", __FUNCTION__, led_rec->c.chmap[ch]);
				if ((led_rec->c.chmap[ch] != ' ') && (led_rec->c.chmap[ch] != 'p'))
					rec_chk++;	
			}

			if(rec_chk)
				nf_dev_keypad_led_on(REC);
			else
				nf_dev_keypad_led_off(REC);
		}

		nf_dev_keypad_led_on(POWER);
		nf_disk_HDD_LED_ON();
	}
	else
	{
		for(try_cnt=0 ; try_cnt<5; try_cnt++)
		{
			// nf_dev_keypad_led_off(ALARM);
			nf_dev_keypad_led_off(NETWORK);
			nf_dev_keypad_led_off(REC);
			nf_dev_keypad_led_on(POWER);
			nf_disk_HDD_LED_ON();
		}
	}
}

void micom_upgrade_thread_create(void)
{
	pthread_t micom_up_trd;
	int thr_id;
	int status;

	GTimeVal start_time, curr_time;
	gettimeofday((struct timeval *)&start_time, NULL);

	thr_id = pthread_create(&micom_up_trd, NULL, micom_upgrade_from_nand, 0);
	if(thr_id < 0)
	{
		perror("thread create error : ");
		return ;
	}
	printf("\e[33m [%s] micom thread start\e[0m\n", __FUNCTION__);

	while(1)
	{
		gettimeofday((struct timeval *)&curr_time, NULL);
		if(start_time.tv_sec+60 < curr_time.tv_sec)
		{
			printf("\e[33m [%s] time over\e[0m\n", __FUNCTION__);
			nf_dev_board_reset();
		}
		if(micom_trd_end == 1)
			break;
		sleep(1);
	}
	pthread_join(micom_up_trd, (void *)&status);
}

int micom_is_upgrade(image_header_t *micom_img_header)
{
	int ret = 0;

	if (strnicmp((char *)micom_img_header->ih_name, MICOM_FW_HEADER_MAGIC, strlen(MICOM_FW_HEADER_MAGIC)) == 0) {
		ret = 1;
	} else {
		printf("[Micom][Up] Invalid Image Name --> [%32.32s]\n", micom_img_header->ih_name);
	}

	printf("[Micom][Up] check upgrade.. [%d]\n", ret);

	return ret;
}

int micom_upgrade_from_nand(int is_forceup)
{
	#if defined(SUPPORT_STORAGE_NAND)
		char dataBuf[FW_UPGRADE_NAND_PAGE_SIZE]={0, };
		u_char readBlk[MICOM_NAND_ERASE_SIZE]={0, };
	#endif
	int retry_cnt=0, ret=0, is_fail=FALSE;
	u_int micom_size=0;
	u_int nand_dcrc=0, micom_dcrc=0;
	u_char *psrc=NULL;

	micom_trd_end = 0;

	image_header_t micom_img_header;
	NF_PARAM_HW hw_param;

	memset(&hw_param, 0x0, sizeof(NF_PARAM_HW));
	memset(&micom_img_header, 0x0, sizeof(image_header_t));

	nf_api_param_hw_get_protect(&hw_param, 0);
	if(strncmp(hw_param.magic, NF_HW_PARAM_MAGIC, 4) != 0 )
	{
		printf("\e[33m [%s] hwparam magic fail\e[0m\n", __FUNCTION__);
		memset(&hw_param, 0x00, sizeof(hw_param));
	}

	#if defined(SUPPORT_STORAGE_NAND)
		if (!nf_flash_read(FW_UPGRADE_DSP_PING_MTD_NUM, 0, 0, (unsigned char *)dataBuf, NULL)) {
			printf("[Micom][Up] Micom Header Data Read Fail!!\n");
			goto micom_up_fail_nand;
		}
		memcpy(&micom_img_header, dataBuf, sizeof(image_header_t));
	#endif

	#if 1
		micom_print_header(&micom_img_header);
	#endif

	if (!is_forceup)
	{
		ret = micom_is_upgrade(&micom_img_header);
		if (ret == 0)
		{
			printf("[Micom][Up] Don't need to MICOM Upgrade...\n");
			goto micom_up_success_nand;
		}
	}

	micom_size = ntohl(micom_img_header.ih_size);

	#if defined(SUPPORT_STORAGE_NAND)
		if (!nf_flash_read_block(FW_UPGRADE_DSP_PING_MTD_NUM, 0, (unsigned char *)readBlk)) {
			printf("[Micom][Up] Micom Data Read Fail!!\n");
			goto micom_up_fail_nand;
		}
	#endif

	#if defined(NF_MICOM_CRC_CHECK)
		micom_dcrc = ntohl(micom_img_header.ih_dcrc);
		#if defined(SUPPORT_STORAGE_NAND)
			nand_dcrc = crc32(0, (u_char *)(readBlk + sizeof(image_header_t)), micom_size);
		#endif

		if (micom_dcrc != nand_dcrc) {
			printf("[Micom][Up] Micom Data Crc Check Error!!!, nand_dcrc[%08x] micom_dcrc[%08x]\n", nand_dcrc, micom_dcrc);
		} else {
			printf("[Micom][Up] Micom Data CRC Check Passed!!!\n");
		}
	#endif

	printf("[Micom][Up] ===============> MICOM upgrade Start\n");

	psrc=(u_char *)(readBlk + sizeof(image_header_t));

	nf_dev_micom_upgrade_flag_set(1);
	sleep(1);

	micom_boot_ldrom();
	sleep(1);

micom_up_retry:
	micom_err_flag = 0;
	printf("[Micom][Up] ===============> MICOM Erase Start\n");
	if (micom_fw_erase()) {
		printf("[Micom][Up] ===============> MICOM Erase Finish\n");
	} else {
		printf("[Micom][Up] ===============> MICOM Erase Fail!!\n");
		goto micom_up_fail_nand;
	}

	printf("[Micom][Up] ===============> MICOM Write Start.. size %d\n", micom_size);
	micom_fw_write((char *)psrc, micom_size);

	printf("[Micom][Up] ===============> MICOM Verify Start\n");
	is_fail = micom_fw_verify((char *)psrc, micom_size);

	if (is_fail) {
		printf("[Micom][Up] ===============> MICOM Verify Fail\n");
		printf("[Micom][Up] ===============> MICOM Upgrade Fail\n");
		printf("[Micom][Up] ===============> MICOM Upgrade Retry %d\n", ++retry_cnt);
		micom_boot_ldrom();
		sleep(1);

		if (retry_cnt == 3)
			goto micom_up_fail_nand;

		goto micom_up_retry;
	} else {
		printf("[Micom][Up] ===============> MICOM Verify Finish\n");
		printf("[Micom][Up] ===============> MICOM BOOT APROM\n");
		micom_boot_aprom();
		micom_boot_aprom();
		printf("[Micom][Up] ===============> MICOM Upgrade Finish\n");
	}

	printf("[Micom][Up] ===============> Wait for Micom Boot..\n");
	sleep(3);
	ret = nf_dev_micom_check_aprom();
	if(ret) {
		printf("[Micom][Up] ===============> APROM Check.. pass..\n");

		nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_MICOM_UPGRADE, 0);
		nf_dev_micom_upgrade_flag_set(0);

		micom_upgrade_led_blink(0);
	}
	else {
		printf("[Micom][Up] ===============> APROM Check.. fail..\n");
	}

micom_up_success_nand:
	micom_trd_end = 1;
	return TRUE;

micom_up_fail_nand:
	micom_trd_end = 1;
	return FALSE;
}

static int micom_fw_write(char *psrc, unsigned int size)
{
	unsigned char buf[8]={0, };
	int i=0, j=(size % MICOM_DATA_SIZE), k=0;
	char data=0;
	unsigned char temp=0, flag_end=0;

	for (k = size; k > 0; k -= MICOM_DATA_SIZE) {
		if ((k - MICOM_DATA_SIZE) < 0)
			flag_end = 1;

		for (i = 0; i < MICOM_WRITE_SIZE; i++) {
			buf[0] = 'w';
			buf[1] = 's';

			if (i == 0) {
				buf[2] = MICOM_HEX[0];
				if (flag_end)
					buf[3] = MICOM_HEX[0];
				else
					buf[3] = MICOM_HEX[1];
			} else {
				if (flag_end && i > j)
					temp = 0xff;
				else
					temp = *psrc++;
				buf[2] = MICOM_HEX[(temp >> 4) & 0xf];
				buf[3] = MICOM_HEX[temp & 0xf];
			}

			buf[4] = 'z';
			nf_dev_micom_puts((const char *)buf);
			#if defined(MICOM_DEBUG)
				printf("[Micom][Up][FW_W] k[%d] i[%d] write data 0x%x\n", k, i, *psrc);
			#endif
		}

		if (!flag_end) {
			data = nf_dev_micom_getc();
			if (data == 'r') {
				#if defined(MICOM_DEBUG)
					printf("[Micom][Up][FW_W] %d. getc err flag = %d, cnt = %d, data = %d\n", 
								k, micom_err_flag, micom_getc_cnt, data);
				#endif
			} else {
				printf("[Micom][Up][FW_W] %d data write fail, flag = %d, cnt = %d, data = %d\n", 
							k, micom_err_flag, micom_getc_cnt, data);
				return 0;
			}
		}
	}

	data = nf_dev_micom_getc();
	if (data == 'c') {
		#if defined(MICOM_DEBUG)
			printf("[Micom][Up][FW_W] micom getc data = %c\n", data);
		#endif
	} else {
		printf("[Micom][Up][FW_W] %d data write fail, flag = %d, cnt = %d, data = %d\n", 
						k, micom_err_flag, micom_getc_cnt, data);
		return 0;
	}

	return 1;
}

static int micom_fw_erase(void)
{
	unsigned char buf[8]={0, };
	unsigned char data=0;
	int flush_cnt=0;
	#if defined(MICOM_DEBUG)
		int i=0;
	#endif

	buf[0] = 'e';
	buf[1] = 's';
	buf[2] = 'z';

	nf_dev_micom_puts((const char *)buf);
	
	sleep(1);

	data = nf_dev_micom_getc();

	//data flush
	while(flush_cnt<18) {
		if (data == 'a' || data == '5' || data == 10 || data == 13) {
			printf("[Micom][Up][FW_E] data flush : %d\n", data);
			data = nf_dev_micom_getc();
			flush_cnt++;
		}
		else
			break;
	}
	printf("[Micom][Up][FW_E] data : %c\n", data);

	if (data == 'c') {
		#if defined(MICOM_DEBUG)
			printf("[Micom][Up][FW_E] Erase Finish.. getc_cnt = %d, micom data = 0x%x %c\n", micom_getc_cnt, data, data);
		#endif
		return 1;
	} else if (data == 0xf3) {
		data = nf_dev_micom_getc();
		#if defined(MICOM_DEBUG)
			printf("[Micom][Up][FW_E] %d. micom data = 0x%x %c\n", i++, data, data);
		#endif
		if (data == 'c')
			return 1;
	} else {
		return 0;
	}

	return 0;
}

static int micom_fw_verify(char *psrc, unsigned int size)
{
	unsigned char buf[8] = {0, };
	char temp = 0;
	int i = 0, j = 0, k = 0;
	unsigned char nand_data = 0;
	unsigned char data[16] = {0,};
	unsigned char cmp = 0;
	int cmp_len = size + (size % MICOM_CMP_DATA_CNT);

	buf[0] = 'r';
	buf[1] = 's';
	buf[2] = 'z';
	
get_data:
	nf_dev_micom_puts((const char *)buf);
	buf[1] = 'r';
	
	while (1) {
		if (k > cmp_len - MICOM_CMP_DATA_CNT)
			break;

		temp = nf_dev_micom_getc();
		if ((temp >= 0x30 && temp <= 0x39) || (temp >= 0x41 && temp <= 0x46)) {
			data[i++] = temp;
				
			#if defined(MICOM_DEBUG)
				printf("[Micom][Up][FW_V] temp 0x%02x -> %x\n", temp, temp);
			#endif
		} else if (temp == 'r') {
			i = 0;
			for (j = 0; j < MICOM_CMP_DATA_CNT; j++) {
				cmp = (micom_chartoint(data[j * 2]) << 4) | micom_chartoint(data[(j * 2) + 1]);
				nand_data = (*psrc) & 0xff;
				psrc++;
				if (nand_data != cmp) {
					printf("[Micom][Up][FW_V] %d. nand data = 0x%x, micom data = 0x%x Fail!!!\n", k, nand_data, cmp);
					return 1;
				}
				#if defined(MICOM_DEBUG)
				else {
					printf("[Micom][Up][FW_V] %d. micom data = 0x%x micom data = 0x%x\n", i++, nand_data, cmp);
				}
				#endif
				k++;
			}
		
			goto get_data;
		}
	}
	return 0;
}

static unsigned char micom_chartoint(unsigned char data)
{
	if (data >= 0x30 && data <= 0x39)
		data -= 48;
	else if (data >= 0x41 && data <= 0x46)
		data -= 55;

	return data;
}

void print_micom_fw_data(u_char *psrc)
{
	int i = 0;

	while (i != 7000) {
		if (i % 16 == 0) {
			printf("\naddr %08x | ", i);
		}

		printf("%02x ", *psrc++);
		i++;
	}

	printf("\n");
}

static void micom_print_header(image_header_t *info)
{
	printf("==================================================================\n");
	printf("ih_magic [%08x]\n", info->ih_magic);            /* Image Header Magic Number    */
	printf("ih_hcrc  [%08x]\n", info->ih_hcrc);             /* Image Header CRC Checksum    */
	printf("ih_time  [%08x]\n", info->ih_time);             /* Image Creation Timestamp     */
	printf("ih_size  [%d]\n", ntohl(info->ih_size)); 	    /* Image Data Size              */
	printf("ih_load  [%08x]\n", ntohl(info->ih_load));		/* Data  Load  Address          */
	printf("ih_ep    [%08x]\n", ntohl(info->ih_ep));		/* Entry Point Address          */
	printf("ih_dcrc  [%08x]\n", info->ih_dcrc);             /* Image Data CRC Checksum      */
	printf("ih_os    [%x]\n", info->ih_os);                 /* Operating System             */
	printf("ih_arch  [%x]\n", info->ih_arch);               /* CPU architecture             */
	printf("ih_type  [%x]\n", info->ih_type);               /* Image Type                   */
	printf("ih_comp  [%x]\n", info->ih_comp);               /* Compression Type             */
	printf("ih_name  [%-32.32s]\n", info->ih_name);         /* Image Name                   */
	printf("==================================================================\n");
}


int micom_check_ldrom()
{
    int data=0;
    
    nf_dev_micom_puts("erz");

    data=nf_dev_micom_getc();
	#if defined(MICOM_DEBUG)
		printf("%s getc data = %d\n", __FUNCTION__, data);
	#endif
    if (data == 'c') {

		return TRUE;
	}

    return FALSE;
}

void micom_boot_aprom()
{
	unsigned char buf[8] = {0, };

	buf[0] = 'b';
	buf[1] = 'a';
	buf[2] = 'z';

	nf_dev_micom_puts((const char *)buf);
}

void micom_boot_ldrom()
{
	unsigned char buf[8] = {0, };

	buf[0] = 'b';
	buf[1] = 'l';
	buf[2] = 'z';

	nf_dev_micom_puts((const char *)buf);
}

void micom_clear_data_register()
{
	int i = 0;
	for (i=0; i<10; i++)
		nf_dev_micom_getc();
}

#if defined(DEBUG_JBSHELL_MICOM)
static char nf_micom_up_jbshell_cmd_help[] = "mup up\n";
static int nf_micom_up_jbshell_cmd(int argc, char **argv)
{
	if(argc < 2) {
		printf("[Micom][Up][JBSHELL] Invalid arguments\n%s\n", nf_micom_up_jbshell_cmd_help);
		return -1;
	}

	if(strcmp(argv[1], "up" ) == 0){
		micom_upgrade_thread_create();
	}
	else {
		goto nf_micom_up_help_cmd;
	}

    return 0;

nf_micom_up_help_cmd:
	printf("[Micom][Up][JBSHELL] Invalid arguments\n%s\n", nf_micom_up_jbshell_cmd_help);

    return -1;
}

__commandlist(nf_micom_up_jbshell_cmd, "mup", nf_micom_up_jbshell_cmd_help, nf_micom_up_jbshell_cmd_help);

#endif

