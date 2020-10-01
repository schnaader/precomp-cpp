Precomp
=======

[![Join the chat at https://gitter.im/schnaader/precomp-cpp](https://badges.gitter.im/schnaader/precomp-cpp.svg)](https://gitter.im/schnaader/precomp-cpp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.com/schnaader/precomp-cpp.svg?branch=master)](https://travis-ci.com/schnaader/precomp-cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/noofdvr23uk2oyyi/branch/master?svg=true)](https://ci.appveyor.com/project/schnaader/precomp-cpp)

[![Packaging status](https://repology.org/badge/vertical-allrepos/precomp.svg)](https://repology.org/metapackage/precomp)

What is Precomp?
----------------
Precomp is a command line precompressor that can be used to further compress files that are already compressed. It improves compression on some file-/streamtypes - works on files and streams that are compressed with zLib or the Deflate compression method (like PDF, PNG, ZIP and many more), bZip2, GIF, JPG and MP3. Precomp tries to decompress the streams, and if they can be decompressed and "re-"compressed so that they are bit-to-bit-identical with the original stream, the decompressed stream can be used instead of the compressed one.

The result of Precomp is either a smaller, LZMA2 compressed file with extension .pcf (PCF = PreCompressedFile) or, when using `-cn`, a file containing decompressed data from the original file together with reconstruction data. In this case, the file is larger than the original file, but can be compressed with any compression algorithm stronger than Deflate to get better compression.

Since version 0.4.3, Precomp is available for Linux/*nix/macOS, too. The different versions are completely compatible, PCF files are exchangeable between Windows/Linux/*nix/macOS systems.

Usage example
-------------
|Command|Comment|
|--|--|
|`wget http://mattmahoney.net/dc/silesia.zip` <br> (or download from [here](http://mattmahoney.net/dc/silesia.html))|We want to compress this file (the [Silesia compression corpus](http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia)). <br>Size: 67,633,896 bytes (100,0%)|
|`7z a -mx=9 silesia.7z silesia.zip`|Compressing with [7-Zip](https://www.7-zip.org/) LZMA2, setting "Ultra". <br>Size: 67,405,052 bytes (99,7%)|
|`precomp silesia.zip`|Compressing with Precomp results in `silesia.pcf`. <br>Size: 47,122,779 bytes (69,7%)|
|`precomp -r -osilesia.zip_ silesia.pcf`|This restores the original file to a new file named `silesia.zip_`. <br> Without the `-o` parameter, Precomp would decompress to `silesia.zip`.|
|`diff -s silesia.zip silesia.zip_`|Compares the original file to the result file, they're identical|


How can I contribute?
---------------------
* You can have a look at the [Issue list](https://github.com/schnaader/precomp-cpp/issues)
  * If you are looking for easy issues that don't require deeper understanding of the whole project, look for [issues with the `low hanging fruits` tag](https://github.com/schnaader/precomp-cpp/labels/low%20hanging%20fruits)
* You can build the project or download the binaries (see below), run it on your system and report bugs or make enhancement proposals.

Releases/Binaries
-----------------
[Official GitHub releases](https://github.com/schnaader/precomp-cpp/releases) for both Windows and Linux.

[Alternative binary download](http://schnaader.info/precomp.php#d) of the latest official release for both Windows and Linux.

Binaries for older version can be found at [this Google Drive folder](https://drive.google.com/open?id=0B-yOP4irObphSGtMMjJSV2tueEE).

Contact
-------
Christian Schneider

schnaader@gmx.de

http://schnaader.info

Donations
---------
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=X5SVF9YUQC9UG)

To donate, you can either use the donate button here, the one at the top of the page ("Sponsor") or the one on [my homepage](http://schnaader.info). You can also send bitcoins to

    1KvQxn6KHp4tv92Z5Fy8dTPLz4XdosQpbz

Credits
-------
Thanks for support, help and comments:

- Stephan Busch (Squeeze Chart Author, http://www.squeezechart.com)
- Werner Bergmans (Maximum Compression Benchmark Author, http://www.maximumcompression.com)
- Matthias Stirner (packJPG, packMP3, https://github.com/packjpg, http://www.matthiasstirner.com)
- Mark Adler (http://www.zlib.net)
- Matt Mahoney (http://www.mattmahoney.net)
- Malcolm Taylor (http://www.msoftware.co.nz)
- Simon Berger (helped to fix many bugs)
- The whole encode.su forum (https://encode.su/)

Legal stuff
-----------
- brunsli (https://github.com/google/brunsli) by Google LLC is used for compression/decompression of JPG files.
- brotli (https://github.com/google/brotli) by the Brotli Authors is used for compression/decompression of JPG metadata.
- packJPG v2.5k (https://github.com/packjpg/packJPG) by Matthias Stirner is used for compression/decompression of JPG files.
- packMP3 v1.0g by Matthias Stirner is used for compression/decompression of MP3 files.
- bZip2 1.0.6 (http://www.bzip.org) by Julian Seward is used for compression/decompression of bZip2 streams.
- zLib 1.2.11 (http://www.zlib.net) by Jean-loup Gailly and Mark Adler is used for compression/decompression of zLib streams.
- GifLib 4.1.4 (http://sourceforge.net/projects/giflib) is used for compression/decompression of GIF files. The GIFLIB distribution is Copyright (c) 1997 Eric S. Raymond
- liblzma from XZ Utils 5.2.3 (http://tukaani.org/xz) is used for compression/decompresson of lzma streams.
- preflate v0.3.5 (https://github.com/deus-libri/preflate) by Dirk Steinke is used to create and use reconstruction information of deflate streams

License
-------
Copyright 2006-2020 Christian Schneider

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
