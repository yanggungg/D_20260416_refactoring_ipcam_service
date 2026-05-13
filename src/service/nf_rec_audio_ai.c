#include <glib.h>

#include "itx_ai_def.h"
#include "nf_common.h"
#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
	#include "nf_rec_audio.h"
#elif defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
	#include "nf_HI_aud_3536.h"
#endif
#include "nf_rec_audio_ai.h"

#if defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
extern void nf_ipcam_send_stream(gint ch, gchar *data, guint len);
#endif

static char *nf_rec_audio_ai_get_sysdb_alarm(int ch, int rule, int event)
{
	char *s=NULL;
	gchar tmp_key[256]={0, };
	static char str_audio[256]={0, };

	sprintf(tmp_key, "cam.dvabx.rule.R%d.Z%d.E%d.event_audio", ch, rule, event);
	s=nf_sysdb_get_str_nocopy(tmp_key);

	sprintf(str_audio, "/opt/""%s", s);

	#if 0
		g_message("%s line%d ch%d rule%d event%d audio_path[%s]", __FUNCTION__, __LINE__,
					ch, rule, event, str_audio);
	#endif

	return str_audio;
}

#if defined(ENABLE_AI_ALARM_AUDIO_DSP) || defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
gint nf_rec_aud_ai_send_frame(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud)
#elif defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
gint nf_rec_aud_ai_send_frame(NF_HI_AUD_DATA_AI_ALARM *ai_aud)
#endif
{
	FILE *fp=NULL;
	gint ch=0, size_wav=0;
	guint size=0;
	guint *mask_ai_event_test=NULL;
	guchar *data=NULL; 
	gboolean *is_playing=NULL;
	static guint *mask_ai_event[NUM_ACTIVE_CH]={NULL, };

	mask_ai_event_test=&ai_aud->mask_ai_evt_test;

	size=ai_aud->ai_aud_size_send;

	for(ch=0; ch<NUM_ACTIVE_CH; ch++) {
		guint *size_remain=&ai_aud->ai_aud_size_remain[ch];
		
		is_playing=&ai_aud->is_playing[ch];
		if(*is_playing == FALSE) {
			#if 0
				if(*mask_ai_event_test != 0x0) {
					g_message("%s line%d ch%d alarm_mask_test[0x%08x]", __FUNCTION__, __LINE__, ch, *mask_ai_event_test);
				}
			#endif
			if((*mask_ai_event_test >> ch) & 0x1) {		// Test!!
				char *filename=NULL;
		
				filename=ai_aud->ai_evt_aud_test_filename;
				ai_aud->fp[ch]=fopen(filename, "r");
				fp=ai_aud->fp[ch];
				if(fp == NULL) {
					g_warning("%s line%d File Open Fail!! ch[%d] %s", __FUNCTION__, __LINE__, ch, filename);
					goto nf_rec_aud_send_fail3_ai;
				}

				// obtain file size:
				fseek(fp, 0 , SEEK_END);
				size_wav=ftell(fp);
				rewind(fp);

				*size_remain=(guint)size_wav;

				*is_playing=TRUE;
				
				g_message("%s line%d ch%d File Opened!! name[%s] filesize[%d]", __FUNCTION__, __LINE__, ch, filename, size_wav);
			}
			else {
				gint ai_evt_cnt=0;
				guint ai_evt_total=ai_aud->ai_evt_cnt;

				for(ai_evt_cnt=0; ai_evt_cnt < (gint)ai_evt_total; ai_evt_cnt++) {
					gint ai_evt_rule=ai_aud->ai_evt_rule[ai_evt_cnt][ch];
		
					mask_ai_event[ch]=&ai_aud->mask_ai_evt[ai_evt_cnt][ch];
					#if 0
						if(*mask_ai_event[ch] != 0x0) {
							g_message("%s line%d ch%d mask_ai_event[0x%08x]", __FUNCTION__, __LINE__, ch, *mask_ai_event[ch]);
						}
					#endif

					if(*mask_ai_event[ch]) {
						gint ai_evt_type=0;

						for(ai_evt_type=0; ai_evt_type<NF_REC_AUDIO_AI_EVENT_NR; ai_evt_type++) {

							if((*mask_ai_event[ch] >> ai_evt_type) & 0x1) {
								char *filename=NULL;

								filename=nf_rec_audio_ai_get_sysdb_alarm((gint)ch, ai_evt_rule, ai_evt_type);

								ai_aud->fp[ch]=fopen(filename, "r");
								fp=ai_aud->fp[ch];
								if(fp == NULL) {
									g_warning("%s line%d File Open Fail!! ch[%d] event[%d] filename[%s]", 
												__FUNCTION__, __LINE__, ch, ai_evt_type, filename);
									goto nf_rec_aud_send_fail3_ai;
								}

								// obtain file size:
								fseek(fp, 0 , SEEK_END);
								size_wav=ftell(fp);
								rewind(fp);

								*size_remain=(guint)size_wav;

								*is_playing=TRUE;
								
//								g_message("%s line%d CH%d File Opened!! event[%d] rule[%d] name[%s] filesize[%d]", 
//												__FUNCTION__, __LINE__, ch, ai_evt_cnt, ai_evt_rule, filename, size_wav);
							}
							
							if(*is_playing == TRUE) {
								break;
							}
						}	// end for
					}	// end if

					if(*is_playing == TRUE) {
						break;
					}
				}	// end for
			}	// end else
		}

		if(*is_playing == TRUE) {
			#if 0
				if((*mask_ai_event_test >> ch) & 0x1) {
					g_message("%s line%d ch%d Audio Playing For Test!! [%d][%d]", __FUNCTION__, __LINE__, ch, *size_remain, size);
				}

				if((mask_ai_event[ch]) != NULL) {
					if(*mask_ai_event[ch]) {
						g_message("%s line%d ch%d Audio Playing For AI Event!! [%d][%d]", 
										__FUNCTION__, __LINE__, ch, *size_remain, size);
					}
				}
			#endif

			data=(guchar *)g_malloc0(size);
			if(data == NULL) {
				g_warning("%s line%d malloc fail!! ch%d size[%d]", __FUNCTION__, __LINE__, ch, size);
				goto nf_rec_aud_send_fail2_ai;
			}

			memset(data, 0x0, size);

			fp=ai_aud->fp[ch];
			if(fp == NULL) {
				g_warning("%s line%d ch%d fp is null!!", __FUNCTION__, __LINE__, ch);
				goto nf_rec_aud_send_fail3_ai;
			}

			if(*size_remain < size) {
				if(fread(data, *size_remain, 1, fp) != 1) {
					//g_warning("File Read Fail!! Remain[%d]", *size_remain);
					goto nf_rec_aud_send_fail1_ai;
				}
				*size_remain-=*size_remain;
			}
			else {
				if(fread(data, size, 1, fp) != 1) {
					//g_warning("File Read Fail!! ch%d Remain[%d]", ch, *size_remain);
					goto nf_rec_aud_send_fail1_ai;
				}
				*size_remain-=size;
			}

			#if defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
				nf_ipcam_send_stream(ch, (gchar *)data, size);
			#else
				nf_rec_aud_send_to_ipcam(ch, (gchar *)data, size);
			#endif

			if(*size_remain == 0) {
				fclose(fp);
				*is_playing=FALSE;

				if((*mask_ai_event_test >> ch) & 0x1) {
					*mask_ai_event_test &= (guint)~(1 << ch);
					g_message("%s line%d ch%d Audio Send Finish!! Test!!", __FUNCTION__, __LINE__, ch);
				}

				if((mask_ai_event[ch]) != NULL) {
					if(*mask_ai_event[ch]) {
						#if 0
							*mask_ai_event[ch] &= (guint)~(1 << ch);
						#else
							*mask_ai_event[ch]=0;
						#endif
						g_message("%s line%d ch%d Audio Send Finish!! Warning!!", __FUNCTION__, __LINE__, ch);
					}
				}
			}

			g_free(data);

			continue;

			nf_rec_aud_send_fail1_ai:
				g_free(data);
			nf_rec_aud_send_fail2_ai:
				fclose(fp);
			nf_rec_aud_send_fail3_ai:
				*is_playing=FALSE;

				if((*mask_ai_event_test >> ch) & 0x1) {
					*mask_ai_event_test &= (guint)~(1 << ch);
				}

				if((mask_ai_event[ch]) != NULL) {
					if(*mask_ai_event[ch]) {
						#if 0
							*mask_ai_event[ch] &= (guint)~(1 << ch);
						#else
							*mask_ai_event[ch]=0;
						#endif
					}
				}
		}
	}

	return TRUE;
}
#endif

