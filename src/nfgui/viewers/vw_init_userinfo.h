#ifndef _VW_INIT_USERINFO_H_
#define _VW_INIT_USERINFO_H_

#define QNA_COUNT				(3)

static gchar *QUESTIONS[] = {
	"When you were young, what did you want to be when you grew up?",
	"Who was your childhood hero?",
	"What is your dream holiday destination?",
	"What is the name of your first pet?",
	"What was your first car?",
	"What is your mother’s maiden name?",
	"What primary school did you attend?",
	"What is the name of the town where you were born?",
	"In what city or town did your parents meet?",
	"In what city or town was your first job?",
	"What is the name of the street you grew up on?"
};

char *strlwr(char *str);
gint vw_init_userinfo_open(NFWINDOW *parent);

#endif

