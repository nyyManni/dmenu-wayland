#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include <cglm/mat4.h>
#include <cglm/cam.h>

#include <xkbcommon/xkbcommon.h>

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include "egl_util.h"
#include "wlterm.h"


/* double font_size = 10.0; */
/* double font_size = 10.0; */

msdfgl_font_t active_font = NULL;


int parse_color_gl(const char *color, vec3 ret) {

    char buf[3] = {0};
    for (int channel = 0; channel < 3; ++channel) {
        memcpy(buf, &color[channel * 2], 2);
        ret[channel] = strtol(buf, NULL, 16) / 255.0;
    }
    return 1;
}


static const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

const struct wl_callback_listener frame_listener;
static void frame_handle_done(void *data, struct wl_callback *callback, uint32_t time) {
    struct wlterm_frame *f = data;

    wl_callback_destroy(callback);

    if (!f->open)
        return;

    wlterm_frame_render(f);
    callback = wl_surface_frame(f->surface);
    wl_callback_add_listener(callback, &frame_listener, f);
}

const struct wl_callback_listener frame_listener = {
    .done = frame_handle_done,
};
static void handle_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                                      int32_t width, int32_t height,
                                      struct wl_array *states) {

    struct wlterm_frame *f = data;

    wlterm_frame_resize(f, width, height);
}

static void handle_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    struct wlterm_frame *f = data;
    wlterm_frame_destroy(f);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = handle_toplevel_configure,
    .close = handle_toplevel_close,
};
static void handle_xdg_buffer_configure(void *data, struct xdg_surface *xdg_surface,
                                        uint32_t serial) {
    struct wlterm_frame *f = data;

    xdg_surface_ack_configure(f->xdg_surface, serial);
}

static struct xdg_surface_listener xdg_surface_listener = {
    .configure = handle_xdg_buffer_configure};


static void pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface, wl_fixed_t sx,
                                 wl_fixed_t sy) {
}


static void pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface) {}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time,
                                  wl_fixed_t sx, wl_fixed_t sy) {

}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t serial, uint32_t time, uint32_t button,
                                  uint32_t state) {
}

static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
                                uint32_t axis, wl_fixed_t value) {
}

static void pointer_handle_frame(void *data, struct wl_pointer *wl_pointer) {
}

static void pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
                                       uint32_t axis_source) {
}

static void pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                     uint32_t time, uint32_t axis) {
}
static void pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
                                         uint32_t axis, int32_t discrete) {
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_handle_enter,
    .leave = pointer_handle_leave,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = pointer_handle_axis,
    .frame = pointer_handle_frame,
    .axis_source = pointer_handle_axis_source,
    .axis_stop = pointer_handle_axis_stop,
    .axis_discrete = pointer_handle_axis_discrete,
};

static void keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format,
                            int32_t fd, uint32_t size) {

    struct wlterm_application *app = data;
    struct xkb_context *xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        exit(1);
    }
    char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_shm == MAP_FAILED) {
        close(fd);
        exit(1);
    }
    struct xkb_keymap *xkb_keymap =
        xkb_keymap_new_from_string(xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    munmap(map_shm, size);
    close(fd);

    app->xkb_state = xkb_state_new(xkb_keymap);
}

static void keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
                           struct wl_surface *surface, struct wl_array *keys) {

    struct wlterm_application *app = data;
    app->active_frame = wl_surface_get_user_data(surface);
    // Who cares
}

