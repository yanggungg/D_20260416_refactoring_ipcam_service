#ifndef __MULTI_LANGUAGE_SUPPORT__
#define __MULTI_LANGUAGE_SUPPORT__

enum	{
	LANGUAGE_NONE		=	0,
	LANGUAGE_KEY		=	1,
	LANGUAGE_ENGLISH	=	2,
};
//#define NUMBER_SUPPORTED_LANGUAGES 12

int init_multi_language_support(char *lang);
int deinit_multi_language_support();

int get_language_index(char* lang);

/*inline*/ const char* lookup_string(const char* key);
void change_language_by_db();

int get_string_by_language(const char *lang, const char* key, char* val);

#if 0
const char* get_selected_encoding();
#endif

#endif //	__MULTI_LANGUAGE_SUPPORT__

