CC = gcc
CFLAGS = -Wall -lrt -lpthread -pthread
PROGRAMS = sender receiverI

ALL: ${PROGRAMS}

sender: sender.c
	${CC} ${CFLAGS} -o sender header.c sender.c

receiverI: receiverI.c
	${CC} ${CFLAGS} -o receiverI header.c receiverI.c

header: header.c
	${CC} ${CFLAGS} -o header header.c

clean:
	rm -f ${PROGRAMS}
