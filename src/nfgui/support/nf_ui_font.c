
#include "nf_ui_font.h"

static GdkFont* nffont[NFFONT_END];

static const gchar *strFont[NFFONT_CHARSET_END][NUM_DISP_MODE][NFFONT_END] = {
	{	// ISO8859-1
		{
			"-misc-fixed-bold-r-normal-*-13-*-*-*-*-*-iso8859-1",
			"-misc-fixed-bold-r-semicondensed-*-13-*-*-*-*-*-iso8859-1",

			"-misc-fixed-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
			"-misc-fixed-bold-r-semicondensed-*-14-*-*-*-*-*-iso8859-1",
			
			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-1",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-1",

			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-1",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-1",
            
			"-misc-fixed-bold-r-normal-*-18-*-*-*-*-*-iso8859-1",
			"-misc-fixed-bold-r-semicondensed-*-18-*-*-*-*-*-iso8859-1",
		},
		{
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-1",
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-1",

			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-1",
			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-1",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-1",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-1",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-1",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-1",

			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-1",
			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-1",
		}
	},
	{	// ISO8859-2
		{
			"-misc-fixed-bold-r-normal-*-13-*-*-*-*-*-iso8859-2",
			"-misc-fixed-bold-r-semicondensed-*-13-*-*-*-*-*-iso8859-2",

			"-misc-fixed-bold-r-normal-*-14-*-*-*-*-*-iso8859-2",
			"-misc-fixed-bold-r-semicondensed-*-14-*-*-*-*-*-iso8859-2",
			
			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-2",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-2",

			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-2",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-2",
			
			"-misc-fixed-bold-r-normal-*-18-*-*-*-*-*-iso8859-2",
			"-misc-fixed-bold-r-semicondensed-*-18-*-*-*-*-*-iso8859-2",
		},
		{

			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-2",
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-2",

			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-2",
			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-2",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-2",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-2",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-2",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-2",

			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-2",
			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-2",
		}
	},
	{	// ISO8859-5
		{
			"-misc-fixed-bold-r-normal-*-13-*-*-*-*-*-iso8859-5",
			"-misc-fixed-bold-r-semicondensed-*-13-*-*-*-*-*-iso8859-5",

			"-misc-fixed-bold-r-normal-*-14-*-*-*-*-*-iso8859-5",
			"-misc-fixed-bold-r-semicondensed-*-14-*-*-*-*-*-iso8859-5",
			
			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-5",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-5",

			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-5",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-5",
            
			"-misc-fixed-bold-r-normal-*-18-*-*-*-*-*-iso8859-5",
			"-misc-fixed-bold-r-semicondensed-*-18-*-*-*-*-*-iso8859-5",
		},
		{
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-5",
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-5",

			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-5",
			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-5",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-5",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-5",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-5",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-5",

			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-5",
			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-5",
		}
	},
	{	// ISO8859-9
		{
			"-misc-fixed-bold-r-normal-*-13-*-*-*-*-*-iso8859-9",
			"-misc-fixed-bold-r-semicondensed-*-13-*-*-*-*-*-iso8859-9",

			"-misc-fixed-bold-r-normal-*-14-*-*-*-*-*-iso8859-9",
			"-misc-fixed-bold-r-semicondensed-*-14-*-*-*-*-*-iso8859-9",
			
			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-9",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-9",

			"-misc-fixed-bold-r-normal-*-15-*-*-*-*-*-iso8859-9",
			"-misc-fixed-bold-r-semicondensed-*-15-*-*-*-*-*-iso8859-9",
            
			"-misc-fixed-bold-r-normal-*-18-*-*-*-*-*-iso8859-9",
			"-misc-fixed-bold-r-semicondensed-*-18-*-*-*-*-*-iso8859-9",
		},
		{

			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-9",
			"-*-helvetica-bold-r-normal-*-17-*-*-*-*-*-iso8859-9",

			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-9",
			"-*-helvetica-bold-r-normal-*-18-*-*-*-*-*-iso8859-9",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-9",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-9",

			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-9",
			"-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-iso8859-9",

			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-9",
			"-*-helvetica-bold-r-normal-*-24-*-*-*-*-*-iso8859-9",
		}
	}, // gulim
	{
		{
			"-*-*-bold-r-normal--12-120-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--12-120-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--14-140-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--14-140-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--20-200-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--20-200-75-75-*-*-ksc5601.1987-0",
		},
		{
			"-*-*-bold-r-normal--12-120-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--12-120-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--14-140-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--14-140-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--18-180-75-75-*-*-ksc5601.1987-0",

			"-*-*-bold-r-normal--20-200-75-75-*-*-ksc5601.1987-0",
			"-*-*-bold-r-normal--20-200-75-75-*-*-ksc5601.1987-0",
		}
	}
};

