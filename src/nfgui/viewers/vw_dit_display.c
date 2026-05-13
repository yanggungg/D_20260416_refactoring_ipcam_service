/*
 * vw_dit_display.c
 *  - dependency :
 *      vw
 *      dit
 *
 * Written by Jungkyu <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */


#include <math.h>

#include "nf_afx.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"

#include "ix_mem.h"
#include "dit.h"
#include "vw_dit_vca.h"
#include "vw_dit_dva.h"


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _DIT_DP {
	GdkDrawable 	*drawable;
	GdkGC 			*gc;
	GdkRectangle	plt_area;
    gint            degree;
} DIT_DP;




////////////////////////////////////////////////////////////
//
// private variable
//

static DITID g_vca_ditid = 0;
static VCA_VMAP g_vca_vmap = {0, };

static DITID g_dva_ditid = 0;
static DVA_VMAP g_dva_vmap = {0, };



////////////////////////////////////////////////////////////
//
// private functions
//

static gint _trans_x_rpoint(gint rcnvs, gint vpoint, gint vcnvs)
{
    gint rpoint;    
    
    rpoint = (gint)((float)(vpoint*rcnvs/vcnvs));
    if (g_vca_vmap.use) rpoint = (rpoint-g_vca_vmap.x)*rcnvs/g_vca_vmap.w;
    
    return rpoint;
}

static gint _trans_y_rpoint(gint rcnvs, gint vpoint, gint vcnvs)
{
    gint rpoint;    

    rpoint = (gint)((float)(vpoint*rcnvs/vcnvs));
    if (g_vca_vmap.use) rpoint = (rpoint-g_vca_vmap.y)*rcnvs/g_vca_vmap.h;
    
    return rpoint;
}

static gint _trans_w_size(gint rcnvs, gint vsize, gint vcnvs)
{
    gint rsize;    

    rsize = (gint)((float)(vsize*rcnvs/vcnvs));
    if (g_vca_vmap.use) rsize = rsize*rcnvs/g_vca_vmap.w;

    return rsize;
}

static gint _trans_h_size(gint rcnvs, gint vsize, gint vcnvs)
{
    gint rsize;    

    rsize = (gint)((float)(vsize*rcnvs/vcnvs));
    if (g_vca_vmap.use) rsize = rsize*rcnvs/g_vca_vmap.h;

    return rsize;
}

static GdkPoint rotate_point(int cnvs_w, int cnvs_h, int x, int y, gint degree)
{
    float rad = degree*3.1415926/180;
    float s = sin(rad);
    float c = cos(rad);
    GdkPoint npt;

    if (degree == 0) {
        npt.x = x;
        npt.y = y;
    }
    else {
        npt.x = cnvs_w/2 + (x-cnvs_h/2) * c - (y-cnvs_w/2) * s;
        npt.y = cnvs_h/2 + (x-cnvs_h/2) * s + (y-cnvs_w/2) * c;
    }
    return npt;
}

static gint _get_draw_point(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, IGPOINT *src_pt, gint pt_cnt, GdkPoint *dst_pt, gint nr)
{
    gint i;
    gint dst_cnvs_w = cnvs_w, dst_cnvs_h = cnvs_h;
    gint degree = 0;

    if ((nr == 0) && (pdp->degree != 0)) {
        dst_cnvs_w = cnvs_h;
        dst_cnvs_h = cnvs_w;
        degree = pdp->degree;
    }

    for (i = 0; i < pt_cnt; i++)
    {
        dst_pt[i] = rotate_point(dst_cnvs_w, dst_cnvs_h, src_pt[i].x, src_pt[i].y, degree);
        dst_pt[i].x = pdp->plt_area.x + _trans_x_rpoint(pdp->plt_area.width, dst_pt[i].x, dst_cnvs_w) + src_pt[i].dx;
        dst_pt[i].y = pdp->plt_area.y + _trans_y_rpoint(pdp->plt_area.height, dst_pt[i].y, dst_cnvs_h) + src_pt[i].dy;
    }
    return 0;
}

static gint _get_vertices_rect(gint x, gint y, GdkRectangle *rect)
{
    rect->x = x-6;
    rect->y = y-6;
    rect->width = 12;
    rect->height = 12;  
    return 0;
}

static gint _get_vertices_highlight_rect(gint x, gint y, GdkRectangle *rect)
{
    rect->x = x-8;
    rect->y = y-8;
    rect->width = 16;
    rect->height = 16;  
    return 0;
}

