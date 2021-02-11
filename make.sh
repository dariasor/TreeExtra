rm shared/*.o
rm AdditiveGroves/*.o
rm BaggedTrees/*.o
rm Visualization/*.o
cd AdditiveGroves
gmake --makefile Makefile
cd ../BaggedTrees
gmake --makefile Makefile
cd ../Visualization
gmake --makefile Makefile
