
#include "nf_va_statistic.h"
#include "nf_api_eventlog.h"
#include "libsst.h"

#define VA_STA_RES_CNT	100


int convert_event(u32 event_type){

	switch(event_type){
		case IVCA_ET_DIR_POS:
			return VA_EVENT_DIR_POS;
		case IVCA_ET_DIR_NEG:
			return VA_EVENT_DIR_NEG;
		case IVCA_ET_ENTER:
			return VA_EVENT_ENTER;
		case IVCA_ET_EXIT:
			return VA_EVENT_EXIT;
		case IVCA_ET_STOPPED:
			return VA_EVENT_STOPPED;
		case IVCA_ET_ABANDONED:
			return VA_EVENT_ABANDONED;
		case IVCA_ET_REMOVED:
			return VA_EVENT_REMOVED;
		case IVCA_ET_LOITERED:
			return VA_EVENT_LOITERED;
		case IVCA_ET_FALL:
			return VA_EVENT_FALL;
		case IVCA_ET_COUNTER:
			return VA_EVENT_COUNTER;
		default:
			g_warning("%s event_type[%08x] Wrong Data", __FUNCTION__, event_type); 	
			return 0;
	}
}

int cal_time_interval(int period){
	
	switch(period){
		case PERIOD_HOUR:
			return 60*5;
		case PERIOD_DAY:
			return 60*60;
		case PERIOD_WEEK:
			return 60*60*24;
		case PERIOD_MONTH:
			return 60*60*24;
		case PERIOD_YEAR:
			return 60*60*24*30;
		default:
			g_warning("%s period[%d] Wrong Data", __FUNCTION__, period); 	
			return 0;

	}
}

guint fix_start_time(struct tm *stime,guint start_time, int period){
			
	switch(period){
		case PERIOD_HOUR:
			return start_time - (stime->tm_min%5)*60 -  stime->tm_sec;
		case PERIOD_DAY:
			return start_time - stime->tm_min*60 -  stime->tm_sec;
		case PERIOD_WEEK:
			return start_time - stime->tm_wday*60*60*24 - stime->tm_hour*60*60 - stime->tm_min*60 -  stime->tm_sec;
		case PERIOD_MONTH:
			return start_time - stime->tm_hour*60*60 - stime->tm_min*60 -  stime->tm_sec;
		case PERIOD_YEAR:
			return start_time - (stime->tm_mday - 1)*60*60*24 - stime->tm_hour*60*60 - stime->tm_min*60 -  stime->tm_sec;
		default:
			g_warning("%s period[%d] Wrong Data", __FUNCTION__, period); 	
			return 0;
	}
}

