
#include "nf_afx.h"
#include "nf_ui_image.h"
#include "nf_ui_tool.h"

#include "iux_afx.h"

#include "ix_func.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"IMG"

//#define	__MEMORY_PROBE__
//#define LIMIT_IMG_MEMORY    (20000)     //KByte

#define	IMG_PATH		"./data/gui/bmp/OTM/%s/%s"
#define	MKFULLPATH(x)	(0 ? (g_strdup_printf(IMG_PATH, "D1", x)):(g_strdup_printf(IMG_PATH, "4D1", x)))

#define COLOR2INT(a)    ((a.red >> 8) << 24 | (a.green >> 8) << 16 | (a.blue >> 8) << 8 | 0xff)
#define PCOLOR2INT(a)   (a->red << 24 | a->green << 16 | a->blue << 8 | 0xff)

static GHashTable* pbuf_hash_table = NULL;
static double use_mem = 0.0;


static gint _insert_image_hash_table(const gchar *key, GdkPixbuf* image)
{
    GdkPixbuf *hash_pbuf = NULL;
    int img_w, img_h;
	int added_mem = 0;

    if (!key) g_assert(0);
    if (!image) g_assert(0);

	if (pbuf_hash_table == NULL)
		pbuf_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

    hash_pbuf = (GdkPixbuf *)g_hash_table_lookup(pbuf_hash_table, key);

    if (hash_pbuf) return -1;

#ifdef	__MEMORY_PROBE__
    img_w = gdk_pixbuf_get_width(image);
    img_h = gdk_pixbuf_get_height(image);

    if (gdk_pixbuf_get_has_alpha(image))
        added_mem = (img_w * img_h * 4);
    else
        added_mem = (img_w * img_h * 3);

    use_mem += added_mem;

    DMSG(1, "ADD) W=[%d], H=[%d], SIZE=[%d Bytes], TOTAL=[%.3f KB]", img_w, img_h, added_mem, use_mem/1024);
#endif

    g_hash_table_insert(pbuf_hash_table, g_strdup(key), image);

    return 0;
}

static gint _remove_image_hash_table(const gchar *key)
{
    GdkPixbuf *hash_pbuf = NULL;
    int img_w, img_h;
	int added_mem = 0;

    if (!key) g_assert(0);	
	if (pbuf_hash_table == NULL) return -1;
    
	hash_pbuf = (GdkPixbuf *)g_hash_table_lookup(pbuf_hash_table, key);

	if (hash_pbuf == NULL) return -1;

#ifdef	__MEMORY_PROBE__
    img_w = gdk_pixbuf_get_width(hash_pbuf);
    img_h = gdk_pixbuf_get_height(hash_pbuf);

    if (gdk_pixbuf_get_has_alpha(hash_pbuf))
        added_mem = (img_w * img_h * 4);
    else
        added_mem = (img_w * img_h * 3);

    use_mem -= added_mem;

    DMSG(1, "RMV) W=[%d], H=[%d], SIZE=[%d Bytes], TOTAL=[%.3f KB]", img_w, img_h, added_mem, use_mem/1024);
#endif

	g_hash_table_remove(pbuf_hash_table, key);

    return 0;
}

static GdkPixbuf* _lookup_image_hash_table(const gchar *key)
{
    GdkPixbuf *hash_pbuf = NULL;  

    if (!key) g_assert(0);
	if (pbuf_hash_table == NULL) return 0;

	hash_pbuf = (GdkPixbuf *)g_hash_table_lookup(pbuf_hash_table, key);

    return hash_pbuf;
}

static GdkPixbuf* _create_image_main_method(const gchar *create_image, 
									const gchar *top_img, const gchar *bottom_img, 
									const gchar *left_img, const gchar *right_img, 
									gint menu_size_w, gint menu_size_h, GdkColor bg_color)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;

	GdkPixbuf *nbuf = NULL;
	GdkPixbuf *inner = NULL;

	gint img_w, img_h;
	gint top_img_h;

    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, menu_size_w, menu_size_h);
	gdk_pixbuf_fill (nbuf, 0x000000ff);

// <---- fill bg color
	inner = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, menu_size_w, menu_size_h);
	gdk_pixbuf_fill(inner, COLOR2INT(bg_color));
	gdk_pixbuf_composite(inner, nbuf, 0, 0, menu_size_w, menu_size_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(inner);

// <---- top image
  	full_path = MKFULLPATH(top_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }
    
    top_img_h = img_h;

// <---- left image
  	full_path = MKFULLPATH(left_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, top_img_h, img_w, img_h, 0, top_img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }
    
// <---- right image
  	full_path = MKFULLPATH(right_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, menu_size_w-img_w, top_img_h, img_w, img_h, menu_size_w-img_w, top_img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }
    
// <---- bottom image
  	full_path = MKFULLPATH(bottom_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, menu_size_h-img_h, img_w, img_h, 0, menu_size_h-img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

    _insert_image_hash_table(create_image, nbuf);

// <-------- end
  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// <-------- end  
	pbImage = NULL;

	return nbuf;
}

static GdkPixbuf* _create_image_popup_method(const gchar *create_image, guint popup_width, guint popup_height, 
				const gchar *up_left_img, const gchar *up_img, const gchar *up_right_img,
				const gchar *down_left_img, const gchar *down_img, const gchar *down_right_img,
				const gchar *left_img, const gchar *right_img, const gchar *line_img, GdkColor bg_color)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;

	GdkPixbuf *nbuf = NULL;
	GdkPixbuf *inner = NULL;

	gint img_w, img_h;

	gint pos_x, pos_y;
	gint up_height, down_height, left_width, right_width;
	gint copy_x, copy_y, copy_width, copy_height;

    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;
	
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, popup_width, popup_height);
	gdk_pixbuf_fill (nbuf, 0x000000ff);
//
	if(!nbuf)
	{
		g_assert(0);
		return NULL;
	}

// <---- left up image
  	full_path = MKFULLPATH(up_left_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }
    

	left_width = img_w;

