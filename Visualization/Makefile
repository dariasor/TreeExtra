SHAREDDIR = ../shared
AGDIR = ../AdditiveGroves
LIBDIR=../ThreadPool
CXXFLAGS = -I$(SHAREDDIR) -I$(AGDIR) -I$(LIBDIR)
OBJS = $(AGDIR)/ag_functions.o $(AGDIR)/Grove.o $(SHAREDDIR)/SplitInfo.o  $(SHAREDDIR)/INDdata.o $(SHAREDDIR)/TreeNode.o  $(SHAREDDIR)/functions.o $(SHAREDDIR)/LogStream.o $(LIBDIR)/thread_pool.o
PGMS = vis_iplot vis_effect
PGMOBJS = vis_iplot.o vis_effect.o
LIBS = -lpthread

all: directories $(PGMS)

directories:
	mkdir -p ../Bin

clean:
	rm -f $(OBJS)
	rm -f $(PGMOBJS)
	rm -f $(PGMS)

.cpp.o:
	g++ -O3 $(CXXFLAGS) -c $< -o $@

vis_iplot.o: $(OBJS)

vis_effect.o: $(OBJS)

vis_iplot: vis_iplot.o $(OBJS)
	g++ -O3 -o ../Bin/vis_iplot vis_iplot.o $(OBJS) $(LIBS)

vis_effect: vis_effect.o $(OBJS)
	g++ -O3 -o ../Bin/vis_effect vis_effect.o $(OBJS) $(LIBS)
