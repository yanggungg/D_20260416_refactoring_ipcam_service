#include <stdio.h>
#include "iux_afx.h"
#include "nf_api_archive.h"

int ux_main(int skip_sst_init)
{
	printf("#####################################################\n");
	printf("#####################################################\n");
	printf("##\n");
	printf("##\n");
	printf("[IUX:MAIN] ux_main() is called, (UI thread = %p)\n", g_thread_self());
	printf("##\n");
	printf("##\n");
	printf("#####################################################\n");
	printf("#####################################################\n");


#ifdef GUI_4CH_SUPPORT
	start_iux(4, skip_sst_init);
#elif GUI_8CH_SUPPORT
	start_iux(8, skip_sst_init);
#elif GUI_16CH_SUPPORT
	start_iux(16, skip_sst_init);
#elif GUI_32CH_SUPPORT
	start_iux(32, skip_sst_init);	
#endif

	return 0;
}
