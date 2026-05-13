#include <string.h>

#include "nf_afx.h"
#include "scm.h"

#include "vw_codec_data_translate.h"


////////////////////////////////////////////////////////////
//
// private variable
//

#define _MASK(a)	(1ULL << (a))

const gchar* g_resol_info[] = 
{	
    "352x240",		//B     (0)
	"704x240",		//C     (1)
	"704x480",		//D     (2)
	"704x480P",		//E     (3)
	"352x288",		//F     (4)
	"704x288",		//G     (5)
	"704x576",		//H     (6)
	"704x576P",		//I     (7)									
	"640x480",		//J     (8)
	"720x480",		//K     (9)
	"720x576",		//L     (10)
	"800x600",		//M     (11)
	"1024x768",		//N     (12)
	"1280x1024",	//O     (13)
	"1600x1200",	//P     (14)
	"1280x720",		//Q     (15)
	"1920x1080",	//R     (16)
	"640x352",		//S     (17)
	"640x360",		//T     (18)
	"640x360I",		//U     (19)
	"1280x720I",	//V     (20)
	"1920x1280I",	//W     (21)
	"640x400",	    //X     (22)
	"800x450",	    //Y     (23)
	"1440x900",	    //Z     (24)
	"960x480",	    //a     (25)
	"960x576",	    //b     (26)
	"320x180",	    //c     (27)
	"2304x1296",	//d     (28)	
	"2048x1536",	//e     (29)	
	"2560x1440",	//f     (30)	
	"2688x1520",	//g     (31)	
	"2560x1600",	//h     (32)	
	"2560x1920",	//i     (33)	
	"2592x1920",	//j     (34)	
	"2592x1944",	//k     (35)	
	"2992x1680",	//l     (36)	
	"2880x1800",	//m     (37)	
	"3200x1800",	//n     (38)	
	"2880x2160",	//o     (39)	
	"3072x2048",	//p     (40)	
	"3200x2400",	//q     (41)	
	"3840x2160",	//r     (42)	
	"2592x1520",	//s     (43)	
	"3000x3000",	//1     (44)	
	"2048x2048",	//2     (45)	
	"1280x1280",	//3     (46)	
	"640x640",		//4     (47)	
	"320x320",		//5     (48)	

	"UNKNOWN",	    // error
};
						
const gchar* g_fps_info[] = 
{	
    "30",			//A     (0)
    "25",			//A     (1)
    "15",			//B     (2)
    "12",			//B     (3)
    "7",			//C     (4)
    "6",			//C     (5)
    "4",			//D     (6)
    "3",			//D     (7)
    "2",			//E     (8)
    "1",			//F     (9)
    "0",			//G     (10)
    "UNKNOWN",	    // error
};

const gchar* g_quality_info[] = 
{	
    "HIGHEST",		//A
    "HIGH",			//B
    "STANDARD",		//C
    "LOW",			//D
    "LOWEST",		//E
};


////////////////////////////////////////////////////////////
//
// public interfaces
//