static gchar name_buf[NUM_DISP_MODE][NFFONT_END][64] = { 0, };

static gchar* font_name[NUM_DISP_MODE][NFFONT_END] = { 
	{ 
		name_buf[0][NFFONT_MINI_NORMAL],
		name_buf[0][NFFONT_MINI_SEMI],

		name_buf[0][NFFONT_MINI_NORMAL_1],
		name_buf[0][NFFONT_MINI_SEMI_1],

		name_buf[0][NFFONT_MINI_NORMAL_3],
		name_buf[0][NFFONT_MINI_SEMI_3],

		name_buf[0][NFFONT_MINI_NORMAL_4],
		name_buf[0][NFFONT_MINI_SEMI_4],

		name_buf[0][NFFONT_MINI_NORMAL_5],
		name_buf[0][NFFONT_MINI_SEMI_5],

		name_buf[0][NFFONT_SMALL_NORMAL],
		name_buf[0][NFFONT_SMALL_SEMI],
		name_buf[0][NFFONT_SMALL_THIN],

		name_buf[0][NFFONT_SMALL_NORMAL_1],
		name_buf[0][NFFONT_SMALL_SEMI_1],

		name_buf[0][NFFONT_MEDIUM_NORMAL],
		name_buf[0][NFFONT_MEDIUM_SEMI],
		name_buf[0][NFFONT_MEDIUM_THIN],

		name_buf[0][NFFONT_LARGE_NORMAL],
		name_buf[0][NFFONT_LARGE_SEMI],

		name_buf[0][NFFONT_LARGE_NORMAL_1],
		name_buf[0][NFFONT_LARGE_SEMI_1],

		name_buf[0][NFFONT_XLARGE_NORMAL],
		name_buf[0][NFFONT_XLARGE_SEMI],

		name_buf[0][NFFONT_XLARGE_NORMAL_1],
		name_buf[0][NFFONT_XLARGE_SEMI_1],

		name_buf[0][NFFONT_XLARGE_NORMAL_2],
		name_buf[0][NFFONT_XLARGE_SEMI_2],

		name_buf[0][NFFONT_XLARGE_NORMAL_3],
		name_buf[0][NFFONT_XLARGE_SEMI_3],
	},
	{
		name_buf[1][NFFONT_MINI_NORMAL],
		name_buf[1][NFFONT_MINI_SEMI],

		name_buf[1][NFFONT_MINI_NORMAL_1],
		name_buf[1][NFFONT_MINI_SEMI_1],

		name_buf[1][NFFONT_MINI_NORMAL_3],
		name_buf[1][NFFONT_MINI_SEMI_3],

		name_buf[1][NFFONT_MINI_NORMAL_4],
		name_buf[1][NFFONT_MINI_SEMI_4],

		name_buf[1][NFFONT_MINI_NORMAL_5],
		name_buf[1][NFFONT_MINI_SEMI_5],

		name_buf[1][NFFONT_SMALL_NORMAL],
		name_buf[1][NFFONT_SMALL_SEMI],
		name_buf[1][NFFONT_SMALL_THIN],

		name_buf[1][NFFONT_SMALL_NORMAL_1],
		name_buf[1][NFFONT_SMALL_SEMI_1],

		name_buf[1][NFFONT_MEDIUM_NORMAL],
		name_buf[1][NFFONT_MEDIUM_SEMI],
		name_buf[1][NFFONT_MEDIUM_THIN],

		name_buf[1][NFFONT_LARGE_NORMAL],
		name_buf[1][NFFONT_LARGE_SEMI],

		name_buf[1][NFFONT_LARGE_NORMAL_1],
		name_buf[1][NFFONT_LARGE_SEMI_1],

		name_buf[1][NFFONT_XLARGE_NORMAL],
		name_buf[1][NFFONT_XLARGE_SEMI],

		name_buf[1][NFFONT_XLARGE_NORMAL_1],
		name_buf[1][NFFONT_XLARGE_SEMI_1],

		name_buf[1][NFFONT_XLARGE_NORMAL_2],
		name_buf[1][NFFONT_XLARGE_SEMI_2],

		name_buf[1][NFFONT_XLARGE_NORMAL_3],
		name_buf[1][NFFONT_XLARGE_SEMI_3],
	}
};



