OBJECTS = gifalloc.o gif_err.o dgif_lib_gcc.o egif_lib_gcc.o
CFLAGS  = -DLINUX -D_FILE_OFFSET_BITS=64 -O2 -Wall

.PHONY: all
all: $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o

%.o: %.c
	gcc -g -c $(CFLAGS) $<
