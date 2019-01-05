# dmenu version
VERSION = 4.2.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/cairo
LIBS = -lwayland-client -lrt -lcairo -lpango-1.0 -lglib-2.0 -lgobject-2.0 -lpangocairo-1.0 -lxkbcommon

# flags
CPPFLAGS = -D_BSD_SOURCE -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pedantic -Wall -O0 -g ${INCS} ${CPPFLAGS}
LDFLAGS  = -s ${LIBS} 

# compiler and linker
CC = cc

