#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/vw_menu.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nftable.h"

#include "modules/ssm.h"
#include "smt.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_disp_main.h"
#include "vw_sys_sound_main.h"
#include "vw_sys_user_main.h"
#include "vw_sys_net_main.h"
#include "vw_evt_act_main.h"
#include "vw_sys_main.h"
#include "vw_disk_main.h"

#include "vw_menu.h"
#include "vw_main_menu_honeywell.h"


#define WIN_SIZE_W			(1920)
#define WIN_SIZE_H			(1080)


// dependencies : vw_menu.h

static gchar *cam_subScript[MAX_SUB_CNT] = {
    "Change the camera title that\nis displayed on screen.",
	"Adjust the brightness,\ncontrast,color and quality\nfor each channel.",
	"Adjust the brightness,\ncontrast,color and quality\nfor each channel.",
	"Authorize covert camera to\nADMIN/MANAGER/USER/-\nLOG OUT.",
	"Set the motion sensor of the\ncamera so that it can detect\na motion event.",
	"Set the motion sensor of the\ncamera so that it can detect\na motion event.",
	"TBD",
	"Set PTZ Camera ID, Protocol\nand BAUD RATE",
	"Prevent operators from\nviewing areas that should not\nbe monitored",
	"Select the installation mode\nfor your network environment",
	"TBD",
	"Change the installation mode.",
	"Add cameras over the internet\nor local network",
	"Configure the VA settings.",
	"TBD",
	"TBD",
	};

static gchar *dis_subScript[MAX_SUB_CNT] = {
    "Configure the setting for the\ntime, title, boundary, icon, and\nlanguage that will be displayed\non screen.",
	"Set the interval of the sequence\nif changing from monitoring to\nsequence mode.",
	"Select a split mode for the\nsequence and also select a list\nof archive items when the\nsequence is performed.",
	"TBD",
	"TBD",
	"TBD",
	"Configure the POS/ATM settings.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *snd_subScript[MAX_SUB_CNT] = {
    "Choose whether to receive the\nlive sound source and select an\naudio channel.",
	"Set buzzer remote control\noutput options.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *usr_subScript[MAX_SUB_CNT] = {
    "Add a user account(s) that can\nbe edited at a later time.",
	"Grant different user groups\ndifferent permissions to\nspecific menus.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *net_subScript[MAX_SUB_CNT] = {
    "Specify the IP address as well\nas the remote service port.",
	"Configure the DDNS settings so\nthat remote users who are\nconnected to the network can\naccess remotely.",
	"Register and test an e-mail\naddress so that an e-mail\nnotification is delivered at a\nspecific interval or if an event\noccurs.",
	"Check the Internet connection\nstatus, IP camera connection\nstatus, and also the details of\nthe connection status for\neach camera.",
	"Setup encryption of port and\nIP filtering for security",
	"Configure the SNMP settings.",
	"Configure the Cable test.",
	"Configure the RTP settings.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *sys_subScript[MAX_SUB_CNT] = {
    "Specify the current date and\ntime.",
	"Check, update or reset\nthe system information.",
	"Check the current system\nversion and system-related\nsettings.",
	"Configure the settings of\nthe remote control\nand keyboard controller.",
	"TBD",
	"Setup Audio/Snapshot and\nenhanced password rule,\ncheck password when\nSEARCH/BACKUP.",
	"Configure the POS/ATM settings.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *dsk_subScript[MAX_SUB_CNT] = {
    "Show information about\nthe connected disk.",
	"Set to delete recordings\nautomatically, set overwrite\noptions, and format the HDD\nrecording data.",
	"Configure the Disk settings.",
	"Check the S.M.A.R.T.\ninformation of the disk and\nspecify the checking frequency.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };

static gchar *evt_subScript[MAX_SUB_CNT] = {
    "Specify the alarm output\nconditions with the work\nschedule.",
	"Specify the methods of\nnotification such as buzzer,\nvideo pop-up, or e-mail\n if an event occurs.",
	"Configure the settings of\nthe alarm sensor and specify\nthe operation of the sensor\nif an event occurs.",
	"Set an action to be executed\nwhen motion is detected.",
	"Specify a reaction in the event\nthat no video signal is received\nfrom the camera.",
	"Configure the VA settings.",
	"Set any action to an event\nrelated to the disk, recording,\nnetwork, or system.",
	"TBD",
	"TBD",
	"Configure the POS/ATM settings.",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
	"TBD",
    };


static GdkPixbuf *pbBG = NULL;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *submenu_fixed[SYS_SUBMENU_CNT];
static NFOBJECT *f_mButton = NULL;						// focused menu button

static gint submenu_cnt[SYS_SUBMENU_CNT];
static gint f_mIndex = CAMERA_SUBMENU;					// focused menu index
static gchar *strSubmenu[SYS_SUBMENU_CNT][MAX_TAB_CNT];
static gchar *strSubscript[SYS_SUBMENU_CNT][MAX_TAB_CNT];


static gint get_subscript_line_count(gint submenu, gint index)
{
	gchar *offset;
	gint lcnt = 1;

	offset = strSubscript[submenu][index];
	while((offset = strchr(offset, '\n'))) {
		if(!offset)
			break;

		offset += 1;
		lcnt += 1;
	}
	return (lcnt > 1 ? lcnt : 0);
}

static gboolean submenu_fixed_post_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkPixbuf *bgImg;
		GdkDrawable *drawable = NULL;
		GdkGC* gc = NULL;

		if(!pbBG) {
			bgImg = nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);
			pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 750, 524);
			gdk_pixbuf_copy_area(bgImg, 693, 284, 750, 524, pbBG, 0, 0);
		}

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfutil_draw_pixbuf(drawable, gc, pbBG, 693, 284, 750, 524, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);

	}
	else if(evt->type == GDK_DELETE) {
		f_mIndex = CAMERA_SUBMENU;
		if(pbBG) {
			g_object_unref(pbBG);
			pbBG = NULL;
		}
	}

	return FALSE;
}

