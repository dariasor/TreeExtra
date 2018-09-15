#!/usr/bin/env bash


if [ $# -ne 3 ]
then
    echo "Usage: blockend.sh _input_ _col1_ _col2_"
    exit
fi

cut -f$2 $1 > temp
cut -f$3 $1 | paste temp - | last_in_block.pl

rm temp