// <---- right up image
  	full_path = MKFULLPATH(up_right_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, popup_width-img_w, 0, img_w, img_h, popup_width-img_w, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	right_width = img_w;

// <---- up image
  	full_path = MKFULLPATH(up_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, left_width, 0, popup_width-left_width-right_width, img_h, left_width, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	up_height = img_h;

// <---- left down image
  	full_path = MKFULLPATH(down_left_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, popup_height-img_h, img_w, img_h, 0, popup_height-img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// <---- right down image
  	full_path = MKFULLPATH(down_right_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, popup_width-img_w, popup_height-img_h, img_w, img_h, popup_width-img_w, popup_height-img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// <---- down image
  	full_path = MKFULLPATH(down_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, left_width, popup_height-img_h, popup_width-left_width-right_width, img_h, left_width, popup_height-img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }
		
	down_height = img_h;

// <---- left image
  	full_path = MKFULLPATH(left_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, up_height, img_w, popup_height-up_height-down_height, 0, up_height, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// <---- right image
  	full_path = MKFULLPATH(right_img);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, popup_width-img_w, up_height, img_w, popup_height-up_height-down_height, popup_width-img_w, up_height, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// filling
	inner = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, popup_width-left_width-right_width, popup_height-up_height-down_height);
	gdk_pixbuf_fill(inner, COLOR2INT(bg_color));
	gdk_pixbuf_composite(inner, nbuf, left_width, up_height, popup_width-left_width-right_width, popup_height-up_height-down_height, left_width, up_height, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(inner);

// <---- line image
	if (line_img != NULL)
	{
	  	full_path = MKFULLPATH(line_img);
	 
	  	if (!full_path)
	  	{
	  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
			g_assert(0);
	  		return NULL;
	    }  		

	    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
		if(!pbImage)
		{		
			DMSG(1, "%s[%s] fail... ", __func__, full_path);
			g_assert(0);
			return NULL;
		}

		img_w = gdk_pixbuf_get_width(pbImage);
		img_h = gdk_pixbuf_get_height(pbImage);

		gdk_pixbuf_composite(pbImage, nbuf, 2, 42, popup_width-2-9, img_h, 2, 42, 1, 1, GDK_INTERP_BILINEAR, 255);
		g_object_unref(pbImage);

      	if(full_path != NULL)
      	{
    		g_free(full_path);
    		full_path = NULL;
        }		
	}

// <---- hash table insert
    _insert_image_hash_table(create_image, nbuf);

// <-------- end
	pbImage = NULL;

	return nbuf;
}

static void _create_backspace_button_image(const gchar *create_image, const gchar *bg_image, const gchar *backspace_image)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;
	GdkPixbuf *nbuf = NULL;

	gint bg_img_w, bg_img_h;
	gint bs_img_w, bs_img_h;

    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;
	
// <---- bg image
  	full_path = MKFULLPATH(bg_image);

  	if (!full_path)
	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
		return NULL;
	}

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
  		g_assert(0);
		return NULL;
	}

	bg_img_w = gdk_pixbuf_get_width(pbImage);
	bg_img_h = gdk_pixbuf_get_height(pbImage);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, bg_img_w, bg_img_h);
	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, bg_img_w, bg_img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);
  
  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

// <---- backspace image
  	full_path = MKFULLPATH(backspace_image);

  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);  		
		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);		
		return NULL;
	}

	bs_img_w = gdk_pixbuf_get_width(pbImage);
	bs_img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, (bg_img_w-bs_img_w)/2, (bg_img_h-bs_img_h)/2, bs_img_w, bs_img_h, 
						(bg_img_w-bs_img_w)/2, (bg_img_h-bs_img_h)/2, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

    _insert_image_hash_table(create_image, nbuf);

	pbImage = NULL;

	return nbuf;

}

static GdkPixbuf* _create_menu_inner_bg(const gchar *create_image, 
							const gchar *menu_image, const gchar *inner_image,
							gint inner_img_x, gint inner_img_y,
							gint cut_x, gint cut_y, gint cut_w, gint cut_h)
{
	GdkPixbuf *nbuf = NULL;
	GdkPixbuf *tmp_buf = NULL;
	GdkPixbuf *pbImage = NULL;
	gint img_w, img_h;

    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

// create_image 
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, cut_w, cut_h);
	gdk_pixbuf_fill (nbuf, 0x000000ff);

// menu image load
    pbImage = nfui_get_image_from_memory(menu_image);
	if(!pbImage)
  	{
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

// create_image 
	tmp_buf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, img_w, img_h);
	gdk_pixbuf_fill (tmp_buf, 0x000000ff);
	gdk_pixbuf_composite(pbImage, tmp_buf, 0, 0, img_w, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);	

// inner image load
    pbImage = nfui_get_image_from_memory(inner_image);
	if(!pbImage)
	{		
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);
	gdk_pixbuf_composite(pbImage, tmp_buf, inner_img_x, inner_img_y, img_w, img_h, inner_img_x, inner_img_y, 1, 1, GDK_INTERP_BILINEAR, 255);

	gdk_pixbuf_copy_area(tmp_buf, cut_x, cut_y, cut_w, cut_h, nbuf, 0, 0);
    g_object_unref(tmp_buf);

    _insert_image_hash_table(create_image, nbuf);

	return nbuf;
}

static gint _create_dynamic_btn_image()
{
	// TYPE1
	nftool_create_type1_btn_image(84);
	nftool_create_type1_btn_image(166);
	nftool_create_type1_btn_image(192);
	nftool_create_type1_btn_image(226);
	nftool_create_type1_btn_image(230);

	// TYPE2
	nftool_create_type2_btn_image(192);

	// TYPE3
	nftool_create_type3_btn_image(84);
	nftool_create_type3_btn_image(192);
	nftool_create_type3_btn_image(203);
	
	return 0;
}

static gint _create_combo_small_image(gchar *img_file, gchar *mk_img)
{
	GdkPixbuf *pbuf;
	GdkPixbuf *nbuf;
	
	gint img_w, img_h;

    nbuf = _lookup_image_hash_table(mk_img);
    if (nbuf != NULL) return -1;

	pbuf = nfui_get_image_from_file(img_file, NULL);

	img_w = gdk_pixbuf_get_width(pbuf);
	img_h = gdk_pixbuf_get_height(pbuf);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, img_w, 30);
	gdk_pixbuf_fill (nbuf, 0x000000ff);
	gdk_pixbuf_copy_area(pbuf, 0, 5, img_w, 30, nbuf, 0, 0);
	
    _insert_image_hash_table(mk_img, nbuf);	
    return 0;
}