#if defined(ENABLE_AI_ALARM_AUDIO_DSP) || defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
#if defined(ENABLE_AI_ALARM_AUDIO_DSP)
void nf_rec_aud_ai_evt_check(NF_REC_AUDIO_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt)
#elif  defined(ENABLE_AI_ALARM_AUDIO_HICHIP)
void nf_rec_aud_ai_evt_check(NF_HI_AUD_DATA_AI_ALARM *ai_aud, ai_rule_event_t *pevt)
#endif
{
	guint mask_ai_evt=0;
	gint ai_evt_type=0;
	gint ai_evt_cnt=0;
	guint ai_evt_total=ai_aud->ai_evt_cnt;

	for(ai_evt_cnt=0; ai_evt_cnt < (gint)ai_evt_total; ai_evt_cnt++) {
		for(ai_evt_type=0; ai_evt_type<AI_IVCA_NR; ai_evt_type++) {
			if(pevt->type & AI_IVCA_ET_DIR_POS) {
				mask_ai_evt |= (1<< NF_REC_AUDIO_AI_EVENT_DIR_POS);
			} else if(pevt->type & AI_IVCA_ET_DIR_NEG) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_DIR_NEG);
			} else if(pevt->type & AI_IVCA_ET_ENTER) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_ENTER);
			} else if(pevt->type & AI_IVCA_ET_EXIT) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_EXIT);
			} else if(pevt->type & AI_IVCA_ET_STOPPED) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_STOPPED);
			} else if(pevt->type & AI_IVCA_ET_REMOVED) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_REMOVED);
			} else if(pevt->type & AI_IVCA_ET_INTRUSION) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_INTRUSION);
			} else if(pevt->type & AI_IVCA_ET_LOITERED) {
				mask_ai_evt |= (1 << NF_REC_AUDIO_AI_EVENT_LOITERED);
			} else {
				/*
				 *	Do Nothing
				*/
				;
			}
		}

		ai_aud->mask_ai_evt[ai_evt_cnt][pevt->ch]=mask_ai_evt;
		ai_aud->ai_evt_rule[ai_evt_cnt][pevt->ch]=pevt->rule_id;
	}

	#if 0
		for(ai_evt_cnt=0; ai_evt_cnt < (gint)ai_evt_total; ai_evt_cnt++) {
			g_message("%s line%d event[%d] ch%d count[%d] type[0x%08x] mask_ai_evt[0x%08x] rule[%d]\n", 
						__FUNCTION__, __LINE__, ai_evt_cnt, pevt->ch, ai_aud->ai_evt_cnt, pevt->type, 
						ai_aud->mask_ai_evt[ai_evt_cnt][pevt->ch], ai_aud->ai_evt_rule[ai_evt_cnt][pevt->ch]);
		}
	#endif
}
#endif

