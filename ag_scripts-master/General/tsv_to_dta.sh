#!/usr/bin/env bash

if [ $# -ne 3 ]
then
    echo "Usage: tsv_to_dta.sh _stem_ _target_ _unused_features_file_"
    exit
fi

stem=$1
features=$stem.feature_names.txt
unused=$3.copy

head -1 $stem.tsv | tr '\t' '\n' > $features

ag_scripts-master/General/rnd_tv_dta --input $stem.tsv --stem $stem --header --files-n 1

ag_scripts-master/General/attrbool $features $stem.dta $stem.attr $2

cp $3 $unused
perl -pi -e 's/(.)$/$1 never/g' $unused
cat $unused >> $stem.attr
rm $features
rm $unused


