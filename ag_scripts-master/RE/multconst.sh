#!/usr/bin/env bash


if [ $# -ne 3 ]
then
    echo "Usage: multconst.sh _input_ _col1_ _const_"
    exit
fi

cut -f$2 $1 | multconst.pl $3