int get_va_statistic(va_statistic_t* data){

	GTimeVal tv;
	GTimeVal tv1;
	struct tm *stime;	
	struct tm *etime;	
	gint result = 0, log_sid;
	guint cur_time=0;
	guint start_time=0;
	guint end_time=0;
	guint time_interval=0;
	guint start_week=0;
	char param1[LOG_P1_MASK_SIZE];
	NF_LOG_DATA elem[VA_STA_RES_CNT];
	u64 log_id = 0;
	gint log_cnt = 0;
	gint total_log_cnt = 0;
	gint cal_cnt = 0;
	gint i,k;
	gint event_id;
	ivca_rule_event_t* event_data;
	NF_LOG_PARAM_MODE_E search_mode = NF_LOG_PARAM_MODE_TIME;
	gint aver_freq[AVERAGE_MAX][AVERAGE_DATA_MAX_LEN];
	
	printf("Enter get_va_statistic !!!\n");

	memset(param1, 0x00, sizeof(param1));
	param1[LOG_P1_MASKOFS_VCA + data->ch/8] = (1 << (data->ch % 8));

	//clear output
	memset(data->main_data, 0 , sizeof(data->main_data));
	memset(data->zone_data, 0 , sizeof(data->zone_data));
	memset(data->event_data, 0 , sizeof(data->event_data));
	memset(data->average_data, 0 , sizeof(data->average_data));
	
	memset(aver_freq, 0 , sizeof(aver_freq));

	//test input
	//data->rule_list[0] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[1] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[2] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[3] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;

	g_get_current_time(&tv);
	cur_time = tv.tv_sec;

	GUINT64_TO_GTIMEVAL(data->start_time, tv);
	start_time = tv.tv_sec;

	if(cur_time < start_time){
		printf("cur_time < start_time!!!\n");
		return -1;
	}
	
	stime = localtime(&tv.tv_sec);
	start_week = stime->tm_wday;
	
	printf("s_mon %d s_day %d s_yday %d s_week %d s_hour %d !!\n",stime->tm_mon,stime->tm_mday,stime->tm_yday,stime->tm_wday,stime->tm_hour);

	GUINT64_TO_GTIMEVAL(data->end_time, tv1);
	end_time = tv1.tv_sec;
		
	printf("cur_time %d start_time %d %lld end_time %d %lld!!!\n",cur_time,start_time,data->start_time,end_time, data->end_time);

	time_interval = cal_time_interval(data->period);		

	if(data->period >= PERIOD_WEEK){

		if(cur_time < end_time)
			etime = localtime(&cur_time);
		else
			etime = localtime(&end_time);

		printf("e_mon %d e_day %d e_yday %d e_week %d e_hour %d !!\n",etime->tm_mon,etime->tm_mday,etime->tm_yday,etime->tm_wday,etime->tm_hour);

		
		if(data->period == PERIOD_WEEK){
			for(i=0; i < AVERAGE_HOUR_LEN ; i++)
				aver_freq[AVERAGE_HOUR][i] = etime->tm_wday;
			for(i=0; i <= etime->tm_hour ; i++)
				aver_freq[AVERAGE_HOUR][i]++;
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
		}

		if(data->period == PERIOD_MONTH){
			for(i=0; i < AVERAGE_HOUR_LEN ; i++)
				aver_freq[AVERAGE_HOUR][i] = etime->tm_mday - 1;
			for(i=0; i <= etime->tm_hour ; i++)
				aver_freq[AVERAGE_HOUR][i]++;	
			for(i=0; i < etime->tm_mday ; i++)
				aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;
			
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
			
			printf("aver_freq week data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_WEEK][i]);
			}
			printf("\n");
		}

		if(data->period == PERIOD_YEAR){
			for(i=0; i < AVERAGE_HOUR_LEN ; i++)
				aver_freq[AVERAGE_HOUR][i] = etime->tm_yday;
			for(i=0; i <= etime->tm_hour ; i++)
				aver_freq[AVERAGE_HOUR][i]++;
			for(i=0; i < etime->tm_yday ; i++)
				aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;
			for(i=0; i < etime->tm_mon ; i++){
				int days;
				days = g_date_get_days_in_month(i, etime->tm_year+1900);
				for(k=0; k < days ; k ++)
					aver_freq[AVERAGE_DAY][k]++;
			}
			for(i=0; i < etime->tm_mday ; i++)
				aver_freq[AVERAGE_DAY][i]++;		
			
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
			
			printf("aver_freq week data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_WEEK][i]);
			}
			printf("\n");
			
			printf("aver_freq day data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_DAY][i]);
			}
			printf("\n");
			
		}


	}

	while(1){
		
		log_sid = result = sst_log_get_open ( search_mode, NF_LOG_PARAM_DIR_FORWARD, NF_LOG_PARAM_HIDE_OFF,
									LT_MASK_VCA, param1,
									data->start_time, data->end_time, data->start_time, log_id);	
		if ( result < 0 ) {
			g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
			return -2;
		}
		
		result = sst_log_get( log_sid, VA_STA_RES_CNT, (struct log_data_t *)elem );
		if ( result < 0 ) {
			g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 

			result = sst_log_get_close(log_sid);
			if ( result < 0 ) {		
				g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
				return -3;
			}
			return -4;
		}

		log_cnt = result;
		total_log_cnt += log_cnt;
		
		//g_message( "[%s] result_count[%d]", __FUNCTION__, log_cnt);
		for(i=0; i<log_cnt;i++)
		{
		/*
			g_message( "%s [%d] timestamp[%lld] type[%d] p1[%d]p2[%d] text[%16.16s] log_id [%lld]", 
					__FUNCTION__, i,
					elem[i].timestamp, elem[i].type, 
					elem[i].param1, elem[i].param2, elem[i].text ,elem[i].log_id);
					*/
			
			event_data = elem[i].text;
			/*
			g_message( "%s  ch[%d] rule_id[%d] type[%08x] object_id[%d] ", 
					__FUNCTION__,
					event_data->ch, event_data->rule_id,event_data->type,event_data->object_id);
			*/
			// zone check
			if(data->rule_list[event_data->rule_id] & event_data->type){
				
				GUINT64_TO_GTIMEVAL(elem[i].timestamp, tv);
				if((tv.tv_sec - start_time < 0) || (tv.tv_sec - start_time) / time_interval >= MAIN_DATA_MAX_LEN){
					g_warning("%s log time[%d] start_time [%d]  time_interval [%d] \n", __FUNCTION__, tv.tv_sec,start_time,time_interval); 
					continue;
				}
				
				cal_cnt++;
				data->zone_data[event_data->rule_id]++;
				event_id = convert_event(event_data->type);
				data->event_data[event_id]++;		
				
				
				
			//	int tm_sec;   /* Seconds. [0-60] (1 leap second) 윤초로 인해 61 이 들어 갈 수 있음*/
			//	int tm_min;   /* Minutes. [0-59] */
			//	int tm_hour;   /* Hours. [0-23] */
			//	int tm_mday;   /* Day. [1-31] */
			//	int tm_mon;   /* Month. [0-11] */
			//	int tm_year;   /* Year - 1900. 1900 년 이후의 연도  */
			//	int tm_wday;   /* Day of week. [0-6] 요일 */
			//	int tm_yday;   /* Days in year.[0-365] 1월 1일 이후 몇 일이 지났는지 */
			//	int tm_isdst;   /* DST. [-1/0/1] 썸머 타임 여부 */


				stime = localtime(&tv.tv_sec);
				
				if(data->period == PERIOD_YEAR){
					data->main_data[stime->tm_mon]++;	
				}
				else{
					data->main_data[(tv.tv_sec - start_time) / time_interval]++;	
				}
				
				if(data->period >= PERIOD_WEEK){
					data->average_data[AVERAGE_HOUR][stime->tm_hour]++;
					if(data->period >= PERIOD_MONTH){
						data->average_data[AVERAGE_WEEK][stime->tm_wday]++;
						if(data->period == PERIOD_YEAR)
							data->average_data[AVERAGE_DAY][stime->tm_mday -1]++;
					}
				}

			}
			else{
				continue;
			}
				
		}
				
		result = sst_log_get_close(log_sid);
		if ( result < 0 ) {		
			g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
			return -5;
		}

		if(log_cnt == VA_STA_RES_CNT){
			search_mode = NF_LOG_PARAM_MODE_LOGID;
			log_id = elem[log_cnt-1].log_id;
		}
		else{

			if(data->period >= PERIOD_WEEK){
				for(i=0; i< AVERAGE_HOUR_LEN ; i++){
					if(aver_freq[AVERAGE_HOUR][i] > 0)
						data->average_data[AVERAGE_HOUR][i] = data->average_data[AVERAGE_HOUR][i]/aver_freq[AVERAGE_HOUR][i];
					else
						data->average_data[AVERAGE_HOUR][i] = 0;
				}
				if(data->period >= PERIOD_MONTH){
					for(i=0; i< AVERAGE_WEEK_LEN ; i++){
						if(aver_freq[AVERAGE_WEEK][i] > 0)
							data->average_data[AVERAGE_WEEK][i] = data->average_data[AVERAGE_WEEK][i]/aver_freq[AVERAGE_WEEK][i];
						else
							data->average_data[AVERAGE_WEEK][i] = 0;
					}
					if(data->period == PERIOD_YEAR){
						for(i=0; i< AVERAGE_DAY_LEN ; i++){
							if(aver_freq[AVERAGE_DAY][i] > 0)
								data->average_data[AVERAGE_DAY][i] = data->average_data[AVERAGE_DAY][i]/aver_freq[AVERAGE_DAY][i];
							else
								data->average_data[AVERAGE_DAY][i] = 0;
						}
					}
				}
			}
				
			printf("total %d log detect %d calculated !!! break!!\n",total_log_cnt,cal_cnt);
			printf("zone data %d %d %d %d %d %d \n",data->zone_data[0],data->zone_data[1],data->zone_data[2],data->zone_data[3],data->zone_data[4],data->zone_data[5]);
			printf("event data %d %d %d %d %d %d \n",data->event_data[0],data->event_data[1],data->event_data[2],data->event_data[3],data->event_data[4],data->event_data[5]);
			printf("main data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",data->main_data[i]);
			}
			
			if(data->period >= PERIOD_WEEK){
				printf("average data week ");
				for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
					printf("%d ",data->average_data[AVERAGE_HOUR][i]);
				}
				printf("\n");
				if(data->period >= PERIOD_MONTH){
					printf("average data mon ");
					for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
						printf("%d ",data->average_data[AVERAGE_WEEK][i]);
					}
					printf("\n");
					if(data->period == PERIOD_YEAR){
						printf("average data year ");
							for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
								printf("%d ",data->average_data[AVERAGE_DAY][i]);
							}
						printf("\n");
					}
				}
				
			}
			break;
		}
	}

}


