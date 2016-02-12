OBJECTS = bitops.o aricoder.o packjpg.o
CFLAGS  = -O3 -DUNIX -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer

.PHONY: all
all: $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o

%.o: %.cpp
	g++ -c $(CFLAGS) $<
