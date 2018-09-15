#!/usr/bin/env bash

if [ $# -ne 3 ]
then
	echo "Usage: ag_exp_par.sh b a n"
	exit
fi

for i in {1..5}
do 
  	cd AG$i
	nohup nice -10 ag_expand -b $1 -a $2 -n $3 > cout.txt &
	cd .. 	
done   
