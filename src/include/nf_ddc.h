#ifndef __NF_DDC_H__
#define __NF_DDC_H__

#define NF_DDC_EDID_DUMP_FILE_PATH				"/tmp/webra-info"
#define NF_DDC_EDID_DUMP_FILE					"/tmp/webra-info/edid_dump.txt"

#define NF_DDC_EDID_SIZE						128
#define NF_DDC_EDID_EXT_MAX_SIZE				512
#define NF_DDC_MAX_EDID_EXT_NUM					4

#define DDC_EDID_HEADER							0x00
#define DDC_EDID_HEADER_END						0x07
#define DDC_ID_MANUFACTURER_NAME				0x08
#define DDC_ID_MANUFACTURER_NAME_END			0x09
#define DDC_ID_MODEL							0x0a
#define DDC_EDID_STRUCT_VERSION					0x12
#define DDC_EDID_STRUCT_REVISION				0x13
#define DDC_DPMS_FLAGS							0x18
#define DDC_ESTABLISHED_TIMING_1				0x23
#define DDC_ESTABLISHED_TIMING_2				0x24
#define MANUFACTURERS_TIMINGS					0x25

#define NF_DDC_EDID_EXT_FLAG					0x7e

#define DDC_DETAILED_TIMING_DESCRIPTIONS_START	0x36
#define DDC_DETAILED_TIMING_DESCRIPTION_SIZE	18
#define DDC_NO_DETAILED_TIMING_DESCRIPTIONS		4

#define DDC_MONITOR_NAME						0xfc
#define DDC_MONITOR_LIMITS						0xfd

#define DDC_UNKNOWN_DESCRIPTOR					-1
#define DDC_DETAILED_TIMING_BLOCK				-2
#define DDC_STANDARD_TIMING_BLOCK				0xfa
#define DDC_EDID_DETAIL_CVT_3BYTE				0xf8

#define DDC_DESCRIPTOR_DATA						5
#define DDC_V_MIN_RATE							block[5]
#define DDC_V_MAX_RATE							block[6]
#define DDC_H_MIN_RATE							block[7]
#define DDC_H_MAX_RATE							block[8]

#define DDC_MAX_PIXEL_CLOCK						(((int)block[ 9 ]) * 10)
#define DDC_GTF_SUPPORT							block[10]

#define DDC_DPMS_ACTIVE_OFF						(1 << 5)
#define DDC_DPMS_SUSPEND						(1 << 6)
#define DDC_DPMS_STANDBY						(1 << 7)


#define COMBINE_HI_8LO( hi, lo )				((((unsigned)hi) << 8) | (unsigned)lo)
#define UPPER_NIBBLE( x )						(((128|64|32|16) & (x)) >> 4)
#define LOWER_NIBBLE( x )						((1|2|4|8) & (x))
#define COMBINE_HI_8LO( hi, lo )				((((unsigned)hi) << 8) | (unsigned)lo)
#define COMBINE_HI_4LO( hi, lo )				((((unsigned)hi) << 4) | (unsigned)lo)


#define NF_EDID_LENGTH							0x80

#define PIXEL_CLOCK_LO							(unsigned)dtd[ 0 ]
#define PIXEL_CLOCK_HI							(unsigned)dtd[ 1 ]
#define PIXEL_CLOCK								(COMBINE_HI_8LO( PIXEL_CLOCK_HI,PIXEL_CLOCK_LO )*10000)

#define H_ACTIVE_LO								(unsigned)dtd[ 2 ]
#define H_BLANKING_LO							(unsigned)dtd[ 3 ]
#define H_ACTIVE_HI								UPPER_NIBBLE( (unsigned)dtd[ 4 ] )
#define H_ACTIVE								COMBINE_HI_8LO( H_ACTIVE_HI, H_ACTIVE_LO )
#define H_BLANKING_HI							LOWER_NIBBLE( (unsigned)dtd[ 4 ] )
#define H_BLANKING								COMBINE_HI_8LO( H_BLANKING_HI, H_BLANKING_LO )

#define V_ACTIVE_LO								(unsigned)dtd[ 5 ]
#define V_BLANKING_LO							(unsigned)dtd[ 6 ]
#define V_ACTIVE_HI								UPPER_NIBBLE( (unsigned)dtd[ 7 ] )
#define V_ACTIVE								COMBINE_HI_8LO( V_ACTIVE_HI, V_ACTIVE_LO )
#define V_BLANKING_HI							LOWER_NIBBLE( (unsigned)dtd[ 7 ] )
#define V_BLANKING								COMBINE_HI_8LO( V_BLANKING_HI, V_BLANKING_LO )