static gint _create_slider_small_image(gchar *img_file, gchar *mk_img)
{
	GdkPixbuf *pbuf;
	GdkPixbuf *nbuf;
	
	gint img_w, img_h;

    nbuf = _lookup_image_hash_table(mk_img);
    if (nbuf != NULL) return -1;

	pbuf = nfui_get_image_from_file(img_file, NULL);

	img_w = gdk_pixbuf_get_width(pbuf);
	img_h = gdk_pixbuf_get_height(pbuf);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, img_w, 30);
	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_copy_area(pbuf, 0, 5, img_w, 30, nbuf, 0, 0);
	
    _insert_image_hash_table(mk_img, nbuf);	
    return 0;
}

static gint _create_spinup_small_image(gchar *img_file, gchar *mk_img)
{
	GdkPixbuf *spin;
	GdkPixbuf *nbuf;
	
	gint img_w, img_h;

    nbuf = _lookup_image_hash_table(mk_img);
    if (nbuf != NULL) return -1;

	spin = nfui_get_image_from_file(img_file, NULL);

	img_w = gdk_pixbuf_get_width(spin);
	img_h = gdk_pixbuf_get_height(spin);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, img_w, 15);
	gdk_pixbuf_fill (nbuf, 0x000000ff);
	gdk_pixbuf_copy_area(spin, 0, 5, img_w, 15, nbuf, 0, 0);
	
    _insert_image_hash_table(mk_img, nbuf);	
    return 0;
}

static gint _create_spindown_small_image(gchar *img_file, gchar *mk_img)
{
	GdkPixbuf *spin;
	GdkPixbuf *nbuf;
	
	gint img_w, img_h;

    nbuf = _lookup_image_hash_table(mk_img);
    if (nbuf != NULL) return -1;

	spin = nfui_get_image_from_file(img_file, NULL);

	img_w = gdk_pixbuf_get_width(spin);
	img_h = gdk_pixbuf_get_height(spin);

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, img_w, 15);
	gdk_pixbuf_fill (nbuf, 0x000000ff);
	gdk_pixbuf_copy_area(spin, 0, 0, img_w, 15, nbuf, 0, 0);
	
    _insert_image_hash_table(mk_img, nbuf);	
    return 0;
}

static gint _create_dynamic_small_image()
{
    _create_spinup_small_image(IMG_N_POPUP_SPIN_UP, MK_IMG_N_POPUP_SPIN_UP_SMALL);
    _create_spinup_small_image(IMG_O_POPUP_SPIN_UP, MK_IMG_O_POPUP_SPIN_UP_SMALL);
    _create_spinup_small_image(IMG_P_POPUP_SPIN_UP, MK_IMG_P_POPUP_SPIN_UP_SMALL);
    _create_spinup_small_image(IMG_D_POPUP_SPIN_UP, MK_IMG_D_POPUP_SPIN_UP_SMALL);

    _create_spindown_small_image(IMG_N_POPUP_SPIN_DOWN, MK_IMG_N_POPUP_SPIN_DOWN_SMALL);
    _create_spindown_small_image(IMG_O_POPUP_SPIN_DOWN, MK_IMG_O_POPUP_SPIN_DOWN_SMALL);
    _create_spindown_small_image(IMG_P_POPUP_SPIN_DOWN, MK_IMG_P_POPUP_SPIN_DOWN_SMALL);
    _create_spindown_small_image(IMG_D_POPUP_SPIN_DOWN, MK_IMG_D_POPUP_SPIN_DOWN_SMALL);
    
    _create_combo_small_image(IMG_N_POPUP_DROPDOWN_01, MK_IMG_N_POPUP_DROPDOWN_SMALL_01);
    _create_combo_small_image(IMG_O_POPUP_DROPDOWN_01, MK_IMG_O_POPUP_DROPDOWN_SMALL_01);
    _create_combo_small_image(IMG_P_POPUP_DROPDOWN_01, MK_IMG_P_POPUP_DROPDOWN_SMALL_01);
    _create_combo_small_image(IMG_D_POPUP_DROPDOWN_01, MK_IMG_D_POPUP_DROPDOWN_SMALL_01);    

    _create_combo_small_image(IMG_N_POPUP_DROPDOWN_02, MK_IMG_N_POPUP_DROPDOWN_SMALL_02);
    _create_combo_small_image(IMG_O_POPUP_DROPDOWN_02, MK_IMG_O_POPUP_DROPDOWN_SMALL_02);
    _create_combo_small_image(IMG_P_POPUP_DROPDOWN_02, MK_IMG_P_POPUP_DROPDOWN_SMALL_02);
    _create_combo_small_image(IMG_D_POPUP_DROPDOWN_02, MK_IMG_D_POPUP_DROPDOWN_SMALL_02);    
    
    _create_slider_small_image(IMG_SLIDEBAR_N, MK_IMG_SLIDEBAR_SMALL_N);
    _create_slider_small_image(IMG_SLIDEBAR_O, MK_IMG_SLIDEBAR_SMALL_O);
    _create_slider_small_image(IMG_SLIDEBAR_S, MK_IMG_SLIDEBAR_SMALL_S);
    _create_slider_small_image(IMG_SLIDEBAR_D, MK_IMG_SLIDEBAR_SMALL_D);    

    _create_slider_small_image(IMG_SLIDE_BG_N_L, MK_IMG_SLIDE_SMALL_BG_N_L);    
    _create_slider_small_image(IMG_SLIDE_BG_N_M, MK_IMG_SLIDE_SMALL_BG_N_M);    
    _create_slider_small_image(IMG_SLIDE_BG_N_R, MK_IMG_SLIDE_SMALL_BG_N_R);    

    _create_slider_small_image(IMG_SLIDE_BG_D_L, MK_IMG_SLIDE_SMALL_BG_D_L);    
    _create_slider_small_image(IMG_SLIDE_BG_D_M, MK_IMG_SLIDE_SMALL_BG_D_M);    
    _create_slider_small_image(IMG_SLIDE_BG_D_R, MK_IMG_SLIDE_SMALL_BG_D_R);        
    
    return 0;
}

