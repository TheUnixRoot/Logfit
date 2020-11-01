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
#include <string.h>

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#ifdef ENERGYCOUNTERS
#ifdef Win32
#include "PCM_Win/windriver.h"
#else
#include "cpucounters.h"
#endif
#endif

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

#ifdef PJTRACER
#include "pfortrace.h"
#endif

using namespace std;
using namespace tbb;


/*****************************************************************************
 * types
 * **************************************************************************/
/* Definido en la implementacion particular del scheduler
typedef struct{
	int numcpus;
	int numgpus;
	int gpuChunk;
	char benchName[256];
	char kernelName[50];
	float ratioG;
}Params;
*/
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

//profiler
#ifdef PJTRACER
PFORTRACER * tracer;
#endif

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

void printPlatformInfo() {
	cl_uint num;
        cl_platform_id plataformas[10];
	error = clGetPlatformIDs(10, plataformas, &num);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No platforms were found.\n");
		exit(0);
	}

        cout << "Number of platforms: " << num << endl;

	cl_device_id dispositivos[10];

        for (int i=0; i< num; i++) {
            cout << "Platform " << i << ":" << endl;
            cl_uint num2;
            error = clGetDeviceIDs(plataformas[i], CL_DEVICE_TYPE_CPU, 10, dispositivos, &num2);
            if (error != CL_SUCCESS) {
                cout << "   No CPU devices found" << endl;
            } else {
                cout << "   Number of CPU devices found: " << num2 << endl;
                for (int j = 0; j < num2; j++) {
                    char device_name[256];
                    error = clGetDeviceInfo(dispositivos[j], CL_DEVICE_NAME, sizeof(char)*256, &device_name, NULL);
                    if (error != CL_SUCCESS) {
                        strcpy(device_name, " (NO NAME) ");
                    }
                    cl_uint unidades = 0;
                    error = clGetDeviceInfo(dispositivos[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &unidades, NULL);
                    cout << "       Device " << j <<" -> Name: " << device_name << ", compute units: " << unidades << endl;
                }
            }
            error = clGetDeviceIDs(plataformas[i], CL_DEVICE_TYPE_GPU, 10, dispositivos, &num2);
            if (error != CL_SUCCESS) {
                cout << "   No GPU devices found" << endl;
            } else {
                cout << "   Number of GPU devices found: "<< num2 << endl;
                for (int j = 0; j < num2; j++) {
                    char device_name[256];
                    error = clGetDeviceInfo(dispositivos[j], CL_DEVICE_NAME, sizeof(char)*256, &device_name, NULL);
                    if (error != CL_SUCCESS) {
                        strcpy(device_name, " (NO NAME) ");
                    }
                    cl_uint unidades = 0;
                    error = clGetDeviceInfo(dispositivos[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &unidades, NULL);
                    cout << "       Device " << j <<" -> Name: " << device_name << ", compute units: " << unidades << endl;
                }
            }
        }
}

void createCommandQueue() {
#ifdef DEBUG
        printPlatformInfo();
#endif
	num_max_platforms = 2;
	cl_platform_id platforms_id_array[2];
	error = clGetPlatformIDs(num_max_platforms, platforms_id_array, &num_platforms);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No platforms were found.\n");
		exit(0);
	}
	char pname[256];
	error = clGetPlatformInfo(platforms_id_array[1], CL_PLATFORM_NAME, 256, &pname, NULL);

	num_max_devices = 1;
	auto did = CL_DEVICE_TYPE_GPU;
	error = clGetDeviceIDs(platforms_id_array[1], CL_DEVICE_TYPE_GPU, num_max_devices, &device_id, &num_devices);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No devices were found\n");
		exit(0);
	}
	error = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &computeUnits, NULL);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No compute units found\n");
		exit(0);
	}

	char device_name[50];
	error = clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(char)*50, &device_name, NULL);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "No device name found\n");
		exit(0);
	}

	cerr << "GPU's name: " << device_name << " with "<< computeUnits << " computes Units" << endl;
	num_devices=1;
	context = clCreateContext(NULL, num_devices, &device_id, NULL, NULL,
			&error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Context couldn't be created\n");
		exit(0);
	}

	command_queue = clCreateCommandQueue/*WithProperties*/(context, device_id, CL_QUEUE_PROFILING_ENABLE, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr, "Command Queue couldn't be created\n");
		exit(0);
	}
}

