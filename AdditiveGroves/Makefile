SHAREDDIR=../shared
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(LIBDIR)
LIBS = -lpthread


OBJS = Grove.o ag_functions.o $(LIBDIR)/thread_pool.o \
	$(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o \
	$(SHAREDDIR)/TreeNode.o $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o
ADDOBJS = $(OBJS) ag_layeredjob.o
PGMS = ag_predict ag_train ag_save ag_expand ag_merge ag_fs \
	ag_interactions ag_nway ag_mergepreds ag_savemerge
PGMOBJS = ag_predict.o ag_train.o ag_save.o ag_expand.o ag_merge.o ag_fs.o \
	ag_interactions.o ag_nway.o ag_mergepreds.o ag_savemerge.o
# XW. List of programs where parallel bagging is needed
# PGMS = ag_train ag_expand ag_fs ag_interactions ag_nway
# PGMOBJS = ag_train.o ag_expand.o ag_fs.o ag_interactions.o ag_nway.o

all: directories $(PGMS)

directories:
	mkdir -p ../Bin

clean:
	# XW. Should not abort if the files do not exist
	rm -f $(OBJS)
	rm -f $(PGMOBJS)
	rm -rf ../Bin/ag_train ../Bin/ag_expand ../Bin/ag_fs ../Bin/ag_interactions \
		../Bin/ag_save ../Bin/ag_nway

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

# XW. Put thread-specific data into a separate class
$(SHAREDDIR)/INDdata.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/INDsample.o: $(SHAREDDIR)/definitions.h

$(SHAREDDIR)/functions.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/SplitInfo.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/LogStream.o: $(SHAREDDIR)/definitions.h
$(SHAREDDIR)/TreeNode.o: $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o # XW

Grove.o: $(SHAREDDIR)/TreeNode.o
ag_functions.o: $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o TrainInfo.h # XW
ag_layeredjob.o: $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/INDsample.o TrainInfo.h # XW

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

ag_fs.o: $(ADDOBJS)

ag_fs: ag_fs.o $(ADDOBJS)
	g++ -O3 -o ../Bin/ag_fs ag_fs.o $(ADDOBJS) $(LIBS)

ag_interactions.o: $(ADDOBJS)

ag_interactions: ag_interactions.o $(ADDOBJS)
	g++ -O3 -o ../Bin/ag_interactions ag_interactions.o $(ADDOBJS) $(LIBS)

ag_nway.o: $(ADDOBJS)

ag_nway: ag_nway.o $(ADDOBJS)
	g++ -O3 -o ../Bin/ag_nway ag_nway.o $(ADDOBJS) $(LIBS)

ag_mergepreds.o: $(OBJS)

ag_mergepreds: ag_mergepreds.o $(OBJS)
	g++ -O3 -o ../Bin/ag_mergepreds ag_mergepreds.o $(OBJS) $(LIBS)

ag_savemerge.o: $(OBJS)

ag_savemerge: ag_savemerge.o $(OBJS)
	g++ -O3 -o ../Bin/ag_savemerge ag_savemerge.o $(OBJS) $(LIBS)