gchar* vw_codec_get_resol_info(gchar data)
{
	guint main_cap, main_cur;
	guint sec_cap, sec_cur;
	guint result;
	guint vloss_val;

	g_return_val_if_fail(data, NULL);

	switch(data) {
		case 'B':
			return g_resol_info[CAP_RES_NTSC_CIF];
		case 'C':
			return g_resol_info[CAP_RES_NTSC_2CIF];
		case 'D':
			return g_resol_info[CAP_RES_NTSC_4CIF];
		case 'E':
			return g_resol_info[CAP_RES_NTSC_4CIFP];
		case 'F':
			return g_resol_info[CAP_RES_PAL_CIF];
		case 'G':
			return g_resol_info[CAP_RES_PAL_2CIF];
		case 'H':
			return g_resol_info[CAP_RES_PAL_4CIF];
		case 'I':
			return g_resol_info[CAP_RES_PAL_4CIFP];
		case 'J':
			return g_resol_info[CAP_RES_640x480];
		case 'K':
			return g_resol_info[CAP_RES_720x480];
		case 'L':
			return g_resol_info[CAP_RES_720x576];
		case 'M':
			return g_resol_info[CAP_RES_800x600];
		case 'N':
			return g_resol_info[CAP_RES_1024x768];
		case 'O':
			return g_resol_info[CAP_RES_1280x1024];
		case 'P':
			return g_resol_info[CAP_RES_1600x1200];
		case 'Q':
			return g_resol_info[CAP_RES_1280x720];
		case 'R':
			return g_resol_info[CAP_RES_1920x1080];
		case 'S':
			return g_resol_info[CAP_RES_640x352];
		case 'T':
			return g_resol_info[CAP_RES_640x360];
		case 'U':
			return g_resol_info[CAP_RES_640x360I];
		case 'V':
			return g_resol_info[CAP_RES_1280x720I];			
		case 'W':
			return g_resol_info[CAP_RES_1920x1080I];
		case 'X':
			return g_resol_info[CAP_RES_640x400];		
		case 'Y':
			return g_resol_info[CAP_RES_800x450];		
		case 'Z':
			return g_resol_info[CAP_RES_1440x900];		
		case 'a':
			return g_resol_info[CAP_RES_960x480];		
		case 'b':
			return g_resol_info[CAP_RES_960x576];					
		case 'c':
			return g_resol_info[CAP_RES_320x180];
		case 'd':
			return g_resol_info[CAP_RES_2304x1296];
		case 'e':
			return g_resol_info[CAP_RES_2048x1536];
		case 'f':
			return g_resol_info[CAP_RES_2560x1440];
		case 'g':
			return g_resol_info[CAP_RES_2688x1520];
		case 'h':
			return g_resol_info[CAP_RES_2560x1600];
		case 'i':
			return g_resol_info[CAP_RES_2560x1920];
		case 'j':
			return g_resol_info[CAP_RES_2592x1920];
		case 'k':
			return g_resol_info[CAP_RES_2592x1944];
		case 'l':
			return g_resol_info[CAP_RES_2992x1680];
		case 'm':
			return g_resol_info[CAP_RES_2880x1800];
		case 'n':
			return g_resol_info[CAP_RES_3200x1800];
		case 'o':
			return g_resol_info[CAP_RES_2880x2160];
		case 'p':
			return g_resol_info[CAP_RES_3072x2048];
		case 'q':
			return g_resol_info[CAP_RES_3200x2400];
		case 'r':
			return g_resol_info[CAP_RES_3840x2160];			
		case 's':
			return g_resol_info[CAP_RES_2592x1520];			
		case '1':
			return g_resol_info[CAP_RES_3000x3000];			
		case '2':
			return g_resol_info[CAP_RES_2048x2048];			
		case '3':
			return g_resol_info[CAP_RES_1280x1280];			
		case '4':
			return g_resol_info[CAP_RES_640x640];			
		case '5':
			return g_resol_info[CAP_RES_320x320];															
		default :
		    g_message("%s, %d, data:%c", __FUNCTION__, __LINE__, data);
			iassert(0);
			break;
	}

	return NULL;
}

gchar* vw_codec_get_fps_info(gchar data, gint is_ntsc)
{
	g_return_val_if_fail(data, NULL);

	if (is_ntsc)
	{
		switch(data) {
			case 'A':
				return g_fps_info[CAP_FPS_30];
			case 'B':
				return g_fps_info[CAP_FPS_15];
			case 'C':
				return g_fps_info[CAP_FPS_07];
			case 'D':
				return g_fps_info[CAP_FPS_03];
			case 'E':
				return g_fps_info[CAP_FPS_02];
			case 'F':
				return g_fps_info[CAP_FPS_01];
			case 'G':
				return g_fps_info[CAP_FPS_00];
			default :
				g_assert(0);
				break;		
		}
	}
	else
	{
		switch(data) {
			case 'A':
				return g_fps_info[CAP_FPS_25];
			case 'B':
				return g_fps_info[CAP_FPS_12];
			case 'C':
				return g_fps_info[CAP_FPS_06];
			case 'D':
				return g_fps_info[CAP_FPS_03];
			case 'E':
				return g_fps_info[CAP_FPS_02];
			case 'F':
				return g_fps_info[CAP_FPS_01];
			case 'G':
				return g_fps_info[CAP_FPS_00];
			default :
				g_assert(0);
				break;			
		}		
	}

	return NULL;
}

gchar* vw_codec_get_quality_info(gchar data)
{
	g_return_val_if_fail(data, NULL);
	
	switch(data) {
		case 'A':
			return g_quality_info[0];
		case 'B':
			return g_quality_info[1];
		case 'C':
			return g_quality_info[2];
		case 'D':
			return g_quality_info[3];
		case 'E':
			return g_quality_info[4];
		default :
			return NULL;					
	}

	return NULL;
}