int get_va_statistic_smart(event_buffer_t* event_list , int event_cnt, va_statistic_t* data){
	
	GTimeVal tv;
	GTimeVal tv1;
	struct tm *stime;	
	struct tm *etime;	
	guint start_time=0;
	guint start_mon=0;
	guint start_year=0;
	guint start_week=0;
	guint start_hour=0;
	guint start_day=0;
	guint start_yday=0;
	guint end_time=0;
	guint fixed_start_time=0;
	guint time_interval=0;
	gint i,k;
	gint event_id;
	ivca_rule_event_t* event_data;
	gint aver_freq[AVERAGE_MAX][AVERAGE_DATA_MAX_LEN];

	printf("Enter get_va_statistic_smart !!!\n");
	
	//clear output
	memset(data->main_data, 0 , sizeof(data->main_data));
	memset(data->zone_data, 0 , sizeof(data->zone_data));
	memset(data->event_data, 0 , sizeof(data->event_data));
	memset(data->average_data, 0 , sizeof(data->average_data));
	
	memset(aver_freq, 0 , sizeof(aver_freq));
	
	//test input
	//data->rule_list[0] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[1] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[2] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	//data->rule_list[3] = IVCA_ET_ENTER | IVCA_ET_EXIT |IVCA_ET_STOPPED |IVCA_ET_REMOVED |IVCA_ET_LOITERED;
	
	GUINT64_TO_GTIMEVAL(data->start_time, tv);
	start_time = tv.tv_sec;
	stime = localtime(&start_time);
	start_hour = stime->tm_hour;
	start_week = stime->tm_wday;
	start_day = stime->tm_mday;
	start_mon = stime->tm_mon;
	start_year = stime->tm_year;
	start_yday = stime->tm_yday;

	fixed_start_time = fix_start_time(stime, start_time, data->period);
	

	time_interval = cal_time_interval(data->period);
	
	GUINT64_TO_GTIMEVAL(data->end_time, tv1);
	end_time = tv1.tv_sec;
	printf("start_time %d fixed_start_time %d end time %d !!!\n",start_time,fixed_start_time,end_time);

	if(data->period >= PERIOD_WEEK){
		
		etime = localtime(&end_time);
		
		printf("s_mon %d s_day %d s_yday %d s_week %d s_hour %d !!\n",start_mon,start_day,start_yday,start_week,start_hour);
		printf("e_mon %d e_day %d e_yday %d e_week %d e_hour %d !!\n",etime->tm_mon,etime->tm_mday,etime->tm_yday,etime->tm_wday,etime->tm_hour);
		
		if(data->period == PERIOD_WEEK){
			if(etime->tm_wday - start_week > 1){
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] =etime->tm_wday - start_week - 1;
				for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				for(i=0; i <= etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
			}
			else if(etime->tm_wday - start_week == 1){
				for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				for(i=0; i <= etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
			}
			else{
				for(i=start_hour; i <= etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
			}
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
		}

		if(data->period == PERIOD_MONTH){
		
			if(start_mon != etime->tm_mon){
				int days;
				int tmp;
				days = g_date_get_days_in_month(start_mon, start_year+1900);
				if(days == start_day){
					for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
						aver_freq[AVERAGE_HOUR][i]++;
				}
				else{
					for(i=0; i < AVERAGE_HOUR_LEN ; i++)
						aver_freq[AVERAGE_HOUR][i] = days - start_day;
					for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
						aver_freq[AVERAGE_HOUR][i]++;
				}
				
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] += etime->tm_mday - 1;
				for(i=0; i < etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				
				for(i=start_day; i <= days ; i++)
					aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;
				tmp = start_week + i + 1;
				for(i=0; i < etime->tm_mday ; i++)
					aver_freq[AVERAGE_WEEK][(tmp + i)%AVERAGE_WEEK_LEN]++;

			}
			else{
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] =etime->tm_mday - start_day - 1;
				for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				for(i=0; i <= etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				
				for(i=start_day; i <= etime->tm_mday ; i++)
					aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;
			}
			
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
			
			printf("aver_freq week data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_WEEK][i]);
			}
			printf("\n");
		}

		if(data->period == PERIOD_YEAR){
			if(start_year != etime->tm_year){
				int tmp;
				int days;
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] += 365 - start_yday;
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] += etime->tm_yday;		
				for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				for(i=0; i < etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
	
				for(i=start_yday; i < 365 ; i++)
					aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;
				tmp = start_week + i + 1;
				for(i=0; i < etime->tm_yday ; i++)
					aver_freq[AVERAGE_WEEK][(tmp + i)%AVERAGE_WEEK_LEN]++;

				
				for(i=start_mon; i < PERIOD_YEAR_LEN ; i++){
					days = g_date_get_days_in_month(i, start_year+1900);
					for(i = 0; i < days ; i++)
						aver_freq[AVERAGE_DAY][i]++;
				}
				days = g_date_get_days_in_month(start_mon, start_year+1900);
				for(i = start_day - 1; i < days ; i++)
					aver_freq[AVERAGE_DAY][i]++;
				

				for(i=0; i < etime->tm_mon ; i++){
					days = g_date_get_days_in_month(i, etime->tm_year+1900);
					for(i = 0; i < days ; i++)
						aver_freq[AVERAGE_DAY][i]++;
				}
				for(i = 0; i < etime->tm_mday ; i++)
					aver_freq[AVERAGE_DAY][i]++;
				
			}
			else{
				for(i=0; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i] += etime->tm_yday - start_yday - 1;			
				for(i=start_hour; i < AVERAGE_HOUR_LEN ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
				for(i=0; i < etime->tm_hour ; i++)
					aver_freq[AVERAGE_HOUR][i]++;
	
				for(i=start_yday; i <= etime->tm_yday ; i++)
					aver_freq[AVERAGE_WEEK][(start_week + i)%AVERAGE_WEEK_LEN]++;

				if(etime->tm_mon - start_mon > 1){
					int days;
					for(i=start_mon + 1; i < etime->tm_mon ; i++){
						days = g_date_get_days_in_month(i, etime->tm_year+1900);
						for(k=0; k < days ; k ++)
							aver_freq[AVERAGE_DAY][k]++;
					}
					days = g_date_get_days_in_month(start_mon, etime->tm_year+1900);
					for(i = start_day - 1; i < days ; i++)
						aver_freq[AVERAGE_DAY][i]++;
					for(i = 0; i < etime->tm_mday ; i++)
						aver_freq[AVERAGE_DAY][i]++;
						
				}
				else{
					int days;
					days = g_date_get_days_in_month(start_mon, etime->tm_year+1900);
					for(i = start_day - 1; i < days ; i++)
						aver_freq[AVERAGE_DAY][i]++;
					for(i = 0; i < etime->tm_mday ; i++)
						aver_freq[AVERAGE_DAY][i]++;
				}
				

			}
	
			
			printf("aver_freq hour data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_HOUR][i]);
			}
			printf("\n");
			
			printf("aver_freq week data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_WEEK][i]);
			}
			printf("\n");
			
			printf("aver_freq day data ");
			for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
				printf("%d ",aver_freq[AVERAGE_DAY][i]);
			}
			printf("\n");
			
		}


	}
	
	for(i = 0 ; i <  event_cnt ; i++){
		
		if(data->rule_list[event_list[i].event.rule_id] & event_list[i].event.type){
		    if((event_list[i].event.timestamp < fixed_start_time) || (event_list[i].event.timestamp > end_time) || (event_list[i].event.timestamp - fixed_start_time) / time_interval >= MAIN_DATA_MAX_LEN){
				//g_warning("%s timestamp[%d] start_time [%d]  time_interval [%d] \n", __FUNCTION__, event_list[i].event.timestamp ,start_time,time_interval); 
				continue;
			}
			
			data->zone_data[event_list[i].event.rule_id]++;
			event_id = convert_event(event_list[i].event.type);
			data->event_data[event_id]++;	
			
			stime = localtime(&event_list[i].event.timestamp);
			if(data->period == PERIOD_YEAR){
				if(stime->tm_year == start_year){
					data->main_data[stime->tm_mon - start_mon]++;
				}
				else{
					data->main_data[stime->tm_mon + (PERIOD_YEAR_LEN - start_mon)]++;
				}	
			}
			else{
				data->main_data[(event_list[i].event.timestamp - fixed_start_time) / time_interval]++;	
			}
			
			if(data->period >= PERIOD_WEEK){
				data->average_data[AVERAGE_HOUR][stime->tm_hour]++;
				if(data->period >= PERIOD_MONTH){
					data->average_data[AVERAGE_WEEK][stime->tm_wday]++;
					if(data->period == PERIOD_YEAR)
						data->average_data[AVERAGE_DAY][stime->tm_mday -1]++;
				}
			}
			
			
		}
		else{
			continue;
		}

	}

	if(data->period >= PERIOD_WEEK){
		for(i=0; i< AVERAGE_HOUR_LEN ; i++){
			if(aver_freq[AVERAGE_HOUR][i] > 0)
				data->average_data[AVERAGE_HOUR][i] = data->average_data[AVERAGE_HOUR][i]/aver_freq[AVERAGE_HOUR][i];
			else
				data->average_data[AVERAGE_HOUR][i] = 0;
		}
		if(data->period >= PERIOD_MONTH){
			for(i=0; i< AVERAGE_WEEK_LEN ; i++){
				if(aver_freq[AVERAGE_WEEK][i] > 0)
					data->average_data[AVERAGE_WEEK][i] = data->average_data[AVERAGE_WEEK][i]/aver_freq[AVERAGE_WEEK][i];
				else
					data->average_data[AVERAGE_WEEK][i] = 0;
			}
			if(data->period == PERIOD_YEAR){
				for(i=0; i< AVERAGE_DAY_LEN ; i++){
					if(aver_freq[AVERAGE_DAY][i] > 0)
						data->average_data[AVERAGE_DAY][i] = data->average_data[AVERAGE_DAY][i]/aver_freq[AVERAGE_DAY][i];
					else
						data->average_data[AVERAGE_DAY][i] = 0;
				}
			}
		}
	}

	printf("zone data %d %d %d %d %d %d \n",data->zone_data[0],data->zone_data[1],data->zone_data[2],data->zone_data[3],data->zone_data[4],data->zone_data[5]);
	printf("event data %d %d %d %d %d %d \n",data->event_data[0],data->event_data[1],data->event_data[2],data->event_data[3],data->event_data[4],data->event_data[5]);
	printf("main data ");
	for(i=0; i< MAIN_DATA_MAX_LEN ; i++){
		printf("%d ",data->main_data[i]);
	}
	printf("\n");

	if(data->period >= PERIOD_WEEK){
		printf("average hour_data ");
		for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
			printf("%d ",data->average_data[AVERAGE_HOUR][i]);
		}
		printf("\n");
		if(data->period >= PERIOD_MONTH){
			printf("average week data ");
			for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
				printf("%d ",data->average_data[AVERAGE_WEEK][i]);
			}
			printf("\n");
			if(data->period == PERIOD_YEAR){
				printf("average day data ");
					for(i=0; i< AVERAGE_DATA_MAX_LEN ; i++){
						printf("%d ",data->average_data[AVERAGE_DAY][i]);
					}
				printf("\n");
			}
		}
		
	}

}