#if 0
static gint _draw_arrow_line(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, GdkPoint pt1, GdkPoint pt2, gint dir)
{
    GdkPoint arrow_pt[7] = {{-4,-15}, {-4, 6}, {-9, 6}, {0, 16}, {10, 6}, {5, 6}, {5, -15}};
    gint mptx = (pt1.x + pt2.x) >> 1;
    gint mpty = (pt1.y + pt2.y) >> 1;
    gint dx = pt2.x - pt1.x, dy = pt2.y - pt1.y;
    gint x, y, i;
    float l = sqrtf((float)(dx * dx + dy * dy));
    GdkPoint arr[7];

    for (i = 0; i < 7; i++) 
    {
        x = arrow_pt[i].x;
        y = arrow_pt[i].y;

        arr[i].x = (gint)((float)((x * dx - y * dy) * cnvs_w / pdp->plt_area.width * 4 /
                (cnvs_w / pdp->plt_area.width + cnvs_h/pdp->plt_area.height)) / l) + mptx;              
        arr[i].y = (gint)((float)((y * dx + x * dy) * cnvs_h / pdp->plt_area.height * 4 /
                (cnvs_w / pdp->plt_area.width + cnvs_h/pdp->plt_area.height)) / l) + mpty;
    }

    gdk_draw_polygon(pdp->drawable, pdp->gc, TRUE, arr, 7);
    return 0;
}

static gint _draw_arrow_image(DIT_DP *pdp, GdkPoint pt1, GdkPoint pt2, gint dir)
{
    GdkPixbuf *arrow_image = 0;
    GdkPoint arrow_pt;

    gint mptx = (pt1.x + pt2.x) >> 1;
    gint mpty = (pt1.y + pt2.y) >> 1;   
    gint dx = pt2.x - pt1.x, dy = pt2.y - pt1.y;    

    if ((dx == 0) || (dy == 0))
    {
        if (dx == 0)
        {
            if (dy < 0)         arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_RIGHT), NULL);
            else if (dy > 0)    arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_LEFT), NULL);
        }
        else if (dy == 0)
        {
            if (dx < 0)         arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_UP), NULL);
            else if (dx > 0)    arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_DOWN), NULL);
        }
    }
    else
    {
        if ((abs(dx) == abs(dy)) || ((abs(dx) > abs(dy)) && (abs(dy) > abs(dx)/2)) || ((abs(dy) > abs(dx)) && (abs(dx) > abs(dy)/2)))
        {
            if ((dx > 0) && (dy > 0))       arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_DOWNLEFT), NULL);
            else if ((dx > 0) && (dy < 0))  arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_DOWNRIGHT), NULL);
            else if ((dx < 0) && (dy > 0))  arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_UPLEFT), NULL);
            else if ((dx < 0) && (dy < 0))  arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_UPRIGHT), NULL);           
        }
        else if (abs(dx) > abs(dy))
        {
            if (dx > 0)         arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_DOWN), NULL);
            else if (dx < 0)    arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_UP), NULL);
        }
        else if (abs(dy) > abs(dx))
        {
            if (dy > 0)         arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_LEFT), NULL);
            else if (dy < 0)    arrow_image = nfui_get_image_from_file((IMG_VA_ARROW_RIGHT), NULL);
        }
    }

    arrow_pt.x = mptx-26;
    arrow_pt.y = mpty-26;

    if (arrow_image) 
    {
        if (dir == 2) arrow_image = gdk_pixbuf_rotate_simple(arrow_image, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
        nfutil_draw_pixbuf(pdp->drawable, pdp->gc, arrow_image, arrow_pt.x, arrow_pt.y, -1, -1, NFALIGN_LEFT, 0);
    }
    
    return 0;
}
#endif

