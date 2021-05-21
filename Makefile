CFLAGS= -Wall -O3 -g -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers --std=c11 -std=gnu99
CXXFLAGS=$(CFLAGS)
CC		 := $(CROSS_COMPILE)gcc

TARGET   = scrunner

# Imagemagic flags, only needed if actually compiled.


RGB_INCDIR=$(LIB_DISTRIBUTION)/include
LDFLAGS+=-L$(RGB_LIBDIR) -lrt -lm -lpthread 

INCLUD=-I$(RGB_INCDIR)
SRC      = $(wildcard *.cpp) $(wildcard *.c) $(wildcard *.cc)


OBJECTS = $(SRC:.c=.o) 


all: $(TARGET) 

$(TARGET): $(OBJECTS) 
	$(CC) $(INCLUD) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c 
	$(CC)  $(INCLUD) $(CXXFLAGS) -c -o $@ $<

%.o : %.cc
	$(CC) $(INCLUD) $(CXXFLAGS) -c -o $@ $<
 

FORCE:
.PHONY: FORCE


clean:
	rm -f $(OBJECTS) 
# $(TARGET)

