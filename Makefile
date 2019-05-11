
CFLAGS = -g -Wall

all: src/psutil.o src/pscgrp.o src/common.o
	gcc -shared -fPIC -o libsysx.so $^
	ar crv libsysx.a $^
example: all
	gcc -o eprocess example/eprocess.c libsysx.a

%.o: %.c
	gcc ${CFLAGS} -fPIC -o $@ $< -c

clean:
	rm -rf libsysx* eprocess src/*.o