static gint _get_region_polygon(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, FIGURE_INFO *info, GdkRegion *region)
{
    GdkPoint polygon_pt[MAX_PT];   
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    GdkRegion *tmpregion;
    GdkRectangle clipbox;

// SKSHIN
    if (info->cnt == 0) return -1;

    _get_draw_point(pdp, cnvs_w, cnvs_h, info->pt, info->cnt, polygon_pt, info->nr);

    if (info->cnt == 1)
    {
        GdkRectangle tmp;

        tmp.x = polygon_pt[0].x - 1;
        tmp.y = polygon_pt[0].y - 1;
        tmp.width = 2;
        tmp.height = 2;
        
        tmpregion = gdk_region_rectangle(&tmp);
        gdk_region_get_clipbox (tmpregion, &clipbox);
        gdk_region_destroy(tmpregion);
    }
    else if (info->cnt == 2)
    {
        GdkRectangle tmp;

        tmp.x = polygon_pt[0].x > polygon_pt[1].x ? polygon_pt[1].x : polygon_pt[0].x;
        tmp.y = polygon_pt[0].y > polygon_pt[1].y ? polygon_pt[1].y : polygon_pt[0].y;
        tmp.width = abs(polygon_pt[0].x-polygon_pt[1].x);
        tmp.height = abs(polygon_pt[0].y-polygon_pt[1].y);    
        
        tmpregion = gdk_region_rectangle(&tmp);
        gdk_region_get_clipbox (tmpregion, &clipbox);
        gdk_region_destroy(tmpregion);
    }
    else
    {
        tmpregion = gdk_region_polygon(polygon_pt, info->cnt, GDK_WINDING_RULE);
        gdk_region_get_clipbox (tmpregion, &clipbox);
        gdk_region_destroy(tmpregion);   
    }

    clipbox.x -= info->wi;
    clipbox.y -= info->wi;
    clipbox.width += (info->wi*2);
    clipbox.height += (info->wi*2);

    tmpregion = gdk_region_rectangle(&clipbox);
    gdk_region_union (region, tmpregion);
    gdk_region_destroy(tmpregion);

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            if (info->pt[i].opt)
            {
                _get_vertices_highlight_rect(polygon_pt[i].x, polygon_pt[i].y, &verticehi_rect);
                verticehi_rect.x -= info->wi;
                verticehi_rect.y -= info->wi;
                verticehi_rect.width += (info->wi*2);
                verticehi_rect.height += (info->wi*2);

                tmpregion = gdk_region_rectangle(&verticehi_rect);
                gdk_region_union (region, tmpregion);
                gdk_region_destroy(tmpregion);
            }
            
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            vertice_rect.x -= info->wi;
            vertice_rect.y -= info->wi;
            vertice_rect.width += (info->wi*2);
            vertice_rect.height += (info->wi*2);

            tmpregion = gdk_region_rectangle(&vertice_rect);
            gdk_region_union (region, tmpregion);
            gdk_region_destroy(tmpregion);
        }
    }

    return 0;
}

static gint _erase_polygon(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, FIGURE_INFO *info)
{
    GdkPoint polygon_pt[MAX_PT];
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    gdk_gc_set_line_attributes(pdp->gc, info->wi, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

// SKSHIN
    if (info->cnt == 0) return -1;

    _get_draw_point(pdp, cnvs_w, cnvs_h, info->pt, info->cnt, polygon_pt, info->nr);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    if (info->fi == 1) gdk_draw_polygon(pdp->drawable, pdp->gc, TRUE, polygon_pt, info->cnt);
    else gdk_draw_polygon(pdp->drawable, pdp->gc, FALSE, polygon_pt, info->cnt);

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            if (info->pt[i].opt)
            {
                _get_vertices_highlight_rect(polygon_pt[i].x, polygon_pt[i].y, &verticehi_rect);
                gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
                gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, verticehi_rect.x, verticehi_rect.y, verticehi_rect.width, verticehi_rect.height);
            }
            
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
            gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, vertice_rect.x, vertice_rect.y, vertice_rect.width, vertice_rect.height);
        }
    }

    return 0;
}

static gint _draw_polygon(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, FIGURE_INFO *info)
{
    GdkPoint polygon_pt[MAX_PT];
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    gdk_gc_set_line_attributes(pdp->gc, info->wi, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

// SKSHIN
    if (info->cnt == 0) return -1;

    _get_draw_point(pdp, cnvs_w, cnvs_h, info->pt, info->cnt, polygon_pt, info->nr);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(info->cl.i));

    if (info->st == 0)
    {
        if (info->fi == 1) gdk_draw_polygon(pdp->drawable, pdp->gc, TRUE, polygon_pt, info->cnt);
        else gdk_draw_polygon(pdp->drawable, pdp->gc, FALSE, polygon_pt, info->cnt);
    }       
    else if (info->st != 9)
    {
        gdk_draw_lines(pdp->drawable, pdp->gc, polygon_pt, info->cnt);
    }

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            if (info->pt[i].opt)
            {
                _get_vertices_highlight_rect(polygon_pt[i].x, polygon_pt[i].y, &verticehi_rect);
                gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FFFFFF)));
                gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, verticehi_rect.x, verticehi_rect.y, verticehi_rect.width, verticehi_rect.height);
            }
            
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(info->cl.i));
            gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, vertice_rect.x, vertice_rect.y, vertice_rect.width, vertice_rect.height);
        }
    }

    return 0;
}