static void keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
                           struct wl_surface *surface) {
    // Who cares
}
static void keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                 int32_t rate, int32_t delay) {}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                               uint32_t serial, uint32_t mods_depressed,
                               uint32_t mods_latched, uint32_t mods_locked,
                               uint32_t group) {
	/* struct dmenu_panel *panel = data; */
    struct wlterm_application *app = data;
    xkb_state_update_mask(app->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    app->control = xkb_state_mod_name_is_active(app->xkb_state, XKB_MOD_NAME_CTRL,
												XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
    app->shift = xkb_state_mod_name_is_active(app->xkb_state, XKB_MOD_NAME_SHIFT,
											  XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
}

static void keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
                         uint32_t time, uint32_t key, uint32_t _key_state) {
    struct wlterm_application *app = data;
    enum wl_keyboard_key_state key_state = _key_state;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(app->xkb_state, key + 8);

	if (app->on_keyevent)
		app->on_keyevent(app->active_frame, key_state, sym, app->control, app->shift);
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
    struct wlterm_application *app = data;
    /* struct frame *w = data; */
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        app->kbd = wl_seat_get_keyboard(app->seat);
        wl_keyboard_add_listener(app->kbd, &keyboard_listener, app);
    }
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        app->pointer = wl_seat_get_pointer(app->seat);
        wl_pointer_add_listener(app->pointer, &pointer_listener, app);
    }
}

static void seat_handle_name(void *data, struct wl_seat *wl_seat, const char *name) {
    // Who cares
}

const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = seat_handle_name,
};


static void handle_global(void *data, struct wl_registry *registry, uint32_t name,
                          const char *interface, uint32_t version) {

    struct wlterm_application *app = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        app->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(app->seat, &seat_listener, app);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        app->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		app->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {

    }
}

static void handle_global_remove(void *data, struct wl_registry *registry,
                                 uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};


int missing_glyph(msdfgl_font_t font, int32_t glyph, void *data) {
    struct wlterm_application *app = data;
    EGLContext c = eglGetCurrentContext();
    EGLSurface drw = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface rd = eglGetCurrentSurface(EGL_READ);

    // Switch to the root context, as our font textures are stored there
    eglMakeCurrent(app->gl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   app->gl_context);
    int generated = msdfgl_generate_glyph(font, glyph);

    // Restore context
    eglMakeCurrent(app->gl_display, drw, rd, c);

    return generated;
}

bool wlterm_load_font(struct wlterm_application *app, const char *font_name) {

    eglMakeCurrent(app->gl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, app->gl_context);
    app->msdfgl_ctx = msdfgl_create_context("320 es");
    msdfgl_set_dpi(app->msdfgl_ctx, 156.0, 156.0);

    /* Enable auto-generating glyphs as they are encountered for the first time. */
    msdfgl_set_missing_glyph_callback(app->msdfgl_ctx, missing_glyph, app);

    msdfgl_atlas_t atlas = msdfgl_create_atlas(app->msdfgl_ctx, 1024, 2);
    active_font = msdfgl_load_font(app->msdfgl_ctx, font_name, 4.0, 1.0, atlas);
    msdfgl_generate_ascii(active_font);

    return true;
}

void wlterm_frame_resize(struct wlterm_frame *f, int width, int height) {

    f->width = width;
    f->height = height;
    wl_egl_window_resize(f->gl_window, width * f->scale, height * f->scale, 0, 0);
    glm_ortho(0.0, f->width, f->height, 0.0, -1.0, 1.0, f->projection);

    f->root_window->width = width;
    f->root_window->height = height;
    /* f->root_window->height = height - f->minibuffer_height; */

    wl_surface_commit(f->surface);
}

static void layer_surface_configure(void *data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial, uint32_t width, uint32_t height) {
	zwlr_layer_surface_v1_ack_configure(surface, serial);
    wlterm_frame_resize((struct wlterm_frame *)data, width, height);
}

static void layer_surface_closed(void *_data,
		struct zwlr_layer_surface_v1 *surface) {
}

struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

/* int max(int a, int b) { return a > b ? a : b; } */

/* static inline void set_region(struct wlterm_frame *f, int x, int y, int w, int h) { */
/*     /\* glScissor wants the botton-left corner of the area, the origin being in */
/*      the bottom-left corner of the frame. *\/ */

/*     glScissor(max(0, x) * f->scale, max(0, f->height - y - h) * f->scale, */
/*               max(0, w) * f->scale, max(0, h) * f->scale); */
/* } */

