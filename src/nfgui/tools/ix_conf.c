/*
 * ix_conf.c
 * 	- text database based on XML
 * 	- read only database
 *	- dependency :
 *		libxml2 (current ver : libxml2-2.7.8)
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 18, 2010
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "ix_func.h"
#include "ix_conf.h"

#define DBG_LEVEL		0
#define DBG_MODULE		"IX_CONF"
#define DMSG(level, format, args...) \
	do { \
		if (DBG_LEVEL && level && DBG_LEVEL >= level) { \
			fprintf(stderr, "[IUX:"DBG_MODULE"] %s():%d: "format"\n", __FUNCTION__, __LINE__, ##args); \
		} \
	} while (0)



#define ICF_LOCK()		pthread_mutex_lock(&iicf.mtx)
#define ICF_UNLOCK()	pthread_mutex_unlock(&iicf.mtx)


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _IXCONF_T {
	pthread_mutex_t		mtx;
	xmlDocPtr 			doc;
	xmlNodePtr 			cur;
} IXCONF_T;


////////////////////////////////////////////////////////////
//
// private variable
//

static IXCONF_T iicf;


////////////////////////////////////////////////////////////
//
// private functions
//

static void _get_value(xmlDocPtr doc, xmlNodePtr cur, const char *key, char *buf)
{

	xmlChar *val;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)key))) {
			val = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (val) {
				strcpy(buf, (const char *)val);
				xmlFree(val);
			}
			break;
		}
		cur = cur->next;
	}
	return;
}

static int _find_category(xmlNodePtr cur, const char *cat)
{
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)cat))){
			return 0;
		}

		cur = cur->next;
	}

	return -1;
}

static int _reset_cur_node(IXCONF_T *piicf)
{
	piicf->cur = xmlDocGetRootElement(piicf->doc);

	if (piicf->cur == NULL) {
		DMSG(1, "empty document\n");
		xmlFreeDoc(piicf->doc);
		return -1;
	}
	return 0;	
}

int _get_string(IXCONF_T *piicf, const char *cat, const char *key, char *buf)
{
	int ret = -1;

	if (piicf->doc == NULL) { return -1; }
	_reset_cur_node(piicf);

	piicf->cur = piicf->cur->xmlChildrenNode;
	while (piicf->cur != NULL) {
		if ((!xmlStrcmp(piicf->cur->name, (const xmlChar *)cat))){
			_get_value(piicf->doc, piicf->cur, key, buf);
			ret = 0;
			break;
		}

		piicf->cur = piicf->cur->next;
	}

	return ret;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int icf_get_string(const char *cat, const char *key, char *buf)
{
	int ret = -1;
	ICF_LOCK();

	if (iicf.doc == NULL) { ICF_UNLOCK(); return -1; }
	_reset_cur_node(&iicf);

	iicf.cur = iicf.cur->xmlChildrenNode;
	while (iicf.cur != NULL) {
		if ((!xmlStrcmp(iicf.cur->name, (const xmlChar *)cat))){
			_get_value(iicf.doc, iicf.cur, key, buf);
			ret = 0;
			break;
		}

		iicf.cur = iicf.cur->next;
	}

	ICF_UNLOCK();	
	return ret;
}

int icf_get_value(const char *cat, const char *key, int *val)
{
	char buf[32];
	ICF_LOCK();

	if (_get_string(&iicf, cat, key, buf) == -1) { ICF_UNLOCK(); return -1; }
	if (!ifn_is_all_digit(buf)) { ICF_UNLOCK(); return -1; }
	*val = atoi(buf);

	ICF_UNLOCK();
	return 0;
}

int icf_get_value_by_int(const char *cat, const char *key)
{
	char buf[32];
	int ret = 0;
	ICF_LOCK();

	if (_get_string(&iicf, cat, key, buf) == -1) { ICF_UNLOCK(); return -1; }
	if (!ifn_is_all_digit(buf)) { ICF_UNLOCK(); return -1; }
	ret = atoi(buf);

	ICF_UNLOCK();
	return ret;
}

int icf_is_opened()
{
	int ret;
	ICF_LOCK();
	ret = (iicf.doc != NULL); // 1 means config file is opened
	ICF_UNLOCK();
	return ret;
}

int icf_init(const char *filename)
{
	int ret;
	memset(&iicf, 0x00, sizeof(iicf));
	pthread_mutex_init(&iicf.mtx, NULL);
	ret = icf_open_conf(filename);
	return ret;
}

int icf_cleanup()
{
	pthread_mutex_destroy(&iicf.mtx);
}

int icf_open_conf(const char *filename) 
{
	ICF_LOCK();

	iicf.doc = xmlParseFile(filename);

	if (iicf.doc == NULL ) {
		DMSG(1, "Document not parsed. \n");
		ICF_UNLOCK();
		return -1;
	}

	_reset_cur_node(&iicf);
	ICF_UNLOCK();
	return 0;
}

int icf_close_conf()
{
	ICF_LOCK();
	if (iicf.doc == NULL) { ICF_UNLOCK(); return -1; }
	
	xmlFreeDoc(iicf.doc);
	iicf.doc = NULL;
	iicf.cur = NULL;

	ICF_UNLOCK();
	return 0;
}

