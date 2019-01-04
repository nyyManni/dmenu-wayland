/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
/* #include <X11/Xlib.h> */
#include "draw.h"
#include "shm.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
/* #include "cat.h" */

#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define DEFFONT     "fixed"

/* struct xdg_toplevel *xdg_toplevel; */
/* static Bool loadfont(DC *dc, const char *fontstr); */

/* void */
/* drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, Bool fill, unsigned long color) { */
/* 	XRectangle r = { dc->x + x, dc->y + y, w, h }; */

/* 	if(!fill) { */
/* 		r.width -= 1; */
/* 		r.height -= 1; */
/* 	} */
/* 	XSetForeground(dc->dpy, dc->gc, color); */
/* 	(fill ? XFillRectangles : XDrawRectangles)(dc->dpy, dc->canvas, dc->gc, &r, 1); */
/* } */


/* void */
/* drawtext(DC *dc, const char *text, unsigned long col[ColLast]) { */
/* 	char buf[256]; */
/* 	size_t n, mn; */

/* 	/\* shorten text if necessary *\/ */
/* 	n = strlen(text); */
/* 	for(mn = MIN(n, sizeof buf); textnw(dc, text, mn) > dc->w - dc->font.height/2; mn--) */
/* 		if(mn == 0) */
/* 			return; */
/* 	memcpy(buf, text, mn); */
/* 	if(mn < n) */
/* 		for(n = MAX(mn-3, 0); n < mn; buf[n++] = '.'); */

/* 	drawrect(dc, 0, 0, dc->w, dc->h, True, BG(dc, col)); */
/* 	drawtextn(dc, buf, mn, col); */
/* } */

/* void */
/* drawtextn(DC *dc, const char *text, size_t n, unsigned long col[ColLast]) { */
/* 	int x, y; */

/* 	x = dc->x + dc->font.height/2; */
/* 	y = dc->y + dc->font.ascent+1 + dc->text_offset_y; */

/* 	XSetForeground(dc->dpy, dc->gc, FG(dc, col)); */
/* 	if(dc->font.set) */
/* 		XmbDrawString(dc->dpy, dc->canvas, dc->font.set, dc->gc, x, y, text, n); */
/* 	else { */
/* 		XSetFont(dc->dpy, dc->gc, dc->font.xfont->fid); */
/* 		XDrawString(dc->dpy, dc->canvas, dc->gc, x, y, text, n); */
/* 	} */
/* } */

void
eprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
freedc(DC *dc) {
	/* if(dc->font.set) */
	/* 	XFreeFontSet(dc->dpy, dc->font.set); */
	/* if(dc->font.xfont) */
	/* 	XFreeFont(dc->dpy, dc->font.xfont); */
	/* if(dc->canvas) */
	/* 	XFreePixmap(dc->dpy, dc->canvas); */
	/* XFreeGC(dc->dpy, dc->gc); */
	/* XCloseDisplay(dc->dpy); */
	free(dc);
}

/* unsigned long */
/* getcolor(DC *dc, const char *colstr) { */
/* 	Colormap cmap = DefaultColormap(dc->dpy, DefaultScreen(dc->dpy)); */
/* 	XColor color; */

/* 	if(!XAllocNamedColor(dc->dpy, cmap, colstr, &color, &color)) */
/* 		eprintf("cannot allocate color '%s'\n", colstr); */
/* 	return color.pixel; */
/* } */
static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t width, uint32_t height) {
	/* struct swaybar_output *output = data; */
	/* output->width = width; */
	/* output->height = height; */
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	/* set_output_dirty(output); */
}

static void layer_surface_closed(void *_data,
		struct zwlr_layer_surface_v1 *surface) {
	/* struct swaybar_output *output = _output; */
	/* swaybar_output_free(output); */
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

static void add_layer_surface(DC *dc) {
  dc->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      dc->layer_shell, dc->surface, dc->output,
      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "panel");
  zwlr_layer_surface_v1_set_size(dc->layer_surface, 1920, 20);
  zwlr_layer_surface_v1_set_anchor(dc->layer_surface,
								   ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
								   ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);

  zwlr_layer_surface_v1_add_listener(dc->layer_surface, &layer_surface_listener,
                                     dc);
  /* printf("%f\n", dc->output->scale); */
}


static void noop() {
	// This space intentionally left blank
}

/* static void xdg_surface_handle_configure(void *data, */
/* 		struct xdg_surface *xdg_surface, uint32_t serial) { */
/* 	xdg_surface_ack_configure(xdg_surface, serial); */
/* } */

/* static const struct xdg_surface_listener xdg_surface_listener = { */
/* 	.configure = xdg_surface_handle_configure, */
/* }; */

/* static void xdg_toplevel_handle_close(void *data, */
/* 		struct xdg_toplevel *xdg_toplevel) { */
/* 	/\* running = false; *\/ */
/* } */

/* static const struct xdg_toplevel_listener xdg_toplevel_listener = { */
/* 	.configure = noop, */
/* 	.close = xdg_toplevel_handle_close, */
/* }; */

/* struct zxdg_output_v1_listener xdg_output_listener = { */
/* 	.logical_position = xdg_output_handle_logical_position, */
/* 	.logical_size = xdg_output_handle_logical_size, */
/* 	.done = xdg_output_handle_done, */
/* 	.name = xdg_output_handle_name, */
/* 	.description = xdg_output_handle_description, */
/* }; */

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	DC *dc = data;
	
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		dc->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {

	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		dc->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);

	/* } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) { */
	/* 	dc->xdg = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1); */

	/* } else if (strcmp(interface, wl_output_interface.name) == 0) { */

	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		dc->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);

	/* } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) { */

	}
}

