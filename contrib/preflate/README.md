preflate v0.1.2
===============
Library to split deflate streams into uncompressed data and reconstruction information,
or reconstruct the original deflate stream from those two. 


What's it all about?
--------------------
Long term storage. There are a lot of documents, archives, etc that contain deflate stream.
Deflate is a very simple, but not very strong compression algorithm. For long term storage,
it makes sense to compress the uncompressed deflate stream with a stronger algorithm, like
lzma, rolz, bwt or others.

Reconstructing the original deflate stream becomes important if the position or size
of the reconstructed deflate streams must not differ, e.g. if those streams are embedded
into executables, or unsupported archive files where the indices cannot be adapted correctly.

There are currently at least two tools available which try to solve this problem:
- "precomp" is a tool which can do the bit-correct reconstruction very efficiently,
  but only for deflate streams that were created by the zlib library. (It only needs
  to store the three relevant zlib parameters to allow reconstruction.)
  It bails out for anything created by 7zip, kzip, or similar tools.
  Of course, "precomp" also handles JPG, PNG, ZIP, GIF, PDF, MP3, etc, which makes
  a very nice tool, and it is open source.
- "reflate" can reconstruct any deflate stream (also 7zip, kzip, etc), but it is only 
  efficient for streams that were created by zlib, compression level 9. 
  All lower compression levels of zlib require increasing reconstruction info, the further
  from level 9, the bigger the required reconstruction data.
  "reflate" only handles deflate streams, and is not open source. As far as I know,
  it is also part of PowerArchiver.


What about "difflate"?
----------------------
The author of "precomp" has announced quite some time ago to work on "difflate", 
which should basically be an open source alternative to "reflate". It is still in
development and not yet feature complete. Let's wait and see.


So, what is the point of "preflate"?
------------------------------------
The goal of "preflate" is to get the best of both worlds:
- for deflate streams created by zlib at any compression level, we want to
  be able to reconstruct it with only a few bytes of reconstruction information (like "precomp")
- for all other deflate streams, we want to be able to reconstruct them with
  reconstruction information that is not much larger than "reflate"

Right now, it has been tested on 11159 valid deflate streams, extracted with
"rawdet" from archives etc., and preflate was capable of inflating and reconstructing 
all of them.

There are still known (and also likely unknown) corner cases of valid deflate stream in 
which preflate will fail.


So, is "preflate" already better than "precomp" and "reflate"? 
--------------------------------------------------------------
No. It isn't. 

Test coverage is still quite low, and testing it is currently quite cumbersome
(because it only works on raw deflate streams which need to be extracted first.)

It's very slow. (50-500% slower than "precomp" or "reflate"). Especially for files
containing long runs of the same byte (e.g. \0 or spaces), it gets very, very slow.

It will not handle all valid deflate streams. (E.g., preflate will fail if the reference
length 258 is encoded as 227+31.) I don't know if this is really a problem. All good
encoders would never encode a length of 227+31 anyway.

The detection of the zlib compression parameters is not always on spot, which leads to
the creation of larger diffs than necessary.

Right now, it is a proof of concept, that we can do better than both "precomp" and "reflate". 
It just isn't stable and fast enough yet to be of practical use.


How do I build it?
------------------
There is a make file, but it has only been tested so far with MinGW gmake.
The produced executable is larger than 1MiB, while the MSVC compiler generated
executables were around 100KiB. The reason for that is unclear at the moment.


Credits
-------
- "precomp" by Christian Schneider
- "reflate" and "rawdet" by Eugene Shelwien
- "zlib" by Mark Adler et al.
- "7zip" by Igor Pavlov
- "kzip" by Ken Silverman

All of the software above is just AWESOME!

- the ENCODE.RU data compression forum (http://encode.ru/forum/2-Data-Compression)

If you want to know about the new hot stuff in data compression (doesn't happen
too often though), look here first.

- www.squeezechart.com by Stephan Busch

Contains information about a lot of interesting compression tools of which I probably
would never have known without this site. Also, Stephan helped getting rid of several
bugs in preflate. Thank you.


Notes
-----
Currently, "preflate" uses code from two libraries:
- packARI by Matthias Stirner, which is licensed under LGPL3

  (directory packARI)

  It is used for the arithmetic coding of the reconstruction information.
- zlib 1.2.11 by Mark Adler et al., under the zlib license.

  (directory zlib 1.2.11.dec. Does NOT contain the full zlib library!)

  It is used for the decoding of deflate streams, and some callbacks were
  added to get the decoded trees and tokens, which are then used to build
  the reconstruction information.

The usage of both libraries will be removed in the future. 
There already is a new implementation of deflate decoding (which is still slower 
than the zlib one with callbacks).
And packARI is much more powerful and flexible than what is actually needed
in "preflate" right now.


License
-------
Copyright 2018 Dirk Steinke

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
