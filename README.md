dmenu - dynamic menu
====================
dmenu is an efficient dynamic menu for X.


Requirements
------------
In order to build dmenu you need the Xlib header files.


Installation
------------
Edit config.mk to match your local setup (dmenu is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install dmenu
(if necessary as root):

    make clean install


Running dmenu
-------------
See the man page for details.

Usage: dmenu [OPTION]...

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
    -m,  --monitor MONITOR            dmenu appears on the given Xinerama screen
    -p,  --prompt  PROMPT             prompt to be displayed to the left of the
                                      input field
    -po, --prompt-only  PROMPT        same as -p but don't wait for stdin
                                      useful for a prompt with no menu
    -r,  --return-early               return as soon as a single match is found
    -fn, --font-name FONT             font or font set to be used
    -nb, --normal-background COLOR    normal background color
                                      #RGB, #RRGGBB, and color names supported
    -nf, --normal-foreground COLOR    normal foreground color
    -sb, --selected-background COLOR  selected background color
    -sf, --selected-foreground COLOR  selected foreground color
    -v,  --version                    display version information
