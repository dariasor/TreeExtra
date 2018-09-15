#!/usr/bin/env bash  

if [ $# -ne 1 ]
then
    echo "Usage: create_dec_col.sh _#lines_"
    exit
fi

n=$1
for (( i=n; i>0; i-- ))
do
    echo $i
done
