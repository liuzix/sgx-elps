CC = gcc

hello: hello.c
	$(CC) -static hello.c -nostdlib -o hello -L../libOS -lOS

.PHONY: clean
clean:
	rm hello
