

#include "nf_afx.h"




/**********************************************
 * REMOCON ID
 * *******************************************/

static guint remocon_id = 0;

void nfcd_init_remocon_id()
{
	remocon_id = 0;
}

guint nfcd_get_remocon_id()
{
	return remocon_id;
}

void nfcd_set_remocon_id(guint id)
{
	remocon_id = id;
}




