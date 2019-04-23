SHAREDDIR=../shared
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(LIBDIR)
OBJS = Tree.o $(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/TreeNode.o bt_functions.o $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o $(LIBDIR)/thread_pool.o
PGMS = bt_predict bt_train 
PGMOBJS = bt_predict.o bt_train.o 
LIBS = -lpthread

all: directories $(PGMS)

directories:
	mkdir -p ../Bin

clean:
	rm $(OBJS)
	rm $(PGMOBJS)
	rm $(PGMS)

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

$(SHAREDDIR)/INDdata.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/functions.o: $(SHAREDDIR)/definitions.h

bt_predict.o: $(OBJS)

bt_predict: bt_predict.o $(OBJS)
	g++ -O3 -o ../Bin/bt_predict bt_predict.o $(OBJS) $(LIBS)

bt_train.o: $(OBJS)

bt_train: bt_train.o $(OBJS)
	g++ -O3 -o ../Bin/bt_train bt_train.o $(OBJS) $(LIBS)