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
/* #include <X11/Xlib.h> */
/* #include <X11/Xatom.h> */
/* #include <X11/Xutil.h> */
/* #ifdef XINERAMA */
/* #include <X11/extensions/Xinerama.h> */
/* #endif */
#include "draw.h"



#define INRECT(x,y,rx,ry,rw,rh) ((x) >= (rx) && (x) < (rx)+(rw) && (y) >= (ry) && (y) < (ry)+(rh))
#define MIN(a,b)                ((a) < (b) ? (a) : (b))
#define MAX(a,b)                ((a) > (b) ? (a) : (b))

#define HEIGHT 50 

typedef struct Item Item;
struct Item {
	char *text;
	Item *next;          /* traverses all items */
	Item *left, *right;  /* traverses matching items */
};

typedef enum {
    LEFT,
    RIGHT,
    CENTRE
} TextPosition;

static uint32_t color_bg = 0x222222ff;
static uint32_t color_fg = 0xbbbbbbff;
static uint32_t color_prompt_bg = 0x222222ff;
static uint32_t color_prompt_fg = 0xbbbbbbff;
static uint32_t color_selected_bg = 0x005577ff;
static uint32_t color_selected_fg = 0xeeeeeeff;


static void appenditem(Item *item, Item **list, Item **last);
static void calcoffsets(void);
static void drawmenu(void);
static char *fstrstr(const char *s, const char *sub);
static void grabkeyboard(void);
static void insert(const char *s, ssize_t n);
/* static void keypress(XKeyEvent *ev); */
static void match(void);
static size_t nextrune(int incr);
/* static void paste(void); */
static void readstdin(void);
/* static void run(DC *); */
static void setup(void);
static void alarmhandler(int signum);
static void handle_return(char* value);
static void usage(void);

static char text[BUFSIZ];
static char text_[BUFSIZ];
static int bh, mw, mh;
static int height = 0;
static int inputw = 0;
static int itemcount = 0;
static int lines = 0;
static int monitor = -1;
static int promptw;
static int timeout = 3;
static size_t cursor = 0;
/* static const char *font = NULL; */
static const char *prompt = NULL;
static const char *normbgcolor = "#222222";
static const char *normfgcolor = "#bbbbbb";
static const char *selbgcolor  = "#005577";
static const char *selfgcolor  = "#eeeeee";
static unsigned long normcol[ColLast];
static unsigned long selcol[ColLast];
/* static Atom utf8; */
static bool message = false;
static bool nostdin = false;
static bool returnearly = false;
static bool topbar = true;
static TextPosition messageposition = LEFT;
/* static DC *dc; */
static Item *items = NULL;
static Item *matches, *sel;
static Item *prev, *curr, *next;
/* static Window root, win; */
char *font = "DejaVu Sans Mono";

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

