/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon.h>
#include "draw.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"


static const char overflow[] = "[buffer overflow]";
static const int max_chars = 16384;

struct monitor_info *monitors[16] = {0};
static int n_monitors = 0;

int32_t round_to_int(double val) {
	return (int32_t)(val + 0.5);
}

static void randname(char *buf) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}

static int anonymous_shm_open(void) {
	char name[] = "/dmenu-XXXXXX";
	int retries = 100;

	do {
		randname(name + strlen(name) - 6);

		--retries;
		// shm_open guarantees that O_CLOEXEC is set
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			shm_unlink(name);
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);

	return -1;
}

int create_shm_file(off_t size) {
	int fd = anonymous_shm_open();
	if (fd < 0) {
		return fd;
	}

	if (ftruncate(fd, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

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

	struct monitor_info *m = panel->monitor;
	double factor = m->scale / ((double)m->physical_width
											/ m->logical_width);

	int32_t width = round_to_int(m->physical_width * factor);
	int32_t height = panel->height * m->scale;

	if (panel->draw) {
		panel->draw(cairo, width, height, m->scale);
	}

	wl_surface_attach(panel->surface.surface, panel->surface.buffer, 0, 0);
	zwlr_layer_surface_v1_set_keyboard_interactivity(panel->surface.layer_surface, true);
	wl_surface_damage(panel->surface.surface, 0, 0, m->logical_width, panel->height);
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

void
eprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t width, uint32_t height) {
	zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void layer_surface_closed(void *_data,
		struct zwlr_layer_surface_v1 *surface) {
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};


int32_t subpixel;
int32_t physical_height;


static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
		int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
		const char *make, const char *model, int32_t transform) {
	struct monitor_info *monitor = data;
	monitor->subpixel = subpixel;
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh) {
	struct monitor_info *monitor = data;
	monitor->physical_width = width;
	monitor->physical_height = height;
}

static void output_done(void *data, struct wl_output *wl_output) {
}

static void output_scale(void *data, struct wl_output *wl_output,
		int32_t factor) {
	struct monitor_info *monitor = data;
	monitor->scale = factor;
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
	struct monitor_info *monitor = data;

	monitor->logical_width = width;
	monitor->logical_height = height;
}

static void xdg_output_handle_done(void *data,
		struct zxdg_output_v1 *xdg_output) {
	// Who cares
}

static void xdg_output_handle_name(void *data,
		struct zxdg_output_v1 *xdg_output, const char *name) {
	struct monitor_info *monitor = data;
	strncpy(monitor->name, name, MAX_MONITOR_NAME_LEN);
}

static void xdg_output_handle_description(void *data,
		struct zxdg_output_v1 *xdg_output, const char *description) {
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

static void keyboard_repeat(struct dmenu_panel *panel) {
	if (panel->on_keyrepeat) {
		panel->on_keyrepeat(panel);
	}

	struct itimerspec spec = { 0 };
	spec.it_value.tv_sec = panel->repeat_period / 1000;
	spec.it_value.tv_nsec = (panel->repeat_period % 1000) * 1000000l;
	timerfd_settime(panel->repeat_timer, 0, &spec, NULL);
}

static void keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t _key_state) {
	struct dmenu_panel *panel = data;

	enum wl_keyboard_key_state key_state = _key_state;
	xkb_keysym_t sym = xkb_state_key_get_one_sym(panel->keyboard.xkb_state, key + 8);
	if (panel->on_keyevent) {
		panel->on_keyevent(panel, key_state, sym, panel->keyboard.control,
						   panel->keyboard.shift);

		if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED && panel->repeat_period >= 0) {
			panel->repeat_key_state = key_state;
			panel->repeat_sym = sym;

			struct itimerspec spec = { 0 };
			spec.it_value.tv_sec = panel->repeat_delay / 1000;
			spec.it_value.tv_nsec = (panel->repeat_delay % 1000) * 1000000l;
			timerfd_settime(panel->repeat_timer, 0, &spec, NULL);
		} else if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED) {
			struct itimerspec spec = { 0 };
			timerfd_settime(panel->repeat_timer, 0, &spec, NULL);
		}
	}
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
		int32_t rate, int32_t delay) {
	struct dmenu_panel *panel = data;
	panel->repeat_delay = delay;
	if (rate > 0) {
		panel->repeat_period = 1000 / rate;
	} else {
		panel->repeat_period = -1;
	}
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
	panel->keyboard.shift = xkb_state_mod_name_is_active(panel->keyboard.xkb_state,
		XKB_MOD_NAME_SHIFT,
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
	if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		panel->keyboard.kbd = wl_seat_get_keyboard (panel->display_info.seat);
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

void set_monitor_xdg_output(struct dmenu_panel *panel, struct monitor_info *monitor){
	monitor->xdg_output =
		zxdg_output_manager_v1_get_xdg_output(panel->display_info.xdg_output_manager,
												monitor->output);
	zxdg_output_v1_add_listener(monitor->xdg_output, &xdg_output_listener,
								monitor);
}

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct dmenu_panel *panel = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		panel->display_info.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		panel->display_info.seat = wl_registry_bind (registry, name, &wl_seat_interface, 4);
		wl_seat_add_listener (panel->display_info.seat, &seat_listener, panel);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		panel->surface.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {

		if(n_monitors >= 16) return;

		monitors[n_monitors] = malloc(sizeof(struct monitor_info));
		monitors[n_monitors]->panel = panel;
		memset(monitors[n_monitors]->name, 0, MAX_MONITOR_NAME_LEN);
		monitors[n_monitors]->output = wl_registry_bind(registry, name, &wl_output_interface, 2);

		wl_output_add_listener(monitors[n_monitors]->output, &output_listener,
							   monitors[n_monitors]);

		if (panel->display_info.xdg_output_manager != NULL) {
			set_monitor_xdg_output(panel, monitors[n_monitors]);
		}
		n_monitors++;

	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		panel->surface.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);

	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		panel->display_info.xdg_output_manager = wl_registry_bind(registry, name,
			&zxdg_output_manager_v1_interface, 2);

		for(int m = 0; m < n_monitors; m++){
			set_monitor_xdg_output(panel, monitors[m]);
		}
	}

}

