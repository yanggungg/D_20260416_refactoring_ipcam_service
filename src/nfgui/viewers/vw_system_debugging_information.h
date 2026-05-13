#ifndef _VW_SYSTEM_DEBUGGING_INFORMATION_H_
#define _VW_SYSTEM_DEBUGGING_INFORMATION_H_

#include "objects/nfobject.h"

#if 0
typedef struct _DEBUGGING_INFO_T
{
    guint mem_total;
    guint mem_used;

    guint nand_total;
    guint nand_used;

    guint cpu_idle;
    
}DEBUGGING_INFO;

#endif


void System_Debugging_Information_Open(NFWINDOW *parent);

#endif
