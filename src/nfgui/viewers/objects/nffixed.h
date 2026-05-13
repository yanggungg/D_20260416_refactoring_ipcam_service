#ifndef	__NFFIXED_H__
#define	__NFFIXED_H__

#include "nfobject.h"

/**********************************************************************************
 *
 *	NFCONTAINER Data structure
 *	 
 *	
 * *******************************************************************************/

typedef	struct {
	NFOBJECT object;
	GSList *children;
} NFCONTAINER;



/**********************************************************************************
 *
 *	NFFIXED Data structure
 *	 
 *	
 * *******************************************************************************/

typedef	struct {
#if 0
	NFCONTAINER container;
#else
	NFOBJECT object;
	GSList *children;

	gboolean draw_outline;
#endif

} NFFIXED;

NFFIXED *nfui_nffixed_new();
void nfui_nffixed_destroy(NFFIXED *nffixed);
void nfui_nffixed_put(NFFIXED *nffixed, NFOBJECT *child, guint x, guint y);
void nfui_nffixed_move(NFFIXED *nffixed, NFOBJECT *child, guint x, guint y);
//void nfui_nffixed_paint(NFFIXED *nffixed);

void nfui_nffixed_set_drawing_outline(NFFIXED *nffixed, gboolean draw_outline);





#endif	// __NFFIXED_H__