void nfui_preload_image(void)
{
	// SEARCH BY TIME
	nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

	nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
	nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);
	
	nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);	
	nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);	
	nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);	

	nfui_get_image_from_file(HRZ_CURTAIN, NULL);
	nfui_get_image_from_file(HRZ_CURTAIN2, NULL);

	nfui_get_image_from_file((IMG_CHECK_OFF_N), NULL);
	nfui_get_image_from_file((IMG_CHECK_ON_N), NULL);	
	nfui_get_image_from_file((IMG_CHECK_OFF_O), NULL);	
	nfui_get_image_from_file((IMG_CHECK_ON_O), NULL);	
	nfui_get_image_from_file((IMG_CHECK_OFF_D), NULL);	
	nfui_get_image_from_file((IMG_CHECK_ON_D), NULL);	

	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_N_BTN), NULL);
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_O_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_P_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_D_BTN), NULL);	

	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_N_BTN), NULL);
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_O_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_P_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_D_BTN), NULL);	

	nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);	

	nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);	
	nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);	


	// THUMBNAIL SEARCH
	nfui_get_image_from_file((IMG_N_DROPDOWN_01), NULL);
	nfui_get_image_from_file((IMG_O_DROPDOWN_01), NULL);
	nfui_get_image_from_file((IMG_P_DROPDOWN_01), NULL);
	nfui_get_image_from_file((IMG_D_DROPDOWN_01), NULL);

	// ARCHIVING
	nfui_get_image_from_file((IMG_BTN_N_DELETE), NULL);
	nfui_get_image_from_file((IMG_BTN_O_DELETE), NULL);
	nfui_get_image_from_file((IMG_BTN_P_DELETE), NULL);
	nfui_get_image_from_file((IMG_BTN_D_DELETE), NULL);


	// SYSTEM MAIN SETUP
	nfui_get_image_from_file(IMG_SUBMENU_FOCUS_BAR, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_NOFOCUS_BAR, NULL);

	nfui_get_image_from_file(IMG_MAINMENU_BG, NULL);

	nfui_get_image_from_file(IMG_SUBMENU_N_CAMERA, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_DISPLAY, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_SOUND	, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_USER, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_NETWORK, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_SYSTEM, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_STORAGE, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_N_EVENT, NULL);

	nfui_get_image_from_file(IMG_SUBMENU_O_CAMERA, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_DISPLAY, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_SOUND, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_USER, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_NETWORK, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_SYSTEM, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_STORAGE, NULL);
	nfui_get_image_from_file(IMG_SUBMENU_O_EVENT, NULL);
}

