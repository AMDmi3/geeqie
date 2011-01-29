/*
 * Geeqie
 * (C) 2006 John Ellis
 * Copyright (C) 2008 - 2010 The Geeqie Team
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#ifndef PIXBUF_RENDERER_H
#define PIXBUF_RENDERER_H

#include <gtk/gtkeventbox.h>
#include <gtk/gtkwindow.h>


#define TYPE_PIXBUF_RENDERER		(pixbuf_renderer_get_type())
#define PIXBUF_RENDERER(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_PIXBUF_RENDERER, PixbufRenderer))
#define PIXBUF_RENDERER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), TYPE_PIXBUF_RENDERER, PixbufRendererClass))
#define IS_PIXBUF_RENDERER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PIXBUF_RENDERER))
#define IS_PIXBUF_RENDERER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_PIXBUF_RENDERER))
#define PIXBUF_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), TYPE_PIXBUF_RENDERER, PixbufRendererClass))

/* alpha channel checkerboard background (same as gimp) */
#define PR_ALPHA_CHECK1 0x00999999
#define PR_ALPHA_CHECK2 0x00666666
#define PR_ALPHA_CHECK_SIZE 16
/* when scaling image to below this size, use nearest pixel for scaling
 * (below about 4, the other scale types become slow generating their conversion tables)
 */
#define PR_MIN_SCALE_SIZE 8

/* default size of tile cache (mb) */
#define PR_CACHE_SIZE_DEFAULT 8

/* round A up/down to integer count of B */
#define ROUND_UP(A,B)   ((gint)(((A)+(B)-1)/(B))*(B))
#define ROUND_DOWN(A,B) ((gint)(((A))/(B))*(B))


typedef struct _RendererFuncs RendererFuncs;

typedef struct _PixbufRenderer PixbufRenderer;
typedef struct _PixbufRendererClass PixbufRendererClass;


typedef gint (* PixbufRendererTileRequestFunc)(PixbufRenderer *pr, gint x, gint y,
					       gint width, gint height, GdkPixbuf *pixbuf, gpointer user_data);
typedef void (* PixbufRendererTileDisposeFunc)(PixbufRenderer *pr, gint x, gint y,
					       gint width, gint height, GdkPixbuf *pixbuf, gpointer user_data);

typedef void (* PixbufRendererPostProcessFunc)(PixbufRenderer *pr, GdkPixbuf **pixbuf, gint x, gint y,
					       gint width, gint height, gpointer user_data);

typedef enum {
	PR_SCROLL_RESET_TOPLEFT = 0,
	PR_SCROLL_RESET_CENTER,
	PR_SCROLL_RESET_NOCHANGE,
	PR_SCROLL_RESET_COUNT,
} PixbufRendererScrollResetType;

typedef enum {
	TILE_RENDER_NONE = 0,	/* do nothing */
	TILE_RENDER_AREA,	/* render an area of the tile */
	TILE_RENDER_ALL		/* render the whole tile */
} ImageRenderType;

typedef enum {
	OVL_NORMAL 	= 0,
	OVL_RELATIVE 	= 1 << 0, /* x,y coordinates are relative, negative values start bottom right */
	/* OVL_HIDE_ON_SCROLL = 1 << 1*/ /* hide temporarily when scrolling (not yet implemented) */
} OverlayRendererFlags;

typedef enum {
	PR_STEREO_NONE     = 0,	  /* do nothing */
	PR_STEREO_DUAL     = 1 << 0, /* independent stereo buffers, for example nvidia opengl */
	PR_STEREO_HORIZ    = 1 << 2, /* side by side */
	PR_STEREO_VERT     = 1 << 3, /* above below */
	/* flags for renderer: */
	PR_STEREO_RIGHT    = 1 << 4, /* above below */
	PR_STEREO_ANAGLYPH = 1 << 5  /* anaglyph */
	
} PixbufRendererStereoMode;

struct _RendererFuncs
{
	void (*queue)(void *renderer, gint x, gint y, gint w, gint h,
                     gint clamp, ImageRenderType render, gboolean new_data, gboolean only_existing);
	void (*queue_clear)(void *renderer);
	void (*border_draw)(void *renderer, gint x, gint y, gint w, gint h);
	void (*invalidate_all)(void *renderer);
	void (*invalidate_region)(void *renderer, gint x, gint y, gint w, gint h);
	void (*scroll)(void *renderer, gint x_off, gint y_off);
	void (*update_sizes)(void *renderer);

	gint (*overlay_add)(void *renderer, GdkPixbuf *pixbuf, gint x, gint y, OverlayRendererFlags flags);
	void (*overlay_set)(void *renderer, gint id, GdkPixbuf *pixbuf, gint x, gint y);
	gboolean (*overlay_get)(void *renderer, gint id, GdkPixbuf **pixbuf, gint *x, gint *y);
	void (*overlay_draw)(void *renderer, gint x, gint y, gint w, gint h);

	void (*free)(void *renderer);
};

struct _PixbufRenderer
{
	GtkEventBox eventbox;