static const gchar* font_name_eng[NUM_DISP_MODE][NFFONT_END] = {
	{
		// NFFONT_MINI_NORMAL
		"Calibri bold 7",			// not used
		"Calibri 7",			// not used

		// NFFONT_MINI_NORMAL_1
		"Calibri bold 7",			// not used
		"Calibri 7",			// not used

		// NFFONT_MINI_NORMAL_3
		"Calibri bold 8",			// not used
		"Calibri 8",			// not used

		// NFFONT_MINI_NORMAL_4
		"Calibri bold 8",			// not used
		"Calibri 8",			// not used

		// NFFONT_MINI_NORMAL_5
		"Calibri bold 8",			// not used
		"Calibri 8",			// not used

		// NFFONT_SMALL_NORMAL
		"Calibri bold 9",			// not used
		"Calibri 9",			// not used
		"Calibri 9",			// not used

		// NFFONT_SMALL_NORMAL_1
		"Calibri bold 9",			// not used
		"Calibri 9",			// not used

		// NFFONT_MEDIUM_NORMAL
		"Calibri bold 12",			// not used
		"Calibri 12",			// not used
		"Calibri 12",			// not used

		// NFFONT_LARGE_NORMAL
		"Calibri bold 13",			// not used
		"Calibri 13",			// not used

		// NFFONT_LARGE_NORMAL_1
		"Calibri bold 14",			// not used
		"Calibri 14",			// not used

		// NFFONT_XLARGE_NORMAL
		"Calibri bold 20",			// not used
		"Calibri 20",			// not used

		// NFFONT_XLARGE_NORMAL_1
		"Calibri bold 26",			// not used
		"Calibri 26",			// not used

		// NFFONT_XLARGE_NORMAL_2
		"Calibri bold 26",			// not used
		"Calibri 26",			// not used

		// NFFONT_XLARGE_NORMAL_3
		"Calibri bold 26",			// not used
		"Calibri 26",			// not used
	},
// ENGLISH - 4D1
	{
		// NFFONT_MINI_NORMAL
		"Calibri bold 10",
		"Calibri 10",

		// NFFONT_MINI_NORMAL_1
		"Calibri bold 11",
		"Calibri 11",

		// NFFONT_MINI_NORMAL_3
		"Calibri bold 13",
		"Calibri 13",

		// NFFONT_MINI_NORMAL_4
		"Calibri bold 14",
		"Calibri 14",

		// NFFONT_MINI_NORMAL_5
		"Calibri bold 15",
		"Calibri 15",

		// NFFONT_SMALL_NORMAL
		"Calibri bold 16",
		"Calibri 16",
		"Calibri 16",

		// NFFONT_SMALL_NORMAL_1
		"Calibri bold 17",
		"Calibri 17",

		// NFFONT_MEDIUM_NORMAL
		"Calibri bold 18",
		"Calibri 18",
		"Calibri 18",

		// NFFONT_LARGE_NORMAL
		"Calibri bold 20",
		"Calibri 20",

		// NFFONT_LARGE_NORMAL_1
		"Calibri bold 22",
		"Calibri 22",

		// NFFONT_XLARGE_NORMAL
		"Calibri bold 24",
		"Calibri 24",

		// NFFONT_XLARGE_NORMAL_1
		"Calibri bold 26",
		"Calibri 26",

		// NFFONT_XLARGE_NORMAL_2
		"Calibri bold 30",
		"Calibri 30",

		// NFFONT_XLARGE_NORMAL_3
		"Calibri bold 36",
		"Calibri 36",
	}
};

