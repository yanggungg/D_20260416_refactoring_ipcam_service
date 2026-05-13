/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2007 Intellix                                                 *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
12/17/2007 Jongbin Yim    Created.
01/28/2011 Donguk Park    Network Shell Supported.

................................................................................

DESCRIPTION:

  This file contains implementations for jbshell base functions.
  The codes were originally generated for the HTL project, by the same author.

................................................................................
*/

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/socket.h> 
#include <sys/stat.h> 
#include <arpa/inet.h> 
#include "jbshell.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

const char *shell_version = "09.18.2011.1.28";

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static commandlist_t *commands = NULL;
static void init_commands(void);

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

/*
 * ugie add 20110128
 */
char (*shell_getch)(void) = getchar;
int (*shell_printf)(const char *format, ...) = printf;
static void init_commands();

#define MAXBUF  256 

static int      scon = -1;
static int		_is_error = 0;
static int		_is_fw_mode = 0;
static char		_jb_passwd[128] = {0,};

#define         INMAX  32
static char     _inBuf[INMAX];
static int      _inIdx = 0;
static int      _inCnt = 0;

int ipx_printf(const char *format, ...)
{
   va_list ap;
   char    buffer[256];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);
   send( scon, buffer, size, 0 );
   return( size );
}

int ipx_printf_large(const char *format, ...)
{
   va_list ap;
   char    buffer[MAX_PRINTF_BUFF];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);
   send( scon, buffer, size, 0 );
   return( size );
}


char ipx_getchar()
{
    char   c;
    struct timeval timeout;

	//console time out: 20 minutes
    timeout.tv_sec  = 5 * 60;
    timeout.tv_usec = 0;

    while( 1 )
    {
        while( !_inCnt )
        {
            fd_set ibits;
            int    cnt;

            FD_ZERO(&ibits);
            FD_SET(scon, &ibits);

            // Wait for input
            cnt = select( scon+1, &ibits, 0, 0, &timeout );
            if( !cnt )
                goto abort_console;
                
            if( FD_ISSET(scon, &ibits) )
            {
                cnt = (int)recv( scon, _inBuf, INMAX, 0 );
                if( cnt > 0 )
                {
                    _inIdx = 0;
                    _inCnt = cnt;
                }
                if( !cnt || (cnt<0 && errno!=EWOULDBLOCK) )
                    goto abort_console;
            }
        }

        _inCnt--;
        c = _inBuf[_inIdx++];

        if( c != '\n' )
            return( c );
    }
    
abort_console:
	_is_error = 1;
    //close( scon );
    return( 0 );
}

void ipxshell(int port)
{
    int server_sockfd, client_sockfd; 
    int client_len, n; 
    struct sockaddr_in clientaddr, serveraddr; 
    int yes;    
        
	init_commands();

	if( port == 0 ) {
		_is_fw_mode = 1;
		port = 773;
	}
	
    shell_printf = ipx_printf;
    shell_getch = ipx_getchar;

	while(1)
	{
		struct stat buf;			
		if( _is_fw_mode && stat( "/NFDVR/data/debug", &buf ) != 0 )
			sleep(60);		
		else
			break;
	}

    client_len = sizeof(clientaddr); 

   	if( (server_sockfd = socket( AF_INET, SOCK_STREAM, 0) ) < 0) 
    	perror("socket error : ");
   	    
    yes=1;      // for setsockopt() SO_REUSEADDR, below
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
    }

    bzero(&serveraddr, sizeof(serveraddr)); 

   	serveraddr.sin_family = AF_INET; 
   	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);     	     
    serveraddr.sin_port = htons( port ); 
        
    bind(server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); 
    listen(server_sockfd, 5); 

    while(1) 
    { 
    	_is_error = 0;
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);     
        scon = client_sockfd;
        jbshell();        
        close(client_sockfd); 
    } 
}
/* ********************************* */

