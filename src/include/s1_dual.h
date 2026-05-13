#ifndef __S1_DUAL_H__
#define __S1_DUAL_H__
#include "nf_object.h"
#include "nf_common.h"
struct _Nfs1Notify {
	GTimeVal	current_time;
	int notify_state;
};
typedef struct _Nfs1Notify 	NfS1Notify;

struct _Nfs1NotifyClass {
  NfObjectClass	parent_class;

  /* signals */

  /*< public >*/

  /*< private >*/

};
typedef struct _Nfs1NotifyClass 	Nfs1NotifyClass;
typedef enum _S1_DUAL_REG_IDX_E{
	S1_DUAL_REG_IDX_SECURITY 	= 0,
	S1_DUAL_REG_IDX_EMERGENCY 	= 1,
	S1_DUAL_REG_IDX_ENTER 		= 2,
	S1_DUAL_REG_IDX_DEVICE 		= 3,
	S1_DUAL_REG_IDX_NR
} S1_DUAL_REG_IDX_E;


typedef struct _NF_RECORD_NOTIFY_DATA_T {
	guint 		cb_change_flag;

	guint 		cb_rise_alarm;
	guint 		cb_rise_vloss;
	guint 		cb_rise_motion;

	guint 		cb_rise_va_event;
	guint 		cb_rise_user_event;

	guint 		cb_curr_alarm;
	guint 		cb_curr_vloss;
	guint 		cb_old_vloss;
	guint 		cb_curr_motion;

	guint 		cb_disk_full;
	guint 		cb_disk_exhaust;
	guint 		cb_disk_smart;
	guint 		cb_sys_booting;
	guint 		cb_dvr_login_failed;
	guint 		cb_net_login_failed;
	guint 		cb_panic_record;
	guint 		cb_disk_write_failed;
	guint 		cb_no_disk;
	guint 		cb_curr_va_event;
	guint 		cb_curr_user_event;

} NF_RECORD_NOTIFY_DATA;

typedef struct _S1_NOTFY_TIMER_T {
	glong	notify_time;
	int 	notify_timer;
	int 	init_notify_timer;
	NF_RECORD_NOTIFY_DATA notify_data;
}S1_NOTIFY_TIMER;


typedef struct _S1_DUAL_CTRL_INFO_T {
	
	int		is_running;	
	int		is_sysdb_changed;
	
	GMutex	*lock;
	GThread	*thread_id;
	
	int		udp_fd;	
	int		enq_timer;
		
	int		linked_cam_init;	
	unsigned int linked_cam[S1_DUAL_REG_IDX_NR];
		// ħ�԰�	
		// ����̺�Ʈ  C1(���), CA(���), QQ(����)��ȣ�� ����
		// �����̺�Ʈ
		// ����̺�Ʈ
						
	unsigned int dual_server;
	int		dual_port;
	S1_NOTIFY_TIMER notify;
} S1_DUAL_CTRL_INFO;


typedef struct  _S1_DUAL_HEADER_T {

	unsigned short pack_len;		// header + body + crc
	unsigned char  pack_ver;		// 0x10
	unsigned char  encrypt_ver;		// 0x10
	
	unsigned char  src_ipaddr[4];

	unsigned char  src_mac[6];
	unsigned short src_port;
	
	unsigned char  src_block;	
	unsigned char  src_node;
	unsigned char  dest_block;
	unsigned char  dest_node;

/*
00/01	 ��Ʈ�ѷ� 00����
01/01	 ��Ʈ�ѷ� 01����
...	
99/01	 ��Ʈ�ѷ� 99����
...	 �̻��
111/01	 DVR/NVR �ý���
����) node : (������ 01)
*/

	unsigned short pack_type;
	unsigned short msg_count;
/*
�� Message Count
	-. �ʱ�⵿�� �������� 0xffff��, �������� 0���� �ʱ�ȭ �ϸ�, 
	   0 ~ 1 ~ 65535 ~ 1 �� �����Ͽ� �ʱ�⵿�� ���� Count�� ���� �̺�Ʈ �н��� �����Ѵ�.
	-. ENQ ��Ŷ ���۽� 0�� ��� ��.
	-. ���� ��Ŷ�� ���ŵ� MESSAGE COUNT���� ��� ��.
	-. Packet ��з����� Message Count�� ���� �����Ѵ�.
	-. �������� ������⸶�� Message Count�� ���� �����Ѵ�.
	-. EVENT ACK ���Ž� �����Ѵ�.
*/
	unsigned char  retry_count;
/*
�� Packet Retry Count
-. ��Ÿ� ���¸� Ȯ���ϱ� ���� �ΰ������� �̿��Ѵ�.
-. �ʱ�⵿�� �������� 0����, �������� 0���� �ʱ�ȭ �ϸ�, �����۽� 0 ~ 255���� 
   �����Ͽ� 255 �ʰ��� 255�� ���� ��. 
-. ENQ ��Ŷ ���۽� 0�� ��� ��.  
-. ���� ��Ŷ�� ���ŵ� Retry Count���� ��� ��.
*/	
	unsigned char  bcd_year;
	unsigned char  bcd_month;	
	unsigned char  bcd_day;
	
	unsigned char  bcd_hour;
	unsigned char  bcd_min;
	unsigned char  bcd_sec;
/*
�� Year/Month/Day/Hour/Minute/Second
-. ��Ʈ�ѷ� �� ���� : �̺�Ʈ �� ��Ŷ �߻��ð�.
-. ��Ʈ�ѷ� �� ���� : ���� �� ��Ŷ �߻��ð�.
-. Retry�ô� ������ ��Ŷ �߻��ð��� �̿��ؾ� ��.
�� BCD�� ó����. 
*/	
	unsigned char  is_fire;
	
	unsigned char   contractno[10];
	unsigned char   reserved[8];
	
	unsigned short  data_len;	
} __attribute__((packed)) S1_DUAL_HEADER;