static const gchar* font_name_kor[NUM_DISP_MODE][NFFONT_END] = {
	{
		// NFFONT_MINI_NORMAL
		"MALGUN bold 8",
		"MALGUN 8",

		// NFFONT_MINI_NORMAL_1
		"MALGUN bold 9",
		"MALGUN 9",

		// NFFONT_MINI_NORMAL_3
		"MALGUN bold 10",
		"MALGUN 10",

		// NFFONT_MINI_NORMAL_4
		"MALGUN bold 12",
		"MALGUN 12",

		// NFFONT_MINI_NORMAL_5
		"MALGUN bold 13",
		"MALGUN 13",

		// NFFONT_SMALL_NORMAL
		"MALGUN bold 14",
		"MALGUN 14",
		"MALGUN 14",

		// NFFONT_SMALL_NORMAL_1
		"MALGUN bold 15",
		"MALGUN 15",

		// NFFONT_MEDIUM_NORMAL
		"MALGUN bold 16",
		"MALGUN 16",
		"MALGUN 16",

		// NFFONT_LARGE_NORMAL
		"MALGUN bold 18",
		"MALGUN 18",

		// NFFONT_LARGE_NORMAL_1
		"MALGUN bold 20",
		"MALGUN 20",

		// NFFONT_XLARGE_NORMAL
		"MALGUN bold 22",
		"MALGUN 22",

		// NFFONT_XLARGE_NORMAL_1
		"MALGUN bold 23",
		"MALGUN 23",

		// NFFONT_XLARGE_NORMAL_2
		"MALGUN bold 26",
		"MALGUN 26",

		// NFFONT_XLARGE_NORMAL_3
		"MALGUN bold 29",
		"MALGUN 29",
	},
// KOREAN - 4D1
	{
		// NFFONT_MINI_NORMAL
		"MALGUN bold 8",
		"MALGUN 8",

		// NFFONT_MINI_NORMAL_1
		"MALGUN bold 9",
		"MALGUN 9",

		// NFFONT_MINI_NORMAL_3
		"MALGUN bold 10",
		"MALGUN 10",

		// NFFONT_MINI_NORMAL_4
		"MALGUN bold 12",
		"MALGUN 12",

		// NFFONT_MINI_NORMAL_5
		"MALGUN bold 13",
		"MALGUN 13",

		// NFFONT_SMALL_NORMAL
		"MALGUN bold 14",
		"MALGUN 14",
		"MALGUN 14",

		// NFFONT_SMALL_NORMAL_1
		"MALGUN bold 15",
		"MALGUN 15",

		// NFFONT_MEDIUM_NORMAL
		"MALGUN bold 16",
		"MALGUN 16",
		"MALGUN 16",

		// NFFONT_LARGE_NORMAL
		"MALGUN bold 18",
		"MALGUN 18",

		// NFFONT_LARGE_NORMAL_1
		"MALGUN bold 20",
		"MALGUN 20",

		// NFFONT_XLARGE_NORMAL
		"MALGUN bold 22",
		"MALGUN 22",

		// NFFONT_XLARGE_NORMAL_1
		"MALGUN bold 23",
		"MALGUN 23",

		// NFFONT_XLARGE_NORMAL_2
		"MALGUN bold 26",
		"MALGUN 26",

		// NFFONT_XLARGE_NORMAL_3
		"MALGUN bold 29",
		"MALGUN 29",
	}
};

