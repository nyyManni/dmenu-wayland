# dmenu - dynamic menu
# See LICENSE file for copyright and license details.

include config.mk

all: options dmenu dmenu_path

options:
	@echo dmenu build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

dmenu: dmenu.o draw.o xdg-shell-protocol.o shm.o wlr-layer-shell-unstable-v1-protocol.o
dmenu_path: dmenu_path.o

.c.o: config.mk
	@echo CC -c $<
	echo ${CC} -c $< ${CFLAGS}
	@${CC} -c $< ${CFLAGS}

dmenu dmenu_path:
	@echo CC -o $@
	echo ${CC} -o $@ $+ ${LDFLAGS}
	@${CC} -o $@ $+ ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f dmenu dmenu.o draw.o dmenu_path dmenu_path.o dmenu-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p dmenu-${VERSION}
	@cp LICENSE Makefile README config.mk dmenu.1 dmenu.c draw.c draw.h dmenu_path.c dmenu_run dmenu-${VERSION}
	@tar -cf dmenu-${VERSION}.tar dmenu-${VERSION}
	@gzip dmenu-${VERSION}.tar
	@rm -rf dmenu-${VERSION}

install: all
	@echo installing executables to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dmenu dmenu_path dmenu_run ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu_path
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dmenu_run
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < dmenu.1 > ${DESTDIR}${MANPREFIX}/man1/dmenu.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/dmenu.1

uninstall:
	@echo removing executables from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu_path
	@rm -f ${DESTDIR}${PREFIX}/bin/dmenu_run
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/dmenu.1

.PHONY: all options clean dist install uninstall
