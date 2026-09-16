# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = barparse.c drw.c dwm.c util.c
OBJ = ${SRC:.c=.o}

all: dwm

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

tests/barparse_test: tests/barparse_test.c barparse.c barparse.h
	${CC} -std=c99 -pedantic -Wall -Os -o $@ tests/barparse_test.c barparse.c

test: tests/barparse_test
	./tests/barparse_test

clean:
	rm -f dwm ${OBJ} dwm-${VERSION}.tar.gz tests/barparse_test

dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.h config.mk\
		barparse.h dwm.1 drw.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${MANPREFIX}/man1/dwm.1
	rm ${HOME}/.dwm

.PHONY: all clean dist install uninstall test
