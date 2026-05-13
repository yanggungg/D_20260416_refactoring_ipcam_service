#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <err.h>


#include "nf_common.h"
#include "nf_dvrlib.h"

ssize_t r_write(int fd, const void *buf, size_t count)
{
	ssize_t bytes_written = 0;
	ssize_t bytes_remain = count;
	ssize_t res;
	
	while ((res = write(fd, buf+bytes_written, bytes_remain)) != 0) {
		if (res == -1) {
			if (errno != EINTR)
				return -1;
		}
		else {
			bytes_written += res;
			bytes_remain = count - bytes_written;
			if (!bytes_remain)
				return bytes_written;
		}
	}
	return 0;
}

ssize_t r_read(int fd, void *buf, size_t count)
{
	ssize_t bytes_read = 0;
	ssize_t bytes_remain = count;
	ssize_t res;
	
	while ((res = read(fd, buf+bytes_read, bytes_remain)) != 0) {
		if (res == -1) {
			if (errno != EINTR) {
				return -1;
			}
		} else {
			bytes_read += res;
			bytes_remain = count - bytes_read;
			if (!bytes_remain) {
				return bytes_read;
			}
		}
	}
	return 0;
}

int r_nanosleep(struct timespec *req)
{
	struct timespec rem;
	while (nanosleep(req, &rem) == -1) 
	{
		if (errno != EINTR)
			return -1;
		*req = rem;
	}

	return 0;
}

int r_semop(int semid, struct sembuf *sops, int nsops)
{
	while (semop(semid, sops, nsops) == -1)
	{
		if (errno != EINTR)
			return -1;		
	}

	return 0;
}

int r_sem_wait(sem_t *sem)
{
	while (sem_wait(sem) == -1)
	{
		if (errno != EINTR)
			return -1;
	}

	return 0;
}

char *hstrcpy(char *dest, const char *src)
{
	if (dest == NULL || src == NULL)
		return NULL;
	
	return strcpy(dest, src);
}

char *hstrncpy(char *dest, const char *src, size_t n)
{
	if (dest == NULL || src == NULL || n == 0)
		return NULL;
	
	return strncpy(dest, src, n);
}
#if 0

#define		Y_R_IN			0.257
#define		Y_G_IN			0.504
#define		Y_B_IN			0.098
#define		Y_ADD_IN		16

#define		U_R_IN			0.148
#define		U_G_IN			0.291
#define		U_B_IN			0.439
#define		U_ADD_IN		128

#define		V_R_IN			0.439
#define		V_G_IN			0.368
#define		V_B_IN			0.071
#define		V_ADD_IN		128

#define		SCALEBITS_IN	8
#define		FIX_IN(x)		((Uint16) ((x) * (1L<<SCALEBITS_IN) + 0.5))

void RGBtoYUV(Uint32 r, Uint32 g, Uint32 b, Uint8 *y, Uint8 *u, Uint8 *v)	/* 05.12.01 modified by josco & cultfactory : exchange 'r' with 'b' */
{
	*y = (Uint8)((
				  FIX_IN(Y_R_IN) * r
				+ FIX_IN(Y_G_IN) * g
				+ FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;
	
	/* 05.12.01 modified by josco & cultfactory : exchange '*u' with '*v' */
	*u = (Uint8)((
				- FIX_IN(U_R_IN) * r * 2
				- FIX_IN(U_G_IN) * g * 2
				+ FIX_IN(U_B_IN) * b * 2) >> (SCALEBITS_IN + 1)) + U_ADD_IN;
	
	*v = (Uint8)((
				  FIX_IN(V_R_IN) * r * 2
				- FIX_IN(V_G_IN) * g * 2
				- FIX_IN(V_B_IN) * b * 2) >> (SCALEBITS_IN + 1)) + V_ADD_IN; 
}
#endif 


unsigned int BITMASK[] = {
	0x00000001,	0x00000002,	0x00000004,	0x00000008,
	0x00000010,	0x00000020,	0x00000040,	0x00000080,
	0x00000100,	0x00000200,	0x00000400,	0x00000800,
	0x00001000,	0x00002000,	0x00004000,	0x00008000,
	0x00010000,	0x00020000,	0x00040000,	0x00080000,
	0x00100000,	0x00200000,	0x00400000,	0x00800000,
	0x01000000,	0x02000000,	0x04000000,	0x08000000,
	0x10000000,	0x20000000,	0x40000000,	0x80000000,
	0xffffffff, 0x00000000
};

unsigned int BITMASK_EVENT[] = {
	0x00000001,	0x00000002,	0x00000004,	0x00000008,
	0x00000010,	0x00000020,	0x00000040,	0x00000080,
	0x00000100,	0x00000200,	0x00000400,	0x00000800,
	0x00001000,	0x00002000,	0x00004000,	0x00008000,
	0x0000ffff,	0x00000000,	0x00000000,	0x00000000,
	0x00000000,	0x00000000,	0x00000000,	0x00000000,
	0x00000000,	0x00000000,	0x00000000,	0x00000000,
	0x00000000,	0x00000000,	0x00000000,	0x00000000
};
