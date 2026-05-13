#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
//#include <locale.h>
#include <X11/Xlocale.h>
#include "multi_language_support.h"
#include "nf_ui_font.h"
#include "vw.h"
#include "ix_conf.h"
#include "support/color_conf.h"
#include "ix_mem.h"


//#include "nfdal.h"

#define NF_MULTI_LANG_STRING "/NFDVR/data/lang/nf_multilang_string_utf8.txt" 



#if 0
static LANGUAGE_SELECT selected_language = LANGUAGE_NONE;
static char selected_encoding[32];
#endif
static GHashTable* table = NULL;
static int init_completed = 0;

//static LANGUAGE_SELECT (*get_language_enum_by_vendor)(char* lang);
//static LANGUAGE_SELECT get_language_enum_by_ITX(char* str);
//static LANGUAGE_SELECT get_language_enum_by_ITX2(char* str);			 // for test



static int _getline(FILE * f, char *str, size_t size, size_t * len)
{
	char c;
	size_t i;
	i = 0;
	while (i + 1 < size) {
		if (fread(&c, sizeof(char), 1, f) == 1) {
			if(i > 0 && ('\\'==str[i-1]) && ('n' == c) )	{
				// it's for taking care of the key which has "\\n" 
				// in case key having "\\n" 
				str[i-1] = '\n';
			} else {
				str[i++] = c;
				if ('\n' == c) {
					break;
				}
			}
		} else {
			if (ferror(f)) {
				return -1;
			} else {    /* EOF */
				break;
			}
		}
	}
	str[i] = '\0';
	*len = i;
	return 0;
}

static int _get_lang_idx(char* str, char *lang_alias)
{
	int i = LANGUAGE_ENGLISH;
	int idx = LANGUAGE_ENGLISH;
	char** items = NULL;

	items = g_strsplit(str, "\t", 0);
	if(items == NULL) return idx;

	while (items[i]) {
		if (strcmp(lang_alias, items[i]) == 0) { 
			idx = i; 
			break;
		}
		++i;
	}
	g_strfreev(items);

	return idx;
}

static int parse_line(char* str, unsigned int key_index, unsigned int value_index, char* key, char* value)	{
	char** items = NULL;
	if(key_index >= value_index)	{
		return -1;
	}

//	items = g_strsplit(str, "\t", value_index);		// is it more efficient ? or not
	items = g_strsplit(str, "\t", 0);
	if(items != NULL && items[key_index] != NULL && items[value_index] != NULL)	{
		g_stpcpy(key, items[key_index]);
		g_stpcpy(value, items[value_index]);
	//	printf("%s %s\n", key, value);
	} else {
		if(items != NULL)	{
			g_strfreev(items);
		}
		return -1;
	}
	g_strfreev(items);

	return 0;
}

#define TEXTDB_LENGTH		(5120 * 4)

// make_table 
static int parse_file(FILE* fp, int lang)	{
	char *buf;
	char key[1024];
	char value[1024];
	size_t len = 0;
	int ret = 0;

	buf = imalloc(TEXTDB_LENGTH);
	memset(key, 0, 1024);
	memset(value, 0, 1024);

	g_assert(table);

	while(1)	{
		len = 0;
		memset(buf, 0, TEXTDB_LENGTH);
		ret = _getline(fp, buf, TEXTDB_LENGTH, &len); 
		if(0 == ret)	{
			if(0 == len)	{	// end of file
				break;
			}
			else if (len >= TEXTDB_LENGTH - 2) {
				g_warning("ERROR: Too many characters in language table\n");
			}
		} else {				// error 
			g_warning("parse_file() : getline error\n");
			ret = -1;
			break;
		}
		ret = parse_line(buf, LANGUAGE_KEY, lang, key, value);
		if(ret != 0)	{
			printf("parse_file() : parse_line error at below line : \n");
			printf("\t[%s]\n", buf);
			ret = -2;
			break;
		} else {
		//	printf("[%s]\t\t[%s]\n", key, value);
			g_hash_table_insert(table, g_strdup(key), g_strdup(value));
#if 0	// for testing... 
			char* r = (char*)g_hash_table_lookup(table, key);
			if(r != NULL)	{
				printf("[%s]\t\t[%s]\n", key, r);
			} else {
				printf("fail\n");
			}
#endif
		}
	}

	ifree(buf);	
	return ret;
}

