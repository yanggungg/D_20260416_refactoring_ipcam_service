#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>


#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/soundcard.h>
#include <netinet/in.h>
#include <glib.h>


#include "nf_sysman.h"
#include "nf_util_netif.h"
#include "nf_util_device.h"
#include "nf_sysdb.h"
#include "nf_api_cam.h"
#include "nf_qc.h"
#include "nf_qc_app.h"
#include "nf_qc_main.h"
#include "nf_qc_audio.h"
#include "nf_ptz.h"

#include "nf_codec_header.h"

#include <novatek/hdal.h>
#include "nf_audio_common.h"
#include "nf_audio_novatek.h"
#include "nf_audio.h"

#define QC_TEST_ALARM 		30
#define QC_TEST_AUDIO 		31
#define QC_TEST_NET 		32
#define QC_TEST_SDCARD 		33
#define QC_TEST_LED 		34
#define QC_TEST_RS485 		35
#define QC_TEST_FAC_KEY		36
#define QC_TEST_CVBS		37
#define QC_TEST_IRIS		38
#define QC_TEST_DAY_NIGHT	39
#define QC_TEST_ZOOM_FOCUS	40
#define QC_TEST_IR			41
#define QC_TEST_MIC			43
#define QC_TEST_END			42
#define QC_TEST_NUM_MIN		QC_TEST_ALARM	
#define QC_TEST_NUM_MAX		QC_TEST_IR

float	Sine[] = {
	0.000000, 0.024541, 0.049068, 0.073565,	0.098017, 0.122411,
	0.146730, 0.170962, 0.195090, 0.219101, 0.242980, 0.266713,
	0.290285, 0.313682, 0.336890, 0.359895, 0.382683, 0.405241,
	0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998,
	0.555570, 0.575808, 0.595699, 0.615232,	0.634393, 0.653173,
	0.671559, 0.689541, 0.707107, 0.724247, 0.740951, 0.757209,
	0.773010, 0.788346, 0.803208, 0.817585, 0.831470, 0.844854,
	0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.914210,
	0.923880, 0.932993, 0.941544, 0.949528,	0.956940, 0.963776,
	0.970031, 0.975702, 0.980785, 0.985278, 0.989177, 0.992480,
	0.995185, 0.997290, 0.998795, 0.999699,	1.000000, 0.999699,
	0.998795, 0.997290, 0.995185, 0.992480, 0.989177, 0.985278,
	0.980785, 0.975702, 0.970031, 0.963776,	0.956940, 0.949528,
	0.941544, 0.932993, 0.923880, 0.914210, 0.903989, 0.893224,
	0.881921, 0.870087, 0.857729, 0.844854,	0.831470, 0.817585,
	0.803208, 0.788346, 0.773010, 0.757209, 0.740951, 0.724247,
	0.707107, 0.689541, 0.671559, 0.653173,	0.634393, 0.615232,
	0.595699, 0.575808, 0.555570, 0.534998, 0.514103, 0.492898,
	0.471397, 0.449611, 0.427555, 0.405241, 0.382683, 0.359895,
	0.336890, 0.313682, 0.290285, 0.266713, 0.242980, 0.219101,
	0.195090, 0.170962, 0.146730, 0.122411,	0.098017, 0.073565,
	0.049068, 0.024541, 0.000000, -0.024541, -0.049068, -0.073565,
	-0.098017, -0.122411, -0.146730, -0.170962, -0.195090, -0.219101,
	-0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
	-0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898,
	-0.514103, -0.534998, -0.555570, -0.575808, -0.595699, -0.615232,
	-0.634393, -0.653173, -0.671559, -0.689541, -0.707107, -0.724247,
	-0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
	-0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224,
	-0.903989, -0.914210, -0.923880, -0.932993, -0.941544, -0.949528,
	-0.956940, -0.963776, -0.970031, -0.975702, -0.980785, -0.985278,
	-0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699,
	-1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480,
	-0.989177, -0.985278, -0.980785, -0.975702, -0.970031, -0.963776,
	-0.956940, -0.949528, -0.941544, -0.932993, -0.923880, -0.914210,
	-0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854,
	-0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209,
	-0.740951, -0.724247, -0.707107, -0.689541, -0.671559, -0.653173,
	-0.634393, -0.615232, -0.595699, -0.575808, -0.555570, -0.534998,
	-0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241,
	-0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713,
	-0.242980, -0.219101, -0.195090, -0.170962, -0.146730, -0.122411,
	-0.098017, -0.073565, -0.049068, -0.024541
};

#define	sine(x)		Sine[(x)]
#define DB double
#define pi 3.14
#define MAX 1024

int rs485_handle = -1;

int m_led_text = 0;
int m_key_text = 0;
int m_rtc_text = 0;
int m_rs485_text = 0;
int m_audio_text = 0;
int m_alarm_text = 0;
int m_cvbs_text = 0;
int m_daynight_text = 0;
int m_iris_text = 0;
int m_key = 0;
int m_led = 0;
int m_mic = 0;
int m_rtc = 0;
int m_rs485 = 0;
int m_audio = 0;
int m_alarm = 0;
int m_daynight = 0;
int m_test = 0;
int m_wait = 0;
int m_audio_warning = 0;
int m_ping = 0;
int m_mac = 0;
int m_receive_mac = 0;
int m_iris = 0;
int m_sdcard = 0;
int m_zoomfocus = 0;

int m_rs485_ret = 0;
int m_audio_ret = 0;

int m_select = 0;
int m_ir = 0;
int vio_select = 0;