static gboolean post_subButton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint page_idx = 0;
	gint x, y, w, h;
	guint subtitle_fg[2] = {COLOR_IDX(703), COLOR_IDX(702)};
	guint subscript_fg[2] = {COLOR_IDX(705), COLOR_IDX(704)};
	gint fgIdx1 = 0, fgIdx2 = 0;
	gint lcnt;

	gchar lang[32];
    gint font_size;

	switch(evt->type) {
		case GDK_EXPOSE:
		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			{
				drawable = nfui_nfobject_get_window(obj);
				gc = nfui_nfobject_get_gc(obj);
				nfui_nfobject_get_offset(obj, &x, &y);

				page_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "subBtn idx"));

				switch(obj->status) {
					case NFOBJECT_STATE_NORMAL:
						fgIdx1 = fgIdx2 = 0;
						break;
					case NFOBJECT_STATE_PRELIGHT:
					case NFOBJECT_STATE_ACTIVE:
						fgIdx1 = fgIdx2 = 1;
						break;
					default:
						return FALSE;
				}

				lcnt = get_subscript_line_count(f_mIndex, page_idx);

				x += 95;
				if(lcnt >= 5) y -= 50; //40;	// 5 line
				else 		  y -= 30;//20; 	// default
				w = obj->width;
				h = obj->height;
				nfutil_draw_text_with_pango(NULL, NULL, NULL,
						drawable, gc, strSubmenu[f_mIndex][page_idx],
						x, y, w, h,
						nffont_get_pango_font(NFFONT_MEDIUM_SEMI),
						&UX_COLOR(subtitle_fg[fgIdx1]), NFALIGN_LEFT, 0);

				y += 37;//27;
				if(lcnt > 4) h += 52; 	// 5 line
				else if(lcnt > 3) h += 27; 	// 4 line
				else if(lcnt > 2) h += 13; 	// 3 line
				else if(lcnt == 0) h -= 27; 	// 1 line
				//else if(lcnt > 1) h += 0; 	// 2 line

            	DAL_get_language(lang);

                if (strncmp(lang, "JAPANESE", 8) == 0)
                    font_size = NFFONT_MINI_SEMI_3;
                else if (strncmp(lang, "CHINESE", 7) == 0)
                    font_size = NFFONT_MINI_SEMI_4;
                else
                    font_size = NFFONT_MINI_SEMI_5;

				nfutil_draw_text_with_pango(NULL, NULL, NULL,
						drawable, gc, strSubscript[f_mIndex][page_idx],
						x, y, w, h,
						nffont_get_pango_font(font_size),
						&UX_COLOR(subscript_fg[fgIdx2]), NFALIGN_LEFT, 0);

				nfui_nfobject_gc_unref(gc);


				if(evt->type == GDK_BUTTON_RELEASE) {
					smt_set_service(SMT_SYSTEM_SETUP);
					switch(f_mIndex) {
						case CAMERA_SUBMENU: 	SystemSetupCam_Open(g_curwnd, NULL, page_idx); 		break;
						case DISPLAY_SUBMENU: 	SystemSetupDisp_Open(g_curwnd, NULL, page_idx); 	break;
						case AUDIO_SUBMENU: 	SystemSetupSound_Open(g_curwnd, NULL, page_idx); 	break;
						case USER_SUBMENU: 		SystemSetupUser_Open(g_curwnd, NULL, page_idx); 	break;
						case NETWORK_SUBMENU: 	SystemSetupNetwork_Open(g_curwnd, NULL, page_idx); 	break;
						case SYSTEM_SUBMENU: 	VW_SetupSystem_Open(g_curwnd, NULL, page_idx); 		break;
						case STORAGE_SUBMENU: 	VW_DiskSetup_Open(g_curwnd, NULL, page_idx); 		break;
						case EVENT_SUBMENU: 	VW_Evt_Act_Open(g_curwnd, NULL, page_idx); 			break;
					}
					smt_set_service(SMT_MAIN_MENU);
				}
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			//		if(kpid == KEYPAD_LEFT) {
			//			nfui_set_key_focus(f_mButton, TRUE);
			//		}
		}
		break;

		default:
		return FALSE;
	}

	return FALSE;
}