#if 0	// Don't need. By hakeya. 2009-12-23
static int select_encoding(LANGUAGE_SELECT lang)
{
	int ret = 0;
	nffont_charset_type charset_type = NFFONT_CHARSET_ISO8859_1;
	memset(selected_encoding, 0, sizeof(selected_encoding));
	switch(lang)	{
		case LANGUAGE_ENGLISH:
			setlocale(LC_ALL, 			"en_EN.ISO8859-1");
			g_stpcpy(selected_encoding,	"ISO8859-1");
			break;
		case LANGUAGE_KOREAN:	
			setlocale(LC_ALL,		  	"ko_KR.eucKR");
			g_stpcpy(selected_encoding, 	"eucKR");
			charset_type = NFFONT_CHARSET_GULIIM;
			break;
		case LANGUAGE_ITALIAN:	
			setlocale(LC_ALL,			"it_IT.ISO8859-1");
			g_stpcpy(selected_encoding,	"ISO8859-1");
			break;
		case LANGUAGE_SPANISH:	
			setlocale(LC_ALL,			"es_ES.ISO8859-1");
			g_stpcpy(selected_encoding,	"ISO8859-1");
			break;
		case LANGUAGE_FRENCH:	
			setlocale(LC_ALL,			"fr_FR.ISO8859-1");
			g_stpcpy(selected_encoding,	"ISO8859-1");
			break;
		case LANGUAGE_GERMAN:	
			setlocale(LC_ALL,			"de_DE.ISO8859-1");
			g_stpcpy(selected_encoding,	"ISO8859-1");
			break;
//		case LANGUAGE_PORTUGUESE:
//			setlocale(LC_ALL,			"pt_PT.ISO8859-1");
//			g_stpcpy(selected_encoding,	"ISO8859-1");
//			break;
		case LANGUAGE_RUSSIAN:	
			g_stpcpy(selected_encoding,	"ISO8859-5");
			setlocale(LC_ALL,			"ru_RU.ISO8859-5");
			charset_type = NFFONT_CHARSET_ISO8859_5;
			break;
		case LANGUAGE_POLISH:	
			g_stpcpy(selected_encoding,	"ISO8859-2");
			setlocale(LC_ALL,			"pl_PL.ISO8859-2");
			charset_type = NFFONT_CHARSET_ISO8859_2;
			break;
		case LANGUAGE_TURKISH:
			g_stpcpy(selected_encoding,	"ISO8859-9");
			setlocale(LC_ALL,			"tr_TR.ISO8859-9");
			charset_type = NFFONT_CHARSET_ISO8859_9;
			break;
		case LANGUAGE_THAI:
			g_stpcpy(selected_encoding,	"ISO8859-11");
			setlocale(LC_ALL,			"th_TH.ISO8859-11");
			break;
		case LANGUAGE_JAPANESE:
			g_stpcpy(selected_encoding,	"ISO8859-11");
			setlocale(LC_ALL,			"ja_JP.eucJP");
			break;
		case LANGUAGE_CHINESE_S:
			g_stpcpy(selected_encoding,	"ISO8859-11");
			setlocale(LC_ALL,			"ja_JP.eucJP");
//			setlocale(LC_ALL,			"chinese-simplified");
			break;
		case LANGUAGE_CHINESE_T:
			g_stpcpy(selected_encoding,	"ISO8859-11");
			setlocale(LC_ALL,			"ja_JP.eucJP");
//			setlocale(LC_ALL,			"chinese-simplified");
			break;
//		case LANGUAGE_CZECH:	
//		case LANGUAGE_CHINESE:	
//			// don't know yet ~ 
//			g_stpcpy(selected_encoding, "ISO8859-1");
//			break;
		default:
			ret = -1;
			break;
	}
	if(init_nffont(charset_type) != 0)	{
		ret = -1;
	}
	return ret;
}
#endif

