#!/usr/bin/env bash

if [ $# -ne 8 ]
then
	echo "Usage: ag_tr_par.sh train valid attr b a n metric data_folder"
	exit
fi

for i in {1..5}
do 
	mkdir AG$i
  	cd AG$i
	nohup nice -10 ag_train -t ../$8/$1 -v ../$8/$2 -r ../$8/$3 -b $4 -a $5 -n $6 -c $7 -i $i -s slow> cout.txt &
	cd .. 	
done   
