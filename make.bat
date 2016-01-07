@echo off

set GIF=gifalloc.c gif_err.c dgif_lib_gcc.c egif_lib_gcc.c
set BZIP=bzlib.c blocksort.c crctable.c compress.c decompress.c huffman.c randtable.c 
set ZLIB=adler32.c crc32.c zutil.c trees.c inftrees.c inffast.c inflate.c deflate.c
set ZLIB_O=adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o
set JPG=aricoder.o bitops.o packjpg.o
gcc -c -O2 -s -fomit-frame-pointer -march=pentiumpro -Wno-attributes %ZLIB%
pushd contrib\packjpg
g++ -c -O3 -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer aricoder.cpp bitops.cpp packjpg.cpp
if exist ..\..\aricoder.o del ..\..\aricoder.o
if exist ..\..\bitops.o del ..\..\bitops.o
if exist ..\..\packjpg.o del ..\..\packjpg.o
move aricoder.o ..\..\
move bitops.o ..\..\
move packjpg.o ..\..\
popd
g++ -static -static-libgcc -static-libstdc++ -lpthread -Wall precomp.cpp %JPG% %GIF% %BZIP% %ZLIB_O% -O2 -march=pentiumpro -fomit-frame-pointer -s -oprecomp.exe
if not %ERRORLEVEL% == 0 echo FEHLER!!!
if %ERRORLEVEL% == 0 echo.
if %ERRORLEVEL% == 0 echo Kompilierung erfolgreich.
set ZLIB_O=
set ZLIB=
set BZIP=
set GIF=
set JPG=