#if 0
LANGUAGE_SELECT get_language_enum(char* str)	{
	LANGUAGE_SELECT lang = LANGUAGE_ENGLISH;
	if			(	!strcmp(str, "ENGLISH") )	{
		lang = LANGUAGE_ENGLISH;
	} else if	(	!strcmp(str, "KOREAN") )	{
		lang = LANGUAGE_KOREAN;
	} else if	(	!strcmp(str, "ITALIAN") )	{
		lang = LANGUAGE_ITALIAN;
	} else if	(	!strcmp(str, "SPANISH") )	{
		lang = LANGUAGE_SPANISH;
	} else if	(	!strcmp(str, "FRENCH") )	{
		lang = LANGUAGE_FRENCH;
	} else if	(	!strcmp(str, "GERMAN") )	{
		lang = LANGUAGE_GERMAN;
	} else if	(	!strcmp(str, "RUSSIAN") )	{
		lang = LANGUAGE_RUSSIAN;
	} else if	(	!strcmp(str, "POLISH") )	{
		lang = LANGUAGE_POLISH;
	} else if	(	!strcmp(str, "TURKISH") )	{
		lang = LANGUAGE_TURKISH;
	} else if	(	!strcmp(str, "THAI") )	{
		lang = LANGUAGE_THAI;
	} else if	(	!strcmp(str, "JAPANESE") )	{
		lang = LANGUAGE_JAPANESE;
	} else if	(	!strcmp(str, "CHINESE(S)") )	{
		lang = LANGUAGE_CHINESE_S;
	} else if	(	!strcmp(str, "CHINESE(T)") )	{
		lang = LANGUAGE_CHINESE_T;
	} else if	(	!strcmp(str, "PORTUGUESE PT") )	{
		lang = LANGUAGE_PORTUGUESE;		
	} else if	(	!strcmp(str, "PORTUGUESE BR") )	{
		lang = LANGUAGE_BRAZIL;
	} else if	(	!strcmp(str, "DUTCH") )	{
		lang = LANGUAGE_DUTCH;		
	} else if	(	!strcmp(str, "GREEK") )	{
		lang = LANGUAGE_GREEK;		
	} else if	(	!strcmp(str, "BULGARIAN") )	{
		lang = LANGUAGE_BULGARIAN;		
	}
	else {
		g_warning("get_language_enum use default language(ENGLISH)");
		lang = LANGUAGE_ENGLISH;
	}
	return lang;
}
#endif
/*
static LANGUAGE_SELECT get_language_enum_by_ITX(char* str)
{
	LANGUAGE_SELECT lang = LANGUAGE_ENGLISH;
	if			(	!strcmp(str, "ENGLISH") )	{
		lang = LANGUAGE_ENGLISH;
	} else if	(	!strcmp(str, "KOREAN") )	{
		lang = LANGUAGE_KOREAN;
	} else if	(	!strcmp(str, "ITALIAN") )	{
		lang = LANGUAGE_ITALIAN;
	} else if	(	!strcmp(str, "SPANISH") )	{
		lang = LANGUAGE_SPANISH;
	} else if	(	!strcmp(str, "FRENCH") )	{
		lang = LANGUAGE_FRENCH;
	} else if	(	!strcmp(str, "GERMAN") )	{
		lang = LANGUAGE_GERMAN;
	} else if	(	!strcmp(str, "RUSSIAN") )	{
		lang = LANGUAGE_RUSSIAN;
	} else if	(	!strcmp(str, "POLISH") )	{
		lang = LANGUAGE_POLISH;
	} else if	(	!strcmp(str, "TURKISH") )	{
		lang = LANGUAGE_TURKISH;
	} else if	(	!strcmp(str, "THAI") )	{
		lang = LANGUAGE_THAI;
	} else if	(	!strcmp(str, "JAPANESE") )	{
		lang = LANGUAGE_JAPANESE;
	} else if	(	!strcmp(str, "CHINESE(S)") )	{
		lang = LANGUAGE_CHINESE_S;
	} else if	(	!strcmp(str, "CHINESE(T)") )	{
		lang = LANGUAGE_CHINESE_T;
	} else if	(	!strcmp(str, "PORTUGUESE PT") )	{
		lang = LANGUAGE_PORTUGUESE;		
	} else if	(	!strcmp(str, "PORTUGUESE BR") )	{
		lang = LANGUAGE_BRAZIL;
	} else if	(	!strcmp(str, "DUTCH") )	{
		lang = LANGUAGE_DUTCH;		
	} else if	(	!strcmp(str, "GREEK") )	{
		lang = LANGUAGE_GREEK;		
	} else if	(	!strcmp(str, "BULGARIAN") )	{
		lang = LANGUAGE_BULGARIAN;		
	}
	else {
		g_warning("get_language_enum use default language(ENGLISH)");
		lang = LANGUAGE_ENGLISH;
	}
	return lang;
}

// for example
static LANGUAGE_SELECT get_language_enum_by_ITX2(char* str)
{
	LANGUAGE_SELECT lang = LANGUAGE_ENGLISH;
	if			(	!strcmp(str, "ENGLISH") )	{
		lang = LANGUAGE_ENGLISH;
	} else if	(	!strcmp(str, "KOREAN") )	{
		lang = LANGUAGE_KOREAN;
	} else if	(	!strcmp(str, "JAPANESE") )	{
		lang = LANGUAGE_KOREAN;
	} else {
		g_warning("get_language_enum use default language(ENGLISH)");
		lang = LANGUAGE_ENGLISH;
	}
	return lang;
}
*/

