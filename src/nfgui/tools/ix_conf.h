/*
 * ix_conf.h
 * 	- text database based on XML
 * 	- read only database
 *	- dependency :
 *		libxml2 (current ver : libxml2-2.7.8)
 *
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 18, 2010
 *
 *
 * Note:
 *
 * 		- this tool is thread-safe.
 * 		- but, it is not allowed multiple file by one instance.
 */

#ifndef __IX_CONF_H
#define __IX_CONF_H

#include <libxml/xmlreader.h>

int icf_init();
int icf_cleanup();
int icf_open_conf(const char *filename);
int icf_close_conf();
int icf_is_opened();
int icf_get_string(const char *cat, const char *key, char *buf);
int icf_get_value(const char *cat, const char *key, int *val);
int icf_get_value_by_int(const char *cat, const char *key);

#endif
