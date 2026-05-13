// arm-linux-gcc -o nf_sntp nf_util_sntp.c -I../include/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>

#include "unp.h"
#include "nf_common.h"
#include "nf_util_sntp.h"
#include "nf_network.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_sntp"

#define DEBUG_SNTP_JBSHELL

#ifdef DEBUG_SNTP_JBSHELL
	#include "jbshell.h"
#endif


#define DIFF19901970 2208988800.0

static void pack_ul(unsigned char *buf, unsigned long val)
{
	buf[0] = (val >> 24) & 0xff;
	buf[1] = (val >> 16) & 0xff;
	buf[2] = (val >> 8) & 0xff;
	buf[3] = val & 0xff;
}

static unsigned long unpack_ul(const unsigned char *buf)
{
	return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]) & 0xffffffff;
}

static void pack_ts(unsigned char *buf, double ts)
{
	double i, f;
	
	f = modf(ts, &i);
	pack_ul(buf, (unsigned long) i);
	pack_ul(buf + 4, (unsigned long) (f * 4294967296.0));
}

static double unpack_ts(const unsigned char *buf)
{
	return unpack_ul(buf) + unpack_ul(buf + 4) / 4294967296.0;
}

static void sntp_pack(unsigned char *buf, const struct sntp *sntp)
{
	buf[0] = (sntp->li << 6) | (sntp->vn << 3) | sntp->mode;
	buf[1] = sntp->stratum;
	buf[2] = sntp->poll;
	buf[3] = (signed char) sntp->precision;
	pack_ul(&buf[4], sntp->delay * 65536.0);
	pack_ul(&buf[8], sntp->dispersion * 65536.0);
	memcpy(&buf[12], &sntp->identifier, 4);
	pack_ts(&buf[16], sntp->reference);
	pack_ts(&buf[24], sntp->originate);
	pack_ts(&buf[32], sntp->receive);
	pack_ts(&buf[40], sntp->transmit);
}

static void sntp_unpack(struct sntp *sntp, const unsigned char *buf)
{
	sntp->li = buf[0] >> 6;
	sntp->vn = (buf[0] >> 3) & 0x07;
	sntp->mode = buf[0] & 0x07;
	sntp->stratum = buf[1];
	sntp->poll = buf[2];
	sntp->precision = buf[3];
	sntp->delay = unpack_ul(&buf[4]) / 65536.0;
	sntp->dispersion = unpack_ul(&buf[8]) / 65536.0;
	memcpy(&sntp->identifier, &buf[12], 4);
	sntp->reference = unpack_ts(&buf[16]);
	sntp->originate = unpack_ts(&buf[24]);
	sntp->receive = unpack_ts(&buf[32]);
	sntp->transmit = unpack_ts(&buf[40]);
}

static void sntp_tstotv(double ts, struct timeval *tv)
{
	double i, f;
	
	/* don't care the leap seconds */
	f = modf(ts - DIFF19901970, &i);
	tv->tv_sec = i;
	tv->tv_usec = f * 1e6;
}

static double sntp_tvtots(const struct timeval *tv)
{
	/* don't care the leap seconds */
	return DIFF19901970 + tv->tv_sec + tv->tv_usec * 1e-6;
}

static double sntp_now()
{
	struct timeval now;
	
	if (gettimeofday(&now, NULL) < 0)
		return -1;
	
	return sntp_tvtots(&now);
}

static const char *sntp_inspect(const struct sntp *sntp)
{
	static char buf[1024];

	snprintf(buf, sizeof buf,
		"li: %d\n"
		"vn: %d\n"
		"mode:%d\n"
		"stratum: %d\n"
		"poll: %d\n"
		"precision: %d\n"
		"delay: %f\n"
		"dispersion: %f\n"
		"identifier: %02x %02x %02x %02x\n"
		"reference: %f\n"
		"originate: %f\n"
		"receive: %f\n"
		"transmit: %f\n",
		sntp->li, sntp->vn, sntp->mode,
		sntp->stratum, sntp->poll, sntp->precision,
		sntp->delay, sntp->dispersion,
		sntp->identifier[0], sntp->identifier[1],
		sntp->identifier[2], sntp->identifier[3],
		sntp->reference, sntp->originate, sntp->receive, sntp->transmit);
		
	return buf;
}

/******************************************************************************/                                    

const double max_offset_adjtime = 1.0;
const double max_offset_settimeofday = 10.0;

enum action { SHOW, ADJTIME, SETTIMEOFDAY, GETTIME };
enum action action = SHOW;
static int debug = 0;
static int forceupdate = 0;
static int check = 0;

static time_t get_ntptime_now(double ts)
{
	struct timeval tv;
	time_t time;

	sntp_tstotv(ts, &tv);
	time = tv.tv_sec;

	return time;
}

static void print_time(double ts)
{
	struct timeval tv;
	time_t time;
	struct tm buff_tm;
	char buf[64];
	const char *fmt = "%a %b %d %H:%M:%S %Z %Y";

	sntp_tstotv(ts, &tv);
	time = tv.tv_sec;
	localtime_r(&time, &buff_tm);
	
	if (strftime(buf, sizeof(buf), fmt, &buff_tm) > 0)
		printf(buf);
}

