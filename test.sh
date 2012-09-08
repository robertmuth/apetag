#!/bin/bash

set -o nounset
set -o errexit


readonly APETAG=./apetag
readonly MP3=./empty.mp3
readonly MP3_CLONE=./clone.mp3

COUNTER=0

clone() {
    rm -f ${MP3_CLONE}
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


newtest
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title3--" -p "Year="
${APETAG} -i ${MP3_CLONE} -m read

newtest
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -p "Title=--title3--"
${APETAG} -i ${MP3_CLONE} -m read

echo "done"
