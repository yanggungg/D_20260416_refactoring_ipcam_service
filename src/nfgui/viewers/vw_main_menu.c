
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/color_conf.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"

#include "services/vsm.h"
#include "smt.h"

#include "vw_main_menu.h"
#include "vw_system_setup.h"
#include "vw_system_setup_videcon.h"
#include "vw_main_menu_itx2.h"
#include "vw_main_menu_cbc.h"
#include "vw_main_menu_novus.h"
#include "vw_main_menu_honeywell.h"



void VW_MainMenu_Open(NFWINDOW *parent)
{
    gint ui_type;
	char user[64];

    ui_type = get_ui_type();

	smt_set_service(SMT_MAIN_MENU);	
	
	if (var_get_vendor_code() != 230) {
    	scm_put_log(OPEN_SYS_SETUP, 0, 0);
	}

	switch(ui_type) {
		case 100:	// ITX
		case 108:	// ASP
		case 183:	// ORION
		case 31:	// I3
			if (vsm_get_omode() == OMODE_NORMAL) vw_open_system_setup(parent, 98 - 59, 138);
			else 								 vw_open_system_setup(parent, 192 - 59, 138);
			break;

		case 28:	// VIDECON
		case 128:	// VIDECON_US
			if (vsm_get_omode() == OMODE_NORMAL) vw_open_system_setup_videcon(parent, (1920-192-1709)/2, 138);
			else 								 vw_open_system_setup_videcon(parent, (1920-1709)/2, 138);
			break;

		case 18:
		case 99:
			VW_HoneyWell_MainMenu_Open(parent);
			break;

		case 200: 
			VW_ITX2_MainMenu_Open(parent);
			break;

		case 32: 
			VW_CBC_MainMenu_Open(parent);
			break;

		case 46:
			if (vsm_get_omode() == OMODE_NORMAL) VW_NOVUS_MainMenu_Open(parent, 99, 357);
			else 								 VW_NOVUS_MainMenu_Open(parent, 195, 357);
			break;

		default:
			break;
	}

	memset(user, 0x00, sizeof(user));
	ssm_get_cur_id(user);
	
	if (var_get_vendor_code() != 230) {
    	if (strlen(user)) scm_put_log(CLOSE_SYS_SETUP, 0, 0);	
	}

	smt_set_service(SMT_LIVE);
}




