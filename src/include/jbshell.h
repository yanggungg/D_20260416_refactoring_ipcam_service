#ifndef	_JBSHELL_H_
#define	_JBSHELL_H_

#if 0
#include <gnu/targets/std.h>
#include <xdc/std.h>

#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/utils/trace/TraceUtil.h>
#endif
//#define JBSHELL_TYPE_DEF_SIKP
#ifndef JBSHELL_TYPE_DEF_SIKP
typedef unsigned int Uint32;
typedef unsigned short Uint16;
typedef unsigned char Uint8;
#endif


#define	SHELL_EXIT			-12345
#define	SHELL_NOCMD			-23456
#define	SHELL_NOTSUPPORT	-34567

#define	MAX_CMD_HISTORY		32
#define	MAX_CMD_STRLEN		256		/* include null */
#define MAX_PRINTF_BUFF		(128*1024)

typedef struct _cmd_line_t
{
	char	*input[MAX_CMD_HISTORY];
	Uint32	index;
} cmd_line_t;

#define COMMAND_MAGIC (0x436d6420)  /* "Cmd " */

typedef int(*commandfunc_t)(int, char **);

typedef struct commandlist {
	Uint32 magic;
	char *name;
	char *description;
	char *help;
	commandfunc_t callback;
	struct commandlist *next;
} __attribute__((packed))  commandlist_t;


#define __command __attribute__((unused, __section__(".commandlist")))

#if 1
#define __commandlist(func, name, desc, help) \
    static commandlist_t __command_##func \
    __attribute__((__used__, section(".jbshell_cmd"), aligned(1))) = \
    { COMMAND_MAGIC, name, desc, help, func, NULL }
#else
#define __commandlist(fn, nm, desc, hlp)	\
static commandlist_t __command_##fn __command = {	\
	magic:		COMMAND_MAGIC,	\
	name:		nm,				\
	description:	desc,		\
	help:		hlp,			\
	callback:	fn }
#endif	

void jbshell(void);
int kbhit(void);
int readch(void);
int do_shell_cmd(int argc, char **argv);

int my_command_slient(const char *string);
int my_command(const char *string);

/*20110128 added by ugie*/
/* network supported shell */
void ipxshell(int port);
int ipx_printf(const char *format, ...);
int ipx_printf_large(const char *format, ...);

#endif	/* _JBSHELL_H_ */