void keypress(struct dmenu_panel *panel, enum wl_keyboard_key_state state,
			  xkb_keysym_t sym, bool ctrl) {
	char buf[8];
	size_t len = strlen(text);

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED) return;

	if (ctrl) {
		switch (xkb_keysym_to_lower(sym)) {
		case XKB_KEY_f:
			sym = XKB_KEY_Right;
			break;
		case XKB_KEY_b:
			sym = XKB_KEY_Left;
			break;
		case XKB_KEY_h:
			sym = XKB_KEY_BackSpace;
			break;
		case XKB_KEY_g:
		case XKB_KEY_c:
			if (ctrl) {
				dmenu_close(panel);
			}
			return;
		}
	}
	switch (sym) {
	case XKB_KEY_KP_Enter: /* fallthrough */
	case XKB_KEY_Return:
    /*     handle_return((sel && !(ev->state & ShiftMask)) ? sel->text : text); */
		dmenu_close(panel);
		if (sel)
			printf("%s", sel->text);
		break;
	case XKB_KEY_Escape:
		dmenu_close(panel);
		break;
		/* case XKB_KEY_Shift_L: */
		/* case XKB_KEY_Shift_R: */
		/* case XKB_KEY_Control_L: */
		/* case XKB_KEY_Control_R: */
		/* case XKB_KEY_Meta_L: */
		/* case XKB_KEY_Meta_R: */
		/* case XKB_KEY_Alt_L: */
		/* case XKB_KEY_Alt_R: */
		/* case XKB_KEY_Super_L: */
		/* case XKB_KEY_Super_R: */
		/* 	// Ignore modifiers here. */
		/* 	break; */
	case XKB_KEY_Left:
		if(cursor && (!sel || !sel->left || lines > 0)) {
			cursor = nextrune(-1);
		} else if (!cursor) {
			if (sel->left) sel = sel->left;
		}

		/* else if(lines > 0) */
		/* 	return; */
		break;
	case XKB_KEY_Right:
		if (cursor < len) {
			cursor = nextrune(+1);
		} else if (cursor == len) {
			if (sel->right) sel = sel->right;
		}
		/* else if(lines > 0) */
		/* 	return; */
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
	/* printf("%s\n", text); */

	/* char buf[32]; */
	/* size_t len; */
	/* KeySym ksym; */

	/* len = strlen(text); */
	/* XLookupString(ev, buf, sizeof buf, &ksym, NULL); */
	/* if(ev->state & ControlMask) { */
	/* 	switch(tolower(ksym)) { */
	/* 	default: */
	/* 		return; */
	/* 	case XK_a: */
	/* 		ksym = XK_Home; */
	/* 		break; */
	/* 	case XK_b: */
	/* 		ksym = XK_Left; */
	/* 		break; */
	/* 	case XK_c: */
	/* 		ksym = XK_Escape; */
	/* 		break; */
	/* 	case XK_d: */
	/* 		ksym = XK_Delete; */
	/* 		break; */
	/* 	case XK_e: */
	/* 		ksym = XK_End; */
	/* 		break; */
	/* 	case XK_f: */
	/* 		ksym = XK_Right; */
	/* 		break; */
	/* 	case XK_h: */
	/* 		ksym = XK_BackSpace; */
	/* 		break; */
	/* 	case XK_i: */
	/* 		ksym = XK_Tab; */
	/* 		break; */
	/* 	case XK_j: */
	/* 		ksym = XK_Return; */
	/* 		break; */
	/* 	case XK_k:  /\* delete right *\/ */
	/* 		text[cursor] = '\0'; */
	/* 		match(); */
	/* 		break; */
	/* 	case XK_n: */
	/* 		ksym = XK_Down; */
	/* 		break; */
	/* 	case XK_p: */
	/* 		ksym = XK_Up; */
	/* 		break; */
	/* 	case XK_u:  /\* delete left *\/ */
	/* 		insert(NULL, 0 - cursor); */
	/* 		break; */
	/* 	case XK_w:  /\* delete word *\/ */
	/* 		while(cursor > 0 && text[nextrune(-1)] == ' ') */
	/* 			insert(NULL, nextrune(-1) - cursor); */
	/* 		while(cursor > 0 && text[nextrune(-1)] != ' ') */
	/* 			insert(NULL, nextrune(-1) - cursor); */
	/* 		break; */
	/* 	case XK_y:  /\* paste selection *\/ */
	/* 		XConvertSelection(dc->dpy, XA_PRIMARY, utf8, utf8, win, CurrentTime); */
	/* 		return; */
	/* 	} */
	/* } */
	/* switch(ksym) { */
	/* case XK_End: */
	/* 	if(cursor < len) { */
	/* 		cursor = len; */
	/* 		break; */
	/* 	} */
	/* 	while(next) { */
	/* 		sel = curr = next; */
	/* 		calcoffsets(); */
	/* 	} */
	/* 	while(sel && sel->right) */
	/* 		sel = sel->right; */
	/* 	break; */
	/* case XK_Home: */
	/* 	if(sel == matches) { */
	/* 		cursor = 0; */
	/* 		break; */
	/* 	} */
	/* 	sel = curr = matches; */
	/* 	calcoffsets(); */
	/* 	break; */
	/* case XK_Up: */
	/* 	if(sel && sel->left && (sel = sel->left)->right == curr) { */
	/* 		curr = prev; */
	/* 		calcoffsets(); */
	/* 	} */
	/* 	break; */
	/* case XK_Next: */
	/* 	if(!next) */
	/* 		return; */
	/* 	sel = curr = next; */
	/* 	calcoffsets(); */
	/* 	break; */
	/* case XK_Prior: */
	/* 	if(!prev) */
	/* 		return; */
	/* 	sel = curr = prev; */
	/* 	calcoffsets(); */
	/* 	break; */
	/* case XK_Return: */
	/* case XK_KP_Enter: */
    /*     handle_return((sel && !(ev->state & ShiftMask)) ? sel->text : text); */
	/* case XK_Down: */
	/* 	if(sel && sel->right && (sel = sel->right) == next) { */
	/* 		curr = next; */
	/* 		calcoffsets(); */
	/* 	} */
	/* 	break; */
	/* } */
	/* drawmenu(); */

	/* swaylock_handle_key(state, sym, codepoint); */
	dmenu_draw(panel);
}

