#ifndef __VW_SEARCH_BY_AI_FILTER_POPUP_H__
#define __VW_SEARCH_BY_AI_FILTER_POPUP_H__

#define MAX_SHOW_LIST_CNT   5
#define MAX_STR_OPER_CNT	2
#define MAX_STR_NUM_CNT		(MAX_STR_OPER_CNT + 1)

typedef enum {
    OPT_AI_GRP1 = 0,
    OPT_AI_GRP2,
    OPT_AI_GRP3,
    OPT_AI_GRP4,
    OPT_AI_GRP5,
    OPT_AI_GRP6,
    OPT_AI_GRP7,
    OPT_AI_GRP8,
    OPT_AI_GRP9,
} OPT_AI_GRP_E;

typedef enum {
    OPT_AI_DTR_FORWARD = 0,
    OPT_AI_DTR_INTRUSION,
    OPT_AI_DTR_LOITERING,
    OPT_AI_DTR_REVERSE,
    OPT_AI_DTR_REMOVED,
    OPT_AI_DTR_STOPPED,
} OTP_AI_DTR_E;

typedef struct {
    gint forward;
    gint intrusion;
    gint loitering;
    gint reverse;
    gint removed;
    gint stopped;
} AI_DTR_EVT_T;

typedef struct {
    gint human;
    gint car;
    gint bike;
    gint custom;
    gchar strcustom[256];
} AI_DTR_OBJ_T;

typedef struct {
    AI_DTR_EVT_T evt;
    AI_DTR_OBJ_T obj;
} OPT_AI_DTR_T; 

typedef struct {
    gboolean match_case;
    gboolean match_whole;
    guint evt_cnt;
    gchar *evt_type;
    gboolean evt_type_chk[100];
    guint evt_len;
    gchar oper[MAX_STR_OPER_CNT];
    gchar text[MAX_STR_NUM_CNT][128];
} OPT_AI_GENERIC_T;

typedef struct {
    gint filter_grp_cnt;
    gchar **group;
    gint chk_name;
    gint chk_match_name;
    gchar name[128];
    gint chk_age;
    gint age;
    gint chk_gender;
    gchar gender[32];
} OPT_AI_FR_T; 

typedef struct {
    gint group[9];
    gint group_name[9][64];
    gint chk_num;
    gint chk_match_num;
    gchar number[128];
    gint chk_country;
    gint chk_match_country;
    gchar country[32];
} OPT_AI_LPR_T; 

typedef struct {
    gint human;
    gint car;
    gint bike;
} OPT_BUILTIN_IDZ_T;

typedef struct {
    gint car;
    gint bike;
} OPT_BUILTIN_IPZ_T;

gint VW_Search_By_Ai_Detector_Filter_Popup(NFWINDOW *parent, OPT_AI_DTR_T *opt);
//gint VW_Search_By_Ai_FR_Filter_Popup(NFWINDOW *parent, OPT_AI_FR_T *opt);
//gint VW_Search_By_Ai_LPR_Filter_Popup(NFWINDOW *parent, OPT_AI_LPR_T *opt);
//gint VW_Search_By_Builtin_Idz_Filter_Popup(NFWINDOW *parent, OPT_BUILTIN_IDZ_T *opt);
//gint VW_Search_By_Builtin_Ipz_Filter_Popup(NFWINDOW *parent, OPT_BUILTIN_IDZ_T *opt);



#endif

