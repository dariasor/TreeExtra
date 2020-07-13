SHAREDDIR=../shared
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(LIBDIR)
OBJS = Tree.o bt_functions.o $(LIBDIR)/thread_pool.o \
	$(SHAREDDIR)/SplitInfo.o $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o \
	$(SHAREDDIR)/TreeNode.o $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o # XW
PGMS = gbt_train # bt_predict bt_train
PGMOBJS = gbt_train.o # bt_predict.o bt_train.o
LIBS = -lpthread

all: directories $(PGMS)

directories:
	mkdir -p ../Bin

clean:
	# XW. Should not abort if the files do not exist
	rm -f $(OBJS)
	rm -f $(PGMOBJS)
	rm -f ../Bin/bt_predict ../Bin/bt_train ../Bin/gbt_train

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

$(SHAREDDIR)/INDdata.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/INDsample.o: $(SHAREDDIR)/definitions.h # XW
$(SHAREDDIR)/functions.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/SplitInfo.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/LogStream.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/TreeNode.o: $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o # XW

bt_functions.o: $(SHAREDDIR)/definitions.h
Tree.o: $(SHAREDDIR)/TreeNode.o

bt_predict.o: $(OBJS)

bt_predict: bt_predict.o $(OBJS)
	g++ -O3 -o ../Bin/bt_predict bt_predict.o $(OBJS) $(LIBS)

bt_train.o: $(OBJS)

bt_train: bt_train.o $(OBJS)
	g++ -O3 -o ../Bin/bt_train bt_train.o $(OBJS) $(LIBS)

gbt_train.o: $(OBJS)

gbt_train: gbt_train.o $(OBJS)
	g++ -O3 -o ../Bin/gbt_train gbt_train.o $(OBJS) $(LIBS)

