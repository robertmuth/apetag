#!/bin/bash

set -o nounset
set -o errexit


readonly APETAG=./apetag
readonly MP3=TestData/empty.mp3
readonly MP3_APEONLY=TestData/empty_ape.mp3
readonly MP3_CLONE=TestData/clone.mp3
readonly MP3_CLONEAPE=TestData/clone_ape.mp3
readonly BIN1=./test.sh
readonly BIN2=./COPYING
readonly BIN3=./README.md

COUNTER=0

cleanup() {
    rm -f ${MP3_CLONE}
    rm -f ${MP3_CLONEAPE}
}
trap cleanup EXIT

clone() {
    cp ${MP3} ${MP3_CLONE}
    cp ${MP3_APEONLY} ${MP3_CLONEAPE}
}


newtest() {
    echo "============================================================"
    echo "test ${1}"
    clone
}

newtest NoApeTag
${APETAG} -i ${MP3_CLONE} -m read

${APETAG} -i ${MP3_APEONLY} -m update -p  "one=id3v1magicdetecttag1"
${APETAG} -i ${MP3_APEONLY} -m update -p  "two=id3v1magicdetecttagtwo2"

newtest UpdateMissingTags
${APETAG} -i ${MP3_CLONE} -m update -p  "Title=--title--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p  "title=--title--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -ro "titl"
${APETAG} -i ${MP3_CLONE} -m update -rw "titl"
${APETAG} -i ${MP3_CLONE} -m read


newtest UpdateTags
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title3--" -p "Year="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest BadTags
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--tïtlé2--" -p "Year=--yeár2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -p "Tïtle=--tïtlé2--" -p "Yeár=--yeár2--"
${APETAG} -i ${MP3_CLONE} -m read


newtest ReadOnlyTags
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m update -ro "Title"
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title3--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -rw "Title"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read


newtest MoreUpdates
${APETAG} -i ${MP3_CLONE} -m update -p "Title=--title2--" -p "Year=--year2--"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -p "Title=--title3--"
${APETAG} -i ${MP3_CLONE} -m read


newtest ContentFromFile
${APETAG} -i ${MP3_CLONE} -m update -f  "Test.sh"=${BIN1}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f  "test.sh"=${BIN1}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -ro "test"
${APETAG} -i ${MP3_CLONE} -m update -rw "test"
${APETAG} -i ${MP3_CLONE} -m read


newtest ContentFromFile2
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN2} -f "NotTest"=${BIN2}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN3} -f "NotTest="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest ContentFromFile3
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN2} -f "NotTest"=${BIN2}
${APETAG} -i ${MP3_CLONE} -m update -ro "Test.sh"
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN3}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -rw "Test.sh"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read


newtest ContentFromFile4
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN2} -f "NotTest"=${BIN2}
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -f "Test.sh=${BIN3}"
${APETAG} -i ${MP3_CLONE} -m read


newtest Test1
${APETAG} -i ${MP3_CLONE} -m update -r  "TestPage=http://bo.gus/addr"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -r  "testpage=http://bo.gus/addr"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -ro "page"
${APETAG} -i ${MP3_CLONE} -m update -rw "page"
${APETAG} -i ${MP3_CLONE} -m read


newtest Test2
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/website/page.html" -r "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/addr/testpage.html" -r "ExternalCover="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m erase
${APETAG} -i ${MP3_CLONE} -m read


newtest Test3
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/website/page.html" -r "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m update -ro "TestPage"
${APETAG} -i ${MP3_CLONE} -m update -p "TestPage=http://bo.gus/addr/testpage.html" -p "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/addr/testpage.html" -r "ExternalCover="
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -rw "TestPage"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite
${APETAG} -i ${MP3_CLONE} -m read


newtest Test4
${APETAG} -i ${MP3_CLONE} -m update -r "TestPage=http://bo.gus/website/page.html" -r "ExternalCover=/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -r "TestPage=http://bo.gus/addr/testpage.html"
${APETAG} -i ${MP3_CLONE} -m read

 
newtest  Test5
${APETAG} -i ${MP3_CLONE} -m update -f "Test.sh"=${BIN1} -p "Test"="File" -r "ExternalCover"="/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN1} -p "test"="File" -r "externalcover"="/dev/zero"
${APETAG} -i ${MP3_CLONE} -m read


newtest  Test6
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN1} -p "test"="file" -r "externalcover"="/dev/zero"
${APETAG} -i ${MP3_CLONE} -m update -ro "test.sh" -ro "test" -ro "externalcover"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN2} -p "test"="FILE2" -r "externalcover"="/DEV/ZERO"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -rw "test.sh" -rw "test" -rw "externalcover"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN2} -p "test"="FILE2" -r "externalcover"="/DEV/ZERO"
${APETAG} -i ${MP3_CLONE} -m read

newtest  Test7
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN1} -p Title="--title2--" -r "testpage"="http://bo.gus/website/page.html"
${APETAG} -i ${MP3_CLONE} -m setro
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m setrw
${APETAG} -i ${MP3_CLONE} -m read

newtest  Test8
${APETAG} -i ${MP3_CLONE} -m update -f "test.sh"=${BIN1} -p Title="--title2--" -r "testpage"="http://bo.gus/website/page.html"
${APETAG} -i ${MP3_CLONE} -m read
${APETAG} -i ${MP3_CLONE} -m overwrite -file ${MP3}
${APETAG} -i ${MP3_CLONE} -m read

newtest  Test9
${APETAG} -i ${MP3_CLONE} -m update -p "ID3"="--title--"
${APETAG} -i ${MP3_CLONE} -m read

newtest  Test10
${APETAG} -i ${MP3_CLONEAPE} -m read


echo "done"
