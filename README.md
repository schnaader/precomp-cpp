Precomp
=======

What is Precomp?
----------------
Precomp is a command line precompressor. You can use it to achieve better compression on some filetypes (works on files that are compressed with zLib or the Deflate compression method, and on GIF files). Precomp tries to decompress the streams in those files, and if they can be decompressed and "re-"compressed so that they are bit-to-bit-identical with the original stream, the decompressed stream can be used instead of the compressed one.

The result is a .pcf file (PCF = PreCompressedFile) that contains more decompressed data than the original file. Note that this file is larger than the original file, but if you compress it with a compression method stronger than Deflate, the compression is better than before.

Since version 0.4.3, Precomp is available for Linux, too. The Linux and Windows versions are completely compatible, PCF files are exchangeable between Windows and Linux systems.

Contact
-------
Christian Schneider

schnaader@gmx.de

http://schnaader.info

Credits
-------
Thanks for support, help and comments:

- Stephan Busch (Squeeze Chart Author, http://www.squeezechart.de)
- Werner Bergmans (Maximum Compression Benchmark Author, http://www.maximumcompression.com)
- Matthias Stirner (PackJPG, http://www.elektronik.htw-aalen.de/packjpg/index.htm)
- Mark Adler (http://www.zlib.net)
- Matt Mahoney (http://www.mattmahoney.net, http://www.cs.fit.edu/~mmahoney/compression)
- Malcolm Taylor (http://www.msoftware.co.nz/WinRK_about.php)
- Simon Berger (helped to fix many bugs)
- The whole ENCODE.RU forum (http://encode.ru/forum)

Legal stuff
-----------
- PackJPG v2.5 (http://www.elektronik.htw-aalen.de/packjpg/index.htm) by Matthias Stirner is used for compression/decompression of JPG files.
- bZip2 1.05 (http://www.bzip.org) by Julian Seward is used for compression/decompression of bZip2 streams.
- zLib 1.2.5 (http://www.zlib.net) by Jean-loup Gailly and Mark Adler is used for compression/decompression of zLib streams.
- GifLib 4.1.4 (http://sourceforge.net/projects/libungif) is used for compression/decompression of GIF files. The GIFLIB distribution is Copyright (c) 1997 Eric S. Raymond

License
-------
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