static void _get_sub1_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB1_CNT; i++)
    {
        pos = mcf.sys_sub1.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB1_CAMERA_TITLE)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_TITLE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_TITLE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_TITLE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_TITLE_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_IMAGE_SETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_ONVIF_IMAGE_SETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_IMAGE_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_COVERT_SETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_COVERT_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_COVERT_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_COVERT_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_COVERT_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_MOTION_SENSOR)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_ONVIF_MOTION_SENSOR)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_MOTION_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_PTZ_SETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_PTZ_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_PTZ_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_PTZ_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_PTZ_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_PRIVACY_MASK)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_PRIVATE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_PRIVATE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_PRIVATE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_PRIVATE_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_INSTALL_MODE)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_MODE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_MODE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_MODE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_MODE_ICON_N), NULL);
            }
            else if (i == SYS_SUB1_IPCAMERA_INSTALL)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_INSTALL_ICON_N), NULL);
            }
			else if (i == SYS_SUB1_VCA_REV_SETUP_ITX)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_CAMERA_VASETUP_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_CAMERA_VASETUP_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_CAMERA_VASETUP_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_CAMERA_VASETUP_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub2_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB2_CNT; i++)
    {
        pos = mcf.sys_sub2.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB2_OSD)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISP_OSD_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISP_OSD_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISP_OSD_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISP_OSD_ICON_N), NULL);
            }
            else if (i == SYS_SUB2_MONITOR)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISP_MONITOR_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISP_MONITOR_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISP_MONITOR_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISP_MONITOR_ICON_N), NULL);
            }
            else if (i == SYS_SUB2_SEQUENCE)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISP_SEQ_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISP_SEQ_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISP_SEQ_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISP_SEQ_ICON_N), NULL);
            }
            else if (i == SYS_SUB2_SPOTOUT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISP_SPOT_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISP_SPOT_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISP_SPOT_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISP_SPOT_ICON_N), NULL);
            }
			else if(i == SYS_SUB2_POSATM)
			{
				icon[pos][0] = nfui_get_image_from_file((IMG_DISP_POSATM_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISP_POSATM_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISP_POSATM_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISP_POSATM_ICON_N), NULL);
			}
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub3_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB3_CNT; i++)
    {
        pos = mcf.sys_sub3.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB3_AUDIO)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SOUND_AUDIO_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SOUND_AUDIO_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SOUND_AUDIO_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SOUND_AUDIO_ICON_N), NULL);
            }
            else if (i == SYS_SUB3_BUZZER)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SOUND_BUZZER_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SOUND_BUZZER_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SOUND_BUZZER_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SOUND_BUZZER_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub4_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB4_CNT; i++)
    {
        pos = mcf.sys_sub4.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB4_MANAGEMENT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_USER_MANAGE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_USER_MANAGE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_USER_MANAGE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_USER_MANAGE_ICON_N), NULL);
            }
            else if (i == SYS_SUB4_AUTHORITY)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_USER_GROUP_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_USER_GROUP_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_USER_GROUP_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_USER_GROUP_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub5_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB5_CNT; i++)
    {
        pos = mcf.sys_sub5.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB5_IPSETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_IP_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_IP_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_IP_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_IP_ICON_N), NULL);
            }
            else if (i == SYS_SUB5_DDNS)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_DDNS_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_DDNS_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_DDNS_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_DDNS_ICON_N), NULL);
            }
            else if (i == SYS_SUB5_EMAIL)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_EMAIL_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_EMAIL_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_EMAIL_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_EMAIL_ICON_N), NULL);
            }
            else if (i == SYS_SUB5_NETSTAT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_STATUS_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_STATUS_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_STATUS_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_STATUS_ICON_N), NULL);
            }
            else if (i == SYS_SUB5_SECURITY)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_SECURITY_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_SECURITY_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_SECURITY_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_SECURITY_ICON_N), NULL);
            }
			else if (i == SYS_SUB5_SNMP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_SNMP_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_SNMP_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_SNMP_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_SNMP_ICON_N), NULL);
            }
			else if (i == SYS_SUB5_RTP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_RTP_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_RTP_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_RTP_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_RTP_ICON_N), NULL);
            }
			else if (i == SYS_SUB5_CABLE_TEST)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_NET_CABLETEST_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_NET_CABLETEST_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_NET_CABLETEST_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_NET_CABLETEST_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub6_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB6_CNT; i++)
    {
        pos = mcf.sys_sub6.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB6_DATETIME)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_TIME_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_TIME_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_TIME_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_TIME_ICON_N), NULL);
            }
            else if (i == SYS_SUB6_MANAGEMENT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_MANAGE_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_MANAGE_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_MANAGE_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_MANAGE_ICON_N), NULL);
            }
            else if (i == SYS_SUB6_INFORMATION)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_INFO_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_INFO_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_INFO_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_INFO_ICON_N), NULL);
            }
            else if (i == SYS_SUB6_CONTROLDEVICE)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_CONTROL_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_CONTROL_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_CONTROL_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_CONTROL_ICON_N), NULL);
            }
            else if (i == SYS_SUB6_SECURITY)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_SECURITY_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_SECURITY_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_SECURITY_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_SECURITY_ICON_N), NULL);
            }
			else if (i == SYS_SUB6_POSATM)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_SYS_POSATM_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_SYS_POSATM_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_SYS_POSATM_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_SYS_POSATM_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub7_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB7_CNT; i++)
    {
        pos = mcf.sys_sub7.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB7_INFOMATION)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISK_INFO_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISK_INFO_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISK_INFO_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISK_INFO_ICON_N), NULL);
            }
            else if (i == SYS_SUB7_OPERATION)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISK_OPER_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISK_OPER_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISK_OPER_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISK_OPER_ICON_N), NULL);
            }
            else if (i == SYS_SUB7_SMARTSETUP)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISK_SMART_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISK_SMART_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISK_SMART_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISK_SMART_ICON_N), NULL);
            }
			else if (i == SYS_SUB7_CONFIGURATION)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_DISK_CONF_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_DISK_CONF_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_DISK_CONF_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_DISK_CONF_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

