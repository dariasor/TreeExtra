perl -pi -e 's/\t\t/\t0\t/g' $1
perl -pi -e 's/\t\t/\t0\t/g' $1
perl -pi -e 's/^\t/0\t/g' $1
perl -pi -e 's/\t\n/\t0\n/g' $1
perl -pi -e 's/\t$/\t0/g' $1