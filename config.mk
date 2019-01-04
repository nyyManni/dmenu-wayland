# dmenu version
VERSION = 4.2.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Xinerama, comment if you don't want it
#XINERAMALIBS  = -lXinerama
#XINERAMAFLAGS = -DXINERAMA

# includes and libs
INCS = 
LIBS = -lwayland-client -lrt -lwlroots

# flags
CPPFLAGS = -D_BSD_SOURCE -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
CFLAGS   = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS  = -s ${LIBS} -Wl,--no-undefined -Wl,--as-needed -Wl,--start-group

# compiler and linker
CC = cc