void cairo_set_source_u32(cairo_t *cairo, uint32_t color) {
	cairo_set_source_rgba(cairo,
			(color >> (3*8) & 0xFF) / 255.0,
			(color >> (2*8) & 0xFF) / 255.0,
			(color >> (1*8) & 0xFF) / 255.0,
			(color >> (0*8) & 0xFF) / 255.0);
}

int32_t draw_text(cairo_t *cairo, int32_t width, int32_t height, char *str,
				  int32_t x, int32_t scale, uint32_t
				  foreground_color, uint32_t background_color) {
	
	int32_t text_width, text_height;
	get_text_size(cairo, font, &text_width, &text_height,
				  NULL, scale, false, str);
	int32_t text_y = (height / 2.0) - (text_height / 2.0);
	int32_t padding = text_y * scale;

	if (x + padding + text_width + 20 * scale > width) {

		cairo_move_to(cairo, width - 20 * scale, text_y);
		pango_printf(cairo, font, scale, false, ">");

		return width;
	}

	if (background_color) {
		cairo_set_source_u32(cairo, background_color);
		cairo_rectangle(cairo, x, 0, text_width + 2 * padding, height);
		cairo_fill(cairo);
	}

	cairo_move_to(cairo, x + padding, text_y);
	cairo_set_source_u32(cairo, foreground_color);

	pango_printf(cairo, font, scale, false, str);

	return x + text_width + 2 * padding;
}

void draw(cairo_t *cairo, int32_t width, int32_t height, int32_t scale) {
	
	int32_t x = 0;

	cairo_set_source_u32(cairo, color_bg);
	cairo_paint(cairo);

	if (prompt)
		x = draw_text(cairo, width, height, prompt, x, scale, color_prompt_fg,
					  color_prompt_bg);

	cairo_set_source_u32(cairo, color_prompt_bg);
	cairo_rectangle(cairo, x, 0, 400, height);
	cairo_fill(cairo);

	draw_text(cairo, width, height, text, x, scale, color_prompt_fg, 0);


	{
		/* draw cursor */
		memset(text_, 0, BUFSIZ);
		strncpy(text_, text, cursor);
		int32_t text_width, text_height;
		get_text_size(cairo, font, &text_width, &text_height, NULL, scale,
					  false, text_);
		int32_t text_y = (height / 2.0) - (text_height / 2.0);
		int32_t padding = text_y * scale;
		cairo_rectangle(cairo, x + padding + text_width, text_y,
						scale, text_height);
		cairo_fill(cairo);
	}

	x += 400;

	if (matches) {
		/* draw matches */

		Item *item;
		for (item = matches; item; item = item->right) {
			uint32_t bg_color = sel == item ? color_selected_bg : color_bg;
			uint32_t fg_color = sel == item ? color_selected_fg : color_fg;
			/* uint32_t bg_color = curr == item ? 0x333333ff : 0x000000ff; */
			/* uint32_t fg_color = curr == item ? 0x333333ff : 0x000000ff; */
			if (x < width)
				x = draw_text(cairo, width, height, item->text, x, scale,
							  fg_color, bg_color);
		}


		/* for (item = curr; item != next; item = item->right) { */
		/* 	printf("%s\n", item->text); */
		/* } */
	}

        /* } else if(matches) { */
        /*     dc->x += dc->w; */
        /*     if(curr->left) { */
        /*         dc->w = textw(dc, "<"); */
        /*         drawtext(dc, "<", normcol); */
        /*         dc->x += dc->w; */
        /*     } */
        /*     for(item = curr; item != next; item = item->right) { */
        /*         dc->w = MIN(textw(dc, item->text), mw - dc->x - textw(dc, ">")); */
        /*         drawtext(dc, item->text, (item == sel) ? selcol : normcol); */
        /*         dc->x += dc->w; */
        /*     } */
        /*     dc->w = textw(dc, ">"); */
        /*     dc->x = mw - dc->w; */
        /*     if(next) { */
        /*         drawtext(dc, ">", normcol); */
        /*     } */
        /* /\* printf("%d\n", width); *\/ */

	/* cairo_set_source_rgba(cairo, 1.0, 0.0, 0.0, 0.5); */
	/* cairo_rectangle(cairo, 0, 0, text_width + 2 * padding, height); */
	/* /\* cairo_rectangle(cairo, width - 300, 0, 300, height); *\/ */
	/* cairo_fill(cairo); */

	/* cairo_move_to(cairo, padding, text_y); */
	/* cairo_set_source_rgba(cairo, 0.0, 1.0, 0.0, 0.5); */

	/* pango_printf(cairo, "DejaVu Sans Mono", scale, false, prompt); */

	/* cairo_move_to(cairo, width / 2, height - 1); */
	/* cairo_line_to(cairo, width / 2 + 30, height -1); */
	/* cairo_stroke(cairo); */
}

