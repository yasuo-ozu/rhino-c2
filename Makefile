SRCS = rh_*.c
HEADERS = rh_*.h

CC = /usr/bin/gcc
CC_OPTS = 
CC_RELEASE = -DRELEASE -DNDEBUG -Ofast -march=native
CC_DEBUG = -DDEBUG -g3 -O0 -Wall -Wextra
UPX = /usr/bin/upx

.PHONY:	clean score log

rhino:	${SRCS} ${HEADERS} Makefile
	${CC} ${CC_OPTS} ${CC_DEBUG} -o rhino ${SRCS}
rhino.release:	${SRCS} ${HEADERS} Makefile
	${CC} ${CC_OPTS} ${CC_RELEASE} -o rhino.release ${SRCS}
	strip rhino.release
	@echo $( which upx && upx -9 rhino.release 2>&1 > /dev/null ) > /dev/null 
	@echo "## Compressed binary file size:" `LANG=C du -b rhino.release | sed -e 's/^\([^\s]*\)\srhino.release/\1/'`
clean:
	rm -rf *.o *.s *~ .*~ *.swp .*.swp Session.vim Debug scsc rhino rhino.release tags GPATH GRTAGS GTAGS
scsc:	scsc.c
	${CC} scsc.c -o scsc
score:	scsc ${SRCS} ${HEADERS}
	@echo "## SCSC Score:"
	@./scsc ${SRCS} ${HEADERS}
	@echo "## Char count:" `(LANG=C;wc -m ${SRCS} ${HEADERS}) | sed -n -e '/total/p'`
	@echo "## Line count:" `(LANG=C;wc -l ${SRCS} ${HEADERS}) | sed -n -e '/total/p'`
log:
	git log --pretty=format:" - %s %n   http://github.com/yasuo-ozu/rhino-c2/commit/%H" --since=10hour
debug:	rhino
	rlwrap ./rhino -d -
