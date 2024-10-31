#!/bin/sh

if test -n $1;then
    ./build/"$1"_test
    exit
fi

./build/lexer_test
./build/parser_test
