#ifndef EGL_UTIL_H
#define EGL_UTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <GLES3/gl32.h>
#include <stdbool.h>
#include <EGL/egl.h>

bool check_egl_extension(const char *extensions, const char *extension);

void *platform_get_egl_proc_address(const char *address);

EGLDisplay platform_get_egl_display(EGLenum platform, void *native_display,
                                           const EGLint *attrib_list);

EGLSurface platform_create_egl_surface(EGLDisplay dpy, EGLConfig config,
                                              void *native_window, const EGLint *attrib_list);

EGLBoolean platform_destroy_egl_surface(EGLDisplay display, EGLSurface surface);

char *read_file(const char *filename);
char **read_buffer_contents(const char *filename, uint32_t *);

GLuint create_shader(const char *source, GLenum shader_type);

static inline GLuint make_shader(char *filename, GLenum type) {
    
    char *src = read_file(filename);
    GLuint shader = create_shader(src, type);
    free(src);
    return shader;
}
static inline GLuint create_program(char *vertex_shader, char *fragment_shader,
                                    char *geometry_shader) {
    GLuint vert = make_shader(vertex_shader, GL_VERTEX_SHADER);
    GLuint frag = make_shader(fragment_shader, GL_FRAGMENT_SHADER);
    GLuint geo;
    if (geometry_shader)
        geo = make_shader(geometry_shader, GL_GEOMETRY_SHADER);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    if (geometry_shader)
        glAttachShader(shader, geo);
    glLinkProgram(shader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    if (geometry_shader)
        glDeleteShader(geo);

    GLint status;
    glGetProgramiv(shader, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(shader, 1000, &len, log);
        fprintf(stderr, "Error: linking:\n%*s\n", len, log);
        exit(1);
    }
    return shader;
}

static inline uint32_t timestamp() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif /* EGL_UTIL_H */
