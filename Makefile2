CC = gcc
CFLAGS = -Wall -lrt -lpthread -pthread
PROGRAMS = sender receiver

ALL: ${PROGRAMS}

sender: sender.c
	${CC} ${CFLAGS} -o sender header.c sender.c

receiver: receiver.c
	${CC} ${CFLAGS} -o receiver header.c receiver.c

header: header.c
	${CC} ${CFLAGS} -o header header.c

clean:
	rm -f ${PROGRAMS}
