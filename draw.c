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
#include "draw.h"
#include "shm.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"


/* #define MAX(a, b)   ((a) > (b) ? (a) : (b)) */
/* #define MIN(a, b)   ((a) < (b) ? (a) : (b)) */
#define DEFFONT     "fixed"

/* #define HEIGHT 40  */

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


void dmenu_draw(struct dmenu_panel *panel) {

	cairo_t *cairo = panel->surface.cairo;
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

	double factor = panel->monitor.scale / ((double)panel->monitor.physical_width
											/ panel->monitor.logical_width);

	int32_t width = panel->monitor.physical_width * factor;

	int32_t height = panel->height / ((double)panel->monitor.physical_width
									   / panel->monitor.logical_width);
	height *= panel->monitor.scale;

	/* TODO: Figure out why needed? */
	/* height += 1; */

	if (panel->draw) {
		panel->draw(cairo, width, height, panel->monitor.scale);
	}
	wl_surface_attach(panel->surface.surface, panel->surface.buffer, 0, 0);
	wl_surface_damage(panel->surface.surface, 0, 0, panel->monitor.logical_width, height);
	wl_surface_commit(panel->surface.surface);

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


/* void */
/* drawtext(DC *dc, const char *text, unsigned long col[ColLast]) { */
/* /\* 	char buf[256]; *\/ */
/* /\* 	size_t n, mn; *\/ */

/* /\* 	/\\* shorten text if necessary *\\/ *\/ */
/* /\* 	n = strlen(text); *\/ */
/* /\* 	for(mn = MIN(n, sizeof buf); textnw(dc, text, mn) > dc->w - dc->font.height/2; mn--) *\/ */
/* /\* 		if(mn == 0) *\/ */
/* /\* 			return; *\/ */
/* /\* 	memcpy(buf, text, mn); *\/ */
/* /\* 	if(mn < n) *\/ */
/* /\* 		for(n = MAX(mn-3, 0); n < mn; buf[n++] = '.'); *\/ */

/* /\* 	drawrect(dc, 0, 0, dc->w, dc->h, True, BG(dc, col)); *\/ */
/* /\* 	drawtextn(dc, buf, mn, col); *\/ */
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

/* void */
/* freedc(DC *dc) { */
/* 	/\* if(dc->font.set) *\/ */
/* 	/\* 	XFreeFontSet(dc->dpy, dc->font.set); *\/ */
/* 	/\* if(dc->font.xfont) *\/ */
/* 	/\* 	XFreeFont(dc->dpy, dc->font.xfont); *\/ */
/* 	/\* if(dc->canvas) *\/ */
/* 	/\* 	XFreePixmap(dc->dpy, dc->canvas); *\/ */
/* 	/\* XFreeGC(dc->dpy, dc->gc); *\/ */
/* 	/\* XCloseDisplay(dc->dpy); *\/ */
/* 	free(dc); */
/* } */

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


static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
		int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
		const char *make, const char *model, int32_t transform) {
	struct dmenu_panel *panel = data;
	panel->monitor.subpixel = subpixel;
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh) {
	struct dmenu_panel *panel = data;
	panel->monitor.physical_width = width;
	panel->monitor.physical_height = height;
}

static void output_done(void *data, struct wl_output *wl_output) {
	/* struct swaybar_output *output = data; */
	/* set_output_dirty(output); */
}

static void output_scale(void *data, struct wl_output *wl_output,
		int32_t factor) {
	struct dmenu_panel *panel = data;
	panel->monitor.scale = factor;
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
	struct dmenu_panel *panel = data;

	panel->monitor.logical_width = width;
	panel->monitor.logical_height = height;
}

static void xdg_output_handle_done(void *data,
		struct zxdg_output_v1 *xdg_output) {
	// Who cares
}

static void xdg_output_handle_name(void *data,
		struct zxdg_output_v1 *xdg_output, const char *name) {
	// Who cares
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


static void keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t format, int32_t fd, uint32_t size) {

	struct dmenu_panel *panel = data;

	panel->keyboard.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		exit(1);
	}
	char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (map_shm == MAP_FAILED) {
		close(fd);
		exit(1);
	}
	panel->keyboard.xkb_keymap = xkb_keymap_new_from_string(
			panel->keyboard.xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
	munmap(map_shm, size);
	close(fd);

	panel->keyboard.xkb_state = xkb_state_new(panel->keyboard.xkb_keymap);
}
	
