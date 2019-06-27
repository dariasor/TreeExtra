SHAREDDIR=../shared
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(LIBDIR)
OBJS = Grove.o $(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/TreeNode.o ag_functions.o $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o $(LIBDIR)/thread_pool.o
PGMS = ag_predict ag_train ag_save ag_expand ag_merge ag_fs ag_interactions ag_nway ag_mergepreds ag_savemerge
PGMOBJS = ag_predict.o ag_train.o ag_save.o ag_expand.o ag_merge.o ag_fs.o ag_interactions.o ag_nway.o ag_mergepreds.o ag_savemerge.o
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
$(SHAREDDIR)/SplitInfo.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/LogStream.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/TreeNode.o: $(SHAREDDIR)/INDdata.o

Grove.o: $(SHAREDDIR)/TreeNode.o
ag_functions.o: $(SHAREDDIR)/INDdata.o TrainInfo.h

ag_predict.o: $(OBJS)

ag_predict: ag_predict.o $(OBJS)
	g++ -O3 -o ../Bin/ag_predict ag_predict.o $(OBJS) $(LIBS)

ag_train.o: $(OBJS)

ag_train: ag_train.o $(OBJS)
	g++ -O3 -o ../Bin/ag_train ag_train.o $(OBJS) $(LIBS)

ag_save.o: $(OBJS)

ag_save: ag_save.o $(OBJS)
	g++ -O3 -o ../Bin/ag_save ag_save.o $(OBJS) $(LIBS)	

ag_expand.o: $(OBJS)

ag_expand: ag_expand.o $(OBJS)
	g++ -O3 -o ../Bin/ag_expand ag_expand.o $(OBJS) $(LIBS)

ag_merge.o: $(OBJS)

ag_merge: ag_merge.o $(OBJS)
	g++ -O3 -o ../Bin/ag_merge ag_merge.o $(OBJS)  $(LIBS)

ag_fs.o: $(OBJS)

ag_fs: ag_fs.o $(OBJS)
	g++ -O3 -o ../Bin/ag_fs ag_fs.o $(OBJS) $(LIBS)

ag_interactions.o: $(OBJS)

ag_interactions: ag_interactions.o $(OBJS)	
	g++ -O3 -o ../Bin/ag_interactions ag_interactions.o $(OBJS) $(LIBS)

ag_nway.o: $(OBJS)

ag_nway: ag_nway.o $(OBJS)
	g++ -O3 -o ../Bin/ag_nway ag_nway.o $(OBJS) $(LIBS)

ag_mergepreds.o: $(OBJS)

ag_mergepreds: ag_mergepreds.o $(OBJS)
	g++ -O3 -o ../Bin/ag_mergepreds ag_mergepreds.o $(OBJS) $(LIBS)

ag_savemerge.o: $(OBJS)

ag_savemerge: ag_savemerge.o $(OBJS)
	g++ -O3 -o ../Bin/ag_savemerge ag_savemerge.o $(OBJS) $(LIBS)




