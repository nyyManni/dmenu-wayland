/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon.h>
/* #include <X11/Xlib.h> */
#include "draw.h"
#include "shm.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"


/* #define MAX(a, b)   ((a) > (b) ? (a) : (b)) */
/* #define MIN(a, b)   ((a) < (b) ? (a) : (b)) */
#define DEFFONT     "fixed"

#define HEIGHT 40 

static const char overflow[] = "[buffer overflow]";
static const int max_chars = 16384;


PangoLayout *get_pango_layout(cairo_t *cairo, const char *font,
		const char *text, double scale, bool markup) {
	PangoLayout *layout = pango_cairo_create_layout(cairo);
	PangoAttrList *attrs;
	if (markup) {
		char *buf;
		GError *error = NULL;
		if (pango_parse_markup(text, -1, 0, &attrs, &buf, NULL, &error)) {
			pango_layout_set_text(layout, buf, -1);
			free(buf);
		} else {
			/* wlr_log(WLR_ERROR, "pango_parse_markup '%s' -> error %s", text, */
			/* 		error->message); */
			g_error_free(error);
			markup = false; // fallback to plain text
		}
	}
	if (!markup) {
		attrs = pango_attr_list_new();
		pango_layout_set_text(layout, text, -1);
	}

	pango_attr_list_insert(attrs, pango_attr_scale_new(scale));
	PangoFontDescription *desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(layout, desc);
	pango_layout_set_single_paragraph_mode(layout, 1);
	pango_layout_set_attributes(layout, attrs);
	pango_attr_list_unref(attrs);
	pango_font_description_free(desc);
	return layout;
}

void get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
		int *baseline, double scale, bool markup, const char *fmt, ...) {
	char buf[max_chars];

	va_list args;
	va_start(args, fmt);
	if (vsnprintf(buf, sizeof(buf), fmt, args) >= max_chars) {
		strcpy(&buf[sizeof(buf) - sizeof(overflow)], overflow);
	}
	va_end(args);

	PangoLayout *layout = get_pango_layout(cairo, font, buf, scale, markup);
	pango_cairo_update_layout(cairo, layout);
	pango_layout_get_pixel_size(layout, width, height);
	if (baseline) {
		*baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
	}
	g_object_unref(layout);
}

void pango_printf(cairo_t *cairo, const char *font,
		double scale, bool markup, const char *fmt, ...) {
	char buf[max_chars];

	va_list args;
	va_start(args, fmt);
	if (vsnprintf(buf, sizeof(buf), fmt, args) >= max_chars) {
		strcpy(&buf[sizeof(buf) - sizeof(overflow)], overflow);
	}
	va_end(args);

	PangoLayout *layout = get_pango_layout(cairo, font, buf, scale, markup);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_get_font_options(cairo, fo);
	pango_cairo_context_set_font_options(pango_layout_get_context(layout), fo);
	cairo_font_options_destroy(fo);
	pango_cairo_update_layout(cairo, layout);
	pango_cairo_show_layout(cairo, layout);
	g_object_unref(layout);
}

cairo_subpixel_order_t to_cairo_subpixel_order(enum wl_output_subpixel subpixel) {
	switch (subpixel) {
	case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
		return CAIRO_SUBPIXEL_ORDER_RGB;
	case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
		return CAIRO_SUBPIXEL_ORDER_BGR;
	case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
		return CAIRO_SUBPIXEL_ORDER_VRGB;
	case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
		return CAIRO_SUBPIXEL_ORDER_VBGR;
	default:
		return CAIRO_SUBPIXEL_ORDER_DEFAULT;
	}
	return CAIRO_SUBPIXEL_ORDER_DEFAULT;
}

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


void
drawtext(DC *dc, const char *text, unsigned long col[ColLast]) {
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
}

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

  printf("logical width: %d\n", dc->logical_width);
  int32_t height = HEIGHT / ((double)dc->width / dc->logical_width);

  zwlr_layer_surface_v1_set_size(dc->layer_surface, dc->logical_width, height);
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

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
		int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
		const char *make, const char *model, int32_t transform) {
	DC *dc = data;
	dc->subpixel = subpixel;
	/* printf("x: %d, y: %d\n", x, y); */
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh) {
	DC *dc = data;
	printf("w: %d, h: %d\n", width, height);
	dc->width = width;
	dc->height = height;
}

