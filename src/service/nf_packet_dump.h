/**
  Packet Dump API Header
 */
#ifndef __NF_IPCAM_PACKET_DUMP_H__
#define __NF_IPCAM_PACKET_DUMP_H__

struct dump_status {
	int status;
	unsigned long size;   // Total Size
	unsigned long used;    // Used Size
	unsigned long avail;  // Avail Size
	char path[128];
};

enum status{
	PACKET_DUMP_STATE_STOP = 0,
	PACKET_DUMP_STATE_READY = 1,
	PACKET_DUMP_STATE_RUNN = 2,
};

extern int nf_ipcam_packet_dump_usb_check(char *path);
extern int nf_ipcam_packet_dump_usb_remove();
extern int nf_ipcam_packet_dump_get_status(struct dump_status *status);
extern int nf_ipcam_packet_dump_stop();
extern int nf_ipcam_packet_dump_start();
extern void nf_ipcam_packet_dump_init();

#endif //__NF_IPCAM_PACKET_DUMP_H__