#define H_SYNC_OFFSET_LO						(unsigned)dtd[ 8 ]
#define H_SYNC_WIDTH_LO							(unsigned)dtd[ 9 ]

#define V_SYNC_OFFSET_LO						UPPER_NIBBLE( (unsigned)dtd[ 10 ] )
#define V_SYNC_WIDTH_LO							LOWER_NIBBLE( (unsigned)dtd[ 10 ] )

#define V_SYNC_WIDTH_HI							((unsigned)dtd[ 11 ] & (1|2))
#define V_SYNC_OFFSET_HI						(((unsigned)dtd[ 11 ] & (4|8)) >> 2)

#define H_SYNC_WIDTH_HI							(((unsigned)dtd[ 11 ] & (16|32)) >> 4)
#define H_SYNC_OFFSET_HI						(((unsigned)dtd[ 11 ] & (64|128)) >> 6)

#define V_SYNC_WIDTH							COMBINE_HI_4LO( V_SYNC_WIDTH_HI, V_SYNC_WIDTH_LO )
#define V_SYNC_OFFSET							COMBINE_HI_4LO( V_SYNC_OFFSET_HI, V_SYNC_OFFSET_LO )

#define H_SYNC_WIDTH							COMBINE_HI_4LO( H_SYNC_WIDTH_HI, H_SYNC_WIDTH_LO )
#define H_SYNC_OFFSET							COMBINE_HI_4LO( H_SYNC_OFFSET_HI, H_SYNC_OFFSET_LO )

#define H_SIZE_LO								(unsigned)dtd[ 12 ]
#define V_SIZE_LO								(unsigned)dtd[ 13 ]

#define H_SIZE_HI								UPPER_NIBBLE( (unsigned)dtd[ 14 ] )
#define V_SIZE_HI								LOWER_NIBBLE( (unsigned)dtd[ 14 ] )

#define H_SIZE									COMBINE_HI_8LO( H_SIZE_HI, H_SIZE_LO )
#define V_SIZE									COMBINE_HI_8LO( V_SIZE_HI, V_SIZE_LO )

#define H_BORDER								(unsigned)dtd[ 15 ]
#define V_BORDER								(unsigned)dtd[ 16 ]

#define FLAGS									(unsigned)dtd[ 17 ]

#define V_INTERLACED							(FLAGS&0x80)
#define SYNC_TYPE								(FLAGS&3<<3)  /* bits 4,3 */
#define SYNC_SEPARATE							(3<<3)
#define VSYNC_POSITIVE							(FLAGS & 4)
#define HSYNC_POSITIVE							(FLAGS & 2)

#define INTERLACED								0x04
#define PROGRESSIVED							0x00

typedef struct{
	guchar HEADER[8];                   // 0~7
	guchar MID[2];                      // 8~9
	guchar PID[2];                      // 10~11
	guchar S_NUMBER[4];                 // 12~15
	guchar week_manufacture[1];         // 16
	guchar year_manufacture[1];         // 17
	guchar EDID_VER[1];                 // 18
	guchar EDID_REV[1];                 // 19
	guchar IN_FMT[1];                   // 20
	guchar IN_MAX_HSIZE[1];             // 21
	guchar IN_MAX_VSIZE[1];             // 22
	guchar DISP_GAMMA[1];               // 23
	guchar PWR_MANAGE[1];               // 24
	guchar CROMA_INFO[10];              // 25~34
	guchar EST_TIM[2];                  // 35~36
	guchar M_TIM[1];                    // 37       Manufacturer's reserved timing
	guchar STD_TIM[16];                 // 38~53
	guchar DESCRIPTOR[4][18];           // 54~71 72~89 90~107 108~125
	guchar EXTENTION_FLAG[1];           // Extension Flag
	guchar CHKSUM[1];                   // Checksum. This byte should be programmed such that the sum of all 128 bytes equals 00h.
} DDC;

#if 0
struct est_timings {
	guchar t1;
	guchar t2;
	guchar mfg_rsvd;
};

struct std_timing {
	guchar hsize; /* need to multiply by 8 then add 248 */
	guchar vfreq_aspect;
};

