CROSS := mingw32-
#CROSS := i686-w64-mingw32-
#CROSS := x86_64-w64-mingw32-

CC      = $(CROSS)gcc
CCX     = $(CROSS)g++
LD      = $(CROSS)ld
AR      = $(CROSS)ar

CFLAGS  = -O3 -include ./unvc.h -Wno-multichar
LDFLAGS = -O3 
CFLAGS_CPP = -O3 -include ./unvc.h -Wno-multichar

#INC = -I./Include/DShow
INC = -I./Include

ODIR := .
SDIR := .

LDLIBS  = 

PROG  = libvideoInput.a

#PKG_CONFIG = $(CROSS)pkg-config

# Object files .o necessary to build the main program
_OBJS  = videoInput2.o 
		

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
 
all: $(PROG)

# Compilation and link
$(PROG): $(OBJS)
	ar r $(PROG) $(OBJS)
	ranlib $(PROG)


$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CCX) -c -o $@ $< $(CFLAGS_CPP) $(INC)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) 

clean:
	-rm -f $(PROG)
	-rm -f $(ODIR)/*.o
