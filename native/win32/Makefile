CROSS := mingw32-
#CROSS := i686-w64-mingw32-
#CROSS := x86_64-w64-mingw32-

CC      = $(CROSS)gcc
CCX     = $(CROSS)g++
LD      = $(CROSS)ld
AR      = $(CROSS)ar

CFLAGS  = -O2 
LDFLAGS = -O2 
CFLAGS_CPP = -O2 

LIBS_PATH = ../libs

#dxguid.lib ole32.lib  strmiids.lib uuid.lib
LIBS = $(LIBS_PATH)/dxguid.lib $(LIBS_PATH)/ole32.lib $(LIBS_PATH)/oleaut32.lib \
		$(LIBS_PATH)/strmiids.lib $(LIBS_PATH)/uuid.lib

ODIR := ../obj
SDIR := ../src

LDLIBS  = $(LIBS_PATH)/FlashRuntimeExtensions.a $(LIBS_PATH)/libvideoInput.a $(LIBS)

PROG  = capane.dll


_OBJS  = CaptureANE.o Capture.o impl/CaptureImplDirectShow.o CaptureSurface.o CCapture.o
		

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
 
all: $(PROG)

# Compilation and link
$(PROG): $(OBJS)
	$(CCX) -shared -static-libgcc -static-libstdc++ -o $(PROG) $(OBJS) $(LDFLAGS) $(LDLIBS)


$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CCX) -c -o $@ $< $(CFLAGS_CPP)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) 

clean:
	-rm -f $(PROG)
	-rm -f $(ODIR)/*.o
	-rm -f $(ODIR)/impl/*.o
