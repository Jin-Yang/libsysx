
CFLAGS = -g -Wall

all: psutil.o pscgrp.o
	gcc -shared -fPIC -o libsysx.so $^
	ar crv libsysx.a $^

%.o: %.c
	gcc ${CFLAGS} -fPIC -o $@ $< -c

clean:
	rm -rf *.o libsysx*

