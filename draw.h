/* See LICENSE file for copyright and license details. */
#include <stdbool.h>
#include <wayland-client.h>
#include <cairo/cairo.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <xkbcommon/xkbcommon.h>

#define MAX_MONITOR_NAME_LEN 255

#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

struct dmenu_panel;

struct monitor_info {
	int32_t physical_width;
	int32_t physical_height;
	int32_t logical_width;
	int32_t logical_height;
	double scale;
	
	char name[MAX_MONITOR_NAME_LEN];

	enum wl_output_subpixel subpixel;

	struct zxdg_output_v1 *xdg_output;

	struct wl_output *output;
	struct dmenu_panel *panel;
};

extern struct monitor_info *monitors[];

struct display_info {
	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct wl_display * display;
	struct wl_compositor *compositor;
	struct wl_seat *seat;

};

struct keyboard_info {
	struct wl_keyboard *kbd;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	bool control;
	bool shift;
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
	/* struct monitor_info monitor; */
	int selected_monitor;
	char *selected_monitor_name;
	
	struct monitor_info *monitor;
	struct display_info display_info;

	struct surface surface;

	void (*on_keyevent)(struct dmenu_panel *,enum wl_keyboard_key_state,
						xkb_keysym_t, bool, bool);
	void (*on_keyrepeat)(struct dmenu_panel *);

	void (*draw)(cairo_t *, int32_t, int32_t, int32_t);

	int32_t width;
	int32_t height;

	int repeat_timer;
	int repeat_delay;
	int repeat_period;
	enum wl_keyboard_key_state repeat_key_state;
	xkb_keysym_t repeat_sym;

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
int32_t round_to_int(double val);

extern const char *progname;