static char help_help[] = "help [command]\r\nGet help on [command], or a list of supported commands if a command is omitted.";
static int
jbshell_help(int argc, char *argv[])
{
	commandlist_t *cmd;

	if ( argc >= 2 ) {
		for (cmd = commands; cmd; cmd = cmd->next) {
			if( !strncmp(cmd->name, argv[1], 128) ) {
				ipx_printf("Help for '%s': %s\r\n Usage: %s\r\n", argv[1], cmd->description, cmd->help);
				return 0;
			}
		}
		ipx_printf("Invalid command: '%s'.\r\n", argv[1]);
		return -EINVAL;
	}

	ipx_printf("<jb shell ver %s>\r\n", shell_version);
	ipx_printf("Supported shell commands:\r\n");
	for (cmd = commands; cmd; cmd = cmd->next)
		ipx_printf(" - %-24.24s:  %s\r\n", cmd->name, cmd->description);

	ipx_printf("\r\nUse \"help command\" to get help on a specific command.\r\n");
	return 0;
}
__commandlist(jbshell_help, "help", "Help command", help_help);

static int
compare_commandlist(commandlist_t *c1, commandlist_t *c2)
{
	return strcmp(c1->name, c2->name);
}

static void
init_commands(void)
{
	Uint32 i;
    commandlist_t *lastcommand;
    commandlist_t *cmd, *next_cmd;

	if ( commands )
		return;

	lastcommand = &__command_jbshell_help;
	for (cmd = &__command_jbshell_help, i = 0 ; i < 128; i++, commands = cmd--)
		if ( cmd->magic != COMMAND_MAGIC )
			break;
	for (cmd = &__command_jbshell_help, i = 0 ; i < 128; i++, lastcommand = ++cmd)
		if ( cmd->magic != COMMAND_MAGIC )
			break;

    /* Make a sorted list. */
	qsort(commands, lastcommand - commands, sizeof(commandlist_t),
			(int (*)(const void *, const void *))compare_commandlist);

    for (cmd = commands, next_cmd = commands + 1; next_cmd < lastcommand; cmd++, next_cmd++)
        cmd->next = next_cmd;
}

static void
print_prompt(void)
{
	ipx_printf("jbsh:> ");
}

