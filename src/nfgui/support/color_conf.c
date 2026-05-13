
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gdk/gdk.h>

#include "support/nf_ui_color.h"

#include "tools/ix_mem.h"

#include "color_conf.h"


#define CONF_FILE			"./data/gui/color.cfg"
#define STR_COLOR_CNT		"color.cnt"


typedef struct _COLOR_CONF_INFO {
	int ui_type;
	char ver[128];
	int color_cnt;
}CONF_INFO;

enum {
	ITEM_KEY_POS = 0,
	VAL_POS		 = 4
};

int prg_colors[UX_COLOR_END] = 
{
    0, 
    0x929292,
    0xff0000,
    0xffffff,
    0x394e4a,    
    0xffff00,
    0x0000ff,    
    0x00ff00,        
    0x010101,
    0x808080,
    0xff3f00,
    0xff7f00,
    0xffbf00,
    0xbfff00,
    0x202020,
    0x7fff00,
    0x3fff00,
    0x00ff3f,
    0x00ff7f,
    0x00ffbf,
    0x606060,
    0x00ffff,
    0x00bfff,
    0x007fff,
    0x003fff,
    0x3f00ff,
    0xa0a0a0,
    0x7f00ff,
    0xbf00ff,
    0xff00ff,
    0xff00bf,
    0xff007f,
    0xff003f,
    0xff0080,
    0x80ffff,    
	0xff7e00, 

	0x8594a6,
	0x5c6d82,
};

int rtile_colors[UX_RTILE_COLOR_END] = 
{
    0xfff59b,
    0xf567ff,
    0x8dcbff,
    0x37c61f,
    0x664fb4,
    0xa75c78,    
    0xff5a09,        
    0x1f7311,
    0x007eff,
    0x461ce3,    
    0xff9518,        
    0x0f4d0c,
    0x002dff,
    0x5e009e,    
    0xe000be,
    0xffe715,
    0x1a6f7c,
    0x192a67,    
    0x00eae5,
    0x8c2422,
    0x2faec2,
    0x9ff00c,    
    0x3f4996,        
    0xfe1809,    
};

GdkColor *ux_colors;
GdkColor *ux_rtile_colors;
static CONF_INFO c_info;

static xmlTextReaderPtr 	reader;


static int open_conf();
static int close_conf();
static int read_conf();
static int get_info();
static int alloc_mem();
static int put_color();
static int put_prg_color();
static int put_rtile_color();

static void print_xml()
{
	const xmlChar *value;
	int ret;

	ret = xmlTextReaderHasAttributes(reader);
	printf("return %d \n", ret);

	if(ret == 1) {
		value = xmlTextReaderGetAttributeNo(reader, 0); // item key
		if(value) printf("item key= %s ", value);

		value = xmlTextReaderGetAttributeNo(reader, 1); // type
		if(value) printf("type= %s ", value);

		value = xmlTextReaderGetAttributeNo(reader, 2); // min
		if(value) printf("min= %s ", value);

		value = xmlTextReaderGetAttributeNo(reader, 3); // max
		if(value) printf("max= %s ", value);

		value = xmlTextReaderGetAttributeNo(reader, 4); // val
		if(value) printf("val= %s \n", value);
	}
}

static void print_color()
{
	int i;

	if(ux_colors) {
		for(i=0; i<c_info.color_cnt+UX_COLOR_END; i++) {
			printf("ux_colors[%d].red:%0x, ux_colors[%d].green:%0x, ux_colors[%d].blue:%0x\n", 
					i, ux_colors[i].red, 
					i, ux_colors[i].green, 
					i, ux_colors[i].blue);
		}
	}
}

static void print_info()
{
	printf("header.ui_type : %d\n", c_info.ui_type);
	printf("header.ver : %s\n", c_info.ver);
	printf("color_cnt : %d\n", c_info.color_cnt);
}

