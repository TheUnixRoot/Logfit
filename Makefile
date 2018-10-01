CXX=g++
CFLAGS = -std=c++11 -Wno-deprecated-declarations
INCLUDES = -I /opt/intel/opencl/include -I ../tbb-2018_U2/include
LIBRARIES = -L /opt/intel/opencl
LIBS = -ltbb -lOpenCL -lpthread -lm 

all: logfit logfitDebug 

logfit: ./source/cpp/main.cpp ./source/cpp/*.h 
	$(CXX) $(CFLAGS) ./source/cpp/main.cpp $(INCLUDES) $(LIBRARIES) $(LIBS) -o logfit

logfitDebug: ./source/cpp/main.cpp ./source/cpp/*.h 
	$(CXX) $(CFLAGS) -DDEBUG ./source/cpp/main.cpp $(INCLUDES) $(LIBRARIES) $(LIBS) -o logfitDebug
