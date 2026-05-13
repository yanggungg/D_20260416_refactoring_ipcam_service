#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "nf_common.h"
#include "nf_util_time.h"

#include "proxy_cli.h"

#include <ctype.h>

#include "nf_util_parse_s1.h"

// test
#include "nf_api_ipcam.h"
#include "nf_util_fw.h"

typedef enum{
	START,
	INONEWORD,		
	INASSIGN,
	INSTRING,
	DONE
}StateType;

typedef enum {
	ENDFILE,ERROR,
	ONEWORD,STRING,
	ASSIGN,LPAREN,RPAREN,
} TokenType;

#define MAXTOKENLEN 1024
#define BUFLEN 256

TokenType getToken(void);
char tokenString[MAXTOKENLEN+1];

static char lineBuf[BUFLEN];
static int linepos = 0;
static int bufsize = 0;
static int EOF_flag = FALSE;
static int lineno = 0;
static FILE * source;
static TokenType token;

#define PF_MAX	NUM_ACTIVE_CH+1

static int pf_num = 0;
static TreeNode* pf_tree[PF_MAX];

static TreeNode * stmt_sequence(void);
static TreeNode * statement(void);
static TreeNode * assign_stmt(void);
static TreeNode * factor(void);

static void _getTokenMsg( char *buf, TokenType token, const char* tokenString )
{
	g_assert(buf);
	g_assert(tokenString);	
	
	switch (token)
	{ 	
		case ASSIGN: 
			sprintf(buf, "="); 
		break;

		case ONEWORD: 
			sprintf(buf, "WORD > %s", tokenString); 
		break;

		case STRING: 
			sprintf(buf, "STRING > %s", tokenString); 
		break;

		case LPAREN: 
			sprintf(buf, "{"); 
		break;

		case RPAREN: 
			sprintf(buf, "}"); 
		break;

		case ENDFILE: 
			sprintf(buf, "EOF"); 
		break;
		
		case ERROR:
			sprintf(buf, "ERROR: %s",tokenString);
		break;
		
		default:
			sprintf(buf, "Unknown token: %d\n",token);
		break;	
	}
}

static void _display_token_msg(TokenType token, const char* tokenString)
{
	char TokenMsg[MAXTOKENLEN+128];
	_getTokenMsg( TokenMsg, token, tokenString);
	g_message("Token => %s", TokenMsg);
}

static void _display_SyntaxError(void)
{	
	char TokenMsg[MAXTOKENLEN+128];
	_getTokenMsg( TokenMsg, token, tokenString);
	g_message("SyntaxError at line %d: %s", lineno, TokenMsg);
}

static int getNextChar(void)
{ 
	if (!(linepos < bufsize))
	{ 
		lineno++;

		if (fgets(lineBuf,BUFLEN-1,source))
		{		
			bufsize = strlen(lineBuf);
			linepos = 0;
			
			return lineBuf[linepos++];
		}
		else
		{ 
			EOF_flag = TRUE;
			
			return EOF;
		}
	}
	else 
		return lineBuf[linepos++];
}

static void ungetNextChar(void)
{ 
	if (!EOF_flag) 
		linepos-- ;
}

TokenType getToken(void)
{
	int tokenStringIndex = 0;
	TokenType currentToken;
	StateType state = START;
	int save;
	
	while (state != DONE)
	{ 
		int c = getNextChar();
		save = TRUE;
		
		switch (state)
		{ 
			case START:
				if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
					save = FALSE;				
				else if (c == '='){
					state = DONE;
					save = FALSE;
					currentToken = ASSIGN;
				}
				else if (c == '{')
				{
					state = DONE;
					save = FALSE;
					currentToken = LPAREN;					
				}
				else if (c == '}')
				{
					state = DONE;
					save = FALSE;					
					currentToken = RPAREN;
				}
				else if (c == '"'){
					state = INSTRING;
					save = FALSE;
				}				
				else if (isgraph(c))
					state = INONEWORD;				
				else
				{ 
					state = DONE;
					save = FALSE;
					
					switch (c)
					{
						case EOF:
							currentToken = ENDFILE;
						break;
												
						default:
							currentToken = ERROR;
						break;						
					}

				}
			break;

/*
			case INASSIGN:
				state = DONE;
				
				if (c == '=')
					currentToken = ASSIGN;
				else
				{
					ungetNextChar();
					currentToken = ERROR;
				}
			break;
*/
			case INSTRING:
				if ( c == '"' )
				{
					save = FALSE;
					state = DONE;
					currentToken = STRING;
				}
				else
					save = TRUE;
			break;

			case INONEWORD:
				if (!isgraph(c) || (c == '{') || (c == '}') || (c == '=') || (c == '"'))
				{
					ungetNextChar();
					save = FALSE;
					state = DONE;
					currentToken = ONEWORD;
				}
			break;
		
			case DONE:
			default:
				g_message("Scanner Bug: state= %d\n",state);
				state = DONE;
				currentToken = ERROR;
			break;
		}
		
		if ((save) && (tokenStringIndex <= MAXTOKENLEN))
			tokenString[tokenStringIndex++] = (char) c;
		
		if (state == DONE)
		{ 
			tokenString[tokenStringIndex] = '\0';
		}
	}

	if( currentToken == ERROR )
		g_message("TokenError at line : %d", lineno);

#if 0
	_display_token_msg(currentToken, tokenString);
#endif
	
	return currentToken;
}

