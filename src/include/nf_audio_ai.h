#ifndef __NF_AUDIO_AI_H__
#define __NF_AUDIO_AI_H__

/** IMSI AI BOX **/
typedef enum _AI_EVENT_TYPE_E
{
	/* Event types. */
	AI_IVCA_ET_DIR_POS		= 0x00000001,  /**< Crossed positive direction. */
	AI_IVCA_ET_DIR_NEG		= 0x00000002,  /**< Crossed negative direction. */
	AI_IVCA_ET_ENTER		= 0x00000010,  /**< Entered. */
	AI_IVCA_ET_EXIT			= 0x00000020,  /**< Exited. */
	AI_IVCA_ET_STOPPED		= 0x00000040,  /**< Stopped. */
	AI_IVCA_ET_ABANDONED	= 0x00000080,  /**< Abandoned. */
	AI_IVCA_ET_REMOVED		= 0x00000100,  /**< Removed. */
	AI_IVCA_ET_LOITERED		= 0x00000200,  /**< Loitered. */
	AI_IVCA_ET_FALL			= 0x00000400,  /**< Fall. */
	AI_IVCA_ET_COUNTER		= 0x00004000,  /**< Counter value exceeded. */
	AI_IVCA_ET_TAMPER		= 0x00008000,  /**< Camera tamper detected. */
	AI_IVCA_ET_COLOR		= 0x00010000,  /**< Color filter. */
	AI_IVCA_ET_SIZE			= 0x00020000,  /**< Size filter. */
	AI_IVCA_ET_CLASS		= 0x00040000,  /**< Class filter. */
	AI_IVCA_ET_SPEED		= 0x00080000,  /**< Speed filter. */
	AI_IVCA_ET_INTRUSION	= 0x00100000,  /**< Intrusion. */

	AI_IVCA_NR				= 32
	/* FILL ME */
} AI_EVENT_TYPE;

typedef enum _NF_AUDIO_AI_TYPE_E
{
	NF_AUDIO_AI_EVENT_INTRUSION		= 0,
	NF_AUDIO_AI_EVENT_ENTER			= 1,
	NF_AUDIO_AI_EVENT_EXIT			= 2,
	NF_AUDIO_AI_EVENT_REMOVED		= 3,
	NF_AUDIO_AI_EVENT_LOITERED		= 4,
	NF_AUDIO_AI_EVENT_STOPPED		= 5,
	NF_AUDIO_AI_EVENT_DIR_POS		= 6,
	NF_AUDIO_AI_EVENT_DIR_NEG		= 7,

	NF_AUDIO_AI_EVENT_NR
} NF_AUDIO_AI_TYPE;

#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
gint nf_rec_aud_ai_send_frame(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud);
void nf_rec_aud_ai_evt_check(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt);
#elif  defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
gint nf_rec_aud_ai_send_frame(NF_AUD_DATA_AI_ALARM *ai_aud);
void nf_rec_aud_ai_evt_check(NF_AUD_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt);
#endif

#endif