void initQC(void)
{
	m_led_text = 0;
	m_key_text = 0;
	m_rtc_text = 0;
	m_rs485_text = 0;
	m_audio_text = 0;
	m_alarm_text = 0;
	m_cvbs_text = 0;
	m_daynight_text = 0;
	m_iris_text = 0;
	m_key = 0;
	m_led = 0;
	m_mic = 0;
	m_rtc = 0;
	m_rs485 = 0;
	m_audio = 0;
	m_alarm = 0;
	m_daynight = 0;
	m_test = 0;
	m_wait = 0;
	m_audio_warning = 0;
	m_ping = 0;
	m_mac = 0;
	m_receive_mac = 0;
	m_iris = 0;
	m_sdcard = 0;
	m_zoomfocus = 0;

	m_rs485_ret = 0;
	m_audio_ret = 0;

	m_select = 0;
	m_ir = 0;
	vio_select = 0;
}
int comMode(trans_info transInfo, int cmd, char*msg , int msgParam )
{
	char tmpBuf[1024]={0,};
	int len = 0;
	int ret = 0;
	if(msg == NULL) return -1;
	if(transInfo.mode == NET)
	{
		switch (cmd)
		{
			case READ:
				memset(tmpBuf,0,1024);
				read(transInfo.sock,&len,sizeof(int));
				read(transInfo.sock,msg,len);
				printf("[read len %d ]%s\n",len,msg);
				ret = len;
			break;
			case WRITE:
				write(transInfo.sock,msg,strlen(msg));
			break;
			case PROTOCOL:
				sprintf(tmpBuf,"%c%s%c",0x02,msg,0x03);
				write(transInfo.sock,tmpBuf,strlen(tmpBuf));
			break;
			case PROTOCOL_PARAM:
				sprintf(tmpBuf,"%c%s%d%c",0x02,msg,msgParam,0x03);
				write(transInfo.sock,tmpBuf,strlen(tmpBuf));
			break;
		}
	}
	else if(transInfo.mode == COM)
	{
		switch (cmd)
		{
			case READ:
				ret = scanf("%s",msg);
			break;
			case WRITE:
				printf(tmpBuf);
			break;
			case PROTOCOL:
				sprintf(tmpBuf,"%c%s%c",0x02,msg,0x03);
				printf(tmpBuf);
			break;
		}
	}
	return ret;
}
int himmSend(trans_info transInfo, char *himmCmd)
{
	char arrHimmCmd[100]={0,};
	char arrReadBuf[1024]={0,};
	ssize_t rd_size;
	int fd;
	//printf("\e[33m [%s] himmSend START\e[0m\n", __FUNCTION__); 
	if(transInfo.mode == NET)
	{
		sprintf(arrHimmCmd,"%s > himm.txt",himmCmd);
		system(arrHimmCmd);
		if ( 0 < ( fd = open( "./himm.txt", O_RDONLY)))
		{
			while( 0 < ( rd_size = read( fd, arrReadBuf, 1024-1)))   
	      		{
	        		arrReadBuf[rd_size]  = '\0';
	         		comMode(transInfo, WRITE, arrReadBuf ,0);
				//printf("\e[33m [%s] himm = %s\e[0m\n", __FUNCTION__,arrReadBuf); 
	     		 }
	      		close( fd);
		}
		else
	  	{
	  		printf("\e[33m [%s] file open failed!!\e[0m\n", __FUNCTION__); 
	   	}
	}
	else if(transInfo.mode == COM)
	{
		//printf("\e[33m [%s] himmSend COM\e[0m\n", __FUNCTION__); 
		system(arrHimmCmd);
	}
	//printf("\e[33m [%s] himmSend END\e[0m\n", __FUNCTION__); 
	
}
/*===============================================
RTC
===============================================*/
time_t cur_time_return(char *c_time)
{
	char *month[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",0};
	char *days[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun",0};

	struct tm user_time;
	time_t cur_time=0;
	unsigned char c_year, c_month, c_day, c_hour, c_min, c_sec,c_days;

	FILE *fp;
	char str[100];
	char t_str[16];
	int i;
	int g_step = 0;

	printf("-------->  Time Get Start...\n");
	{
		sprintf(str,"hwclock -r >rtc_test");
		system(str);

		fp = fopen("./rtc_test","rt");

		g_step = 0;

		while(!feof(fp))
		{
			memset(str,0,sizeof(str));
			fscanf(fp,"%s",str);
			//printf("%s \n",str);
			//printf("---> %d \n",g_step);
			switch(g_step)
			{
				case 0 :
					for(i=0;i<7;i++)
					{
						if(strcmp(str,days[i]) == 0)
						{
							c_days = i + 1;
						}
					}
					break;

				case 1 :
					for(i=0;i<=11;i++)
					{
						//printf("%d \n",i);
						if(strcmp(str,month[i]) == 0)
						{
							c_month = i + 1;
						}
					}
					break;
				case 2 :
					c_day = atoi(str);
					break;
				case 3 :
					t_str[0] = str[0];
					t_str[1] = str[1];
					t_str[2] = 0;
					c_hour = atoi(t_str);
					t_str[0] = str[3];
					t_str[1] = str[4];
					t_str[2] = 0;
					c_min = atoi(t_str);
					t_str[0] = str[6];
					t_str[1] = str[7];
					t_str[2] = 0;
					c_sec = atoi(t_str);
					break;
				case 4 :
					c_year = atoi(str);

					printf("YEAR :%d , MON : %d , DAYS : %d , DAY : %d , HOUR : %d , MIN : %d , SEC : %d \n",c_year , c_month , c_day , c_days , c_hour , c_min , c_sec);
					break;
			}
			g_step++;
		}

		fclose(fp);
		system("rm ./rtc_test");
	}

	c_year -= 2000;
	printf("%4d Y, %02d M, %02d D, %02d H, %02d M, %02d S\n",c_year+2000 ,c_month ,c_day ,c_hour ,c_min ,c_sec );
	sprintf(c_time,"%04d-%02d-%02d %02d:%02d:%02d",c_year+2000,c_month,c_day,c_hour,c_min,c_sec);

	user_time.tm_year  = c_year+2000-1900;     // start year 1900
	user_time.tm_mon   = c_month-1;            // start month 0
	user_time.tm_mday  = c_day;
	user_time.tm_hour  = c_hour;
	user_time.tm_min   = c_min;
	user_time.tm_sec   = c_sec;

	cur_time = mktime(&user_time);

	return cur_time;
}
/*===============================================
AUDIO
===============================================*/
void FastFourierTransform(DB Re[MAX], DB Im[MAX], int p)
{
	int i, ip, j, k, m, me, me1, n, nv2;
	DB uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;

	DB temp_cos[8] = {-0.999999,0.000796,0.707388,0.923956,0.980805,0.995190,0.998797,0.999699};
	DB temp_sin[8] = {-0.001593,-1.000000,-0.706825,-0.382499,-0.194993,-0.097968,-0.049043,-0.024529};

	n = 1 << p;		//number of step
	nv2 = n / 2;
	j =0;			//initalize

	//part of bit reverse
	for (i = 0; i < n - 1; i++)
	{
		if (j > i)
		{
			tRe = Re[j];	tIm = Im[j];
			Re[j] = Re[i];	Im[j] = Im[i];
			Re[i] = tRe;	Im[i] = tIm;
		}

		k = nv2;
		while (j >= k) {
			j -= k;
			k /= 2;
		}
		j += k;
	}

	//butterfly loop
	for(m = 1; m <= p; m++)
	{
		me = 1 << m;	me1 = me / 2;
		uRe = 1.0;	uIm = 0.0;
		//wRe = cos(pi / me1); wIm =- sin(pi / me1);
		wRe = temp_cos[m-1]; wIm = temp_sin[m-1];

		for(j = 0; j < me1; j++)
		{
			for(i = j; i < n; i += me)
			{
				ip = i + me1;
				tRe = Re[ip] * uRe - Im[ip] * uIm;
				tIm = Re[ip] * uIm + Im[ip] * uRe;
				Re[ip] = Re[i] - tRe;
				Im[ip] = Im[i] - tIm;
				Re[i] += tRe;
				Im[i] += tIm;
			}
			vRe = uRe * wRe - uIm * wIm;
			vIm = uRe * wIm + uIm * wRe;
			uRe = vRe;	uIm = vIm;
		}
	}
}
int frq_result(DB *table, int cnt)
{
	int bigfrq = 0, i=0;
	double temp_val = 0;

	for(i=1;i<cnt;i++)
	{
		if((table[i]-temp_val) >0)
		{
			temp_val = table[i];
			bigfrq = i;
		}
	}

	printf("freq:%d\n",bigfrq);

	return bigfrq;
}

double m_sqrt(double input)
{
	double x = 2;

	for(int i = 0; i<10; i++)
	{
		x = (x+(input/x))/2;
	}
	return x;
}

#if 0
void audio_Func(trans_info transInfo)
{
	
	system("rm -vf IN0.pcm");
	printf("\e[33m [%s] AUDIO SAMPLE START\e[0m\n", __FUNCTION__); 
	system("./sample_audio&");
	sleep(5);
	
	int i, n, p = 8;
	int ch;

	DB Re[MAX_CH][MAX], Im[MAX_CH][MAX], s;
	DB DFTR[MAX_CH][MAX], DFTI[MAX_CH][MAX];

	memset(&fft_src, 0x0, sizeof(fft_src));
	memset(&fft_tar, 0x0, sizeof(fft_tar));

	n = 1 << p;

	//******************** audio playback file read & floating point converting  & FFT *************/
	if(1)
	{
		FILE *fp;

		unsigned short buf[MAX_CH][512];
		unsigned short temp=0;

		int i, dummy;
		memset(&buf, 0x0, sizeof(buf));

		fp = fopen(src_file,"rb");

		// dummy read no recording section
		dummy = 1000;

		for(i=0;i<dummy;i++)
		{        
			fread(&temp,2,1,fp);
		}      

		for(i=0; i<256; i++)
		{
			for(ch=0;ch<1;ch++)
			{
				Im[ch][i] = 0;
				fread(&temp, 2, 1, fp);
				buf[ch][i] = temp;

				unsigned short tbuf;
				tbuf = buf[ch][i] >> 8;
				Re[ch][i] = sine(tbuf);
			}
		}
		fclose(fp);

		for(ch=0; ch<1; ch++)
		{
			FastFourierTransform(Re[ch], Im[ch], p);
			for (i=1; i<n; i++){
				s= sqrt(Re[ch][i]*Re[ch][i] + Im[ch][i]*Im[ch][i]);
				fft_src[ch][i-1] = s;
				//printf("fft_src[%d][%d] : 0x%x\n",ch,i,s);
			}
		}
	}
	//******************** audio recording file read & floating point converting  & FFT *************/
	if(1)
	{
		FILE *fp;

		unsigned short buf[MAX_CH][512];
		unsigned short temp=0;

		int i,dummy;
		memset(&buf, 0x0, sizeof(buf));

		fp = fopen(des_file,"rb");

		// dummy read no recording section
		dummy = 500;

		for(i=0;i<dummy;i++)
		{
			fread(&temp,2,1,fp);
		}

		for(i=0; i<256; i++)
		{
			for(ch=1; ch<2; ch++)
			{
				if(feof(fp))
				{
					printf("end of file!\n");
					break;
				}

				Im[ch][i] = 0;

				fread(&temp,2,1,fp);
				buf[ch][i] = temp;

				unsigned short tbuf;
				tbuf = buf[ch][i] >> 8;
				Re[ch][i] = sine(tbuf);
			}
		}

		fclose(fp);

		for(ch=1;ch<2;ch++)
		{
			FastFourierTransform(Re[ch], Im[ch], p);

			for ( i = 1; i < n; i++)
			{
				s= sqrt(Re[ch][i]*Re[ch][i] + Im[ch][i]*Im[ch][i]);
				fft_tar[ch][i-1] = s;
				//	printf("fft_src[%d][%d] : 0x%x\n",ch,i,s);
			}
		}
	}
	//********************  FFT compare *************/
	int audio_success = 0;
	if(1)
	{
		double compare[MAX_CH];
		int result[MAX_CH];
	
		for(ch=0;ch<1;ch++)
		{
			int ret_src,ret_tar;
			compare[ch] = 0;
			result[ch] = 0;

			ret_src = frq_result(&fft_src[0],128);
			ret_tar = frq_result(&fft_tar[1],128);

			if((ret_src-3) <= ret_tar && ret_tar <= (ret_src+3))
			{
				printf("channel %d is success!!\n",ch);
				result[ch] = 1;
			}
			else
			{
				result[ch] = 0;
			}
		}
		
		if(result[0] == 1)
		{
			m_audio_ret = 1;
		}
		else
		{
			m_audio_ret = 0;
		}

		//printf("%cAD%d%c\n",0x02,m_audio_ret,0x03);
		printf("\e[33m [%s] AUDIO SAMPLE END\e[0m\n", __FUNCTION__); 
		comMode(transInfo,PROTOCOL_PARAM,"AD",m_audio_ret);
	}
	
}
#else
#define QC_AUDIO_MAX_BUF 10000
#define QC_AUDIO_PI 3.14
#define QC_AUDIO_DB double

int audio_Func(int conv_inch)
{
	FILE *fpsrc=NULL, *fptar=NULL;
	unsigned short tsrc=0, ttar=0, tmpsrc=0, tmptar=0;
	unsigned int i=0;
	int ret_src = 0, ret_tar = 0;
	int p = 8;
	double Re[QC_AUDIO_MAX_BUF]={0,}, Te[QC_AUDIO_MAX_BUF]={0,}, Im[QC_AUDIO_MAX_BUF]={0,}, s=0, fft_src[QC_AUDIO_MAX_BUF + QC_AUDIO_MAX_BUF]={0,}, fft_tar[QC_AUDIO_MAX_BUF + QC_AUDIO_MAX_BUF]={0,};
	char aszFileName[128]={0,};

    fpsrc = fopen("/NFDVR/data/source_sine0.pcm","rb");
	sprintf(aszFileName, "/tmp/IN0_chn%d.pcm", conv_inch);
	fptar = fopen(aszFileName, "rb");

	if (NULL == fptar) {
		printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
		return 0;
	}

	printf("open stream file:\"%s\" for compare ok\n", aszFileName);

	for (i = 0; i < 32000; i++) {
		fread(&tsrc, 2, 1, fpsrc);
		fread(&ttar, 2, 1, fptar);
	}

	for (i = 0 ; i < 256; i++) {
		fread(&tsrc, 2, 1, fpsrc);
		fread(&ttar, 2, 1, fptar);
		tmpsrc = tsrc>>8;
		tmptar = ttar>>8;
		Re[i] = sine(tmpsrc);
		Te[i] = sine(tmptar);
	}

	fclose(fpsrc);
	fclose(fptar);

	FastFourierTransform(Re, Im, p);
	
	for (i = 0; i < 256; i++) {
		Im[i] = 0;
	}

	for (i = 1; i < 256; i++) {
		s= m_sqrt(Re[i] * Re[i] + Im[i] * Im[i]);
		fft_src[i - 1] = s;
	}

	FastFourierTransform(Te, Im, p);

	for (i = 0; i < 256; i++) {
		Im[i] = 0;
	}

	for (i = 1; i < 256; i++) {
		s = m_sqrt(Te[i] * Te[i] + Im[i] * Im[i]);
		fft_tar[i - 1] = s;
	}
#if 0
	for (i=1; i<256; i++){
		printf("fft src[%d] = %f, fft tar[%d] = %f\n",i, fft_src[i], i, fft_tar[i]);
	}
#endif

    ret_src = frq_result(fft_src, 128);
    ret_tar = frq_result(fft_tar, 128);

    if (ret_src == ret_tar) {
       return 1;
	} else {
       return 0;
	}
	printf("\e[33m [%s] AUDIO SAMPLE END\e[0m\n", __FUNCTION__); 

}
#endif

gboolean nf_qc_auto_test_audio(trans_info transInfo)
{
	return ;
}
/*===============================================
ALARM
===============================================*/
void alarm_error(void* arg)
{
	int flag = 0, alarm_in = 0;
	trans_info *transInfo = (trans_info *)arg;

	while(1)
	{
		if(m_alarm == 0)
		{
			if(flag == 0)
			{
				nf_dev_relay_ch_on(0);
				flag = 1;
			}
			else if(flag == 1)
			{
				nf_dev_relay_off(0);
				flag = 0;
			}
			sleep(1);
			alarm_in = nf_notify_get_param0("sensor");
			printf("\033[0;36m %s %d\033[0;39m\n", __FUNCTION__, alarm_in);
			if(alarm_in == 0x1)
				comMode(*transInfo, WRITE, "00:  0000001" ,0);
			else
				comMode(*transInfo, WRITE, "00:  0000000" ,0);
		}
		else if(m_alarm == 1)
		{
			printf("\e[33m [%s] alarm error thread end!\e[0m\n", __FUNCTION__); 
			break;
		}
	}
}
/*===============================================
RS485
===============================================*/
void rs485_error(void* arg)
{
	int flag = 0;
	trans_info *transInfo = (trans_info *)arg;
	NF_PTZ_CMD	ptz_cmd;
	ptz_cmd.ch=0;
	ptz_cmd.cmd=NF_PTZ_CMD_GOTO_PRESET;
	while(1)
	{
		if(m_rs485 == 0)
		{
			printf("\033[0;35m %s >>>>> PTZ <<<<< \033[0;39m\n", __FUNCTION__);
			nf_ptz_cmd(&ptz_cmd);
			sleep(1);
		}
		else if(m_rs485 == 1)
		{
			printf("rs485 error thread end!\n");
			break;
		}
	}
}

/*===============================================
NETWORK
===============================================*/
void ping_Func(trans_info transInfo)
{
	comMode(transInfo,PROTOCOL,"NW",0);
	unsigned char input_char[256];
	unsigned char test_end_cmd[8] = "PING_END";
	unsigned char ping_cmd[100];

	while(1)
	{
		if(comMode(transInfo,READ,input_char,0) > 0)
		{
			printf("input_char = %s\n",input_char);

			if(strncmp(input_char,test_end_cmd,sizeof(test_end_cmd)) != 0)
			{ 
				
				sprintf(ping_cmd,"ping -w 5 %s",input_char);
				himmSend(transInfo,ping_cmd);
			}
			else if(strncmp(input_char,test_end_cmd,sizeof(test_end_cmd)) == 0)
			{
				break;
			}
		}
	}
}
/*===============================================
SDCARD
===============================================*/
void sdcard_Func(trans_info transInfo)
{
	char arrCmd[100]={0,};
	char arrReadBuf[1024]={0,};
	int fd = NULL;
	int len = 0;
	struct   stat  file_info;
	comMode(transInfo,PROTOCOL,"CW",0);
	system("cd /");
	//sprintf(arrCmd,"%s > sdcard.txt","hdparm -tT /dev/mmcblk1"); //emmc
	sprintf(arrCmd,"%s > sdcard.txt","hdparm -tT /dev/mmcblk0");
	system(arrCmd);
	if((fd = open("./sdcard.txt",O_RDONLY)) < 0)
	{
		printf("\e[33m  Open failed sdcard.txt\e[0m\n");
		sprintf(arrReadBuf,"hdparm: can't open '/dev/mmcblk1': No such file or directory");
	}
	else
	{
		len = read(fd,arrReadBuf,1024-1);
		if(len == 0)
			sprintf(arrReadBuf,"hdparm: can't open '/dev/mmcblk1': No such file or directory");
		else
			arrReadBuf[len] = '\0';
	}
	comMode(transInfo, WRITE, arrReadBuf ,0);
	close(fd);	
}
/*===============================================
LED
===============================================*/
#if 0
void led_error(void *arg)
{
	#if 1
	trans_info *transInfo = (trans_info *)arg;
	while(1)
	{	
		if(m_led == 0)
		{
			nf_dev_board_pp_cntl_led(0, 1);
			nf_dev_board_pp_cntl_led(1, 0);
			sleep(1);
			nf_dev_board_pp_cntl_led(0, 0);
			nf_dev_board_pp_cntl_led(1, 1);
			sleep(1);
			nf_dev_board_pp_cntl_led(0, 0);
			nf_dev_board_pp_cntl_led(1, 0);
			sleep(1);
		}

		else if(m_led == 1)
		{
			printf("led error thread end!\n");
			nf_dev_board_pp_cntl_led(0, 0);
			nf_dev_board_pp_cntl_led(1, 0);
			break;
		}

	} 
	#else
	while(1)
	{

		if(m_led == 0)
		{
			nf_dev_rear_green_led_blink(0);
			nf_dev_rear_red_led_blink(0);
		}

		else if(m_led == 1)
		{
			printf("led error thread end!\n");
			nf_dev_rear_led_off();
			break;
		}

	} 
	#endif

}
#endif
/*===============================================
FACTORY KEY
===============================================*/
void key_error(void* arg)
{
	trans_info *transInfo = (trans_info*)arg;
	while(1)
	{
		if(m_key == 0)
		{
			int  key_val = 0;
			
			key_val = nf_notify_get_param0("key");
			
			sleep(1);
			
			if(key_val== 0x0)
				comMode(*transInfo, WRITE, "0000:  00000001" ,0);
			else
				comMode(*transInfo, WRITE, "0000:  00000000" ,0);
		}
		else if(m_key == 1)
		{
			printf("key error thread end!\n");
			m_key++;
			break;
		}
	}
}
#if 0
/*===============================================
DAY&NIGHT
===============================================*/
void daynight_error(void)
{
	int flag = 0;
	int day_night_mode = 0;
	while(1)
	{
		sleep(1);
		if(m_daynight == 0)
		{
			if(flag == 0)
			{
				day_night_mode = NF_CAM_DAYNIGHT_DAY;
				flag = 1;
			}
			else if(flag == 1)
			{	
				day_night_mode = NF_CAM_DAYNIGHT_NIGHT;
				flag = 0;
			} 
			nf_cam_set_camimgs(NF_CAM_SET_DAYNIGHT_MODE, &day_night_mode, NULL, NULL, NULL);
		}
		else if(m_daynight == 1)
		{
			printf("daynight thread end!\n");
			day_night_mode = NF_CAM_DAYNIGHT_DAY;
			nf_cam_set_camimgs(NF_CAM_SET_DAYNIGHT_MODE, &day_night_mode, NULL, NULL, NULL);
			break;
		}
	}
}
/*===============================================
IRIS
===============================================*/
void iris_error(void *arg)
{
	int i;
	trans_info *transInfo = (trans_info *)arg;
	unsigned char input_char[256];
	NF_CAM_MFZ_PARAM mfz_param;
	while(1)
	{
	
		memset(&mfz_param, 0x00, sizeof(NF_CAM_MFZ_PARAM));
		float iris_pos = 0;
		comMode(*transInfo,READ,input_char,0);
		if(strncmp(input_char,"IRIS_ON",sizeof("IRIS_ON")) == 0)
		{
			mfz_param.af_cmd = CMD_PIRIS;
			mfz_param.af_value = (int)nf_caps_get_uint( nf_caps_get_root_obj() , "lens.iris.max", NULL );
			printf("\e[31m ##########Relative IRIS[%d] ########### \e[0m\n",mfz_param.af_value);
			nf_cam_mfz_ctrl_cmd(&mfz_param);
			comMode(*transInfo,PROTOCOL,"EM",0);
		}
		else if(strncmp(input_char,"IRIS_OFF",sizeof("IRIS_OFF")) == 0)
		{
			mfz_param.af_cmd = CMD_PIRIS;
			mfz_param.af_value =0;
			printf("\e[31m ##########Relative IRIS[%d] ########### \e[0m\n",mfz_param.af_value);
			nf_cam_mfz_ctrl_cmd(&mfz_param);
			comMode(*transInfo,PROTOCOL,"EM",0);
		}
		else if(strncmp(input_char,"IRIS_END",sizeof("IRIS_END")) == 0)
		{
			mfz_param.af_cmd = CMD_PIRIS;
			mfz_param.af_value = (int)nf_caps_get_uint( nf_caps_get_root_obj() , "lens.iris.max", NULL );
			printf("\e[31m ##########Relative IRIS[%d] ########### \e[0m\n",mfz_param.af_value);
			nf_cam_mfz_ctrl_cmd(&mfz_param);
			comMode(*transInfo,PROTOCOL,"EM",0);
			printf("iris thread end!\n");
			m_iris = 1;
			break;
		}

	} 


}
/*===============================================
ZOOM/FOCUS
===============================================*/
void zoom_in(void)
{
	NF_CAM_MFZ_PARAM mfz_param;
	memset(&mfz_param, 0x00, sizeof(NF_CAM_MFZ_PARAM));

	int pos = 0, curPos = 0;
	mfz_param.af_cmd = CMD_ZOOM;
	mfz_param.af_value = (float)nf_caps_get_uint(nf_caps_get_root_obj(),"lens.zoom.max",NULL);
	curPos=nf_sysdb_get_uint("cam.C0.zoom");
	printf("\e[31m ##########Relative Zoom[%d] curPos[%d] ########### \e[0m\n",mfz_param.af_value,curPos);
	nf_cam_mfz_ctrl_cmd(&mfz_param);
	#if 0
	while(1)
	{
		pos=nf_sysdb_get_uint("cam.C0.zoom");
		printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
		if((pos != curPos) || (pos == mfz_param.af_value))
			break;
	}
	#endif
	while(1)
	{
		if(nf_cam_get_mfz_state() == 1)
			break;
	}
}
void zoom_out(void)
{
	NF_CAM_MFZ_PARAM mfz_param;
	memset(&mfz_param, 0x00, sizeof(NF_CAM_MFZ_PARAM));
	int pos = 0, curPos = 0;
	mfz_param.af_cmd = CMD_ZOOM;
	mfz_param.af_value = 0;
	curPos=nf_sysdb_get_uint("cam.C0.zoom");
	printf("\e[31m ##########Relative Zoom[%d] curPos[%d]########### \e[0m\n",mfz_param.af_value,curPos);
	nf_cam_mfz_ctrl_cmd(&mfz_param);
	#if 0
	while(1)
	{
		pos=nf_sysdb_get_uint("cam.C0.zoom");
		printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
		if((pos != curPos) || (pos == mfz_param.af_value))
			break;
	}
	#endif
	while(1)
	{
		if(nf_cam_get_mfz_state() == 1)
			break;
	}
}
void focus_in(void)
{

	NF_CAM_MFZ_PARAM mfz_param;
	int pos = 0, curPos = 0;
	memset(&mfz_param, 0x00, sizeof(NF_CAM_MFZ_PARAM));
	mfz_param.af_cmd = CMD_FOCUS;
	mfz_param.af_value = (int)(nf_caps_get_uint(nf_caps_get_root_obj(),"lens.focus.max",NULL));
	curPos = nf_sysdb_get_uint("cam.C0.focus");
	printf("\e[31m ############ FOCUS Position[%d] curPos[%d] ############# \e[0m\n", mfz_param.af_value,curPos);
	nf_cam_mfz_ctrl_cmd(&mfz_param);
	#if 0
	while(1)
	{
		pos = nf_sysdb_get_uint("cam.C0.focus");
		if((pos != curPos) || (pos == mfz_param.af_value))
		{
			printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
			break;
		}
		printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
	}
	#endif
	while(1)
	{
		if(nf_cam_get_mfz_state() == 1)
			break;
	}
}
void focus_out(void)
{

	NF_CAM_MFZ_PARAM mfz_param;
	memset(&mfz_param, 0x00, sizeof(NF_CAM_MFZ_PARAM));
	int cnt = 0;
	int pos = 0,curPos = 0;
	mfz_param.af_cmd = CMD_FOCUS;
	mfz_param.af_value = (int)0;
	curPos = nf_sysdb_get_uint("cam.C0.focus");
	printf("\e[31m ############ FOCUS Position[%d] curPos[%d] ############# \e[0m\n", mfz_param.af_value,curPos);
	nf_cam_mfz_ctrl_cmd(&mfz_param);
	#if 0
	while(1)
	{
		pos = nf_sysdb_get_uint("cam.C0.focus");
		if((pos != curPos) || (pos == mfz_param.af_value))
		{
			printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
			break;
		}
		printf("\033[0;33m %s pos[%d]\033[0;39m\n", __FUNCTION__,pos);
	}
	#endif
	while(1)
	{
		if(nf_cam_get_mfz_state() == 1)
			break;
	}
}

void zoom_focus(void *arg)
{
	unsigned char input_char[256];
	trans_info *transInfo = (trans_info*)arg;
	while(1)
	{
		comMode(*transInfo,READ,input_char,0);
		if(strncmp(input_char,"ZOOM_IN",sizeof("ZOOM_IN")) == 0)
			m_zoomfocus = 1;
		else if(strncmp(input_char,"ZOOM_OUT",sizeof("ZOOM_OUT")) == 0)
			m_zoomfocus = 2;
		else if(strncmp(input_char,"FOCUS_IN",sizeof("FOCUS_IN")) == 0)
			m_zoomfocus = 3;
		else if(strncmp(input_char,"FOCUS_OUT",sizeof("FOCUS_OUT")) == 0)
			m_zoomfocus = 4;
		else if(strncmp(input_char,"ZOOM_END",sizeof("ZOOM_END")) == 0)
			m_zoomfocus = 5;

		if(m_zoomfocus == 1)
		{	
			zoom_in();
			comMode(*transInfo,PROTOCOL,"EN",0);
		}
		if(m_zoomfocus == 2)
		{
			zoom_out();
			comMode(*transInfo,PROTOCOL,"EN",0);
		}
		if(m_zoomfocus == 3)
		{
			focus_in();
			comMode(*transInfo,PROTOCOL,"EN",0);
		}
		if(m_zoomfocus == 4)
		{
			focus_out();
			comMode(*transInfo,PROTOCOL,"EN",0);
		}
		if(m_zoomfocus == 5)
		{
			m_zoomfocus = 6;
			printf("ZOOM/FOCUS AUTO END!!\n");
			break;
		}
	}
}
#endif

void test_num(trans_info transInfo, int cmd)
{
	int ret;
	unsigned char input_char[256];
	int a = 1,b = 2,c = 3;
	int status;
	switch(cmd)
	{
		case QC_TEST_ALARM:
			//alarm
			comMode(transInfo,PROTOCOL,"AW",0);
			pthread_t alarm_thread;
			int thr_id;

			thr_id = pthread_create(&alarm_thread, NULL, (void *)alarm_error, (void *)&transInfo);

			if(thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			printf("alarm thead initialized.......\n");
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"ALARM_END",sizeof("ALARM_END")) == 0)
				{
					m_alarm = 1;
					break;
				}
			}
			//printf("\e[33m [%s] alarm error thread 1\e[0m\n", __FUNCTION__); 
			pthread_join(alarm_thread, (void *)&status);
			//printf("\e[33m [%s] alarm error thread 2\e[0m\n", __FUNCTION__); 
			break;
		case QC_TEST_AUDIO:
			nf_sysdb_set_bool("audio.enable",1);
			nf_sysdb_set_uint("audio.in_volume",100);
			nf_sysdb_set_uint("audio.out_volume",100);
			nf_sysdb_save_all();
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);
			sleep(1);
			
			nf_qc_auto_test_audio(transInfo);

			nf_sysdb_set_uint("audio.in_volume",50);
			nf_sysdb_set_uint("audio.out_volume",50);
			nf_sysdb_set_bool("audio.enable",0);
			nf_sysdb_save_all();
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);
			break;
		case QC_TEST_NET:
			ping_Func(transInfo);
			break;
		case QC_TEST_SDCARD:
			sdcard_Func(transInfo);
			break;
		#if 0
		case QC_TEST_LED:
			//Led			
			comMode(transInfo,PROTOCOL,"LW",0);
			#if 1
			pthread_t led_thread;
			int blk_thr_id;
			blk_thr_id = pthread_create(&led_thread, NULL, (void *)led_error, (void *)&transInfo);

			if(blk_thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			comMode(transInfo,WRITE,"led blink thread initialized.......\n",0);
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"LED_END",sizeof("LED_END")) == 0)
				{
					m_led = 1;
					break;
				}
			}
			pthread_join(led_thread, (void *)&status);
			#endif
			break;
		#endif
		case QC_TEST_MIC:
			/*TEST MIC*/
			
			nf_sysdb_set_bool("audio.enable",1);
			nf_sysdb_set_uint("audio.in_volume",100);
			nf_sysdb_set_uint("audio.out_volume",100);
			nf_sysdb_save_all();
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);
			
			#if 0		// Blocked By pakkhman
				nf_HI_aud_init();
			#endif
			comMode(transInfo,PROTOCOL,"PW",0);
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"MIC_END",sizeof("MIC_END")) == 0)
				{
					nf_sysdb_set_uint("audio.in_volume",50);
					nf_sysdb_set_uint("audio.out_volume",50);
					nf_sysdb_set_bool("audio.enable",0);
					nf_sysdb_save_all();
					nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);
					break;
				}
			}
			break;
		case QC_TEST_RS485 : //35:
			printf("rs485 thead initialized.......\n");
			comMode(transInfo,PROTOCOL,"RW",0);
			int thr_rs485_id;
			pthread_t rs485_thread;
			thr_rs485_id = pthread_create(&rs485_thread, NULL, (void *)rs485_error, (void *)&transInfo);
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"RS485_END",sizeof("RS485_END")) == 0)
				{
					m_rs485 = 1;
					break;
				}
			}
			pthread_join(rs485_thread, (void *)&status);
			break;
		case QC_TEST_FAC_KEY : //36:
			//factory key
			comMode(transInfo,PROTOCOL,"KW",0);
			pthread_t key_thread;

			thr_id = pthread_create(&key_thread, NULL, (void *)key_error, (void *)&transInfo);
			if(thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			comMode(transInfo,WRITE,"key thead initialized.......\n",0);
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"KEY_END",sizeof("KEY_END")) == 0)
				{
					m_key = 1;
					break;
				}
			}
			pthread_join(key_thread, (void *)&status);
			break;
		case QC_TEST_CVBS ://37:	//cvbs
			comMode(transInfo,PROTOCOL,"VW",0);
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"ANALOG_END",sizeof("ANALOG_END")) == 0) {
					break;
				}
			}
			break;
