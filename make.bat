@echo off

REM Usage:
REM "make" for a 32-bit compile of Precomp
REM "make 64" for a 64-bit compile of Precomp
REM "make comfort" for a 32-bit compile of Precomp Comfort
REM "make comfort 64" or "make 64 comfort" for a 64-bit compile of Precomp Comfort
REM "nocontrib" parameter to build only Precomp, can be used for a much faster build
REM   if nothing was changed in the "contrib" folder

REM gcc/g++ 32-bit/64-bit commands - change them according to your environment
set GCC32=gcc
set GPP32=g++
set GCC64=x86_64-w64-mingw32-gcc
set GPP64=x86_64-w64-mingw32-g++

set EXE1=precomp
set EXE2=
set DCOMFORT=
set DBIT=
set MPARAM=-march=pentiumpro
set GCC=%GCC32%
set GPP=%GPP32%
set NOCONTRIB=
:parse
if "%1%"=="" goto endparse
if "%1%"=="64" (
  set GCC=%GCC64%
  set GPP=%GPP64%
  set EXE2=64
  set DBIT=-DBIT64
  set MPARAM=-march=x86-64 -m64
)
if "%1%"=="comfort" (
  set EXE1=precomf
  set DCOMFORT=-DCOMFORT
)
if "%1"=="nocontrib" (
  set NOCONTRIB=1
)
SHIFT
goto parse
:endparse

set GIF_O=gifalloc.o gif_err.o dgif_lib_gcc.o egif_lib_gcc.o
set BZIP_O=bzlib.o blocksort.o crctable.o compress.o decompress.o huffman.o randtable.o
set ZLIB_O=adler32.o crc32.o zutil.o trees.o inftrees.o inffast.o inflate.o deflate.o
set JPG_O=aricoder.o bitops.o packjpg.o
SET MP3_O=packmp3.o huffmp3.o
set LIBLZMA_O=alone_decoder.o alone_encoder.o arm.o armthumb.o auto_decoder.o block_buffer_decoder.o block_buffer_encoder.o
set LIBLZMA_O=%LIBLZMA_O% block_decoder.o block_encoder.o block_header_decoder.o block_header_encoder.o block_util.o
set LIBLZMA_O=%LIBLZMA_O% check.o common.o crc32_table.o crc32_fast.o crc64_table.o crc64_fast.o
set LIBLZMA_O=%LIBLZMA_O% delta_common.o delta_decoder.o delta_encoder.o easy_buffer_encoder.o easy_decoder_memusage.o
set LIBLZMA_O=%LIBLZMA_O% easy_encoder.o easy_encoder_memusage.o easy_preset.o fastpos_table.o filter_buffer_decoder.o
set LIBLZMA_O=%LIBLZMA_O% filter_buffer_encoder.o filter_common.o filter_decoder.o filter_encoder.o filter_flags_decoder.o
set LIBLZMA_O=%LIBLZMA_O% filter_flags_encoder.o hardware_cputhreads.o hardware_physmem.o ia64.o index.o index_decoder.o
set LIBLZMA_O=%LIBLZMA_O% index_encoder.o index_hash.o lzma2_decoder.o lzma2_encoder.o lzma_decoder.o lzma_encoder.o
set LIBLZMA_O=%LIBLZMA_O% lzma_encoder_optimum_fast.o lzma_encoder_optimum_normal.o lzma_encoder_presets.o lz_decoder.o
set LIBLZMA_O=%LIBLZMA_O% lz_encoder.o lz_encoder_mf.o outqueue.o powerpc.o price_table.o sha256.o simple_coder.o
set LIBLZMA_O=%LIBLZMA_O% simple_decoder.o simple_encoder.o sparc.o stream_decoder.o stream_buffer_encoder.o stream_buffer_decoder.o
set LIBLZMA_O=%LIBLZMA_O% stream_encoder.o stream_flags_decoder.o stream_encoder_mt.o stream_flags_common.o stream_flags_encoder.o
set LIBLZMA_O=%LIBLZMA_O% tuklib_cpucores.o tuklib_physmem.o vli_decoder.o vli_encoder.o vli_size.o x86.o
set LIBLZMA_CPP=contrib\liblzma\compress_easy_mt.cpp

