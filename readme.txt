Precomp v0.4.6
--------------
1. What is Precomp?
2. What is Precomp Comfort?
3. Filetypes
4. How to use it
5. FAQ
6. Contact
7. Credits
8. Legal stuff

1. What is Precomp?
-------------------
Precomp is a command line precompressor. You can use it to achieve better compression on some filetypes (works on files that are compressed with zLib or the Deflate compression method, and on GIF files). Precomp tries to decompress the streams in those files, and if they can be decompressed and "re-"compressed so that they are bit-to-bit-identical with the original stream, the decompressed stream can be used instead of the compressed one.

The result is a .pcf file (PCF = PreCompressedFile) that contains more decompressed data than the original file. Note that this file is larger than the original file, but if you compress it with a compression method stronger than Deflate, the compression is better than before.

Since version 0.4.3, Precomp is available for Linux, too. The Linux and Windows versions are completely compatible, PCF files are exchangeable between Windows and Linux systems.

2. what is Precomp Comfort?
---------------------------
Precomp Comfort is a variation of Precomp. It supports drag and drop of single files and uses an INI file for the parameters.
Run precomp.exe for the original version, precomf.exe for the Comfort version.

3. Filetypes
------------
Here is a list of filetypes that can eventually achieve better compression with Precomp and how you can check if they can.
Note that this list is not complete, and that other filetypes can contain Deflate or zLib streams, too, but you should use the intense mode parameter (-intense) for them.

PDF
Adobe's PDF files often use zLib compression to compress their documents.
Check: "FlateDecode" appears in the file, but not paired with "ASCII85Decode".

