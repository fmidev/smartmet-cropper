HTML = cropper
PROG = cropper cropper_auth

MAINFLAGS = -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wredundant-decls -Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code

CC = g++

# Default compiler flags

CFLAGS = -DUNIX -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = -s

# Special modes

CFLAGS_DEBUG = -DUNIX -O0 -g $(MAINFLAGS) $(EXTRAFLAGS) -Werror
CFLAGS_PROFILE = -DUNIX -O2 -g -pg -DNDEBUG $(MAINFLAGS)

LDFLAGS_DEBUG =
LDFLAGS_PROFILE =

INCLUDES = -I $(includedir) \
	-I $(includedir)/smartmet \
	-I $(includedir)/smartmet/newbase \
	-I $(includedir)/smartmet/imagine \
	-I $(includedir)/smartmet/webauthenticator

LIBS = -L$(libdir) \
	-lsmartmet_imagine \
	-lsmartmet_newbase \
	-lsmartmet_webauthenticator \
	-lfreetype -ljpeg -lpng -lz

# -lboost_iostreams

# Common library compiling template

# Installation directories

prosessor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(prosessor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

objdir = obj
includedir = $(PREFIX)/include

ifeq ($(origin BINDIR), undefined)
  bindir = $(PREFIX)/bin
else
  bindir = $(BINDIR)
endif

# rpm variables

CWP = $(shell pwd)
BIN = $(shell basename $(CWP))
rpmsourcedir = /smartmet/src/redhat/SOURCES
rpmerr = "There's no spec file ($(specfile)). RPM wasn't created. Please make a spec file or copy and rename it into $(specfile)"

rpmversion := $(shell grep "^Version:" $(HTML).spec  | cut -d\  -f 2 | tr . _)
rpmrelease := $(shell grep "^Release:" $(HTML).spec  | cut -d\  -f 2 | tr . _)

# Special modes

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_DEBUG)
  LDFLAGS = $(LDFLAGS_DEBUG)
endif

ifneq (,$(findstring profile,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_PROFILE)
  LDFLAGS = $(LDFLAGS_PROFILE)
endif

# Compilation directories

vpath %.cpp source
vpath %.h include
vpath %.o $(objdir)

# How to install

INSTALL_PROG = install -m 775
INSTALL_DATA = install -m 664

# The files to be compiled

SRCS = $(patsubst source/%,%,$(wildcard *.cpp source/*.cpp))
HDRS = $(patsubst include/%,%,$(wildcard *.h include/*.h))
OBJS = $(SRCS:%.cpp=%.o)

OBJFILES = $(OBJS:%.o=obj/%.o)

MAINSRCS = $(PROG:%=%.cpp)
SUBSRCS = $(filter-out $(MAINSRCS),$(SRCS))
SUBOBJS = $(SUBSRCS:%.cpp=%.o)
SUBOBJFILES = $(SUBOBJS:%.o=obj/%.o)

INCLUDES := -I include $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(PROG)
debug: objdir $(PROG)
release: objdir $(PROG)
profile: objdir $(PROG)

$(PROG): % : $(SUBOBJS) %.o
	$(CC) $(LDFLAGS) -o $@ obj/$@.o $(SUBOBJFILES) $(LIBS)

clean:
	rm -f $(PROG) $(OBJFILES) *~ source/*~ include/*~

install:
	mkdir -p $(bindir)
	@list='$(PROG)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done

depend:
	makedepend $(INCLUDES)

test:
	cd test && make test

html::
	mkdir -p ../../../../html/bin/$(HTML)
	doxygen $(HTML).dox

objdir:
	@mkdir -p $(objdir)

rpm: clean depend
	if [ -a $(BIN).spec ]; \
	then \
	  tar -C ../ -cf $(rpmsourcedir)/smartmet-$(BIN).tar $(BIN) ; \
	  gzip -f $(rpmsourcedir)/smartmet-$(BIN).tar ; \
	  rpmbuild -ta $(rpmsourcedir)/smartmet-$(BIN).tar.gz ; \
	else \
	  echo $(rpmerr); \
	fi;

tag:
	cvs -f tag 'smartmet_$(HTML)_$(rpmversion)-$(rpmrelease)' .


.SUFFIXES: $(SUFFIXES) .cpp

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $(objdir)/$@ $<

# -include Dependencies
# DO NOT DELETE THIS LINE -- make depend depends on it.
