CXX=g++
CFLAGS = -std=c++11 -Wno-deprecated-declarations
INCLUDES = -I /opt/intel/opencl/include -I /opt/intel/tbb44_20151115oss/include
LIBRARIES = -L /opt/intel/opencl
LIBS = -ltbb -lOpenCL -lpthread -lm 

all: logfit logfitDebug 

logfit: main.cpp *.h 
	$(CXX) $(CFLAGS) main.cpp $(INCLUDES) $(LIBRARIES) $(LIBS) -o logfit

logfitDebug: main.cpp *.h 
	$(CXX) $(CFLAGS) -DDEBUG main.cpp $(INCLUDES) $(LIBRARIES) $(LIBS) -o logfitDebug
