#include "nf_afx.h"
#include "cheat.h"

#include "../modules/mda.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"

////////////////////////////////////////////////////////////
//
// private variable
//

enum {
	CHEATKEY_FW_UP,
	CHEATKEY_CAM_INSTALL,

	CHEATKEY_MODE
};


enum {
	CHEAT_SIZE_3	= 3,
	CHEAT_SIZE_4	= 4,

	CHEAT_SIZE_NUM
};


static guint key_code[CHEATKEY_MODE][8] = {
    { KEYPAD_DISP, KEYPAD_SEQ, KEYPAD_FREEZE, KEYPAD_LOCK, 0, 0, 0, 0},     // CHEATKEY_FW_UP
    { KEYPAD_CH01, KEYPAD_CH02, KEYPAD_CH03, KEYPAD_CH04, 0, 0, 0, 0},     // CHEATKEY_CAM_INSTALL
};

static guint cheat_size[CHEATKEY_MODE] = {
	CHEAT_SIZE_4,        // CHEATKEY_FW_UP
	CHEAT_SIZE_4,        // CHEATKEY_CAM_INSTALL
};

static int cheat_mode = 0;
static int key_count = 0;

////////////////////////////////////////////////////////////
//
// private functions
//
//

void _do_cheat()
{
	switch(cheat_mode)
	{
		case CHEATKEY_FW_UP:
		{
			if ((smt_get_service() == SMT_LIVE) || 
				(smt_get_service() == SMT_LOGOUT))
			{
				g_message("[%s][%d] CHEAT FWUPGRADE START", __FUNCTION__, __LINE__);
				evt_send_to_local(IREQ_CHEAT_AUTO_FWUP, 0, 0, 0);

			}
		}
		break;
		
		case CHEATKEY_CAM_INSTALL:
		{
			if (smt_get_service() == SMT_SYSTEM_SETUP)
			{
				g_message("[%s][%d] CHEATKEY_SHOW OLD VERSION CAM INSTALL", __FUNCTION__, __LINE__);
				evt_send_to_local(INFY_SHOW_OLD_VER_CAM_INSTALL, 0, 0, 0);

			}
		}
		break;

		default:
		break;
	}
}

gboolean _is_equal_key_size()
{
	if(key_count == cheat_size[cheat_mode])
		return TRUE;

	return FALSE;
}


void _find_cheat_mode(KEYPAD_KID kpid)
{
    gint i = 0, j = 0;

	if (key_code[cheat_mode][key_count] == kpid)
	{
		key_count++;
	}
	else
	{
        for (i = cheat_mode + 1; i < CHEATKEY_MODE; i++)
		{
            if (key_code[i][key_count] == kpid)
            {
				// check previous key
				for(j = 0; j < key_count; j++)
				{
					if (key_code[cheat_mode][j] != key_code[i][j])
						break;
				}

				if(j == key_count)
				{
					cheat_mode = i;
					key_count++;

					break;
				}
            }
		}
		if (i == CHEATKEY_MODE)
		{
			cheat_mode = 0;
			key_count = 0;    
		}
	}
	if(_is_equal_key_size())
	{
		_do_cheat();
		cheat_mode = 0;
		key_count = 0;    
	}
}


////////////////////////////////////////////////////////////
//
// public interfaces
//
//

void cheat_init()
{
	cheat_mode = 0;
	key_count = 0;    
}

void cheat_set_kpid(KEYPAD_KID kpid)
{
	_find_cheat_mode(kpid);
}