if "%NOCONTRIB%" == "1" goto nocontrib
set GIF=contrib\giflib\gifalloc.c contrib\giflib\gif_err.c contrib\giflib\dgif_lib_gcc.c contrib\giflib\egif_lib_gcc.c
set BZIP=contrib\bzip2\bzlib.c contrib\bzip2\blocksort.c contrib\bzip2\crctable.c contrib\bzip2\compress.c contrib\bzip2\decompress.c contrib\bzip2\huffman.c contrib\bzip2\randtable.c 
set ZLIB=contrib\zlib\adler32.c contrib\zlib\crc32.c contrib\zlib\zutil.c contrib\zlib\trees.c contrib\zlib\inftrees.c contrib\zlib\inffast.c contrib\zlib\inflate.c contrib\zlib\deflate.c
echo Building giflib...
%GCC% %MPARAM% -c -O2 -s -fomit-frame-pointer -Wall %GIF%
echo Building zlib...
%GCC% %MPARAM% -c -O2 -s -fomit-frame-pointer -Wno-attributes %ZLIB%
echo Building bzip...
%GCC% %MPARAM% -c -O2 -s -fomit-frame-pointer -Wall %BZIP%
echo Building packJPG...
pushd contrib\packjpg
%GPP% %MPARAM% -c -O3 -DBUILD_LIB -Wall -Wno-misleading-indentation -pedantic -funroll-loops -ffast-math -fomit-frame-pointer aricoder.cpp bitops.cpp packjpg.cpp
move /Y aricoder.o ..\..\ > nul
move /Y bitops.o ..\..\ > nul
move /Y packjpg.o ..\..\ > nul
popd
echo Building packMP3...
pushd contrib\packmp3
copy ..\packjpg\aricoder.* > nul
copy ..\packjpg\bitops.* > nul
%GPP% %MPARAM% -c -O3 -DBUILD_LIB -Wall -Wno-misleading-indentation -pedantic -funroll-loops -ffast-math -fomit-frame-pointer aricoder.cpp bitops.cpp huffmp3.cpp packmp3.cpp
move /Y huffmp3.o ..\..\ > nul
move /Y packmp3.o ..\..\ > nul
if exist aricoder.* del aricoder.*
if exist bitops.* del bitops.*
popd
echo Building liblzma...
pushd contrib\liblzma
set LIBLZMA=common\tuklib_physmem.c common\tuklib_cpucores.c common\common.c common\block_util.c common\easy_preset.c
set LIBLZMA=%LIBLZMA% common\filter_common.c common\hardware_physmem.c common\index.c common\stream_flags_common.c
set LIBLZMA=%LIBLZMA% common\vli_size.c common\alone_encoder.c common\block_buffer_encoder.c common\block_encoder.c
set LIBLZMA=%LIBLZMA% common\block_header_encoder.c common\easy_buffer_encoder.c common\easy_encoder.c
set LIBLZMA=%LIBLZMA% common\easy_encoder_memusage.c common\filter_buffer_encoder.c common\filter_encoder.c
set LIBLZMA=%LIBLZMA% common\filter_flags_encoder.c common\index_encoder.c common\stream_buffer_encoder.c
set LIBLZMA=%LIBLZMA% common\stream_encoder.c common\stream_flags_encoder.c common\vli_encoder.c
set LIBLZMA=%LIBLZMA% common\hardware_cputhreads.c common\outqueue.c common\stream_encoder_mt.c common\alone_decoder.c
set LIBLZMA=%LIBLZMA% common\auto_decoder.c common\block_buffer_decoder.c common\block_decoder.c
set LIBLZMA=%LIBLZMA% common\block_header_decoder.c common\easy_decoder_memusage.c common\filter_buffer_decoder.c
set LIBLZMA=%LIBLZMA% common\filter_decoder.c common\filter_flags_decoder.c common\index_decoder.c common\index_hash.c
set LIBLZMA=%LIBLZMA% common\stream_decoder.c common\stream_buffer_decoder.c common\stream_flags_decoder.c
set LIBLZMA=%LIBLZMA% common\vli_decoder.c check\check.c check\crc32_table.c check\crc32_fast.c
set LIBLZMA=%LIBLZMA% check\crc64_table.c check\crc64_fast.c check\sha256.c lz\lz_encoder.c
set LIBLZMA=%LIBLZMA% lz\lz_encoder_mf.c lz\lz_decoder.c lzma\lzma_encoder.c lzma\lzma_encoder_presets.c
set LIBLZMA=%LIBLZMA% lzma\lzma_encoder_optimum_fast.c lzma\lzma_encoder_optimum_normal.c lzma\fastpos_table.c
set LIBLZMA=%LIBLZMA% lzma\lzma_decoder.c lzma\lzma2_encoder.c lzma\lzma2_decoder.c rangecoder\price_table.c
set LIBLZMA=%LIBLZMA% delta\delta_common.c delta\delta_encoder.c delta\delta_decoder.c simple\simple_coder.c
set LIBLZMA=%LIBLZMA% simple\simple_encoder.c simple\simple_decoder.c simple\x86.c simple\powerpc.c simple\ia64.c
set LIBLZMA=%LIBLZMA% simple\arm.c simple\armthumb.c simple\sparc.c
%GCC% %MPARAM% -std=c99 -c -O2 -s -fomit-frame-pointer -Iapi\ -Icheck\ -Icommon\ -Idelta\ -Ilz\ -Ilzma\ -Irangecoder\ -Isimple\ -Wno-implicit-function-declaration -DHAVE__BOOL %LIBLZMA%
move /Y *.o ..\..\ > nul
popd
:nocontrib
echo Building precomp...
%GPP% %DCOMFORT% %MPARAM% -std=c++11 -static -static-libgcc -static-libstdc++ -DMINGW %DBIT% -lpthread -Wall precomp.cpp %JPG_O% %MP3_O% %GIF_O% %BZIP_O% %ZLIB_O% %LIBLZMA_CPP% %LIBLZMA_O% -O2 -fomit-frame-pointer -s -o%EXE1%%EXE2%.exe
if not %ERRORLEVEL% == 0 echo ERROR!!!
if %ERRORLEVEL% == 0 echo.
if %ERRORLEVEL% == 0 echo Build successful.
set LIBLZMA_CPP=
set LIBLZMA_O=
set LIBLZMA=
set ZLIB_O=
set ZLIB=
set BZIP_O=
set BZIP=
set GIF_O=
set GIF=
set JPG_O=
set MP3_O=
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