void nfui_create_image(void)
{
	_create_image_main_method(MK_IMG_MENU_BG, 
			MENU_BG_TOP, MENU_BG_BOTTOM, 
			MENU_BG_LEFT, MENU_BG_RIGHT,
			SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT, UX_COLOR(0));		

	_create_image_main_method(MK_IMG_TEMP_MENU_BG, 
			MENU_BG_TOP, MENU_BG_BOTTOM, 
			MENU_BG_LEFT, MENU_BG_RIGHT,
			SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT, UX_COLOR(1));		
		
	_create_image_popup_method(MK_IMG_TEMP_MENU_V_INNER, MENU_V_INNER_X+MENU_V_INNER_W+20, MENU_V_INNER_Y+MENU_V_INNER_H+20,
			MENU_INNER_BG_1, MENU_INNER_BG_2, MENU_INNER_BG_3,
			MENU_INNER_BG_7, MENU_INNER_BG_8, MENU_INNER_BG_9,
			MENU_INNER_BG_4, MENU_INNER_BG_6,  NULL, UX_COLOR(0));	

	_create_image_popup_method(MK_IMG_TEMP_MENU_H_INNER, MENU_H_INNER_X+MENU_H_INNER_W+20, MENU_H_INNER_Y+MENU_H_INNER_H+20, 
			MENU_INNER_BG_1, MENU_INNER_BG_2, MENU_INNER_BG_3,
			MENU_INNER_BG_7, MENU_INNER_BG_8, MENU_INNER_BG_9,
			MENU_INNER_BG_4, MENU_INNER_BG_6, NULL, UX_COLOR(0));	

	_create_image_popup_method(MK_IMG_TEMP_MENU_V_SUBTAB_INNER, MENU_V_SUBTAB_INNER_X+MENU_V_SUBTAB_INNER_W+18, MENU_V_SUBTAB_INNER_Y+MENU_V_SUBTAB_INNER_H+18, 
			MENU_SUBTAB_INNER_BG_1, MENU_SUBTAB_INNER_BG_2, MENU_SUBTAB_INNER_BG_3,
			MENU_SUBTAB_INNER_BG_7, MENU_SUBTAB_INNER_BG_8, MENU_SUBTAB_INNER_BG_9,
			MENU_SUBTAB_INNER_BG_4, MENU_SUBTAB_INNER_BG_6, NULL, UX_COLOR(186));	
						
	_create_image_popup_method(MK_IMG_TEMP_MENU_V_IPCAMSET_SUBTAB_INNER, MENU_V_IPCAMSET_SUBTAB_INNER_X+MENU_V_IPCAMSET_SUBTAB_INNER_W+18, MENU_V_IPCAMSET_SUBTAB_INNER_Y+MENU_V_IPCAMSET_SUBTAB_INNER_H+18, 
			MENU_SUBTAB_INNER_BG_1, MENU_SUBTAB_INNER_BG_2, MENU_SUBTAB_INNER_BG_3,
			MENU_SUBTAB_INNER_BG_7, MENU_SUBTAB_INNER_BG_8, MENU_SUBTAB_INNER_BG_9,
			MENU_SUBTAB_INNER_BG_4, MENU_SUBTAB_INNER_BG_6, NULL, UX_COLOR(186));	
												
	_create_menu_inner_bg(MK_IMG_MENU_V_PAGE_BG, 
			MK_IMG_MENU_BG, MK_IMG_TEMP_MENU_V_INNER,
			MENU_V_PAGE_X, MENU_V_PAGE_Y,
			MENU_V_PAGE_X, MENU_V_PAGE_Y, MENU_V_PAGE_W, MENU_V_PAGE_H);

	_create_menu_inner_bg(MK_IMG_MENU_H_PAGE_BG, 
			MK_IMG_MENU_BG, MK_IMG_TEMP_MENU_H_INNER,
			MENU_H_PAGE_X, MENU_H_PAGE_Y,
			MENU_H_PAGE_X, MENU_H_PAGE_Y, MENU_H_PAGE_W, MENU_H_PAGE_H);

	_create_menu_inner_bg(MK_IMG_MENU_V_SUBTAB_FIXED_BG,
			MK_IMG_TEMP_MENU_BG, MK_IMG_TEMP_MENU_V_SUBTAB_INNER,
			MENU_V_SUBTAB_FIXED_X+MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_SUBTAB_PAGE_Y, 
			MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y, MENU_V_SUBTAB_FIXED_W, MENU_V_SUBTAB_FIXED_H);
			
	_create_menu_inner_bg(MK_IMG_MENU_V_SUBTAB_PAGE_BG,
			MK_IMG_TEMP_MENU_BG, MK_IMG_TEMP_MENU_V_SUBTAB_INNER,
			MENU_V_SUBTAB_FIXED_X+MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_SUBTAB_PAGE_Y, 
			MENU_V_SUBTAB_FIXED_X+MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_SUBTAB_PAGE_Y, MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);

	_create_menu_inner_bg(MK_IMG_MENU_V_IPCAMSET_SUBTAB_FIXED_BG,
			MK_IMG_TEMP_MENU_BG, MK_IMG_TEMP_MENU_V_IPCAMSET_SUBTAB_INNER,
			MENU_V_SUBTAB_FIXED_X+MENU_V_IPCAMSET_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_IPCAMSET_SUBTAB_PAGE_Y, 
			MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y, MENU_V_SUBTAB_FIXED_W, MENU_V_SUBTAB_FIXED_H);
			
	_create_menu_inner_bg(MK_IMG_MENU_V_IPCAMSET_SUBTAB_PAGE_BG,
			MK_IMG_TEMP_MENU_BG, MK_IMG_TEMP_MENU_V_IPCAMSET_SUBTAB_INNER,
			MENU_V_SUBTAB_FIXED_X+MENU_V_IPCAMSET_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_IPCAMSET_SUBTAB_PAGE_Y, 
			MENU_V_SUBTAB_FIXED_X+MENU_V_IPCAMSET_SUBTAB_PAGE_X, MENU_V_SUBTAB_FIXED_Y+MENU_V_IPCAMSET_SUBTAB_PAGE_Y, MENU_V_IPCAMSET_SUBTAB_PAGE_W, MENU_V_IPCAMSET_SUBTAB_PAGE_H);

    _remove_image_hash_table(MK_IMG_TEMP_MENU_BG);
    _remove_image_hash_table(MK_IMG_TEMP_MENU_V_INNER);
    _remove_image_hash_table(MK_IMG_TEMP_MENU_H_INNER);
    _remove_image_hash_table(MK_IMG_TEMP_MENU_V_SUBTAB_INNER);    
    _remove_image_hash_table(MK_IMG_TEMP_MENU_V_IPCAMSET_SUBTAB_INNER);    

	_create_image_popup_method(MK_IMG_POPUP_BG, 1920/2+20, 1080/2+20,
			POPUP_MENU_BG1, POPUP_MENU_BG2, POPUP_MENU_BG3,
			POPUP_MENU_BG7, POPUP_MENU_BG8, POPUP_MENU_BG9,
			POPUP_MENU_BG4, POPUP_MENU_BG6, POPUP_MENU_LINE, UX_COLOR(200));

	_create_image_popup_method(MK_IMG_POPUP_BG_NO_LINE, 1920/2+20, 1080/2+20,
			POPUP_MENU_BG1, POPUP_MENU_BG2, POPUP_MENU_BG3,
			POPUP_MENU_BG7, POPUP_MENU_BG8, POPUP_MENU_BG9,
			POPUP_MENU_BG4, POPUP_MENU_BG6, NULL, UX_COLOR(200));

	_create_image_popup_method(MK_IMG_POPUP_TAB_BG, 1920/2+10, 1080/2+10,
			MAIN_MENU_POPUP_TAP_BG1, MAIN_MENU_POPUP_TAP_BG2, MAIN_MENU_POPUP_TAP_BG3,
			MAIN_MENU_POPUP_TAP_BG7, MAIN_MENU_POPUP_TAP_BG8, MAIN_MENU_POPUP_TAP_BG9,
			MAIN_MENU_POPUP_TAP_BG4, MAIN_MENU_POPUP_TAP_BG6, NULL, UX_COLOR(291));

	_create_image_popup_method(MK_IMG_SUB_GROUP_BG, 1920/2+10, 1080/2+10,
			SUB_GROUP_BG1, SUB_GROUP_BG2, SUB_GROUP_BG3,
			SUB_GROUP_BG7, SUB_GROUP_BG8, SUB_GROUP_BG9,
			SUB_GROUP_BG4, SUB_GROUP_BG6, NULL, UX_COLOR(194));

	_create_image_popup_method(MK_IMG_TAB_SUB_GROUP_BG, 1920/2+10, 1080/2+10,
			TAB_SUB_GROUP_BG1, TAB_SUB_GROUP_BG2, TAB_SUB_GROUP_BG3,
			TAB_SUB_GROUP_BG7, TAB_SUB_GROUP_BG8, TAB_SUB_GROUP_BG9,
			TAB_SUB_GROUP_BG4, TAB_SUB_GROUP_BG6, NULL, UX_COLOR(980));
	
	_create_image_popup_method(MK_IMG_TAB_SUB_GROUP02_BG, 1920/2+10, 1080/2+10,
			TAB_SUB_GROUP02_BG1, TAB_SUB_GROUP02_BG2, TAB_SUB_GROUP02_BG3,
			TAB_SUB_GROUP02_BG7, TAB_SUB_GROUP02_BG8, TAB_SUB_GROUP02_BG9,
			TAB_SUB_GROUP02_BG4, TAB_SUB_GROUP02_BG6, NULL, UX_COLOR(186));


	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_N_260, 260, IMG_TAP_N_L, IMG_TAP_N_M, IMG_TAP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_O_260, 260, IMG_TAP_O_L, IMG_TAP_O_M, IMG_TAP_O_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_S_260, 260, IMG_TAP_S_L, IMG_TAP_S_M, IMG_TAP_S_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_D_260, 260, IMG_TAP_D_L, IMG_TAP_D_M, IMG_TAP_D_R);

	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_N_340, 340, IMG_TAP_N_L, IMG_TAP_N_M, IMG_TAP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_O_340, 340, IMG_TAP_O_L, IMG_TAP_O_M, IMG_TAP_O_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_S_340, 340, IMG_TAP_S_L, IMG_TAP_S_M, IMG_TAP_S_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_DIR_H_D_340, 340, IMG_TAP_D_L, IMG_TAP_D_M, IMG_TAP_D_R);

	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_300, 300, IMG_SUBTAP_N_L, IMG_SUBTAP_N_M, IMG_SUBTAP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_300, 300, IMG_SUBTAP_S_L, IMG_SUBTAP_S_M, IMG_SUBTAP_S_R);

	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_250, 250, IMG_SUBTAP_N_L, IMG_SUBTAP_N_M, IMG_SUBTAP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_250, 250, IMG_SUBTAP_S_L, IMG_SUBTAP_S_M, IMG_SUBTAP_S_R);

	// TEMPORARY USING
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_116, 116, IMG_SUBTAB_N_L, IMG_SUBTAB_N_M, IMG_SUBTAB_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_116, 116, IMG_SUBTAB_S_L, IMG_SUBTAB_S_M, IMG_SUBTAB_S_R);

	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_131, 131, IMG_SUBTAB_N_L, IMG_SUBTAB_N_M, IMG_SUBTAB_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_131, 131, IMG_SUBTAB_S_L, IMG_SUBTAB_S_M, IMG_SUBTAB_S_R);

	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_160, 160, IMG_SUBTAB_N_L, IMG_SUBTAB_N_M, IMG_SUBTAB_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_160, 160, IMG_SUBTAB_S_L, IMG_SUBTAB_S_M, IMG_SUBTAB_S_R);

	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_N_235, 235, IMG_SUBTAB_N_L, IMG_SUBTAB_N_M, IMG_SUBTAB_N_R);
	nf_ui_create_image_button_method(MKB_IMG_SUBTAB_DIR_H_S_235, 235, IMG_SUBTAB_S_L, IMG_SUBTAB_S_M, IMG_SUBTAB_S_R);

	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_H_N_274, 274, IMG_TAP_POP_N_L, IMG_TAP_POP_N_M, IMG_TAP_POP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_H_S_274, 274, IMG_TAP_POP_S_L, IMG_TAP_POP_S_M, IMG_TAP_POP_S_R);

	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_H_N_208, 208, IMG_TAP_POP_N_L, IMG_TAP_POP_N_M, IMG_TAP_POP_N_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_H_S_208, 208, IMG_TAP_POP_S_L, IMG_TAP_POP_S_M, IMG_TAP_POP_S_R);

	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_V_N_208, 208, IMG_TAP_POP_V_N_L, IMG_TAP_POP_V_N_M, IMG_TAP_POP_V_N_R);
	nf_ui_create_image_button_method(MKB_IMG_TAB_POP_DIR_V_S_208, 208, IMG_TAP_POP_V_S_L, IMG_TAP_POP_V_S_M, IMG_TAP_POP_V_S_R);

    _create_backspace_button_image(MK_IMG_N_KEY_BACKSPACE, IMG_N_KEY_02, IMG_N_KEY_BACK);
	_create_backspace_button_image(MK_IMG_O_KEY_BACKSPACE, IMG_O_KEY_02, IMG_O_KEY_BACK);
	_create_backspace_button_image(MK_IMG_P_KEY_BACKSPACE, IMG_P_KEY_02, IMG_P_KEY_BACK);
	_create_backspace_button_image(MK_IMG_D_KEY_BACKSPACE, IMG_D_KEY_02, IMG_D_KEY_BACK);

	_create_backspace_button_image(MK_IMG_N_KEY03_BACKSPACE, IMG_N_KEY_03, IMG_N_KEY_BACK);
	_create_backspace_button_image(MK_IMG_O_KEY03_BACKSPACE, IMG_O_KEY_03, IMG_O_KEY_BACK);
	_create_backspace_button_image(MK_IMG_P_KEY03_BACKSPACE, IMG_P_KEY_03, IMG_P_KEY_BACK);
	_create_backspace_button_image(MK_IMG_D_KEY03_BACKSPACE, IMG_D_KEY_03, IMG_D_KEY_BACK);

	_create_backspace_button_image(MK_IMG_N_NUM_KEY_BACKSPACE, IMG_N_NUM_KEY_03, IMG_N_KEY_BACK);
	_create_backspace_button_image(MK_IMG_O_NUM_KEY_BACKSPACE, IMG_O_NUM_KEY_03, IMG_O_KEY_BACK);
	_create_backspace_button_image(MK_IMG_P_NUM_KEY_BACKSPACE, IMG_P_NUM_KEY_03, IMG_P_KEY_BACK);
	_create_backspace_button_image(MK_IMG_D_NUM_KEY_BACKSPACE, IMG_D_NUM_KEY_03, IMG_D_KEY_BACK);

	_create_dynamic_btn_image();
	_create_dynamic_small_image();
}

