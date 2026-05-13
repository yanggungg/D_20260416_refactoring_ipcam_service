/****************************************************************************
*  (c) COPYRIGHT 2009  ITXSecurity                                          *
*                                                                           *
*  ALL RIGHT RESERVED                                                       *
*                                                                           *
*  ITXSecurity Systemsoftware2 team                                         *
*                                                                           *
* ***************************************************************************

  REVISION HISTORY:

  Date        Version  Name            Description
  ___________ _______  _______________ _____________________________________
  04/01/2009  0.1      Hosik Yoon      Created

  ..........................................................................

  DESCRIPTION:

  ..........................................................................
*/

#ifndef	_SPOT_ANF_IOCTL_H
#define	_SPOT_ANF_IOCTL_H

#define		SPOT_ANF_IOCTL_MAGIC		'S'
#define		SPOT_ANF_IOCTL_MAXNR		(32)	//총 ioctl 명령어의 수를 써주면 됨

struct spot_anf_spot_status {
	unsigned int	spot_num;			//0:1번 포트, 1:2번 포트, 2:3번 포트, 3:4번 포트
	unsigned int	seq_cnt;			//sequence 화면의 갯 수
	unsigned char	change_time[16];	//각 시퀀서 화면의 full, quad 설정 화면 변경 시간(sec)
	unsigned char 	channel[16];		//각 시퀀서 화면의 display 될 화면 설정  채널 7번 비트는 covert를 나타내고 하위 4비트는 채널 번호를 나타냄
	unsigned char	enable_f;			//0 : spot disable, 1 : spot enable
};

#define		SPOT_ANF_SET_SEQUENCE			_IOW(SPOT_ANF_IOCTL_MAGIC, 3, struct spot_anf_spot_status)	//스팟의 화면 설정
//#define		AM8816_SPOT_SET_BORDER_COLOR	_IOW(AM8816_IOCTL_MAGIC, 5, unsigned int)			//이거는 스팟 4개 출력이 공통의 색으로 설정되어야 함
//#define		AM8816_SPOT_CAMERA_TITLE_ON		_IO( AM8816_IOCTL_MAGIC, 6 )
//#define		AM8816_SPOT_CAMERA_TITLE_OFF	_IO( AM8816_IOCTL_MAGIC, 7 )
//#define		AM8816_SPOT_DATE_ON				_IO( AM8816_IOCTL_MAGIC, 8 )
//#define		AM8816_SPOT_DATE_OFF			_IO( AM8816_IOCTL_MAGIC, 9 )
//#define		AM8816_SET_DATE_FORMAT			_IOW(AM8816_IOCTL_MAGIC, 10, unsigned char)			//0xAB	A : date format (0:YYYY/MM/DD,1:MM/DD/YYYY,2:DD/MM/YYYY)
//																								//		B : time format (0:0~24, 1:AM,PM)
//#define		AM8816_SPOT_SET_BORDER_SIZE		_IOW(AM8816_IOCTL_MAGIC, 12, struct am_border_size_st)	//border size
//#define		AM8816_SET_NTSC					_IO( AM8816_IOCTL_MAGIC, 13 )
//#define		AM8816_SET_PAL					_IO( AM8816_IOCTL_MAGIC, 14 )
//#define		AM8816_GET_SIGTYPE				_IOR(AM8816_IOCTL_MAGIC, 15, unsigned int)
//#define		AM8816_SET_TITLE_NAME			_IOW(AM8816_IOCTL_MAGIC, 16, struct am_title_name)
#define		SPOT_ANF_GET_VIDEO_LOSS_CH		_IOR(SPOT_ANF_IOCTL_MAGIC, 17, unsigned short)
#define		SPOT_ANF_SET_VIDEO_LOSS_CH		_IOW(SPOT_ANF_IOCTL_MAGIC, 18, unsigned short)
//#define		AM8816_GET_GMT_OFFSET			_IOR(AM8816_IOCTL_MAGIC, 19, int)
//#define		AM8816_SET_GMT_OFFSET			_IOW(AM8816_IOCTL_MAGIC, 20, int)
//
//#define		AM8816_REG_DUMP					_IO( AM8816_IOCTL_MAGIC, 0 )		//디버그용 레지스터 출력
//#define		AM8816_WRITE_REG				_IOW(AM8816_IOCTL_MAGIC, 1, unsigned int)	//debug 용 나중에 꼭 지워 두기
//#define		AM8816_SPOT_OSD_SET				_IO( AM8816_IOCTL_MAGIC, 2 )		//이건 나중에 지우기

#endif	//_SPOT_ANF_IOCTL_H