void window_render(struct wlterm_window *w) {

    /* if (w->position[1] > 0) w->position[1] = 0; */
    /* if (w->position[0] > 0) w->position[0] = 0; */

    /* Prevent changing anything outside the window. */

    set_region(w->frame, w->x, w->y, w->width, w->height);

    /* Set projection to offset content to window location. */
    glm_ortho(-w->x, w->width + (w->frame->width - w->width - w->x),
              w->height + (w->frame->height - w->height - w->x), -w->y,
              -1.0, 1.0, w->projection);

    /* int modeline_h = msdfgl_vertical_advance(active_font, font_size); */
    /* vec3 _color; */
    /* /\* parse_color_gl("ff0000", _color); *\/ */
    /* parse_color_gl("0c1014", _color); */
    /* glClearColor(_color[0], _color[1], _color[2], 0.8); */
    /* glClear(GL_COLOR_BUFFER_BIT); */

	/* if (!active_font) return; */

	if (w->frame->application->on_redisplay)
		w->frame->application->on_redisplay(w, w->width, w->height);

    /* Modeline */
    /* draw_rect(0, w->height - modeline_h, w->width, modeline_h, "0a3749", w); */

}

void wlterm_frame_render(struct wlterm_frame *f) {

    eglMakeCurrent(f->application->gl_display, f->gl_surface, f->gl_surface, f->gl_context);
    /* eglSwapInterval(app->gl_display, 0); */

    glViewport(0, 0, f->width * f->scale, f->height * f->scale);
    glEnable(GL_BLEND);
    /* glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    /* set_region(f, 0, f->height - f->minibuffer_height, f->width, f->minibuffer_height); */
    set_region(f, 0, f->height, f->width, 0);
    /* vec3 _color; */
    /* parse_color_gl("0c1014", _color); */

    /* glClearColor(_color[0], _color[1], _color[2], 1.0); */
    /* glClear(GL_COLOR_BUFFER_BIT); */

    FOR_EACH_WINDOW (f, w) {
        window_render(w);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);

    eglSwapBuffers(f->application->gl_display, f->gl_surface);
}

struct wlterm_application *wlterm_application_create() {
    struct wlterm_application *app = malloc(sizeof (struct wlterm_application));
    if (!app) return NULL;

    app->display = wl_display_connect(NULL);
    app->layer_shell = NULL;

    app->registry = wl_display_get_registry(app->display);
    wl_registry_add_listener(app->registry, &registry_listener, app);

    wl_display_roundtrip(app->display);

    app->root_frame = NULL;
	app->on_keyevent = NULL;
	app->on_redisplay = NULL;


    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLE_BUFFERS, 1,
        /* EGL_SAMPLES, 4,  // This is for 4x MSAA. */
        EGL_NONE
    };

    app->gl_display = platform_get_egl_display(EGL_PLATFORM_WAYLAND_KHR,
                                                     app->display, NULL);
    EGLint major, minor, count, n, size, i;
    EGLConfig *configs;
    eglInitialize(app->gl_display, &major, &minor);
    eglBindAPI(EGL_OPENGL_ES_API);
    eglGetConfigs(app->gl_display, NULL, 0, &count);
    configs = calloc(count, sizeof *configs);
    eglChooseConfig(app->gl_display, config_attribs, configs, count, &n);

    for (i = 0; i < n; i++) {
        eglGetConfigAttrib(app->gl_display, configs[i], EGL_BUFFER_SIZE, &size);
        if (size == 32) {
            app->gl_conf = configs[i];
            break;
        }
    }

    free(configs);

    app->gl_context = eglCreateContext(app->gl_display, app->gl_conf,
                                             EGL_NO_CONTEXT, context_attribs);
    eglMakeCurrent(app->gl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   app->gl_context);


    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    eglSwapInterval(app->gl_display, 0);

    /* wlterm_load_font(app, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"); */


    return app;
}

void wlterm_application_destroy(struct wlterm_application *app) {
    eglTerminate(app->gl_display);
    eglReleaseThread();

    
    /**
     * PANEL
     */
	zwlr_layer_shell_v1_destroy(app->layer_shell);

    wl_registry_destroy(app->registry);
    wl_display_disconnect(app->display);
}

