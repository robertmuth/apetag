#!/bin/bash

set -o nounset
set -o errexit


readonly APETAG=./apetag
readonly MP3=./empty.mp3
readonly MP3_CLONE=./clone.mp3
readonly BIN1=./test.sh
readonly BIN2=./COPYING
readonly BIN3=./README.md

COUNTER=0

cleanup() {
    rm -f ${MP3_CLONE}
}
trap cleanup EXIT

clone() {
    cp ${MP3} ${MP3_CLONE}
}


newtest() {
    COUNTER=$((${COUNTER} + 1))
    echo "============================================================"
    echo "test ${COUNTER}"
    clone
}

newtest
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -p  "Title=--title--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p  "title=--title--"
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title3--" -p "Year="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -p "Title=--title3--"
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -f  "Test.sh"=${BIN1}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f  "test.sh"=${BIN1}
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN2} -f "NotTest"=${BIN2}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN3} -f "NotTest="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN2} -f "NotTest"=${BIN2}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -f "Test.sh=${BIN3}"
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -r  "TestPage=http://bo.gus/addr"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -r  "testpage=http://bo.gus/addr"
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/website/page.html" -r "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/addr/testpage.html" -r "ExternalCover="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/website/page.html" -r "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -r "TestPage=http://bo.gus/addr/testpage.html"
${APETAG} -i ${MP3_CLONE} -m read


newtest
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN1} -p "Test"="File" -r "ExternalCover"="/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN1} -p "test"="File" -r "externalcover"="/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read

echo "done"
