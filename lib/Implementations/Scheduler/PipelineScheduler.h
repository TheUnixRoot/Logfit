//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_PIPELINESCHEDULER_H
#define BARNESLOGFIT_PIPELINESCHEDULER_H

#include "tbb/pipeline.h"
#include "tbb/parallel_for.h"
#include "../../Interfaces/Scheduler/IScheduler.h"

#include "../../Helpers/Pipeline/SerialFilter.h"
#include "../../Helpers/Pipeline/ParallelFilter.h"


cl_int error;
cl_uint num_max_platforms;
cl_uint num_max_devices;
cl_uint num_platforms;
cl_uint num_devices;
cl_platform_id platforms_id;
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
int computeUnits;
size_t vectorization;

template<typename TSchedulerEngine, typename TExectionBody,
        typename ...TArgs>
class PipelineScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExectionBody &body;
    cl_command_queue command_queue;

public:
    PipelineScheduler(Params p, TExectionBody &body, TSchedulerEngine &engine) :
    IScheduler(p), body{body}, engine{engine} {
        initializeOPENCL(p.kernelName);
        initializeHOSTPRI();
        runtime = 0.0;
    }
    void StartParallelExecution() {
        int begin = 0;
        int end = body.GetVsize();
        engine.reStart();
        body.firsttime = true;

        if (parameters.numgpus < 1) {  // solo CPUs
            tbb::parallel_for(tbb::blocked_range<int>(begin, end ),
                [&](const tbb::blocked_range<int> &range){
                    startCpu = tbb::tick_count::now();
                    body.OperatorCPU(range.begin(), range.end());
                    stopCpu = tbb::tick_count::now();
                },
                tbb::auto_partitioner() );
        } else {  // ejecucion con GPU
            tbb::pipeline pipe;
            SerialFilter<PipelineScheduler> serial_filter(begin, end, this);
            ParallelFilter<PipelineScheduler> parallel_filter(begin, end, this);
            pipe.add_filter(serial_filter);
            pipe.add_filter(parallel_filter);

            pipe.run(parameters.numcpus + parameters.numgpus);
            pipe.clear();
        }
    }

    void* getEngine() {
        return &engine;
    }

    void* getBody() {
        return &body;
    }


    ~PipelineScheduler() {}

    friend class SerialFilter<PipelineScheduler>;
    friend class ParallelFilter<PipelineScheduler>;
private:

    void initializeOPENCL(char * kernelName){
        createCommandQueue();
        CreateAndCompileProgram(kernelName);
    }
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

        char * programSource = ReadSources(kname);

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
};


#endif //BARNESLOGFIT_PIPELINESCHEDULER_H