	gint image_width;	/* image actual dimensions (pixels) */
	gint image_height;
	gint stereo_pixbuf_off; /* offset of the right part of the stereo image in pixbuf */

	GdkPixbuf *pixbuf;

	gint window_width;	/* allocated size of window (drawing area) */
	gint window_height;

	gint viewport_width;	/* allocated size of viewport (same as window for normal mode, half of window for SBS mode) */
	gint viewport_height;

	gint x_offset;		/* offset of image start (non-zero when viewport < window) */
	gint y_offset;
	
	gint x_mouse; /* coordinates of the mouse taken from GtkEvent */
	gint y_mouse;

	gint vis_width;		/* dimensions of visible part of image */
	gint vis_height;

	gint width;		/* size of scaled image (result) */
	gint height;

	gint x_scroll;		/* scroll offset of image (into width, height to start drawing) */
	gint y_scroll;

	gdouble norm_center_x;	/* coordinates of viewport center in the image, in range 0.0 - 1.0 */
	gdouble norm_center_y;  /* these coordinates are used for PR_SCROLL_RESET_NOCHANGE and should be preserved over periods with NULL pixbuf */
	
	gdouble subpixel_x_scroll; /* subpixel scroll alignment, used to prevent acumulation of rounding errors */
	gdouble subpixel_y_scroll;

	gdouble zoom_min;
	gdouble zoom_max;
	gdouble zoom;		/* zoom we want (0 is auto) */
	gdouble scale;		/* zoom we got (should never be 0) */

	gdouble aspect_ratio;   /* screen pixel aspect ratio (2.0 for 3DTV SBS mode) */

	GdkInterpType zoom_quality;
	gboolean zoom_2pass;
	gboolean zoom_expand;

	GdkRgbDither dither_quality;

	PixbufRendererScrollResetType scroll_reset;

	gboolean has_frame;

	GtkWidget *parent_window;	/* resize parent_window when image dimensions change */

	gboolean window_fit;
	gboolean window_limit;
	gint window_limit_size;

	gboolean autofit_limit;
	gint autofit_limit_size;



	/*< private >*/
	gboolean in_drag;
	gint drag_last_x;
	gint drag_last_y;
	gint drag_moved;

	gboolean source_tiles_enabled;
	gint source_tiles_cache_size;

	GList *source_tiles;	/* list of active source tiles */
	gint source_tile_width;
	gint source_tile_height;

	PixbufRendererTileRequestFunc func_tile_request;
	PixbufRendererTileDisposeFunc func_tile_dispose;

	gpointer func_tile_data;

	PixbufRendererPostProcessFunc func_post_process;
	gpointer post_process_user_data;
	gint post_process_slow;

	gboolean delay_flip;
	gboolean loading;
	gboolean complete;
	gboolean debug_updated; /* debug only */

	guint scroller_id; /* event source id */
	gint scroller_overlay;
	gint scroller_x;
	gint scroller_y;
	gint scroller_xpos;
	gint scroller_ypos;
	gint scroller_xinc;
	gint scroller_yinc;

	gint orientation;

	gint stereo_mode;
	
	RendererFuncs *renderer;
	RendererFuncs *renderer2;
};

struct _PixbufRendererClass
{
	GtkEventBoxClass parent_class;

	void (*zoom)(PixbufRenderer *pr, gdouble zoom);
	void (*clicked)(PixbufRenderer *pr, GdkEventButton *event);
	void (*scroll_notify)(PixbufRenderer *pr);
	void (*update_pixel)(PixbufRenderer *pr);

	void (*render_complete)(PixbufRenderer *pr);
	void (*drag)(PixbufRenderer *pr, GdkEventButton *event);
};




GType pixbuf_renderer_get_type(void);

PixbufRenderer *pixbuf_renderer_new(void);

void pixbuf_renderer_set_parent(PixbufRenderer *pr, GtkWindow *window);
GtkWindow *pixbuf_renderer_get_parent(PixbufRenderer *pr);

/* display a pixbuf */

void pixbuf_renderer_set_pixbuf(PixbufRenderer *pr, GdkPixbuf *pixbuf, gdouble zoom);

/* same as pixbuf_renderer_set_pixbuf but waits with redrawing for pixbuf_renderer_area_changed */
void pixbuf_renderer_set_pixbuf_lazy(PixbufRenderer *pr, GdkPixbuf *pixbuf, gdouble zoom, gint orientation);


GdkPixbuf *pixbuf_renderer_get_pixbuf(PixbufRenderer *pr);

void pixbuf_renderer_set_orientation(PixbufRenderer *pr, gint orientation);
gint pixbuf_renderer_get_orientation(PixbufRenderer *pr);

void pixbuf_renderer_set_post_process_func(PixbufRenderer *pr, PixbufRendererPostProcessFunc func, gpointer user_data, gboolean slow);

/* display an on-request array of pixbuf tiles */

void pixbuf_renderer_set_tiles(PixbufRenderer *pr, gint width, gint height,
			       gint tile_width, gint tile_height, gint cache_size,
			       PixbufRendererTileRequestFunc func_request,
			       PixbufRendererTileDisposeFunc func_dispose,
			       gpointer user_data,
			       gdouble zoom);