#if 0
		case QC_TEST_IRIS : //38
			//iris
			if (!nf_sysdb_set_uint("cam.C0.iris_control", 1))
				printf("\e[33m [%s] IRIS AUTO CHANGE ERROR\e[0m\n", __FUNCTION__);
			comMode(transInfo,PROTOCOL,"IW",0);
			pthread_t iris_thread;
			
			thr_id = pthread_create(&iris_thread, NULL, (void *)iris_error, (void *)&transInfo);
			
			if(thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			
			printf("iris thread initialized.......\n");
			
			while(1)
			{
				if(m_iris == 1)
				{
					if (!nf_sysdb_set_uint("cam.C0.iris_control", 0))
						printf("\e[33m [%s] IRIS AUTO CHANGE ERROR\e[0m\n", __FUNCTION__);
					break;
				}
			}
			break;
		case QC_TEST_DAY_NIGHT : //39
			comMode(transInfo,PROTOCOL,"YW",0);
			pthread_t daynight_thread;
			thr_id = pthread_create(&daynight_thread, NULL, (void *)daynight_error, (void *)&a);
			if(thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"DAYNIGHT_END",sizeof("DAYNIGHT_END")) == 0)
				{
					m_daynight = 1;
					break;
				}
			}
			break;
		case QC_TEST_ZOOM_FOCUS : //40
			comMode(transInfo,PROTOCOL,"ZW",0);
			pthread_t zoomfocus_thread;
		
			thr_id = pthread_create(&zoomfocus_thread,NULL,(void *)zoom_focus, (void *)&transInfo);

			if(thr_id < 0)
			{
				perror("thread create error : ");
				exit(0);
			}
			printf("zoom & focus thread initialized.......\n");
			while(1)
			{
				if(m_zoomfocus == 6)
					break;
			}
			break;
		case QC_TEST_IR: // 41
			// IR TEST
			comMode(transInfo,PROTOCOL,"SW",0);
			int flag_ir_led = 0;
			while(1)
			{
				comMode(transInfo,READ,input_char,0);
				if(strncmp(input_char,"IR_END",sizeof("IR_END")) == 0)
				{
					m_ir = 1;
					break;
				}
			}
			break;
