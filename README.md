# dmenu-wl - dynamic menu
dmenu-wl is an efficient dynamic menu for wayland (wlroots).

## TODO
Missing dmenu (for X) features:
- Echo (non-interactive use)
- Handling pagination
- Vertical layout
- Return-early
- Control-Enter handling
- Keyrepeat
- Choice (-c) -mode

Other TODO items:
- Cleaner exiting
- Generate protocol files with `wayland-scanner`

## Requirements
Requires a compositor which implements wlr-layer-shell and xdg-output
protocols. Basically this means a wlroots-based compositor is needed.
Gnome and KDE are therefore [not supported](https://github.com/nyyManni/dmenu-wayland/issues/16).
Tested with sway 1.0.

Required libraries (and headers):
- wayland-client
- cairo
- pango-1.0
- pangocairo-1.0
- xkbcommon
- glib-2.0
- gobject-2.0


## Installation
```
    mkdir build
    meson build
    ninja -C build
    sudo ninja -C build install
```

## Running dmenu-wl ...

### ... as the application launcher in Sway

Add to sway configuration (`~/.config/sway/config`) to run the launcher on Win+D.

    bindsym $mod+d exec dmenu-wl_run -i
    
### ... from the command-line

See the man page for details.
```
Usage: dmenu-wl [OPTION]...

Display newline-separated input stdin as a menubar

    -e,  --echo                       display text from stdin with no user
                                      interaction
    -ec, --echo-centre                same as -e but align text centrally
    -er, --echo-right                 same as -e but align text right
    -et, --echo-timeout SECS          close the message after SEC seconds
                                      when using -e, -ec, or -er
    -b,  --bottom                     dmenu appears at the bottom of the screen
    -h,  --height N                   set dmenu to be N pixels high
    -i,  --insensitive                dmenu matches menu items case insensitively
    -l,  --lines LINES                dmenu lists items vertically, within the
                                      given number of lines
    -m,  --monitor MONITOR            dmenu appears on the given monitor
                                      (0-based index or monitor name)
    -p,  --prompt  PROMPT             prompt to be displayed to the left of the
                                      input field
    -po, --prompt-only  PROMPT        same as -p but don't wait for stdin
                                      useful for a prompt with no menu
    -r,  --return-early               return as soon as a single match is found
    -fn, --font-name FONT             font or font set to be used
    -nb, --normal-background COLOR    normal background color
                                      #RRGGBB and #RRGGBBAA supported
    -nf, --normal-foreground COLOR    normal foreground color
    -sb, --selected-background COLOR  selected background color
    -sf, --selected-foreground COLOR  selected foreground color
    -v,  --version                    display version information
```
