rm ./precomp
gcc -c -D_FILE_OFFSET_BITS=64 -O2 adler32.c crc32.c zutil.c trees.c inftrees.c inffast.c inflate.c deflate.c
pushd include/packjpg
g++ -O3 -c -DUNIX -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer \
                  aricoder.cpp bitops.cpp packjpg.cpp
cp aricoder.o bitops.o packjpg.o ../../
popd
g++ -DLINUX -D_FILE_OFFSET_BITS=64 -Wall gifalloc.c gif_err.c dgif_lib_gcc.c egif_lib_gcc.c \
                  bzlib.c blocksort.c crctable.c compress.c decompress.c huffman.c randtable.c \
                  adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o \
                  aricoder.o bitops.o packjpg.o \
                  precomp.cpp -O2 -s -oprecomp
[ -f precomp ] && echo -e "\nKompilierung erfolgreich.\n" || echo -e "\nFehler bei der Kompilierung!\n"