#endif
		case QC_TEST_END:
			comMode(transInfo,PROTOCOL,"ED",0);
			break;
	}
}

int qc_auto_setrtc_test(u_char *rtc_data, char *str)
{
	int r_year, r_month, r_day, r_hour, r_min, r_sec;
	struct tm rev_stime;
	time_t rev_time=0, cur_time=0;
	char temp_str[100];
	
	rtc_data[3]&=0x7F;  //
	r_year  = BCD_BIN(rtc_data[0]);
	r_month = BCD_BIN(rtc_data[1]);
	r_day   = BCD_BIN(rtc_data[2]);
	r_hour  = BCD_BIN(rtc_data[3]);
	r_min   = BCD_BIN(rtc_data[4]);
	r_sec   = BCD_BIN(rtc_data[5]);

	rev_stime.tm_year  = r_year+2000-1900;     // start year 1900
	rev_stime.tm_mon   = r_month;
	rev_stime.tm_mday  = r_day;
	rev_stime.tm_hour  = r_hour;
	rev_stime.tm_min   = r_min;
	rev_stime.tm_sec   = r_sec;

	rev_time = mktime(&rev_stime);
	sprintf(temp_str,"date -s '%04d-%02d-%02d %02d:%02d:%02d'",r_year+2000,r_month,r_day,r_hour,r_min,r_sec);
	printf("====-=-=-=> %s\n",temp_str);
	system(temp_str);
#if 1
	strcpy(temp_str,"hwclock -w");
	system(temp_str);
	strcpy(temp_str,"hwclock --systohc");
	system(temp_str);
#else
	// strcpy(temp_str,"hwclock_qc --systohc --fast");
	system("/sbin/hwclock_qc.dat --systohc --fast");
#endif
	cur_time = cur_time_return(str);
	printf("\033[0;34m %s cur_time[%d] rev_time[%d]\033[0;39m\n", __FUNCTION__,cur_time,rev_time);
	if((cur_time-rev_time)<3) return 1;
	else return 0;
}