static void output_done(void *data, struct wl_output *wl_output) {
	/* struct swaybar_output *output = data; */
	/* set_output_dirty(output); */
}

static void output_scale(void *data, struct wl_output *wl_output,
		int32_t factor) {
	DC *dc = data;
	dc->scale = factor;
	printf("%d\n", factor);
	/* struct swaybar_output *output = data; */
	/* output->scale = factor; */
	/* if (output == output->bar->pointer.current) { */
	/* 	update_cursor(output->bar); */
	/* 	render_frame(output); */
	/* } */
}

struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};
static void xdg_output_handle_logical_position(void *data,
		struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y) {
	// Who cares
}

static void xdg_output_handle_logical_size(void *data,
		struct zxdg_output_v1 *xdg_output, int32_t width, int32_t height) {
	// Who cares
	DC *dc = data;
	printf("logical size: %dx%d\n\n\n\n", width, height);
	dc->logical_width = width;
	dc->logical_height = height;
}

static void xdg_output_handle_done(void *data,
		struct zxdg_output_v1 *xdg_output) {
	/* struct swaybar_output *output = data; */
	/* struct swaybar *bar = output->bar; */

	/* assert(output->name != NULL); */
	/* if (!bar_uses_output(bar, output->name)) { */
	/* 	swaybar_output_free(output); */
	/* 	return; */
	/* } */

	/* if (wl_list_empty(&output->link)) { */
	/* 	wl_list_remove(&output->link); */
	/* 	wl_list_insert(&bar->outputs, &output->link); */

	/* 	output->surface = wl_compositor_create_surface(bar->compositor); */
	/* 	assert(output->surface); */

	/* 	determine_bar_visibility(bar, false); */
	/* } */
}

static void xdg_output_handle_name(void *data,
		struct zxdg_output_v1 *xdg_output, const char *name) {
	/* struct swaybar_output *output = data; */
	/* free(output->name); */
	/* output->name = strdup(name); */
}

static void xdg_output_handle_description(void *data,
		struct zxdg_output_v1 *xdg_output, const char *description) {
	// Who cares
}

struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_position = xdg_output_handle_logical_position,
	.logical_size = xdg_output_handle_logical_size,
	.done = xdg_output_handle_done,
	.name = xdg_output_handle_name,
	.description = xdg_output_handle_description,
};

static struct wl_seat *seat = NULL;
static struct xkb_context *xkb_context;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;

static void keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	/* char *keymap_string = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0); */
	/* xkb_keymap_unref (keymap); */
	/* keymap = xkb_keymap_new_from_string (xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS); */
	/* munmap (keymap_string, size); */
	/* close (fd); */
	/* xkb_state_unref (xkb_state); */
	/* xkb_state = xkb_state_new (keymap); */
}
static void keyboard_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
	
}
static void keyboard_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
	
}
static void keyboard_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	printf("key pressed\n");
	/* if (state == WL_KEYBOARD_KEY_STATE_PRESSED) { */
	/* 	xkb_keysym_t keysym = xkb_state_key_get_one_sym (xkb_state, key+8); */
	/* 	uint32_t utf32 = xkb_keysym_to_utf32 (keysym); */
	/* 	if (utf32) { */
	/* 		if (utf32 >= 0x21 && utf32 <= 0x7E) { */
	/* 			printf ("the key %c was pressed\n", (char)utf32); */
	/* 			/\* if (utf32 == 'q') running = 0; *\/ */
	/* 		} */
	/* 		else { */
	/* 			printf ("the key U+%04X was pressed\n", utf32); */
	/* 		} */
	/* 	} */
	/* 	else { */
	/* 		char name[64]; */
	/* 		xkb_keysym_get_name (keysym, name, 64); */
	/* 		printf ("the key %s was pressed\n", name); */
	/* 	} */
	/* } */
}
static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	/* xkb_state_update_mask (xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group); */
}
static struct wl_keyboard_listener keyboard_listener = {&keyboard_keymap, &keyboard_enter, &keyboard_leave, &keyboard_key, &keyboard_modifiers};

