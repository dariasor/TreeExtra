cut $1 -f$2 | sort | uniq -c > sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/^ //g' sush.temp1
perl -pi -e 's/ /\t/' sush.temp1
cat sush.temp1 | sort -g -k 1 -r