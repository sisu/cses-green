DIRS:=.
ODIR:=obj
ODIRS:=$(addprefix $(ODIR)/, $(DIRS))

SRC:=$(wildcard $(addsuffix /*.cpp,$(DIRS)))
OBJ:=$(patsubst %.cpp,obj/%.o,$(SRC))

DBFILE:=model_db
DBOBJ=$(ODIR)/$(DBFILE)-odb.o

TMPLSRC:=$(wildcard $(addsuffix /*.tmpl,$(DIRS)))
TMPLCPP:=$(patsubst %.tmpl,obj/%.cpp,$(TMPLSRC))
TMPLOBJ:=$(patsubst %.tmpl,obj/%.o,$(TMPLSRC))
OBJ := $(OBJ)

BASEFLAGS:=-Wall -Wextra -std=c++0x -MMD -Wno-unknown-pragmas -I. -I$(ODIR)
DFLAGS:=-g
OFLAGS:=-O3
CXXFLAGS:=$(BASEFLAGS) $(DFLAGS)
#CXXFLAGS:=$(BASEFLAGS) $(OFLAGS)
LDFLAGS:=-L /usr/lib/odb/ -lodb -lodb-sqlite -lcppcms -lbooster -lssl -lcrypto

.PHONY: all clean

all: $(ODIRS) cses

cses: $(OBJ) $(DBOBJ) $(TMPLOBJ)
	g++ -o "$@" $^ $(CXXFLAGS) $(LDFLAGS)

$(OBJ): $(ODIR)/%.o: %.cpp $(ODIR)/$(DBFILE)-odb.hxx
	g++ "$<" -c -o "$@" $(CXXFLAGS)

$(DBOBJ): %.o: %.cxx
	g++ "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLOBJ): %.o: %.cpp
	g++ "$<" -c -o "$@" $(CXXFLAGS)

$(TMPLCPP): $(ODIR)/%.cpp: %.tmpl
	cppcms_tmpl_cc $< -o $@

clean:
	rm -rf "$(ODIR)"

$(ODIRS):
	mkdir -p "$@"

$(ODIR)/%-odb.hxx $(ODIR)/%-odb.cxx: $(DBFILE).hxx
	odb -o $(ODIR) -d sqlite -q -s $< -x "-std=c++0x" -x '-Wno-pragmas'

include $(wildcard $(ODIR)/*.d)