static void seat_capabilities (void *data, struct wl_seat *seat, uint32_t capabilities) {
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *pointer = wl_seat_get_pointer (seat);
		/* wl_pointer_add_listener (pointer, &pointer_listener, NULL); */
	}
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard *keyboard = wl_seat_get_keyboard (seat);
		wl_keyboard_add_listener (keyboard, &keyboard_listener, NULL);
	}
}
static struct wl_seat_listener seat_listener = {&seat_capabilities};

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	DC *dc = data;
	
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		dc->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener (seat, &seat_listener, NULL);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		dc->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);

	/* } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) { */
	/* 	dc->xdg = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1); */

	} else if (strcmp(interface, wl_output_interface.name) == 0) {

		dc->output = wl_registry_bind(registry, name, &wl_output_interface, 3);
		wl_output_add_listener(dc->output, &output_listener, dc);
		if (dc->xdg_output_manager != NULL) {
			dc->xdg_output = zxdg_output_manager_v1_get_xdg_output(dc->xdg_output_manager,
																   dc->output);
			zxdg_output_v1_add_listener(dc->xdg_output, &xdg_output_listener, dc);
			/* add_xdg_output(output); */
		}

	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		dc->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);

	/* } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) { */
	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		dc->xdg_output_manager = wl_registry_bind(registry, name,
			&zxdg_output_manager_v1_interface, 2);
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

	/* Second roundtrip for xdg-output. Will populate display dimensions. */
	wl_display_roundtrip(dc->dpy);

	dc->buffer = create_buffer(dc);
	/* wl_display_roundtrip(dc->dpy); */

	dc->surface = wl_compositor_create_surface(dc->compositor);
	/* struct zwlr_layer_surface_v1 *wlr_surface; */
	/* struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(dc->xdg, dc->surface); */

	/* xdg_toplevel = xdg_surface_get_toplevel(xdg_surface); */

	/* xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL); */
	/* xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL); */
	add_layer_surface(dc);


	printf("monitor width: %d\n", dc->width);

	wl_surface_set_buffer_scale(dc->surface, 2.0);
	wl_surface_commit(dc->surface);
	wl_display_roundtrip(dc->dpy);

	wl_surface_attach(dc->surface, dc->buffer, 0, 0);
	wl_surface_commit(dc->surface);


	/* dc->gc = XCreateGC(dc->dpy, DefaultRootWindow(dc->dpy), 0, NULL); */
	/* XSetLineAttributes(dc->dpy, dc->gc, 1, LineSolid, CapButt, JoinMiter); */
	/* dc->font.xfont = NULL; */
	/* dc->font.set = NULL; */
	/* dc->canvas = None; */
	printf("exiting init\n");
	return dc;
}

static void buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* struct pool_buffer *buffer = data; */
	/* buffer->busy = false; */
	printf("released buffer\n");
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release
};

static struct wl_buffer *create_buffer(DC *dc) {
	int width = dc->width;
	int height = dc->height;
	printf("w: %d, h: %d\n", width, height);
	printf("w: %d, h: %d (logical)\n", dc->logical_width, dc->logical_height);
	printf("scale factor %f\n", 2.0 / 1.75);
	printf("scale_coeff: %f\n", (double)dc->width / dc->logical_width);

	/* Oh my... */
	double factor = dc->scale / ((double)dc->width / dc->logical_width);
	width = dc->width * factor;
	printf("width: %d\n", width);
	
	/* width = 3840 * (2.0 / 1.75); */
	/* width = 2194 * 1.75; */
	/* width = 2194 * 3;  */
	height = HEIGHT * factor;



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

	wl_buffer_add_listener(buffer, &buffer_listener, dc);

	/* memset(dc->shm_data, 0x44, size); */

	cairo_surface_t *s = cairo_image_surface_create_for_data(dc->shm_data, CAIRO_FORMAT_ARGB32,
															 width, height, width * 4);
	/* cairo_surface_t *recorder = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL); */
	dc->cairo = cairo_create(s);
	cairo_set_antialias(dc->cairo, CAIRO_ANTIALIAS_BEST);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(fo, to_cairo_subpixel_order(dc->subpixel));
	cairo_set_font_options(dc->cairo, fo);
	cairo_font_options_destroy(fo);
	cairo_save(dc->cairo);
	


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
}

int
textnw(DC *dc, const char *text, size_t len) {
/* 	if(dc->font.set) { */
/* 		XRectangle r; */

/* 		XmbTextExtents(dc->font.set, text, len, NULL, &r); */
/* 		return r.width; */
/* 	} */
/* 	return XTextWidth(dc->font.xfont, text, len); */
	return 1;
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