static const gchar* font_name_jpn[NUM_DISP_MODE][NFFONT_END] = {
	{
		// NFFONT_MINI_NORMAL
		"MALGUN bold 8",
		"MALGUN bold 8",

		// NFFONT_MINI_NORMAL_1
		"MALGUN bold 9",
		"MALGUN bold 9",

		// NFFONT_MINI_NORMAL_3
		"MALGUN bold 10",
		"MALGUN bold 10",

		// NFFONT_MINI_NORMAL_4
		"MALGUN bold 12",
		"MALGUN bold 12",

		// NFFONT_MINI_NORMAL_5
		"MALGUN bold 13",
		"MALGUN bold 13",

		// NFFONT_SMALL_NORMAL
		"MALGUN bold 14",
		"MALGUN bold 14",
		"MALGUN 14",

		// NFFONT_SMALL_NORMAL_1
		"MALGUN bold 14",
		"MALGUN bold 14",

		// NFFONT_MEDIUM_NORMAL
		"MALGUN bold 15",
		"MALGUN bold 15",
		"MALGUN 15",

		// NFFONT_LARGE_NORMAL
		"MALGUN bold 16",		//org: "MALGUN bold 18",
		"MALGUN bold 16",		//org: "MALGUN bold 18",

		// NFFONT_LARGE_NORMAL_1
		"MALGUN bold 18",		//org: "MALGUN bold 20",
		"MALGUN bold 18",		//org: "MALGUN bold 20",

		// NFFONT_XLARGE_NORMAL
		"MALGUN bold 18",		//org: "MALGUN bold 22",
		"MALGUN bold 18",		//org: "MALGUN bold 22",

		// NFFONT_XLARGE_NORMAL_1
		"MALGUN bold 23",
		"MALGUN bold 23",

		// NFFONT_XLARGE_NORMAL_2
		"MALGUN bold 26",
		"MALGUN bold 26",

		// NFFONT_XLARGE_NORMAL_3
		"MALGUN bold 29",
		"MALGUN bold 29",
	},
// JAPANESE - 4D1
	{
		// NFFONT_MINI_NORMAL
		"MALGUN bold 8",
		"MALGUN bold 8",

		// NFFONT_MINI_NORMAL_1
		"MALGUN bold 9",
		"MALGUN bold 9",

		// NFFONT_MINI_NORMAL_3
		"MALGUN bold 10",
		"MALGUN bold 10",

		// NFFONT_MINI_NORMAL_4
		"MALGUN bold 12",
		"MALGUN bold 12",

		// NFFONT_MINI_NORMAL_5
		"MALGUN bold 13",
		"MALGUN bold 13",

		// NFFONT_SMALL_NORMAL
		"MALGUN bold 14",
		"MALGUN bold 14",
		"MALGUN 14",

		// NFFONT_SMALL_NORMAL_1
		"MALGUN bold 14",
		"MALGUN bold 14",

		// NFFONT_MEDIUM_NORMAL
		"MALGUN bold 15",
		"MALGUN bold 15",
		"MALGUN 15",

		// NFFONT_LARGE_NORMAL
		"MALGUN bold 16",		//org: "MALGUN bold 18",
		"MALGUN bold 16",		//org: "MALGUN bold 18",

		// NFFONT_LARGE_NORMAL_1
		"MALGUN bold 18",		//org: "MALGUN bold 20",
		"MALGUN bold 18",		//org: "MALGUN bold 20",

		// NFFONT_XLARGE_NORMAL
		"MALGUN bold 18",		//org: "MALGUN bold 22",
		"MALGUN bold 18",		//org: "MALGUN bold 22",

		// NFFONT_XLARGE_NORMAL_1
		"MALGUN bold 23",
		"MALGUN bold 23",

		// NFFONT_XLARGE_NORMAL_2
		"MALGUN bold 26",
		"MALGUN bold 26",

		// NFFONT_XLARGE_NORMAL_3
		"MALGUN bold 29",
		"MALGUN bold 29",
	}
};

