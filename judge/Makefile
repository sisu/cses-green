DIRS:=. common
ODIR:=obj
ODIRS:=$(addprefix $(ODIR)/, $(DIRS))
CXX=g++-4.7

SRC:=$(wildcard $(addsuffix /*.cpp,$(DIRS)))
OBJ:=$(patsubst %.cpp,obj/%.o,$(SRC))

TFILE=judge
TSRV=Judge
THRIFT_CPP:=$(TFILE)_constants.cpp  $(TSRV).cpp  $(TFILE)_types.cpp
THRIFT_OBJ:=$(patsubst %.cpp,obj/gen-cpp/%.o,$(THRIFT_CPP))
THRIFT_SRC:=$(patsubst %.o,%.cpp,$(THRIFT_OBJ))

BASEFLAGS:=-Wall -Wextra -std=c++0x -MMD -Wno-unknown-pragmas -I. -I$(ODIR) -Icommon/
DFLAGS:=-g
OFLAGS:=-O3
CXXFLAGS:=$(BASEFLAGS) $(DFLAGS)
#CXXFLAGS:=$(BASEFLAGS) $(OFLAGS)
LDFLAGS:=-lthrift -lcppcms -lcrypto -lpthread

.PHONY: all clean

all: $(ODIRS) judge syscalls/restrict_syscalls

judge: $(OBJ) $(THRIFT_OBJ)
	$(CXX) -o "$@" $^ $(CXXFLAGS) $(LDFLAGS)

$(OBJ): $(ODIR)/%.o: %.cpp $(THRIFT_SRC)
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(THRIFT_OBJ): %.o: %.cpp
	$(CXX) "$<" -c -o "$@" $(CXXFLAGS)

$(THRIFT_SRC): $(ODIR)/gen-cpp/$(TFILE)_types.h

$(ODIR)/gen-cpp/$(TFILE)_types.h: common/$(TFILE).thrift
	thrift -o $(ODIR) --gen cpp $<

syscalls/restrict_syscalls: syscalls/%: syscalls/%.cpp
	$(CXX) "$<" -o "$@" -Wall -Wextra -std=c++0x

clean:
	rm -rf "$(ODIR)"

$(ODIRS):
	mkdir -p "$@"

include $(wildcard $(ODIR)/*.d)