int
main(int argc, char *argv[]) {
	int i;

	progname = "dmenu";
	for(i = 1; i < argc; i++)
		/* single flags */
		if(!strcmp(argv[i], "-v") || !strcmp(argv[1], "--version")) {
			fputs("dmenu-"VERSION", © 2006-2011 dmenu engineers, see LICENSE for details\n", stdout);
			exit(EXIT_SUCCESS);
		}
		else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bottom"))
			topbar = false;
		else if(!strcmp(argv[i], "-e") || !strcmp(argv[i], "--echo"))
            message = true;
		else if(!strcmp(argv[i], "-ec") || !strcmp(argv[i], "--echo-centre"))
            message = true, messageposition = CENTRE;
		else if(!strcmp(argv[i], "-er") || !strcmp(argv[i], "--echo-right"))
            message = true, messageposition = RIGHT;
		else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--insensitive"))
			fstrncmp = strncasecmp;
		else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--return-early"))
			returnearly = true;
        else if(i==argc-1)
            usage();
        /* opts that need 1 arg */
		else if(!strcmp(argv[i], "-et") || !strcmp(argv[i], "--echo-timeout"))
			timeout = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--height"))
			height = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-l") || !strcmp(argv[i], "--lines"))
			lines = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--monitor"))
			monitor = atoi(argv[++i]);
		else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--prompt"))
			prompt = argv[++i];
		else if(!strcmp(argv[i], "-po") || !strcmp(argv[i], "--prompt-only"))
			prompt = argv[++i], nostdin = true;
		else if(!strcmp(argv[i], "-fn") || !strcmp(argv[i], "--font-name"))
			font = argv[++i];
		else if(!strcmp(argv[i], "-nb") || !strcmp(argv[i], "--normal-background"))
			normbgcolor = argv[++i];
		else if(!strcmp(argv[i], "-nf") || !strcmp(argv[i], "--normal-foreground"))
			normfgcolor = argv[++i];
		else if(!strcmp(argv[i], "-sb") || !strcmp(argv[i], "--selected-background"))
			selbgcolor = argv[++i];
		else if(!strcmp(argv[i], "-sf") || !strcmp(argv[i], "--selected-foreground"))
			selfgcolor = argv[++i];
		else
			usage();

    if(message) {
        signal(SIGALRM, alarmhandler);
        alarm(timeout);
    }

	struct dmenu_panel dmenu;
	dmenu_init_panel(&dmenu, HEIGHT);

	dmenu.on_keyevent = keypress;
	dmenu.draw = draw;
	/* dc = initdc(HEIGHT); */
	/* initfont(dc, font); */
    if(!nostdin) {
        readstdin();
    }
	/* setup(); */
    /* run(dc); */
	match();

	/* blocks */
	dmenu_show(&dmenu);

	/* dmenu_draw(&dmenu); */
	/* draw(dc); */


	return EXIT_FAILURE;  /* should not reach */
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

/* void */
/* calcoffsets(void) { */
/* 	unsigned int i, n; */

/* 	if(lines > 0) */
/* 		n = lines * bh; */
/* 	else */
/* 		n = mw - (promptw + inputw + textw(dc, "<") + textw(dc, ">")); */

/* 	for(i = 0, next = curr; next; next = next->right) */
/* 		if((i += (lines > 0) ? bh : MIN(textw(dc, next->text), n)) > n) */
/* 			break; */
/* 	for(i = 0, prev = curr; prev && prev->left; prev = prev->left) */
/* 		if((i += (lines > 0) ? bh : MIN(textw(dc, prev->left->text), n)) > n) */
/* 			break; */
/* } */

void
drawmenu(void) {
	int curpos;
	Item *item;

	/* dc->x = 0; */
	/* dc->y = 0; */
    /* dc->w = 0; */
	/* dc->h = height; */

    /* dc->text_offset_y = 0; */

    /* if(mh < height) { */
    /*     dc->text_offset_y = (height - mh) / 2; */
    /* } */

    /* drawrect(dc, 0, 0, mw, height, true, BG(dc, normcol)); */

	/* if(message) { */
    /*     if(messageposition == RIGHT || messageposition == CENTRE) { // Find starting position aligned text */
    /*         dc->x = mw; */
    /*         for(item = curr; item != next; item = item->right) { */
    /*             dc->x -= textw(dc, item->text); */
    /*         } */
    /*     } */
    /*     if(messageposition == CENTRE) { */
    /*         dc->x /= 2; */
    /*     } */
    /*     for(item = curr; item != next; item = item->right) { */
    /*         dc->w = textw(dc, item->text); */
    /*         drawtext(dc, item->text, normcol); */
    /*         dc->x += dc->w; */
    /*     } */
	/* } */
    /* else { */





        /* if(prompt) { */
        /*     dc->w = promptw; */
        /*     drawtext(dc, prompt, selcol); */
        /*     dc->x = dc->w; */
        /* } */
        /* dc->w = (lines > 0 || !matches) ? mw - dc->x : inputw; */
        /* drawtext(dc, text, normcol); */
        /* if((curpos = textnw(dc, text, cursor) + mh/2 - 2) < dc->w) */
        /*     /\* drawrect(dc, curpos, 2 + dc->text_offset_y, 1, mh - 4, true, FG(dc, normcol)); *\/ */
        /*   ; */

        /* if(lines > 0) { */
        /*     dc->w = mw - dc->x; */
        /*     for(item = curr; item != next; item = item->right) { */
        /*         dc->y += dc->h; */
        /*         drawtext(dc, item->text, (item == sel) ? selcol : normcol); */
        /*     } */
        /* } else if(matches) { */
        /*     dc->x += dc->w; */
        /*     if(curr->left) { */
        /*         dc->w = textw(dc, "<"); */
        /*         drawtext(dc, "<", normcol); */
        /*         dc->x += dc->w; */
        /*     } */
        /*     for(item = curr; item != next; item = item->right) { */
        /*         dc->w = MIN(textw(dc, item->text), mw - dc->x - textw(dc, ">")); */
        /*         drawtext(dc, item->text, (item == sel) ? selcol : normcol); */
        /*         dc->x += dc->w; */
        /*     } */
        /*     dc->w = textw(dc, ">"); */
        /*     dc->x = mw - dc->w; */
        /*     if(next) { */
        /*         drawtext(dc, ">", normcol); */
        /*     } */
        /* } */




    /* } */
	/* mapdc(dc, win, mw, height); */
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
grabkeyboard(void) {
	int i;

    /* if(!message) { */
    /*     for(i = 0; i < 1000; i++) { */
    /*         if(!XGrabKeyboard(dc->dpy, root, true, GrabModeAsync, GrabModeAsync, CurrentTime)) */
    /*             return; */
    /*         usleep(1000); */
    /*     } */
    /*     eprintf("cannot grab keyboard\n"); */
    /* } */
}



void
match(void) {
	size_t len;
	Item *item, *itemend, *lexact, *lprefix, *lsubstr, *exactend, *prefixend, *substrend;

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

    if(returnearly && !curr->right) {
        handle_return(curr->text);
    }
}

size_t
nextrune(int incr) {
	size_t n, len;

	len = strlen(text);
	for(n = cursor + incr; n >= 0 && n < len && (text[n] & 0xc0) == 0x80; n += incr);
	return n;
}

/* void */
/* paste(void) { */
/* 	char *p, *q; */
/* 	int di; */
/* 	unsigned long dl; */
/* 	Atom da; */

/* 	XGetWindowProperty(dc->dpy, win, utf8, 0, (sizeof text / 4) + 1, false, */
/* 	                   utf8, &da, &di, &dl, &dl, (unsigned char **)&p); */
/* 	insert(p, (q = strchr(p, '\n')) ? q-p : strlen(p)); */
/* 	XFree(p); */
/* 	drawmenu(); */
/* } */

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
		if(!(item->text = strdup(buf))) {
			eprintf("cannot strdup %u bytes\n", strlen(buf)+1);
        }
		item->next = item->left = item->right = NULL;
		/* inputw = MAX(inputw, textw(dc, item->text)); */
	}
}


