/*
 * cdump.h
 *  - camera packet module
 *  - dependency : scm, mda
 *	
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Oct 2, 2015
 *
 */

#ifndef __CDUMP_H
#define __CDUMP_H

#include "mda.h"
#include "../../service/nf_packet_dump.h"


////////////////////////////////////////////////////////////
//
// public data type 
//


typedef struct _CDUMP_T {
    MEDIA_ID        id;
    
} CDUMP_T;

typedef struct dump_status DUMP_STATUS;


////////////////////////////////////////////////////////////
//
// public interfaces
//

int cdump_init();
int cdump_change_media();

int cdump_packet_dump_start();
int cdump_packet_dump_pause();
int cdump_packet_dump_resume();
int cdump_packet_dump_stop();
int cdump_packet_dump_get_status(DUMP_STATUS *status);
int cdump_packet_dump_get_usb_title(gchar *buf);

#endif 

