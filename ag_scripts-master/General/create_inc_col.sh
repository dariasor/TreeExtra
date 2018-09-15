#!/usr/bin/env bash  

if [ $# -ne 1 ]
then
    echo "Usage: create_inc_col.sh _#lines_"
    exit
fi

n=$1
for (( i=1; i<=n; i++ ))
do
    echo $i
done