/*
static void init_get_language_enum_func()
{
	get_language_enum_by_vendor = get_language_enum_by_ITX; 

	// for example
#if 0
	switch(get_ui_type()) {
		case 32: 	// cbc
		case 200: 	// itx2
		case 100: 	// itx1
			get_language_enum_by_vendor = get_language_enum_by_ITX; 
			break;

		default:
			g_assert(0);
			break;
	}
#endif
}
*/

static gboolean _is_exist_language_list(gchar *lang)
{
	char strLang[64];
	char strLangAlias[64];
	int langCnt = 0;
	int i;
	
	memset(strLang, 0x00, sizeof(strLang));	
	memset(strLangAlias, 0x00, sizeof(strLangAlias));
	langCnt = DAL_get_support_lang_cnt();
		
	for (i = 0; i < langCnt; ++i) 
	{
		DAL_get_support_lang(i, strLang);

		if (strcmp(strLang, lang) == 0) break;
		else if(i == langCnt-1) return FALSE;
	}
	DAL_get_support_lang_alias(i, strLangAlias);
	
	if (strLangAlias[0] == '\0')
	{	
		return FALSE;
	}
	
	return TRUE;
}

int get_language_index(char* lang)
{
/*	if(!get_language_enum_by_vendor)
		init_get_language_enum_func();

	return get_language_enum_by_vendor(lang);*/

	char strLang[64];
	char strLangAlias[64];
	int langCnt = 0;
	int i;
	FILE *fp = NULL;
	char *buf;
	size_t len = 0;
	int idx = LANGUAGE_ENGLISH;
	int ret;

	langCnt = DAL_get_support_lang_cnt();
	if (langCnt < 0) return LANGUAGE_ENGLISH;
	
	memset(strLang, 0x00, sizeof(strLang));
	memset(strLangAlias, 0x00, sizeof(strLangAlias));

	for (i = 0; i < langCnt; ++i) {
		DAL_get_support_lang(i, strLang);
		if (strcmp(strLang, lang) == 0) break;
	}

	DAL_get_support_lang_alias(i, strLangAlias);

	fp = fopen(NF_MULTI_LANG_STRING, "r+");
	if(fp == NULL)	{
		g_warning("init_multi_language_support() fopen error");
		return LANGUAGE_ENGLISH;
	}

	len = 0;

	buf = imalloc(TEXTDB_LENGTH);
	ret = _getline(fp, buf, TEXTDB_LENGTH, &len); 
	if(ret != 0) {
		fclose(fp);
		ifree(buf);
		return LANGUAGE_ENGLISH;
	}

	idx = _get_lang_idx(buf, strLangAlias);
	printf("language = [%s], language alias = [%s], index = [%d]\n", lang, strLangAlias, idx);
	fclose(fp);
	ifree(buf);
	return idx;
}