static int _get_dir_position(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, DIRECTION_INFO *info, GdkPoint *dst)
{
    GdkPoint line_pt[2];
    gint mptx, mpty;
    gint dx, dy;
    gint x, y, i;
    float l;

    _get_draw_point(pdp, cnvs_w, cnvs_h, info->ln, 2, line_pt, info->nr);

    mptx = (line_pt[0].x + line_pt[1].x) >> 1;
    mpty = (line_pt[0].y + line_pt[1].y) >> 1;
    dx = line_pt[1].x - line_pt[0].x;
    dy = line_pt[1].y - line_pt[0].y;
    l = sqrtf((float)(dx * dx + dy * dy));
    for (i = 0; i < info->cnt; i++) 
    {
        x = _trans_x_rpoint(pdp->plt_area.width, info->pt[i].x, cnvs_w) + info->pt[i].dx;
        y = _trans_y_rpoint(pdp->plt_area.height, info->pt[i].y, cnvs_h) + info->pt[i].dy;

        dst[i].x = (int)((float)((x * dx - y * dy) * 4 / l) + mptx);
        dst[i].y = (int)((float)((y * dx + x * dy) * 4 / l) + mpty);
    }
    
    return 0;
}

static gint _get_region_direction(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, DIRECTION_INFO *info, GdkRegion *region)
{
    GdkPoint polygon_pt[MAX_PT];   
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    GdkRegion *tmpregion;
    GdkRectangle clipbox;

// SKSHIN
    if (info->cnt == 0) return -1;

    _get_dir_position(pdp, cnvs_w, cnvs_h, info, polygon_pt);

    tmpregion = gdk_region_polygon(polygon_pt, info->cnt, GDK_WINDING_RULE);
    gdk_region_get_clipbox (tmpregion, &clipbox);
    gdk_region_destroy(tmpregion);   

    clipbox.x -= info->wi;
    clipbox.y -= info->wi;
    clipbox.width += (info->wi*2);
    clipbox.height += (info->wi*2);

    tmpregion = gdk_region_rectangle(&clipbox);
    gdk_region_union (region, tmpregion);
    gdk_region_destroy(tmpregion);

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            vertice_rect.x -= info->wi;
            vertice_rect.y -= info->wi;
            vertice_rect.width += (info->wi*2);
            vertice_rect.height += (info->wi*2);

            tmpregion = gdk_region_rectangle(&vertice_rect);
            gdk_region_union (region, tmpregion);
            gdk_region_destroy(tmpregion);
        }
    }

    return 0;
}

static gint _erase_direction(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, DIRECTION_INFO *info)
{
    GdkPoint polygon_pt[MAX_PT];
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    gdk_gc_set_line_attributes(pdp->gc, info->wi, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

// SKSHIN
    if (info->cnt == 0) return -1;

   _get_dir_position(pdp, cnvs_w, cnvs_h, info, polygon_pt);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));

    if (info->fi == 1) gdk_draw_polygon(pdp->drawable, pdp->gc, TRUE, polygon_pt, info->cnt);
    else gdk_draw_polygon(pdp->drawable, pdp->gc, FALSE, polygon_pt, info->cnt);

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
            gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, vertice_rect.x, vertice_rect.y, vertice_rect.width, vertice_rect.height);
        }
    }

    return 0;
}

static gint _draw_direction(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, DIRECTION_INFO *info)
{
    GdkPoint polygon_pt[MAX_PT];
    GdkRectangle vertice_rect;
    GdkRectangle verticehi_rect;
    gint i;

    gdk_gc_set_line_attributes(pdp->gc, info->wi, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

// SKSHIN
    if (info->cnt == 0) return -1;

    _get_dir_position(pdp, cnvs_w, cnvs_h, info, polygon_pt);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(info->cl.i));

    if (info->st == 0)
    {
        if (info->fi == 1) gdk_draw_polygon(pdp->drawable, pdp->gc, TRUE, polygon_pt, info->cnt);
        else gdk_draw_polygon(pdp->drawable, pdp->gc, FALSE, polygon_pt, info->cnt);
    }       
    else if (info->st != 9)
    {
        gdk_draw_lines(pdp->drawable, pdp->gc, polygon_pt, info->cnt);
    }

    if (info->vt == 1) 
    {
        for (i = 0; i < info->cnt; i++)
        {
            _get_vertices_rect(polygon_pt[i].x, polygon_pt[i].y, &vertice_rect);
            gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(info->cl.i));
            gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, vertice_rect.x, vertice_rect.y, vertice_rect.width, vertice_rect.height);
        }
    }

    return 0;
}