gchar vw_codec_get_resol_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_NTSC_CIF])) 			return 'B';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_NTSC_2CIF])) 	return 'C';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_NTSC_4CIF])) 	return 'D';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_NTSC_4CIFP])) 	return 'E';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_PAL_CIF])) 		return 'F';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_PAL_2CIF])) 	    return 'G';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_PAL_4CIF])) 	    return 'H';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_PAL_4CIFP])) 	return 'I';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x480])) 		return 'J';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_720x480])) 		return 'K';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_720x576])) 		return 'L';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_800x600])) 		return 'M';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1024x768])) 	    return 'N';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1280x1024])) 	return 'O';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1600x1200])) 	return 'P';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1280x720])) 	    return 'Q';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1920x1080])) 	return 'R';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x352])) 		return 'S';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x360])) 		return 'T';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x360I])) 	    return 'U';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1280x720I])) 	return 'V';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1920x1080I])) 	return 'W';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x400])) 	    return 'X';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_800x450])) 	    return 'Y';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1440x900])) 	    return 'Z';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_960x480])) 	    return 'a';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_960x576])) 	    return 'b';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_320x180])) 	    return 'c';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2304x1296])) 	return 'd';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2048x1536])) 	return 'e';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2560x1440])) 	return 'f';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2688x1520])) 	return 'g';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2560x1600])) 	return 'h';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2560x1920])) 	return 'i';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2592x1920])) 	return 'j';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2592x1944])) 	return 'k';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2992x1680])) 	return 'l';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2880x1800])) 	return 'm';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_3200x1800])) 	return 'n';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2880x2160])) 	return 'o';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_3072x2048])) 	return 'p';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_3200x2400])) 	return 'q';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_3840x2160])) 	return 'r';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2592x1520])) 	return 's';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_3000x3000])) 	return '1';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_2048x2048])) 	return '2';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_1280x1280])) 	return '3';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_640x640])) 		return '4';
	else if(!g_ascii_strcasecmp(info, g_resol_info[CAP_RES_320x320])) 		return '5';				

	return '\0';
}

gchar vw_codec_get_fps_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, "1")) return 'F';
	else if(!g_ascii_strcasecmp(info, "2")) return 'E';
	else if(!g_ascii_strcasecmp(info, "3")) return 'D';
	else if(!g_ascii_strcasecmp(info, "6")) return 'C';
	else if(!g_ascii_strcasecmp(info, "7")) return 'C';
	else if(!g_ascii_strcasecmp(info, "12")) return 'B';
	else if(!g_ascii_strcasecmp(info, "15")) return 'B';
	else if(!g_ascii_strcasecmp(info, "25")) return 'A';
	else if(!g_ascii_strcasecmp(info, "30")) return 'A';		
	else if(!g_ascii_strcasecmp(info, "0")) return 'G';	

	return '\0';
}

gchar vw_codec_get_quality_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, "LOWEST")) return 'E';
	else if(!g_ascii_strcasecmp(info, "LOW")) return 'D';
	else if(!g_ascii_strcasecmp(info, "STANDARD")) return 'C';
	else if(!g_ascii_strcasecmp(info, "HIGH")) return 'B';
	else if(!g_ascii_strcasecmp(info, "HIGHEST")) return 'A';
	
	return '\0';
}

