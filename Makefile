C_SRC=testlib.c sdlmain.c realmain.c
SCM_LIB_SRC=link.scm
SCM_DYNLIB_SRC=math.scm common.scm scmlib.scm
GAMBIT_ROOT?=/usr/local/Gambit-C
GSC=$(GAMBIT_ROOT)/bin/gsc -:s
XML_INCLUDE:=-I/usr/include/libxml2
CFLAGS+=-I$(GAMBIT_ROOT)/include `sdl-config --cflags` $(XML_INCLUDE)
SDL_LIBS:=`sdl-config --libs` -lSDL_image
LDFLAGS=$(SDL_LIBS) -L$(GAMBIT_ROOT)/lib -lxml2

MKMOD=make -f Mkmod
MAKE_XML2=$(MKMOD) SCM_SRC=xml2.scm OUTPUT=xml2 CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"


SCM_LIB_C=$(patsubst %.scm,%.c,$(SCM_LIB_SRC)) \
	link_.c

SCM_OBJ=$(patsubst %.c,%.o,$(SCM_LIB_C))
C_OBJS=$(patsubst %.c,%.o,$(C_SRC))
SCM_DYN_OBJ=$(patsubst %.scm,%.o1,$(SCM_DYNLIB_SRC))

all: sdlmain xml2.o1.o $(SCM_DYN_OBJ)


$(SCM_LIB_C): $(SCM_LIB_SRC)
	$(GSC) -f -link -track-scheme $(SCM_LIB_SRC)

$(SCM_OBJ): $(SCM_LIB_C)
	$(GSC) -cc-options "-D___DYNAMIC $(CFLAGS)" -obj $(SCM_LIB_C)

%.o1: %.scm
	$(GSC) -o $@ $<

sdlmain: $(SCM_OBJ) $(C_OBJS)
	$(CC) $(CFLAGS) -o $@ $(C_OBJS) $(SCM_OBJ) $(LDFLAGS) -lgambc

clean:
	rm -f *.o* $(SCM_LIB_C) sdlmain
	$(MAKE_XML2) clean

test_bin: testlib.o testlib_test.o
	$(CC) $(CFLAGS) -o $@ testlib.o testlib_test.o $(LDFLAGS)

test: test_bin
	./test_bin

xml2.o1.o: xml2.scm
	$(MAKE_XML2)

.phony: all

