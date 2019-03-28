# Apetag/Tagdir readme file

This package contains three tagging related tools: apetag, tagdir, rmid3tag

## apetag

Apetag is command line tagging tool for multimedia files such as
monkey's audio and mpc using the APE 2.0 standard.

For usage information run "apetag -h"

## tagdir

Tagdir is a simple python script that will automatically invoke an
appropriate tagging tool on all tracks in the current directory
assuming that they belong to the same album and are systematically
named, i.e. the first two characters are the track number and the
first such number being '01'.  Tagdir obtains the title, album,
etc. information from a "toc" file. A toc file is a freedb record
(c.f. freedb.org) which can be retrieved using freedbtool
(http://muth.org/Robert/Freedbtool).  Currently supported tagging
tools are apetag for mpc files and metaflac for flac files.

For usage information run "tagdir.py -h"

## rmid3tag

Rmid3tag is a simple python script that will remove id3 tags from the
end of music files.

Please send comments and suggestions to robert at muth dot org
http://www.muth.org/Robert