int get_va_statistic_export_data(va_statistic_export_t* data){

	GTimeVal tv;
	GTimeVal tv1;
	struct tm *stime;	
	struct tm *etime;	
	gint result = 0, log_sid;
	guint cur_time=0;
	guint start_time=0;
	guint end_time=0;
	guint time_interval=0;
	guint start_week=0;
	char param1[LOG_P1_MASK_SIZE];
	NF_LOG_DATA elem[VA_STA_RES_CNT];
	u64 log_id = 0;
	gint log_cnt = 0;
	gint total_log_cnt = 0;
	gint cal_cnt = 0;
	gint i,k;
	gint event_id;
	ivca_rule_event_t* event_data;
	NF_LOG_PARAM_MODE_E search_mode = NF_LOG_PARAM_MODE_TIME;

	FILE *fp;
	
	printf("Enter get_va_statistic_Export!!!\n");

	memset(param1, 0x00, sizeof(param1));
	param1[LOG_P1_MASKOFS_VCA + data->ch/8] = (1 << (data->ch % 8));

	g_get_current_time(&tv);
	cur_time = tv.tv_sec;

	GUINT64_TO_GTIMEVAL(data->start_time, tv);
	start_time = tv.tv_sec;

	if(cur_time < start_time){
		printf("cur_time < start_time!!!\n");
		return 0;
	}
	
	stime = localtime(&tv.tv_sec);
	start_week = stime->tm_wday;
	
	printf("s_mon %d s_day %d s_yday %d s_week %d s_hour %d !!\n",stime->tm_mon,stime->tm_mday,stime->tm_yday,stime->tm_wday,stime->tm_hour);

	GUINT64_TO_GTIMEVAL(data->end_time, tv1);
	end_time = tv1.tv_sec;
	
	printf("cur_time %d start_time %d %lld end_time %d %lld!!!\n",cur_time,start_time,data->start_time,end_time, data->end_time);

	time_interval = cal_time_interval(data->period);		

	fp = data->fp;

	if(fp == NULL){
		printf("fopen fail !!!!!\n");
		return -1;
	}
	fprintf(fp,"TIMESTAMP,CHANNEL,EVENT_TYPE,RULE_ID\n");

	while(1){
		
		log_sid = result = sst_log_get_open ( search_mode, NF_LOG_PARAM_DIR_FORWARD, NF_LOG_PARAM_HIDE_OFF,
									LT_MASK_VCA, param1,
									data->start_time, data->end_time, data->start_time, log_id);	
		if ( result < 0 ) {
			g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
			return 0;
		}
		
		result = sst_log_get( log_sid, VA_STA_RES_CNT, (struct log_data_t *)elem );
		if ( result < 0 ) {
			g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 

			result = sst_log_get_close(log_sid);
			if ( result < 0 ) {		
				g_warning("%s result[%d](%s)", __FUNCTION__, result, sst_get_error_string(result)); 
				return 0;
			}
			return 0;
		}

		log_cnt = result;
		//total_log_cnt += log_cnt;
		
		//g_message( "[%s] result_count[%d]", __FUNCTION__, log_cnt);
		for(i=0; i<log_cnt;i++)
		{
	
			event_data = elem[i].text;
		
			// zone check
			if(data->rule_list[event_data->rule_id] & event_data->type){
				
				GUINT64_TO_GTIMEVAL(elem[i].timestamp, tv);
				if((tv.tv_sec - start_time < 0) || (tv.tv_sec - start_time) / time_interval >= MAIN_DATA_MAX_LEN){
					g_warning("%s log time[%d] start_time [%d]  time_interval [%d] \n", __FUNCTION__, tv.tv_sec,start_time,time_interval); 
					continue;
				}
				
				
			//	int tm_sec;   /* Seconds. [0-60] (1 leap second) 윤초로 인해 61 이 들어 갈 수 있음*/
			//	int tm_min;   /* Minutes. [0-59] */
			//	int tm_hour;   /* Hours. [0-23] */
			//	int tm_mday;   /* Day. [1-31] */
			//	int tm_mon;   /* Month. [0-11] */
			//	int tm_year;   /* Year - 1900. 1900 년 이후의 연도  */
			//	int tm_wday;   /* Day of week. [0-6] 요일 */
			//	int tm_yday;   /* Days in year.[0-365] 1월 1일 이후 몇 일이 지났는지 */
			//	int tm_isdst;   /* DST. [-1/0/1] 썸머 타임 여부 */

				if(total_log_cnt >= data->total_events){
					break;
				}
				else{
					total_log_cnt++;
					
					stime = localtime(&tv.tv_sec);

					fprintf(fp,"%d%02d%02d_%02d%02d%02d,%d,%s,%d\n",stime->tm_year + 1900,stime->tm_mon+1,stime->tm_mday
								,stime->tm_hour,stime->tm_min,stime->tm_sec,event_data->ch + 1, nf_vca_event_type_string(event_data->type),event_data->rule_id);
				}	
			}
			else{
				continue;
			}
				
		}
				
		result = sst_log_get_close(log_sid);
		if ( result < 0 ) {		
			g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
			return 0;
		}

		if(total_log_cnt >= data->total_events){
			break;
		}else if(log_cnt == VA_STA_RES_CNT){
			search_mode = NF_LOG_PARAM_MODE_LOGID;
			log_id = elem[log_cnt-1].log_id;
		}
		else{
			break;
		}
	}
}