TreeNode * newNode(NodeKind kind)
{ 
	TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
	int i;
	if (t==NULL)
		g_assert(0);
	else {
		t->child = NULL;		
		t->sibling = NULL;
		t->kind = kind;
		t->name = NULL;
		t->lineno = lineno;
	}
	return t;
}

static void match(TokenType expected)
{ 
	if (token == expected) 
		token = getToken();
	else
		_display_SyntaxError();
}

TreeNode * factor(void)
{ 	
	TreeNode * t = NULL;
	
	switch (token) {
		case ONEWORD :
		case STRING :			
			t = newNode(Value);
			
			if ((t!=NULL) && (token==ONEWORD || token==STRING))
				t->name = g_strdup(tokenString);

			match(token);
		break;

		case LPAREN :
			match(LPAREN);
			t = stmt_sequence();			
			match(RPAREN);			
		break;

		default:
			_display_SyntaxError();
			token = getToken();
		break;
	}
	
	return t;
}

TreeNode * assign_stmt(void)
{ 	
	TreeNode * t = newNode(Variable);
	
	if ((t!=NULL) && (token==ONEWORD))
		t->name = g_strdup(tokenString);
			
	match(ONEWORD);
	match(ASSIGN);
	
	if (t!=NULL) 
		t->child = factor();

	return t;
}

TreeNode * statement(void)
{ 	
	TreeNode * t = NULL;
	
	switch (token) {
		case ONEWORD : 
			t = assign_stmt(); 
		break;

		default :
			_display_SyntaxError();
			token = getToken();
		break;
	}
	
	return t;
}

TreeNode * stmt_sequence(void)
{ 	
	TreeNode * t = NULL;
	TreeNode * p = t;
	
	while ( token!=ENDFILE && token!=RPAREN )		
	{ 
		TreeNode * q;
	
		q = statement();
		
		if (q!=NULL) {
			if (t==NULL) t = p = q;
			else
			{ 
				p->sibling = q;
				p = q;
			}			
		}
	}
	return t;
}

void del_node(TreeNode *node)
{
	g_assert(node);
	
	if(node->name)
		free(node->name);

	free(node);
}

void del_sibling(TreeNode *node)
{
	g_assert(node);
	
	if(node->child)
		del_child(node->child);

	del_node(node);
}

void del_child(TreeNode *node)
{
	TreeNode *next;

	g_assert(node);
	
	next = node;
	
	while(next)
	{
		TreeNode *tmp;

		tmp = next->sibling;
		
		del_sibling(next);

		next = tmp;
	}	
}

void free_TreeNode(TreeNode *rootTree)
{
	g_assert(rootTree);
	
	del_child(rootTree);
}

TreeNode * new_TreeNodeFromFile(char *filename)
{ 
	TreeNode * t;
	char buf[4];

	g_assert(filename);	

	bufsize = 0;
	EOF_flag = FALSE;
	lineno = 0;
	linepos = 0;
	
	source = fopen(filename,"r");
	if (source==NULL)
	{ 
		g_message("File %s not found\n",filename);
		return NULL;
	}

	if( fgets( buf, sizeof(buf), source))
	{
		if( ((buf[0] & 0xff) == 0xef && (buf[1] & 0xff) == 0xbb && (buf[2] & 0xff) == 0xbf) == 0 )
		{
			fclose(source);
			return NULL;
		}
	}
	else
	{
		fclose(source);
		return NULL;
	}	

	token = getToken();	
	t = stmt_sequence();
	
	if (token!=ENDFILE)
		g_message("Syntax error : Code ends before file");		

	fclose(source);
	
	return t;
}

TreeNode* S1_GetSubTree(TreeNode* base_tree, char *cate, int idx)
{
	int n = 0;
	TreeNode *next;
	TreeNode *p = NULL;

	g_assert(base_tree);
	g_assert(cate);

	next = base_tree;

	while(next && n <= idx)
	{
		if( !strcmp(cate, next->name))
		{
			if(next->child)
			{
				if(next->child->kind == Variable)
				{
					if( n == idx ){
						p = next->child;
						break;
					}

					n++;
				}
			}
		}

		next = next->sibling;
	}	
	
	return p;
}

