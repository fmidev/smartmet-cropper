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
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) $(EXTRAFLAGS)
CFLAGS_RELEASE = -DUNIX -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = 
ARFLAGS = -r
INCLUDES = -I $(includedir) -I $(includedir)/newbase -I $(includedir)/webauthenticator -I $(includedir)/imagine -I /usr/include/freetype2
LIBS = -L$(libdir) -static -limagine -lnewbase -lwebauthenticator -lfreetype -ljpeg -lpng -lz

# Common library compiling template

include ../../makefiles/makefile.prog

