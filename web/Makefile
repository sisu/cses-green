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

TFILE=judge
TSRV=Judge
THRIFT_CPP:=$(TFILE)_constants.cpp  $(TSRV).cpp  $(TFILE)_types.cpp
THRIFT_OBJ:=$(patsubst %.cpp,obj/gen-cpp/%.o,$(THRIFT_CPP))
THRIFT_SRC:=$(patsubst %.o,%.cpp,$(THRIFT_OBJ))

BASEFLAGS:=-Wall -Wextra -std=c++0x -MMD -Wno-unknown-pragmas -I. -I$(ODIR) -Icommon/ -pipe
DFLAGS:=-g
OFLAGS:=-O3
CXXFLAGS:=$(BASEFLAGS) $(DFLAGS)
#CXXFLAGS:=$(BASEFLAGS) $(OFLAGS)
LDFLAGS:=-L /usr/lib/odb/ -lodb -lodb-sqlite -lcppcms -lbooster -lssl -lcrypto -lthrift

.PHONY: all clean

all: $(ODIRS) cses

cses: $(OBJ) $(DBOBJ) $(TMPLOBJ) $(THRIFT_OBJ)
	$(CXX) -o "$@" $^ $(CXXFLAGS) $(LDFLAGS)

$(OBJ): $(ODIR)/%.o: %.cpp $(ODIR)/$(DBFILE)-odb.hxx $(THRIFT_SRC)
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(DBOBJ): %.o: %.cxx $(ODIR)/$(DBFILE)-odb.hxx
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLOBJ): %.o: %.cpp $(ODIR)/$(DBFILE)-odb.hxx
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLCPP): $(ODIR)/%.cpp: %.tmpl
	cppcms_tmpl_cc $< -o $@

$(THRIFT_OBJ): %.o: %.cpp
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(THRIFT_SRC): $(ODIR)/gen-cpp/$(TFILE)_types.h

$(ODIR)/gen-cpp/$(TFILE)_types.h: common/$(TFILE).thrift
	thrift -o $(ODIR) --gen cpp $<

clean:
	rm -rf "$(ODIR)"

$(ODIRS):
	mkdir -p "$@"

$(ODIR)/%-odb.hxx $(ODIR)/%-odb.cxx: $(DBFILE).hxx
	odb -o $(ODIR) -d sqlite -q -s $< -x "-std=c++0x" -x '-Wno-pragmas' -Icommon/

include $(wildcard $(ODIR)/*.d)
