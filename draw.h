/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <xkbcommon/xkbcommon.h>


#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

struct monitor_info {
	int32_t physical_width;
	int32_t physical_height;
	int32_t logical_width;
	int32_t logical_height;
	double scale;

	enum wl_output_subpixel subpixel;

	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct zxdg_output_v1 *xdg_output;

	struct wl_display * display;
	struct wl_compositor *compositor;
	struct wl_seat *seat;
	struct wl_output *output;
};

struct keyboard_info {
	struct wl_keyboard *kbd;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	bool control;
};

struct surface {
	cairo_t *cairo;
	struct wl_buffer *buffer;
	struct wl_surface *surface;
	struct wl_shm *shm;
	void *shm_data;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zwlr_layer_surface_v1 *layer_surface;
};

struct dmenu_panel {
	struct keyboard_info keyboard;
	struct monitor_info monitor;

	struct surface surface;

	void (*on_keyevent)(struct dmenu_panel *,enum wl_keyboard_key_state, xkb_keysym_t, bool);

	void (*draw)(cairo_t *, int32_t, int32_t, int32_t);

	int32_t width;
	int32_t height;

	bool running;
};

void dmenu_init_panel(struct dmenu_panel *, int32_t, bool);
void dmenu_draw(struct dmenu_panel *);
void dmenu_show(struct dmenu_panel *);
void dmenu_close(struct dmenu_panel *);


void pango_printf(cairo_t *cairo, const char *font,
				  double scale, bool markup, const char *fmt, ...);
void get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
				   int *baseline, double scale, bool markup, const char *fmt, ...);
void eprintf(const char *fmt, ...);
void weprintf(const char *fmt, ...);

const char *progname;