int S1_GetSubTreeNum(TreeNode* base_tree, char *cate)
{
	int n = 0;
	TreeNode *next;

	g_assert(base_tree);
	g_assert(cate);
	
	next = base_tree;
	
	while(next)
	{
		if( !strcmp(cate, next->name))
		{
			if(next->child)
			{
				if(next->child->kind == Variable)
					n++;
			}
		}

		next = next->sibling;
	}

	return n;
}

char* S1_GetItem(TreeNode* base_tree, char *item, int idx)
{
	int n = 0;
	TreeNode* next;
	char *p = NULL;

	g_assert(base_tree);
	g_assert(item);

	next = base_tree;

	while(next)
	{
		if( !strcmp(item, next->name))
		{
			if(next->child)
			{
				if(next->child->kind == Value)
				{
					if( n == idx ){
						p = next->child->name;
						break;
					}
					
					n++;
				}
			}
		}

		next = next->sibling;		
	}

	return p;
}

int S1_GetItemNum(TreeNode* base_tree, char *item)
{
	int n = 0;
	TreeNode* next;

	g_assert(base_tree);
	g_assert(item);	

	next = base_tree;

	while(next)
	{
		if( !strcmp(item, next->name))
		{
			if(next->child)
			{
				if(next->child->kind == Value)
					n++;
			}
		}

		next = next->sibling;		
	}

	return n;
}

TreeNode* S1_GetRootTreeByModel(char *model)
{
	TreeNode* sub;
	int i;
	char *val;

	g_assert(model);

	for(i=0; i < S1_GetRootTreeNum(); i++)
	{
		sub = S1_GetSubTree(pf_tree[i], "Product", 0);
		if(sub)
		{
			val = S1_GetItem(sub, "ID", 0);
			if(val)
			{
				if(!strcmp(val, model))
					return pf_tree[i];
			}
		}
	}
	
	return NULL;
}

int S1_init_RootTrees(void)
{
	int i;
	
	for(i = 0; i < pf_num; i++)	
		free_TreeNode(pf_tree[i]);

	pf_num = 0;	

	return 0;
}
	
int S1_GetRootTreeNum(void)
{
	return pf_num;
}

int S1_AddRootTree(TreeNode *rootTree)
{
	g_assert(rootTree);
	
	if(pf_num == PF_MAX)
		return 0;
	else
	{
		pf_tree[pf_num++] = rootTree;
		return 1;
	}
}

#define FILE_NAME "/NFDVR/profile.txt"

void sample_parse(void)
{
	TreeNode* syntaxTree;
	TreeNode* subTree;
	
	bufsize = 0;
	EOF_flag = FALSE;
	lineno = 0;
	linepos = 0;
		
#if 0
	source = fopen(FILE_NAME,"r");
	if (source==NULL)
	{ 
		g_message("File %s not found\n",FILE_NAME);
		return NULL;
	}

	token = getToken();
	while( token != ENDFILE && token != ERROR )
	{
		token = getToken();
	}

	fclose(source);
#else
	S1_init_RootTrees();

	pf_tree[0] = new_TreeNodeFromFile(FILE_NAME);
	pf_num = 1;


#if 0
	syntaxTree = S1_GetRootTreeByModel("VBR-9108(R)");

	g_message("ProfileVersion - %s", S1_GetItem(syntaxTree, "ProfileVersion", 0));

	subTree = S1_GetSubTree(syntaxTree, "Product", 0);

	g_message("Product/ID - %s", S1_GetItem(subTree, "ID", 0));

	subTree = S1_GetSubTree(syntaxTree, "Version", 0);
	
	g_message("Version/AllowdDeviceIDNum - %d", S1_GetItemNum(subTree, "AllowdDeviceID"));

	subTree = S1_GetSubTree(subTree, "ReleaseNote", 0);	

	g_message("Version/ReleaseNote/Link - %s", S1_GetItem(subTree, "Link", 0));

	NF_FW_NETWORK_S1_INFO model_info;
	NF_FW_NETWORK_S1_DETAIL detail;

	nf_fw_network_s1_get_cam_fw_info(0, &model_info);
	nf_fw_network_s1_get_detail_by_model_ver(model_info.model, model_info.new_ver, &detail);
	nf_fw_network_s1_get_cam_fw_info(1, &model_info);
	nf_fw_network_s1_get_detail_by_model_ver(model_info.model, model_info.new_ver, &detail);
	nf_fw_network_s1_get_cam_fw_info(2, &model_info);
	nf_fw_network_s1_get_detail_by_model_ver(model_info.model, model_info.new_ver, &detail);
#endif	

//	free_TreeNode(syntaxTree);
#endif
}
