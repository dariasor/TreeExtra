#!/usr/bin/env bash

if [ $# -ne 1 ]
then
	echo "Usage: ag_mrg.sh _phaseNo_"
	exit
fi

mkdir Phase$1
cp -r AG* Phase$1

mkdir Merged
cd Merged
ag_merge -d ../AG1 ../AG2 ../AG3 ../AG4 ../AG5
tail performance.txt
cd ..

