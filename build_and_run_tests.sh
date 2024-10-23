#!/bin/sh

make -C build

if test -n $1;then
    ./build/"$1"_test
    exit
fi
