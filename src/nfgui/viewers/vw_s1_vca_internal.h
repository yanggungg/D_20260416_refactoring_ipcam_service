#ifndef _VW_S1_VCA_INTERNAL_H_
#define _VW_S1_VCA_INTERNAL_H_

#include "objects/nfobject.h"

enum {
	VCA_S1_RULE_INVASION = 0,
	VCA_S1_RULE_LOITERING,
	VCA_S1_RULE_ABANDON,
	VCA_S1_RULE_STEAL,
	VCA_S1_RULE_TOPPLE,
	VCA_S1_RULE_FENCE,
	VCA_S1_RULE_COUNT,
	VCA_S1_RULE_TAMPERING,
#if 0	
	VCA_S1_RULE_PRIVACY,
#endif	
	VCA_S1_RULE_MAX	
};

#define STR_RULE_INVASION		"RULE_INVASION"
#define STR_RULE_LOITERING		"RULE_LOITERING"
#define STR_RULE_ABANDON		"RULE_ABANDON"
#define STR_RULE_STEAL			"RULE_STEAL"
#define STR_RULE_TOPPLE			"RULE_TOPPLE"
#define STR_RULE_FENCE			"RULE_FENCE"
#define STR_RULE_COUNT			"RULE_COUNT"
#define STR_RULE_TAMPERING		"RULE_TAMPERING"
#define STR_RULE_PRIVACY		"RULE_PRIVACY"

#define RULE_NO_EXPOSE				(0)
#define RULE_EXPOSE				(1)



NFOBJECT *_get_S1_VCA_ruleSelect_fixed();
NFOBJECT *_get_S1_VCA_ruleSched_fixed();
NFOBJECT *_get_S1_VCA_ruleSetup_fixed();
NFOBJECT *_get_S1_VCA_ruleVideo_fixed();


// S1 RULE SELECT FIXED
NFOBJECT* _S1_VCA_RuleSelect_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h);
gint _S1_VCA_RuleSelect_update();
gint _S1_VCA_RuleSelect_set_channel(gint ch);
gint _S1_VCA_RuleSelect_set_ruleSched();


// S1 RULE SCHEDULE FIXED
gint _S1_VCA_RuleSchedule_set_channel(gint ch, VCASchedData *data);


// S1 RULE SETUP FIXED
NFOBJECT* _S1_VCA_RuleSetup_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h);
gint _S1_VCA_RuleSetup_hide();
gint _S1_VCA_RuleSetup_update();
gint _S1_VCA_RuleSetup_set_channel(gint ch);
gint _S1_VCA_RuleSetup_set_ruleIdx(gint rule_idx);
gint _S1_VCA_RuleSetup_set_ruleSched();


// S1 RULE VIDEO FIXED
NFOBJECT* _S1_VCA_RuleVideo_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h);
gint _S1_VCA_RuleVideo_set_channel(gint ch);
gint _S1_VCA_RuleVideo_set_preview_on();
gint _S1_VCA_RuleVideo_set_preview_off();
gint _S1_VCA_RuleVideo_set_mevent(gint read, gint write);
gint VW_S1_VCA_attach_mevent(VCA_MEVENT_E mevt_type, VCA_MEVENT_CB_FUNC mevent_cb, gpointer user_data);

#endif

