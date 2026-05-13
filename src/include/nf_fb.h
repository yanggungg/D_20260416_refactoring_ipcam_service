#ifndef __NF_FB_H__
#define __NF_FB_H__

/*
	in kernel
	include/uapi/linux/fb.h
 */
typedef unsigned int	__u32;

struct fb_bitfield {
	__u32 offset;           /* beginning of bitfield    */
	__u32 length;           /* length of bitfield       */
	__u32 msb_right;        /* != 0 : Most significant bit is */
					/* right */
};

struct fb_var_screeninfo {
	__u32 xres;         /* visible resolution       */
	__u32 yres;
	__u32 xres_virtual;     /* virtual resolution       */
	__u32 yres_virtual;
	__u32 xoffset;          /* offset from virtual to visible */
	__u32 yoffset;          /* resolution           */

	__u32 bits_per_pixel;       /* guess what           */
	__u32 grayscale;        /* 0 = color, 1 = grayscale,    */
					/* >1 = FOURCC          */
	struct fb_bitfield red;     /* bitfield in fb mem if true color, */
	struct fb_bitfield green;   /* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;  /* transparency         */

	__u32 nonstd;           /* != 0 Non standard pixel format */

	__u32 activate;         /* see FB_ACTIVATE_*        */

	__u32 height;           /* height of picture in mm    */
	__u32 width;            /* width of picture in mm     */

	__u32 accel_flags;      /* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	__u32 pixclock;         /* pixel clock in ps (pico seconds) */
	__u32 left_margin;      /* time from sync to picture    */
	__u32 right_margin;     /* time from picture to sync    */
	__u32 upper_margin;     /* time from sync to picture    */
	__u32 lower_margin;
	__u32 hsync_len;        /* length of horizontal sync    */
	__u32 vsync_len;        /* length of vertical sync  */
	__u32 sync;         /* see FB_SYNC_*        */
	__u32 vmode;            /* see FB_VMODE_*       */
	__u32 rotate;           /* angle we rotate counter clockwise */
	__u32 colorspace;       /* colorspace for FOURCC-based modes */
	__u32 reserved[4];      /* Reserved for future compatibility */
};

#endif

