#ifndef EGL_WINDOW_H
#define EGL_WINDOW_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdbool.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <msdfgl.h>

#include <cglm/mat4.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>


struct wlterm_window;
struct wlterm_frame;

struct wlterm_application {

    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct xdg_wm_base *xdg_wm_base;

    struct wl_seat *seat;
    /* struct wl_shm *shm; */

    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;
    struct wl_keyboard *kbd;
    struct wl_pointer *pointer;

    EGLDisplay gl_display;
    EGLConfig gl_conf;
    EGLContext gl_context;

    msdfgl_context_t msdfgl_ctx;
    struct wlterm_frame *root_frame;
    /* struct wlterm_frame *root_panel; */
    struct wlterm_frame *active_frame;

	struct zwlr_layer_shell_v1 *layer_shell;

	void (*on_keyevent)(struct wlterm_frame *, enum wl_keyboard_key_state,
						xkb_keysym_t, bool, bool);

	void (*on_redisplay)(struct wlterm_window *, int32_t, int32_t);

	bool shift;
	bool control;
};

enum wlterm_frame_type {
    WLTERM_TOPLEVEL,
    WLTERM_PANEL
};

struct wlterm_frame {
    struct wlterm_application *application;

    enum wlterm_frame_type type;
    struct wlterm_frame *next;
    struct wlterm_frame *prev;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    struct wl_buffer *buffer;
	struct zwlr_layer_surface_v1 *layer_surface;

    bool open;
    int width;
    int height;

    double scale;

    /* OpenGL */
    struct wl_egl_window *gl_window;

    EGLContext gl_context;
    EGLConfig gl_conf;
    EGLDisplay gl_display;
    EGLSurface gl_surface;

    mat4 projection;

    struct wlterm_window *root_window;

};

struct wlterm_window {
    struct wlterm_frame *frame;
    struct wlterm_window *next;

    /* Window position in frame's coordinates */
    int x;
    int y;

    int width;
    int height;

    mat4 projection;
};

static inline int max(int a, int b) { return a > b ? a : b; }
static inline void set_region(struct wlterm_frame *f, int x, int y, int w, int h) {
    /* glScissor wants the botton-left corner of the area, the origin being in
     the bottom-left corner of the frame. */

    glScissor(max(0, x) * f->scale, max(0, f->height - y - h) * f->scale,
              max(0, w) * f->scale, max(0, h) * f->scale);
}


struct wlterm_application *wlterm_application_create();
void wlterm_application_destroy(struct wlterm_application *);
int wlterm_application_run(struct wlterm_application *);
struct wlterm_frame *wlterm_frame_create(struct wlterm_application *,
                                         enum wlterm_frame_type);
void wlterm_frame_destroy(struct wlterm_frame *);
struct wlterm_window *wlterm_window_create(struct wlterm_frame *);
void wlterm_window_destroy(struct wlterm_window *);

bool wlterm_load_font(struct wlterm_application *, const char *);

#define FOR_EACH_WINDOW(frame, w) \
    for (struct wlterm_window *w = frame->root_window; w; w = w->next)

void wlterm_frame_resize(struct wlterm_frame *, int, int);
void wlterm_frame_render(struct wlterm_frame *);

int parse_color_gl(const char *color, vec3 ret);

#define WLTERM_CHECK_GLERROR \
    do {                                                             \
        GLenum err = glGetError();                                   \
        if (err) {                                                   \
            fprintf(stderr, "line %d error: %x \n", __LINE__, err);  \
        }                                                            \
    } while (0);

#endif /* EGL_WINDOW_H */
