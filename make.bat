@echo off

REM Usage:
REM "make" for a 32-bit compile of Precomp
REM "make 64" for a 64-bit compile of Precomp
REM "make comfort" for a 32-bit compile of Precomp Comfort
REM "make comfort 64" or "make 64 comfort" for a 64-bit compile of Precomp Comfort

REM gcc/g++ 32-bit/64-bit commands - change them according to your environment
set GCC32=gcc
set GPP32=g++
set GCC64=x86_64-w64-mingw32-gcc
set GPP64=x86_64-w64-mingw32-g++

set EXE1=precomp
set EXE2=
set DCOMFORT=
set MPARAM=-march=pentiumpro
set GCC=%GCC32%
set GPP=%GPP32%
:parse
if "%1%"=="" goto endparse
if "%1%"=="64" (
  set GCC=%GCC64%
  set GPP=%GPP64%
  set EXE2=64
  set MPARAM=-march=x86-64 -m64
)
if "%1%"=="comfort" (
  set EXE1=precomf
  set DCOMFORT=-DCOMFORT
)
SHIFT
goto parse
:endparse

set GIF=contrib\giflib\gifalloc.c contrib\giflib\gif_err.c contrib\giflib\dgif_lib_gcc.c contrib\giflib\egif_lib_gcc.c
set BZIP=contrib\bzip2\bzlib.c contrib\bzip2\blocksort.c contrib\bzip2\crctable.c contrib\bzip2\compress.c contrib\bzip2\decompress.c contrib\bzip2\huffman.c contrib\bzip2\randtable.c 
set ZLIB=contrib\zlib\adler32.c contrib\zlib\crc32.c contrib\zlib\zutil.c contrib\zlib\trees.c contrib\zlib\inftrees.c contrib\zlib\inffast.c contrib\zlib\inflate.c contrib\zlib\deflate.c
set ZLIB_O=adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o
set JPG=aricoder.o bitops.o packjpg.o
SET MP3=packmp3.o huffmp3.o
%GCC% %MPARAM% -c -O2 -s -fomit-frame-pointer -Wno-attributes %ZLIB%
pushd contrib\packjpg
%GPP% %MPARAM% -c -O3 -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer aricoder.cpp bitops.cpp packjpg.cpp
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
%GPP% %MPARAM% -c -O3 -DBUILD_LIB -Wall -pedantic -funroll-loops -ffast-math -fsched-spec-load -fomit-frame-pointer aricoder.cpp bitops.cpp huffmp3.cpp packmp3.cpp
move huffmp3.o ..\..\ > nul
move packmp3.o ..\..\ > nul
if exist aricoder.* del aricoder.*
if exist bitops.* del bitops.*
popd
%GPP% %DCOMFORT% %MPARAM% -static -static-libgcc -static-libstdc++ -lpthread -Wall precomp.cpp %JPG% %MP3% %GIF% %BZIP% %ZLIB_O% -O2 -fomit-frame-pointer -s -o%EXE1%%EXE2%.exe
if not %ERRORLEVEL% == 0 echo ERROR!!!
if %ERRORLEVEL% == 0 echo.
if %ERRORLEVEL% == 0 echo Build successful.
set ZLIB_O=
set ZLIB=
set BZIP=
set GIF=
set JPG=
set MP3=
set GPP=
set GCC=
set GPP32=
set GCC32=
set GPP64=
set GCC64=
set EXE1=
set EXE2=
set DCOMFORT=
set MPARAM=