static gint _get_region_image(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, IMAGE_INFO *info, GdkRegion *region)
{
    GdkPixbuf *pBuf = 0;
    GdkPoint pt;

    GdkRegion *tmpregion;
    GdkRectangle rect;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    pBuf = nfui_get_image_from_file(info->img.ptr, NULL);
    nfui_get_pixbuf_size(pBuf, &rect.width, &rect.height);

    rect.x = pt.x-rect.width/2;
    rect.y = pt.y-rect.height/2;

    tmpregion = gdk_region_rectangle(&rect);
    gdk_region_union (region, tmpregion);
    gdk_region_destroy(tmpregion);

    return 0;
}

static gint _erase_image(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, IMAGE_INFO *info)
{
    GdkPixbuf *pBuf = 0;
    GdkPoint pt;
    guint size_w, size_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    pBuf = nfui_get_image_from_file(info->img.ptr, NULL);
    nfui_get_pixbuf_size(pBuf, &size_w, &size_h);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, pt.x-size_w/2, pt.y-size_h/2, size_w, size_h);
    
    return 0;
}

static gint _draw_image(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, IMAGE_INFO *info)
{
    GdkPixbuf *pBuf = 0;
    GdkPoint pt;
    guint size_w, size_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    pBuf = nfui_get_image_from_file(info->img.ptr, NULL);
    nfui_get_pixbuf_size(pBuf, &size_w, &size_h);
    nfutil_draw_pixbuf(pdp->drawable, pdp->gc, pBuf, pt.x-size_w/2, pt.y-size_h/2, -1, -1, NFALIGN_LEFT, 0);
    
    return 0;
}

static gint _get_region_text(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, TEXT_INFO *info, GdkRegion *region)
{
    GdkPoint pt;
    gint text_w, text_h;

    GdkRegion *tmpregion;
    GdkRectangle rect;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    text_w = nfutil_string_width(0, pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);
    text_h = nfutil_string_height(pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);

    //pt.x += info->dt.x;
    //pt.y += info->dt.y;

    if (info->layout_psx == -1) pt.x -= text_w;
    else if (info->layout_psx == 0) pt.x -= (text_w/2);

    if (info->layout_psy == -1) pt.y -= text_h;
    else if (info->layout_psy == 0) pt.y -= (text_h/2);

    pt.x = MIN(pt.x, pdp->plt_area.x+pdp->plt_area.width-text_w);
    pt.x = MAX(pdp->plt_area.x+2, pt.x);
    pt.y = MIN(pt.y, pdp->plt_area.y+pdp->plt_area.height-text_h);
    pt.y = MAX(pdp->plt_area.y+2, pt.y);

    rect.x = pt.x;
    rect.y = pt.y;
    rect.width = text_w;
    rect.height = text_h;
    
    tmpregion = gdk_region_rectangle(&rect);
    gdk_region_union (region, tmpregion);
    gdk_region_destroy(tmpregion);
    
    return 0;
}

static gint _erase_text(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, TEXT_INFO *info)
{
    GdkPoint pt;
    gint text_w, text_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    text_w = nfutil_string_width(0, pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);
    text_h = nfutil_string_height(pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);

    //pt.x += info->dt.x;
    //pt.y += info->dt.y;

    if (info->layout_psx == -1) pt.x -= text_w;
    else if (info->layout_psx == 0) pt.x -= (text_w/2);

    if (info->layout_psy == -1) pt.y -= text_h;
    else if (info->layout_psy == 0) pt.y -= (text_h/2);

    pt.x = MIN(pt.x, pdp->plt_area.x+pdp->plt_area.width-text_w);
    pt.x = MAX(pdp->plt_area.x+2, pt.x);
    pt.y = MIN(pt.y, pdp->plt_area.y+pdp->plt_area.height-text_h);
    pt.y = MAX(pdp->plt_area.y+2, pt.y);
    
    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, pt.x, pt.y, text_w, text_h);
    
    return 0;
}

static gint _draw_text(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, TEXT_INFO *info)
{
    GdkPoint pt;
    gint text_w, text_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    text_w = nfutil_string_width(0, pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);
    text_h = nfutil_string_height(pdp->drawable, nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), info->txt, NORMAL_SPACING);

    //pt.x += info->dt.x;
    //pt.y += info->dt.y;

    if (info->layout_psx == -1) pt.x -= text_w;
    else if (info->layout_psx == 0) pt.x -= (text_w/2);

    if (info->layout_psy == -1) pt.y -= text_h;
    else if (info->layout_psy == 0) pt.y -= (text_h/2);

    pt.x = MIN(pt.x, pdp->plt_area.x+pdp->plt_area.width-text_w);
    pt.x = MAX(pdp->plt_area.x+2, pt.x);
    pt.y = MIN(pt.y, pdp->plt_area.y+pdp->plt_area.height-text_h);
    pt.y = MAX(pdp->plt_area.y+2, pt.y);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    nfutil_draw_text_with_pango_outline_eng(NULL, NULL, NULL, pdp->drawable, pdp->gc, info->txt, pt.x, pt.y, text_w, text_h, nffont_get_pango_font(info->size), &UX_COLOR(info->cl.i), NFALIGN_LEFT, 0);
    
    return 0;
}

