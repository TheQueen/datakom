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

# # I am a comment, and I want to say that the variable CC will be
# # the compiler to use.
# CC = gcc
# # Hey!, I am comment number 2. I want to say that CFLAGS will be the
# # options I'll pass to the compiler.
# CFLAGS= -c -Wall -lrt -lpthread -pthread
#
# all: hello
#
# hello: main.o factorial.o hello.o
#     $(CC) main.o factorial.o hello.o -o hello
#
# main.o: main.cpp
#     $(CC) $(CFLAGS) main.cpp
#
# factorial.o: factorial.cpp
#     $(CC) $(CFLAGS) factorial.cpp
#
# hello.o: hello.cpp
#     $(CC) $(CFLAGS) hello.cpp
#
# clean:
#     rm *o hello