static void _get_sub8_icon_image(GdkPixbuf *(*icon)[NFOBJECT_STATE_COUNT])
{
    gint i, pos;

    for (i = 0; i < SYS_SUB8_CNT; i++)
    {
        pos = mcf.sys_sub8.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);

        if (pos != -1)
        {
            if (i == SYS_SUB8_ALARMOUT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_ALARMOUT_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_ALARMOUT_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_ALARMOUT_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_ALARMOUT_ICON_N), NULL);
            }
            else if (i == SYS_SUB8_EVENTNOTI)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_NOTI_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_NOTI_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_NOTI_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_NOTI_ICON_N), NULL);
            }
            else if (i == SYS_SUB8_ALARMSENSOR)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_ALARM_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_ALARM_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_ALARM_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_ALARM_ICON_N), NULL);
            }
            else if (i == SYS_SUB8_MOTIONSENSOR)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_MOTION_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_MOTION_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_MOTION_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_MOTION_ICON_N), NULL);
            }
            else if (i == SYS_SUB8_VIDEOLOSS)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_VLOSS_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_VLOSS_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_VLOSS_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_VLOSS_ICON_N), NULL);
            }
            else if (i == SYS_SUB8_SYSTEMEVENT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_SYSTEM_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_SYSTEM_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_SYSTEM_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_SYSTEM_ICON_N), NULL);
            }
			else if (i == SYS_SUB8_POSATM_EVENT)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_POSATM_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_POSATM_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_POSATM_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_POSATM_ICON_N), NULL);
            }
			else if (i == SYS_SUB8_VCA_EVENT_ITX)
            {
    			icon[pos][0] = nfui_get_image_from_file((IMG_EVT_VA_ICON_N), NULL);
    			icon[pos][1] = nfui_get_image_from_file((IMG_EVT_VA_ICON_F), NULL);
    			icon[pos][2] = nfui_get_image_from_file((IMG_EVT_VA_ICON_F), NULL);
    			icon[pos][3] = nfui_get_image_from_file((IMG_EVT_VA_ICON_N), NULL);
            }
            else
            {
    			icon[pos][0] = NULL;
    			icon[pos][1] = NULL;
    			icon[pos][2] = NULL;
    			icon[pos][3] = NULL;

                g_warning("%s, %d, not supported icon image", __FUNCTION__, __LINE__);
            }
        }
    }
}

