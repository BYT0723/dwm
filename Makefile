# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = barparse.c drw.c dwm.c util.c
OBJ = ${SRC:.c=.o}

all: dwm

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk status-ids.h

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

# status ids: status-ids.def is the single source of truth; make derives
# both the C enum header and the shell assignments from it in one step
# (the shell sources the generated .sh, so editing the def updates both
# sides with no manual sync step)
status-ids.h contrib/status-ids.sh: status-ids.def tests/gen-status-ids.sh
	./tests/gen-status-ids.sh

tests/barparse_test: tests/barparse_test.c barparse.c barparse.h status-ids.h
	${CC} -std=c99 -pedantic -Wall -Os -o $@ tests/barparse_test.c barparse.c

test: tests/barparse_test
	./tests/barparse_test
	./tests/check-status-ids.sh
	./tests/check-pill-radius.sh

# end-to-end bar test on a private Xvfb display; needs Xvfb, xterm, xdotool,
# xwd and ImageMagick (convert/compare)
smoke: dwm
	./tests/dwm-smoke.sh

clean:
	rm -f dwm ${OBJ} status-ids.h contrib/status-ids.sh dwm-${VERSION}.tar.gz tests/barparse_test

dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.h config.mk\
		barparse.h status-ids.def dwm.1 drw.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all install-ids
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1

# ship the generated shell assignments to the writer's home: copy only when
# the ids changed (an unchanged install keeps mtime and is a no-op).
# Override: make install-ids STATUS_IDS_DIR=/somewhere
STATUS_IDS_DIR ?=
install-ids: contrib/status-ids.sh
	@dst='$(STATUS_IDS_DIR)'; \
	if [ -z "$$dst" ]; then \
		home='$(HOME)'; \
		if [ "$$(id -u)" = 0 ] && [ -n "$${SUDO_USER:-}" ]; then home=$$(eval echo "~$${SUDO_USER}"); fi; \
		dst="$$home/.dwm"; \
	fi; \
	mkdir -p "$$dst"; \
	if test -L "$$dst/status-ids.sh" && test "$$dst/status-ids.sh" -ef contrib/status-ids.sh 2>/dev/null; then \
		rm -f "$$dst/status-ids.sh"; \
	fi; \
	if cmp -s contrib/status-ids.sh "$$dst/status-ids.sh" 2>/dev/null; then \
		echo 'install-ids: status ids unchanged'; \
	else \
		cp contrib/status-ids.sh "$$dst/status-ids.sh"; \
		echo "install-ids: installed $$dst/status-ids.sh (restart the status daemon)"; \
	fi

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${MANPREFIX}/man1/dwm.1
	rm ${HOME}/.dwm

.PHONY: all clean dist install install-ids uninstall test smoke
