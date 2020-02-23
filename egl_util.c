#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


bool check_egl_extension(const char *extensions, const char *extension) {
	size_t extlen = strlen(extension);
	const char *end = extensions + strlen(extensions);

	while (extensions < end) {
		size_t n = 0;

		/* Skip whitespaces, if any */
		if (*extensions == ' ') {
			extensions++;
			continue;
		}

		n = strcspn(extensions, " ");

		/* Compare strings */
		if (n == extlen && strncmp(extension, extensions, n) == 0)
			return true; /* Found */

		extensions += n;
	}

	/* Not found */
	return false;
}
void *platform_get_egl_proc_address(const char *address) {
	const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	if (extensions &&
	    (check_egl_extension(extensions, "EGL_EXT_platform_wayland") ||
	     check_egl_extension(extensions, "EGL_KHR_platform_wayland"))) {
		return (void *) eglGetProcAddress(address);
	}

	return NULL;
}
EGLDisplay platform_get_egl_display(EGLenum platform, void *native_display,
				const EGLint *attrib_list) {
	static PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display = NULL;

	if (!get_platform_display) {
		get_platform_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
            platform_get_egl_proc_address(
                "eglGetPlatformDisplayEXT");
	}

	if (get_platform_display)
		return get_platform_display(platform,
					    native_display, attrib_list);

	return eglGetDisplay((EGLNativeDisplayType) native_display);
}

EGLSurface platform_create_egl_surface(EGLDisplay dpy, EGLConfig config,
				   void *native_window, const EGLint *attrib_list) {
	static PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC
		create_platform_window = NULL;

	if (!create_platform_window) {
		create_platform_window = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)
            platform_get_egl_proc_address(
                "eglCreatePlatformWindowSurfaceEXT");
	}

	if (create_platform_window)
		return create_platform_window(dpy, config,
					      native_window,
					      attrib_list);

	return eglCreateWindowSurface(dpy, config,
				      (EGLNativeWindowType) native_window,
				      attrib_list);
}

EGLBoolean platform_destroy_egl_surface(EGLDisplay display, EGLSurface surface) {
	return eglDestroySurface(display, surface);
}

GLuint
create_shader(const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling %s: %*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		exit(1);
	}

	return shader;
}

char *read_file(const char *filename) {
    char *buffer = NULL;
    long length;
    FILE *f = fopen(filename, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length + 1);
        memset(buffer, 0, length + 1);

        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    /* fprintf(stderr, buffer); */
    return buffer;
}


char **read_buffer_contents(const char *filename, uint32_t *len) {
    char *file = read_file(filename);
    
    char *fp;
    
    /* Count the number of lines. */
    *len = 1;
    fp = &file[0] - 1;
    while (*++fp) if (*fp == '\n') (*len)++;
    
    char **lines = malloc(*len * sizeof(char *));

    char **lp = &lines[0];
    *lp = &file[0];

    fp = &file[0] - 1;
    while (*++fp) {
        if (*fp == '\n') {
            *fp = '\0';
            *++lp = fp + 1;
        }
    }

    return lines;
}