struct detailed_data_monitor_range {
	guchar min_vfreq;
	guchar max_vfreq;
	guchar min_hfreq_khz;
	guchar max_hfreq_khz;
	guchar pixel_clock_mhz; /* need to multiply by 10 */
	gushort sec_gtf_toggle; /* A000=use above, 20=use below */
	guchar hfreq_start_khz; /* need to multiply by 2 */
	guchar c; /* need to divide by 2 */
	gushort m;
	guchar k;
	guchar j; /* need to divide by 2 */
}__attribute__((packed));

struct detailed_data_string {
	guchar str[13];
}__attribute__((packed));

struct detailed_data_wpindex {
	guchar white_yx_lo; /* Lower 2 bits each */
	guchar white_x_hi;
	guchar white_y_hi;
	guchar gamma; /* need to divide by 100 then add 1 */
}__attribute__((packed));

struct detailed_data_color_point {
	guchar windex1;
	guchar wpindex1[3];
	guchar windex2;
	guchar wpindex2[3];
}__attribute__((packed));

struct cvt_timing {
	guchar code[3];
}__attribute__((packed));

/* If detailed data is pixel timing */
struct detailed_pixel_timing {
	guchar hactive_lo;
	guchar hblank_lo;
	guchar hactive_hblank_hi;
	guchar vactive_lo;
	guchar vblank_lo;
	guchar vactive_vblank_hi;
	guchar hsync_offset_lo;
	guchar hsync_pulse_width_lo;
	guchar vsync_offset_pulse_width_lo;
	guchar hsync_vsync_offset_pulse_width_hi;
	guchar width_mm_lo;
	guchar height_mm_lo;
	guchar width_height_mm_hi;
	guchar hborder;
	guchar vborder;
	guchar misc;
}__attribute__((packed));

struct detailed_non_pixel {
	guchar pad1;
	guchar type; /* ff=serial, fe=string, fd=monitor range, fc=monitor name
			fb=color point data, fa=standard timing data,
			f9=undefined, f8=mfg. reserved */
	guchar pad2;
	union {
		struct detailed_data_string str;
		struct detailed_data_monitor_range range;
		struct detailed_data_wpindex color;
		struct std_timing timings[5];
		struct cvt_timing cvt[4];
	} __attribute__((packed))data;
}__attribute__((packed));

struct detailed_timing {
	gushort pixel_clock; /* need to multiply by 10 KHz */
	union {
		struct detailed_pixel_timing pixel_data;
		struct detailed_non_pixel other_data;
	} __attribute__((packed))data;
}__attribute__((packed));

struct ddc_edid_st {
	guchar header[8];
	/* Vendor & product info */
	guchar mfg_id[2];
	guchar prod_code[2];
	guint serial; /* FIXME: byte order */
	guchar mfg_week;
	guchar mfg_year;
	/* EDID version */
	guchar version;
	guchar revision;
	/* Display info: */
	guchar input;
	guchar width_cm;
	guchar height_cm;
	guchar gamma;
	guchar features;
	/* Color characteristics */
	guchar red_green_lo;
	guchar black_white_lo;
	guchar red_x;
	guchar red_y;
	guchar green_x;
	guchar green_y;
	guchar blue_x;
	guchar blue_y;
	guchar white_x;
	guchar white_y;
	/* Est. timings and mfg rsvd timings*/
	struct est_timings established_timings;
	/* Standard timings 1-8*/
	struct std_timing standard_timings[8];
	/* Detailing timings 1-4 */
	struct detailed_timing detailed_timings[4];
	/* Number of 128 byte ext. blocks */
	guchar extensions;
	/* Checksum */
	guchar checksum;
}__attribute__((packed));
#endif

gboolean nf_ddc_init(void);
gboolean nf_ddc_get_edid_raw_data(guchar *edid_data_vga, guchar *edid_data_hdmi);
gboolean nf_ddc_dump_start(void);
gboolean nf_ddc_parse_raw_data(guchar *edid_data, FILE *fp);
gint nf_ddc_edid_checksum(guchar *edid);
gint nf_ddc_parse_edid(guchar *edid, FILE *fp);
gchar *nf_ddc_get_vendor_sign(guchar const *block);
gint nf_ddc_block_type(guchar* block);
gchar *nf_ddc_get_monitor_name(guchar const *block);
gint nf_ddc_parse_monitor_limits(guchar *block);
gint nf_ddc_parse_timing_description(guchar *dtd, FILE *fp);

#endif

