#include <glib.h>

#include "nf_common.h"
#include "nf_object.h"

#if defined(CHIP_NVT)
#if defined(CHIP_NVT_NT9833x)
	#include <novatek/vendor/vendor_videoout.h>
#else
	#include <novatek/hd_type.h>
	#include <novatek/hd_common.h>
	#include <novatek/hd_videoout.h>
	#include <novatek/vendor/vendor_videoout.h>
#endif
#endif

#include "nf_edid.h"
#include <itx_edid.h>		// in driver/edid/itx_edid.h
#include "nf_fb.h"
#include "nf_edid.nvt.h"
#include "nf_edid.itx.h"
#include "nf_edid_parse.h"

//#define DEBUG_EDID_JBSHELL
#ifdef DEBUG_EDID_JBSHELL
	#include "jbshell.h"
#endif

static GObjectClass *parent_class = NULL;
static NfEdid *_nf_edid = NULL;

/**
	Extern Function Definition
 **/
extern gboolean nf_dev_board_pp_is_pal(void);

/**
	Gloval Function Definition
 **/
static void nf_edid_class_init (NfEdidClass * klass);
static void nf_edid_instance_init (GTypeInstance * instance, gpointer g_class);
static void nf_edid_set_property (GObject * object, guint prop_id,
			const GValue * value, GParamSpec * pspec);
static void nf_edid_get_property (GObject * object, guint prop_id,
			GValue * value, GParamSpec * pspec);
static void nf_edid_dispose (GObject * object);
static void nf_edid_finalize (GObject * object);

GType
nf_edid_get_type (void)
{
	static GType nf_edid_type = 0;

	if (G_UNLIKELY (nf_edid_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfEdidClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_edid_class_init,
			NULL,
			NULL,
			sizeof (NfEdid),
			0,
			(GInstanceInitFunc) nf_edid_instance_init,
			NULL
		};

		nf_edid_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfEdid", &object_info, 0);
	}

	return nf_edid_type;
}

static void
nf_edid_class_init (NfEdidClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_edid_set_property;
	gobject_class->get_property = nf_edid_get_property;

	gobject_class->dispose = nf_edid_dispose;
	gobject_class->finalize = nf_edid_finalize;

}

static void
nf_edid_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfEdid *self = NF_EDID (instance);

	self->init_done = 0;
	self->init_done_main = 0;

	// event context & loop
	self->context = g_main_context_new ();
	self->loop = g_main_loop_new (self->context, FALSE);

	// queue
	self->queue = g_async_queue_new();
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_edid_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_edid_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_edid_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_edid_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

gboolean nf_edid_init(int wait)
{
	g_return_val_if_fail (_nf_edid == NULL, FALSE);

	_nf_edid = g_object_new ( NF_TYPE_EDID , NULL);

	g_message("%s called", __FUNCTION__);

	#if defined(CHIP_NVT)
		_nf_edid->edid_mode=NF_EDID_MODE_NVT;
	#else
		_nf_edid->edid_mode=NF_EDID_MODE_ITX;
	#endif
	
	return TRUE;
}

/*
	struct edid_data
	{
		unsigned char raw_data[EDID_TOT_SIZE];
		int ext_edid;
		int is_fail;
	};
*/
gboolean nf_edid_check_avariable_resolution(gint resol, gboolean is_vga)
{
	NF_EDID_SUPPORT_RESOLUTION *info;
	struct edid_data edid;
	gboolean is_pal=0;

	g_return_if_fail(_nf_edid != NULL);

	is_pal = nf_dev_board_pp_is_pal();

	if(resol > NF_EDID_RES_MAX)
	{
		g_warning("%s Not Supported Resolution %d", __FUNCTION__, resol);
		return FALSE;
	}
	else
		g_message("[%s] is_vga[%d] resol[%d]", __FUNCTION__, is_vga, resol);

	info=(NF_EDID_SUPPORT_RESOLUTION *)&_nf_edid->info;
	memset(info, 0x0, sizeof(NF_EDID_SUPPORT_RESOLUTION));

	if(_nf_edid->edid_mode == NF_EDID_MODE_NVT) {

		if(is_vga) {
			if(!nf_edid_vga(&edid)) {
				g_warning("[%s][EDID CHECK ERROR.. VGA] Available Resolution XXX !! resol %d", __FUNCTION__, resol);
				return FALSE;
			}
		}
		else {
			#if 0
				nf_edid_chk_nvt_hdmi(info);
			#else
				if(!nf_edid_chk_nvt_hdmi_raw_data(&edid)) {
					g_warning("[%s][EDID CHECK ERROR.. HDMI] Available Resolution XXX !! resol %d", __FUNCTION__, resol);
					return FALSE;
				}
			#endif
		}

		#if 0
		if(nf_edid_chk_nvt_valid(&info, resol)) {
		#else
		if(nf_edid_p_chk_valid(&edid, resol, is_pal)) {
		#endif
			g_message("%s Available Resolution OOO !! resol %d", __FUNCTION__, resol);
			return TRUE;
		}
		else {
			g_message("%s Available Resolution XXX !! resol %d", __FUNCTION__, resol);
			return FALSE;
		}
	}
	else {
		g_message("[%s] Not Implemented Yet!!", __FUNCTION__);

		return FALSE;
	}
}

#ifdef DEBUG_EDID_JBSHELL

static char nf_edid_jbshell_cmd_help[] = "nf_edid resolution[0~7]";
static int nf_edid_jbshell_cmd(int argc, char **argv)
{
	gint resolution=0;
	gboolean is_vga=0;

	if ( argc < 3 ) {
		g_message("Invalid arguments\n%s\n", nf_edid_jbshell_cmd_help);
		return -1;
	}

	resolution=(gboolean)strtoul(argv[1], NULL, 10);
	is_vga=(gboolean)strtoul(argv[2], NULL, 10);

	g_message("%s line%d resol %d is_vga %d", __FUNCTION__, __LINE__, resolution, is_vga);

	if(nf_edid_check_avariable_resolution(resolution, is_vga)) {
		g_message("%s line%d Support resol[%d] \n", __FUNCTION__, __LINE__, resolution);
	}
	else {
		g_message("%s line%d Not Support resol[%d]\n", __FUNCTION__, __LINE__, resolution);
	}

	return 0;
}
__commandlist(nf_edid_jbshell_cmd, "nf_edid", nf_edid_jbshell_cmd_help, nf_edid_jbshell_cmd_help);

#endif

