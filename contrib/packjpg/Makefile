OBJECTS = bitops.o aricoder.o packjpg.o
CFLAGS  = -m64 -O3 -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fomit-frame-pointer

.PHONY: all
all: $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o

%.o: %.cpp
	g++ -c $(CFLAGS) $<
