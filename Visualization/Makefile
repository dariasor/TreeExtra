SHAREDDIR = ../shared
AGDIR = ../AdditiveGroves
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(AGDIR) -I$(LIBDIR)
OBJS = $(AGDIR)/ag_functions.o $(AGDIR)/Grove.o $(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/TreeNode.o  $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o $(LIBDIR)/thread_pool.o
PGMS = vis_iplot vis_effect
PGMOBJS = vis_iplot.o vis_effect.o
LIBS = -lpthread

all: $(PGMS)

clean:
	rm $(OBJS)
	rm $(PGMOBJS)
	rm $(PGMS)

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

vis_iplot: vis_iplot.o $(OBJS)
	g++ -O3 -o vis_iplot vis_iplot.o $(OBJS) $(LIBS)

vis_effect: vis_effect.o $(OBJS)
	g++ -O3 -o vis_effect vis_effect.o $(OBJS) $(LIBS)