gint _get_sub1_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB1_CNT; i++)
    {
        pos = mcf.sys_sub1.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = cam_subScript[i];
    }
    return 0;
}

gint _get_sub2_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB2_CNT; i++)
    {
        pos = mcf.sys_sub2.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = dis_subScript[i];
    }
    return 0;
}

gint _get_sub3_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB3_CNT; i++)
    {
        pos = mcf.sys_sub3.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = snd_subScript[i];
    }
    return 0;
}

gint _get_sub4_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB4_CNT; i++)
    {
        pos = mcf.sys_sub4.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = usr_subScript[i];
    }
    return 0;
}

gint _get_sub5_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB5_CNT; i++)
    {
        pos = mcf.sys_sub5.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = net_subScript[i];
    }
    return 0;
}

gint _get_sub6_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB6_CNT; i++)
    {
        pos = mcf.sys_sub6.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = sys_subScript[i];
    }
    return 0;
}

gint _get_sub7_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB7_CNT; i++)
    {
        pos = mcf.sys_sub7.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = dsk_subScript[i];
    }
    return 0;
}

gint _get_sub8_subscript_str(gchar **subScript)
{
    gint i, pos;

    for (i = 0; i < SYS_SUB8_CNT; i++)
    {
        pos = mcf.sys_sub8.menu_pos[i];
        if (pos >= MAX_TAB_CNT) g_assert(0);
        if(pos != -1) subScript[pos] = evt_subScript[i];
    }
    return 0;
}

