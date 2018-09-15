#!/usr/bin/env bash


if [ $# -ne 2 ]
then
    echo "Usage: bigints.sh _input_ _output_"
    exit
fi

cp $1 temp

for i in 137 138 206 207 276 277 338 390 391 419 420 466 469 472 534 537 626 627 695 698
do
	cut -f$i temp | perl bigint.pl | paste temp - > temp2
	rm temp
	mv temp2 temp
done

mv temp $2