static void keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
	// Who cares
}

static void keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface) {
	// Who cares
}


static void keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t _key_state) {
	struct dmenu_panel *panel = data;

	enum wl_keyboard_key_state key_state = _key_state;
	xkb_keysym_t sym = xkb_state_key_get_one_sym(panel->keyboard.xkb_state, key + 8);
	if (panel->on_keyevent)
		panel->on_keyevent(panel, key_state, sym, panel->keyboard.control);
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
		int32_t rate, int32_t delay) {
	// TODO
}

static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard,
								uint32_t serial, uint32_t mods_depressed,
								uint32_t mods_latched, uint32_t mods_locked,
								uint32_t group) {
	struct dmenu_panel *panel = data;
	xkb_state_update_mask(panel->keyboard.xkb_state,
		mods_depressed, mods_latched, mods_locked, 0, 0, group);
	panel->keyboard.control = xkb_state_mod_name_is_active(panel->keyboard.xkb_state,
		XKB_MOD_NAME_CTRL,
		XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
}
static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
		enum wl_seat_capability caps) {
	struct dmenu_panel *panel = data;
	/* if (caps & WL_SEAT_CAPABILITY_POINTER) { */
	/* 	struct wl_pointer *pointer = wl_seat_get_pointer (dc->seat); */
	/* 	/\* wl_pointer_add_listener (pointer, &pointer_listener, NULL); *\/ */
	/* } */
	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		panel->keyboard.kbd = wl_seat_get_keyboard (panel->monitor.seat);
		wl_keyboard_add_listener (panel->keyboard.kbd, &keyboard_listener, panel);
	}
}
static void seat_handle_name(void *data, struct wl_seat *wl_seat,
		const char *name) {
	// Who cares
}

const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name = seat_handle_name,
};

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct dmenu_panel *panel = data;
	
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		panel->monitor.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		panel->monitor.seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener (panel->monitor.seat, &seat_listener, panel);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		panel->surface.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);

	/* } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) { */
	/* 	dc->xdg = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1); */

	} else if (strcmp(interface, wl_output_interface.name) == 0) {

		panel->monitor.output = wl_registry_bind(registry, name, &wl_output_interface, 3);
		wl_output_add_listener(panel->monitor.output, &output_listener, panel);
		if (panel->monitor.xdg_output_manager != NULL) {
			panel->monitor.xdg_output = zxdg_output_manager_v1_get_xdg_output(panel->monitor.xdg_output_manager,
																   panel->monitor.output);
			zxdg_output_v1_add_listener(panel->monitor.xdg_output, &xdg_output_listener, panel);
			/* add_xdg_output(output); */
		}

	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		panel->surface.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);

	/* } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) { */
	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		panel->monitor.xdg_output_manager = wl_registry_bind(registry, name,
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

static void buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/* struct pool_buffer *buffer = data; */
	/* buffer->busy = false; */
	/* printf("released buffer\n"); */
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release
};

struct wl_buffer *dmenu_create_buffer(struct dmenu_panel *panel) {
	double factor = panel->monitor.scale / ((double)panel->monitor.physical_width
											/ panel->monitor.logical_width);

	int32_t width = panel->monitor.physical_width * factor;
	int32_t height = panel->height * factor;

	int stride = width * 4;
	int size = stride * height;

	int fd = create_shm_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
		return NULL;
	}

	panel->surface.shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (panel->surface.shm_data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(panel->surface.shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
		stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);


	wl_buffer_add_listener(buffer, &buffer_listener, panel);

	cairo_surface_t *s = cairo_image_surface_create_for_data(panel->surface.shm_data, CAIRO_FORMAT_ARGB32,
															 width, height, width * 4);
	/* cairo_surface_t *recorder = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL); */
	panel->surface.cairo = cairo_create(s);
	cairo_set_antialias(panel->surface.cairo, CAIRO_ANTIALIAS_BEST);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(fo, to_cairo_subpixel_order(panel->monitor.subpixel));
	cairo_set_font_options(panel->surface.cairo, fo);
	cairo_font_options_destroy(fo);
	cairo_save(panel->surface.cairo);

	return buffer;
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};