/*QC MODE�� ���� �Ұ��� ����*/

#define SERVERIP "192.168.50.1"
#define SERVERIP_NFS "192.168.101.175"

static void macConvert(char *MAC, char *src)
{
	int i = 0;
	int j = 0;
	for(i = 0; i < 17; i++)
	{
		if(i%3 == 2)
		{
			MAC[i] = ':';
			continue;
		}
		MAC[i] = src[j];
		j++;
	}
	//printf("\e[33m [%s] MAC : %s\e[0m\n", __FUNCTION__,MAC);
}
int enter_qc(trans_info transInfo)
{
	int cmd_num[100] = {0,};
	int total_cmd_num = 0;
	int ii = 0;
	int ret = 0;

	unsigned char input_char[256];
	unsigned char start_cmd[8] = "QC_START";
	unsigned char qc_cmd_load_end[8] = "CMDLSTED";
	unsigned char temp[32] = {0,};
	char str[64];
	QC_LOADSET_INFO ind_board_info;
	
	char sendBuf[MAXBUF] = {0,};
	char recvBuf[MAXBUF] = {0,};
	
	guint tz_idx = 0;
	guint pre_tz_idx = 0;
	char tz_name[128]={0,};
	//*SETTING GMT + 0*//
	pre_tz_idx = nf_sysdb_get_uint("sys.date.tz_index");
	printf("\033[0;34m %s pre_tz_idx[%d] \033[0;39m\n", __FUNCTION__,pre_tz_idx);
	for(tz_idx = 0; tz_idx < nf_zoneinfo_get_count(); tz_idx++)
	{
		if(strncmp(nf_zoneinfo_get_string(tz_idx),"GMT+00:00",strlen("GMT+00:00")) == 0)
		{
			nf_sysdb_set_uint("sys.date.tz_index",tz_idx);
			nf_zoneinfo_set(tz_idx);
			nf_sysdb_save_all();
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);
			nf_notify_fire_params("sysdb_tzone", 0, 0, 0, 0);
			printf("\033[0;34m %s [%d]%s \033[0;39m\n", __FUNCTION__,tz_idx,nf_zoneinfo_get_string(tz_idx));
		}
	}

	comMode(transInfo,PROTOCOL,"WT",0);
	while(1)
	{
		comMode(transInfo,READ,input_char,0);
		if(strncmp(input_char,"INX_QC",sizeof("INX_QC")) == 0)
		{
			m_select = 0;
			vio_select = 1;
			break;
		}
		else if(strncmp(input_char,"INB3H_QC",sizeof("INB3H_QC")) == 0)
		{
			m_select = 1;
			vio_select = 2;
			break;
		}
		else if(strncmp(input_char,"INBH_QC",sizeof("INBH_QC")) == 0)
		{
			m_select = 1;
			vio_select = 1;
			break;
		}
		else if(strncmp(input_char,"NMD_QC",sizeof("NMD_QC")) == 0)
		{
			m_select = 0;
			vio_select = 1;
			break;
		}
		else if(strncmp(input_char,"NSB_TV_QC",sizeof("NSB_TV_QC")) == 0)
		{
			m_select = 0;
			vio_select = 1;
			break;
		}
		else if(strncmp(input_char,"HI3519V101_QC",sizeof("HI3519V101_QC")) == 0) //Only fisheye, 4k
		{
			break;
		}
		else if(strncmp(input_char,"HI3519A100_QC",sizeof("HI3519A100_QC")) == 0) //Only fisheye, 4k
		{
			break;
		}
		else if(strncmp(input_char,"HI3516CV500_QC",sizeof("HI3516CV500_QC")) == 0) //Only fisheye, 4k
		{
			break;
		}
	}
	sleep(4);
	comMode(transInfo,READ,temp,0);	
	if(strncmp(temp,"RTCOK",sizeof("RTCOK")) == 0)
	{
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[0] = BIN_BCD(atoi(temp));
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[1] = BIN_BCD(atoi(temp));
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[2] = BIN_BCD(atoi(temp));
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[3] = BIN_BCD(atoi(temp));
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[4] = BIN_BCD(atoi(temp));
		comMode(transInfo,READ,temp,0);
		ind_board_info.rtc_info[5] = BIN_BCD(atoi(temp));
		printf("\e[33m [%s] rtc_info write\e[0m\n", __FUNCTION__); 
		ret = qc_auto_setrtc_test(ind_board_info.rtc_info,str);
		
		//*SETTING default GMT*//
		nf_sysdb_set_uint("sys.date.tz_index",pre_tz_idx);
		nf_zoneinfo_set(pre_tz_idx);
		nf_sysdb_save_all();
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);
		nf_notify_fire_params("sysdb_tzone", 0, 0, 0, 0);
		comMode(transInfo,PROTOCOL_PARAM,"TI",ret);
	}

	if(transInfo.mode == NET)
	{
		comMode(transInfo,READ,input_char,0);
		//printf("\e[33m [%s] input_char = %s\e[0m\n", __FUNCTION__,input_char); 
		if(strncmp(input_char,start_cmd,sizeof(start_cmd)) == 0)
		{
			comMode(transInfo,PROTOCOL,"ST",0);
			printf("Recieve START COMMAND from PC\n");
		}
	}
	comMode(transInfo,PROTOCOL,"QL",0);
	while(1)
	{	
		if(comMode(transInfo,READ,input_char,0) > 0)
		{
			if(strncmp(input_char,qc_cmd_load_end,sizeof(qc_cmd_load_end)) == 0)
			{
				total_cmd_num = ii;
				printf("total_cmd_num = %d\n",total_cmd_num);				
				break;
			}
			else
			{
				cmd_num[ii] = atoi(input_char);
				printf("test_num = %d\n",cmd_num[ii]);
				ii++;
			}
		}		
	}

	for(ii=0; ii < total_cmd_num; ii++)
		test_num(transInfo,cmd_num[ii]);
	return 1;
}
void close_qc(trans_info transInfo)
{
	if((transInfo.sock > 0) && (transInfo.mode == NET))
		close(transInfo.sock);
}
static void  nf_qc_thread_func(void)
{
	int macFlag=0,licFlag=0, reFlag=1;
	char sendBuf[MAXBUF]={0,};
	char input_char[256]={0,};
	char ip[17]={0, };
	char mac[17]={0, };
	while(1)
	{
		int ret=0;
		#if 0
			//printf("\e[33m [%s] QC START\e[0m\n", __FUNCTION__);
			if(!nf_sysman_is_qcmode(NULL))
			{
				printf("\e[33m [%s] It isn't qc mode\e[0m\n", __FUNCTION__);
				break;
			}
			else
				printf("\e[33m [%s] It is qc mode\e[0m\n", __FUNCTION__);
		#endif
		initQC();
		trans_info transInfo={NET,0};
		struct sockaddr_in serv_addr;
		
		if(nf_sysman_hotkey_is_nfs())
		{
			printf("\e[33m [%s] nfs mode\e[0m\n", __FUNCTION__);
			strncpy(ip,SERVERIP_NFS,strlen(SERVERIP_NFS));
		}
		else
			strncpy(ip,SERVERIP,strlen(SERVERIP));
			
		if(transInfo.mode == NET)
		{
			transInfo.sock = socket(AF_INET, SOCK_STREAM, 0);
			if (transInfo.sock == -1) {
			 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
		}
			memset(&serv_addr, 0, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = inet_addr(ip);
			serv_addr.sin_port = htons(PORT);
			while((ret = connect(transInfo.sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
				printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__); 
				sleep(1);
			}
		}
		comMode(transInfo,PROTOCOL,"MX",0);
		macConvert(mac,nf_sysdb_get_str_nocopy("sys.info.mac"));
		sprintf(sendBuf,"[%-32.32s",mac);
		comMode(transInfo,PROTOCOL,sendBuf,0);
		
		comMode(transInfo,PROTOCOL,"VX",0);
		sprintf(sendBuf,"|%-32.32s", nf_api_param_hw_get_vendor());
		comMode(transInfo,PROTOCOL,sendBuf,0);
		
		sprintf(sendBuf,"FV%s;",nf_sysdb_get_str_nocopy("sys.info.swver"));
		comMode(transInfo,PROTOCOL,sendBuf,0);
		
		ret = 0;
		while(1)
		{
			#if 1
			if(reFlag == 1)
			{
				initQC();
				enter_qc(transInfo);
			}
			reFlag = 0;
			if(comMode(transInfo,READ,input_char,0) > 0)
			{
				if(strncmp(input_char,"RESTARTQC",strlen("RESTARTQC")) == 0)
				{
					reFlag = 1;
					continue;
				}
				else if(strncmp(input_char,"MACWRITER",strlen("MACWRITER")) == 0)
				{
					#if 0
					memset(mac,0x00, sizeof(mac));
					if((ret = nf_sysman_qc_pc_client(ip,mac)) == NF_SYSMAN_QC_PD_ERROR_OK)
					{
						comMode(transInfo,PROTOCOL,"PO",0);
						comMode(transInfo,PROTOCOL,"MX",0);
						sprintf(sendBuf,"[%-32.32s",mac);
						comMode(transInfo,PROTOCOL,sendBuf,0);
						macFlag = 1;
					}
					else
						comMode(transInfo,PROTOCOL,"PX",0);
					#endif
				}
				else if(strncmp(input_char,"LICWRITER",strlen("LICWRITER")) == 0)
				{
					int licCnt = 0;
					#if 0
						ret = nf_sysman_qc_lic_write(ip,&licCnt,NULL);
					#endif
					#if 1
					switch(ret)
					{
						case NF_SYSMAN_QC_PD_ERROR_OK:
							comMode(transInfo,PROTOCOL_PARAM,"LO",licCnt);
							printf("\e[33m [%s] LO%d \e[0m\n", __FUNCTION__,licCnt); 
							licFlag = 1;
							break;
						#if 0
						case NF_SYSMAN_QC_PD_ERROR_NO_LIC_INFO:
							comMode(transInfo,PROTOCOL_PARAM,"LO",0);
							printf("\e[33m [%s] LO0 \e[0m\n", __FUNCTION__); 
							licFlag = 1;
							break;
						#endif
						case NF_SYSMAN_QC_PD_ERROR_NETWORK:
							comMode(transInfo,PROTOCOL_PARAM,"LX",1);
							printf("\e[33m [%s] LX1 \e[0m\n", __FUNCTION__); 
							break;
						case NF_SYSMAN_QC_PD_ERROR_WRONG_DATA:
							comMode(transInfo,PROTOCOL_PARAM,"LX",2);
							printf("\e[33m [%s] LX2 \e[0m\n", __FUNCTION__); ;
							break;
						case NF_SYSMAN_QC_PD_ERROR_SET_DB:
							comMode(transInfo,PROTOCOL_PARAM,"LX",3);
							printf("\e[33m [%s] LX3 \e[0m\n", __FUNCTION__); 
							break;
					}
					#else
					comMode(transInfo,PROTOCOL_PARAM,"LO",16);
					#endif
				}
				if(licFlag == 1 && macFlag == 1)
					break;	
			}
			#else
			if(comMode(transInfo,READ,input_char,0) > 0)
			{
				if(strncmp(input_char,"LICWRITER",strlen("LICWRITER")) == 0)
				{
					int licCnt = 0;
					nf_sysman_qc_lic_write(ip,&licCnt,NULL);
					comMode(transInfo,PROTOCOL_PARAM,"LO",licCnt);
				}		
			}
			#endif
		}
		close_qc(transInfo);
		break;
	}
}
void nf_qc_init()
{
	GThread *thread_id = 0;

	thread_id = g_thread_create((GThreadFunc)nf_qc_thread_func, NULL, FALSE, NULL);
	if (!thread_id)
	{
		printf("[%s][%d] Error - Failed to create thread[nf_issm_ctl_thread_func]!\n",
				__FUNCTION__, __LINE__);
		return;
	}
}
