#ifndef __NF_EDID_H__
#define __NF_EDID_H__

/* type macro */
#define NF_TYPE_EDID					(nf_edid_get_type ())

#define NF_IS_EDID(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NF_TYPE_EDID))
#define NF_IS_EDID_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_EDID))

#define NF_EDID_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_EDID, NfEdidClass))
#define NF_EDID(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj), NF_TYPE_EDID, NfEdid))
#define NF_EDID_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_EDID, NfEdidClass))

#define NF_EDID_CAST(obj)				((NfEdid*)(obj))
#define NF_EDID_CLASS_CAST(klass)		((NfEdidClass*)(klass))

typedef struct _NfEdid		NfEdid;
typedef struct _NfEdidClass	NfEdidClass;

typedef enum _NF_EDID_TYPE_E
{
	NF_EDID_MODE_ITX	= 0,
	NF_EDID_MODE_NVT	= 1

} NF_EDID_TYPE;

enum _NF_EDID_PRI_E
{   
	//AUTO  
	EDID_PRI_UNKNOWN             = 0,
	// NTSC
	EDID_PRI_800_600_60          = 22,
	EDID_PRI_1080P_25            = 24,
	EDID_PRI_720P_60             = 25,
	EDID_PRI_1280_1024_60        = 26,
	EDID_PRI_1080P_50            = 27,       // For NTSC
	EDID_PRI_1080P_30            = 28,
	EDID_PRI_1080P_60            = 29,
	EDID_PRI_2560_1440_30        = 30,
	EDID_PRI_2560_1440_60        = 31,
	EDID_PRI_2560_1600_60        = 32,
	EDID_PRI_3840_2160_30        = 33,
	EDID_PRI_3840_2160_60        = 34,
	
	// PAL
	EDID_PRI_800_600_60_PAL      = 23,
	EDID_PRI_720P_60_PAL         = 35,
	EDID_PRI_1280_1024_60_PAL    = 36,
	EDID_PRI_1080P_30_PAL        = 37,
	EDID_PRI_1080P_60_PAL        = 38,
	EDID_PRI_1080P_25_PAL        = 39,
	EDID_PRI_1080P_50_PAL        = 40,       // For PAL
	EDID_PRI_2560_1440_30_PAL    = 41,
	EDID_PRI_2560_1440_60_PAL    = 42,
	EDID_PRI_2560_1600_60_PAL    = 43,
	#if defined(ENABLE_DISPLAY_2160P)   
		EDID_PRI_3840_2160_30_PAL    = 44,           // 3840x2160@25
		EDID_PRI_3840_2160_60_PAL    = 45,
	#endif
	
	EDID_PRI_NR
};

typedef enum _NF_EDID_RES_E
{
	NF_EDID_RES_1280_1024_60    = 0,
	NF_EDID_RES_1280_1024_75    = 1,
	NF_EDID_RES_720P_30         = 2,
	NF_EDID_RES_720P_25         = 3,
	NF_EDID_RES_720P_60         = 4,
	NF_EDID_RES_720P_50         = 5,
	NF_EDID_RES_1080P_60        = 6,
	NF_EDID_RES_1080P_50        = 7,
	NF_EDID_RES_1080P_30        = 8,
	NF_EDID_RES_1080P_25        = 9,
	NF_EDID_RES_2160P_30        = 10,
	NF_EDID_RES_2160P_60        = 11,
	NF_EDID_RES_2160P_25        = 12,
	NF_EDID_RES_2160P_50        = 13,
	NF_EDID_RES_1440P_30        = 14,
	NF_EDID_RES_1440P_60        = 15,
	NF_EDID_RES_1600P_60        = 16,

	NF_EDID_RES_MAX
} NF_EDID_RES;

typedef struct _NF_EDID_SUPPORT_RESOLUTION_E
{
	gint    is_1280_1024_60;
	gint    is_1280_1024_75;
	gint    is_720p30;
	gint    is_720p25;
	gint    is_720p60;
	gint    is_720p50;
	gint    is_1080p60;
	gint    is_1080p50;
	gint    is_1080p30;
	gint    is_1080p25;
	gint    is_2160p30;
	gint    is_2160p60;
	gint    is_2160p25;
	gint    is_2160p50;
	gint    is_1440p30;
	gint    is_1440p60;
	gint    is_1600p60;

} NF_EDID_SUPPORT_RESOLUTION;

/**
 * NfEdid:
 *
 * NfDVR notify class
 */
struct _NfEdid {
	NfObject            object;

	/*< public >*/
	gint                init_done;
	gboolean            init_done_main;

	GMainContext        *context;
	GMainLoop           *loop;

	GAsyncQueue         *queue;

	GThread             *thread;

	NF_EDID_SUPPORT_RESOLUTION info;
	gint				edid_mode;

}__attribute__((packed));

struct _NfEdidClass {
	NfObjectClass   parent_class;

	/* signals */

	/*< public >*/

	/*< private >*/

}__attribute__((packed));

gboolean nf_edid_check_avariable_resolution(gint resol, gboolean is_vga);

#endif