JPG
Precomp uses packJPG by Matthias Stirner (https://github.com/packjpg/packJPG) to losslessy compress JPG images.

MP3
Precomp uses packMP3 by Matthias Stirner (http://packjpg.encode.ru/?page_id=19) to losslessy compress MP3 audio files.

MJPEG
MJPEG is a video format that consists of JPG images without huffman tables.
Precomp inserts them so that packJPG is able to compress the images.

ZIP/JAR
Most ZIP files use Deflate for compression. JAR files basically are ZIP files with a additional manifest for use with Java.

PNG
PNG uses Deflate to compress its filtered image data.

GIF
The GIF format uses LZW to compress its image data.

GZ
GZip files use Deflate for compression.

BZ2
bZip2 is a format often used in Linux environments.

SWF
Macromedia's Shockwave Flash files can use zLib compression since Version 6.
Check: First three bytes of file are CWS (instead of FWS for uncompressed files).

MIME Base64
This encoding is used to attach binary files to e-mails.

SVGZ
These files contain SVG files compressed with GZip.

ODT
OpenOffice Document files consist of zipped XML data.

SIS (slow mode only)
These files contain informations about software installation on Symbian OS for mobile phones. They use zLib compression.

3DM (slow mode only)
This is a file format for 3D geometry used by Rhino3D that contains zLib streams.

zeno (slow mode only)
Zeno is a file format used by e.g. the german Wikipedia DVD. 

4. How to use it
----------------
Simplest way (Precomp Comfort):
Drag and drop a file on precomf.exe to precompress the file into a .pcf file with the same name
To get back the original file, do the same with the .pcf file.

Or use the command line: (Precomp)
"precomp input_filename" to precompress a file into a .pcf file with the same name
"precomp -rpcf_filename" to restore the original file (-d is still valid, too)

For batch jobs, you'll find these errorlevels useful that are returned: 

0	No error
1 	Various errors (f.e. file access errors)
2 	No streams could be decompressed
3 	Disk full
4 	Temporary file disappeared
5 	Parameter error: Ignore position too big
6 	Parameter error: Identical byte size too big
7 	Parameter error: Recursion level too big
8 	Parameter error: Recursion level set more than once
9 	Parameter error: Minimal identical byte size set more than once
10 	Parameter error: Don't use a space after -o
11 	Parameter error: More than one output file
12 	Parameter error: More than one input file
13      Ctrl-C detected (user break)
14      Parameter error: Intense mode recursion limit too big
15      Parameter error: Brute mode recursion limit too big


Additional switches:

-longhelp:

Only common switches are shown by default. This switch will display a long and detailed help.

-o[filename]:

Specifies the output file name. For precompression, default is the original file name with extension .pcf, for
"decompression", it is the original file name. If the output file exists, you will be asked if you want to overwrite it. Nevertheless, you can specify a different output file name with this option.

-c[bn]: (Comfort: Compression_Method)

The first step that Precomp does is to decompress all the streams in the input file. After this, it can recompress the file using bZip2 ("-cb", default setting) or leave it as it is ("-cn") so you can compress the PCF file with a stronger compression method.

-n[bn]:

This switch is for converting a PCF file from no compression to bZip2 compression and vice versa without running Precomp on the original file again.

-zl: (Comfort: zLib_Levels)

After precompressing a file with Precomp, it tells you how to use this parameter to speed up the precompression the next time you precompress this file. These are one or more two-digit numbers. The first digit is the compression level, the second digit is the memory settings which are tried on this file. However, using this on a different file could lead to Precomp missing some compressed parts of it.

-t: (Comfort: Compression_Types)
Enables or disables detecting of certain compression types. For command-line use, there are two variants:
t+ enables certain types and disables the others, while t- disables certain types and enables the others.
Using -t-j for example disables JPEG recompression and leaves all other types as before, using -t+pf enables only PDF and GIF precompression, disabling everything else.

-d: (Comfort: Maximal_Recursion_Depth)
Sets the maximal recursion depth. Several streams can contain additional streams inside, for example ZIP or MIME Base64 streams. This switch specifies the maximal "depth" where Precomp will look for streams. Setting this to 0 disables recursion, the default is 10 which should be enough for most filetypes.

-f: (Comfort: Fast_Mode)

Fast mode to speed up Precomp. This uses the first found compression for all streams instead of trying all 81 combinations when not sure. This will work fine on files that use only a few compression methods, but will result in worse compression for files with many compression methods used. Good candidates are PDF and ZIP/JAR/GZ files. Bad candidates are archives containing many files.
In non-fast mode, there is a message when only one level combination is used. This means that fast mode will do absolutely the same on this file, but faster.

-intense: (Comfort: Intense_Mode)

Slow mode will slow down Precomp much. It looks for raw zLib headers, and recognizes more file formats like SIS and SWF or special formats used only for one single program. However, the zLib header consists of only 2 bytes, so there can be many false-detected streams that aren't zLib streams but are handled like them, which results in a slower and more instable behaviour.
Slow mode can be combined with fast mode, but it could happen that a false-detected stream is the first stream and prevents further real streams to be detected, so combine them with caution.
Use this mode if you have files that use zLib compression but are not supported (SIS, SWF, game ISO files...).

-brute: (Comfort: Brute_Mode)

Brute mode will slow down Precomp very much. It assumes that there could be zLib streams without headers everywhere. This even recognizes most exotic file formats that don't include zLib headers but will take very much time (more than a minute even for filesizes around 10 KB). If you should have data that has to be processed with this mode, better try to add zLib headers on your own.
Brute mode can be combined with fast mode, but disables slow mode.

-pdfbmp[+-]: (Comfort: PDF_BMP_Mode)

This precedes PDF images with a BMP header to improve compression and speed, especially for PAQ.

-progonly[+-]: (Comfort: JPG_progressive_only)

Recompresses progressive JPGs only. Again, this is especially useful for PAQ which usually has a better JPG compression than packJPG, but lacks progressive JPG support.

-mjpeg[+-]: (Comfort: MJPEG_recompression)

Enables MJPEG recompression by inserting huffman tables into the JPG data.

-v: (Comfort: Verbose)

Verbose (debug) mode to gain additional information about detected streams and recompression success/failure. If you want a file with these informations, forward the output to it, like this: "precomp -v input_filename > verbose.txt".

-i: (Comfort: Ignore_Positions)

In verbose mode, you can see the position of streams in the file. With this parameter, you can ignore certain streams.

-s: (Comfort: Minimal_Size)

With this parameter, you can choose the minimal size of a stream that will be processed. The default is 4 bytes. Setting it to higher values (around 50-200 bytes) sometimes improves recompression, especially in slow or brute mode.

5. FAQ
------
Q: I tried to compress a file precompressed with Precomp and it didn't get smaller.

A: Precomp couldn't find any compressed streams in the file and bZip2 compression didn't help either.

Q: Is the source code for Precomp available?

A: Not yet, because it is very messy at the moment, but it will be available soon.

Q: Are there any known bugs?

A: There are some bugs that lead to crashes on very special corrupt files, but these are very unusual. Nevertheless, Precomp is far from being complete, so if you find a bug, send me a bug report.

Q: I have found a bug. How to report it?

A: Send a mail to schnaader@gmx.de, preferably with "[Precomp]" in subject with a description of the bug and if you
want (and if it is less than 10 MB), the file you wanted to precompress/restore.

Q: What is the difference between using Precomp or Multivalent for PDF files?

A: The main difference is that PDF files compressed with Multivalent can't be restored bit-to-bit-identical because
Multivalent is a lossy compression method (although it is doesn't lose the PDF content). So if you just want to compress PDF files and to have fast access to them later on, use Multivalent. If you want to get them smaller than Multivalent (even in compact mode) does, or want to be sure the file is bit-to-bit-identical with the original PDF, use Precomp. You can also use Precomp on PDF files compressed with Multivalent. 

Q: The precompression for PNG, GIF and ZIP files is bad, although verbose mode says they can be decompressed completely.

A: The decompression of those files is well-defined, but there are many ways to recompress them. Especially zLib can be tuned with deflateTune(), which is not supported by Precomp because there are simply too much variations to try. I'm working on this.

6. Contact
----------
Christian Schneider
schnaader@gmx.de
http://schnaader.info
https://github.com/schnaader/precomp-cpp

7. Credits
----------
Thanks for support, help and comments:
  Stephan Busch (Squeeze Chart Author, http://www.squeezechart.com))
  Werner Bergmans (Maximum Compression Benchmark Author, http://www.maximumcompression.com)
  Matthias Stirner (packJPG, packMP3, https://github.com/packjpg, http://packjpg.encode.ru, http://www.matthiasstirner.com)
  Mark Adler (http://www.zlib.net)
  Matt Mahoney (http://www.mattmahoney.net)
  Malcolm Taylor (http://www.msoftware.co.nz)
  Simon Berger (helped to fix many bugs)
  The whole ENCODE.RU forum (http://encode.ru/forum)

8. Legal stuff
--------------
packJPG v2.5k (https://github.com/packjpg/packJPG) by Matthias Stirner is used for compression/decompression of JPG files.
packMP3 v1.0g by Matthias Stirner (http://packjpg.encode.ru/?page_id=19) by Matthias Stirner is used for compression/decompression of MP3 files.
bZip2 1.0.6 (http://www.bzip.org) by Julian Seward is used for compression/decompression of bZip2 streams.
zLib 1.2.8 (http://www.zlib.net) by Jean-loup Gailly and Mark Adler is used for compression/decompression of zLib streams.
GifLib 4.1.4 (http://sourceforge.net/projects/giflib) is used for compression/decompression of GIF files.
  The GIFLIB distribution is Copyright (c) 1997 Eric S. Raymond

Precomp itself is licensed under Apache License 2.0, also see the LICENSE file

Copyright 2006-2016 Christian Schneider

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
