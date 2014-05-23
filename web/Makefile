DIRS:=. common
ODIR:=obj
ODIRS:=$(addprefix $(ODIR)/, $(DIRS))
CXX=g++-4.7

SRC:=$(wildcard $(addsuffix /*.cpp,$(DIRS)))
OBJ:=$(patsubst %.cpp,obj/%.o,$(SRC))

DBFILE:=model_db
DBOBJ=$(ODIR)/$(DBFILE)-odb.o

TMPLSRC:=$(wildcard $(addsuffix /*.tmpl,$(DIRS)))
TMPLCPP:=$(patsubst %.tmpl,obj/%.cpp,$(TMPLSRC))
TMPLOBJ:=$(patsubst %.tmpl,obj/%.o,$(TMPLSRC))
OBJ := $(OBJ)

BASEFLAGS:=-Wall -Wextra -std=c++0x -MMD -Wno-unknown-pragmas -I. -I$(ODIR) -Icommon/
DFLAGS:=-g
OFLAGS:=-O3
CXXFLAGS:=$(BASEFLAGS) $(DFLAGS)
#CXXFLAGS:=$(BASEFLAGS) $(OFLAGS)
LDFLAGS:=-L /usr/lib/odb/ -lodb -lodb-sqlite -lcppcms -lbooster -lssl -lcrypto

.PHONY: all clean

all: $(ODIRS) cses

cses: $(OBJ) $(DBOBJ) $(TMPLOBJ)
	$(CXX) -o "$@" $^ $(CXXFLAGS) $(LDFLAGS)

$(OBJ): $(ODIR)/%.o: %.cpp $(ODIR)/$(DBFILE)-odb.hxx
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(DBOBJ): %.o: %.cxx $(ODIR)/$(DBFILE)-odb.hxx
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLOBJ): %.o: %.cpp $(ODIR)/$(DBFILE)-odb.hxx
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLCPP): $(ODIR)/%.cpp: %.tmpl
	cppcms_tmpl_cc $< -o $@

clean:
	rm -rf "$(ODIR)"

$(ODIRS):
	mkdir -p "$@"

$(ODIR)/%-odb.hxx $(ODIR)/%-odb.cxx: $(DBFILE).hxx
	odb -o $(ODIR) -d sqlite -q -s $< -x "-std=c++0x" -x '-Wno-pragmas' -Icommon/

include $(wildcard $(ODIR)/*.d)