int init_multi_language_support(char *lang)	{
	FILE* fp = NULL;
	char* ret = NULL;

#if 0	// Don't need. By hakeya. 2009-12-23
	if(0 != select_encoding(lang))	{
		selected_language = LANGUAGE_NONE;
		return -1;
	}
	selected_language = lang;
#endif

	nffont_change_font(lang);

	fp = fopen(NF_MULTI_LANG_STRING, "r+");
	if(NULL == fp)	{
		g_warning("init_multi_language_support() fopen error");
		return -1;
	}
//	table = g_hash_table_new(g_str_hash, g_str_equal);
	table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	if(NULL == table)	{
		fclose(fp);
		return -1;
	}
	
	if(0 != parse_file(fp, get_language_index(lang)))	{
		fclose(fp);
		return -1;
	}
	
	fclose(fp);
	
	init_completed = 1;	

	return 0;
}

int deinit_multi_language_support()	{
	printf("DEINIT_MULTI_LANGUAGE_SUPPORT\n");
	if(table)	{
		g_hash_table_destroy(table);
		table = NULL;
	}
	return 0;
}

/*inline*/ const char* lookup_string(const char* key)	{
	if(0==init_completed)	{
		// if init_multi_language was not successfully completed, 
		return NULL;
	}
	g_assert(key);
//	g_assert(table);

	if(!table) 
	{
		g_print("\nlanguage_table error. lookup_string error. \n");
		return key;
	}
	
	char* p = g_hash_table_lookup(table, key);

	if((p != NULL)&&(strcmp(p, "#N/A") != 0)){
	   	return p;
	}else{	  	
		return key;
	}
}

int get_string_by_language(const char *lang, const char* key, char* val)	
{
	FILE* fp = NULL;
	char *buf;
	char tmpkey[1024];
	char tmpval[1024];
	int len = 0;
	int lang_idx;
	
	g_assert(lang);	
	g_assert(key);

	buf = imalloc(TEXTDB_LENGTH);
	memset(tmpkey, 0, 1024);
	memset(tmpval, 0, 1024);

	fp = fopen(NF_MULTI_LANG_STRING, "r+");
	if(NULL == fp)	{
		g_warning("init_multi_language_support() fopen error");
		return -1;
	}

	lang_idx = get_language_index(lang);

	while(1)	
	{
		len = 0;
		memset(buf, 0, TEXTDB_LENGTH);

		if (_getline(fp, buf, TEXTDB_LENGTH, &len) == 0)	
		{
			if (len == 0)	
			{
				break;
			}
			else if (len >= TEXTDB_LENGTH - 2) 
			{
				g_warning("ERROR: Too many characters in language table\n");
			}
		} 
		else 
		{				// error 
			g_warning("parse_file() : getline error\n");
			break;
		}
			
        if (parse_line(buf, LANGUAGE_KEY, lang_idx, tmpkey, tmpval) == 0)
        {
            if (strcmp(tmpkey, key) == 0)
            {
                strcpy(val, tmpval);
                break;
            }
		}
		else
        {
            break;
        }		
	}

	ifree(buf);	
	
	return 0;
}

#if 0
const char* get_selected_encoding()
{
	return selected_encoding;	
}
#endif

void change_language_by_db()
{
	gchar lang[32];
	DAL_get_language(lang);

	if(!_is_exist_language_list(lang))
	{	
		DAL_get_support_lang(0, lang);
		DAL_set_language(lang);
		DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
	}
	printf("CHANGE_LANGUAGE_BY_DB, %s\n", lang);
	
	deinit_multi_language_support();
	if( 0 != init_multi_language_support(lang) )	{
		g_warning("\ninit_multi_language_support error !0\n");
	} else {
		g_print("\ninit_multi_language_support success \n");
	}

	vw_apply_new_lang();
}

