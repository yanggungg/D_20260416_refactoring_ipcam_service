#ifndef __NF_EDID_PARSE_H__
#define __NF_EDID_PARSE_H__

#if 0
#define EDID_SIZE										128
#define EDID_EXT_TOT_SIZE								512
#define EDID_TOT_SIZE									(EDID_SIZE + EDID_EXT_TOT_SIZE)
#endif
#define EDID_MAX_EDID_EXT_NUM							4

#define EDID_DESCRIPTOR_DATA							5
#define EDID_V_MIN_RATE									block[5]
#define EDID_V_MAX_RATE									block[6]
#define EDID_H_MIN_RATE									block[7]
#define EDID_H_MAX_RATE									block[8]

#define EDID_MAX_PIXEL_CLOCK							(((int)block[ 9 ]) * 10)
#define EDID_GTF_SUPPORT								block[10]

#define EDID_HEADER										0x00
#define EDID_HEADER_END									0x07
#define ID_MANUFACTURER_NAME							0x08
#define EDID_MANUFACTURER_NAME_END						0x09
#define EDID_ID_MODEL									0x0a
#define EDID_STRUCT_VERSION								0x12
#define EDID_STRUCT_REVISION							0x13
#define EDID_DPMS_FLAGS									0x18
#define EDID_ESTABLISHED_TIMING_1						0x23
#define EDID_ESTABLISHED_TIMING_2						0x24
#define EDID_MANUFACTURERS_TIMINGS						0x25

#define EDID_EXT_FLAG									0x7e

#define EDID_DETAILED_TIMING_DESCRIPTIONS_START			0x36
#define EDID_DETAILED_TIMING_DESCRIPTION_SIZE			18
#define EDID_NO_DETAILED_TIMING_DESCRIPTIONS			4

#define EDID_MONITOR_NAME								0xfc
#define EDID_MONITOR_LIMITS								0xfd

#define EDID_UNKNOWN_DESCRIPTOR							-1
#define EDID_DETAILED_TIMING_BLOCK						-2
#define EDID_STANDARD_TIMING_BLOCK						0xfa
#define EDID_EDID_DETAIL_CVT_3BYTE						0xf8

#define EDID_DESCRIPTOR_DATA							5
#define EDID_V_MIN_RATE									block[5]
#define EDID_V_MAX_RATE									block[6]
#define EDID_H_MIN_RATE									block[7]
#define EDID_H_MAX_RATE									block[8]

#define EDID_DPMS_ACTIVE_OFF							(1 << 5)
#define EDID_DPMS_SUSPEND								(1 << 6)
#define EDID_DPMS_STANDBY								(1 << 7)

#define COMBINE_HI_8LO( hi, lo )    					((((unsigned)hi) << 8) | (unsigned)lo)

#define COMBINE_HI_8LO( hi, lo )						((((unsigned)hi) << 8) | (unsigned)lo)
#define UPPER_NIBBLE( x )								(((128|64|32|16) & (x)) >> 4)
#define LOWER_NIBBLE( x )								((1|2|4|8) & (x))
#define COMBINE_HI_8LO( hi, lo )						((((unsigned)hi) << 8) | (unsigned)lo)
#define COMBINE_HI_4LO( hi, lo )						((((unsigned)hi) << 4) | (unsigned)lo)


#define EDID_LENGTH										0x80

#define HPOSITIVE										0x02
#define HNEGATIVE										0x00
#define VPOSITIVE										0x01
#define VNEGATIVE										0x00

#define PIXEL_CLOCK_LO									(unsigned)dtd[ 0 ]
#define PIXEL_CLOCK_HI									(unsigned)dtd[ 1 ]
#define PIXEL_CLOCK										(COMBINE_HI_8LO( PIXEL_CLOCK_HI,PIXEL_CLOCK_LO )*10000)

#define H_ACTIVE_LO										(unsigned)dtd[ 2 ]
#define H_BLANKING_LO									(unsigned)dtd[ 3 ]
#define H_ACTIVE_HI										UPPER_NIBBLE( (unsigned)dtd[ 4 ] )
#define H_ACTIVE										COMBINE_HI_8LO( H_ACTIVE_HI, H_ACTIVE_LO )
#define H_BLANKING_HI									LOWER_NIBBLE( (unsigned)dtd[ 4 ] )
#define H_BLANKING										COMBINE_HI_8LO( H_BLANKING_HI, H_BLANKING_LO )