void CreateAndCompileProgram(char * kname) {

	char kernelname[] = "/home/juanp/logfit/Pipeline/source/cl/kernel.cl";
	char * programSource = ReadSources(kernelname);

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
template <typename T, typename PARAMS>
class Scheduler{
protected:
// Class members
	//Scheduler Itself
	static T *instance;
	task_scheduler_init *init;
	int nCPUs;
	int nGPUs;
#ifdef ENERGYCOUNTERS
	//Energy Counters
	PCM * pcm;
	vector<CoreCounterState> cstates1, cstates2;
        vector<SocketCounterState> sktstate1, sktstate2;
        SystemCounterState sstate1, sstate2;
#endif
	//timing
	tick_count start, end;
	float runtime;
//End class members

	/*Scheduler Constructor, forbidden access to this constructor from outside*/
	Scheduler(PARAMS * p) {
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
#ifdef DEBUG
		cerr << "INITIALIZING pcm" << endl;
#endif
#ifdef ENERGYCOUNTERS
		initializePCM();
#endif
#ifdef DEBUG
		cerr << "INITIALIZING HOSTPRIORITY" << endl;
#endif
		initializeHOSTPRI();
		runtime = 0.0;
	}

	/*Initialize OpenCL environment*/
	void initializeOPENCL(char * kernelName){
		createCommandQueue();
		CreateAndCompileProgram(kernelName);
	}


	void initializeHOSTPRI(){
#ifdef HOST_PRIORITY  // rise GPU host-thread priority
		unsigned long dwError, dwThreadPri;
		//dwThreadPri = GetThreadPriority(GetCurrentThread());
		//printf("Current thread priority is 0x%x\n", dwThreadPri);
		if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)){
			dwError = GetLastError();
			if(dwError){
				cerr << "Failed to set hight priority to host thread (" << dwError << ")" << endl;
			}
		}
	//dwThreadPri = GetThreadPriority(GetCurrentThread());
	//printf("Current thread priority is 0x%x\n", dwThreadPri);
#endif
	}


public:
	/*Class destructor*/
	~Scheduler(){
		init->~task_scheduler_init();
		delete instance;
		instance = NULL;
	}

	/*This function creates only one instance per process, if you want a thread safe behavior protect the if clausule with a Lock*/
	static T * getInstance(PARAMS * params){
		if(! instance){
			instance = new T(params);
		}
		return instance;
	}

#ifdef ENERGYCOUNTERS
	/*Initializing PCM library*/
	void initializePCM(){
		/*This function prints lot of information*/
		pcm = PCM::getInstance();
		pcm->resetPMU();
		if (pcm->program() != PCM::Success){
			cerr << "Error in PCM library initialization" << endl;
			exit(-1);
		}
	}
#endif
	/*Sets the start mark of energy and time*/
	void startTimeAndEnergy(){
#ifdef ENERGYCOUNTERS
		pcm->getAllCounterStates(sstate1, sktstate1, cstates1);
#endif
		start = tick_count::now();
	}

	/*Sets the end mark of energy and time*/
	void endTimeAndEnergy(){
		end = tick_count::now();
#ifdef ENERGYCOUNTERS
		pcm->getAllCounterStates(sstate2, sktstate2, cstates2);
#endif
		runtime = (end-start).seconds()*1000;
	}

	/*Checks if a File already exists*/
	bool isFile(char *filename){
		//open file
		ifstream ifile(filename);
		return !ifile.fail();
	}
};

template <typename T, typename PARAMS>
T* Scheduler<T, PARAMS>::instance = NULL;
