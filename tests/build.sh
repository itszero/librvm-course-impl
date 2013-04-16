#!/bin/sh
[[ -d "tmp" ]] && rm -r tmp
mkdir tmp
cd tmp
gcc -g -o `basename $1` ../$1 ../../librvm.a -I../../ -I../src/testcase1
cd ..
