cat - > hsort_temp
head -1 < hsort_temp
tail -n +2 < hsort_temp | sort
rm hsort_temp