GdkPixbuf* nf_ui_create_image_button_method(const gchar *create_image, guint btn_width, const gchar *left_image, const gchar *middle_image, const gchar *right_image)
{
	GdkPixbuf *pbImage = NULL;
	GdkPixbuf *nbuf = NULL;

	gint img_w, img_h;

	gint pos_x, left_width, right_width;
	gint btn_height;

// <-------- check image 
    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

// <---- left image
    pbImage = nfui_get_image_from_file(left_image, 0);

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	btn_height = img_h;
	
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, btn_width, btn_height);
	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);


	left_width = img_w;

// <---- right image
    pbImage = nfui_get_image_from_file(right_image, 0);
 
	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, btn_width-img_w, 0, img_w, img_h, btn_width-img_w, 0, 1, 1, GDK_INTERP_BILINEAR, 255);

	right_width = img_w;

// <---- copy center image
    pbImage = nfui_get_image_from_file(middle_image, 0);
   
	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	for (pos_x = left_width; pos_x < btn_width-right_width; pos_x++) {
		gdk_pixbuf_composite(pbImage, nbuf, pos_x, 0, img_w, img_h, pos_x, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	}

    _insert_image_hash_table(create_image, nbuf);

// <-------- end
	pbImage = NULL;

	return nbuf;
}

GdkPixbuf* nf_ui_create_image_button_method2(const gchar *create_image, guint btn_width, guint btn_height, const gchar *left_image, const gchar *middle_image, const gchar *right_image)
{
	GdkPixbuf *pbImage = NULL;
	GdkPixbuf *nbuf = NULL;

	gint img_w, img_h;

	gint pos_x, left_width, right_width;

// <-------- check image 
    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

// <---- left image
    pbImage = nfui_get_image_from_file(left_image, 0);    
    img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);
	
    pbImage = gdk_pixbuf_scale_simple(pbImage, img_w, btn_height, GDK_INTERP_BILINEAR);

	//btn_height = img_h;
	
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, btn_width, btn_height);
	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, btn_height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);


	left_width = img_w;

