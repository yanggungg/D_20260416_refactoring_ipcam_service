#ifndef	__NF_UI_FONT_H__
#define	__NF_UI_FONT_H__

#include "nf_afx.h"
#include "multi_language_support.h"

/************** PANGO FONT NAME ***************/
/*
	#define	PANGO_SANS_BOLD_12			"Sans Bold 12"
	#define	PANGO_SANS_18				"Sans 18"
	#define	PANGO_SANS_BOLD_16			"Sans Bold 16"
	#define	PANGO_SANS_BOLD_ITALIC_18	"Sans Bold Italic 18"
	#define	PANGO_SANS_BOLD_ITALIC_25	"Sans Bold Italic 25"
*/

typedef enum {
	NFFONT_MINI_NORMAL = 0,
	NFFONT_MINI_SEMI,

	NFFONT_MINI_NORMAL_1,
	NFFONT_MINI_SEMI_1,

	NFFONT_MINI_NORMAL_3,
	NFFONT_MINI_SEMI_3,

	NFFONT_MINI_NORMAL_4,
	NFFONT_MINI_SEMI_4,

	NFFONT_MINI_NORMAL_5,
	NFFONT_MINI_SEMI_5,

	NFFONT_SMALL_NORMAL,
	NFFONT_SMALL_SEMI,
	NFFONT_SMALL_THIN,

	NFFONT_SMALL_NORMAL_1,
	NFFONT_SMALL_SEMI_1,

	NFFONT_MEDIUM_NORMAL,
	NFFONT_MEDIUM_SEMI,
	NFFONT_MEDIUM_THIN,

	NFFONT_LARGE_NORMAL,
	NFFONT_LARGE_SEMI,

    NFFONT_LARGE_NORMAL_1,
    NFFONT_LARGE_SEMI_1,
       
	NFFONT_XLARGE_NORMAL,
	NFFONT_XLARGE_SEMI,

	NFFONT_XLARGE_NORMAL_1,
	NFFONT_XLARGE_SEMI_1,

	NFFONT_XLARGE_NORMAL_2,
	NFFONT_XLARGE_SEMI_2,

	NFFONT_XLARGE_NORMAL_3,
	NFFONT_XLARGE_SEMI_3,

	NFFONT_END
}nffont_type;

typedef enum {
	NFFONT_CHARSET_ISO8859_1 = 0, 
	NFFONT_CHARSET_ISO8859_2, 
	NFFONT_CHARSET_ISO8859_5, 
	NFFONT_CHARSET_ISO8859_9, 
	NFFONT_CHARSET_GULIIM, 
	NFFONT_CHARSET_END
} nffont_charset_type;

GdkFont* nffont_get_font(nffont_type font_type);
void nffont_unref_all();
gchar* nffont_get_pango_font(nffont_type font_type);
int init_nffont(nffont_charset_type charset_type);	

int nffont_change_font(char *lang);


#endif	// __NF_UI_FONT_H__


