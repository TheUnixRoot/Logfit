//
// Created by juanp on 4/12/18.
//

#ifndef BARNESORACLE_CONSOLEUTILS_H
#define BARNESORACLE_CONSOLEUTILS_H
#define CONSOLE_YELLOW "\033[0;33m"
#define CONSOLE_WHITE "\033[0m"
#define CONSOLE_RED ""

#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <iomanip>
#include "../DataStructures/ProvidedDataStructures.h"

using namespace std;
namespace ConsoleUtils {
    static constexpr unsigned int str2int(const char *str, int h = 0) {
        return !str[h] ? 5381 : (str2int(str, h + 1) + 33) ^ str[h];
    }

    static std::string validateArgs(Params p) {
        string res{""};
        res = res + ((p.ratioG < 0.0 or p.ratioG > 1.0) ? "Invalid number for GPU ratio. " : "")
              + ((p.numcpus < 0) or (p.numcpus > MAX_NUMBER_CPU_SUPPORTED) ? "Invalid number of CPU cores. " : "")
              + ((p.numgpus < 0 or p.numgpus > MAX_NUMBER_GPU_SUPPORTED) ? "Invalid number of GPUs. " : "");
        return res == "" ? "Parameters validation succeed!" : res;
    }

    static Params parseArgs(int argc, char *argv[]) {
        Params params;

//cout << argv[0] << endl;
        bool fflag{false}, cflag{false}, gflag{false}, rflag{false}, bflag{false}, kflag{false}, oflag{false};
        for (int i = 1; i < argc; ++i) {
//cout << argv[i-1] << endl;
            switch (ConsoleUtils::str2int(argv[i])) {
                case ConsoleUtils::str2int("-f"):
                case ConsoleUtils::str2int("--file"):
                    sprintf(params.inputData, "%s", argv[++i]);
                    fflag = true;
                    break;
                case ConsoleUtils::str2int("-c"):
                case ConsoleUtils::str2int("--cpu-number"):
                    params.numcpus = atoi(argv[++i]);
                    cflag = true;
                    break;
                case ConsoleUtils::str2int("-g"):
                case ConsoleUtils::str2int("--gpu-number"):
                    params.numgpus = atoi(argv[++i]);
                    gflag = true;
                    break;
                case ConsoleUtils::str2int("-r"):
                case ConsoleUtils::str2int("--rate-to-gpu"):
                    params.ratioG = atof(argv[++i]);
                    rflag = true;
                    break;
                case ConsoleUtils::str2int("-b"):
                case ConsoleUtils::str2int("--benchmark-name"):
                    sprintf(params.benchName, "%s", argv[++i]);
                    bflag = true;
                    break;
                case ConsoleUtils::str2int("-k"):
                case ConsoleUtils::str2int("--kernel-name"):
                    sprintf(params.kernelName, "%s", argv[++i]);
                    kflag = true;
                    break;
                case ConsoleUtils::str2int("-o"):
                    sprintf(params.openclFile, "%s", argv[++i]);
                    oflag = true;
                    break;
                case ConsoleUtils::str2int("-h"):
                case ConsoleUtils::str2int("--help"):
                    int sep{30};
                    cout << "Use: ./Oracle OPTIONS [ADVANCE_OPTIONS]" << endl << "OPTIONS:" << endl << left <<
                         setw(sep) << "-h | --help" << setw(sep) << "This help text." << endl <<
                         "-f | --file" << right << setw(sep - 1) << "input_file" << left << endl << setw(sep)
                         << "-c | --cpu-number" <<
                         setw(sep) << "numcpus" << endl << setw(sep) << "-g | --gpu-number" << setw(sep) <<
                         "numgpus" << endl << setw(sep) << "-r | --rate-to-gpu" << setw(sep) << "%inGPU" << endl <<
                         endl << "ADVANCED_OPTIONS:" << endl << setw(sep) << "-b | --benchmark-name" << setw(sep) <<
                         "bench_name" << endl << setw(sep) << "-k | --kernel-name" << setw(sep) << "kernel_name" <<
                         endl << setw(sep) << "-o | --opencl-filename" << setw(sep) << "opencl_file" << endl;
                    exit(0);
            }
        }
// Defaults
        if (!bflag) {
            sprintf(params.benchName, "BarnesHut");
        }
        if (!kflag) {
            sprintf(params.kernelName, "IterativeForce");
        }
        if (!oflag) {
            sprintf(params.openclFile, "kernel.cl");
        }
// Validation
        if (!fflag or !cflag or !gflag or !rflag) {
            string errmsg{""};
            errmsg = errmsg
                     + (fflag ? "" : "Invalid format for input file flag. ")
                     + (cflag ? "" : "Invalid format for cpu number flag. ")
                     + (gflag ? "" : "Invalid format for gpu number flag. ")
                     + (rflag ? "" : "Invalid format for gpu rate flag. ");
            cerr << "Parameters wrong formed. Error: " << errmsg << endl;
            exit(-1);
        }
        string message{validateArgs(params)};
        if (message.at(0) == 'I') {
            cerr << "Parameters with wrong values. Error: " << message << endl;
            exit(-1);
        }
        cout << CONSOLE_YELLOW << message << CONSOLE_WHITE << endl << "Console parameters read for BarnesHut Simulation: "
             << params.inputData << ", Number of CPU's cores: " << params.numcpus << ", Number of GPUs: "
             << params.numgpus << ", Percent in GPU: " << params.ratioG << endl;

        return params;
    }
    static void saveResultsForBench(Params p, double runtime) {
        int sep{30};
        std::cout << CONSOLE_YELLOW << "*************************" << CONSOLE_WHITE << std::endl;
        std::cout << CONSOLE_YELLOW << p.numcpus << setw(sep) << p.numgpus << setw(sep) << runtime << CONSOLE_WHITE << std::endl;
    }
}
#endif //BARNESORACLE_CONSOLEUTILS_H
