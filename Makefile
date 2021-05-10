
WIGIG_INCLUDE=-I$(WIGIG_DIR)/include
WIGIG_LIB=-L$(WIGIG_DIR)/lib

ROOT = ./
CXX = g++
CFLAGS = -O3 -DC_LINUX
CXXFLAGS += -Wall -c -g
LIBS +=$(WIGIG_LIB)

INCLUDES +=$(WIGIG_INCLUDE) -I$(ROOT)

LIBRARIES += -lm -lpthread -ldl -lusb-1.0  #-lz
#LIBRARIES += `pkg-config --libs opencv`

OBJS =  mcslwigig.o mcsl_api.o SampleTransfer.o
#OBJS = main.o
OUT=MCSLshvc

all: $(OUT)

$(OUT): $(OBJS)
	$(CXX) -Wall $(LIBS) -o $@ $^ $(LIBRARIES)

%.o: $(ROOT)%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -std=c++11 $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(OUT)