void dmenu_init_panel(struct dmenu_panel *panel, int32_t height) {
	if(!setlocale(LC_CTYPE, ""))
		weprintf("no locale support\n");

	if(!(panel->monitor.display = wl_display_connect(NULL)))
		eprintf("cannot open display\n");

	panel->height = height;
	panel->keyboard.control = false;
	panel->on_keyevent = NULL;

	struct wl_registry *registry = wl_display_get_registry(panel->monitor.display);
	wl_registry_add_listener(registry, &registry_listener, panel);

	wl_display_roundtrip(panel->monitor.display);

	/* Second roundtrip for xdg-output. Will populate display dimensions. */
	wl_display_roundtrip(panel->monitor.display);

	panel->surface.buffer = dmenu_create_buffer(panel);

	panel->surface.surface = wl_compositor_create_surface(panel->monitor.compositor);

	panel->surface.layer_surface =
		zwlr_layer_shell_v1_get_layer_surface(panel->surface.layer_shell,
											  panel->surface.surface,
											  panel->monitor.output,
											  ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
											  "panel");

	int32_t _height = panel->height / ((double)panel->monitor.physical_width
									   / panel->monitor.logical_width);

	zwlr_layer_surface_v1_set_size(panel->surface.layer_surface,
								   panel->monitor.logical_width, _height);
	zwlr_layer_surface_v1_set_anchor(panel->surface.layer_surface,
									 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
									 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);

	zwlr_layer_surface_v1_set_exclusive_zone(panel->surface.layer_surface, 10);
	zwlr_layer_surface_v1_set_keyboard_interactivity(panel->surface.layer_surface, true);

	zwlr_layer_surface_v1_add_listener(panel->surface.layer_surface,
									   &layer_surface_listener, panel);

	wl_surface_set_buffer_scale(panel->surface.surface, panel->monitor.scale);
	wl_surface_commit(panel->surface.surface);
	wl_display_roundtrip(panel->monitor.display);

	wl_surface_attach(panel->surface.surface, panel->surface.buffer, 0, 0);
	wl_surface_commit(panel->surface.surface);
}

void dmenu_show(struct dmenu_panel *dmenu) {
	dmenu_draw(dmenu);

	dmenu->running = true;
	while (wl_display_dispatch(dmenu->monitor.display) != -1 && dmenu->running) {
		// This space intentionally left blank
	}
	/* dmenu_close called */
	wl_display_disconnect(dmenu->monitor.display);

}
void dmenu_close(struct dmenu_panel *dmenu) {
	dmenu->running = false;
}

/* void */
/* mapdc(DC *dc, Window win, unsigned int w, unsigned int h) { */
/* 	XCopyArea(dc->dpy, dc->canvas, win, dc->gc, 0, 0, w, h, 0, 0); */
/* } */

/* void */
/* resizedc(DC *dc, unsigned int w, unsigned int h) { */
/* /\* 	if(dc->canvas) *\/ */
/* /\* 		XFreePixmap(dc->dpy, dc->canvas); *\/ */
/* /\* 	dc->canvas = XCreatePixmap(dc->dpy, DefaultRootWindow(dc->dpy), w, h, *\/ */
/* /\* 	                           DefaultDepth(dc->dpy, DefaultScreen(dc->dpy))); *\/ */
/* /\* 	dc->x = dc->y = 0; *\/ */
/* /\* 	dc->w = w; *\/ */
/* /\* 	dc->h = h; *\/ */
/* /\* 	dc->invert = False; *\/ */
/* } */

/* int */
/* textnw(DC *dc, const char *text, size_t len) { */
/* /\* 	if(dc->font.set) { *\/ */
/* /\* 		XRectangle r; *\/ */

/* /\* 		XmbTextExtents(dc->font.set, text, len, NULL, &r); *\/ */
/* /\* 		return r.width; *\/ */
/* /\* 	} *\/ */
/* /\* 	return XTextWidth(dc->font.xfont, text, len); *\/ */
/* 	return 1; */
/* } */

/* int */
/* textw(DC *dc, const char *text) { */
/* 	/\* return textnw(dc, text, strlen(text)) + dc->font.height; *\/ */
/* 	return 0; */
/* } */

void
weprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: warning: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
