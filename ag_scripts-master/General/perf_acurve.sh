#!/usr/bin/env bash 
echo -e -n 

if [ $# -ne 5 ]
then
    echo "Usage: perf_table.sh a_max a_min b metric targets"
    exit
fi

for a in 1 0.5 0.2 0.1 0.05 0.02 0.01 0.005 0.002 0.001 0.0005 0.0002 0.0001 0.00005 0.00002 0.00001 0.000005 0.0
do
	abool1=`echo "$1 >= $a" | bc`	
	abool2=`echo "$2 <= $a" | bc`

	if [ $abool1 -eq 1 -a $abool2 -eq 1 ]
	then
	    params=a.$a.b.$3
	    echo -e -n "$a\t"
	    $4 $5 preds.$params.txt | awk '{printf "%s", $2}'
	    echo
	fi
done
