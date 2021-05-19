CFLAGS= -Wall -O3 -g -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers -D_FILE_OFFSET_BITS=64
CXXFLAGS=$(CFLAGS)

TARGET   = scrunner

# Imagemagic flags, only needed if actually compiled.
MAGICK_CXXFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)
AV_CXXFLAGS=$(shell pkg-config --cflags  libavcodec libavformat libswscale libavutil)
AV_LDFLAGS=$(shell pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil)



RGB_INCDIR=$(LIB_DISTRIBUTION)/include
LDFLAGS+=-L$(RGB_LIBDIR) -lrt -lm -lpthread 

INCLUD=-I$(RGB_INCDIR)
SRC      = $(wildcard *.cpp) $(wildcard *.c) $(wildcard *.cc)


OBJECTS = $(SRC:.c=.o) 


all: $(TARGET) 

$(TARGET): $(OBJECTS) 
	$(CXX) $(INCLUD) $(CXXFLAGS) $(OBJECTS) -g3 -o $@ $(LDFLAGS) $(AV_LDFLAGS) $(MAGICK_LDFLAGS)

%.o: %.c 
	$(CXX)  $(INCLUD) $(CXXFLAGS) $(MAGICK_CXXFLAGS) $(AV_CXXFLAGS) -c -g3 -o $@ $<

%.o : %.cc
	$(CXX) $(INCLUD) $(CXXFLAGS) $(MAGICK_CXXFLAGS) $(AV_CXXFLAGS) -c -g3 -o $@ $<
 

FORCE:
.PHONY: FORCE all build clean debug release

build: 
	@mkdir -p $(BUILD)

debug: CFLAGS += -DDEBUG -g
debug: all

release: CFLAGS += -O2
release: all

clean:
	rm -f $(OBJECTS) 
# $(TARGET)
distclean:
	-@rm -rvf $(BUILD)
