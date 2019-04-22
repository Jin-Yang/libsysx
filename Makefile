
CFLAGS = -g -Wall

all: src/psutil.o src/pscgrp.o
	gcc -shared -fPIC -o libsysx.so $^
	ar crv libsysx.a $^

%.o: %.c
	gcc ${CFLAGS} -fPIC -o $@ $< -c

clean:
	rm -rf *.o libsysx*