static void
get_cmd_string(cmd_line_t *cl, Uint32 len)
{
	char* rbuf;
	char *p = cl->input[cl->index], *cmd_base = cl->input[cl->index];
	Uint8 insert = 1, in, seq_char[5];
	Uint32 end = 0, prev_cmd_index = cl->index, control_seq = 0;
	int i;

	memset(p, 0, MAX_CMD_STRLEN);
	while ( !end ) {
		in = ipx_getchar();
		if(_is_error)
			break;

		if ( control_seq == 4 ) {
			switch ( in ) {
				case 0x7E:
					switch ( seq_char[3] ) {
						case 0x37:	/* F6 key */
						case 0x38:	/* F7 key */
						case 0x39:	/* F8 key */
							break;
					}
			}
			control_seq = 0;
		}
		else if ( control_seq == 3 ) {
			switch ( in ) {
				case 0x37:
				case 0x38:
				case 0x39:
					seq_char[control_seq++] = in;
					break;
				case 0x41:	/* F1 key */
				case 0x42:	/* F2 key */
				case 0x43:	/* F3 key */
				case 0x44:	/* F4 key */
				case 0x45:	/* F5 key */
					break;
				case 0x7E:	/* sequence end */
					switch ( seq_char[2] ) {
						case 0x32:	/* insert key */
							insert ^= 1;
							break;
						case 0x31:	/* home key */
							ipx_printf("%s", p);
							for (i = strlen(cmd_base); i > 0; i--)
								ipx_printf("%c", 0x08);
							p = cmd_base;
							break;
						case 0x35:	/* page up key */
							break;
						case 0x33:	/* delete key */
							if ( *p ) {
								*p = '\0';
								strcat(cmd_base, p + 1);
								ipx_printf("%s ", p);
								for (i = strlen(p); i >= 0; i--)
									ipx_printf("%c", 0x08);
							}
							break;
						case 0x34:	/* end key */
							ipx_printf("%s", p);
							p += strlen(p);
							break;
						case 0x36:	/* page down key */
							break;
					}
					break;
			}
			if ( control_seq == 3 )
				control_seq = 0;
		}
		else if ( control_seq == 2 ) {
			switch ( in ) {
				case 0x32:
				case 0x31:
				case 0x35:
				case 0x33:
				case 0x34:
				case 0x36:
				case 0x5B:
					seq_char[control_seq++] = in;
					break;
				case 0x41:	/* up key */
					/* Previous command line history. */
					if ( ((prev_cmd_index + MAX_CMD_HISTORY - 1) % MAX_CMD_HISTORY) != cl->index &&
							strlen(cl->input[(prev_cmd_index + MAX_CMD_HISTORY - 1) % MAX_CMD_HISTORY]) ) {
						ipx_printf("%s", p);			/* goto last character */
						/* Delete previous command line. */
						for (i = strlen(cmd_base); i > 0; i--)
							ipx_printf("%c%c%c", 0x08, ' ', 0x08);
						prev_cmd_index = (prev_cmd_index + MAX_CMD_HISTORY - 1) % MAX_CMD_HISTORY;
						cmd_base = cl->input[prev_cmd_index];
						p = cmd_base + strlen(cmd_base);
						ipx_printf("%s", cmd_base);		/* print new command line */
					}
					break;
				case 0x42:	/* down key */
					/* Next command line history. */
					if ( prev_cmd_index != cl->index ) {
						ipx_printf("%s", p);			/* goto last character */
						/* Delete previous command line. */
						for (i = strlen(cmd_base); i > 0; i--)
							ipx_printf("%c%c%c", 0x08, ' ', 0x08);
						prev_cmd_index = (prev_cmd_index + 1) % MAX_CMD_HISTORY;
						cmd_base = cl->input[prev_cmd_index];
						p = cmd_base + strlen(cmd_base);
						ipx_printf("%s", cmd_base);		/* print new command line */
					}
					break;
				case 0x43:	/* right key */
					if ( *p )
						ipx_printf("%c", *p++);
					break;
				case 0x44:	/* left key */
					if ( p > cmd_base ) {
						p--;
						ipx_printf("%c", 0x08);
					}
					break;
			}
			if ( control_seq == 2 )
				control_seq = 0;
		}
		else if ( control_seq == 1 ) {
			switch ( in ) {
				case 0x5B:
					seq_char[control_seq++] = in;
					break;
				default:
					control_seq = 0;
			}
		}
		else
		{
			switch ( in ) {
				case 0x00:
					break;
				case 0x01:		/* ctrl-A */
				case 0x02:		/* ctrl-B */
				case 0x03:		/* ctrl-C */
				case 0x04:		/* ctrl-D */
				case 0x05:		/* ctrl-E */
				case 0x06:		/* ctrl-F */
				case 0x07:		/* ctrl-G */
					break;
				case 0x08:		/* back space */
				case 0x7F:
					if ( p > cmd_base ) {
						*(p - 1) = '\0';
						strcat(cmd_base, p--);
						ipx_printf("%c%s ", 0x08, p);
						for (i = strlen(p); i >= 0; i--)
							ipx_printf("%c", 0x08);
					}
					break;
				case 0x09:		/* tab key, ctrl-I */
					{
						char *tok, *ccopy = strdup(cmd_base);

						for (i = 0, tok = strtok_r(ccopy, " \t", &rbuf ); tok; i++)
							tok = strtok_r(NULL, " \t", &rbuf);
						i = (strchr(cmd_base, ' ') && i <= 2 && !strcmp(ccopy, "help") &&
								strchr(cmd_base, ' ') == strrchr(cmd_base, ' '));
						free(ccopy);
					}
					if ( !strchr(cmd_base, ' ') || i ) {
						char *m, *cc;
						Uint32 match, l, found;
						commandlist_t *cmd, *pcmd = NULL;

						cc = i ? (strchr(cmd_base, ' ') + 1) : cmd_base;
						l = strlen(cc);

						for (match = 0, cmd = commands; cmd; cmd = cmd->next) {
							if ( !strncmp(cc, cmd->name, l) ) {
								pcmd = cmd;
								match++;
							}
						}
						if ( match > 1 ) {
							for (m = &pcmd->name[l], found = 0; ; found++) {
								for (i = 0, cmd = commands; cmd; cmd = cmd->next)
									if ( !strncmp(pcmd->name, cmd->name, m - pcmd->name + 1) )
										i++;
								if ( i != match )
									break;
								ipx_printf("%c", *p++ = *m++);
							}
							if ( !found ) {
								for (match = 0, cmd = commands; cmd; cmd = cmd->next) {
									if ( !strncmp(cc, cmd->name, l) ) {
										if ( !(match++ % 4) )
											ipx_printf("\r\n");
										ipx_printf("%-20s", cmd->name);
									}
								}
								ipx_printf("\r\n");
								print_prompt();
								ipx_printf("%s", cmd_base);
							}
						}
						else if ( match == 1 ) {
							for (m = &pcmd->name[l]; *m; )
								ipx_printf("%c", *p++ = *m++);
							ipx_printf("%c", *p++ = ' ');
						}
					}
					break;
				case '\n':		/* 0x0A, ctrl-J */
				case '\r':		/* 0x0D, ctrl-M */
					if ( prev_cmd_index != cl->index )
						strcpy(cl->input[cl->index], cl->input[prev_cmd_index]);
					end = 1;
					ipx_printf("\r\n");
					break;
				case 0x0B:		/* ctrl-K */
				case 0x0C:		/* ctrl-L */
				case 0x0E:		/* ctrl-N */
				case 0x0F:		/* ctrl-O */
				case 0x10:		/* ctrl-P */
				case 0x11:		/* ctrl-Q */
				case 0x12:		/* ctrl-R */
				case 0x13:		/* ctrl-S */
				case 0x14:		/* ctrl-T */
				case 0x15:		/* ctrl-U */
				case 0x16:		/* ctrl-V */
				case 0x17:		/* ctrl-W */
				case 0x18:		/* ctrl-X */
				case 0x19:		/* ctrl-Y */
				case 0x1A:		/* ctrl-Z */
					break;
				case 0x1B:		/* ctrl-[ */
					seq_char[control_seq++] = in;
					break;
				case 0x1C:		/* ctrl-\ */
				case 0x1D:		/* ctrl-] */
				case 0x1E:		/* ctrl-^ */
				case 0x1F:		/* ctrl-_ */
					break;
				case 0xE0:
					break;
				default:		/* Normal ascii codes. */
					if ( strlen(cmd_base) < len ) {
						if ( insert ) {
							for (i = strlen(p); i > 0; i--)
								p[i] = p[i - 1];
							*p++ = in;
#if 0
							ipx_printf("%c%s", in, p);
							for (i = strlen(p); i > 0; i--)
								ipx_printf("%c", 0x08);
#else
							ipx_printf("%s", p);
							for (i = strlen(p); i > 0; i--)
								ipx_printf("%c", 0x08);
#endif
						}
						else
                        {
#if 0
							ipx_printf("%c", *p++ = in);
#else
                            *p++ = in;
#endif
                        }
					}
					break;
			}
		}
		fflush(stdout);
	}
}

