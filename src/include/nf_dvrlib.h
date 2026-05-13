#ifndef _NF_DVRLIB_H_
#define _NF_DVRLIB_H_

#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>

#if __STDC__
	#define DIFFERENCE 2208988800UL
#else
	#define DIFFERENCE 2208988800
#endif

typedef enum {
	SYS_STATE_STANDBY = 0x00000000,
	SYS_STATE_LOGIN = 0x01010101,
	SYS_STATE_SM = 0x02020202,
	
	SYS_STATE_NONE
	
}	SysState_e;

extern unsigned int BITMASK[];
extern unsigned int BITMASK_EVENT[];

extern ssize_t r_write(int fd, const void *buf, size_t count);
extern ssize_t r_read(int fd, void *buf, size_t count);
extern int r_nanosleep(struct timespec *req);
extern int r_semop(int semid, struct sembuf *sops, int nsops);
extern int r_sem_wait(sem_t *sem);

extern char *hstrcpy(char *dest, const char *src);
extern char *hstrncpy(char *dest, const char *src, size_t n);

extern void RGBtoYUV(unsigned r, unsigned g, unsigned b, 
						unsigned char *y, unsigned char *u, unsigned char *v);

#endif