void vw_codec_translate_capable_data_resol(guint64 capable, gchar *buf)
{
	gint cnt = 0;

	if (capable & _MASK(CAP_RES_3840x2160)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_3840x2160]);
		capable &= ~(_MASK(CAP_RES_3840x2160));				
	}	

	if (capable & _MASK(CAP_RES_3200x2400)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_3200x2400]);
		capable &= ~(_MASK(CAP_RES_3200x2400));				
	}	

	if (capable & _MASK(CAP_RES_3072x2048)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_3072x2048]);
		capable &= ~(_MASK(CAP_RES_3072x2048));				
	}	

	if (capable & _MASK(CAP_RES_2880x2160)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2880x2160]);
		capable &= ~(_MASK(CAP_RES_2880x2160));				
	}	

	if (capable & _MASK(CAP_RES_3200x1800)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_3200x1800]);
		capable &= ~(_MASK(CAP_RES_3200x1800));				
	}	

	if (capable & _MASK(CAP_RES_2880x1800)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2880x1800]);
		capable &= ~(_MASK(CAP_RES_2880x1800));				
	}	

	if (capable & _MASK(CAP_RES_2992x1680)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2992x1680]);
		capable &= ~(_MASK(CAP_RES_2992x1680));				
	}	

	if (capable & _MASK(CAP_RES_2592x1944)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2592x1944]);
		capable &= ~(_MASK(CAP_RES_2592x1944));				
	}	

	if (capable & _MASK(CAP_RES_2592x1920)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2592x1920]);
		capable &= ~(_MASK(CAP_RES_2592x1920));				
	}	

	if (capable & _MASK(CAP_RES_2560x1920)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2560x1920]);
		capable &= ~(_MASK(CAP_RES_2560x1920));				
	}	
	
	if (capable & _MASK(CAP_RES_2560x1600)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2560x1600]);
		capable &= ~(_MASK(CAP_RES_2560x1600));				
	}	

	if (capable & _MASK(CAP_RES_2688x1520)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2688x1520]);
		capable &= ~(_MASK(CAP_RES_2688x1520));				
	}	

	if (capable & _MASK(CAP_RES_2592x1520)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2592x1520]);
		capable &= ~(_MASK(CAP_RES_2592x1520));				
	}	

	if (capable & _MASK(CAP_RES_2560x1440)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2560x1440]);
		capable &= ~(_MASK(CAP_RES_2560x1440));				
	}	

	if (capable & _MASK(CAP_RES_2048x1536)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2048x1536]);
		capable &= ~(_MASK(CAP_RES_2048x1536));				
	}	

	if (capable & _MASK(CAP_RES_2304x1296)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2304x1296]);
		capable &= ~(_MASK(CAP_RES_2304x1296));				
	}	

	if (capable & _MASK(CAP_RES_1920x1080I)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1920x1080I]);
		capable &= ~(_MASK(CAP_RES_1920x1080I));				
	}	
	
	if (capable & _MASK(CAP_RES_1920x1080)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1920x1080]);
		capable &= ~(_MASK(CAP_RES_1920x1080));				
	}

	if (capable & _MASK(CAP_RES_1600x1200)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1600x1200]);
		capable &= ~(_MASK(CAP_RES_1600x1200));				
	}

	if (capable & _MASK(CAP_RES_1440x900)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1440x900]);
		capable &= ~(_MASK(CAP_RES_1440x900));				
	}

	if (capable & _MASK(CAP_RES_1280x1024)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1280x1024]);
		capable &= ~(_MASK(CAP_RES_1280x1024));				
	}

	if (capable & _MASK(CAP_RES_1280x720I)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1280x720I]);
		capable &= ~(_MASK(CAP_RES_1280x720I));				
	}

	if (capable & _MASK(CAP_RES_1280x720)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1280x720]);
		capable &= ~(_MASK(CAP_RES_1280x720));				
	}

	if (capable & _MASK(CAP_RES_1024x768)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1024x768]);
		capable &= ~(_MASK(CAP_RES_1024x768));				
	}

	if (capable & _MASK(CAP_RES_960x576)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_960x576]);
		capable &= ~(_MASK(CAP_RES_960x576));				
	}

	if (capable & _MASK(CAP_RES_960x480)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_960x480]);
		capable &= ~(_MASK(CAP_RES_960x480));				
	}	

	if (capable & _MASK(CAP_RES_800x600)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_800x600]);
		capable &= ~(_MASK(CAP_RES_800x600));				
	}

	if (capable & _MASK(CAP_RES_800x450)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_800x450]);
		capable &= ~(_MASK(CAP_RES_800x450));				
	}

	if (capable & _MASK(CAP_RES_640x400)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x400]);
		capable &= ~(_MASK(CAP_RES_640x400));				
	}

	if (capable & _MASK(CAP_RES_720x576)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_720x576]);
		capable &= ~(_MASK(CAP_RES_720x576));				
	}

	if (capable & _MASK(CAP_RES_720x480)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_720x480]);
		capable &= ~(_MASK(CAP_RES_720x480));				
	}
	
	if (capable & _MASK(CAP_RES_640x480)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x480]);
		capable &= ~(_MASK(CAP_RES_640x480));				
	}

	if (capable & _MASK(CAP_RES_640x360I)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x360I]);
		capable &= ~(_MASK(CAP_RES_640x360I));				
	}
	
	if (capable & _MASK(CAP_RES_640x360)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x360]);
		capable &= ~(_MASK(CAP_RES_640x360));				
	}
	
	if (capable & _MASK(CAP_RES_640x352)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x352]);
		capable &= ~(_MASK(CAP_RES_640x352));				
	}
	
	if (capable & _MASK(CAP_RES_NTSC_4CIFP)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_NTSC_4CIFP]);
		capable &= ~(_MASK(CAP_RES_NTSC_4CIFP));				
	}
	
	if (capable & _MASK(CAP_RES_NTSC_4CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_NTSC_4CIF]);
		capable &= ~(_MASK(CAP_RES_NTSC_4CIF));				
	}
	
	if (capable & _MASK(CAP_RES_NTSC_2CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_NTSC_2CIF]);
		capable &= ~(_MASK(CAP_RES_NTSC_2CIF));				
	}
	
	if (capable & _MASK(CAP_RES_NTSC_CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_NTSC_CIF]);
		capable &= ~(_MASK(CAP_RES_NTSC_CIF));				
	}
	
	if (capable & _MASK(CAP_RES_PAL_4CIFP)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_PAL_4CIFP]);
		capable &= ~(_MASK(CAP_RES_PAL_4CIFP));				
	}
	
	if (capable & _MASK(CAP_RES_PAL_4CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_PAL_4CIF]);
		capable &= ~(_MASK(CAP_RES_PAL_4CIF));				
	}
	
	if (capable & _MASK(CAP_RES_PAL_2CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_PAL_2CIF]);
		capable &= ~(_MASK(CAP_RES_PAL_2CIF));				
	}
	
	if (capable & _MASK(CAP_RES_PAL_CIF)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_PAL_CIF]);
		capable &= ~(_MASK(CAP_RES_PAL_CIF));				
	}
	
	if (capable & _MASK(CAP_RES_320x180)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_320x180]);
		capable &= ~(_MASK(CAP_RES_320x180));				
	}
	
	if (capable & _MASK(CAP_RES_3000x3000)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_3000x3000]);
		capable &= ~(_MASK(CAP_RES_3000x3000));				
	}	

	if (capable & _MASK(CAP_RES_2048x2048)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_2048x2048]);
		capable &= ~(_MASK(CAP_RES_2048x2048));				
	}

	if (capable & _MASK(CAP_RES_1280x1280)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_1280x1280]);
		capable &= ~(_MASK(CAP_RES_1280x1280));				
	}

	if (capable & _MASK(CAP_RES_640x640)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_640x640]);
		capable &= ~(_MASK(CAP_RES_640x640));				
	}

	if (capable & _MASK(CAP_RES_320x320)) {
		buf[cnt++] = vw_codec_get_resol_data(g_resol_info[CAP_RES_320x320]);
		capable &= ~(_MASK(CAP_RES_320x320));				
	}			

	if (capable) {
		g_warning("%s, %d, capable:%08X, undefined size info", __FUNCTION__, __LINE__, capable);
//		g_assert(0);
	}
}

