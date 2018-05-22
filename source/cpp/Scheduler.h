//============================================================================
// Name			: Scheduler.h
// Author		: Antonio Vilches
// Version		: 1.0
// Date			: 26 / 12 / 2014
// Copyright	: Department. Computer's Architecture (c)
// Description	: Main scheduler interface class
//============================================================================

//#define ENERGYCOUNTERS

#include <cstdlib>
#include <iostream>
#include <fstream>

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "DataStructures.h"

using namespace std;
using namespace tbb;


/*****************************************************************************
 * Global Variables For OpenCL
 * **************************************************************************/

cl_int error;
cl_uint num_max_platforms;
cl_uint num_max_devices;
cl_uint num_platforms;
cl_uint num_devices;
cl_platform_id platforms_id;
cl_device_id device_id;
cl_context context;
cl_command_queue command_queue;
cl_program program;
cl_kernel kernel;
int computeUnits;
size_t vectorization;

/*****************************************************************************
 * OpenCL fucntions
 * **************************************************************************/

char *ReadSources(char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
    	cout << "ERROR: Failed to open file \'" << fileName << "\'" << endl;
        return NULL;
    }

    if (fseek(file, 0, SEEK_END))
    {
    	cout << "ERROR: Failed to seek file \'" << fileName << "\'" << endl;
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size == 0)
    {
        cout <<"ERROR: Failed to check position on file \'" << fileName << "\'" << endl;
        fclose(file);
        return NULL;
    }

    rewind(file);

    char *src = (char *)malloc(sizeof(char) * size + 1);
    if (!src)
    {
        cout <<"ERROR: Failed to allocate memory for file \'" << fileName << "\'" << endl;
        fclose(file);
        return NULL;
    }

    cout << "Reading file \'" << fileName << "\' (size " << size << " bytes)" << endl;
    size_t res = fread(src, 1, sizeof(char) * size, file);
    if (res != sizeof(char) * size)
    {
        cout << "ERROR: Failed to read file \'" << fileName << "\'" << endl;
        fclose(file);
        free(src);
        return NULL;
    }
	/* NULL terminated */
    src[size] = '\0'; 
    fclose(file);

    return src;
}

void createCommandQueue() {
	num_max_platforms = 1;
	error = clGetPlatformIDs(num_max_platforms, &platforms_id, &num_platforms);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No platforms were found.\n");
		exit(0);
	}
	num_max_devices = 1;
	error = clGetDeviceIDs(platforms_id, CL_DEVICE_TYPE_GPU, num_max_devices, &device_id, &num_devices);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No devices were found\n");
		exit(0);
	}
	error = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &computeUnits, NULL);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No devices were found");
		exit(0);
	}

	char device_name[50];
	error = clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(char)*50, &device_name, NULL);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No devices were found");
		exit(0);
	}

	cerr << "GPU's name: " << device_name << " with "<< computeUnits << " computes Units" << endl;
	num_devices=1;
	context = clCreateContext(NULL, num_devices, &device_id, NULL, NULL,
			&error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Context couldn't be created");
		exit(0);
	}

	command_queue = clCreateCommandQueue/*WithProperties*/(context, device_id, CL_QUEUE_PROFILING_ENABLE, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Command Queue couldn't be created");
		exit(0);
	}
}

void CreateAndCompileProgram(char * kname, char * filename) {

	char * programSource = ReadSources(filename);

	// Create a program using clCreateProgramWithSource()
	program = clCreateProgramWithSource(context, 1,
			(const char**) &programSource, NULL, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Failed creating programm with source!\n");
		exit(0);
	}
	// Build (compile) the program for the devices with // clBuildProgram()
	error = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Failed Building Program!\n");
		char message[16384];
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
				sizeof(message), message, NULL);
		fprintf(stderr,"Message Error:\n %s\n",message);
		exit(0);
	}
	// Use clCreateKernel() to create a kernel from the
	// vector addition function (named "vecadd")
	kernel = clCreateKernel(program, kname, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Failed Creating programm!\n");
		exit(0);
	}
	
	clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &vectorization, NULL);
	cerr << "Preferred vectorization: " << vectorization << endl;
}

/*****************************************************************************
 * Base Scheduler class
 * **************************************************************************/

/*This Scheduler Base class implementation follows a Singleton pattern*/
template <class T>
class Scheduler{
protected:
// Class members
	//Scheduler Itself
	static T *instance;
	task_scheduler_init *init;
	int nCPUs;
	int nGPUs;
	//timing
	tick_count start, end;
	float runtime;
    Params * p;
//End class members

	/*Scheduler Constructor, forbidden access to this constructor from outside*/
	Scheduler(void * params) {
		p = (Params *) params;
		nCPUs = p->numcpus;
		nGPUs = p->numgpus;
#ifdef DEBUG
		cerr << "TBB scheduler is active " << "(" << nCPUs << ", " << nGPUs << ")" << endl;
#endif
		init = new task_scheduler_init(nCPUs + nGPUs);
#ifdef DEBUG
		cerr << "iNITIALIZING OPENCL" << endl;
#endif
		initializeOPENCL(p->kernelName);
		runtime = 0.0;
	}

	/*Initialize OpenCL environment*/
	void initializeOPENCL(char * kernelName){
		createCommandQueue();
        CreateAndCompileProgram(kernelName, p->openclFile);

	}

 
public:
	/*Class destructor*/
	~Scheduler(){
		init->~task_scheduler_init();
		delete instance;
		instance = NULL;
	}

	/*This function creates only one instance per process, if you want a thread safe behavior protect the if clausule with a Lock*/
	static T * getInstance(void * params){
		if(! instance){
			instance = new T(params);
		}
		return instance;
	}

	/*Sets the start mark of energy and time*/
	void startTimeAndEnergy(){
		start = tick_count::now();
	}

	/*Sets the end mark of energy and time*/
	void endTimeAndEnergy(){
		end = tick_count::now();
		runtime = (end-start).seconds()*1000;
	}

	/*Checks if a File already exists*/
	bool isFile(char *filename){
		//open file
		ifstream ifile(filename);
		return ifile;
	}
};

template <class T>
T* Scheduler<T>::instance = NULL;
