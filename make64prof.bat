@echo off

set GIF=contrib\giflib\gifalloc.c contrib\giflib\gif_err.c contrib\giflib\dgif_lib_gcc.c contrib\giflib\egif_lib_gcc.c
set BZIP=contrib\bzip2\bzlib.c contrib\bzip2\blocksort.c contrib\bzip2\crctable.c contrib\bzip2\compress.c contrib\bzip2\decompress.c contrib\bzip2\huffman.c contrib\bzip2\randtable.c 
set ZLIB=contrib\zlib\adler32.c contrib\zlib\crc32.c contrib\zlib\zutil.c contrib\zlib\trees.c contrib\zlib\inftrees.c contrib\zlib\inffast.c contrib\zlib\inflate.c contrib\zlib\deflate.c
set ZLIB_O=adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o
set JPG=aricoder.o bitops.o packjpg.o
SET MP3=packmp3.o huffmp3.o
gcc -c -O2 -pg -march=x86-64 -m64 -Wno-attributes %ZLIB%
pushd contrib\packjpg
g++ -c -O3 -pg -DBUILD_LIB -Wall -march=x86-64 -m64 -pedantic -funroll-loops -ffast-math -fsched-spec-load aricoder.cpp bitops.cpp packjpg.cpp
if exist ..\..\aricoder.o del ..\..\aricoder.o
if exist ..\..\bitops.o del ..\..\bitops.o
if exist ..\..\packjpg.o del ..\..\packjpg.o
move aricoder.o ..\..\ > nul
move bitops.o ..\..\ > nul
move packjpg.o ..\..\ > nul
popd
pushd contrib\packmp3
copy ..\packjpg\aricoder.* > nul
copy ..\packjpg\bitops.* > nul
g++ -c -O3 -pg -DBUILD_LIB -Wall -march=x86-64 -m64 -pedantic -funroll-loops -ffast-math -fsched-spec-load aricoder.cpp bitops.cpp huffmp3.cpp packmp3.cpp
move huffmp3.o ..\..\ > nul
move packmp3.o ..\..\ > nul
if exist aricoder.* del aricoder.*
if exist bitops.* del bitops.*
popd
g++ -static -static-libgcc -static-libstdc++ -Wall -pg precomp.cpp %JPG% %MP3% %GIF% %BZIP% %ZLIB_O% -O2 -march=x86-64 -m64 -oprecomp64_prof.exe
if not %ERRORLEVEL% == 0 echo ERROR!!!
if %ERRORLEVEL% == 0 echo.
if %ERRORLEVEL% == 0 echo Build successful.
set ZLIB_O=
set ZLIB=
set BZIP=
set GIF=
set JPG=

REM To analyze a profile, run "gprof -b precomp64_prof.exe gmon.out"