static const gchar* font_name_cht[NUM_DISP_MODE][NFFONT_END] = {
	{
		// NFFONT_MINI_NORMAL
		"SIMHEI 8",
		"SIMHEI 8",

		// NFFONT_MINI_NORMAL_1
		"SIMHEI 9",
		"SIMHEI 9",

		// NFFONT_MINI_NORMAL_3
		"SIMHEI 10",
		"SIMHEI 10",

		// NFFONT_MINI_NORMAL_4
		"SIMHEI 12",
		"SIMHEI 12",

		// NFFONT_MINI_NORMAL_5
		"SIMHEI 13",
		"SIMHEI 13",

		// NFFONT_SMALL_NORMAL
		"SIMHEI 14",
		"SIMHEI 14",
		"SIMHEI 14",

		// NFFONT_SMALL_NORMAL_1
		"SIMHEI 14",
		"SIMHEI 14",

		// NFFONT_MEDIUM_NORMAL
		"SIMHEI 15",
		"SIMHEI 15",
		"SIMHEI 15",

		// NFFONT_LARGE_NORMAL
		"SIMHEI 16",		//org: "MALGUN bold 18",
		"SIMHEI 16",		//org: "MALGUN bold 18",

		// NFFONT_LARGE_NORMAL_1
		"SIMHEI 18",		//org: "MALGUN bold 20",
		"SIMHEI 18",		//org: "MALGUN bold 20",

		// NFFONT_XLARGE_NORMAL
		"SIMHEI 18",		//org: "MALGUN bold 22",
		"SIMHEI 18",		//org: "MALGUN bold 22",

		// NFFONT_XLARGE_NORMAL_1
		"SIMHEI 23",
		"SIMHEI 23",

		// NFFONT_XLARGE_NORMAL_2
		"SIMHEI 26",
		"SIMHEI 26",

		// NFFONT_XLARGE_NORMAL_3
		"SIMHEI 29",
		"SIMHEI 29",
	},
// CHINESE - 4D1
	{
		// NFFONT_MINI_NORMAL
		"SIMHEI 8",
		"SIMHEI 8",

		// NFFONT_MINI_NORMAL_1
		"SIMHEI 9",
		"SIMHEI 9",

		// NFFONT_MINI_NORMAL_3
		"SIMHEI 10",
		"SIMHEI 10",

		// NFFONT_MINI_NORMAL_4
		"SIMHEI 12",
		"SIMHEI 12",

		// NFFONT_MINI_NORMAL_5
		"SIMHEI 13",
		"SIMHEI 13",

		// NFFONT_SMALL_NORMAL
		"SIMHEI 14",
		"SIMHEI 14",
		"SIMHEI 14",

		// NFFONT_SMALL_NORMAL_1
		"SIMHEI 14",
		"SIMHEI 14",

		// NFFONT_MEDIUM_NORMAL
		"SIMHEI 15",
		"SIMHEI 15",
		"SIMHEI 15",

		// NFFONT_LARGE_NORMAL
		"SIMHEI 16",		//org: "MALGUN bold 18",
		"SIMHEI 16",		//org: "MALGUN bold 18",

		// NFFONT_LARGE_NORMAL_1
		"SIMHEI 18",		//org: "MALGUN bold 20",
		"SIMHEI 18",		//org: "MALGUN bold 20",

		// NFFONT_XLARGE_NORMAL
		"SIMHEI 18",		//org: "MALGUN bold 22",
		"SIMHEI 18",		//org: "MALGUN bold 22",

		// NFFONT_XLARGE_NORMAL_1
		"SIMHEI 23",
		"SIMHEI 23",

		// NFFONT_XLARGE_NORMAL_2
		"SIMHEI 26",
		"SIMHEI 26",

		// NFFONT_XLARGE_NORMAL_3
		"SIMHEI 29",
		"SIMHEI 29",
	}
};

//static const gchar ***font_name = font_name_eng;

GdkFont* nffont_get_font(nffont_type font_type)
{
	if(font_type<NFFONT_SMALL_NORMAL || font_type>=NFFONT_END)
		return NULL;

	if(nffont[font_type]) {
		return nffont[font_type];
	} else	{
		return NULL;
	}
}