static gint _get_region_arc(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, ARC_INFO *info, GdkRegion *region)
{
    GdkRegion *tmpregion;
    GdkRectangle rect;
    GdkPoint pt;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    rect.x = pt.x;
    rect.y = pt.y;
    rect.width = _trans_w_size(pdp->plt_area.width, info->w, cnvs_w);
    rect.width = _trans_h_size(pdp->plt_area.height, info->h, cnvs_h);

    tmpregion = gdk_region_rectangle(&rect);
    gdk_region_union (region, tmpregion);
    gdk_region_destroy(tmpregion);
    
    return 0;
}

static gint _erase_arc(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, ARC_INFO *info)
{
    GdkPoint pt;
    guint size_w, size_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    size_w = _trans_w_size(pdp->plt_area.width, info->w, cnvs_w);
    size_h = _trans_h_size(pdp->plt_area.height, info->h, cnvs_h);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_arc(pdp->drawable, pdp->gc, TRUE, pt.x, pt.y, size_w, size_h, 0, 64 * 360);
    
    return 0;
}

static gint _draw_arc(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, ARC_INFO *info)
{
    GdkPoint pt;
    guint size_w, size_h;

    _get_draw_point(pdp, cnvs_w, cnvs_h, &info->pt, 1, &pt, info->nr);

    size_w = _trans_w_size(pdp->plt_area.width, info->w, cnvs_w);
    size_h = _trans_h_size(pdp->plt_area.height, info->h, cnvs_h);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(info->cl.i));
    gdk_draw_arc(pdp->drawable, pdp->gc, TRUE, pt.x, pt.y, size_w, size_h, 0, 64 * 360);
    
    return 0;
}

#if 0
static gint _erase_invalid_rect(DIT_DP *pdp, gint cnvs_w, gint cnvs_h, IGRECT rect)
{
    GdkRectangle trans_rect;

    trans_rect.x = pdp->plt_area.x + _trans_x_rpoint(pdp->plt_area.width, rect.x, cnvs_w);
    trans_rect.y = pdp->plt_area.y + _trans_y_rpoint(pdp->plt_area.height, rect.y, cnvs_h);
    trans_rect.width = _trans_w_size(pdp->plt_area.width, rect.w, cnvs_w);
    trans_rect.height = _trans_h_size(pdp->plt_area.height, rect.h, cnvs_h);

    gdk_gc_set_rgb_fg_color(pdp->gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_rectangle(pdp->drawable, pdp->gc, TRUE, trans_rect.x, trans_rect.y, trans_rect.width, trans_rect.height);  
    
    return 0;
}
#endif

static gint _get_region_dic(DITID ditid, DIT_DP *pdp, DIC *pdic, GdkRegion *region)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    switch(pdic->type)
    {
        case DIC_FIG:   
            _get_region_polygon(pdp, cnvs_w, cnvs_h, &pdic->ctns.f, region);
        break;
        case DIC_DIR:
            _get_region_direction(pdp, cnvs_w, cnvs_h, &pdic->ctns.d, region);
        break;
        case DIC_ARC:
            _get_region_arc(pdp, cnvs_w, cnvs_h, &pdic->ctns.a, region);
        break;
        case DIC_IMG:
            _get_region_image(pdp, cnvs_w, cnvs_h, &pdic->ctns.i, region);
        break;
        case DIC_TXT:
            _get_region_text(pdp, cnvs_w, cnvs_h, &pdic->ctns.t, region);
        break;
        default:
        break;
    }
    
    return 0;
}

static gint _erase_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    switch(pdic->type)
    {
        case DIC_FIG:   
            _erase_polygon(pdp, cnvs_w, cnvs_h, &pdic->ctns.f);
        break;
        case DIC_DIR:
            _erase_direction(pdp, cnvs_w, cnvs_h, &pdic->ctns.d);
        break;
        case DIC_ARC:
            _erase_arc(pdp, cnvs_w, cnvs_h, &pdic->ctns.a);
        break;
        case DIC_IMG:
            _erase_image(pdp, cnvs_w, cnvs_h, &pdic->ctns.i);
        break;
        case DIC_TXT:
            _erase_text(pdp, cnvs_w, cnvs_h, &pdic->ctns.t);
        break;
        default:
        break;
    }
    
    return 0;
}

