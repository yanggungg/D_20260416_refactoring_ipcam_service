#ifndef _NF_UTIL_SNTP_H
#define _NF_UTIL_SNTP_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

typedef struct sntp {
	int li;
	int vn;
	int mode;
	int stratum;
	int poll;
	
	signed char precision;
	
	double delay;
	double dispersion;
	
	unsigned char identifier[4];
	
	double reference;
	double originate;
	double receive;
	double transmit;  
} SNTP_PACKET;

#define SNTP_HEADER_SIZE 48

static void sntp_pack(unsigned char *, const struct sntp *);
static void sntp_unpack(struct sntp *, const unsigned char *);
static void sntp_tstotv(double, struct timeval *);
static double sntp_tvtots(const struct timeval *);
static double sntp_now();
static const char *sntp_inspect(const struct sntp *);

time_t nf_util_sntp(const char *hostname);

#endif

