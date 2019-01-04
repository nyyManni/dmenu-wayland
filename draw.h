/* See LICENSE file for copyright and license details. */
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
/* #include <pango/pangocairo.h> */


#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

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
	struct xdg_wm_base *xdg;

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
DC *initdc(void);
/* void mapdc(DC *dc, Window win, unsigned int w, unsigned int h); */
void resizedc(DC *dc, unsigned int w, unsigned int h);
int textnw(DC *dc, const char *text, size_t len);
int textw(DC *dc, const char *text);
void eprintf(const char *fmt, ...);
void weprintf(const char *fmt, ...);

const char *progname;
