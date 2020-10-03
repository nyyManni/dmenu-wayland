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
#include "draw.h"

#define INRECT(x,y,rx,ry,rw,rh) ((x) >= (rx) && (x) < (rx)+(rw) && (y) >= (ry) && (y) < (ry)+(rh))
#define MIN(a,b)                ((a) < (b) ? (a) : (b))
#define MAX(a,b)                ((a) > (b) ? (a) : (b))

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

const char *progname;

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
static void parse_args(int argc, char *argv[]);

static char text[BUFSIZ];
static char text_[BUFSIZ];
static int itemcount = 0;
static int lines = 0;
static int timeout = 3;
static size_t cursor = 0;
static const char *prompt = NULL;
static bool message = false;
static bool use_stdin = true;
static bool returnearly = false;
static bool bar_at_bottom = false;
static TextPosition messageposition = LEFT;
static Item *items = NULL;
static Item *matches, *sel;
static Item *prev, *curr, *next;
static Item *leftmost, *rightmost;
static char *font = "Mono";

static int (*fstrncmp)(const char *, const char *, size_t) = strncmp;

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

void keyrepeat(struct dmenu_panel *panel) {
	if (panel->on_keyevent) {
		panel->on_keyevent(panel, panel->repeat_key_state, panel->repeat_sym,
						   panel->keyboard.control, panel->keyboard.shift);
	}
}

void keypress(struct dmenu_panel *panel, enum wl_keyboard_key_state state,
			  xkb_keysym_t sym, bool ctrl, bool shft) {
	char buf[8];
	size_t len = strlen(text);

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED) return;

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
			dmenu_close(panel);
			return;
		}
	}
	switch (sym) {
	case XKB_KEY_KP_Enter: /* fallthrough */
	case XKB_KEY_Return:
		dmenu_close(panel);
		fputs((sel && !shft) ? sel->text : text, stdout);
		fflush(stdout);
		break;
	case XKB_KEY_Escape:
		retcode = EXIT_FAILURE;
		dmenu_close(panel);
		break;
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
		}
	}
	dmenu_draw(panel);
}

void cairo_set_source_u32(cairo_t *cairo, uint32_t color) {
	cairo_set_source_rgba(cairo,
			(color >> (3*8) & 0xFF) / 255.0,
			(color >> (2*8) & 0xFF) / 255.0,
			(color >> (1*8) & 0xFF) / 255.0,
			(color >> (0*8) & 0xFF) / 255.0);
}

int32_t draw_text(cairo_t *cairo, int32_t width, int32_t height, const char *str,
				  int32_t x, int32_t scale, uint32_t
				  foreground_color, uint32_t background_color, int32_t padding) {

	int32_t text_width, text_height;
	get_text_size(cairo, font, &text_width, &text_height,
				  NULL, scale, false, str);
	int32_t text_y = (height / 2.0) - (text_height / 2.0);

	if (x + padding * scale + text_width + 30 * scale > width) {
	/* if (x + padding * scale + text_width > width) { */

		cairo_move_to(cairo, width, text_y);
		pango_printf(cairo, font, scale, false, ">");
	} else {
		if (background_color) {
			cairo_set_source_u32(cairo, background_color);
			cairo_rectangle(cairo, x, 0, text_width + 2 * padding * scale, height);
			cairo_fill(cairo);
		}

		cairo_move_to(cairo, x + padding * scale, text_y);
		cairo_set_source_u32(cairo, foreground_color);

		pango_printf(cairo, font, scale, false, str);
	}

	return x + text_width + 2 * padding * scale;
}

