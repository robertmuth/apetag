#!/usr/bin/env python

# ======================================================================
#
#     Copyright (C) 2004 and onward Robert Muth <robert at muth dot org>
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, version 3 of the License.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
# ======================================================================

"""
This tools removes id3 v1 tags from music files so that they can
be annotated with more advanced tags
"""

USAGE = """
rmid3tag.py - script for removing id3v1 tags for files
usage:
    rmid3tag.py file1 file2 ...
"""

# python imports
import sys
import logging

# ======================================================================
def usage():
    print USAGE
    return -1

# ======================================================================

def process_file(name):
    try:
        inp = file(name, "r+")
        inp.seek(-128, 2)
        pos = inp.tell()
        tag = inp.read(3)
        if tag == "TAG":
            print name + ": found id3v1 tag - truncating at 0x%08x" % pos
            inp.truncate(pos)
        else:
            logging.warning(name + ": no id3v1 tag found\n")

        inp.close()
        return
    except:
        logging.error("cannot open file: %s", name)
        sys.exit(-1)
    return

# ======================================================================
def main(argv):
    if len(argv) == 0:
        logging.error("you must specify at least on file")
        return usage()

    filelist = sys.argv[1:]
    if filelist == []:
        logging.error("no file specified")
        usage()
        sys.exit(-1)

    for i in filelist:
        process_file(i)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

# ======================================================================
# eof
# ======================================================================