static void handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
}

static void buffer_release(void *data, struct wl_buffer *wl_buffer) {
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release
};

struct wl_buffer *dmenu_create_buffer(struct dmenu_panel *panel) {
	struct monitor_info *m = panel->monitor;
	double factor = m->scale / ((double)m->physical_width
											/ m->logical_width);

	int32_t width = round_to_int(m->physical_width * factor);
	int32_t height = round_to_int(panel->height * m->scale);

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

	cairo_surface_t *s = cairo_image_surface_create_for_data(panel->surface.shm_data,
															 CAIRO_FORMAT_ARGB32,
															 width, height, width * 4);
	panel->surface.cairo = cairo_create(s);
	cairo_set_antialias(panel->surface.cairo, CAIRO_ANTIALIAS_BEST);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(fo, to_cairo_subpixel_order(m->subpixel));
	cairo_set_font_options(panel->surface.cairo, fo);
	cairo_font_options_destroy(fo);
	cairo_save(panel->surface.cairo);

	return buffer;
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};


void dmenu_init_panel(struct dmenu_panel *panel, int32_t height, bool bottom) {
	if(!setlocale(LC_CTYPE, ""))
		weprintf("no locale support\n");

	if(!(panel->display_info.display = wl_display_connect(NULL)))
		eprintf("cannot open display\n");

	if ((panel->repeat_timer = timerfd_create(CLOCK_MONOTONIC, 0)) < 0)
		eprintf("cannot create timer fd\n");

	panel->height = height;
	panel->keyboard.control = false;
	panel->on_keyevent = NULL;

	struct wl_registry *registry = wl_display_get_registry(panel->display_info.display);
	wl_registry_add_listener(registry, &registry_listener, panel);

	wl_display_roundtrip(panel->display_info.display);

	/* Second roundtrip for xdg-output. Will populate display dimensions. */
	wl_display_roundtrip(panel->display_info.display);


	panel->surface.surface = wl_compositor_create_surface(panel->display_info.compositor);

	panel->monitor = NULL;
	if (!panel->selected_monitor_name) {
		panel->monitor = monitors[panel->selected_monitor];
	} else {
		for (int i = 0; i < n_monitors; ++i) {
			if (monitors[i] && !strncmp(panel->selected_monitor_name,
										monitors[i]->name,
										MAX_MONITOR_NAME_LEN)) {
				panel->monitor = monitors[i];
				break;
			}
		}
	}
	if (!panel->monitor) {
		if (!panel->selected_monitor_name)
			eprintf("No monitor with index %i available.\n", panel->selected_monitor);
		else
		eprintf("No monitor with name %s available.\n", panel->selected_monitor_name);
	}

	panel->surface.buffer = dmenu_create_buffer(panel);

	if (!panel->surface.layer_shell)
		eprintf("Compositor does not implement wlr-layer-shell protocol.\n");
	panel->surface.layer_surface =
		zwlr_layer_shell_v1_get_layer_surface(panel->surface.layer_shell,
											  panel->surface.surface,
											  panel->monitor->output,
											  ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
											  "panel");

	zwlr_layer_surface_v1_set_size(panel->surface.layer_surface,
								   panel->monitor->logical_width, panel->height);
	zwlr_layer_surface_v1_set_anchor(panel->surface.layer_surface,
									 ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
									 ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
									 (bottom ? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
									  : ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP));


	zwlr_layer_surface_v1_add_listener(panel->surface.layer_surface,
									   &layer_surface_listener, panel);

	wl_surface_set_buffer_scale(panel->surface.surface,
								panel->monitor->scale);
	wl_surface_commit(panel->surface.surface);
	wl_display_roundtrip(panel->display_info.display);

	zwlr_layer_surface_v1_set_keyboard_interactivity(panel->surface.layer_surface, true);

	wl_surface_attach(panel->surface.surface, panel->surface.buffer, 0, 0);
	wl_surface_commit(panel->surface.surface);
}

void dmenu_show(struct dmenu_panel *dmenu) {
	dmenu_draw(dmenu);

	zwlr_layer_surface_v1_set_keyboard_interactivity(dmenu->surface.layer_surface, true);
	wl_surface_commit(dmenu->surface.surface);

	struct pollfd fds[] = {
		{ wl_display_get_fd(dmenu->display_info.display), POLLIN },
		{ dmenu->repeat_timer, POLLIN },
	};
	const int nfds = sizeof(fds) / sizeof(*fds);

	wl_display_flush(dmenu->display_info.display);

	dmenu->running = true;
	while (dmenu->running) {
		if (wl_display_flush(dmenu->display_info.display) < 0) {
			if (errno == EAGAIN)
				continue;
			break;
		}

		if (poll(fds, nfds, -1) < 0) {
			if (errno == EAGAIN)
				continue;
			break;
		}

		if (fds[0].revents & POLLIN) {
			if (wl_display_dispatch(dmenu->display_info.display) < 0) {
				dmenu->running = false;
			}
		}

		if (fds[1].revents & POLLIN) {
			keyboard_repeat(dmenu);
		}
	}

	/* dmenu_close called */
	wl_display_disconnect(dmenu->display_info.display);

}
void dmenu_close(struct dmenu_panel *dmenu) {
	dmenu->running = false;
}


void
weprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: warning: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