void pango_printf(cairo_t *cairo, const char *font,
				  double scale, bool markup, const char *fmt, ...);

/* void */
/* run(DC *dc) { */
  
/* 	/\* int loop = 0; *\/ */
/* 	/\* printf("loop %d\n", loop++); *\/ */

/* 	/\* cairo_set_line_width(dc->cairo, 1.0); *\/ */
/* 	/\* cairo_move_to(dc->cairo, 100, 10); *\/ */
/* 	/\* cairo_line_to(dc->cairo, 1000, 10); *\/ */
/* 	/\* cairo_stroke(dc->cairo); *\/ */

/* 	/\* pango_printf(dc->cairo, "DejaVu Sans Mono", 1.75, false, "%d", loop); *\/ */
/* 	/\* wl_surface_commit(dc->surface); *\/ */
/* 	/\* draw(dc); *\/ */

/* 	dc->running = true; */
/* 	while (wl_display_dispatch(dc->dpy) != -1 && dc->running) { */
/* 	  /\* printf("loop %d\n", loop++); *\/ */
/* 		// This space intentionally left blank */
/*   } */
/* 	XEvent ev; */

/* 	while(!XNextEvent(dc->dpy, &ev)) */
/* 		switch(ev.type) { */
/* 		case Expose: */
/* 			if(ev.xexpose.count == 0) */
/* 				drawmenu(); */
/* 			break; */
/* 		case KeyPress: */
/* 			keypress(&ev.xkey); */
/* 			break; */
/* 		case SelectionNotify: */
/* 			if(ev.xselection.property == utf8) */
/* 				paste(); */
/* 			break; */
/* 		case VisibilityNotify: */
/* 			if(ev.xvisibility.state != VisibilityUnobscured) */
/* 				XRaiseWindow(dc->dpy, win); */
/* 			break; */
/* 		} */
/* } */