static int open_conf()
{
	int ret;

	reader = xmlReaderForFile(CONF_FILE, NULL, 0);
	if (reader == NULL ) {
		g_warning("%s [%d] unable to open file. \n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

static int close_conf()
{
	xmlFreeTextReader(reader);
	//xmlCleanupParser();

#if 0
	if(ux_colors) {
		ifree(ux_colors);
		ux_colors = NULL;
	}
#endif

	return 0;
}

static int read_conf()
{
	if(reader != NULL) {
		if(get_info() < 0) return -1;
		if(alloc_mem() < 0) return -1;
		if(put_color() < 0) return -1;
		if(put_prg_color() < 0) return -1;		
		if(put_rtile_color() < 0) return -1;
	}else
		return -1;
	
//	print_color();

	return 0;
}

static int get_info()
{
	const xmlChar *key, *value;
	int ret;

	memset(&c_info, 0x00, sizeof(CONF_INFO));

	ret = xmlTextReaderRead(reader);
	while(ret == 1) 
	{
		if(xmlTextReaderHasAttributes(reader) == 1) 
		{
			key = xmlTextReaderGetAttributeNo(reader, ITEM_KEY_POS); 
			if(key) printf("item key= %s ", key);

			value = xmlTextReaderGetAttributeNo(reader, VAL_POS); 
			if(value) printf("val= %s \n", value);

			if(!xmlStrcmp(key, (xmlChar*)"header.ui_type")) {
				value = xmlTextReaderGetAttributeNo(reader, VAL_POS); 
				c_info.ui_type = atoi((char*)value);
			}
			else if(!xmlStrcmp(key, (xmlChar*)"header.ver")) {
				value = xmlTextReaderGetAttributeNo(reader, VAL_POS); 
				strcpy(c_info.ver, (char*)value);
			}
			else if(!xmlStrcmp(key, (xmlChar*)"color.cnt")) {
				value = xmlTextReaderGetAttributeNo(reader, VAL_POS); 
				c_info.color_cnt = atoi((char*)value);

				return 0;
			}
		}
		ret = xmlTextReaderRead(reader);
	}
	
	return -1;
}

static int alloc_mem()
{
	ux_colors = (GdkColor*)imalloc(sizeof(GdkColor) * (unsigned int)(c_info.color_cnt+UX_COLOR_END));
	if(ux_colors == NULL)
		return -1;
	
	ux_rtile_colors = (GdkColor*)imalloc(sizeof(GdkColor) * UX_RTILE_COLOR_END);
	if(ux_rtile_colors == NULL)
		return -1;
	
	return 0;
}

static int put_color()
{
	const xmlChar *key, *value;
	char buf[32];
	unsigned long uVal;
	int idx = 0;
	int ret;

	//printf("return %d \n", ret);

	ret = xmlTextReaderRead(reader);
	while(ret == 1) 
	{
		if(xmlTextReaderHasAttributes(reader) == 1) 
		{
			key = xmlTextReaderGetAttributeNo(reader, ITEM_KEY_POS); 
			//if(key) printf("item key= %s ", key);

			value = xmlTextReaderGetAttributeNo(reader, VAL_POS); 
			//if(value) printf("val= %s \n", value);

			sprintf(buf, "color.%03d", idx);
			if(!xmlStrcmp(key, (xmlChar*)buf)) {
				uVal = strtoul((char*)value, NULL, 16);

				ux_colors[idx].red 	 = (guint16)((uVal & 0xff0000)>>8);
				ux_colors[idx].green = (guint16)((uVal & 0x00ff00));
				ux_colors[idx].blue  = (guint16)((uVal & 0x0000ff)<<8);

				idx++;
			}
		}
		ret = xmlTextReaderRead(reader);
	}

	if(ret != 0) {
		g_warning("%s [%d] failed to parse. \n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	return 0;
}

static int put_prg_color()
{
    int i;

    for (i = 0; i < UX_COLOR_END; i++)
    {
		ux_colors[c_info.color_cnt+i].red   = (guint16)((prg_colors[i] & 0xff0000)>>8);
		ux_colors[c_info.color_cnt+i].green = (guint16)((prg_colors[i] & 0x00ff00));
		ux_colors[c_info.color_cnt+i].blue  = (guint16)((prg_colors[i] & 0x0000ff)<<8);        
    }   
	
	return 0;
}

static int put_rtile_color()
{
    int i;

    for (i = 0; i < UX_RTILE_COLOR_END; i++)
    {
		ux_rtile_colors[i].red   = (guint16)((rtile_colors[i] & 0xff0000)>>8);
		ux_rtile_colors[i].green = (guint16)((rtile_colors[i] & 0x00ff00));
		ux_rtile_colors[i].blue  = (guint16)((rtile_colors[i] & 0x0000ff)<<8);        
    }   
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int color_init()
{
	if(open_conf() < 0)
		return -1;
	
	if(read_conf() < 0)
		return -1;
	
	if(close_conf() < 0)
		return -1;

	return 0;
}

int get_ui_type()
{
	return c_info.ui_type;
}

int get_prg_init_num()
{
	return c_info.color_cnt;
}