static void handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	/* struct swaybar *bar = data; */
	/* struct swaybar_output *output, *tmp; */
	/* wl_list_for_each_safe(output, tmp, &bar->outputs, link) { */
	/* 	if (output->wl_name == name) { */
	/* 		swaybar_output_free(output); */
	/* 		break; */
	/* 	} */
	/* } */
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

static struct wl_buffer *create_buffer();

DC *
initdc(void) {
	DC *dc;

	if(!setlocale(LC_CTYPE, ""))
		weprintf("no locale support\n");
	if(!(dc = malloc(sizeof *dc)))
		eprintf("cannot malloc %u bytes\n", sizeof *dc);

	if(!(dc->dpy = wl_display_connect(NULL)))
		eprintf("cannot open display\n");


	struct wl_registry *registry = wl_display_get_registry(dc->dpy);
	wl_registry_add_listener(registry, &registry_listener, dc);

	wl_display_roundtrip(dc->dpy);

	dc->buffer = create_buffer(dc);

	dc->surface = wl_compositor_create_surface(dc->compositor);
	/* struct zwlr_layer_surface_v1 *wlr_surface; */
	/* struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(dc->xdg, dc->surface); */

	/* xdg_toplevel = xdg_surface_get_toplevel(xdg_surface); */

	/* xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL); */
	/* xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL); */
	add_layer_surface(dc);


	wl_surface_commit(dc->surface);
	wl_display_roundtrip(dc->dpy);

	wl_surface_attach(dc->surface, dc->buffer, 0, 0);
	wl_surface_commit(dc->surface);


	/* dc->gc = XCreateGC(dc->dpy, DefaultRootWindow(dc->dpy), 0, NULL); */
	/* XSetLineAttributes(dc->dpy, dc->gc, 1, LineSolid, CapButt, JoinMiter); */
	/* dc->font.xfont = NULL; */
	/* dc->font.set = NULL; */
	/* dc->canvas = None; */
	return dc;
}
static struct wl_buffer *create_buffer(DC *dc) {
	int width = 1920;
	int height = 128;


	int stride = width * 4;
	int size = stride * height;

	int fd = create_shm_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
		return NULL;
	}

	dc->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (dc->shm_data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(dc->shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
		stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);

	// MagickImage is from cat.h
	/* memcpy(dc->shm_data, MagickImage, size); */
	memset(dc->shm_data, 0xff, size);
	return buffer;
}

void
initfont(DC *dc, const char *fontstr) {
/* 	if(!loadfont(dc, fontstr ? fontstr : DEFFONT)) { */
/* 		if(fontstr != NULL) */
/* 			weprintf("cannot load font '%s'\n", fontstr); */
/* 		if(fontstr == NULL || !loadfont(dc, DEFFONT)) */
/* 			eprintf("cannot load font '%s'\n", DEFFONT); */
/* 	} */
/* 	dc->font.height = dc->font.ascent + dc->font.descent; */
}

/* Bool */
/* loadfont(DC *dc, const char *fontstr) { */
/* 	char *def, **missing; */
/* 	int i, n; */

/* 	if(!*fontstr) */
/* 		return False; */
/* 	if((dc->font.set = XCreateFontSet(dc->dpy, fontstr, &missing, &n, &def))) { */
/* 		char **names; */
/* 		XFontStruct **xfonts; */

/* 		n = XFontsOfFontSet(dc->font.set, &xfonts, &names); */
/* 		for(i = dc->font.ascent = dc->font.descent = 0; i < n; i++) { */
/* 			dc->font.ascent = MAX(dc->font.ascent, xfonts[i]->ascent); */
/* 			dc->font.descent = MAX(dc->font.descent, xfonts[i]->descent); */
/* 		} */
/* 	} */
/* 	else if((dc->font.xfont = XLoadQueryFont(dc->dpy, fontstr))) { */
/* 		dc->font.ascent = dc->font.xfont->ascent; */
/* 		dc->font.descent = dc->font.xfont->descent; */
/* 	} */
/* 	if(missing) */
/* 		XFreeStringList(missing); */
/* 	return (dc->font.set || dc->font.xfont); */
/* } */

/* void */
/* mapdc(DC *dc, Window win, unsigned int w, unsigned int h) { */
/* 	XCopyArea(dc->dpy, dc->canvas, win, dc->gc, 0, 0, w, h, 0, 0); */
/* } */

void
resizedc(DC *dc, unsigned int w, unsigned int h) {
/* 	if(dc->canvas) */
/* 		XFreePixmap(dc->dpy, dc->canvas); */
/* 	dc->canvas = XCreatePixmap(dc->dpy, DefaultRootWindow(dc->dpy), w, h, */
/* 	                           DefaultDepth(dc->dpy, DefaultScreen(dc->dpy))); */
/* 	dc->x = dc->y = 0; */
/* 	dc->w = w; */
/* 	dc->h = h; */
/* 	dc->invert = False; */
/* } */

/* int */
/* textnw(DC *dc, const char *text, size_t len) { */
/* 	if(dc->font.set) { */
/* 		XRectangle r; */

/* 		XmbTextExtents(dc->font.set, text, len, NULL, &r); */
/* 		return r.width; */
/* 	} */
/* 	return XTextWidth(dc->font.xfont, text, len); */
}

int
textw(DC *dc, const char *text) {
	/* return textnw(dc, text, strlen(text)) + dc->font.height; */
	return 0;
}

void
weprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: warning: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