/* void */
/* setup(void) { */
/*   return; */
/* 	int x, y, screen; */
/* 	/\* XSetWindowAttributes wa; *\/ */
/* /\* #ifdef XINERAMA *\/ */
/* /\* 	int n; *\/ */
/* /\* 	XineramaScreenInfo *info; *\/ */
/* /\* #endif *\/ */

/* 	/\* screen = DefaultScreen(dc->dpy); *\/ */
/* 	/\* root = RootWindow(dc->dpy, screen); *\/ */
/* 	/\* utf8 = XInternAtom(dc->dpy, "UTF8_STRING", false); *\/ */

/* 	/\* normcol[ColBG] = getcolor(dc, normbgcolor); *\/ */
/* 	/\* normcol[ColFG] = getcolor(dc, normfgcolor); *\/ */
/* 	/\* selcol[ColBG] = getcolor(dc, selbgcolor); *\/ */
/* 	/\* selcol[ColFG] = getcolor(dc, selfgcolor); *\/ */

/* 	/\* menu geometry *\/ */
/* 	bh = dc->font.height + 2; */
/* 	lines = MAX(lines, 0); */
/* 	mh = (MAX(MIN(lines + 1, itemcount), 1)) * bh; */

/*     if(height < mh) { */
/*         height = mh; */
/*     } */
/* #ifdef XINERAMA */
/* 	/\* if((info = XineramaQueryScreens(dc->dpy, &n))) { *\/ */
/* 	/\* 	int i, di; *\/ */
/* 	/\* 	unsigned int du; *\/ */
/* 	/\* 	Window dw; *\/ */

