#!/usr/bin/env python

# ======================================================================
#     Copyright (C) 2002 and onward Robert Muth <robert at muth dot org>
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, version 3 of the License.

#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
# ======================================================================

"""
This module provides a library for reading/parsing cddb records
as served by freedb.org
"""

# ======================================================================
__all__ = [
    'Toc',
    'TocContainer',
    ]

# ======================================================================

import os
import re


# ======================================================================
_PATTERN_DISC_LENGTH = re.compile(r'# Disc length:[ \t]+([0-9]+).*$')
_PATTERN_TRACK_OFFSET = re.compile(r'# Track frame offsets:[ \t\r]*$')
_PATTERN_OFFSET = re.compile(r'#[ \t]+([0-9]+)[ \t\r]*$')
_PATTERN_TOC_COMMENT = re.compile(r'^[#\.](.*)$')
_PATTERN_TOC_EMPTY = re.compile(r'^[ \t\r\n]*$')
_PATTERN_TOC_TRACK_TITLE = re.compile(r'^TTITLE([0-9]+)\=(.*?)[\r]*$')
_PATTERN_TOC_TRACK_EXT = re.compile(r'^EXTT([0-9]+)\=(.*?)[\r]*$')
_PATTERN_TOC_DISK_TITLE = re.compile(r'^DTITLE\=(.*?)[\r]*$')
_PATTERN_TOC_DISK_YEAR = re.compile(r'^DYEAR\=(.*?)[\r]*$')
_PATTERN_TOC_DISK_GENRE = re.compile(r'^DGENRE\=(.*?)[\r]*$')
_PATTERN_TOC_DISK_ID = re.compile(r'^DISCID\=(.*?)[\r]*$')
_PATTERN_TOC_DISK_EXT = re.compile(r'^EXTD\=(.*?)[\r]*$')
_PATTERN_TOC_PLAY_ORDER = re.compile(r'^PLAYORDER\=(.*)$')

# ======================================================================
class Toc:
    """
    Class to parse and hold the content of a freedb (cddb) record
    """

    def __init__(self, filename):

        inp = file(filename, "r")
        lines = inp.readlines()
        inp.close()

        self._discid = ""
        self._disctitle = ""
        self._discalbum = ""
        self._discartist = ""
        self._discyear = ""
        self._discgenre = ""
        self._discext = ""
        self._tracktitle = []
        self._trackext = []
        self._tracktimes = []
        self._disctime = None

        while lines:
            line = lines.pop(0).decode("utf8")
            match = _PATTERN_TRACK_OFFSET.search(line)
            if match:
                while 1:
                    line = lines.pop(0).decode("utf8")
                    match = _PATTERN_OFFSET.search(line)
                    if match:
                        self._tracktimes.append(int(match.group(1)))
                    else:
                        break
                    # there is an important fall through here

            match = _PATTERN_DISC_LENGTH.search(line)
            if match:
                self._disctime = int(match.group(1))
                self._tracktimes.append( self._disctime * 75 )
                for i in range(len(self._tracktimes)-1):
                    self._tracktimes[i] = (self._tracktimes[i+1] -
                                           self._tracktimes[i]) / 75
                self._tracktimes[-1] = self._tracktimes[-1]/75
                self._tracktitle = [""] * (len(self._tracktimes) - 1)
                self._trackext = [""] * (len(self._tracktimes) - 1)
                continue


            if line[0] in '# \n\r\t':
                continue

            match = _PATTERN_TOC_PLAY_ORDER.search(line)
            if match:
                continue
            match = _PATTERN_TOC_DISK_EXT.search(line)
            if match:
                self._discext =   self._discext + match.group(1)
                continue
            match = _PATTERN_TOC_DISK_ID.search(line)
            if match:
                self._discid =   self._discid + match.group(1)
                continue
            match = _PATTERN_TOC_DISK_GENRE.search(line)
            if match != None:
                self._discgenre =   self._discgenre + match.group(1)
                continue
            match = _PATTERN_TOC_DISK_YEAR.search(line)
            if match != None:
                self._discyear =   self._discyear + match.group(1)
                continue
            match = _PATTERN_TOC_DISK_TITLE.search(line)
            if match != None:
                self._disctitle =   self._disctitle + match.group(1)
                continue
            match = _PATTERN_TOC_TRACK_TITLE.search(line)
            if match != None:
                track =  int(match.group(1))
                self._tracktitle[track] = self._tracktitle[track] +  match.group(2)
                continue
            match = _PATTERN_TOC_TRACK_EXT.search(line)
            if match != None:
                track =  int(match.group(1))
                self._trackext[track] = self._trackext[track] + match.group(2)
                continue
            raise SyntaxError, "BAD LINE:" + line

        components = self._disctitle.split("/", 1)
        self._discartist = components[0].strip()
        if len(components) <= 1:
            self._discalbum = ""
        else:
            self._discalbum = components[1].strip()
        return

    def title_get(self):
        return self._disctitle

    def album_get(self):
        return self._discalbum

    def artist_get(self):
        return self._discartist

    def track_get(self, index):
        if index < len(self._tracktitle) and index >= 0:
            return (self._tracktitle[index],
                    self._trackext[index],
                    self._tracktimes[index])
        else:
            return ("", 0, 0)

    def num_tracks(self):
        return len(self._tracktitle)

    def dump(self):
        print "DISCID:    " + self._discid
        print "DISCTITLE: " + self._disctitle
        print "DISKEXT:   " + self._discext
        print "TRACK TITLES"
        for k in range(len(self._tracktitle)):
            print "%2d:" % k, self._tracktitle[k]
        print "TRACK EXTS"
        for k in range(len(self._trackext)):
            print "%2d:" % k, self._trackext[k]
        print "TRACK TIMES"
        for k in range(len(self._tracktimes)):
            print "%2d:" % k, self._tracktimes[k]
        return

# ======================================================================
class TocContainer:
    """
    Dictionary for caching Toc of given paths
    """

    def __init__(self):
        self._container = {}
        return

    def find_toc(self, dirname):
        filename = os.path.join(dirname, "toc")
        if self._container.has_key(filename):
            return self._container[filename]

        if not os.access(filename, os.R_OK):
            self._container[filename] = None
            return None

        toc = Toc(filename)
        self._container[filename] = toc
        return toc

# ======================================================================
# eof
# ======================================================================