#define V_ACTIVE_LO										(unsigned)dtd[ 5 ]
#define V_BLANKING_LO									(unsigned)dtd[ 6 ]
#define V_ACTIVE_HI										UPPER_NIBBLE( (unsigned)dtd[ 7 ] )
#define V_ACTIVE										COMBINE_HI_8LO( V_ACTIVE_HI, V_ACTIVE_LO )
#define V_BLANKING_HI									LOWER_NIBBLE( (unsigned)dtd[ 7 ] )
#define V_BLANKING										COMBINE_HI_8LO( V_BLANKING_HI, V_BLANKING_LO )

#define H_SYNC_OFFSET_LO								(unsigned)dtd[ 8 ]
#define H_SYNC_WIDTH_LO									(unsigned)dtd[ 9 ]

#define V_SYNC_OFFSET_LO								UPPER_NIBBLE( (unsigned)dtd[ 10 ] )
#define V_SYNC_WIDTH_LO									LOWER_NIBBLE( (unsigned)dtd[ 10 ] )

#define V_SYNC_WIDTH_HI									((unsigned)dtd[ 11 ] & (1|2))
#define V_SYNC_OFFSET_HI								(((unsigned)dtd[ 11 ] & (4|8)) >> 2)

#define H_SYNC_WIDTH_HI									(((unsigned)dtd[ 11 ] & (16|32)) >> 4)
#define H_SYNC_OFFSET_HI								(((unsigned)dtd[ 11 ] & (64|128)) >> 6)

#define V_SYNC_WIDTH									COMBINE_HI_4LO( V_SYNC_WIDTH_HI, V_SYNC_WIDTH_LO )
#define V_SYNC_OFFSET									COMBINE_HI_4LO( V_SYNC_OFFSET_HI, V_SYNC_OFFSET_LO )

#define H_SYNC_WIDTH									COMBINE_HI_4LO( H_SYNC_WIDTH_HI, H_SYNC_WIDTH_LO )
#define H_SYNC_OFFSET									COMBINE_HI_4LO( H_SYNC_OFFSET_HI, H_SYNC_OFFSET_LO )

#define H_SIZE_LO										(unsigned)dtd[ 12 ]
#define V_SIZE_LO										(unsigned)dtd[ 13 ]

#define H_SIZE_HI										UPPER_NIBBLE( (unsigned)dtd[ 14 ] )
#define V_SIZE_HI										LOWER_NIBBLE( (unsigned)dtd[ 14 ] )

#define H_SIZE											COMBINE_HI_8LO( H_SIZE_HI, H_SIZE_LO )
#define V_SIZE											COMBINE_HI_8LO( V_SIZE_HI, V_SIZE_LO )

#define H_BORDER										(unsigned)dtd[ 15 ]
#define V_BORDER										(unsigned)dtd[ 16 ]

#define FLAGS											(unsigned)dtd[ 17 ]

#define V_INTERLACED									(FLAGS & 0x80)
#define SYNC_TYPE										(FLAGS & (3<<3))  /* bits 4,3 */
#define SYNC_SEPARATE									(3 << 3)
#define VSYNC_POSITIVE									(FLAGS & 4)
#define HSYNC_POSITIVE									(FLAGS & 2)

#define INTERLACED										0x04
#define PROGRESSIVED									0x00
#define VGA_ON											0x04
#define PAL4DM											0x08

#define EDID_CEA_EDID_MAX								64
#define EDID_CEA_EDID_MAX_STR							64

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
} EDID;

gboolean nf_edid_p_chk_valid(struct edid_data *info, gint resol, gboolean is_pal);
gboolean nf_edid_p_chk_timing_detailed(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info, gboolean is_pal);
gboolean nf_edid_p_chk_timing_standard(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info);
gboolean nf_edid_p_chk_timing_established(guchar *data, NF_EDID_SUPPORT_RESOLUTION *info);
gboolean nf_edid_p_timing_description(guchar *dtd, NF_EDID_SUPPORT_RESOLUTION *info, gboolean is_pal);
gchar *nf_edid_p_get_monitor_name(guchar *data);
gint nf_edid_p_checksum(guchar *edid);
gint nf_edid_p_block_type(u_char* block);
gchar *nf_edid_p_get_vendor_sign(u_char const *block);
void nf_edid_p_print_raw_data(guchar *edid);

#endif