/* 	/\* 	XQueryPointer(dc->dpy, root, &dw, &dw, &x, &y, &di, &di, &du); *\/ */
/* 	/\* 	for(i = 0; i < n; i++) *\/ */
/* 	/\* 		if((monitor == info[i].screen_number) *\/ */
/* 	/\* 		|| (monitor < 0 && INRECT(x, y, info[i].x_org, info[i].y_org, info[i].width, info[i].height))) *\/ */
/* 	/\* 			break; *\/ */
/* 	/\* 	x = info[i].x_org; *\/ */
/* 	/\* 	y = info[i].y_org + (topbar ? 0 : info[i].height - height); *\/ */
/* 	/\* 	mw = info[i].width; *\/ */
/* 	/\* 	XFree(info); *\/ */
/* 	/\* } *\/ */
/* 	/\* else *\/ */
/* #endif */
/* 	{ */
/* 		/\* x = 0; *\/ */
/* 		/\* y = topbar ? 0 : DisplayHeight(dc->dpy, screen) - height; *\/ */
/* 		/\* mw = DisplayWidth(dc->dpy, screen); *\/ */
/* 	} */
/* 	/\* menu window *\/ */
/* 	/\* wa.override_redirect = true; *\/ */
/* 	/\* wa.background_pixmap = ParentRelative; *\/ */
/* 	/\* wa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask; *\/ */
/* 	/\* win = XCreateWindow(dc->dpy, root, x, y, mw, height, 0, *\/ */
/* 	/\*                     DefaultDepth(dc->dpy, screen), CopyFromParent, *\/ */
/* 	/\*                     DefaultVisual(dc->dpy, screen), *\/ */
/* 	/\*                     CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa); *\/ */

/* 	grabkeyboard(); */
/* 	resizedc(dc, mw, height); */
/* 	inputw = MIN(inputw, mw/3); */
/* 	promptw = prompt ? textw(dc, prompt) : 0; */
/* 	/\* XMapRaised(dc->dpy, win); *\/ */
/* 	text[0] = '\0'; */
/* 	match(); */
/* } */

void handle_return(char* value) {
    fputs(value, stdout);
    fflush(stdout);
    exit(EXIT_SUCCESS);
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
    printf("  -p,  --prompt  PROMPT             prompt to be displayed to the left of the\n");
    printf("                                      input field\n");
    printf("  -po, --prompt-only  PROMPT        same as -p but don't wait for stdin\n");
    printf("                                      useful for a prompt with no menu\n");
    printf("  -r,  --return-early               return as soon as a single match is found\n");
    printf("  -fn, --font-name FONT             font or font set to be used\n");
    printf("  -nb, --normal-background COLOR    normal background color\n");
    printf("                                      #RGB, #RRGGBB, and color names supported\n");
    printf("  -nf, --normal-foreground COLOR    normal foreground color\n");
    printf("  -sb, --selected-background COLOR  selected background color\n");
    printf("  -sf, --selected-foreground COLOR  selected foreground color\n");
    printf("  -v,  --version                    display version information\n");

	exit(EXIT_FAILURE);
}
