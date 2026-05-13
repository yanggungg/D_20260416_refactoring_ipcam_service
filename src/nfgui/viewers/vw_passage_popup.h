
#ifndef _VW_PASSAGE_POPUP_H_
#define _VW_PASSAGE_POPUP_H_

typedef struct _PARAGRAPH_STR
{
    gchar *intro[10];
    gint intro_cnt;   
    gint intro_type;   
    
    gchar *body[10];    
    gint body_cnt;    
    gint body_type;    
    
} PARAGRAPH_STR;

typedef enum _PASSDIR_E {
	DIR_TOP_LEFT		= 0,
	DIR_TOP_RIGHT		= 1,
	DIR_BOTTOM_LEFT		= 2,
	DIR_BOTTOM_RIGHT    = 3,
} PASSDIR_E;



typedef enum _BULLETPOINT_TYPE {
	BULLET_NUM      = 0,
	BULLET_BLANK	= 1,
	BULLET_HYPHEN   = 2,
	BULLET_POINTS
}BULLETPOINT_TYPE;

gint vw_passage_popup_open(NFWINDOW *parent, gint mx, gint my, PASSDIR_E dir, PARAGRAPH_STR *para[], gint para_cnt);

#endif 

