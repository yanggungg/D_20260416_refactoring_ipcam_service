#include <glib.h>

#include "nf_common.h"
#include "jbshell.h"

#include "nf_edid.h"
#include <itx_edid.h>		// in driver/edid/itx_edid.h
#include "nf_edid_parse.h"

/** Global Variable Definition **/
const u_char edid_v1_header[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
const u_char edid_v1_descriptor_flag[] = { 0x00, 0x00 };
const char _edid_est_timing[24][20] =
{
	"720 x 400 @ 70Hz",
	"720 x 400 @ 88Hz",
	"640 x 480 @ 60Hz",
	"640 x 480 @ 67Hz",
	"640 x 480 @ 72Hz",
	"640 x 480 @ 75Hz",
	"800 x 600 @ 56Hz",
	"800 x 600 @ 60Hz",
	"800 x 600 @ 72Hz",
	"800 x 600 @ 75Hz",
	"832 x 624 @ 75Hz",
	"1024 x 768 @ 87Hz",
	"1024 x 768 @ 60Hz",
	"1024 x 768 @ 70Hz",
	"1024 x 768 @ 75Hz",
	"1280 x 1024 @ 75Hz",
	"1152 x 870 @ 75Hz",
	"Manufacture Timing1",
	"Manufacture Timing2",
	"Manufacture Timing3",
	"Manufacture Timing4",
	"Manufacture Timing5",
	"Manufacture Timing6",
	"Manufacture Timing7",
};

const char _edid_vd_timing[EDID_CEA_EDID_MAX][EDID_CEA_EDID_MAX_STR] = {
    "",                                         // 0
    "640 x 480 p @ 60Hz 4:3",                   // 1
    "720 x 480 p @ 60Hz 4:3",                   // 2
    "720 x 480 p @ 60Hz 16:9",                  // 3
    "1280 x 720 p @ 60Hz 16:9",                 // 4
    "1920 x 1080 i @ 60Hz 16:9",                // 5
    "720(1440) x 480 i @ 60Hz 4:3",             // 6
    "720(1440) x 480 i @ 60Hz 16:9",            // 7
    "720(1440) x 240 p @ 60Hz 4:3",             // 8
    "720(1440) x 240 p @ 60Hz 16:9",            // 9
    "(2880) x 480 i @ 60Hz 4:3",                // 10
    "(2880) x 480 i @ 60Hz 16:9",               // 11
    "(2880) x 240 p @ 60Hz 4:3",                // 12
    "(2880) x 240 p @ 60Hz 16:9",               // 13
    "1440 x 480 p @ 60Hz 4:3",                  // 14
    "1440 x 480 p @ 60Hz 16:9",                 // 15
    "1920 x 1080 p @ 60Hz 16:9",                // 16
    "720 x 576 p @ 50Hz 4:3",                   // 17
    "720 x 576 p @ 50Hz 16:9",                  // 18
    "1280 x 720 p @ 50Hz 16:9",                 // 19
    "1920 x 1080 i @ 50Hz 16:9",                // 20
    "720(1440) x 576 i @ 50Hz 4:3",             // 21
    "720(1440) x 576 i @ 50Hz 16:9",            // 22
    "720(1440) x 288 p @ 50Hz 4:3",             // 23
    "720(1440) x 288 p @ 50Hz 16:9",            // 24
    "(2880) x 576 i @ 50Hz 4:3",                // 25
    "(2880) x 576 i @ 50Hz 16:9",               // 26
    "(2880) x 288 p @ 50Hz 4:3",                // 27
    "(2880) x 288 p @ 50Hz 16:9",               // 28
    "1440 x 576 p @ 50Hz 4:3",                  // 29
    "1440 x 576 p @ 50Hz 16:9",                 // 30
    "1920 x 1080 p @ 50Hz 16:9",                // 31
    "1920 x 1080 p @ 24Hz 16:9",                // 32
    "1920 x 1080 p @ 25Hz 16:9",                // 33
    "1920 x 1080 p @ 30Hz 16:9",                // 34
    /** Added pakkhman.. Actually Not Need Currently **/
    "(2880)x480p @ 59.94/60Hz 4:3",             // 35
    "(2880)x480p @ 59.94/60Hz 16:9",            // 36
    "(2880)x576p @ 50Hz 4:3",                   // 37
    "(2880)x576p @ 50Hz 16:9",                  // 38
    "1920x1080i(1250 Total) @ 50Hz* 16:9",      // 39
    "1920x1080i @ 100Hz 16:9",                  // 40
    "1280x720p @ 100Hz 16:9 ",                  // 41
    "720x576p @ 100Hz 4:3",                     // 42
    "720x576p @ 100Hz 16:9",                    // 43
    "720(1440)x576i @ 100Hz 4:3",               // 44
    "720(1440)x576i @ 100Hz 16:9",              // 45
    "1920x1080i @ 119.88/120Hz 16:9",           // 46
    "1280x720p @ 119.88/120Hz 16:9",            // 47
    "720x480p @ 119.88/120Hz 4:3",              // 48
    "720x480p @ 119.88/120Hz 16:9",             // 49
    "720(1440)x480i @ 119.88/120Hz 4:3",        // 50
    "720(1440)x480i @ 119.88/120Hz 16:9",       // 51
    "720x576p @ 200Hz 4:3",                     // 52
    "720x576p @ 200Hz 16:9",                    // 53
    "720(1440)x576i @ 200Hz 4:3",               // 54
    "720(1440)x576i @ 200Hz 16:9",              // 55
    "720x480p @ 239.76/240Hz 4:3",              // 56
    "720x480p @ 239.76/240Hz 16:9",             // 57
    "720(1440)x480i @ 239.76/240Hz 4:3",        // 58
    "720(1440)x480i @ 239.76/240Hz 16:9",       // 59
    "1280x720p @ 23.98/24Hz 16:9",              // 60
    "1280x720p @ 25Hz 16:9",                    // 61
    "1280x720p @ 29.97/30Hz 16:9",              // 62
    "1920x1080p @ 119.88/120Hz 16:9"            // 63
};

NF_EDID_SUPPORT_RESOLUTION	_nf_edid_info;

static gboolean	_nf_edid_dbg=FALSE;

/** Global Function Definition **/

/**
	Function Start
**/
gboolean nf_edid_p_chk_valid(struct edid_data *info, gint resol, gboolean is_pal)
{
	NF_EDID_SUPPORT_RESOLUTION info_res;
	gint i=0;

	memset(&info_res, 0x0, sizeof(NF_EDID_SUPPORT_RESOLUTION));

	if(_nf_edid_dbg)
		nf_edid_p_print_raw_data(info->raw_data);

	if(_nf_edid_dbg)
		g_message("%s Monitor Name --> %s", __FUNCTION__, nf_edid_p_get_monitor_name(info->raw_data));

	nf_edid_p_chk_timing_detailed(info->raw_data, &info_res, is_pal);
	nf_edid_p_chk_timing_standard(info->raw_data, &info_res);
	nf_edid_p_chk_timing_established(info->raw_data, &info_res);
	nf_edid_p_add_detailed_info_eedid(info->raw_data, &info_res, is_pal);

	#if 1
		printf("%s line%d\n", __FUNCTION__, __LINE__);
		printf("is_1280_1024_60 [%d] is_720p30  [%d] is_720p25  [%d] is_720p60  [%d]\n"
			   "is_720p50       [%d] is_1080p60 [%d] is_1080p50 [%d] is_1080p30 [%d]\n"
			   "is_1080p30      [%d] is_1080p25 [%d]\n",
				info_res.is_1280_1024_60, info_res.is_720p30, info_res.is_720p25, info_res.is_720p60,
				info_res.is_720p50, info_res.is_1080p60, info_res.is_1080p50, info_res.is_1080p30,
				info_res.is_1080p30, info_res.is_1080p25);

		#if defined(ENABLE_DISPLAY_2160P)
			printf("is_2160p30 [%d] is_2160p25  [%d] is_2160p50[%d] is_2160p60[%d]\n", 
					info_res.is_2160p30, info_res.is_2160p25, info_res.is_2160p50, info_res.is_2160p60);
		#endif
		printf("is_1440p30  [%d] is_1440p60[%d] is_1600p60[%d]\n",
				info_res.is_1440p30, info_res.is_1440p60, info_res.is_1600p60);
	#endif

	if(resol == NF_EDID_RES_1280_1024_60)
	{
		if(info_res.is_1280_1024_60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_30)
	{
		if(info_res.is_720p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_25)
	{
		if(info_res.is_720p25) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_60)
	{
		if(info_res.is_720p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_720P_50)
	{
		if(info_res.is_720p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_60)
	{
		if(info_res.is_1080p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_50)
	{
		if(info_res.is_1080p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_30)
	{
		if(info_res.is_1080p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1080P_25)
	{
		if(info_res.is_1080p25) return TRUE;
		else return FALSE;
	}
	#if defined(ENABLE_DISPLAY_2160P)
	else if(resol == NF_EDID_RES_2160P_30)
	{
		if(info_res.is_2160p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_25)
	{
		if(info_res.is_2160p25) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_50)
	{
		if(info_res.is_2160p50) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_2160P_60)
	{
		if(info_res.is_2160p60) return TRUE;
		else return FALSE;
	}
	#else // Not Support In ANF5HG & UTM5HG
	else if(resol == NF_EDID_RES_2160P_30)
		return FALSE;
	else if(resol == NF_EDID_RES_2160P_25)
		return FALSE;
	else if(resol == NF_EDID_RES_2160P_50)
		return FALSE;
	else if(resol == NF_EDID_RES_2160P_60)
		return FALSE;
	#endif
	else if(resol == NF_EDID_RES_1440P_30)
	{
		if(info_res.is_1440p30) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1440P_60)
	{
		if(info_res.is_1440p60) return TRUE;
		else return FALSE;
	}
	else if(resol == NF_EDID_RES_1600P_60)
	{
		if(info_res.is_1600p60) return TRUE;
		else return FALSE;
	}
	else
	{
		g_warning("%s Undefined Resolution!! %d", __FUNCTION__, resol);
		return FALSE;
	}
}

gboolean nf_edid_p_chk_timing_detailed(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info, gboolean is_pal)
{
	guchar *block=NULL;
	gint i=0;

	block=(data + EDID_DETAILED_TIMING_DESCRIPTIONS_START);

	g_message("========== Check Detailed Timing ==========");
	for(i=0; i<EDID_NO_DETAILED_TIMING_DESCRIPTIONS; i++, block+=EDID_DETAILED_TIMING_DESCRIPTION_SIZE)
	{
		if(nf_edid_p_block_type(block) == EDID_DETAILED_TIMING_BLOCK)
			nf_edid_p_timing_description(block, info, is_pal);
	}

	return TRUE;
}

gboolean nf_edid_p_chk_timing_standard(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info)
{
	gint i=0;

	g_message("========== Check Standard Timing ==========");
	for(i=7; i>=0; i--)
	{
		if(data[0x26+(i*2)] > 1)
		{
			gshort pixel=0, ratio=0, hz=0, vertical=0;
			gint h=0, v=0;

			pixel=((gshort)data[0x26+i*2]+31)*8;
			hz=((gshort)data[0x27+i*2] & 0x3f)+60;
			ratio=((gshort)data[0x27+i*2]>>6);
			if(ratio == 0)
			{
				h=16;
				v=10;
			}
			else if(ratio == 1)
			{
				h=4;
				v=3;
			}
			else if(ratio == 2)
			{
				h=5;
				v=4;
			}
			else if(ratio == 3)
			{
				h=16;
				v=9;
			}

			vertical=(pixel*v)/h;

			if(_nf_edid_dbg)
			{
				g_message("Active Pixel[%-4d] Vertical[%-4d] %-2dHz Ratio->%d(0->16:10 / 1->4:3 / 2->5:4 / 3->16:9)",
						pixel, vertical, hz, ratio);
			}

			if((pixel == 1280) && (vertical == 1024) && (ratio == 2) && ((hz >= 59) && (hz <= 61)))
				info->is_1280_1024_60=TRUE;
			else if((pixel == 1280) && (vertical == 720) && (ratio == 3) && ((hz >= 59) && (hz <= 61)))
				info->is_720p60=TRUE;
			else if((pixel == 1280) && (vertical == 720) && (ratio == 3) && ((hz >= 49) && (hz <= 51)))
					info->is_720p50=TRUE;
			else if((pixel == 1920) && (vertical == 1080) && (ratio == 3) && ((hz >= 29) && (hz <= 31)))
				info->is_1080p30=TRUE;
			else if((pixel == 1920) && (vertical == 1080) && (ratio == 3) && ((hz >= 59) && (hz <= 61)))
				info->is_1080p60=TRUE;
			else if((pixel == 1920) && (vertical == 1080) && (ratio == 3) && ((hz >= 49) && (hz <= 51)))
				info->is_1080p50=TRUE;
		}
	}

	return TRUE;
}

/** Established Timing Start **/
/** See the data sheet(E-EDID Standard page 15) **/
/**  Established Timing don't have 1920 size.. so check pass **/
gboolean nf_edid_p_chk_timing_established(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info)
{
	gint i=0;
	guint val=0;

	g_message("========== Check Established Timing ==========");
	val = (data[EDID_ESTABLISHED_TIMING_1] << 16) | (data[EDID_ESTABLISHED_TIMING_1+1] << 8) |
						data[EDID_ESTABLISHED_TIMING_1+2];
	for(i=0;i<24;i++)
	{
		if((val>>i)&1)
		{
			if(_nf_edid_dbg)
				printf("## %s\n", _edid_est_timing[23-i]);

//			nf_edid_estd_timing(23-i, priority, mode_output);
		}
	}

	return TRUE;
}

gint nf_edid_p_add_detailed_info_eedid(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info, gboolean is_pal)
{
	gint i=0, j=0, offset=0, edid_ext_num=0, start_offset=0, end_offset=0;
	gchar *edid_ext=NULL;
	guchar *block=NULL;
	EDID *edid=NULL;

	edid=(EDID *)data;

	if ((edid->EDID_VER[0] == 1) && (edid->EDID_REV[0] < 3)) {
		g_message("EDID version is less than 1.3, there is no extension EDID.");
		/* If the EDID version is less than 1.3, there is no
		 * extension EDID.
		 */
		return FALSE;
	}
	if (!edid->EXTENTION_FLAG[0]) {
		g_message("There is no extension EDID, it is unnecessary to parse the E-EDID to get detailed info.");
		/* if there is no extension EDID, it is unnecessary to
		 * parse the E-EDID to get detailed info
		 */
		return FALSE;
	}

	/* Chose real EDID extension number */
	edid_ext_num = (edid->EXTENTION_FLAG[0] > EDID_MAX_EDID_EXT_NUM) ?
						EDID_MAX_EDID_EXT_NUM : edid->EXTENTION_FLAG[0];
	g_message("%s EDID Extention Num ==> %d", __FUNCTION__, edid_ext_num);

	/* Find CEA extension */
	for(i=0; i<edid_ext_num; i++)
	{
		edid_ext = &data[EDID_LENGTH * (i + 1)];
		/* This block is CEA extension */

		if(edid_ext[0] == 0x02)
		{
			/* Video Identification Codes */
			#if 1 //kbulls 110222
				if(edid_ext[1] == 3) //the Version 3 CEA EDID Timing Extension is included in the DTV Monitors EDID data structure.
				{
					gint cnt=(edid_ext[4] & 0x1f);

					g_message("VIDEO IDENTIFIERS.");
					g_message("Timing Cnt ==> %d", cnt);
					g_message("#CEA Short Descriptors");

					for(j=0; j<cnt; j++)
					{
						u_char vd=(edid_ext[5+j] & 0x7f);

						if(vd <35)
							printf("## %s\n", _edid_vd_timing[vd]);

//						ddc_extention_timing(vd, priority, mode_output);
					}
				}
			#endif

			/* Get the start offset of detailed timing block */
			start_offset = edid_ext[2];
			if (start_offset == 0) {
				g_message("The start_offset is zero, it means that neither detailed info nor data block exist. \
							In such case it is also unnecessary to parse the detailed timing info.");
				/* If the start_offset is zero, it means that neither detailed
				 * info nor data block exist. In such case it is also
				 * unnecessary to parse the detailed timing info.
				 */
				continue;
			}
		//  timing_level = standard_timing_level(edid);
			end_offset = EDID_LENGTH;
			end_offset -= EDID_DETAILED_TIMING_DESCRIPTION_SIZE;
			for(offset=start_offset; offset<end_offset; offset+=EDID_DETAILED_TIMING_DESCRIPTION_SIZE)
			{
				block = (u_char *)(edid_ext + offset);
				if(nf_edid_p_block_type(block) == EDID_DETAILED_TIMING_BLOCK)
				{
					g_message("Extended DETAILED_TIMING_BLOCK -->");
					nf_edid_p_timing_description(block, info, is_pal);
				}
				else if(nf_edid_p_block_type(block) == EDID_STANDARD_TIMING_BLOCK)
				{
					g_message("Extended STANDARD_TIMING_BLOCK -->");

					for(j=0; j<8; j++)
					{
						gshort pixel=0, ratio=0, hz=0, vertical=0;
						gint h=0, v=0;

						pixel=((gshort)block[5+j*2]+31)*8;
						hz=((gshort)block[6+j*2] & 0x3f)+60;
						ratio=((gshort)block[6+j*2]>>6);

						if(ratio == 0)
						{
							h=16;
							v=10;
						}
						else if(ratio == 1)
						{
							h=4;
							v=3;
						}
						else if(ratio == 2)
						{
							h=5;
							v=4;
						}
						else if(ratio == 3)
						{
							h=16;
							v=9;
						}
						vertical=(pixel * v)/h;

						#if 0
							printf("%d(%x)) %d %d, %d(16:10,4:3,5:4,16:9)\n",
										j, ((short)block[6+j*2]<<8) | (short)block[5+j*2],
										pixel, hz, ratio);
						#else
							g_message("Active Pixel[%-4d] Vertical[%-4d] %-2dHz Ratio->%d(0->16:10 / 1->4:3 / 2->5:4 / 3->16:9)",
											pixel, vertical, hz, ratio);
						#endif
						#if 0
							ddc_std_timing(pixel, vertical, hz, ratio, priority, mode_output);
						#else
						#endif
					}
				}
				#if 0  // not use
					else if(block_type(block) == DDC_EDID_DETAIL_CVT_3BYTE)
					{
						printf("\nEDID_DETAIL_CVT_3BYTE -->\n");
						drm_cvt_modes((struct detailed_timing *)block);
					}
				#endif
				else
					g_message("ELSE %x -->", nf_edid_p_block_type(block));
			}
		}
	}

	if(i == 0) {
		g_message("There is no additional timing EDID block.");
		/* if there is no additional timing EDID block, return */
		return FALSE;
	}

	return TRUE;
}

gboolean nf_edid_p_timing_description(guchar *dtd, NF_EDID_SUPPORT_RESOLUTION *info, gboolean is_pal)
{
	gint htotal=0, vtotal=0;
	guint res_priority=0, hz=0;

	htotal = H_ACTIVE + H_BLANKING;
	vtotal = V_ACTIVE + V_BLANKING;

	if(_nf_edid_dbg)
		g_message("Mode \"%dx%d\"", H_ACTIVE, V_ACTIVE );

	if( (htotal == 0) || (vtotal == 0) )
	{
		g_warning("%s Unexpected length!! h[%d] v[%d]", __FUNCTION__, htotal, vtotal);
		return FALSE;
	}

	if(_nf_edid_dbg)
	{
		g_message("vfreq %u.%dHz, hfreq %uHz", (guint)PIXEL_CLOCK/((guint)vtotal*(uint)htotal), 
						((guint)PIXEL_CLOCK*1000/((guint)vtotal*(guint)htotal))%1000,
						(guint)PIXEL_CLOCK/(guint)(htotal));

		g_message("DotClock\t%u", (u_int)PIXEL_CLOCK );
		g_message("HTimings\t%u %u %u %u", H_ACTIVE, htotal, H_SYNC_OFFSET, H_SYNC_WIDTH);
		g_message("VTimings\t%u %u %u %u", V_ACTIVE, vtotal, V_SYNC_OFFSET, V_SYNC_WIDTH);
	}

	if(V_INTERLACED || (SYNC_TYPE == SYNC_SEPARATE))
	{
		if(_nf_edid_dbg)
		{
			g_message("Flags\t\t%s\"%sHSync\" \"%sVSync\"",
					((V_INTERLACED >> 4) & INTERLACED) ? "\"Interlace\" ": "", HSYNC_POSITIVE ? "+": "-", VSYNC_POSITIVE ? "+": "-");
		}
	}

	hz = (guint)PIXEL_CLOCK/((guint)vtotal*(guint)htotal);

	if((H_ACTIVE == 1920) && (V_ACTIVE == 1080))
	{
		if((hz >= 59) && (hz <= 61))
			info->is_1080p60=TRUE;
		else if((hz >= 29) && (hz <= 31))
			info->is_1080p30=TRUE;
		else if((hz >= 50) && (hz <= 55))
			info->is_1080p50=TRUE;
		else  if((hz >= 25) && (hz <= 26)) 
			info->is_1080p25=TRUE;
	}
	else if((H_ACTIVE == 3840) && (V_ACTIVE == 2160))        // 4k
	{
		if(is_pal)
		{
			if((hz >= 29) && (hz <= 31))
				info->is_2160p30=TRUE;	
		}
		else
		{
			// NTSC
			if((hz >= 29) && (hz <= 31))
				info->is_2160p30=TRUE;	
			#if 1
				else if((hz >= 59) && (hz <= 61))
					info->is_2160p60=TRUE;	
				// PAL
				else if((hz >= 24) && (hz <= 26))
					info->is_2160p25=TRUE;	
				else if((hz >= 49) && (hz <= 51))
					info->is_2160p50=TRUE;	
			#endif
		}
	}
	else if((H_ACTIVE == 2560) && (V_ACTIVE == 1440))
	{
		if(is_pal)
		{
			if((hz >= 29) && (hz <= 31))
				info->is_1440p30=TRUE;
			else if((hz >= 59) && (hz <= 61))
				info->is_1440p60=TRUE;
		}
		else
		{
			if((hz >= 29) && (hz <= 31))
				info->is_1440p30=TRUE;
			else if((hz >= 59) && (hz <= 61))
				info->is_1440p60=TRUE;
		}
	}
	else if((H_ACTIVE == 2560) && (V_ACTIVE == 1600))
	{
		if(is_pal)
		{
			if((hz >= 59) && (hz <= 61))
				info->is_1600p60=TRUE;
		}
		else
		{
			if((hz >= 59) && (hz <= 61))
				info->is_1600p60=TRUE;
		}
	}
	else if((H_ACTIVE == 1280) && (V_ACTIVE == 720))
	{
		if((hz >= 59) && (hz <= 61))
			info->is_720p60=TRUE;
		else if((hz >= 50) && (hz <= 55))
			info->is_720p50=TRUE;

	}

	return TRUE;
}

gchar *nf_edid_p_get_monitor_name(guchar *data)
{
	guchar *block=NULL;
	gchar *vendor_sign=NULL, *monitor_name=NULL, *ptr=NULL;
	gchar monitor_alt_name[128]={0, };
	gchar tmp[13]={0, };
	gint i=0;

	block = data + EDID_DETAILED_TIMING_DESCRIPTIONS_START;

	for(i=0; i<EDID_NO_DETAILED_TIMING_DESCRIPTIONS; i++, block+=EDID_DETAILED_TIMING_DESCRIPTION_SIZE)
	{
		if(nf_edid_p_block_type(block) == EDID_MONITOR_NAME)
		{
			ptr=(block + EDID_DESCRIPTOR_DATA);

			for(i=0; i<13; i++, ptr++)
			{
				if(*ptr == 0xa)
				{
					tmp[i]=0;
					break;
				}
				tmp[i] = (gchar)*ptr;
			}

			monitor_name=tmp;
			break;
		}
	}

	if (!monitor_name) {
		/* Stupid djgpp hasn't snprintf so we have to hack something together */
		if(strlen(vendor_sign) + 10 > sizeof(monitor_alt_name))
		  vendor_sign[3] = 0;

		sprintf(monitor_alt_name, "%s:%02x%02x", vendor_sign, data[EDID_ID_MODEL], data[EDID_ID_MODEL+1]);
		monitor_name=monitor_alt_name;
	}

	return monitor_name;
}

gint nf_edid_p_checksum(guchar *edid)
{
	u_char i=0, csum=0, all_null=0;

	for(i=0; i<EDID_LENGTH; i++) {
		csum += edid[i];
		all_null |= edid[i];
	}

	if (csum == 0x00 && all_null) {
		/* checksum passed, everything's good */
		return TRUE;
	}
	else
		return FALSE;
}

gint nf_edid_p_block_type(u_char* block)
{
	if(!strncmp((void *)edid_v1_descriptor_flag, (void *)block, 2))
	{
//      printf("# Block type: 2:0x%02x 3:0x%02x\n", block[2], block[3]);
		/* descriptor */
		if(block[2] != 0)
			return EDID_UNKNOWN_DESCRIPTOR;

		return block[3];
	} else {
		/* detailed timing block */
		return EDID_DETAILED_TIMING_BLOCK;
	}
}

gchar *nf_edid_p_get_vendor_sign(u_char const *block)
{
	static char sign[4]={0, };
	gushort h=0;

	/*
	   08h    WORD    big-endian manufacturer ID (see #00136)
			bits 14-10: first letter (01h='A', 02h='B', etc.)
			bits 9-5: second letter
			bits 4-0: third letter
	*/
	h = (gushort)COMBINE_HI_8LO(block[0], block[1]);
	sign[0] = (gchar)(((h>>10) & 0x1f) + 'A' - 1);
	sign[1] = (gchar)(((h>>5) & 0x1f) + 'A' - 1);
	sign[2] = (gchar)((h & 0x1f) + 'A' - 1);
	sign[3] = 0;

	return sign;
}

void nf_edid_p_print_raw_data(guchar *edid)
{
	gint index=0;

	for(index=0; index<EDID_EXT_TOT_SIZE; index++)
	{
		if((index % 16) == 0)
		{
			g_print("\n");
		}

		g_print("0x%02x ", edid[index]);
	}

	g_print("\n\n");
}