static void
_get_arg_arc(char *cmdstr, char *buf, int *argc, char **argv)
{
	char *p, *q;
	int _argc, ignore_sp;

	for (p = cmdstr, q = buf, ignore_sp = 0, _argc = 0; ; p++)
	{
		if ( *p == '"' )
			ignore_sp ^= 1;
		else if ( *p == ' ' ) {
			if ( ignore_sp )
				*q++ = *p;
			else {
				*q = '\0';
				if ( strlen(buf) ) {
					if ( argv )
						argv[_argc] = strdup(buf);
					_argc++;
				}
				q = buf;
			}
		}
		else if ( *p == '\0' ) {
			*q = '\0';
			if ( strlen(buf) ) {
				if ( argv )
					argv[_argc] = strdup(buf);
				_argc++;
			}
			break;
		}
		else
			*q++ = *p;
	}
	if ( argc )
		*argc = _argc;
}

static void
get_arg(char *cmdstr, int *argc, char ***argv)
{
	char temp[128];
	int _argc;

	_get_arg_arc(cmdstr, temp, &_argc, NULL);		/* first get arguments count */
	*argv = (char **)malloc(sizeof(char *) * _argc);
	_get_arg_arc(cmdstr, temp, NULL, *argv);		/* get argumnets */

	*argc = _argc;
}

