
PROG = start

CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

${PROG}: ${PROG}.c 
	${CC} ${CFLAGS} -o ${PROG}.exe ${PROG}.c ${LIBS}
