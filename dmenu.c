/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <fontconfig/fontconfig.h>

#include "wlterm.h"

#define INRECT(x,y,rx,ry,rw,rh) ((x) >= (rx) && (x) < (rx)+(rw) && (y) >= (ry) && (y) < (ry)+(rh))
#define MIN(a,b)                ((a) < (b) ? (a) : (b))
#define MAX(a,b)                ((a) > (b) ? (a) : (b))
void draw();

extern msdfgl_font_t active_font;

const char *progname;
void eprintf(const char *fmt, ...) {
    va_list ap;

    fprintf(stderr, "%s: ", progname);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

typedef struct Item Item;
struct Item {
    char *text;
    Item *next;          /* traverses all items */
    Item *left, *right;  /* traverses matching items */
    int32_t width;
};

typedef enum {
    LEFT,
    RIGHT,
    CENTRE
} TextPosition;

struct {
    int32_t width;
    int32_t height;
    int32_t text_height;
    int32_t text_y;
    int32_t input_field;
    int32_t scroll_left;
    int32_t matches;
    int32_t scroll_right;
} window_config;


static uint32_t color_bg = 0x222222ff;
static uint32_t color_fg = 0xbbbbbbff;
static uint32_t color_input_bg = 0x222222ff;
static uint32_t color_input_fg = 0xbbbbbbff;
static uint32_t color_prompt_bg = 0x005577ff;
static uint32_t color_prompt_fg = 0xeeeeeeff;
static uint32_t color_selected_bg = 0x005577ff;
static uint32_t color_selected_fg = 0xeeeeeeff;

static int32_t panel_height = 20;

static void appenditem(Item *item, Item **list, Item **last);
static char *fstrstr(const char *s, const char *sub);
static void insert(const char *s, ssize_t n);
static void match(void);
static size_t nextrune(int incr);
static void readstdin(void);
static void alarmhandler(int signum);
/* static void handle_return(char* value); */
static void usage(void);
static int retcode = EXIT_SUCCESS;
static int selected_monitor = 0;
static char *selected_monitor_name = 0;

static char text[BUFSIZ];
static char text_[BUFSIZ];
static int itemcount = 0;
static int lines = 0;
static int timeout = 3;
static size_t cursor = 0;
static const char *prompt = NULL;
static bool message = false;
static bool nostdin = false;
static bool returnearly = false;
static bool show_in_bottom = false;
static TextPosition messageposition = LEFT;
static Item *items = NULL;
static Item *matches, *sel;
static Item *prev, *curr, *next;
static Item *leftmost, *rightmost;
static char *font = "Mono";

static int (*fstrncmp)(const char *, const char *, size_t) = strncmp;



FcChar8 *find_font(char *query) {
    FcPattern *pat;
    FcChar8 *s = NULL;

    pat = query ? FcNameParse((FcChar8 *) query) : FcPatternCreate();

    if (!pat) {
      fprintf(stderr, "Unable to parse the pattern\n");
      return NULL;
    }

    FcConfigSubstitute(0, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcFontSet *fs = FcFontSetCreate();

    FcResult result;
    FcPattern *match = FcFontMatch(0, pat, &result);
    if (match)
        FcFontSetAdd(fs, match);
    FcPatternDestroy(pat);
    if (fs && fs->nfont) {
        FcPattern *font = FcPatternFilter(fs->fonts[0], 0);
        s = FcPatternFormat(font, (FcChar8 *)"%{file}");
        FcPatternDestroy(font);
    }
    FcFontSetDestroy(fs);
    FcFini();

    return s;
}

void
insert(const char *s, ssize_t n) {
    if(strlen(text) + n > sizeof text - 1)
        return;
    memmove(text + cursor + n, text + cursor, sizeof text - cursor - MAX(n, 0));
    if(n > 0)
        memcpy(text + cursor, s, n);
    cursor += n;
    match();
}

void keypress(struct wlterm_frame *f, enum wl_keyboard_key_state state,
              xkb_keysym_t sym, bool ctrl, bool shft) {
    if (state != WL_KEYBOARD_KEY_STATE_PRESSED) return;

    char buf[8];
    size_t len = strlen(text);


    if (ctrl) {
        switch (xkb_keysym_to_lower(sym)) {
        case XKB_KEY_a:
            sym = XKB_KEY_Home;
            break;
        case XKB_KEY_e:
            sym = XKB_KEY_End;
            break;
        case XKB_KEY_f:
            sym = XKB_KEY_Right;
            break;
        case XKB_KEY_b:
            sym = XKB_KEY_Left;
            break;
        case XKB_KEY_h:
            sym = XKB_KEY_BackSpace;
            break;
        case XKB_KEY_j:
            sym = XKB_KEY_Return;
            break;
        case XKB_KEY_g:
        case XKB_KEY_c:
            retcode = EXIT_FAILURE;
            wlterm_frame_destroy(f);
            return;
        }
    }
    switch (sym) {
    case XKB_KEY_KP_Enter: /* fallthrough */
    case XKB_KEY_Return:
        wlterm_frame_destroy(f);
        fputs((sel && !shft) ? sel->text : text, stdout);
        fflush(stdout);
        return;
    case XKB_KEY_Escape:
        retcode = EXIT_FAILURE;
        wlterm_frame_destroy(f);
        return;
    case XKB_KEY_Left:
        if(cursor && (!sel || !sel->left)) {
            cursor = nextrune(-1);
        } if (sel && sel->left) {
            sel = sel->left;
        }
        break;
    case XKB_KEY_Right:
        if (cursor < len) {
            cursor = nextrune(+1);
        } else if (cursor == len) {
            if (sel && sel->right) sel = sel->right;
        }
        break;

    case XKB_KEY_End:
        if(cursor < len) {
            cursor = len;
            break;
        }
        while(sel && sel->right)
            sel = sel->right;
        break;
    case XKB_KEY_Home:
        if(sel == matches) {
            cursor = 0;
            break;
        }
        sel = curr = matches;
        /* calcoffsets(); */
        break;

    case XKB_KEY_BackSpace:
        if (cursor > 0)
            insert(NULL, nextrune(-1) - cursor);
        break;
    case XKB_KEY_Delete:
        if (cursor == len)
            return;
        cursor = nextrune(+1);
        break;
    case XKB_KEY_Tab:
        if(!sel) return;
        strncpy(text, sel->text, sizeof text);
        cursor = strlen(text);
        match();
        break;
    default:
        if (xkb_keysym_to_utf8(sym, buf, 8)) {
            insert(buf, strnlen(buf, 8));
        } else {
            return;
        }
    }
    wlterm_frame_render(f);
}

double font_size = 10.0;

void draw(struct wlterm_window *w, int32_t width, int32_t height) {
    int32_t padding = 12;

    float line_height = msdfgl_vertical_advance(active_font, font_size);
    int32_t x = 0;
    int32_t y = (height / 2.0) + (line_height / 2.0) - 4;

    vec3 color_bg, color_fg;
    parse_color_gl("093748", color_fg);
    parse_color_gl("0c1014", color_bg);
    static bool prompt_done = false;
    static int32_t var_x = 0;

    if (prompt && !prompt_done) {
        glClearColor(color_fg[0], color_fg[1], color_fg[2], 0.8);
        glClear(GL_COLOR_BUFFER_BIT);

        x = padding;
        x = msdfgl_printf(x, y, active_font, font_size, 0xd3ebe9ff,
                        (GLfloat *)w->projection, MSDFGL_KERNING | MSDFGL_UTF8,
                        prompt);
        prompt_done = true;
        x += padding;
        var_x = x;
    } else {
        x = var_x;
    }

    set_region(w->frame, x, 0, width - x, height);
    x += padding;

    glClearColor(color_bg[0], color_bg[1], color_bg[2], 0.8);
    glClear(GL_COLOR_BUFFER_BIT);

    set_region(w->frame, x, 0, 300, height);
    memset(text_, 0, BUFSIZ);
    strncpy(text_, text, cursor);
    int32_t x_ = msdfgl_printf(x, y, active_font, font_size, 0xd3ebe9ff,
                               (GLfloat *)w->projection, MSDFGL_KERNING | MSDFGL_UTF8,
                               text_);
    msdfgl_printf(x_, y, active_font, font_size, 0xd3ebe9ff,
                  (GLfloat *)w->projection, MSDFGL_KERNING | MSDFGL_UTF8,
                  &text[cursor]);
    msdfgl_printf(x_, y, active_font, font_size, 0xd3ebe9ff,
                  (GLfloat *)w->projection, MSDFGL_KERNING | MSDFGL_UTF8,
                  "\u2588");
    char c[2];
    c[0] = text[cursor];
    c[1] = 0;
    if (c[0])
        msdfgl_printf(x_, y, active_font, font_size, 0x000000ff,
                      (GLfloat *)w->projection, MSDFGL_KERNING | MSDFGL_UTF8, c);



    x += padding;

    x += 300;
    set_region(w->frame, x, 0, width - x, height);

    if (matches) {
        /* draw matches */
        Item *item;
        for (item = matches; item; item = item->right) {
            if (x > width)
                break;
            if (sel == item) {
                set_region(w->frame, x, 0, width - x, height);
                glClearColor(color_fg[0], color_fg[1], color_fg[2], 0.8);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            x += padding;
            x = msdfgl_printf(x, y, active_font, font_size,  0xd3ebe9ff,
                              (GLfloat *)w->projection,
                              MSDFGL_KERNING | MSDFGL_UTF8, item->text);
            x += padding;
            if (sel == item) {
                set_region(w->frame, x, 0, width - x, height);
                glClearColor(color_bg[0], color_bg[1], color_bg[2], 0.8);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }
    }
}

uint32_t parse_color(char *str) {
    if (!str) eprintf("NULL as color value\n");

    size_t len = strnlen(str, BUFSIZ);

    if ((len != 7 && len != 9) || str[0] != '#')
        eprintf("Color format must be '#rrggbb[aa]'\n");

    uint32_t _val = strtol(&str[1], NULL, 16);

    uint32_t color = 0x000000ff;
    if (len == 9) /* Alpha specified */
        color = _val;
    else /* No alpha specified, assume full opacity */
        color = (_val << 8) + 0xff;

    return color;
}

int
main(int argc, char **argv) {
  int i;

  progname = "dmenu";
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-v") || !strcmp(argv[1], "--version")) {
      fputs("dmenu-wl-" VERSION
            ", © 2006-2018 dmenu engineers, see LICENSE for details\n",
            stdout);
      exit(EXIT_SUCCESS);
    } else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bottom"))
      show_in_bottom = true;
    else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--echo"))
      message = true;
    else if (!strcmp(argv[i], "-ec") || !strcmp(argv[i], "--echo-centre"))
      message = true, messageposition = CENTRE;
    else if (!strcmp(argv[i], "-er") || !strcmp(argv[i], "--echo-right"))
      message = true, messageposition = RIGHT;
    else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--insensitive"))
      fstrncmp = strncasecmp;
    else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--return-early"))
      returnearly = true;
    else if (i == argc - 1) {
      printf("2\n");
      usage();

    }
    /* opts that need 1 arg */
    else if (!strcmp(argv[i], "-et") || !strcmp(argv[i], "--echo-timeout"))
      timeout = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height"))
      panel_height = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--lines"))
      lines = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--monitor")) {
        ++i;
        bool is_num = true;
        for (int j = 0; j < strlen(argv[i]); ++j) {
            if (!isdigit(argv[i][j])) {
                is_num = false;
                break;
            }
        }
        if (is_num) {
            selected_monitor = atoi(argv[i]);
        } else {
            selected_monitor = -1;
            selected_monitor_name = argv[i];
        }
    }
    else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--prompt"))
      prompt = argv[++i];
    else if (!strcmp(argv[i], "-po") || !strcmp(argv[i], "--prompt-only"))
      prompt = argv[++i], nostdin = true;
    else if (!strcmp(argv[i], "-fn") || !strcmp(argv[i], "--font-name"))
      font = argv[++i];
    else if (!strcmp(argv[i], "-nb") || !strcmp(argv[i], "--normal-background"))
      color_bg = color_input_bg = parse_color(argv[++i]);
    else if (!strcmp(argv[i], "-nf") || !strcmp(argv[i], "--normal-foreground"))
      color_fg = color_input_fg = parse_color(argv[++i]);
    else if (!strcmp(argv[i], "-sb") ||
             !strcmp(argv[i], "--selected-background"))
      color_prompt_bg = color_selected_bg = parse_color(argv[++i]);
    else if (!strcmp(argv[i], "-sf") ||
             !strcmp(argv[i], "--selected-foreground"))
      color_prompt_fg = color_selected_fg = parse_color(argv[++i]);
    else {
      usage();
    }
  }

    if (message) {
        signal(SIGALRM, alarmhandler);
        alarm(timeout);
    }
    if(!nostdin) {
        readstdin();
    }

    struct wlterm_application *app = wlterm_application_create();
    struct wlterm_frame *f = wlterm_frame_create(app, WLTERM_PANEL);

    /* window_config.height */


    match();
	FcChar8 *font_file = find_font(font);
    wlterm_load_font(app, (char *)font_file);
	FcStrFree(font_file);
    app->on_redisplay = draw;
    wlterm_frame_render(f);
    app->on_keyevent = keypress;
    /* wl_surface_commit(f->surface); */
    /* struct wl_callback *callback = wl_surface_frame(f->surface); */
    /* wl_callback_add_listener(callback, &frame_listener, f); */

    /* struct dmenu_panel dmenu; */
    /* dmenu.selected_monitor = selected_monitor; */
    /* dmenu.selected_monitor_name = selected_monitor_name; */
    /* dmenu_init_panel(&dmenu, panel_height, show_in_bottom); */



    wlterm_application_run(app);
    /* struct monitor_info *monitor = dmenu.monitor; */

    /* double factor = monitor->scale / ((double)monitor->physical_width / monitor->logical_width); */

    /* window_config.height =round_to_int(dmenu.height / ((double)monitor->physical_width */
    /*                                                / monitor->logical_width)); */
    /* window_config.height *= monitor->scale; */

    /* window_config.width = round_to_int(monitor->physical_width * factor); */
    /* get_text_size(dmenu.surface.cairo, font, NULL, &window_config.text_height, */
    /*                NULL, monitor->scale, false, "Aj"); */
    /* window_config.text_y = (window_config.height / 2.0) - (window_config.text_height / 2.0); */


    /* dmenu_show(&dmenu); */

    return retcode;
}

void
appenditem(Item *item, Item **list, Item **last) {
    if(!*last)
        *list = item;
    else
        (*last)->right = item;
    item->left = *last;
    item->right = NULL;
    *last = item;
}

char *
fstrstr(const char *s, const char *sub) {
    size_t len;

    for(len = strlen(sub); *s; s++)
        if(!fstrncmp(s, sub, len))
            return (char *)s;
    return NULL;
}

void
match(void) {
    size_t len;
    Item *item, *itemend, *lexact, *lprefix, *lsubstr, *exactend, *prefixend, *substrend;

    rightmost = leftmost = NULL;
    len = strlen(text);
    matches = lexact = lprefix = lsubstr = itemend = exactend = prefixend = substrend = NULL;
    for(item = items; item; item = item->next)
        if(!fstrncmp(text, item->text, len + 1)) {
            appenditem(item, &lexact, &exactend);
        }
        else if(!fstrncmp(text, item->text, len)) {
            appenditem(item, &lprefix, &prefixend);
        }
        else if(fstrstr(item->text, text)) {
            appenditem(item, &lsubstr, &substrend);
        }

    if(lexact) {
        matches = lexact;
        itemend = exactend;
    }
    if(lprefix) {
        if(itemend) {
            itemend->right = lprefix;
            lprefix->left = itemend;
        }
        else
            matches = lprefix;
        itemend = prefixend;
    }
    if(lsubstr) {
        if(itemend) {
            itemend->right = lsubstr;
            lsubstr->left = itemend;
        }
        else
            matches = lsubstr;
    }
    curr = prev = next = sel = matches;
    /* calcoffsets(); */

    leftmost = matches;

    if(returnearly && !curr->right) {
        /* handle_return(curr->text); */
    }
}

size_t
nextrune(int incr) {
    size_t n, len;

    len = strlen(text);
    for(n = cursor + incr; n >= 0 && n < len && (text[n] & 0xc0) == 0x80; n += incr);
    return n;
}

void
readstdin(void) {
    char buf[sizeof text], *p;
    Item *item, **end;

    for(end = &items; fgets(buf, sizeof buf, stdin); *end = item, end = &item->next) {
        itemcount++;

        if((p = strchr(buf, '\n'))) {
            *p = '\0';
        }
        if(!(item = malloc(sizeof *item))) {
            eprintf("cannot malloc %u bytes\n", sizeof *item);
        }
        item->width = -1;
        if(!(item->text = strdup(buf))) {
            eprintf("cannot strdup %u bytes\n", strlen(buf)+1);
        }
        item->next = item->left = item->right = NULL;
        /* inputw = MAX(inputw, textw(dc, item->text)); */
    }
}


void
alarmhandler(int signum) {
    exit(EXIT_SUCCESS);
}

void
usage(void) {
    printf("Usage: dmenu [OPTION]...\n");
    printf("Display newline-separated input stdin as a menubar\n");
    printf("\n");
    printf("  -e,  --echo                       display text from stdin with no user\n");
    printf("                                      interaction\n");
    printf("  -ec, --echo-centre                same as -e but align text centrally\n");
    printf("  -er, --echo-right                 same as -e but align text right\n");
    printf("  -et, --echo-timeout SECS          close the message after SEC seconds\n");
    printf("                                      when using -e, -ec, or -er\n");
    printf("  -b,  --bottom                     dmenu appears at the bottom of the screen\n");
    printf("  -h,  --height N                   set dmenu to be N pixels high\n");
    printf("  -i,  --insensitive                dmenu matches menu items case insensitively\n");
    printf("  -l,  --lines LINES                dmenu lists items vertically, within the\n");
    printf("                                      given number of lines\n");
    printf("  -m,  --monitor MONITOR            dmenu appears on the given Xinerama screen\n");
    printf("                                      (does nothing on wayland, supported for)\n");
    printf("                                      compatibility with dmenu.\n");
    printf("  -p,  --prompt  PROMPT             prompt to be displayed to the left of the\n");
    printf("                                      input field\n");
    printf("  -po, --prompt-only  PROMPT        same as -p but don't wait for stdin\n");
    printf("                                      useful for a prompt with no menu\n");
    printf("  -r,  --return-early               return as soon as a single match is found\n");
    printf("  -fn, --font-name FONT             font or font set to be used\n");
    printf("  -nb, --normal-background COLOR    normal background color\n");
    printf("                                      #RRGGBB and #RRGGBBAA supported\n");
    printf("  -nf, --normal-foreground COLOR    normal foreground color\n");
    printf("  -sb, --selected-background COLOR  selected background color\n");
    printf("  -sf, --selected-foreground COLOR  selected foreground color\n");
    printf("  -v,  --version                    display version information\n");

    exit(EXIT_FAILURE);
}