void vw_codec_translate_capable_data_fps(guint capable, gchar *buf)
{
	gint cnt = 0;

	if (capable & _MASK(CAP_FPS_30)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_30]);
		capable &= ~(_MASK(CAP_FPS_30));				
	}

	if (capable & _MASK(CAP_FPS_25)){
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_25]);
		capable &= ~(_MASK(CAP_FPS_25));				
	}
	
	if (capable & _MASK(CAP_FPS_15)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_15]);
		capable &= ~(_MASK(CAP_FPS_15));				
	}

	if (capable & _MASK(CAP_FPS_12)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_12]);
		capable &= ~(_MASK(CAP_FPS_12));
	}

	if (capable & _MASK(CAP_FPS_07)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_07]);
		capable &= ~(_MASK(CAP_FPS_07));
	}

	if (capable & _MASK(CAP_FPS_06)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_06]);
		capable &= ~(_MASK(CAP_FPS_06));
	}

	if (capable & _MASK(CAP_FPS_03)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_03]);
		capable &= ~(_MASK(CAP_FPS_03));
	}

	if (capable & _MASK(CAP_FPS_02)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_02]);
		capable &= ~(_MASK(CAP_FPS_02));
	}

	if (capable & _MASK(CAP_FPS_01)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_01]);
		capable &= ~(_MASK(CAP_FPS_01));
	}

	if (capable & _MASK(CAP_FPS_00)) {
		buf[cnt++] = vw_codec_get_fps_data(g_fps_info[CAP_FPS_00]);
		capable &= ~(_MASK(CAP_FPS_00));
	}

	if (capable) {
		g_warning("%s, %d, capable:%08X, undefined fps info", __FUNCTION__, __LINE__, capable);
//			g_assert(0);
	}
}