static int sntp_query(int s, char* hostname, struct sntp *sntp)
{
	unsigned char buf[SNTP_HEADER_SIZE];
	struct sockaddr_storage ss;
	socklen_t size = sizeof(ss);
	struct sockaddr_in ai_addr;
	struct sockaddr* send_addr;
	struct timeval timeout;

//static int my_gethostbyname(const char *host, struct sockaddr_in *ret_addr)
	if( my_gethostbyname(hostname, &ai_addr) < 0)
	{
		g_message("%s dns[%s] failed!! ", __FUNCTION__, hostname);
		return -1;
	}
	
	sntp->li = 0;
	sntp->vn = 4;
	sntp->mode = 3;
	sntp->stratum = 0;
	sntp->poll = 0;
	sntp->precision = 0;
	sntp->delay = 0.0;
	sntp->dispersion = 0.0;
	memset(sntp->identifier, 0, sizeof(sntp->identifier));
	sntp->reference = 0.0;
	sntp->originate = 0.0;
	sntp->receive = 0.0;
	sntp->transmit = sntp_now();

	sntp_pack(buf, sntp);

	ai_addr.sin_family = AF_INET;	
	ai_addr.sin_port = htons(123);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	send_addr = &ai_addr;
	if (Sendto(s, buf, sizeof(buf), 0, send_addr, sizeof(ai_addr)) < 0)
		return -1;

	if (Setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
		return -1;

	//fprintf(stderr, "recvfrom start\n");
	if (Recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&ss, &size) < 0)
		return -1;

	//fprintf(stderr, "recvfrom end\n");
	sntp_unpack(sntp, buf);

	//fprintf(stderr, "size = %d\n", size);
	//printf(sntp_inspect(sntp));

	return 0;
}

static time_t do_action(const struct sntp *sntp, enum action action)
{
	time_t ntptime_cur;
	double t1 = sntp->originate;
	double t2 = sntp->receive;
	double t3 = sntp->transmit;
	double t4 = sntp_now();
	double d = (t4 - t1) - (t2 - t3);
	double t = ((t2 - t1) + (t3 - t4)) / 2.0;
	int r;
	
	switch (action) {
		case ADJTIME:
			if (fabs(t) > max_offset_adjtime) {
				fprintf(stderr, "%s : Offset range exceeded: %f\n", __FUNCTION__, t);
				return 1;
			}
			break;
		case SETTIMEOFDAY:
			if (!forceupdate && fabs(t) > max_offset_settimeofday) {
				fprintf(stderr, "%s : Offset range exceeded: %f\n", __FUNCTION__, t);
				return 1;
			}
			break;
		default:
			break;
	}

	r = 0;
	switch (action) {
		case SHOW:
			
#ifdef DEBUG_SNTP_MAIN
			print_time(t4 + t);
			printf("\n");
#endif
			break;

		case GETTIME:
			{
				ntptime_cur = get_ntptime_now(t4 + t);
				
#ifdef DEBUG_SNTP_MAIN
				print_time(t4);
				printf(" -> ");
				print_time(t4 + t);
				printf("\n");
#endif;				
				return ntptime_cur;
			}

		case ADJTIME:
			{
				double i, f;
				struct timeval delta;
				f = modf(t, &i);
				delta.tv_sec = i;
				delta.tv_usec = f * 1e6;
				r = adjtime(&delta, NULL);
				/*if (r == 0) {
					printf("adjusting %+.3f seconds\n", t);
				}*/
			}
			break;
			
		case SETTIMEOFDAY:
			{
				struct timeval now;
				sntp_tstotv(t4 + t, &now);
				r = settimeofday(&now, NULL);
				if (r == 0) {
					/*print_time(t4);
					printf(" -> ");
					print_time(t4 + t);
					printf("\n");*/
				}
			}
			break;
	}
	return r;
}

time_t nf_util_sntp(const char *hostname)
{
	int fd;
	time_t ntptime;

	struct sntp sntp;
	
	g_message("%s host[%s] called!!", __FUNCTION__, hostname);
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0 || sntp_query(fd, hostname, &sntp) < 0) {
		perror("sntp_query");
		close(fd);
		return 0;
	}

	ntptime = do_action(&sntp, GETTIME);
	close(fd);
	
	return ntptime;
}

#ifdef DEBUG_SNTP_MAIN
int main (int argc, char *argv[]) { 	
	nf_util_sntp("time.bora.net");	
	return 0;	
}
#endif


#ifdef DEBUG_SNTP_JBSHELL

static char sntp_help[] = "sntp [hostname]";
static int sntp(int argc, char **argv)
{		
	char *hostname = NULL;
	time_t	ntp_time = 0;
	
	
	if(argc < 2){
		printf("%s\n",sntp_help);
		return -1;
	}
	
	hostname = argv[1];	
		
	ntp_time = nf_util_sntp(hostname);	
	g_print("%s ntp_time[%d]\n",__FUNCTION__, ntp_time);
						
	return 0;
}
__commandlist(sntp, "sntp",  sntp_help, sntp_help);

#endif
