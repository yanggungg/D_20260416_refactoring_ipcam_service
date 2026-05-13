#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nf_ipcam_json_utils.h"

json_t* nf_ipcam_get_node_find_str(json_t *p_node, char *p_str)
{
	json_t *child_node = NULL;

	const char *key;
	json_t *value;

	json_object_foreach(p_node, key, value) {
		if ( key )
		{
			if(strstr(key, p_str))
			{
				child_node = value;
				break;
			}
		}
	}

	return child_node;
}

char* nf_ipcam_json_get_string_value(json_t * p_value)
{
	char *val = NULL;

	if(p_value != NULL)
	{
		val = (char*)json_string_value(p_value);
	}

	return val;
}

double nf_ipcam_json_get_real_value(json_t *p_value)
{
	return json_real_value(p_value);
}

json_t * nf_ipcam_load_json(const char *text) 
{
    json_t *root;
    json_error_t error;

    root = json_loads(text, 0, &error);

    if (root) {
        return root;
    } else {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return (json_t *)0;
    }
}

void nf_ipcam_free_json(json_t* p_text)
{
	if(p_text)
	{
		json_decref(p_text);
	}
}

int nf_ipcam_json_get_integer_value(json_t *p_val)
{
	int val = json_integer_value(p_val);
	return val;
}
