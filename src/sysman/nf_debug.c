#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>

//#include <bits/sigcontext.h>

#include <signal.h>
#include <sys/ucontext.h>
#include <execinfo.h>

#include "nf_debug.h"

#define DEBUG_JBSHELL_DEBUG

#ifdef DEBUG_JBSHELL_DEBUG
#include "jbshell.h"
#endif

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "debug"

//#define DEBUG_NF_DEBUG

static gint
_get_contents(gchar* filename, gchar** contents, gsize *length)
{
	GError *error = NULL;

	if(!g_file_get_contents (filename, contents, length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return -1;
	}

	//g_message("fileopen name[%s] len[%d]", filename, *length);

	return 0;
}

static gint _nf_debug_sock = -1;
static struct sockaddr_in  _nf_debug_svr_addr;

static gint
_send_debug_svr(NF_DEBUG_PACKET *pack)
{
	gint	send_size = 0;
	gint	ret = 0;

	g_return_val_if_fail( pack , 0);

	g_return_val_if_fail( pack->size >0 , 0);
	g_return_val_if_fail( pack->size < NF_DEBUG_MAX_PACKET_DATA, 0);

	if(_nf_debug_sock == -1)
	{
		_nf_debug_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		g_return_val_if_fail( _nf_debug_sock != -1 , 0);

		memset(&_nf_debug_svr_addr, 0, sizeof(_nf_debug_svr_addr));

		_nf_debug_svr_addr.sin_family = AF_INET;
		_nf_debug_svr_addr.sin_addr.s_addr = inet_addr(NF_DEBUG_SERVER_IP);
		_nf_debug_svr_addr.sin_port = htons((gushort)NF_DEBUG_SERVER_PORT);
	}

#ifdef DEBUG_NF_DEBUG
	g_message("pack->identity : [%x]", pack->identity);
	g_message("pack->size     : [%d]", pack->size);
	g_message("pack->type     : [%d]", pack->type);
	g_message("pack->time     : [%ld]", pack->time_val.tv_sec);
	g_message("==========================================");
	g_message("pack->data     :\n%*.*s", pack->size, pack->size, pack->data);
#endif

	send_size = NF_DEBUG_PACKET_PRE_SIZE + pack->size;
	ret = sendto( _nf_debug_sock, (void*)pack, send_size , 0,
				(struct sockaddr *)&_nf_debug_svr_addr, sizeof(_nf_debug_svr_addr) );

#ifdef DEBUG_NF_DEBUG
	nf_debug_hexdump( pack, send_size);
#endif

	if(ret != send_size)
	{
		g_warning("%s sendto ret[%d] size[%d]", __FUNCTION__, ret, send_size);
		return -1;
	}
	return ret;
}

gboolean nf_debug_net_send_msg(gint type, const gchar *msg, const gchar *tag)
{
	NF_DEBUG_PACKET pack;
	gint	ret = 0;
	GTimeVal curr_timeval;

	g_return_val_if_fail( msg , 0);

	pack.identity = NF_DEBUG_IDENTITY;
	pack.type = type;
	gettimeofday( &curr_timeval, NULL);
	pack.time_val = curr_timeval;

#if 0
	if(tag && strlen(tag)>NF_DEBUG_MAX_TAG_DATA)
	{
		g_warning("%s tag data is too long..", __FUNCTION__);
		return FALSE;
	}
#endif

	if(tag)
		strncpy(pack.tag, tag, NF_DEBUG_MAX_TAG_DATA - 1);
	else
		pack.tag[0] = 0;

	strncpy(pack.data, msg, NF_DEBUG_MAX_PACKET_DATA);
	pack.size = strnlen(msg, NF_DEBUG_MAX_PACKET_DATA);

	return (_send_debug_svr(&pack) >0) ? 1:0;
}

gboolean nf_debug_net_send_file(gint type, const gchar *filename, const gchar *tag)
{
	NF_DEBUG_PACKET pack;

	gint	ret = 0;
	gint	send_size = 0;
	gint	contents_len = 0;
	gchar	*contents = NULL;
	GTimeVal curr_timeval;

	g_return_val_if_fail( filename , 0);

	pack.identity = NF_DEBUG_IDENTITY;
	pack.type = type;
	gettimeofday( &curr_timeval, NULL);
	pack.time_val = curr_timeval;

#if 0
	if(tag && strlen(tag)>NF_DEBUG_MAX_TAG_DATA)
	{
		g_warning("%s tag data is too long..", __FUNCTION__);
		return FALSE;
	}
#endif

	if(tag)
		strncpy(pack.tag, tag, NF_DEBUG_MAX_TAG_DATA - 1);
	else
		pack.tag[0] = 0;

	ret = _get_contents(filename, &contents, &contents_len);

	g_return_val_if_fail( ret != -1 , 0);

	send_size = (contents_len<NF_DEBUG_MAX_PACKET_DATA)
						? contents_len: (NF_DEBUG_MAX_PACKET_DATA-1);

	memcpy(pack.data, contents, send_size);
	pack.size = send_size;

	g_free(contents);

	return (_send_debug_svr(&pack) >0) ? 1:0;
}

void _net_send_log_hendler( const gchar *log_domain, GLogLevelFlags log_level,
								const gchar *message, gpointer user_data)
{
	if(user_data)
	{
		if(log_level == G_LOG_LEVEL_MESSAGE)
			printf("%s-Message: %s\n", log_domain, message);
		else if(log_level == G_LOG_LEVEL_WARNING)
			printf("%s-WARNING: %s\n", log_domain, message);
		else
			printf("%s[0x%02x]: %s\n", log_domain, log_level, message);

	}

	nf_debug_net_send_msg(NF_DEBUG_PACKET_TYPE_MSG_MESSAGE, message, log_domain);
}

void nf_debug_test(void)
{
	gchar data[1024*64] ={0, };
	gint handler_id;

	g_message("conncet %s, port [%d]", NF_DEBUG_SERVER_IP, NF_DEBUG_SERVER_PORT);
	nf_debug_net_send_file(NF_DEBUG_PACKET_TYPE_CMEM,		"/proc/cmem",		"test cmem tag");
	nf_debug_net_send_file(NF_DEBUG_PACKET_TYPE_IFS, 		"/proc/ifs",		"test ifs tag");
	nf_debug_net_send_file(NF_DEBUG_PACKET_TYPE_MEMINFO,	"/proc/meminfo",	"test meminfo tag");

	handler_id = g_log_set_handler (NULL, G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING ,
								_net_send_log_hendler, NULL);
}


gboolean nf_debug_mem_chkpoint(const gchar *tag)
{
	nf_debug_net_send_file(NF_DEBUG_PACKET_TYPE_CMEM,		"/proc/cmem",		tag);
	nf_debug_net_send_file(NF_DEBUG_PACKET_TYPE_MEMINFO,	"/proc/meminfo",	tag);
}


#define NF_DEBUG_MAX_LOG_CNT	32
#define NF_DEBUG_MAX_CATE_LEN	32

typedef struct _NF_DEBUG_LOG_T {
	char	cate[NF_DEBUG_MAX_CATE_LEN];
	char	**log_str;
	guint	*log_arr;
	guint	log_arr_max_idx;
} NF_DEBUG_LOG;


static GStaticMutex _nf_debug_log_mutex = G_STATIC_MUTEX_INIT;
static NF_DEBUG_LOG _nf_debug_log[NF_DEBUG_MAX_LOG_CNT];
static guint 		_nf_debug_log_cnt = 0;

gboolean nf_debug_category_add(const gchar *cate, const gchar *log_str, guint *log_arr, guint log_arr_max_idx)
{
	NF_DEBUG_LOG	*plog;

	g_static_mutex_lock (&_nf_debug_log_mutex);

	if( _nf_debug_log_cnt >= NF_DEBUG_MAX_LOG_CNT)
	{
		g_static_mutex_unlock (&_nf_debug_log_mutex);

		g_return_val_if_fail( _nf_debug_log_cnt >= NF_DEBUG_MAX_LOG_CNT, 0);
	}

	if( cate == NULL || log_str == NULL || log_arr == NULL || log_arr_max_idx == 0 )
	{
		g_static_mutex_unlock (&_nf_debug_log_mutex);

		g_return_val_if_fail( cate, 0);
		g_return_val_if_fail( log_str, 0);
		g_return_val_if_fail( log_arr, 0);
		g_return_val_if_fail( log_arr_max_idx >0 && log_arr_max_idx < 32, 0);
	}

	if(_nf_debug_log_cnt == 0)
		memset(_nf_debug_log, 0x00, sizeof(_nf_debug_log));

	plog = &_nf_debug_log[_nf_debug_log_cnt];

	strncpy( plog->cate, cate, sizeof(plog->cate)-1 );
	plog->log_str = log_str;
	plog->log_arr = log_arr;
	plog->log_arr_max_idx =log_arr_max_idx;

	++_nf_debug_log_cnt;

	g_static_mutex_unlock (&_nf_debug_log_mutex);

	g_message("%s cate[%s] log_str[0x%08x] log_arr[0x%08x] log_arr_max_idx[%d]  [%d]",
				__FUNCTION__, cate, log_str, log_arr, log_arr_max_idx,
				_nf_debug_log_cnt );

	return 1;
}

gboolean nf_debug_dump(const gchar *cate)
{
	guint i=0, j=0;

	for(i=0; i<_nf_debug_log_cnt; i++)
	{
		if( strncmp( _nf_debug_log[i].cate, cate, NF_DEBUG_MAX_CATE_LEN)  == 0)
		{
			NF_DEBUG_LOG	*plog = &_nf_debug_log[i];

			g_print(" %s (%d) =================================\n\n", plog->cate, plog->log_arr_max_idx);

			for(j=0; j<plog->log_arr_max_idx; j++)
			{
				g_print("    %-32s = %2d, 0x%08x\n", plog->log_str[j], j, plog->log_arr[j]);
				if( (j+1) % 5 == 0) g_print("\n");
			}
			g_print("\n\n");
			return 1;
		}
	}

	g_warning("%s not found cate[%s]", __FUNCTION__, cate);
	return 0;
}

gboolean nf_debug_dump_all()
{
	guint i=0, j=0;

	for(i=0; i<_nf_debug_log_cnt; i++)
	{
		NF_DEBUG_LOG	*plog = &_nf_debug_log[i];

		g_print(" %s (%d) =================================\n\n", plog->cate, plog->log_arr_max_idx);
		for(j=0; j<plog->log_arr_max_idx; j++)
		{
			g_print("    %-32s = %2d, 0x%08x\n", plog->log_str[j], j, plog->log_arr[j]);
			if( (j+1) % 5 == 0) g_print("\n");
		}
		g_print("\n\n");
	}

	return 1;
}


gboolean nf_debug_set(const gchar *cate, guint idx, guint value )
{
	guint i,j;

	for(i=0; i<_nf_debug_log_cnt; i++)
	{
		if( strncmp( _nf_debug_log[i].cate, cate, NF_DEBUG_MAX_CATE_LEN)  == 0)
		{
			NF_DEBUG_LOG	*plog = &_nf_debug_log[i];
			guint			prev_val = 0;

			g_return_val_if_fail( idx <= plog->log_arr_max_idx ,0);

			if(plog->log_arr_max_idx == idx)
			{
				for(j=0;j<plog->log_arr_max_idx;j++)
				{
					prev_val = plog->log_arr[idx];
					plog->log_arr[j] = value;

					g_message("%s cate[%-32s][%2d] prev[0x%08x] new[0x%08x]",
								__FUNCTION__, plog->log_str[j], j, prev_val, value);
				}
			}else{
				prev_val = plog->log_arr[idx];
				plog->log_arr[idx] = value;

				g_message("%s cate[%-32s][%2d] prev[0x%08x] new[0x%08x]",
							__FUNCTION__, plog->log_str[idx], idx, prev_val, value);
			}
			return 1;
		}
	}
	g_warning("%s not found cate[%s]", __FUNCTION__, cate);
	return 0;
}

gboolean nf_debug_hexdump(gpointer p, int len)
{
    static char line[] =
        "00000000  xx xx xx xx  xx xx xx xx  "
        "xx xx xx xx  xx xx xx xx  yyyyyyyy yyyyyyyy";
    static char hex[] = "0123456789abcdef";
    char *s;
    int thistime;
    char *l;

    int col;
	int base = 0;

	g_message( "--------------------------------------------------------------------------------");
//Debug("0         1         2         3         4         5         6         7");
//Debug("0123456789012345678901234567890123456789012345678901234567890123456789012345678");
//Debug("xxxxxxxx  xx xx xx xx  xx xx xx xx  xx xx xx xx  xx xx xx xx  ........ ........");

    base -= (int)p;
    for (s = p; len; len -= thistime) {
        sprintf(line, "%08x", (unsigned int)(s + base));
        line[8] = ' ';
        thistime = len > 16 ? 16 : len;

        l = line + 10;
        for (col = 0; col < thistime; col++) {
            *l++ = hex[(*s & 0xf0) >> 4];
            *l++ = hex[*s++ & 0x0f];
            l += ((col & 3) == 3) + 1;
        }
        while (col < 16) {
            *l = l[1] = ' ';
            l += ((col++ & 3) == 3) + 3;
        }

        s -= thistime;
        for (col = 0; col < thistime; col++) {
            *l++ = isprint(*s) ? *s : '.';
            l += col == 7;
            s++;
        }
        while (col < 16) {
            *l++ = ' ';
            l += col++ == 7;
        }

        g_message("%s",line);
    }

g_message("--------------------------------------------------------------------------------");
    return 0;

}

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
#define REG     "%08lx"
#define REGS_PER_LINE   8
#define LAST_VOLATILE   12

#define MSR_SF_LG	63              /* Enable 64 bit mode */
#define MSR_ISF_LG	61              /* Interrupt 64b mode valid on 630 */
#define MSR_HV_LG 	60              /* Hypervisor state */
#define MSR_VEC_LG	25	        /* Enable AltiVec */
#define MSR_POW_LG	18		/* Enable Power Management */
#define MSR_WE_LG	18		/* Wait State Enable */
#define MSR_TGPR_LG	17		/* TLB Update registers in use */
#define MSR_CE_LG	17		/* Critical Interrupt Enable */
#define MSR_ILE_LG	16		/* Interrupt Little Endian */
#define MSR_EE_LG	15		/* External Interrupt Enable */
#define MSR_PR_LG	14		/* Problem State / Privilege Level */
#define MSR_FP_LG	13		/* Floating Point enable */
#define MSR_ME_LG	12		/* Machine Check Enable */
#define MSR_FE0_LG	11		/* Floating Exception mode 0 */
#define MSR_SE_LG	10		/* Single Step */
#define MSR_BE_LG	9		/* Branch Trace */
#define MSR_DE_LG	9 		/* Debug Exception Enable */
#define MSR_FE1_LG	8		/* Floating Exception mode 1 */
#define MSR_IP_LG	6		/* Exception prefix 0x000/0xFFF */
#define MSR_IR_LG	5 		/* Instruction Relocate */
#define MSR_DR_LG	4 		/* Data Relocate */
#define MSR_PE_LG	3		/* Protection Enable */
#define MSR_PX_LG	2		/* Protection Exclusive Mode */
#define MSR_PMM_LG	2		/* Performance monitor */
#define MSR_RI_LG	1		/* Recoverable Exception */
#define MSR_LE_LG	0 		/* Little Endian */

#ifdef __ASSEMBLY__
#define __MASK(X)	(1<<(X))
#else
#define __MASK(X)	(1UL<<(X))
#endif

/* so tests for these bits fail on 32-bit */
#define MSR_SF		0
#define MSR_ISF		0
#define MSR_HV		0

#define MSR_VEC		__MASK(MSR_VEC_LG)	/* Enable AltiVec */
#define MSR_POW		__MASK(MSR_POW_LG)	/* Enable Power Management */
#define MSR_WE		__MASK(MSR_WE_LG)	/* Wait State Enable */
#define MSR_TGPR	__MASK(MSR_TGPR_LG)	/* TLB Update registers in use */
#define MSR_CE		__MASK(MSR_CE_LG)	/* Critical Interrupt Enable */
#define MSR_ILE		__MASK(MSR_ILE_LG)	/* Interrupt Little Endian */
#define MSR_EE		__MASK(MSR_EE_LG)	/* External Interrupt Enable */
#define MSR_PR		__MASK(MSR_PR_LG)	/* Problem State / Privilege Level */
#define MSR_FP		__MASK(MSR_FP_LG)	/* Floating Point enable */
#define MSR_ME		__MASK(MSR_ME_LG)	/* Machine Check Enable */
#define MSR_FE0		__MASK(MSR_FE0_LG)	/* Floating Exception mode 0 */
#define MSR_SE		__MASK(MSR_SE_LG)	/* Single Step */
#define MSR_BE		__MASK(MSR_BE_LG)	/* Branch Trace */
#define MSR_DE		__MASK(MSR_DE_LG)	/* Debug Exception Enable */
#define MSR_FE1		__MASK(MSR_FE1_LG)	/* Floating Exception mode 1 */
#define MSR_IP		__MASK(MSR_IP_LG)	/* Exception prefix 0x000/0xFFF */
#define MSR_IR		__MASK(MSR_IR_LG)	/* Instruction Relocate */
#define MSR_DR		__MASK(MSR_DR_LG)	/* Data Relocate */
#define MSR_PE		__MASK(MSR_PE_LG)	/* Protection Enable */
#define MSR_PX		__MASK(MSR_PX_LG)	/* Protection Exclusive Mode */
#ifndef MSR_PMM
#define MSR_PMM		__MASK(MSR_PMM_LG)	/* Performance monitor */
#endif
#define MSR_RI		__MASK(MSR_RI_LG)	/* Recoverable Exception */
#define MSR_LE		__MASK(MSR_LE_LG)	/* Little Endian */

#define FULL_REGS(regs)     (((regs)->trap & 1) == 0)
#define TRAP(regs)      ((regs)->trap & ~0xF)

#define MIN_STACK_FRAME 16
#define FRAME_LR_SAVE   1
#define INT_FRAME_SIZE  (sizeof(struct pt_regs) + STACK_FRAME_OVERHEAD)
#define REGS_MARKER 0x72656773ul
#define FRAME_MARKER    2

static struct regbit {
    unsigned long bit;
    const char *name;
} msr_bits[] = {
    {MSR_EE,    "EE"},
    {MSR_PR,    "PR"},
    {MSR_FP,    "FP"},
    {MSR_ME,    "ME"},
    {MSR_IR,    "IR"},
    {MSR_DR,    "DR"},
    {0,     NULL}
};
static void printbits(unsigned long val, struct regbit *bits)
{
    const char *sep = "";

    fprintf(stderr, "<");
    for (; bits->bit; ++bits)
        if (val & bits->bit) {
            fprintf(stderr, "%s%s", sep, bits->name);
            sep = ",";
        }
    fprintf(stderr, ">");
}
#endif

/* Obtain a backtrace and print it to stdout. */
void nf_debug_backtrace (void)
{
	void *frame_addrs[256];
	size_t size;
	char **frame_strings;
	size_t i;

	size = backtrace (frame_addrs, 256);
	frame_strings = backtrace_symbols (frame_addrs, size);

	printf("=== Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf("%3d [0x%08x] %s\n", i, frame_addrs[i], frame_strings[i]);

	free (frame_strings);
}



void nf_debug_show_registers(struct sigcontext *ctx)
{
#ifdef BUILD_ERROR //nskim_de17
#if !defined(_OTM_MODEL) && !defined(_SNF_MODEL)
	fprintf(stderr, "Registers:\n");
	fprintf(stderr, "  R0: %08lx  R1: %08lx  R2: %08lx  R3: %08lx\n",
		ctx->arm_r0, ctx->arm_r1, ctx->arm_r2, ctx->arm_r3);
	fprintf(stderr, "  R4: %08lx  R5: %08lx  R6: %08lx  R7: %08lx\n",
		ctx->arm_r4, ctx->arm_r5, ctx->arm_r6, ctx->arm_r7);
	fprintf(stderr, "  R8: %08lx  R9: %08lx  SL: %08lx  FP: %08lx\n",
		ctx->arm_r8, ctx->arm_r9, ctx->arm_r10, ctx->arm_fp);
	fprintf(stderr, "  IP: %08lx  SP: %08lx  LR: %08lx  PC: %08lx\n",
		ctx->arm_ip, ctx->arm_sp, ctx->arm_lr, ctx->arm_pc);
	fprintf(stderr, "  CPSR: %08lx  Addr: %08lx\n",
		ctx->arm_cpsr, ctx->fault_address);
#else
    int i, trap;
	struct pt_regs * regs = ctx->regs;

    fprintf(stderr, "NIP: "REG" LR: "REG" CTR: "REG"\n",
           regs->nip, regs->link, regs->ctr);
    fprintf(stderr, "REGS: %p TRAP: %04lx\n",
           regs, regs->trap);
    fprintf(stderr, "MSR: "REG" ", regs->msr);
    printbits(regs->msr, msr_bits);
    fprintf(stderr, "  CR: %08lx  XER: %08lx\n", regs->ccr, regs->xer);
    trap = TRAP(regs);
    if (trap == 0x300 || trap == 0x600)
        fprintf(stderr, "DEAR: "REG", ESR: "REG"\n", regs->dar, regs->dsisr);
//  fprintf(stderr, "TASK = %p[%d] '%s' THREAD: %p",
//         current, task_pid_nr(current), current->comm, task_thread_info(current));

    for (i = 0;  i < 32;  i++) {
        if ((i % REGS_PER_LINE) == 0)
            fprintf(stderr, "\n" "GPR%02d: ", i);
        fprintf(stderr, REG " ", regs->gpr[i]);
        if (i == LAST_VOLATILE && !FULL_REGS(regs))
            break;
    }
    fprintf(stderr, "\n");

//    show_stack((unsigned long *) regs->gpr[1]);
#endif
#endif
}


#if defined (_IPX_0412L4) || defined(_IPX_0824P4) || defined(_IPX_1648P4) \
 || defined (_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
 || defined (_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
//IPXP4 HWT??
#else
void nf_debug_backtrace_signal(int sig, struct siginfo *info, struct ucontext *ctx)
{
#ifdef BUILD_ERROR //nskim_de17
#if !defined(_OTM_MODEL) && !defined(_SNF_MODEL)
	void *trace[64];
	char **messages = (char **)NULL;
	int i, trace_size = 0;

 	struct sigcontext *sc = &ctx->uc_mcontext;

	printf("Got signal %d\n", sig);

	nf_debug_show_registers(sc);

	trace[1] = (void *)sc->fault_address;

	trace_size = backtrace(trace, 64);
	/* overwrite sigaction with caller's address */
	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");

	for (i=1; i<trace_size; ++i)
		printf("[bt] [%2d] [0x%08x] %s\n",i , trace[i], messages[i]);
#endif
#endif
	printf("[bt] Execution path:\n");
	exit(0);
}
#endif

//http://www.linuxjournal.com/article/6391

#ifdef DEBUG_JBSHELL_DEBUG

static char debug_logall_help[] = "debug_all";
static int debug_logall(int argc, char **argv)
{
	nf_debug_dump_all();
	return 0;
}
__commandlist(debug_logall,"debug_logall", debug_logall_help, debug_logall_help);

static char debug_help[] = "debug [cate] [idx] [val]";
static int debug(int argc, char **argv)
{
	gint idx;
	guint val;
	gchar *cate;

	if(argc < 4){
		printf("%s\n",debug_help);

		if(argc>1)
			nf_debug_dump(argv[1]);
		else
			nf_debug_dump_all();

		return -1;
	}

	cate = argv[1];
	idx = strtol(argv[2],NULL,0);
	val = strtoul(argv[3],NULL,0);

	nf_debug_set(cate, idx, val);

	return 0;
}
__commandlist(debug,"debug", debug_help, debug_help);

static char debug_mem_help[] = "debug_mem [tag]";
static int debug_mem(int argc, char **argv)
{
	if(argc < 2){
		printf("%s\n",debug_mem_help);
		return -1;
	}

	nf_debug_mem_chkpoint(argv[1]);
	return 0;
}
__commandlist(debug_mem,"debug_mem", debug_mem_help, debug_mem_help);



typedef struct _LOG_DOMAIN_HANDLER_T
{
	gchar	domain[64];
	guint	handler_id;
} LOG_DOMAIN_HANDLER;

#define MAX_LOG_HANDLER_CNT		16

LOG_DOMAIN_HANDLER	_log_handler[MAX_LOG_HANDLER_CNT];
guint 				_log_handler_cnt = 0;

static char debug_netadd_help[] = "debug_netadd [domain] [local_print:1]";
static int debug_netadd(int argc, char **argv)
{

	guint 	i=0;
	gint	local_print = 1;
	gchar	*log_domain;
	guint	handler_id = 0;

	if(argc < 3){
		printf("%s\n",debug_netadd_help);
		return -1;
	}

	log_domain = argv[1];
	local_print = atoi(argv[2]);

	if( _log_handler_cnt >= MAX_LOG_HANDLER_CNT)
	{
		printf("log_handler_full[%d]\n", _log_handler_cnt);
		return -1;
	}

	for(i=0; i<	_log_handler_cnt; i++)
	{
		if( strcasecmp(log_domain, _log_handler[i].domain) == 0)
		{
			printf("log_handler already set id[%d]\n", _log_handler[i].handler_id);
			return -1;
		}
	}

	if(	strcasecmp(log_domain,"null" ) == 0 )
		log_domain = NULL;

	handler_id = g_log_set_handler (log_domain, G_LOG_LEVEL_MESSAGE|G_LOG_LEVEL_WARNING ,
								_net_send_log_hendler,(gpointer)local_print);

	g_static_mutex_lock (&_nf_debug_log_mutex);
	{
		strncpy(_log_handler[_log_handler_cnt].domain, argv[1],
					sizeof(_log_handler[_log_handler_cnt].domain) );

		_log_handler[_log_handler_cnt].handler_id = handler_id;
		++_log_handler_cnt;
	}
	g_static_mutex_unlock (&_nf_debug_log_mutex);

	printf("%s set domain[%s] handler_id[%d]\n", __FUNCTION__,
				(log_domain != NULL) ? log_domain:"null", handler_id);

	return 0;
}
__commandlist(debug_netadd,"debug_netadd", debug_netadd_help, debug_netadd_help);

static char debug_netoff_help[] = "debug_netoff [handler_id]";
static int debug_netoff(int argc, char **argv)
{
	guint	i;
	guint	found = 0;
	guint	handler_id = 0;

	if(argc < 2){
		printf("%s\n",debug_netoff_help);
		printf("log_handler_list[%d]===============\n", _log_handler_cnt);
		for(i=0;i<_log_handler_cnt;i++)
		{
			printf("domain[%-32s] handler_id[%2d]\n", _log_handler[i].domain,
						_log_handler[i].handler_id);
		}
		return -1;
	}

	handler_id = atoi(argv[1]);

	for(i=0; i<	_log_handler_cnt; i++)
	{
		if( _log_handler[i].handler_id == handler_id)
		{
			found = 1;
			break;
		}
	}

	if( found == 0)
	{
		printf("handle_id not found  id[%d]\n",handler_id);
		return -1;
	}else{

		g_log_remove_handler(_log_handler[i].domain, handler_id);
		printf("%s remove domain[%s] handler_id[%d]\n", __FUNCTION__,
				_log_handler[i].domain, handler_id);

		g_static_mutex_lock (&_nf_debug_log_mutex);
		{
			if( i != _log_handler_cnt-1)
			{
				memcpy( &_log_handler[i], &_log_handler[_log_handler_cnt-1],
							sizeof(LOG_DOMAIN_HANDLER));
			}
			--_log_handler_cnt;
		}
		g_static_mutex_unlock (&_nf_debug_log_mutex);
	}

	return 0;
}
__commandlist(debug_netoff,"debug_netoff", debug_netoff_help, debug_netoff_help);


#endif
