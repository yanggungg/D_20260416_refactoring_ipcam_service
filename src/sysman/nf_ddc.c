#include "nf_common.h"

#include "nf_api_param_app.h"
#include "nf_sysman.h"
#include "nf_util_flash.h"
#include "nf_ddc.h"

#include <dirent.h>

#define DEBUG_JBSHELL_DDC
#if defined(DEBUG_JBSHELL_DDC)
	#include "jbshell.h"
#endif
/** Global Variable Definition **/
const guchar _edid_v1_header[] = { 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
const guchar _edid_v1_descriptor_flag[] = { 0x00, 0x00 };

extern guint nf_timer_add( guint interval, GSourceFunc cb_func, gpointer data);

const char _est_timing[24][20] =
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

const char _vd_timing[35][30] = {
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
	"1920 x 1080 p @ 30Hz 16:9"                 // 34
};

/** Gloval Functoin Definition **/
static gboolean _nf_ddc_timer_cb_func(gpointer data);
static gint _nf_ddc_add_detailed_info_eedid(guchar *edid, FILE *fp);

gboolean nf_ddc_init(void)
{
	nf_timer_add(3000, _nf_ddc_timer_cb_func, "ITX EDID Dump Log");

	return TRUE;
}

static gboolean _nf_ddc_timer_cb_func(gpointer data)
{
	DIR *dir;

	dir = opendir(NF_DDC_EDID_DUMP_FILE_PATH);
	if(dir == NULL)
	{
		g_message("%s %s Not Created Directory Yet!!!!", __FUNCTION__, NF_DDC_EDID_DUMP_FILE_PATH);
		return TRUE;
	}
	else
	{
		g_message("%s %s Dir Created!!!!", __FUNCTION__, NF_DDC_EDID_DUMP_FILE_PATH);
		nf_ddc_dump_start();
		return FALSE;
	}
}

gboolean nf_ddc_dump_start(void)
{
	guchar edid_data_vga[NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE]={0, };
	guchar edid_data_hdmi[NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE]={0, };
	FILE *fp=NULL;
	gint i=0;

	if((fp = fopen(NF_DDC_EDID_DUMP_FILE, "w")) == NULL)
	{
		g_warning("%s File Create Error!! Name[%s]", 
						__FUNCTION__, NF_DDC_EDID_DUMP_FILE);
		return FALSE;
	}else{
		fprintf(fp, "EDID Dump Log\n");
	}

	nf_ddc_get_edid_raw_data(edid_data_vga, edid_data_hdmi);
	
	g_message("============= VGA Raw Data =============");
	fprintf(fp, "============= VGA Raw Data =============\n");
	for(i=0; i<NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE; i++)
	{
		if((i % 128) == 0)
		{
			g_print("\n");
			fprintf(fp, "\n");
		}
		g_print("%02x", edid_data_vga[i]);
		fprintf(fp, "%02x", edid_data_vga[i]);
	}
	fprintf(fp, "< VGA EDID Dump >\n");
	nf_ddc_parse_raw_data(edid_data_vga, fp);

	g_message("\n\n\n\n\n============= HDMI Raw Data =============");
	fprintf(fp, "\n\n\n\n============= HDMI Raw Data =============\n");
	for(i=0; i<NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE; i++)
	{
		if((i % 128) == 0)
		{
			g_print("\n");
			fprintf(fp, "\n");
		}
		g_print("%02x", edid_data_hdmi[i]);
		fprintf(fp, "%02x", edid_data_hdmi[i]);
	}
	fprintf(fp, "< HDMI EDID Dump >\n");
	nf_ddc_parse_raw_data(edid_data_hdmi, fp);
	
	fclose(fp);
	return TRUE;
}

gboolean nf_ddc_get_edid_raw_data(guchar *edid_data_vga, guchar *edid_data_hdmi)
{
	NF_PARAM_APP app_param;
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gint ret=0;
	guint offs=0;

	memset(&app_param, 0x0, sizeof(NF_PARAM_APP));
	offs=0;
	ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
	if(!ret)
	{
		g_warning("%s Get App Parameter Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	memcpy(&app_param, dataBuf, sizeof(NF_PARAM_APP));
	
	memcpy(edid_data_vga, app_param.ddc_raw_data_vga, (NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE));
	memcpy(edid_data_hdmi, app_param.ddc_raw_data_hdmi, (NF_DDC_EDID_EXT_MAX_SIZE+NF_DDC_EDID_SIZE));

	return TRUE;
}

gboolean nf_ddc_parse_raw_data(guchar *edid_data, FILE *fp)
{
	gint ext_edid_num=0, i=0;
	guint pixelClk=0;
	gushort ha=0, va=0;

	/** 1. VGA INFO **/
	ext_edid_num=0;

	if(nf_ddc_edid_checksum(edid_data))
	{
		g_print("DDC EDID Checksum Ok!!!\n");
		fprintf(fp, "DDC EDID Checksum Ok!!!\n");

		// Read Extended edid
		ext_edid_num = (edid_data[NF_DDC_EDID_EXT_FLAG] > NF_DDC_MAX_EDID_EXT_NUM) 
								? NF_DDC_MAX_EDID_EXT_NUM : edid_data[NF_DDC_EDID_EXT_FLAG];
		if(ext_edid_num)
		{
			g_print("Have Extention Data [%d]\n", ext_edid_num);
			fprintf(fp, "Have Extention Data [%d]\n", ext_edid_num);
		}

		// Check native display clock
		pixelClk = (guint)(edid_data[0x37]*256 + edid_data[0x36]) * 10;
		ha = (gushort)((edid_data[0x3A]>>4)*256 + edid_data[0x38]);
		va = (gushort)((edid_data[0x3D]>>4)*256 + edid_data[0x3B]);

		g_print("CheckEDID : Detected(PixelClk:%d Hactive:%d Vactive:%d)\n", pixelClk, ha, va);
		fprintf(fp, "CheckEDID : Detected(PixelClk:%d Hactive:%d Vactive:%d)\n", pixelClk, ha, va);

		nf_ddc_parse_edid(edid_data, fp);
	}
	else
	{
		g_print("DDC EDID Checksum Fail!!!\n");
		fprintf(fp, "DDC EDID Checksum Fail!!!\n");

		return FALSE;
	}

	return TRUE;
}

gint nf_ddc_edid_checksum(guchar *edid)
{
	guchar i=0, csum=0, all_null=0;

	for(i=0; i<NF_EDID_LENGTH; i++)
	{
		#if 0		// For Warning MSG
			csum += edid[i];
		#else
			csum = (guchar)(csum + edid[i]);
		#endif
		all_null |= edid[i];
	}

	if((csum == 0x00) && all_null) {
		/* checksum passed, everything's good */
		return TRUE;
	}
	else
		return FALSE;
}

gint nf_ddc_parse_edid(guchar *edid, FILE *fp)
{
	gint ret=0, i=0, j=0;
	gchar *vendor_sign=NULL, *monitor_name=NULL;
	gchar monitor_alt_name[100]={0, };
	guchar *block=NULL;
	guint data=0;

	if(strncmp((void *)(edid+DDC_EDID_HEADER), (void *)_edid_v1_header, DDC_EDID_HEADER_END+1))
	{
		g_print("first bytes don't match EDID version 1 header\n");
		g_print("do not trust output (if any).\n");
		fprintf(fp, "first bytes don't match EDID version 1 header\n");
		fprintf(fp, "do not trust output (if any).\n");

		ret = 1;
	}
	g_print("EDID version %d revision %d\n", (int)edid[DDC_EDID_STRUCT_VERSION],(int)edid[DDC_EDID_STRUCT_REVISION]);
	fprintf(fp, "EDID version %d revision %d\n", (int)edid[DDC_EDID_STRUCT_VERSION],(int)edid[DDC_EDID_STRUCT_REVISION]);

	vendor_sign = nf_ddc_get_vendor_sign(edid+DDC_ID_MANUFACTURER_NAME);

	block = edid + DDC_DETAILED_TIMING_DESCRIPTIONS_START;
	for(i=0; i<DDC_NO_DETAILED_TIMING_DESCRIPTIONS; i++, block+=DDC_DETAILED_TIMING_DESCRIPTION_SIZE)
	{
		if(nf_ddc_block_type(block) == DDC_MONITOR_NAME)
		{
			monitor_name=nf_ddc_get_monitor_name(block);
			break;
		}
	}

	if (!monitor_name) {
		/* Stupid djgpp hasn't sng_print so we have to hack something together */
		if(strlen(vendor_sign) + 10 > sizeof(monitor_alt_name))
		  vendor_sign[3] = 0;

		sprintf(monitor_alt_name, "%s:%02x%02x", vendor_sign, edid[DDC_ID_MODEL], edid[DDC_ID_MODEL+1]);
		monitor_name=monitor_alt_name;
	}

	g_print("Identifier    [%s]\n", monitor_name);
	g_print("VendorName    [%s]\n", vendor_sign);
	g_print("ModelName     [%s]\n", monitor_name);
	fprintf(fp, "Identifier    [%s]\n", monitor_name);
	fprintf(fp, "VendorName    [%s]\n", vendor_sign);
	fprintf(fp, "ModelName     [%s]\n", monitor_name);

	for(i=0; i<DDC_NO_DETAILED_TIMING_DESCRIPTIONS; i++, block+=DDC_DETAILED_TIMING_DESCRIPTION_SIZE)
	{
		if(nf_ddc_block_type(block) == DDC_MONITOR_LIMITS)
		{
			g_print("MONITOR_LIMITS -->\n");
			fprintf(fp, "MONITOR_LIMITS -->\n");
			nf_ddc_parse_monitor_limits(block);
		}
	}

//  ddc_parse_dpms_capabilities(edid[DDC_DPMS_FLAGS]);

	block=(edid + DDC_DETAILED_TIMING_DESCRIPTIONS_START);

	g_print("#Detailed Timing\n");
	fprintf(fp, "#Detailed Timing\n");
	for(i=0; i<DDC_NO_DETAILED_TIMING_DESCRIPTIONS; i++, block+=DDC_DETAILED_TIMING_DESCRIPTION_SIZE)
	{
		if(nf_ddc_block_type(block) == DDC_DETAILED_TIMING_BLOCK)
		{
			g_print("DETAILED_TIMING_BLOCK[%d] -->\n", i);
			fprintf(fp, "DETAILED_TIMING_BLOCK[%d] -->\n", i);
			nf_ddc_parse_timing_description(block, fp);
		}
	}

	/** Stardard Timing **/
	/** See The Data Sheet
	  E-EDID Standard.pdf
	  Page 17 of 32
	 **/
	g_print("#Standard Timing\n");
	fprintf(fp, "#Standard Timing\n");
	for(i=7; i>=0; i--)
	{
		if(edid[0x26+(i*2)] > 1)
		{
			gshort pixel=0, ratio=0, hz=0, vertical=0;
			int h=0, v=0;

			pixel=(gshort)((edid[0x26+i*2]+31)*8);
			hz=(gshort)((edid[0x27+i*2] & 0x3f)+60);
			ratio=(gshort)((edid[0x27+i*2]>>6));
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

			vertical=(gshort)((pixel*v)/h);

			g_print("Active Pixel[%-4d] Vertical[%-4d] %-2dHz Ratio->%d(0->16:10 / 1->4:3 / 2->5:4 / 3->16:9)\n",
					pixel, vertical, hz, ratio);
			fprintf(fp, "Active Pixel[%-4d] Vertical[%-4d] %-2dHz Ratio->%d(0->16:10 / 1->4:3 / 2->5:4 / 3->16:9)\n",
					pixel, vertical, hz, ratio);
			#if 0
				ddc_std_timing(pixel, hz, ratio);
			#endif
		}
	}

	/** Established Timing Start **/
	/** See the data sheet(E-EDID Standard page 15) **/
	/**  Established Timing don't have 1920 size.. so check pass **/
	g_print("#Established Timing\n");
	data = (guint)((edid[DDC_ESTABLISHED_TIMING_1] << 16) | (edid[DDC_ESTABLISHED_TIMING_1+1] << 8) |
						edid[DDC_ESTABLISHED_TIMING_1+2]);
	for(j=0; j<24; j++)
	{
		if((data>>j)&1)
		{
			g_print("## %s\n", _est_timing[23-j]);
			fprintf(fp, "## %s\n", _est_timing[23-j]);
		}
	}

	_nf_ddc_add_detailed_info_eedid(edid, fp);
	
	return TRUE;
}

gchar *nf_ddc_get_vendor_sign(guchar const *block)
{
	static gchar sign[4]={0, };
	gushort h=0;

	/*
	   08h    WORD    big-endian manufacturer ID (see #00136)
			bits 14-10: first letter (01h='A', 02h='B', etc.)
			bits 9-5: second letter
			bits 4-0: third letter
	*/
	h = (gushort)(COMBINE_HI_8LO(block[0], block[1]));
	sign[0] = (gchar)(((h>>10) & 0x1f) + 'A' - 1);
	sign[1] = (gchar)(((h>>5) & 0x1f) + 'A' - 1);
	sign[2] = (gchar)((h & 0x1f) + 'A' - 1);
	sign[3] = 0;

	return sign;
}

gint nf_ddc_block_type(guchar* block)
{
	if(!strncmp((void *)_edid_v1_descriptor_flag, (void *)block, 2))
	{
//      g_print("# Block type: 2:0x%02x 3:0x%02x\n", block[2], block[3]);
		/* descriptor */
		if(block[2] != 0)
			return DDC_UNKNOWN_DESCRIPTOR;

		return block[3];
	} else {
		/* detailed timing block */
		return DDC_DETAILED_TIMING_BLOCK;
	}
}

gchar *nf_ddc_get_monitor_name(guchar const *block)
{
	static gchar name[13]={0, };
	guint i=0;
	guchar const *ptr=(block + DDC_DESCRIPTOR_DATA);

	for(i=0; i<13; i++, ptr++)
	{
		if(*ptr == 0xa)
		{
			name[i]=0;
			return name;
		}
		name[i] = (gchar)*ptr;
	}

	return name;
}

gint nf_ddc_parse_monitor_limits(guchar *block)
{
	g_print("HorizSync %u-%u\n", DDC_H_MIN_RATE, DDC_H_MAX_RATE);
	g_print("VertRefresh %u-%u\n", DDC_V_MIN_RATE, DDC_V_MAX_RATE);

	if(DDC_MAX_PIXEL_CLOCK == (10*0xff))
		g_print("# Max dot clock not given\n");
	else
		g_print("# Max dot clock (video bandwidth) %u MHz\n", (gint)DDC_MAX_PIXEL_CLOCK);

	if(DDC_GTF_SUPPORT)
		g_print( "# EDID version 3 GTF given: contact author\n");

	return TRUE;
}

gint nf_ddc_parse_timing_description(guchar *dtd, FILE *fp)
{
	gint htotal=0, vtotal=0;
	guint res_priority=0, hz=0;

	htotal = (gint)(H_ACTIVE + H_BLANKING);
	vtotal = (gint)(V_ACTIVE + V_BLANKING);

	g_print("Mode \"%dx%d\"\n", H_ACTIVE, V_ACTIVE );
	if( (htotal == 0) || (vtotal == 0) )
	{
		g_print("Unexpected length!! h[%d] v[%d]\n", htotal, vtotal);
		return 0;
	}

	g_print("vfreq %u.%dHz, hfreq %uHz\n",
	(uint)PIXEL_CLOCK/((uint)vtotal*(uint)htotal), ((uint)PIXEL_CLOCK*1000/((uint)vtotal*(uint)htotal))%1000,
	(uint)PIXEL_CLOCK/(uint)(htotal));
	fprintf(fp, "Mode \"%dx%d\"\n", H_ACTIVE, V_ACTIVE );
	fprintf(fp, "vfreq %u.%dHz, hfreq %uHz\n",
	(uint)PIXEL_CLOCK/((uint)vtotal*(uint)htotal), ((uint)PIXEL_CLOCK*1000/((uint)vtotal*(uint)htotal))%1000,
	(uint)PIXEL_CLOCK/(uint)(htotal));

	g_print("DotClock\t%u\n", (u_int)PIXEL_CLOCK );
	g_print("HTimings\t%u %u %u %u\n", H_ACTIVE, htotal, H_SYNC_OFFSET, H_SYNC_WIDTH);
	g_print("VTimings\t%u %u %u %u\n", V_ACTIVE, vtotal, V_SYNC_OFFSET, V_SYNC_WIDTH);
	fprintf(fp, "DotClock\t%u\n", (u_int)PIXEL_CLOCK );
	fprintf(fp, "HTimings\t%u %u %u %u\n", H_ACTIVE, htotal, H_SYNC_OFFSET, H_SYNC_WIDTH);
	fprintf(fp, "VTimings\t%u %u %u %u\n", V_ACTIVE, vtotal, V_SYNC_OFFSET, V_SYNC_WIDTH);

	if(INTERLACED || (SYNC_TYPE == SYNC_SEPARATE))
	{
		g_print("Flags\t\t%s\"%sHSync\" \"%sVSync\"\n",
				INTERLACED ? "\"Interlace\" ": "", HSYNC_POSITIVE ? "+": "-", VSYNC_POSITIVE ? "+": "-");
		fprintf(fp, "Flags\t\t%s\"%sHSync\" \"%sVSync\"\n",
				INTERLACED ? "\"Interlace\" ": "", HSYNC_POSITIVE ? "+": "-", VSYNC_POSITIVE ? "+": "-");
	}

	/** Not Needed **/
	#if 0
		hz = (u_int)PIXEL_CLOCK/((uint)vtotal*(u_int)htotal);

		if((H_ACTIVE == 1920) && (V_ACTIVE == 1080))
		{
			#if defined(CONFIG_ITX_HDSDI0412) || defined(CONFIG_ITX_ATM0824)
				if(!_is_pal)
				{
					if((hz > 55) && (hz <= 60))
						res_priority = ITX_DDC_PRI_1080_P60;
					else if((hz >= 50) && (hz <= 55))
						res_priority = ITX_DDC_PRI_1080_P50;
				}
				else
				{
					if(hz > 55)     // warning -> this priority is defferent to snf1648 
						res_priority = ITX_DDC_PRI_1080_P60;
					else
						res_priority = ITX_DDC_PRI_1080_P50;
				}
			#else
				if((hz > 55) && (hz <= 60))
					res_priority = ITX_DDC_PRI_1080_P60;
				else if((hz >= 50) && (hz <= 55))
					res_priority = ITX_DDC_PRI_1080_P50;
				else
					res_priority = ITX_DDC_PRI_UNKNOWN;
			#endif
		}
		else if((H_ACTIVE == 1680) && (V_ACTIVE == 1050))
			res_priority = ITX_DDC_PRI_1680_1050_60;
		else if((H_ACTIVE == 800) && (V_ACTIVE == 600))
		{
			if((hz > 55) && (hz <= 60))
				res_priority = ITX_DDC_PRI_800_600_60;
			else if((hz > 60) && (hz <= 72))
				res_priority = ITX_DDC_PRI_800_600_72;
			else if((hz > 72) && (hz <= 75))
				res_priority = ITX_DDC_PRI_800_600_75;
			else if((hz > 75) && (hz <= 85))
				res_priority = ITX_DDC_PRI_800_600_85;
			else
				res_priority = ITX_DDC_PRI_UNKNOWN;
		}
		else
			res_priority = ITX_DDC_PRI_UNKNOWN;

		ddc_set_priority(res_priority);
	#endif

	return TRUE;
}

static gint _nf_ddc_add_detailed_info_eedid(guchar *edid, FILE *fp)
{
	gint i=0, edid_ext_num=0, start_offset=0, end_offset=0;
	guchar *edid_ext=NULL;
	guchar *block=NULL;
	DDC *ddc_edid=NULL;

	ddc_edid=(DDC *)edid;

	if ((ddc_edid->EDID_VER[0] == 1) && (ddc_edid->EDID_REV[0] < 3)) {
		g_print("EDID version is less than 1.3, there is no extension EDID.\n");
		fprintf(fp, "EDID version is less than 1.3, there is no extension EDID.\n");
		/* If the EDID version is less than 1.3, there is no
		 * extension EDID.
		 */
		return FALSE;
	}
	if (!ddc_edid->EXTENTION_FLAG[0]) {
		g_print("There is no extension EDID, it is unnecessary to parse the E-EDID to get detailed info.\n");
		fprintf(fp, "There is no extension EDID, it is unnecessary to parse the E-EDID to get detailed info.\n");
		/* if there is no extension EDID, it is unnecessary to
		 * parse the E-EDID to get detailed info
		 */
		return FALSE;
	}

	/* Chose real EDID extension number */
	edid_ext_num = (ddc_edid->EXTENTION_FLAG[0] > NF_DDC_MAX_EDID_EXT_NUM) ?
						NF_DDC_MAX_EDID_EXT_NUM : ddc_edid->EXTENTION_FLAG[0];
	g_print("\n\n%s EDID Extention Num ==> %d\n", __FUNCTION__, edid_ext_num);
	fprintf(fp, "\n\nEDID Extention Num ==> %d\n", edid_ext_num);

	/* Find CEA extension */
	for(i=0; i<edid_ext_num; i++)
	{
		edid_ext = (guchar *)edid + (guchar)(NF_EDID_LENGTH * (i + 1));
		/* This block is CEA extension */
		if(edid_ext[0] == 0x02)
		{
			/* Video Identification Codes */
			if(edid_ext[1] == 3) //the Version 3 CEA EDID Timing Extension is included in the DTV Monitors EDID data structure.
			{
				gint cnt=(edid_ext[4] & 0x1f);

				g_print("VIDEO IDENTIFIERS.\n");
				g_print("Timing Cnt ==> %d \n", cnt);
				g_print("#cea short descriptors\n");
				fprintf(fp, "VIDEO IDENTIFIERS.\n");
				fprintf(fp, "Data Cnt ==> %d \n", cnt);
				fprintf(fp, "#cea short descriptors\n");

				for(i=0;i<cnt;i++)
				{
					guchar vd=(guchar)(edid_ext[5+i] & 0x7f);

					if(vd <35)
					{
						g_print("## %s\n", _vd_timing[vd]);
						fprintf(fp, "## %s\n", _vd_timing[vd]);
						#if 0
							ddc_extention_timing(vd);
						#endif
					}
				}
			}

			/* Get the start offset of detailed timing block */
			start_offset = edid_ext[2];
			if (start_offset == 0) {
				g_print("The start_offset is zero, it means that neither detailed info nor data block exist. \
							In such case it is also unnecessary to parse the detailed timing info.\n");
				fprintf(fp, "The start_offset is zero, it means that neither detailed info nor data block exist. \
							In such case it is also unnecessary to parse the detailed timing info.\n");
				/* If the start_offset is zero, it means that neither detailed
				 * info nor data block exist. In such case it is also
				 * unnecessary to parse the detailed timing info.
				 */
				continue;
			}

		//  timing_level = standard_timing_level(edid);
			end_offset = NF_EDID_LENGTH;
			end_offset -= DDC_DETAILED_TIMING_DESCRIPTION_SIZE;
			for(i=start_offset; i<end_offset; i+=DDC_DETAILED_TIMING_DESCRIPTION_SIZE)
			{
				block = (u_char *)(edid_ext + i);
				if(nf_ddc_block_type(block) == DDC_DETAILED_TIMING_BLOCK)
				{
					g_print("Extended DETAILED_TIMING_BLOCK -->\n");
					fprintf(fp, "Extended DETAILED_TIMING_BLOCK -->\n");
					nf_ddc_parse_timing_description(block, fp);
				}
				else if(nf_ddc_block_type(block) == DDC_STANDARD_TIMING_BLOCK)
				{
					g_print("Extended STANDARD_TIMING_BLOCK -->\n");
					fprintf(fp, "Extended STANDARD_TIMING_BLOCK -->\n");

					for(i=0; i<8; i++)
					{
						gshort pixel=0, ratio=0, hz=0;

						pixel=(gshort)((block[5+i*2]+31)*8);
						hz=(gshort)((block[6+i*2] & 0x3f)+60);
						ratio=((gshort)block[6+i*2]>>6);

						g_print("%d(%x)) %d %d, %d(16:10,4:3,5:4,16:9)\n",
									i, ((gshort)block[6+i*2]<<8) | (gshort)block[5+i*2],
									pixel, hz, ratio);
						fprintf(fp, "%d(%x)) %d %d, %d(16:10,4:3,5:4,16:9)\n",
									i, ((gshort)block[6+i*2]<<8) | (gshort)block[5+i*2],
									pixel, hz, ratio);
						#if 0
							ddc_std_timing(pixel, hz, ratio);
						#endif
					}
				}
				#if 0  // not use
					else if(block_type(block) == DDC_EDID_DETAIL_CVT_3BYTE)
					{
						g_print("\nEDID_DETAIL_CVT_3BYTE -->\n");      
						drm_cvt_modes((struct detailed_timing *)block);
					}
				#endif
				else
				{
					g_print("ELSE %x -->\n", nf_ddc_block_type(block));
					fprintf(fp, "ELSE %x -->\n", nf_ddc_block_type(block));
				}
			}
		}
	}

	if(i == edid_ext_num) {
		g_print("There is no additional timing EDID block.\n");
		fprintf(fp, "There is no additional timing EDID block.\n");

		/* if there is no additional timing EDID block, return */
		return FALSE;
	}

	return TRUE;
}

#if defined(DEBUG_JBSHELL_DDC)
static char nf_ddc_jbshell_cmd_help[] = "ddc info\n";
static int nf_ddc_jbshell_cmd(int argc, char **argv)
{
	if(argc < 2)
		goto nf_ddc_help_cmd;
	
	if(strcmp(argv[1], "info") == 0)
	{
		nf_ddc_dump_start();
	}
	else
		goto nf_ddc_help_cmd;

	return 0;

nf_ddc_help_cmd:
	g_print("Invalid arguments\n%s\n", nf_ddc_jbshell_cmd_help);

	return -1;
}

__commandlist(nf_ddc_jbshell_cmd, "ddc", nf_ddc_jbshell_cmd_help, nf_ddc_jbshell_cmd_help);
#endif

