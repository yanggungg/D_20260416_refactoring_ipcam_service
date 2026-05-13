// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2014 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  HaeRyong Ahn,  captainnn@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2014/11/05 HaeRyong Ahn   Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  va_statistic.h
 * @brief  This file contains stuctures and definitions for the itx va statistic 
 * @author  HaeRyong Ahn.
 * @date  2014/11/05
 */

#ifndef	_VA_STATISTIC_H_
#define	_VA_STATISTIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/
  
#include "nf_common.h"
#include "ivca_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/
 
/* period types */
#define PERIOD_HOUR		0
#define PERIOD_DAY		1
#define PERIOD_WEEK	2
#define PERIOD_MONTH	3
#define PERIOD_YEAR		4

/* average types */
#define AVERAGE_HOUR	0
#define AVERAGE_WEEK	1
#define AVERAGE_DAY	2
#define AVERAGE_MAX	3

/* main data lenth */
#define PERIOD_HOUR_LEN		12	// 60/5
#define PERIOD_DAY_LEN			24
#define PERIOD_WEEK_LEN		7
#define PERIOD_MONTH_LEN		31
#define PERIOD_YEAR_LEN		12
#define MAIN_DATA_MAX_LEN		31

/* average data lenth */
#define AVERAGE_HOUR_LEN		24	// 60/5
#define AVERAGE_DAY_LEN		31
#define AVERAGE_WEEK_LEN		7
#define AVERAGE_DATA_MAX_LEN	31


#define VA_EVENT_DIR_POS		0
#define VA_EVENT_DIR_NEG		1
#define VA_EVENT_ENTER			2
#define VA_EVENT_EXIT			3
#define VA_EVENT_STOPPED		4
#define VA_EVENT_REMOVED		5
#define VA_EVENT_LOITERED		6
#define VA_EVENT_ABANDONED   	7
#define VA_EVENT_FALL			8
#define VA_EVENT_COUNTER		9
#define VA_MAX_EVENT_CNT		10

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/
 typedef struct _va_statistic_t {
	int ch;										// input
	guint64 start_time;							// input
	guint64 end_time;							// input
	int period;									// input
	int rule_list[IVCA_MAX_ZONES];				// input	ivca_def.h : ivca_zone_t->enabled
	int main_data[MAIN_DATA_MAX_LEN];			// output
	int zone_data[IVCA_MAX_ZONES];				// output
	int event_data[VA_MAX_EVENT_CNT];				// output
	int average_data[AVERAGE_MAX][AVERAGE_DATA_MAX_LEN];	// output	
} va_statistic_t;

typedef struct _va_statistic_export_t {
	int ch;										// input
	guint64 start_time;							// input
	guint64 end_time;							// input
	int period;									// input
	int rule_list[IVCA_MAX_ZONES];				// input	
	FILE* fp;										// input
	int total_events;								// input
} va_statistic_export_t;

typedef struct _event_buffer_t {
	gint                no;
	ivca_rule_event_t   event;
} event_buffer_t;

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function prototypes                                                         *
 ******************************************************************************/
 
int get_va_statistic(va_statistic_t* data);
int get_va_statistic_smart(event_buffer_t* event_list , int event_cnt, va_statistic_t* data);
int get_va_statistic_export_data(va_statistic_export_t* data);
int get_va_statistic_smart_export_data(event_buffer_t* event_list , int event_cnt, va_statistic_export_t* data);

/* EXAMPLE IN LIVE*/

/*
{
	va_statistic_t data;		
	
	//input data
	data.ch = 	
	data.start_time = 	
	.
	.
	.			
	
	
	res = get_va_statistic(&data);
	if(res < 0)
		FAIL;
	
	
}
*/

/* EXAMPLE IN SMART SEARCH*/

/*
{
	va_statistic_t data;
	ivca_rule_event_t* event_list;						
	int event_cnt;
	
	//input data
	data.ch = 	
	data.start_time = 	
	.
	.
	.	
	
	res = get_va_statistic_smart(event_list , event_cnt, &data);
	if(res < 0)
		FAIL;
	
	
}
*/



#endif	/* _VA_STATISTIC_H_ */
