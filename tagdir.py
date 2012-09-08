#!/usr/bin/env python

# ======================================================================
#      Copyright (C) 2002 and onwwards Robert Muth <robert at muth dot org>
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; version 2 of June 1991.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program in form of the file COPYING;
#      if not, write to the
#
#      Free Software Foundation, Inc. <http://www.fsf.org>
#      59 Temple Place, Suite 330,
#      Boston, MA 02111-1307  USA
#
# ======================================================================

"""
tool for tagging music files of various formats based on
cddb records from freedb.org
"""

USAGE = """
Tagdir - a script for tagging music files (mp3,mpc,flac,ape,etc.)
         based on information provided by the freedb.org database

Usage:
\ttagdir [option]* [freedb-file]*

Options:
-t: select path to freedb-file [default ./toc]
-m: set operation mode [default tag], currently there are four modes

mode dump
\tprint the content of all freedb=files listed at the end of the commandline

mode tag
\ttag all music files in the current directory using the appropriate tag tool.
\tCurrently only apetag (for mpc files) and metaflac (for flac file) are supported
\tThe matching between music file and track number is based on filename.
\tThe filename must either start with a two digit tracknumber or start with
\t'track' followed by the two digit tracknumber.
\tNB: the smallest track number is one and NOT zero.

mode test
\tdryrun, same as tag but skip actual invocation of the tagging tool

 mode check
\tcheck whether tracks in current directory match the number in the freedb-file
"""

# ======================================================================

# python imports
import getopt
import glob
import logging
import os
import re
import sys

# local imports
import cddb

# ======================================================================
# config section
# ======================================================================
_TRACK_PATTERNS = [ re.compile(r'^([0-9]+)[^0-9].*$'),      # leading digits
                   re.compile(r'^track([0-9]+)[^0-9].*$'), # trailing digits
                   ]

FLAC_TAG_CMD = ["metaflac",
                "--no-utf8-convert",
                "--remove-tag=TITLE",
                "--set-tag=TITLE=%(title)s",
                "--remove-tag=ALBUM",
                "--set-tag=ALBUM=%(album)s",
                "--remove-tag=ARTIST",
                "--set-tag=ARTIST=%(artist)s",
                "--remove-tag=TRACKNUMBER",
                "--set-tag=TRACKNUMBER=%(track)s",
                "%(filename)s",
                ]

MID3V2_TAG_CMD = ["mid3v2",
                  "--song=%(title)s",
                  "--album=%(album)s",
                  "--artist=%(artist)s",
                  "--track=%(track)s",
                  "%(filename)s",
                  ]

ID3_TAG_CMD = ["id3v2",
               "--song",
               "%(title)s",
               "--album",
               "%(album)s",
               "--artist",
               "%(artist)s",
               "--track",
               " %(track)s",
               "%(filename)s",
               ]

APT_TAG_CMD = ["apetag",
               "-m",
               "overwrite",
               "-i",
               "%(filename)s",
               "-p",
               "Title=%(title)s",
               "-p",
               "Album=%(album)s",
               "-p",
               "Artist=%(artist)s",
               "-p",
               "Track=%(track)s",
               ]

# ======================================================================

def extract_index(pathname):
    dummy, filename = os.path.split(pathname)
    for trackpat in _TRACK_PATTERNS:
        match = trackpat.search(filename)
        if match:
            return int(match.group(1)) - 1
    return - 1

# ======================================================================
def extract_file_list(files, toc):
    filelist = []
    for entry in files:
        index = extract_index(entry)
        if index < 0:
#            print "skiping " + entry
            continue

        track = toc.track_get(index)
        if not track[0]:
            Warning("bad track " + entry)
            continue
#        if track[0].find('"') >= 0:
#            Warning("track contains double quotes")
#            continue

        filelist.append(entry)

    if filelist and len(filelist) != toc.num_tracks():
        Warning("track number mismatch")
    return filelist

# ======================================================================

def tag_files(files, toc, cmdpattern, test, verbose):
    """
    tag files base toc using the cmdpattern
    """
    for entry in files:
        args = {}
        args['album'] = toc.album_get().encode('utf8')

        index = extract_index(entry)
        args['track'] = "%d" % (1+ index)

        song = toc.track_get(index)
        song = song[0].split("/")

        if len(song) == 1:
            args['artist'] = toc.artist_get().encode('utf8')

            args['title'] = song[0].strip().encode('utf8')
        elif len(song) == 2:
            args['artist'] = song[0].strip().encode('utf8')
            args['title'] = song[1].strip().encode('utf8')
            logging.warning("%s: found track artist: %s", entry, "|".join(song))
        else:
            logging.error("%s bad track: %s", entry, "|".join(song))
            return
        args['filename'] = entry

        realcmd    = [tok % args for tok in cmdpattern]
        if verbose:
            print realcmd
        if not test:
            ret = os.spawnvp(os.P_WAIT, realcmd[0], realcmd)
            if ret:
                logging.error("command failed")
    return
# ======================================================================

def mode_dump(files):
    """
    perform read test and all (toc) files  and dump their content
    """
    for i in files:
        print "TOC: ", i
        if not os.access(i, os.R_OK):
            logging.warning("cannot open " + i)
            continue
        toc = cddb.Toc(i)
        toc.dump()
    return 0


# ======================================================================

def mode_toc(tocpath, test, check, verbose):
    """
    tag files
    """
    try:
        toc = cddb.Toc(tocpath)
    except Exception, ex:
        logging.error("cannot open toc file at %s  %s",
                      os.path.abspath(tocpath), str(ex))
        return -1

    album = toc.album_get()
    if album == "":
        logging.error("no album")
        return -1

    if album.find('/') >= 0:
        logging.error("album contains slash")
        return -1

    artist = toc.artist_get()
    if artist == "":
        logging.error("no artist")
        return -1

    if artist.find('/') >= 0:
        logging.error("artist contains slash")
        return -1

    filelist = extract_file_list(glob.glob("*.flac"), toc)
    if filelist and not check:
        tag_files(filelist, toc, FLAC_TAG_CMD, test, verbose)

    filelist = extract_file_list(glob.glob("*.mp3"), toc)
    if filelist and not check:
        tag_files(filelist, toc, ID3_TAG_CMD, test, verbose)

    filelist = extract_file_list(glob.glob("*.mpc"), toc)
    if filelist and not check:
        tag_files(filelist, toc, APT_TAG_CMD, test, verbose)

    return 0


# ======================================================================
def main(argv):
    try:
        opts, args = getopt.getopt(argv, "t:m:v")
    except getopt.error:
        print USAGE
        return -1

    mode = "tag"
    tocpath = "./toc"
    verbose = 1
    for opt, val in opts:
        if opt == "-m":
            mode = val
        elif opt == "-t":
            tocpath = val
        elif opt == "-t":
            verbose = 1
        else:
            logging.error("bad option: >" + opt + "<")
            print USAGE
            return -1
    if mode == "dump":
        return mode_dump(args)
    elif mode == "tag":
        return mode_toc(tocpath, 0, 0, verbose)
    elif mode == "test":
        return mode_toc(tocpath, 1, 0, verbose)
    elif mode == "check":
        return mode_toc(tocpath, 0, 1, verbose)
    else:
        logging.error("unknonw mode " + mode)
        return -1
    return 0

# ======================================================================
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))


# ======================================================================
# eof
# ======================================================================
