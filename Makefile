# dmenu - dynamic menu
# See LICENSE file for copyright and license details.

include config.mk

all: options dmenu-wl dmenu-wl_path

options:
	@echo dmenu build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

dmenu-wl: dmenu.o draw.o xdg-shell-protocol.o wlr-layer-shell-unstable-v1-protocol.o xdg-output-unstable-v1-protocol.o
dmenu-wl_path: dmenu_path.o

.c.o: config.mk
	@echo CC -c $<
	echo ${CC} -c $< ${CFLAGS}
	@${CC} -c $< ${CFLAGS}

dmenu-wl dmenu-wl_path:
	@echo CC -o $@
	echo ${CC} -o $@ $+ ${LDFLAGS}
	@${CC} -o $@ $+ ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f dmenu-wl dmenu.o draw.o xdg-shell-protocol.o wlr-layer-shell-unstable-v1-protocol.o dmenu-wl_path dmenu_path.o dmenu-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p dmenu-${VERSION}
	@cp LICENSE Makefile README config.mk dmenu-wl.1 dmenu.c draw.c draw.h dmenu_path.c dmenu-wl_run dmenu-${VERSION}
	@tar -cf dmenu-${VERSION}.tar dmenu-${VERSION}
	@gzip dmenu-${VERSION}.tar
	@rm -rf dmenu-${VERSION}

install: all
	@echo installing executables to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dmenu-wl dmenu-wl_path dmenu-wl_run ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu-wl
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu-wl_path
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu-wl_run
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < dmenu-wl.1 > ${DESTDIR}${MANPREFIX}/man1/dmenu-wl.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/dmenu-wl.1

uninstall:
	@echo removing executables from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu-wl
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu-wl_path
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu-wl_run
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/dmenu-wl.1

.PHONY: all options clean dist install uninstall
