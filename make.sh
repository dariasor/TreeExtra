rm -f shared/*.o
rm -f AdditiveGroves/*.o
rm -f BaggedTrees/*.o
rm -f Visualization/*.o
cd AdditiveGroves
make --makefile Makefile
cd ../BaggedTrees
make --makefile Makefile
cd ../Visualization
make --makefile Makefile