int wlterm_application_run(struct wlterm_application *app) {
    while (wl_display_dispatch(app->display) != -1 && app->root_frame) {}
    return 0;
}

struct wlterm_frame *wlterm_frame_create(struct wlterm_application *app,
                                         enum wlterm_frame_type type) {

    struct wlterm_frame **fp = &app->root_frame;
    struct wlterm_frame *prev = NULL;

    /* Add to the end of the linked list. */
    while (*fp) {prev = *fp; fp = &((*fp)->next);};
    struct wlterm_frame *f = *fp = malloc(sizeof(struct wlterm_frame));

    f->application = app;
    f->type = type;
    f->width = 1920;
    f->height = 32;
    f->open = true;
    f->scale = 1.0;
    f->next = NULL;
    f->prev = prev;

    f->root_window = malloc(sizeof(struct wlterm_window));
    f->root_window->width = f->width;
    f->root_window->height = f->height;
    f->root_window->frame = f;
    f->root_window->x = 0;
    f->root_window->y = 0;
    /* f->root_window->contents = NULL; */
    f->root_window->next = NULL;

    /* Share the context between frames */
    f->gl_context = eglCreateContext(app->gl_display, app->gl_conf,
                                     app->gl_context, context_attribs);

    f->surface = wl_compositor_create_surface(app->compositor);
    wl_surface_set_user_data(f->surface, f);
    wl_surface_set_buffer_scale(f->surface, f->scale);

    f->gl_window = wl_egl_window_create(f->surface, f->width, f->height);
    f->gl_surface = platform_create_egl_surface(app->gl_display,
                                                app->gl_conf,
                                                f->gl_window, NULL);

    if (f->type == WLTERM_TOPLEVEL) {
        f->xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base, f->surface);
        f->xdg_toplevel = xdg_surface_get_toplevel(f->xdg_surface);

        xdg_surface_add_listener(f->xdg_surface, &xdg_surface_listener, f);
        xdg_toplevel_add_listener(f->xdg_toplevel, &xdg_toplevel_listener, f);

        xdg_toplevel_set_title(f->xdg_toplevel, "wlterm");

    } else if (f->type == WLTERM_PANEL) {
        f->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            app->layer_shell, f->surface, NULL,
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "panel");
        zwlr_layer_surface_v1_add_listener(f->layer_surface, &layer_surface_listener, f);
        zwlr_layer_surface_v1_set_size(f->layer_surface, 0, f->height);
        zwlr_layer_surface_v1_set_anchor(f->layer_surface,
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
        zwlr_layer_surface_v1_set_keyboard_interactivity(f->layer_surface, true);
    }

    wl_surface_commit(f->surface);

    wl_display_roundtrip(app->display);

    eglMakeCurrent(app->gl_display, f->gl_surface, f->gl_surface, f->gl_context);

    glEnable(GL_SCISSOR_TEST);

    eglSwapBuffers(app->gl_display, f->gl_surface);
    /* struct wl_callback *callback = wl_surface_frame(f->surface); */
    /* wl_callback_add_listener(callback, &frame_listener, f); */

    /* wlterm_frame_render(f); */
    return f;
}

void wlterm_frame_destroy(struct wlterm_frame *f) {
    f->open = false;

    platform_destroy_egl_surface(f->application->gl_display, f->gl_surface);

    if (f->type == WLTERM_TOPLEVEL) {
        xdg_toplevel_destroy(f->xdg_toplevel);
        xdg_surface_destroy(f->xdg_surface);
    } else if (f->type == WLTERM_TOPLEVEL) {
        zwlr_layer_surface_v1_destroy(f->layer_surface);
    }

    wl_surface_destroy(f->surface);

    if (!f->prev)  /* root frame */
        f->application->root_frame = f->next;

    if (f->next)
        f->next->prev = f->prev;
    if (f->prev)
        f->prev->next = f->next;
    free(f);
}