static gint _draw_fig_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    if (pdic->type != DIC_FIG) return -1;

    _draw_polygon(pdp, cnvs_w, cnvs_h, &pdic->ctns.f);
    return 0;
}

static gint _draw_dir_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    if (pdic->type != DIC_DIR) return -1;

    _draw_direction(pdp, cnvs_w, cnvs_h, &pdic->ctns.d);
    return 0;
}

static gint _draw_img_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    if (pdic->type != DIC_IMG) return -1;

    _draw_image(pdp, cnvs_w, cnvs_h, &pdic->ctns.i);
    return 0;
}

static gint _draw_txt_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    if (pdic->type != DIC_TXT) return -1;

    _draw_text(pdp, cnvs_w, cnvs_h, &pdic->ctns.t);
    return 0;
}

static gint _draw_arc_dic(DITID ditid, DIT_DP *pdp, DIC *pdic)
{
    gint cnvs_w, cnvs_h;

    dit_get_cnvs_info(ditid, &cnvs_w, &cnvs_h);

    if (pdic->type != DIC_ARC) return -1;

    _draw_arc(pdp, cnvs_w, cnvs_h, &pdic->ctns.a);
    return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int vw_dit_display_set_vca_ditid(DITID id)
{
    g_vca_ditid = id;    
    return 0;
}

int vw_dit_display_set_vca_vmap(VCA_VMAP *vmap)
{
    memcpy(&g_vca_vmap, vmap, sizeof(VCA_VMAP));
    return 0;
}

int vw_dit_display_get_vca_diclist(VCA_CLON *clon)
{
    IGRECT cnvs_ivrect;

    if (!g_vca_ditid) return -1;
    if (!dit_is_enabled(g_vca_ditid)) return -1;

    clon->pdics = dit_new_dic_list(g_vca_ditid, &clon->dic_cnt, &cnvs_ivrect);
    return 0;
}

int vw_dit_display_free_vca_diclist(VCA_CLON clon)
{
    int i;

    if (!clon.dic_cnt) return -1;

    for (i = 0; i < clon.dic_cnt; i++)
    {
        ifree(clon.pdics[i]);
    }
    ifree(clon.pdics);
    return 0;
}

int vw_dit_display_compare_vca_diclist(VCA_CLON pre_clon, VCA_CLON post_clon)
{
    gint i;

    if (pre_clon.dic_cnt != post_clon.dic_cnt) return 0;

    for (i = 0; i < pre_clon.dic_cnt; i++)
    {
        if (memcmp(pre_clon.pdics[i], post_clon.pdics[i], sizeof(DIC))) return 0;
        //if (memcmp(&pre_clon.pdics[i]->ctns, &post_clon.pdics[i]->ctns, sizeof(DIC_CONTENTS))) return 0;
    }

    return 1;
}

int vw_dit_display_get_vca_region(VCA_DP *pdp, VCA_CLON clon, GdkRegion *region)
{
    IGRECT cnvs_ivrect;
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    GdkRegion *tmpregion;

    if (!g_vca_ditid) return 0;
    if (!dit_is_enabled(g_vca_ditid)) return 0;

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _get_region_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i], region);
    }

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);

    tmpregion = gdk_region_rectangle(&cr);
    gdk_region_intersect(region, tmpregion);
    gdk_region_destroy(tmpregion);

    return 0;
}

int vw_dit_display_vca_erase(VCA_DP *pdp, VCA_CLON pre_clon, VCA_CLON post_clon)
{
    IGRECT cnvs_ivrect;
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    if (!g_vca_ditid) return 0;
    if (!dit_is_enabled(g_vca_ditid)) return 0;

    gdk_gc_set_clip_rectangle(pdp->gc, &pdp->plt_area);

    for (i = 0; i < pre_clon.dic_cnt; i++)
    {
        if (i < post_clon.dic_cnt)
        {
            if (memcmp(pre_clon.pdics[i], post_clon.pdics[i], sizeof(DIC))) {
                _erase_dic(g_vca_ditid, (DIT_DP*)pdp, pre_clon.pdics[i]);
            }
        }
        else
        {
            _erase_dic(g_vca_ditid, (DIT_DP*)pdp, pre_clon.pdics[i]);
        }
    }

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(pdp->gc, &cr);

    return 0;
}