// <---- right image
    pbImage = nfui_get_image_from_file(right_image, 0);
 
	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);
    pbImage = gdk_pixbuf_scale_simple(pbImage, img_w, btn_height, GDK_INTERP_BILINEAR);
    
	gdk_pixbuf_composite(pbImage, nbuf, btn_width-img_w, 0, img_w, btn_height, btn_width-img_w, 0, 1, 1, GDK_INTERP_BILINEAR, 255);

	right_width = img_w;

// <---- copy center image
    pbImage = nfui_get_image_from_file(middle_image, 0);
   
	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);
	pbImage = gdk_pixbuf_scale_simple(pbImage, img_w, btn_height, GDK_INTERP_BILINEAR);

	for (pos_x = left_width; pos_x < btn_width-right_width; pos_x++) {
		gdk_pixbuf_composite(pbImage, nbuf, pos_x, 0, img_w, btn_height, pos_x, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	}

    _insert_image_hash_table(create_image, nbuf);

// <-------- end
	pbImage = NULL;

	return nbuf;
}

GdkPixbuf* nf_ui_create_image_button_no_alpha(const gchar *create_image, guint btn_width, const gchar *left_image, const gchar *middle_image, const gchar *right_image)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;
	GdkPixbuf *nbuf = NULL;

	gint img_w, img_h;

	gint pos_x, left_width, right_width;
	gint btn_height;

// <-------- check image 
    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

// <---- left image
  	full_path = MKFULLPATH(left_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	btn_height = img_h;

	
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, btn_width, btn_height);
//	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);


  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	left_width = img_w;

// <---- right image
  	full_path = MKFULLPATH(right_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);  		
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);		
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, btn_width-img_w, 0, img_w, img_h, btn_width-img_w, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	right_width = img_w;

// <---- copy center image
  	full_path = MKFULLPATH(middle_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
  		g_assert(0);
		return NULL;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	for (pos_x = left_width; pos_x < btn_width-right_width; pos_x++) {
		gdk_pixbuf_composite(pbImage, nbuf, pos_x, 0, img_w, img_h, pos_x, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	}
	g_object_unref(pbImage);

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

    _insert_image_hash_table(create_image, nbuf);

// <-------- end
	pbImage = NULL;

	return nbuf;
}

GdkPixbuf* nf_ui_create_main_submenu_image(const gchar *create_image, const gchar *top_image, const gchar *bottom_image)
{
	GdkPixbuf *pbImage_top = NULL;
	GdkPixbuf *pbImage_bottom = NULL;
	gchar *full_path = NULL;
	GdkPixbuf *nbuf = NULL;

	gint top_img_w, top_img_h;
	gint bottom_img_w, bottom_img_h;

// <-------- check image 
    nbuf = _lookup_image_hash_table(create_image);

    if (nbuf != NULL) return nbuf;

// top image load
  	full_path = MKFULLPATH(top_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage_top = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage_top)
	{		
  		g_assert(0);
		return NULL;
	}

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	top_img_w = gdk_pixbuf_get_width(pbImage_top);
	top_img_h = gdk_pixbuf_get_height(pbImage_top);

// bottom image load
  	full_path = MKFULLPATH(bottom_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
  		g_assert(0);
  		return NULL;
    }  		

    pbImage_bottom = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage_bottom)
	{		
  		g_assert(0);
		return NULL;
	}

  	if(full_path != NULL)
  	{
		g_free(full_path);
		full_path = NULL;
    }

	bottom_img_w = gdk_pixbuf_get_width(pbImage_bottom);
	bottom_img_h = gdk_pixbuf_get_height(pbImage_bottom);

// make image	
	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, top_img_w, top_img_h+bottom_img_h);
	gdk_pixbuf_fill (nbuf, 0x00000000);
	gdk_pixbuf_composite(pbImage_top, nbuf, 0, 0, top_img_w, top_img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbImage_bottom, nbuf, 0, top_img_h, bottom_img_w, bottom_img_h, 0, top_img_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	
	g_object_unref(pbImage_top);
	g_object_unref(pbImage_bottom);

    _insert_image_hash_table(create_image, nbuf);

	pbImage_top = NULL;
	pbImage_bottom = NULL;

	return nbuf;
}

GdkPixbuf* nf_ui_create_image_remote_msg_popup(const gchar *org_image, gchar *create_image, gint dest_h)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;

    GdkPixbuf *nbuf = NULL;

	gint img_w, img_h;
    gint i;

    nbuf = _lookup_image_hash_table(create_image);
    if (nbuf != NULL) return nbuf;

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 534, dest_h);
	gdk_pixbuf_fill (nbuf, 0x00000000);

	if(!nbuf)
	{
		g_assert(0);
		return 0;
	}

  	full_path = MKFULLPATH(org_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
		return 0;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return 0;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, img_w, img_h/2, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbImage, nbuf, 0, dest_h-(img_h/2), img_w, img_h/2, 0, (dest_h-img_h), 1, 1, GDK_INTERP_BILINEAR, 255);

    if (dest_h > img_h)
    {
        for (i = 0; i < (dest_h-img_h)/10+1; i++)
        {
            gdk_pixbuf_composite(pbImage, nbuf, 0, img_h/2+10*i, img_w, 10, 0, 10*i, 1, 1, GDK_INTERP_BILINEAR, 255);
        }
    }

	_insert_image_hash_table(create_image, nbuf);
	g_object_unref(pbImage);    

    return nbuf;
}