static void
free_arg(int *argc, char ***argv)
{
	int i;

	for (i = 0; i < *argc; i++) {
		if ( (*argv)[i] ) {
			free( (*argv)[i] );
			(*argv)[i] = NULL;
		}
	}
	if ( *argv ) {
		free(*argv);
		*argv = NULL;
	}
	*argc = 0;
}

static void
init_cmd_line(cmd_line_t *c)
{
	Uint32 i;

	for (i = 0; i < MAX_CMD_HISTORY; i++) {
		c->input[i] = (char *)malloc(MAX_CMD_STRLEN);
		c->input[i][0] = '\0';
	}
	c->index = 0;
}

static void
free_cmd_line(cmd_line_t *c)
{
	Uint32 i;

	for (i = 0; i < MAX_CMD_HISTORY; i++)
		free(c->input[i]);
}

static void
get_cmd(cmd_line_t *cl, int *argc, char ***argv)
{
	get_cmd_string(cl, MAX_CMD_STRLEN - 1);
	free_arg(argc, argv);
	get_arg(cl->input[cl->index], argc, argv);

	if ( *argc )
		cl->index = (cl->index + 1) % MAX_CMD_HISTORY;
}

int do_shell_cmd(int argc, char **argv)
{
	commandlist_t *cmd = commands;

	for ( ; cmd; cmd = cmd->next) {
		if ( strcmp(argv[0], cmd->name) == 0 )
			break;
	}
	if ( cmd ) {
		if ( cmd->callback )
			return cmd->callback(argc, argv);
		ipx_printf("%s: command not yet supported.\r\n", argv[0]);
		return SHELL_NOTSUPPORT;
	}
	return SHELL_NOCMD;
}

static struct termios initial_settings, new_settings;

static void
set_terminal(void)
{
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	//new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

static void
restore_terminal(void)
{
	tcsetattr(0, TCSANOW, &initial_settings);
}

static int peek_character = -1;

int
kbhit(void)
{
	char ch;
	int nread;

	if ( peek_character != -1 )
		return 1;
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
	if ( nread == 1 ) {
		peek_character = ch;
		return 1;
	}
	return 0;
}

int
readch(void)
{
	char ch;

	if ( peek_character != -1 ) {
		ch = peek_character;
		peek_character = -1;
		return ch;
	}
	read(0, &ch, 1);
	return ch;
}

static char exit_help[] = "exit";
static int
jbshell_exit(int argc, char **argv)
{
	return SHELL_EXIT;
}
__commandlist(jbshell_exit, "exit", "Terminate jbshell", exit_help);


static char passwd_help[] = "passwd [passwd]";
static int
jbshell_passwd(int argc, char **argv)
{

	if(argc < 2){
		printf("%s\n",passwd_help);
		return -1;
	}	
	strncpy( _jb_passwd, argv[1], sizeof(_jb_passwd)-1);
	
	return 0;		
}
__commandlist(jbshell_passwd, "passwd", "login jbshell", passwd_help);


void
jbshell(void)
{
	int argc = 0, res;
	char **argv = NULL;
	cmd_line_t cmd_line;

	init_commands();
	init_cmd_line(&cmd_line);
	set_terminal();
	ipx_printf("Welcome to jbshell!\r\n");
	
	while (1) {
		print_prompt();
		get_cmd(&cmd_line, &argc, &argv);
		
		if(_is_error) {			
			sleep(5); break;
		}
	
		if( _is_fw_mode )  {
			struct stat buf;			
			if( stat( "/NFDVR/data/debug", &buf ) != 0 )
			{
				_is_error = 1; sleep(5); break;				
			}			
		}
		
		if ( argc ) {

			if( _is_fw_mode && strcmp( argv[0], "passwd") != 0 )
			{			
				if( strncmp("itxjbshell3398", _jb_passwd, 14 ) != 0 )
				{
					_is_error = 1; sleep(5); break;
				}
			}

			res = do_shell_cmd(argc, argv);
			if ( res == SHELL_NOCMD )
				ipx_printf("%s: command not found\r\n", argv[0]);
			else if ( res == SHELL_EXIT )
				break;
		}
	}
	free_arg(&argc, &argv);
	free_cmd_line(&cmd_line);

	restore_terminal();
}