int vw_dit_display_vca_draw(VCA_DP *pdp, VCA_CLON clon)
{
    IGRECT cnvs_ivrect; 
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    if (!g_vca_ditid) return 0;
    if (!dit_is_enabled(g_vca_ditid)) return 0;

    gdk_gc_set_clip_rectangle(pdp->gc, &pdp->plt_area);

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_txt_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_arc_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_dir_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_fig_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_img_dic(g_vca_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }    

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(pdp->gc, &cr);

    return 0;
}

DITID vw_dit_display_get_dva_ditid()
{
    return g_dva_ditid;
}

int vw_dit_display_set_dva_ditid(DITID id)
{
    g_dva_ditid = id;    
    return 0;
}

int vw_dit_display_set_dva_vmap(DVA_VMAP *vmap)
{
    memcpy(&g_dva_vmap, vmap, sizeof(DVA_VMAP));
    return 0;
}

int vw_dit_display_get_dva_diclist(DVA_CLON *clon)
{
    IGRECT cnvs_ivrect;

    if (!g_dva_ditid) return -1;
    if (!dit_is_enabled(g_dva_ditid)) return -1;

    clon->pdics = dit_new_dic_list(g_dva_ditid, &clon->dic_cnt, &cnvs_ivrect);
    return 0;
}

int vw_dit_display_free_dva_diclist(DVA_CLON clon)
{
    int i;

    if (!clon.dic_cnt) return -1;

    for (i = 0; i < clon.dic_cnt; i++)
    {
        ifree(clon.pdics[i]);
    }
    ifree(clon.pdics);
    return 0;
}

int vw_dit_display_compare_dva_diclist(DVA_CLON pre_clon, DVA_CLON post_clon)
{
    gint i;

    if (pre_clon.dic_cnt != post_clon.dic_cnt) return 0;

    for (i = 0; i < pre_clon.dic_cnt; i++)
    {
        if (memcmp(pre_clon.pdics[i], post_clon.pdics[i], sizeof(DIC))) return 0;
        //if (memcmp(&pre_clon.pdics[i]->ctns, &post_clon.pdics[i]->ctns, sizeof(DIC_CONTENTS))) return 0;
    }

    return 1;
}

int vw_dit_display_get_dva_region(DVA_DP *pdp, DVA_CLON clon, GdkRegion *region)
{
    IGRECT cnvs_ivrect;
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    GdkRegion *tmpregion;

    if (!g_dva_ditid) return 0;
    if (!dit_is_enabled(g_dva_ditid)) return 0;

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _get_region_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i], region);
    }

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);

    tmpregion = gdk_region_rectangle(&cr);
    gdk_region_intersect(region, tmpregion);
    gdk_region_destroy(tmpregion);

    return 0;
}

int vw_dit_display_dva_erase(DVA_DP *pdp, DVA_CLON pre_clon, DVA_CLON post_clon)
{
    IGRECT cnvs_ivrect;
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    if (!g_dva_ditid) return 0;
    if (!dit_is_enabled(g_dva_ditid)) return 0;

    gdk_gc_set_clip_rectangle(pdp->gc, &pdp->plt_area);

    for (i = 0; i < pre_clon.dic_cnt; i++)
    {
        if (i < post_clon.dic_cnt)
        {
            if (memcmp(pre_clon.pdics[i], post_clon.pdics[i], sizeof(DIC))) {
                _erase_dic(g_dva_ditid, (DIT_DP*)pdp, pre_clon.pdics[i]);
            }
        }
        else
        {
            _erase_dic(g_dva_ditid, (DIT_DP*)pdp, pre_clon.pdics[i]);
        }
    }

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(pdp->gc, &cr);

    return 0;
}

int vw_dit_display_dva_draw(DVA_DP *pdp, DVA_CLON clon)
{
    IGRECT cnvs_ivrect; 
    gint i;
    GdkRectangle cr;
    gint dic_cnt;

    if (!g_dva_ditid) return 0;
    if (!dit_is_enabled(g_dva_ditid)) return 0;

    gdk_gc_set_clip_rectangle(pdp->gc, &pdp->plt_area);

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_txt_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_arc_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_dir_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_fig_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }

    for (i = 0; i < clon.dic_cnt; i++)
    {
        _draw_img_dic(g_dva_ditid, (DIT_DP*)pdp, clon.pdics[i]);
    }    

    memset(&cr, 0x00, sizeof(GdkRectangle));
    gdk_drawable_get_size(pdp->drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(pdp->gc, &cr);

    return 0;
}