GdkPixbuf* nf_ui_create_image_progress_bg(const gchar *org_image, gchar *create_image, gint dest_w)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;

    GdkPixbuf *nbuf = NULL;

	gint img_w, img_h;
    gint i;

	gchar image_name[64];

	memset(image_name, 0x00, sizeof(image_name));
	g_sprintf(image_name, "%s_%d", create_image, dest_w);

    nbuf = _lookup_image_hash_table(image_name);
    if (nbuf != NULL) return nbuf;

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, dest_w, 22);
	gdk_pixbuf_fill (nbuf, COLOR2INT(UX_COLOR(200)));

	if(!nbuf)
	{
		g_assert(0);
		return 0;
	}

  	full_path = MKFULLPATH(org_image);
 
  	if (!full_path)
  	{
  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
		g_assert(0);
		return 0;
    }  		

    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	if(!pbImage)
	{		
		DMSG(1, "%s[%s] fail... ", __func__, full_path);
		g_assert(0);
		return 0;
	}

	img_w = gdk_pixbuf_get_width(pbImage);
	img_h = gdk_pixbuf_get_height(pbImage);

	gdk_pixbuf_composite(pbImage, nbuf, 0, 0, 40, img_h, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(pbImage, nbuf, dest_w-80/2, 0, 40, img_h, (dest_w-img_w), 0, 1, 1, GDK_INTERP_BILINEAR, 255);

    if (dest_w > 80)
    {
        for (i = 0; i < (dest_w-80)/10+1; i++)
        {
            gdk_pixbuf_composite(pbImage, nbuf, 40+10*i, 0, 10, img_h, 10*i, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
        }
    }

	_insert_image_hash_table(image_name, nbuf);
	g_object_unref(pbImage);    

    return nbuf;
}

GdkPixbuf* nfui_get_popup_pixbuf_composite(const gchar *org_image, gint dest_w, gint dest_h)
{
	GdkPixbuf *obuf = NULL;
	GdkPixbuf *nbuf = NULL;
    gchar dest_name[64];

    gint org_w, org_h;
    gint half_w, half_h;

    memset(dest_name, 0x00, sizeof(dest_name));
    g_sprintf(dest_name, "%s_%d_%d", org_image, dest_w, dest_h);
   
    nbuf = _lookup_image_hash_table(dest_name);
	if (nbuf) return nbuf;
  
    obuf = _lookup_image_hash_table(org_image);
	if (obuf == NULL) g_assert(0);		

	org_w = gdk_pixbuf_get_width(obuf);
	org_h = gdk_pixbuf_get_height(obuf);

    half_w = dest_w/2 + dest_w%2;
    half_h = dest_h/2 + dest_h%2;

	nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, dest_w, dest_h);
	gdk_pixbuf_fill(nbuf, 0x000000ff);

	gdk_pixbuf_composite(obuf, nbuf, 0, 0, dest_w/2, dest_h/2, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(obuf, nbuf, dest_w/2, 0, half_w, dest_h/2, dest_w/2+half_w-org_w, 0, 1, 1, GDK_INTERP_BILINEAR, 255);	
	gdk_pixbuf_composite(obuf, nbuf, 0, dest_h/2, dest_w/2, half_h, 0, dest_h/2+half_h-org_h, 1, 1, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(obuf, nbuf, dest_w/2, dest_h/2, half_w, half_h, dest_w/2+half_w-org_w, dest_h/2+half_h-org_h, 1, 1, GDK_INTERP_BILINEAR, 255);

    _insert_image_hash_table(dest_name, nbuf);

	return nbuf;
}

gint nfui_unref_popup_pixbuf(const gchar *org_image, gint dest_w, gint dest_h)
{
    gchar dest_name[64];
	GdkPixbuf *pbuf = NULL;

    memset(dest_name, 0x00, sizeof(dest_name));
    g_sprintf(dest_name, "%s_%d_%d", org_image, dest_w, dest_h);

    pbuf = _lookup_image_hash_table(dest_name);
	if (!pbuf) return -1;
    
    _remove_image_hash_table(dest_name);

    return 0;
}

GdkPixbuf *nfui_get_pixbuf_from_file(const gchar *path, GError *err)
{
	GdkPixbuf *ret_img = NULL;
	gchar *full_path = NULL;

	full_path = MKFULLPATH(path);

	if (!full_path) 
	{
		DMSG(1, "%s %d:pix buffer is NULL", __FILE__, __LINE__);
		g_assert(0);
		return NULL;
	}

	ret_img = gdk_pixbuf_new_from_file(full_path, NULL);

	if (full_path)  g_free(full_path);
	
	full_path = NULL;

	return ret_img;
}

GdkPixbuf* nfui_get_image_from_file(const gchar *path, GError *err)
{
	GdkPixbuf *pbImage = NULL;
	gchar *full_path = NULL;

    pbImage = _lookup_image_hash_table(path);

	if (pbImage == NULL)
	{
		gint img_w, img_h;

	  	full_path = MKFULLPATH(path);
	  	
	  	if (!full_path)
	  	{
	  		DMSG(1, "%s %d : Wrong Image Path...", __FILE__, __LINE__);
			g_assert(0);
	  		return NULL;
	    }  		
	
	    pbImage = gdk_pixbuf_new_from_file(full_path, NULL);
	    
		if (!pbImage)
		{
			DMSG(1, "%s\n", path);
			DMSG(1, "%s[%s] the file could not be opened ", __func__, full_path);

			if (full_path != NULL)
			{
				g_free(full_path);
				full_path = NULL;
			}
			return NULL;
		}

        _insert_image_hash_table(path, pbImage);

	  	if (full_path != NULL)
	  	{
			g_free(full_path);
			full_path = NULL;
	    }
	}

	return pbImage;
}

GdkPixbuf* nfui_get_image_from_memory(const gchar *image)
{
	GdkPixbuf *hash_pbuf = NULL;

    hash_pbuf = _lookup_image_hash_table(image);

	if (hash_pbuf == NULL) g_assert(0);		
 
	return hash_pbuf;
}

GdkPixbuf *nfui_get_pixbuf_from_drawable(GdkDrawable *drawable, GdkRectangle *rect, GError *err)
{
	GdkPixbuf *ret_img = NULL;

	ret_img = gdk_pixbuf_get_from_drawable(0, drawable, 0, rect->x, rect->y, 0, 0, rect->width, rect->height);
	return ret_img;
}

void nfui_get_image_size(const gchar *path, gint* width, gint* height)
{
	gchar *full_path = NULL;
	GdkPixbuf *hash_pbuf = NULL;

    hash_pbuf = _lookup_image_hash_table(path);

	if (hash_pbuf == NULL)
		hash_pbuf = nfui_get_image_from_file(path, NULL);

	*width = gdk_pixbuf_get_width(hash_pbuf);
	*height = gdk_pixbuf_get_height(hash_pbuf);
}

void nfui_get_pixbuf_size(GdkPixbuf* pbuf, gint* width, gint* height)
{
	*width = gdk_pixbuf_get_width(pbuf);
	*height = gdk_pixbuf_get_height(pbuf);
}

void nfui_get_pixmap_size(GdkPixmap *pmap, gint *width, gint *height)
{
	if (!(GDK_IS_DRAWABLE(pmap))) {
		*width = 0;
		*height = 0;
		return;
	}

	gdk_drawable_get_size(pmap, width, height);
}

gboolean watch_hash_table_size(gpointer data)
{
	DMSG(1, "TOTAL=[%.3f KB]", use_mem/1024);

    return TRUE;
}

