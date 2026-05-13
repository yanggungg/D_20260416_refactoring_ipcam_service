#ifndef __NF_IPCAM_JSON_UTILS_C__
#define __NF_IPCAM_JSON_UTILS_C__

#include "jansson.h"

json_t* nf_ipcam_get_node_find_str(json_t *p_node, char *p_str);
char* nf_ipcam_json_get_string_value(json_t * p_value);
double nf_ipcam_json_get_real_value(json_t *p_value);
json_t * nf_ipcam_load_json(const char *text);
void nf_ipcam_free_json(json_t* p_text);
int nf_ipcam_json_get_integer_value(json_t *p_val);

#endif // __NF_IPCAM_ZMQ_UTILS_C__