int init_nffont(nffont_charset_type charset_type)	{
	GdkFont* gdkfont = NULL;
	int i=0;
	int ret = 0;
	guint disp_mode = DISPLAY_IS_D1 ? DISPLAY_MODE_D1 : DISPLAY_MODE_4D1;

	for(i=0; i<NFFONT_END; i++)	{
		gdkfont = gdk_fontset_load(strFont[charset_type][disp_mode][i]);
		if(!gdkfont)	{
			g_warning("gdk_fontset_load error : %s", strFont[charset_type][disp_mode][i]);
			nffont[i] = NULL;
			ret = -1;
		} else {
		//	g_warning("gdk_fontset_load success : %s", strFont[charset_type][disp_mode][i]);
			nffont[i] = gdkfont;
		}
	}
	return ret;
}

void nffont_unref_all()
{
	guint j;

	for(j=0; j<NFFONT_END; j++)
	{
		if(nffont[j])
		{
			gdk_font_unref(nffont[j]);
			nffont[j] = NULL;
		}
	}
}

int init_nffont_for_pango(nffont_charset_type charset_type)	{
	int ret = 0;
	return ret;
}

int nffont_change_font(char *lang)
{
	int i, j;
	
	memset(name_buf, 0x00, sizeof(name_buf));

	if			(	!strcmp(lang, "KOREAN") )	{
		for (i = 0; i < NUM_DISP_MODE; ++i) {
			for (j = 0; j < NFFONT_END; ++j) {
				strcpy(name_buf[i][j], font_name_kor[i][j]);
//				printf("LANG TABLE [%d][%d], [%s]\n", i, j, name_buf[i][j]);
			}
		}
	} else if	(	!strcmp(lang, "JAPANESE") )	{
		for (i = 0; i < NUM_DISP_MODE; ++i) {
			for (j = 0; j < NFFONT_END; ++j) {
				strcpy(name_buf[i][j], font_name_jpn[i][j]);
//				printf("LANG TABLE [%d][%d], [%s]\n", i, j, name_buf[i][j]);
			}
		}
	} else if	(	!strcmp(lang, "CHINESE(T)") )	{
		for (i = 0; i < NUM_DISP_MODE; ++i) {
			for (j = 0; j < NFFONT_END; ++j) {
				strcpy(name_buf[i][j], font_name_cht[i][j]);
//				printf("LANG TABLE [%d][%d], [%s]\n", i, j, name_buf[i][j]);
			}
		}
	}
	else {
		for (i = 0; i < NUM_DISP_MODE; ++i) {
			for (j = 0; j < NFFONT_END; ++j) {
				strcpy(name_buf[i][j], font_name_eng[i][j]);
//				printf("LANG TABLE [%d][%d], [%s]\n", i, j, name_buf[i][j]);
			}
		}
	}

#if 0
	if			(	!strcmp(lang, "KOREAN") )	{
		font_name[0] = font_name_kor[0];
		font_name[1] = font_name_kor[1];
	} else if	(	!strcmp(lang, "JAPANESE") )	{
		font_name[0] = font_name_jpn[0];
		font_name[1] = font_name_jpn[1];
	} else if	(	!strcmp(lang, "CHINESE(T)") )	{
		font_name[0] = font_name_cht[0];
		font_name[1] = font_name_cht[1];
	}
	else {
		g_warning("get_language_enum use default language(ENGLISH)");
		font_name[0] = font_name_eng[0];
		font_name[1] = font_name_eng[1];
	}
#endif
	return 0;
}

gchar* nffont_get_pango_font(nffont_type font_type)
{
	guint disp_mode;

	if(DISPLAY_IS_D1)	disp_mode = DISPLAY_MODE_D1;
	else				disp_mode = DISPLAY_MODE_4D1;

	return font_name[disp_mode][font_type];
}

gboolean nffont_is_system_font(char *font)
{
	int i, j;
	for (i = 0; i < NUM_DISP_MODE; ++i) {
		for (j = 0; j < NFFONT_END; ++j) {
			if (font == name_buf[i][j]) return TRUE;
		}
	}

	return FALSE;
}
