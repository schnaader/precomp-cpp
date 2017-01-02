OBJECTS = bzlib.o blocksort.o crctable.o compress.o decompress.o huffman.o randtable.o
CFLAGS  = -DLINUX -D_FILE_OFFSET_BITS=64 -m64 -O2 -Wall

.PHONY: all
all: $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o

%.o: %.c
	gcc -g -c $(CFLAGS) $<
