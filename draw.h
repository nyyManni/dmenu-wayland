/* See LICENSE file for copyright and license details. */
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
/* #include <pango/pangocairo.h> */


#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

struct panel {
	
};

typedef struct {
	int x, y, w, h;
    int text_offset_y;
	/* Bool invert; */
	struct wl_buffer *buffer;
	struct wl_display * dpy;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct wl_seat *seat;
	struct wl_shm *shm;
	void *shm_data;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct wl_output *output;

	struct wl_keyboard *kbd;
	bool running;

	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	bool control;

	int32_t panel_height;
	int32_t scale;
	int32_t width;
	int32_t height;
	int32_t logical_width;
	int32_t logical_height;

	enum wl_output_subpixel subpixel;

	/* struct xdg_wm_base *xdg; */
	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct zxdg_output_v1 *xdg_output;

	/* Display *dpy; */
	cairo_t *cairo;
	/* GC gc; */
	/* Pixmap canvas; */
	struct {
		int ascent;
		int descent;
		int height;
		/* XFontSet set; */
		/* XFontStruct *xfont; */
	} font;
} DC;  /* draw context */

unsigned long getcolor(DC *dc, const char *colstr);
/* void drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, Bool fill, unsigned long color); */
void drawtext(DC *dc, const char *text, unsigned long col[ColLast]);
void drawtextn(DC *dc, const char *text, size_t n, unsigned long col[ColLast]);
void initfont(DC *dc, const char *fontstr);
void freedc(DC *dc);
DC *initdc(int32_t);
void draw(DC *);
/* void mapdc(DC *dc, Window win, unsigned int w, unsigned int h); */
void resizedc(DC *dc, unsigned int w, unsigned int h);
int textnw(DC *dc, const char *text, size_t len);
int textw(DC *dc, const char *text);
void eprintf(const char *fmt, ...);
void weprintf(const char *fmt, ...);

const char *progname;