typedef struct  _S1_DUAL_PACK_ACK_T {
	S1_DUAL_HEADER  header;			

	unsigned short	pack_type;	
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_ACK;


#define S1_DUAL_MAX_DATA_LEN	128
#define S1_DUAL_CRC_LEN			2
#define S1_DUAL_MAX_PACKET_LEN  (sizeof(S1_DUAL_HEADER)+S1_DUAL_CRC_LEN+ S1_DUAL_MAX_DATA_LEN)
#define S1_DUAL_MIN_PACKET_LEN  (sizeof(S1_DUAL_HEADER)+S1_DUAL_CRC_LEN)

#define NF_TYPE_S1_NOTIFY				(nf_s1_notify_get_type())
#define NF_S1_NOTIFY(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_S1_NOTIFY, NfS1Notify))

/*
ENQ					0xa9	0x00	���Polling		DVR/NVR �� ��Ʈ�ѷ�
ENQ-ACK				0xb9	0x00	Polling-Ack		��Ʈ�ѷ� �� DVR/NVR

��Ʈ�ѷ� ����̺�Ʈ	0xa1	0x00	��� EVENT		��Ʈ�ѷ� �� DVR/NVR
					0xb1	0x00	DVR-Ack			DVR/NVR �� ��Ʈ�ѷ�

��Ʈ�ѷ� �����̺�Ʈ	0xa2	0x00	ī�� EVENT		��Ʈ�ѷ� �� DVR/NVR
							0x01	���� EVENT		��Ʈ�ѷ� �� DVR/NVR
					0xb2	0x00	DVR-Ack			DVR/NVR �� ��Ʈ�ѷ�

DVR �̺�Ʈ			0xa3	0x00	DVR EVENT		DVR/NVR �� ��Ʈ�ѷ�
					0xb3	0x00	��Ʈ�ѷ�-Ack	��Ʈ�ѷ� �� DVR/NVR

���������û		0xa4	0x00	DB��û			DVR/NVR �� ��Ʈ�ѷ�
					0xb4	0x00	��Ʈ�ѷ�-Ack	��Ʈ�ѷ� �� DVR/NVR

�����������		0xa5	0x00	DB���� ����		��Ʈ�ѷ� �� DVR/NVR
					0xb5	0x00	DVR-Ack			DVR/NVR �� ��Ʈ�ѷ�


# �ʱ� �������

1) 
ENQ					0xa9	0x00	���Polling		DVR/NVR �� ��Ʈ�ѷ�
ENQ-ACK				0xb9	0x00	Polling-Ack		��Ʈ�ѷ� �� DVR/NVR

2)
���������û		0xa4	0x00	DB��û			DVR/NVR �� ��Ʈ�ѷ�
					0xb4	0x00	��Ʈ�ѷ�-Ack	��Ʈ�ѷ� �� DVR/NVR

3) 
�����������		0xa5	0x00	DB���� ����		��Ʈ�ѷ� �� DVR/NVR
					0xb5	0x00	DVR-Ack			DVR/NVR �� ��Ʈ�ѷ�

*/
typedef enum _S1_DUAL_PACKET_TYPE_E{
	S1_DUAL_PACKET_TYPE_ENQ			= 0xa900,
	S1_DUAL_PACKET_TYPE_ENQ_ACK		= 0xb900,

	S1_DUAL_PACKET_TYPE_EVENT		= 0xa100,
	S1_DUAL_PACKET_TYPE_EVENT_ACK	= 0xb100,

	S1_DUAL_PACKET_TYPE_ENTER_CARD	= 0xa200,
	S1_DUAL_PACKET_TYPE_ENTER_STATUS= 0xa201,
	S1_DUAL_PACKET_TYPE_ENTER_ACK	= 0xb200,

	S1_DUAL_PACKET_TYPE_DVR			= 0xa300,
	S1_DUAL_PACKET_TYPE_DVR_ACK		= 0xb300,

	S1_DUAL_PACKET_TYPE_REQ_INFO		= 0xa400,
	S1_DUAL_PACKET_TYPE_REQ_INFO_ACK	= 0xb400,

	S1_DUAL_PACKET_TYPE_REG_INFO		= 0xa500,
	S1_DUAL_PACKET_TYPE_REG_INFO_ACK	= 0xb500,
					
} S1_DUAL_PACKET_TYPE_E;



typedef struct _S1_DUAL_PACK_ENQ_T {	// 
	S1_DUAL_HEADER  header;

	unsigned char   reserved[10]; // all 0xff
	unsigned char   crc[2];
} __attribute__((packed)) S1_DUAL_PACK_ENQ;


typedef struct _S1_DUAL_PACK_ENQACK_T {	// 
	S1_DUAL_HEADER  header;		

	unsigned char   reserved[10]; // all 0xff
	unsigned char   crc[2];
} __attribute__((packed)) S1_DUAL_PACK_ENQACK;		



/*
I	���	 	Initial
S	���(��Ʈ)	Set
R	����	 	Release
P	��ȸ	 	Patrol
A	�˶�	 	Alarm
O	����	 	One shot
H	���ü�Ʈ
W	��纸��
F	����̻�
X	������
B	����
*/

typedef enum _S1_DUAL_SYS_MODE_E{
	S1_DUAL_SYS_MODE_INIT		= 'I', // ���
	S1_DUAL_SYS_MODE_SET		= 'S', // ���
	S1_DUAL_SYS_MODE_RELEASE	= 'R', // ����
	S1_DUAL_SYS_MODE_PATROL		= 'P', // ��ȸ
	S1_DUAL_SYS_MODE_ALARM		= 'A', // �˶�
	S1_DUAL_SYS_MODE_ONESHOT	= 'O', // ����
	S1_DUAL_SYS_MODE_HOME		= 'H', // ���ü�Ʈ
	S1_DUAL_SYS_MODE_REPAIR		= 'W', // ��纸��
	S1_DUAL_SYS_MODE_COMM_ERR	= 'F', // ����̻�
	S1_DUAL_SYS_MODE_X			= 'X', // ������
	S1_DUAL_SYS_MODE_BUFF		= 'B'  // ����
} S1_DUAL_SYS_MODE_E;

// System status
/*
00 : ����
OV : ���� Overflow
AC : ����
AR : ��������
KK : �Ѳ�����
CM : ����̻�
BT : ���͸� ������
TB : ���͸� ������ ����
PP : ��������

T1 : �Ϲ�ħ��
T3 : ����ħ��
T4 : �ݰ�ħ��

F1 : ȭ��
C1 : ��������
CA : �������
QQ : ����
G1 : ����
WC : �����ܸ� ����̻�
WB : �����ܸ� BatLow
BB : ���͸��ҷ�
YA : ���Ĺ���
*/

typedef enum _S1_DUAL_LOOP_POS_E{
	S1_DUAL_LOOP_POS_RECOVER = '0',
	S1_DUAL_LOOP_POS_SHORT   = 'S',
	S1_DUAL_LOOP_POS_OPEN    = 'P',
	S1_DUAL_LOOP_POS_NA 	 = '*',	
} S1_DUAL_LOOP_POS_E;

/*
0x01	HOST(����)
0x02	LPC(��Į����)
0x03	IST
0x04	ANN(�˶�ǥ�ñ�)
0x05	MCN(������Ʈ�ѷ�)
0x06	OPU(����ǥ�ñ�)
0x07	LC(��Į��Ʈ�ѷ�)
0x08	������
0x09	ī������
0x0C	����Ʈ��
0xFF	�ش����
*/
typedef enum _S1_DUAL_OP_TYPE_E{
	S1_DUAL_OP_TYPE_HOST		= 0x01,	
	S1_DUAL_OP_TYPE_LPC			= 0x02,	
	S1_DUAL_OP_TYPE_IST			= 0x03,	
	S1_DUAL_OP_TYPE_ANN			= 0x04,	
	S1_DUAL_OP_TYPE_MCN			= 0x05,	
	S1_DUAL_OP_TYPE_OPU			= 0x06,	
	S1_DUAL_OP_TYPE_LC			= 0x07,	
	S1_DUAL_OP_TYPE_SCHD		= 0x08,	
	S1_DUAL_OP_TYPE_CARD		= 0x09,
	S1_DUAL_OP_TYPE_SPHONE		= 0x0c,
	S1_DUAL_OP_TYPE_NONE		= 0xFF	
} S1_DUAL_OP_TYPE_E;

typedef struct _S1_DUAL_PACK_EVENT_T {
	S1_DUAL_HEADER  header;		

	unsigned char   sys_mode[1];
	unsigned char   sys_status[2];
	unsigned char   loop_addr[2];
	unsigned char   loop_pos[1];	
	unsigned char   card_num[14];
	unsigned char   op[1];
	
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_EVENT;



typedef struct _S1_DUAL_PACK_CARD_T {
	S1_DUAL_HEADER  header;		

	unsigned char   gate[1];		// 0xff N/A
	unsigned char   reader[1];		// 0xff N/A  0x01:enter 0x02:exit
	unsigned char   mode[1];
	unsigned char   status[2];	
	unsigned char   card_num[14];
		
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_CARD;

typedef struct _S1_DUAL_PACK_STATUS_T {
	S1_DUAL_HEADER  header;		

	unsigned char   gate[1];		// 0xff N/A
	unsigned char   reader[1];		// 0xff N/A  0x01:enter 0x02:exit
	unsigned char   mode[1];
	unsigned char   status[2];	
	unsigned char   card_num[14];
		
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_STATUS;

/*
�ڵ�(Stauts)
Hexa	�� �� ��
4104	�н�ī�� ���ԺҰ�
4106	�̵��ī�� ���ԺҰ�
4109	ī�� ��ȿ�Ⱓ ���Ŵ��
4110	ī�� ��ȿ�Ⱓ ����
4112	���ԺҰ��ð� ���ԺҰ�
4114	ī���̻� ��ȸOTP�̻�)
4115	��ô��� ���ԺҰ�
4116	����� ���ԺҰ�
4118	�ڵ��� ����� ���ԺҰ�
4136	�ӽñⰣ �����㰡
4200	�����㰡 (�Խ�)
4201	�����㰡 (���)
4206	������ ��ȸ ����ī�� ����
4207	����� ��ȸ ����ī�� ���ԺҰ�
4208	����� ����
4218	DOUBLE CARD ����
4230	��Ƽ�н� ���ԺҰ�
4233	�����ڰݾ��� ���ԺҰ�
4235	����ڰݾ��� ����Ұ�
4237	�̵��ī�� ����Ұ�
4238	�н�ī�� ����Ұ�
4300	��� (����)
4301	Ǯ�� (����)
4302	����
4303	����
4304	������ (��������)
4305	��迭�� (��������)
4306	Ǯ������ (��������)
4307	Ǯ������ (��������)
*/

typedef struct _S1_DUAL_PACK_DVR_T {
	S1_DUAL_HEADER  header;		

	unsigned char   sys_mode[1];	// 'O' One shot
	unsigned char   sys_status[2];
	unsigned char   loop_addr[2];	// 0x2a2a
	unsigned char   loop_pos[1];	// 0x2a
	
	unsigned char	dev_code[2];	// '2I'
	unsigned char	sub_code[1];	// 0x01 : ī�޶�1  '*':�ش���׾���
		
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_DVR;
typedef struct _S1_DUAL_PACK_DVR_ACK_T {
	S1_DUAL_HEADER  header;

	unsigned short data_len;
	unsigned char  source_packet_type[2];
	unsigned char 	crc[2];
} __attribute__((packed)) S1_DUAL_PACK_DVR_ACK;
/*
����	ǥ�ó���	STATUS	 �μ� �ڵ�	 ����
1	HDD ERR				I2	2I	HDD FULL
2	SMART ERR			I4	2I	��ȭ�� �ȵǴ� ���
3	DVR �����ν�		I1	2I	ī�޶�1~4
4	�����ν� ����		I0	2I
5	����̻�			IM	2I	10�� 1ȸ
6	����̻� ����		IR	2I	
*/

typedef struct _S1_DUAL_PACK_REQ_INFO_T {	
	S1_DUAL_HEADER  header;		

	unsigned char   reserved[10]; // all 0xff
	
	unsigned char   crc[2];
} __attribute__((packed)) S1_DUAL_PACK_REQ_INFO;		

typedef struct _S1_DUAL_PACK_REG_INFO_T {	
	S1_DUAL_HEADER  header;		

	unsigned char   data[S1_DUAL_REG_IDX_NR][4]; 		// ħ�԰�	
	//unsigned char   data1[4]; 		// ����̺�Ʈ  C1(���), CA(���), QQ(����)��ȣ�� ����
	//unsigned char   data2[4]; 		// �����̺�Ʈ
	//unsigned char   data3[4]; 		// ����̺�Ʈ
			
	unsigned char   crc[2];		
} __attribute__((packed)) S1_DUAL_PACK_REG_INFO;




/*
*/


#endif