void draw(cairo_t *cairo, int32_t width, int32_t height, int32_t scale) {

	int32_t x = window_config.input_field;

	int32_t item_padding = 10;

	int32_t text_width, text_height;
	get_text_size(cairo, font, &text_width, &text_height, NULL, scale,
				  false, "Aj");
	int32_t text_y = (height / 2.0) - (text_height / 2.0);

	cairo_set_source_u32(cairo, color_bg);
	cairo_paint(cairo);

	if (prompt) {
		x = draw_text(cairo, width, height, prompt, 0, scale, color_prompt_fg,
					  color_prompt_bg, 6);
		window_config.input_field = x;
	} else {
		window_config.input_field = 0;
	}

	cairo_set_source_u32(cairo, color_input_bg);
	cairo_rectangle(cairo, window_config.input_field, 0, 300 * scale, height);
	cairo_fill(cairo);

	draw_text(cairo, width, height, text, x, scale, color_input_fg, 0, 6);

	{
		/* draw cursor */
		memset(text_, 0, BUFSIZ);
		strncpy(text_, text, cursor);
		int32_t text_width, text_height;
		get_text_size(cairo, font, &text_width, &text_height, NULL, scale,
					  false, text_);
		/* int32_t text_y = (height / 2.0) - (text_height / 2.0); */
		int32_t padding = 6 * scale;
		cairo_rectangle(cairo, x + padding + text_width, text_y,
						scale, text_height);
		cairo_fill(cairo);
	}

	x += 300 * scale;

	/* Scroll indicator will be drawn later if required. */
	int32_t scroll_indicator_pos = x;
	x += 20 * scale;

	if (matches) {
		/* draw matches */
		Item *item;
		/* for (item = matches; item; item = item->right) { */
		/* 	if (item->width == -1) { */
		/* 		get_text_size(cairo, font, &item->width, NULL, NULL, scale, */
		/* 					  false, item->text); */
		/* 		item->width += item_padding; */
		/* 		/\* printf("%d ", item->width); *\/ */
		/* 	} */
		/* } */

		/* /\* Figure out if we need to scroll. *\/ */
		/* int32_t item_pos = x; */
		/* bool found = false; */
		/* rightmost = NULL; */
		/* for (item = leftmost; item; item = item->right) { */
		/* 	item_pos += item->width; */
		/* 	if (item_pos >= (width - x - 80 * scale)) { */
		/* 		rightmost = item->left; */
		/* 		printf("rightmost: %s\n", item->left->text); */
		/* 		found = true; */
		/* 		break; */
		/* 	} */
		/* } */

		for (item = matches; item; item = item->right) {
			uint32_t bg_color = sel == item ? color_selected_bg : color_bg;
			uint32_t fg_color = sel == item ? color_selected_fg : color_fg;
			if (x < width) {
				/* x = draw_text(cairo, width - 20 * scale, height, item->text, */
				/* 			  x, scale, fg_color, bg_color, item_padding); */
				x = draw_text(cairo, width - 20 * scale, height, item->text,
							  x, scale, fg_color, bg_color, item_padding);
			} else {
				break;
			}
		}

		if (leftmost != matches) {
			cairo_move_to(cairo, scroll_indicator_pos, text_y);
			pango_printf(cairo, font, scale, false, "<");
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
 
	if (message) {
		signal(SIGALRM, alarmhandler);
		alarm(timeout);
	}
	if(use_stdin) {
		readstdin();
	}

	struct dmenu_panel dmenu;
	dmenu.selected_monitor = selected_monitor;
	dmenu.selected_monitor_name = selected_monitor_name;
	dmenu_init_panel(&dmenu, panel_height, bar_at_bottom);


	dmenu.on_keyevent = keypress;
	dmenu.on_keyrepeat = keyrepeat;
	dmenu.draw = draw;
	match();

	struct monitor_info *monitor = dmenu.monitor;

	double factor = monitor->scale / ((double)monitor->physical_width / monitor->logical_width);

	window_config.height =round_to_int(dmenu.height / ((double)monitor->physical_width
												  / monitor->logical_width));
	window_config.height *= monitor->scale;

	window_config.width = round_to_int(monitor->physical_width * factor);
	get_text_size(dmenu.surface.cairo, font, NULL, &window_config.text_height,
				  NULL, monitor->scale, false, "Aj");
	window_config.text_y = (window_config.height / 2.0) - (window_config.text_height / 2.0);


	dmenu_show(&dmenu);

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

static void 
parse_args(int argc, char *argv[])
{
	int i;

	progname = "wlmenu";
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v") || !strcmp(argv[1], "--version")) {
			fputs("wlmenu-wl-" VERSION
			", Â© 2006-2018 wlmenu engineers \n see LICENSE for details\n", stdout);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bottom")){
			bar_at_bottom = true;
		} else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--echo")){
			message = true;
		} else if (!strcmp(argv[i], "-ec") || !strcmp(argv[i], "--echo-center")){
			message = true;
			messageposition = CENTER;
		} else if (!strcmp(argv[i], "-er") || !strcmp(argv[i], "--echo-right")){
			message = true;
			messageposition = RIGHT;
		} else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--insensitive")){
			fstrncmp = strncasecmp;
		} else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--return-early")){
			returnearly = true;
		} else if (!strcmp(argv[i], "--help")){
			usage(1); /* the user intentionally chose to show the help msg*/
		} else if (i == argc - 1) {
			printf("2\n");
			usage(0);
		}
		/* opts that need 1 arg */ 
		else if (!strcmp(argv[i], "-et") || !strcmp(argv[i], "--echo-timeout")) {
			timeout = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height")) {
			panel_height = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--lines")) {
			lines = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--monitor")) {
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
		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--prompt")){
			prompt = argv[++i];
		} else if (!strcmp(argv[i], "-po") || !strcmp(argv[i], "--prompt-only")){
			prompt = argv[++i], use_stdin = false;
		} else if (!strcmp(argv[i], "-fn") || !strcmp(argv[i], "--font-name")){
			font = argv[++i];
		} else if (!strcmp(argv[i], "-nb") || !strcmp(argv[i], "--normal-background")){
			color_bg = color_input_bg = parse_color(argv[++i]);
		} else if (!strcmp(argv[i], "-nf") || !strcmp(argv[i], "--normal-foreground")){
			color_fg = color_input_fg = parse_color(argv[++i]);
		} else if (!strcmp(argv[i], "-sb") || !strcmp(argv[i], "--selected-background")){
			color_prompt_bg = color_selected_bg = parse_color(argv[++i]);
		} else if (!strcmp(argv[i], "-sf") || !strcmp(argv[i], "--selected-foreground")){
			color_prompt_fg = color_selected_fg = parse_color(argv[++i]);
		} else {
			usage(0);
		}
	}	
}


void
usage(int status) {
	printf("Usage: dmenu [OPTION]...\n");
	printf("Display newline-separated input stdin as a menubar\n");
	printf("\n");
	printf("       --help                       display this help text\n");
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

	exit(status);
}