void pixbuf_renderer_set_tiles_size(PixbufRenderer *pr, gint width, gint height);
gint pixbuf_renderer_get_tiles(PixbufRenderer *pr);

/* move image data from source to pr, source is then set to NULL image */

void pixbuf_renderer_move(PixbufRenderer *pr, PixbufRenderer *source);

/* update region of existing image */

void pixbuf_renderer_area_changed(PixbufRenderer *pr, gint x, gint y, gint width, gint height);

/* scrolling */

void pixbuf_renderer_scroll(PixbufRenderer *pr, gint x, gint y);
void pixbuf_renderer_scroll_to_point(PixbufRenderer *pr, gint x, gint y,
				     gdouble x_align, gdouble y_align);

void pixbuf_renderer_get_scroll_center(PixbufRenderer *pr, gdouble *x, gdouble *y);
void pixbuf_renderer_set_scroll_center(PixbufRenderer *pr, gdouble x, gdouble y);
/* zoom */

void pixbuf_renderer_zoom_adjust(PixbufRenderer *pr, gdouble increment);
void pixbuf_renderer_zoom_adjust_at_point(PixbufRenderer *pr, gdouble increment, gint x, gint y);

void pixbuf_renderer_zoom_set(PixbufRenderer *pr, gdouble zoom);
gdouble pixbuf_renderer_zoom_get(PixbufRenderer *pr);
gdouble pixbuf_renderer_zoom_get_scale(PixbufRenderer *pr);

void pixbuf_renderer_zoom_set_limits(PixbufRenderer *pr, gdouble min, gdouble max);

/* sizes */

gboolean pixbuf_renderer_get_image_size(PixbufRenderer *pr, gint *width, gint *height);
gboolean pixbuf_renderer_get_scaled_size(PixbufRenderer *pr, gint *width, gint *height);

/* region of image in pixel coordinates */
gboolean pixbuf_renderer_get_visible_rect(PixbufRenderer *pr, GdkRectangle *rect);

/* actual size of the PixbufRenderer window minus borders,
 * x and y are the scroll offset and include zoom factor.
 */
gboolean pixbuf_renderer_get_virtual_rect(PixbufRenderer *pr, GdkRectangle *rect);

/* background color */
void pixbuf_renderer_set_color(PixbufRenderer *pr, GdkColor *color);

/* overlay */

gint pixbuf_renderer_overlay_add(PixbufRenderer *pr, GdkPixbuf *pixbuf, gint x, gint y,
				 OverlayRendererFlags flags);
void pixbuf_renderer_overlay_set(PixbufRenderer *pr, gint id, GdkPixbuf *pixbuf, gint x, gint y);
gboolean pixbuf_renderer_overlay_get(PixbufRenderer *pr, gint id, GdkPixbuf **pixbuf, gint *x, gint *y);
void pixbuf_renderer_overlay_remove(PixbufRenderer *pr, gint id);

gboolean pixbuf_renderer_get_mouse_position(PixbufRenderer *pr, gint *x_pixel, gint *y_pixel);
/* x_pixel and y_pixel are the pixel coordinates \see pixbuf_renderer_get_mouse_position */
gboolean pixbuf_renderer_get_pixel_colors(PixbufRenderer *pr, gint x_pixel, gint y_pixel,
	 				gint *r_mouse, gint *g_mouse, gint *b_mouse);

/* protected - for renderer use only*/

typedef struct _SourceTile SourceTile;
struct _SourceTile
{
	gint x;
	gint y;
	GdkPixbuf *pixbuf;
	gboolean blank;
};


gboolean pr_clip_region(gint x, gint y, gint w, gint h,
			       gint clip_x, gint clip_y, gint clip_w, gint clip_h,
			       gint *rx, gint *ry, gint *rw, gint *rh);
void pr_render_complete_signal(PixbufRenderer *pr);

void pr_tile_coords_map_orientation(gint orientation,
				     gdouble tile_x, gdouble tile_y, /* coordinates of the tile */
				     gdouble image_w, gdouble image_h,
				     gdouble tile_w, gdouble tile_h,
				     gdouble *res_x, gdouble *res_y);
void pr_tile_region_map_orientation(gint orientation,
				     gint area_x, gint area_y, /* coordinates of the area inside tile */
				     gint tile_w, gint tile_h,
				     gint area_w, gint area_h,
				     gint *res_x, gint *res_y,
				     gint *res_w, gint *res_h);
void pr_coords_map_orientation_reverse(gint orientation,
				     gint area_x, gint area_y,
				     gint tile_w, gint tile_h,
				     gint area_w, gint area_h,
				     gint *res_x, gint *res_y,
				     gint *res_w, gint *res_h);

GList *pr_source_tile_compute_region(PixbufRenderer *pr, gint x, gint y, gint w, gint h, gboolean request);

void pr_create_anaglyph(GdkPixbuf *pixbuf, GdkPixbuf *right, gint x, gint y, gint w, gint h);
#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
