CC = gcc
CFLAGS =-c -Wall
LDFLAGS =
SOURCES = server.c server_functions.c
OBJECTS = ${SOURCES: .c = .o}
EXECUTABLE = server

all: ${SOURCES} ${EXECUTABLE}

${EXECUTABLE}: ${OBJECTS}
	$(CC) ${LDFLAGS} ${OBJECTS} -o $@

.c.o:
	${CC} ${CFLAGS} $< -o $@

.PHONY: clean
clean:
	rm -f *.o ${EXECUTABLE}