static NFOBJECT* create_submenu_fixed(gint submenu)
{
	NFOBJECT *fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	ICONPOSITION pos;

	GdkPixbuf *submenu_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *icon_img[MAX_TAB_CNT][NFOBJECT_STATE_COUNT];

	guint btn_fg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(703), COLOR_IDX(702), COLOR_IDX(702), COLOR_IDX(703)};
	guint tbl_w[] = {375, 375};
	gint i = 0;


	if(submenu == CAMERA_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub1(strSubmenu[submenu]);
    	_get_sub1_icon_image(icon_img);
    	_get_sub1_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == DISPLAY_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub2(strSubmenu[submenu]);
    	_get_sub2_icon_image(icon_img);
    	_get_sub2_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == AUDIO_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub3(strSubmenu[submenu]);
    	_get_sub3_icon_image(icon_img);
    	_get_sub3_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == USER_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub4(strSubmenu[submenu]);
    	_get_sub4_icon_image(icon_img);
    	_get_sub4_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == NETWORK_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub5(strSubmenu[submenu]);
    	_get_sub5_icon_image(icon_img);
    	_get_sub5_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == SYSTEM_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub6(strSubmenu[submenu]);
    	_get_sub6_icon_image(icon_img);
    	_get_sub6_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == STORAGE_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub7(strSubmenu[submenu]);
    	_get_sub7_icon_image(icon_img);
    	_get_sub7_subscript_str(strSubscript[submenu]);
    }
	else if(submenu == EVENT_SUBMENU) {
	    vw_menu_get_str_sys_menu_sub8(strSubmenu[submenu]);
    	_get_sub8_icon_image(icon_img);
    	_get_sub8_subscript_str(strSubscript[submenu]);
    }

	submenu_img[0] = nfui_get_image_from_file((IMG_SUBMENU_01_N), NULL);
	submenu_img[1] = nfui_get_image_from_file((IMG_SUBMENU_01_F), NULL);
	submenu_img[2] = nfui_get_image_from_file((IMG_SUBMENU_01_F), NULL);
	submenu_img[3] = nfui_get_image_from_file((IMG_SUBMENU_01_N), NULL);


	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 750, 524);
	nfui_regi_post_event_callback(fixed, submenu_fixed_post_event_handler);
	nfui_nfobject_show(fixed);

	// table
	tbl = (NFOBJECT*)nfui_nftable_new(2, 4, 0, 0, tbl_w, 131);
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, 0, 0);

	memset(&pos, 0x00, sizeof(ICONPOSITION));
	pos.x = 10;
	pos.y = 25;
	pos.right_margin = 13;

    for(i; i<submenu_cnt[submenu]; i++) {
		obj = nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), submenu_img);
		if (icon_img[i][0]) nfui_nfbutton_set_icon_image(NF_BUTTON(obj), icon_img[i]);
		nfui_nfbutton_set_icon_position(NF_BUTTON(obj), pos);
		nfui_nfobject_set_data(obj, "subBtn idx", GINT_TO_POINTER(i));
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_subButton_event_handler);
		nfui_nftable_attach((NFTABLE*)tbl, obj, (i%2), (i/2));
	}

	return fixed;
}

static void show_submenu(gint submenu)
{
	GdkPixbuf *bgImg;
	GdkDrawable *drawable = NULL;
	GdkGC* gc = NULL;

	if(f_mIndex != submenu) {
//		if(!pbBG) {
//			bgImg = nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);
//			pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 750, 524);
//			gdk_pixbuf_copy_area(bgImg, 693, 284, 750, 524, pbBG, 0, 0);
//		}

//		drawable = nfui_nfobject_get_window(submenu_fixed[submenu]);
//		gc = nfui_nfobject_get_gc(submenu_fixed[submenu]);
//		nfutil_draw_pixbuf(drawable, gc, pbBG, 693, 284, 750, 524, NFALIGN_LEFT, 0);
//		nfui_nfobject_gc_unref(gc);

		nfui_nfobject_hide(submenu_fixed[f_mIndex]);
		nfui_signal_emit(submenu_fixed[f_mIndex], GDK_EXPOSE, TRUE);

		f_mIndex = submenu;

		nfui_nfobject_show(submenu_fixed[submenu]);
		nfui_signal_emit(submenu_fixed[submenu], GDK_EXPOSE, TRUE);


		nfui_make_key_hierarchy(g_curwnd);
	}
}