int get_va_statistic_smart_export_data(event_buffer_t* event_list , int event_cnt, va_statistic_export_t* data){
	
	GTimeVal tv;
	GTimeVal tv1;
	struct tm *stime;	
	struct tm *etime;	
	guint start_time=0;
	guint start_mon=0;
	guint start_year=0;
	guint start_week=0;
	guint start_hour=0;
	guint start_day=0;
	guint start_yday=0;
	guint end_time=0;
	guint fixed_start_time=0;
	guint time_interval=0;
	gint i,k;
	gint event_id;
	ivca_rule_event_t* event_data;

	FILE *fp;

	printf("Enter get_va_statistic_smart !!!\n");

	GUINT64_TO_GTIMEVAL(data->start_time, tv);
	start_time = tv.tv_sec;
	stime = localtime(&start_time);
	start_hour = stime->tm_hour;
	start_week = stime->tm_wday;
	start_day = stime->tm_mday;
	start_mon = stime->tm_mon;
	start_year = stime->tm_year;
	start_yday = stime->tm_yday;

	fixed_start_time = fix_start_time(stime, start_time, data->period);
	
	time_interval = cal_time_interval(data->period);
	
	GUINT64_TO_GTIMEVAL(data->end_time, tv1);
	end_time = tv1.tv_sec;
	printf("start_time %d fixed_start_time %d end time %d !!!\n",start_time,fixed_start_time,end_time);
	
	fp = data->fp;

	if(fp == NULL){
		printf("fopen fail !!!!!\n");
		return -1;
	}
	
	fprintf(fp,"TIMESTAMP,CHANNEL,EVENT_TYPE,RULE_ID\n");

	
	for(i = 0 ; i <  event_cnt ; i++){
		
		if(data->rule_list[event_list[i].event.rule_id] & event_list[i].event.type){
		    if((event_list[i].event.timestamp < fixed_start_time) || (event_list[i].event.timestamp > end_time) || (event_list[i].event.timestamp - fixed_start_time) / time_interval >= MAIN_DATA_MAX_LEN){
				//g_warning("%s timestamp[%d] start_time [%d]  time_interval [%d] \n", __FUNCTION__, event_list[i].event.timestamp ,start_time,time_interval); 
				continue;
			}
			
			stime = localtime(&event_list[i].event.timestamp);
			
			fprintf(fp,"%d%02d%02d_%02d%02d%02d,%d,%s,%d\n",stime->tm_year + 1900,stime->tm_mon+1,stime->tm_mday
				,stime->tm_hour,stime->tm_min,stime->tm_sec,event_data->ch + 1, nf_vca_event_type_string(event_data->type),event_data->rule_id);
			
		}
		else{
			continue;
		}

	}

}


