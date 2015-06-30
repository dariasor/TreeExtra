

SHAREDDIR=../shared
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(LIBDIR)
OBJS = Grove.o $(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/TreeNode.o ag_functions.o $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o $(LIBDIR)/thread_pool.o
PGMS = ag_predict ag_train ag_save ag_addbag ag_expand ag_merge ag_fs ag_interactions ag_nway ag_mergepreds ag_savemerge
PGMOBJS = ag_predict.o ag_train.o ag_save.o ag_addbag.o ag_expand.o ag_merge.o ag_fs.o ag_interactions.o ag_nway.o ag_mergepreds.o ag_savemerge.o
LIBS = -lpthread


all: $(PGMS)

clean:
	rm $(OBJS)
	rm $(PGMOBJS)
	rm $(PGMS)

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

ag_predict: ag_predict.o $(OBJS)
	g++ -O3 -o ag_predict ag_predict.o $(OBJS) $(LIBS)

ag_train: ag_train.o $(OBJS)
	g++ -O3 -o ag_train ag_train.o $(OBJS) $(LIBS)

ag_save: ag_save.o $(OBJS)
	g++ -O3 -o ag_save ag_save.o $(OBJS) $(LIBS)	

ag_addbag: ag_addbag.o $(OBJS)
	g++ -O3 -o ag_addbag ag_addbag.o $(OBJS) $(LIBS)

ag_expand: ag_expand.o $(OBJS)
	g++ -O3 -o ag_expand ag_expand.o $(OBJS) $(LIBS)

ag_merge: ag_merge.o $(OBJS)
	g++ -O3 -o ag_merge ag_merge.o $(OBJS)  $(LIBS)

ag_fs: ag_fs.o $(OBJS)
	g++ -O3 -o ag_fs ag_fs.o $(OBJS) $(LIBS)

ag_interactions: ag_interactions.o $(OBJS)	
	g++ -O3 -o ag_interactions ag_interactions.o $(OBJS) $(LIBS)

ag_nway: ag_nway.o $(OBJS)
	g++ -O3 -o ag_nway ag_nway.o $(OBJS) $(LIBS)

ag_mergepreds: ag_mergepreds.o $(OBJS)
	g++ -O3 -o ag_mergepreds ag_mergepreds.o $(OBJS) $(LIBS)

ag_savemerge: ag_savemerge.o $(OBJS)
	g++ -O3 -o ag_savemerge ag_savemerge.o $(OBJS)	$(LIBS)