static gboolean post_cam_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(CAMERA_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			SystemSetupCam_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				NFOBJECT *ntb;
				NFOBJECT *child;
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				//ntb = (NFOBJECT*)g_slist_nth_data(((NFFIXED*)submenu_fixed[f_mIndex])->children, 0);
				//child = nfui_nftable_get_child((NFTABLE*)ntb, 0, 0);
				//if(kpid == KEYPAD_RIGHT) {
				//	nfui_set_key_focus(child, TRUE);
				//}
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disp_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(DISPLAY_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			SystemSetupDisp_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_snd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(AUDIO_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			SystemSetupSound_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_usr_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(USER_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			SystemSetupUser_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_net_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(NETWORK_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			SystemSetupNetwork_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_sys_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(SYSTEM_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			VW_SetupSystem_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_disk_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(STORAGE_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			VW_DiskSetup_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_evt_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_BUTTON_PRESS:
			show_submenu(EVENT_SUBMENU);
			break;

		case GDK_BUTTON_RELEASE:
			if(f_mButton != obj) {
				f_mButton = obj;
				return FALSE;
			}

			smt_set_service(SMT_SYSTEM_SETUP);
			VW_Evt_Act_Open(g_curwnd, NULL, 0);
			smt_set_service(SMT_MAIN_MENU);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				NFOBJECT *ntb;
				NFOBJECT *child;
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				//ntb = (NFOBJECT*)g_slist_nth_data(((NFFIXED*)submenu_fixed[f_mIndex])->children, 0);
				//child = nfui_nftable_get_child((NFTABLE*)ntb, 0, 0);
//				if(kpid == KEYPAD_RIGHT) {
//					nfui_set_key_focus(child, TRUE);
//				}
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean post_exit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		nfui_nfobject_destroy((NFOBJECT*)g_curwnd);

	return FALSE;
}

static gboolean post_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	 if (evt->type == GDK_DELETE) {
		nfui_page_close(PGID_SETUPMENU, obj);
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfutil_draw_image(drawable, gc, IMG_MAINMENU_BG, 0, 0, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

gboolean VW_HoneyWell_MainMenu_Open(NFWINDOW *parent)
{
	NFOBJECT* window;
	NFOBJECT* fixed;
	NFOBJECT* tbl;
	NFOBJECT *obj;

	GdkPixbuf *menu_img[SYS_SUBMENU_CNT][NFOBJECT_STATE_COUNT];
	GdkPixbuf *exit_img[NFOBJECT_STATE_COUNT];

	GSList* mlist;

	guint tbl_w[] = {214};
	guint btn_fg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(701), COLOR_IDX(700), COLOR_IDX(700), COLOR_IDX(701)};
	gint i;

	const gchar *strBtn[] = {"CAMERA",
							"DISPLAY",
							"SOUND",
							"USER",
							"NETWORK",
							"SYSTEM",
							"STORAGE",
							"EVENT"};

	menu_img[CAMERA_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_CAMERA), NULL);
	menu_img[CAMERA_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_CAMERA), NULL);
	menu_img[CAMERA_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_CAMERA), NULL);
	menu_img[CAMERA_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_CAMERA), NULL);

	menu_img[DISPLAY_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_DISPLAY), NULL);
	menu_img[DISPLAY_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_DISPLAY), NULL);
	menu_img[DISPLAY_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_DISPLAY), NULL);
	menu_img[DISPLAY_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_DISPLAY), NULL);

	menu_img[AUDIO_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_SOUND), NULL);
	menu_img[AUDIO_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_SOUND), NULL);
	menu_img[AUDIO_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_SOUND), NULL);
	menu_img[AUDIO_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_SOUND), NULL);

	menu_img[USER_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_USER), NULL);
	menu_img[USER_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_USER), NULL);
	menu_img[USER_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_USER), NULL);
	menu_img[USER_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_USER), NULL);

	menu_img[NETWORK_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_NETWORK), NULL);
	menu_img[NETWORK_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_NETWORK), NULL);
	menu_img[NETWORK_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_NETWORK), NULL);
	menu_img[NETWORK_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_NETWORK), NULL);

	menu_img[SYSTEM_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_SYSTEM), NULL);
	menu_img[SYSTEM_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_SYSTEM), NULL);
	menu_img[SYSTEM_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_SYSTEM), NULL);
	menu_img[SYSTEM_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_SYSTEM), NULL);

	menu_img[STORAGE_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_STORAGE), NULL);
	menu_img[STORAGE_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_STORAGE), NULL);
	menu_img[STORAGE_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_STORAGE), NULL);
	menu_img[STORAGE_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_STORAGE), NULL);

	menu_img[EVENT_SUBMENU][0] = nfui_get_image_from_file((IMG_SUBMENU_N_EVENT), NULL);
	menu_img[EVENT_SUBMENU][1] = nfui_get_image_from_file((IMG_SUBMENU_O_EVENT), NULL);
	menu_img[EVENT_SUBMENU][2] = nfui_get_image_from_file((IMG_SUBMENU_F_EVENT), NULL);
	menu_img[EVENT_SUBMENU][3] = nfui_get_image_from_file((IMG_SUBMENU_N_EVENT), NULL);



	// window
	window = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, WIN_SIZE_W, WIN_SIZE_H);
	nfui_nfwindow_set_title((NFWINDOW*)window, "SYSTEM SETUP");
	nfui_regi_post_event_callback(window, post_wnd_event_handler);
	g_curwnd = (NFWINDOW*)window;


	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_SIZE_W, WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	// left menu
	tbl = (NFOBJECT*)nfui_nftable_new(1, 8, 1, 1, tbl_w, 59);
	//nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)fixed, tbl, 458, 219);



	for(i=0; i<SYS_SUBMENU_CNT; i++) {
		obj = nfui_nfbutton_new_with_param(menu_img[i], strBtn[i]);
		nfui_nfbutton_set_drawing_outline(NF_BUTTON(obj), FALSE);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
		nfui_nfobject_set_size(obj, 214, 59);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

		if(i == CAMERA_SUBMENU)	mlist = nfui_radio_button_get_group((NFBUTTON*)obj);
		else					nfui_radio_button_add_group((NFBUTTON*)obj, mlist);

		if(i == CAMERA_SUBMENU) nfui_radio_button_set_toggled((NFBUTTON*)obj, TRUE);
		if(i == CAMERA_SUBMENU) f_mButton = obj;

		if (i == CAMERA_SUBMENU)		 nfui_regi_post_event_callback(obj, post_cam_button_event_handler);
		else if (i == DISPLAY_SUBMENU)  nfui_regi_post_event_callback(obj, post_disp_button_event_handler);
		else if (i == AUDIO_SUBMENU)    nfui_regi_post_event_callback(obj, post_snd_button_event_handler);
		else if (i == USER_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_usr_button_event_handler);
		else if (i == NETWORK_SUBMENU)  nfui_regi_post_event_callback(obj, post_net_button_event_handler);
		else if (i == SYSTEM_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_sys_button_event_handler);
		else if (i == STORAGE_SUBMENU)  nfui_regi_post_event_callback(obj, post_disk_button_event_handler);
		else if (i == EVENT_SUBMENU) 	 nfui_regi_post_event_callback(obj, post_evt_button_event_handler);
	}


	// right menu page
    submenu_cnt[CAMERA_SUBMENU] = mcf.sys_sub1.cnt;
    submenu_cnt[DISPLAY_SUBMENU] = mcf.sys_sub2.cnt;
    submenu_cnt[AUDIO_SUBMENU] = mcf.sys_sub3.cnt;
    submenu_cnt[USER_SUBMENU] = mcf.sys_sub4.cnt;
    submenu_cnt[NETWORK_SUBMENU] = mcf.sys_sub5.cnt;
    submenu_cnt[SYSTEM_SUBMENU] = mcf.sys_sub6.cnt;
    submenu_cnt[STORAGE_SUBMENU] = mcf.sys_sub7.cnt;
    submenu_cnt[EVENT_SUBMENU] = mcf.sys_sub8.cnt;

	for(i=0; i<SYS_SUBMENU_CNT; i++) {
		submenu_fixed[i] = create_submenu_fixed(i);
		nfui_nffixed_put((NFFIXED*)fixed, submenu_fixed[i], 693, 224);

		if(i != CAMERA_SUBMENU)
			nfui_nfobject_hide(submenu_fixed[i]);
	}


	// exit button
	exit_img[0] = nfui_get_image_from_file((IMG_MAINMENU_N_EXIT), NULL);
	exit_img[1] = nfui_get_image_from_file((IMG_MAINMENU_F_EXIT), NULL);
	exit_img[2] = nfui_get_image_from_file((IMG_MAINMENU_F_EXIT), NULL);
	exit_img[3] = nfui_get_image_from_file((IMG_MAINMENU_N_EXIT), NULL);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, exit_img);
	nfui_nfobject_set_size(obj, 17, 17);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_exit_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 1442, 171);


	nfui_nfwindow_add((NFWINDOW*)window, fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_set_key_focus(f_mButton, TRUE);
	nfui_make_key_hierarchy((NFWINDOW*)window);

	nfui_page_open(PGID_SETUPMENU, window, ssm_get_cur_id(NULL));

	gtk_main();

	return TRUE;
}