// LogFit V3 (Logfit por intervalos y simplificacion de la fase final)
//
//============================================================================
// Name			: LogFit.h
// Author		: Antonio Vilches & Francisco Corbera
// Version		: 1.0
// Date			: 06 / 01 / 2014
// Copyright	: Department. Computer's Architecture (c)
// Description	: LogFit scheduler implementation
//============================================================================

//#define DEEP_GPU_REPORT       // vuelca th de la GPU por cada chunk
//#define DEEP_CPU_REPORT       // vuelca th de las CPUs por cada chunk
//#define TIMESTEP_THR_REPORT   // vuelca th del sistema en cada time step
//#define TIMESTEP_PW_REPORT    // vuelca el consumo del sistema en cada time step

//#define TRACER  // crear traza de uso de CPUs y GPU en formato google chrome
//#define DEBUGLOG  // Depurar fitting y stop condition

//#define LDEBUG    // mensajes de depuracion del LogFitEngine
//#define DRVDEBUG    // mensajes de depuracion del LogFitDriver

#define H2D //no tener en cuenta el primer envio para logfit quitando así el efecto del H2D si solo se hace en exploracion
#define SLOPE  //criterio de parada de fase inicial basado en la pendiente de la recta de las dos ultimas medidas
#define SECONDCHANCE // version en la que LogFit da una segunda oportunidad a un chunk si el nuevo no ha mejorado su TH

//#define OVERHEAD_STUDY // record OCL overheads
//#define CACHE_STUDY // record cache behavior
//#define HOM_PROF    // Hacer mediciones homogeneas para decidir cual es la mejor configuracion (solo CPUs, solo GPU, het)
#ifdef ENERGYCOUNTERS
//#define ENERGYPROF  // medir potencia ademas de tiempo en el profiling
#endif

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <list>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
#include "tbb/task.h"
#include "tbb/atomic.h"
#include "tbb/parallel_for.h"
#define TBB_PREVIEW_GLOBAL_CONTROL 1
#include "tbb/global_control.h"
#include "Scheduler.h"
#include <math.h>
#ifdef TRACER
#include "pfortrace.h"
#endif

using namespace std;
using namespace tbb;


/*****************************************************************************
 * Defines
 * **************************************************************************/
#define CPU 0
#define GPU 1
#define GPU_OFF -100
#define SEP "\t"
#define MAXPOINTS 1024

/*****************************************************************************
 * types
 * **************************************************************************/
typedef struct{
	int numcpus;
	int numgpus;
	int gpuChunk;
	char benchName[100];
	char kernelName[100];
	pid_t main_thread_id;
} Params;

/*****************************************************************************
 * Global variables
 * **************************************************************************/

#ifdef DEEP_GPU_REPORT
ofstream deep_gpu_report;
#endif
#ifdef DEEP_CPU_REPORT
ofstream deep_cpu_report;
#include <tbb/spin_mutex.h>
using myMutex_t = spin_mutex;
myMutex_t deepCPUMutex;
#endif
#ifdef TIMESTEP_THR_REPORT
ofstream timestep_thr_report;
#endif

#ifdef OVERHEAD_STUDY
// overhead accumulators
float overhead_sp = 0.0;
float overhead_h2d = 0.0;
float overhead_kl = 0.0;
float kernel_execution = 0.0;
float overhead_d2h = 0.0;
float overhead_td = 0.0;
cl_ulong tg1, tg2, tg3, tg4, tg5;
#endif

tbb::atomic<int> gpuStatus;

int timeStep;

char* benchName;

//Summarize GPU statistics
int itemsOnGPU = 0;
long totalIterationsGPU = 0;
long totalIterations = 0;
tick_count end_tc;

#ifdef TRACER
// Trace de work done by the threads (CPU, GPU)
PFORTRACER* tr = NULL;
#endif

/*****************************************************************************
 * Heterogeneous Scheduler
 * **************************************************************************/

#define MAXSEG 64           // numero maximo del array que almacena los segmentos
#define CHKTHRESHOLD 0.10   // tanto por ciento de las iteraciones restantes por debajo del cual no se reparte y se lo queda todo un dispositivo
#define MINPOINTS 4         // numero minimo de puntos para hacer logfit
#define GPUSLOPE 0.005      // pendiente del th para encontrar el codo
#define LogFitTHProfit 0.9  // tanto por ciento que se permite empeorar el th de la gpu antes de dar segunda oportunidad

class LogFitEngine {
    unsigned int limits[MAXSEG];    // array con los limites de los segmentos del logfit
    unsigned int actual;            // indice al limite actual para hacer una busqueda mas rapida
    unsigned int lastLimit;         // indice al ultimo limite inicializado en el codigo actual
    unsigned int npoints;           // indice del punto donde se alcanza el codo
    unsigned int initGChunk;        // chunk inicial para la GPU (y minimo tambien)
    unsigned int initCChunk;        // chunk inicial para la CPU (y minimo tambien)
    unsigned int chk[MAXSEG];       // array con los chunks de GPU utilizados para el ajuste
    float thr[MAXSEG];              // array con los throughputs registrados para los chunks de GPU usados
    enum {EXPLORATION, ELBOW, STABLE} stage; // etapa en la que se encuentra el algoritmo logfit
    bool lastChunk;                 // ultimo chunk para la GPU

#ifdef LDEBUG
    const char* stageStrings[3] = { "EXPLORATION", "ELBOW", "STABLE" };
    const char* logFitMode_strings[3] = { "NORMAL","SCAN","CONSOLIDATE" };
#endif

    enum {NORMAL, SCAN, CONSOLIDATE} logFitMode;
    float gpuThroughputCandidate;   // para no olvidar el th de la segunda oportunidad
    unsigned int chunkGPUCandidate; // ni el chunk

    bool GPUstopped;                // indica si la GPU esta parada o no
    float threshold;                // threshold donde se encuentra el codo

public:
#ifdef H2D
    bool h2d;
#endif
    float CPUTh;
    float GPUTh;
    unsigned int nCPUs;             // numero de cores
    unsigned int nGPUs;             // numero de GPUs
    unsigned int nextGPUChk;        // siguiente chunk de GPU a asignar (calculado previamente)
    unsigned int nextCPUChk;        // siguiente chunk de CPU a asignar (calculado previamente)

public:
    LogFitEngine(unsigned int _nCPUs, unsigned int _nGPUs, unsigned int _initGChunk, unsigned int _initCChunk) :
        initGChunk(_initGChunk), initCChunk(_initCChunk),
        nextGPUChk(_initGChunk), nextCPUChk(_initCChunk), nCPUs(_nCPUs), nGPUs(_nGPUs) {
            actual = 0;
            lastLimit = 0;
            npoints = 0;        // donde se ha encontrado el codo (puede haber más puntos por encima)
            stage = EXPLORATION;
            logFitMode = NORMAL;
            CPUTh = 0.0;
            GPUTh = 0.0;
            GPUstopped = (_nGPUs == 0);
            lastChunk = false;
            threshold = 0.0;
#ifdef H2D
            h2d = true;
#endif
#ifdef LDEBUG
            cerr << "LogFitEngine Configuration: " << endl;
            cerr << "  initGChunk " << initGChunk << " initCChunk " << initCChunk
                    << " nCPUs " << nCPUs << " nGPUs " << nGPUs << endl
                    << " stage " << stageStrings[stage] << " Mode " << logFitMode_strings[logFitMode]
                    << " GPUstopped " << GPUstopped << endl;
#endif
	}

    // inicializacion para un TS nuevo
    void reStart() {
        GPUstopped = (nGPUs == 0);
        lastChunk = false;
#ifdef H2D
        if (stage != STABLE)
            h2d = true;
#endif
    }

    // registra un nuevo throughput de un core
    void recordCPUTh(unsigned int chunk, float time) {
        CPUTh = (float)chunk / time;
    }

    bool stablePhase() {
        return stage == STABLE;
    }

#ifdef LDEBUG
#define DEBUGSEC
#endif
    // registra un nuevo throughput de la GPU marcando la evolucion del algoritmo LogFit
    // cambia de estado del algoritmo y precalcula el chunk de gpu siguiente (adapta tamaño)
    // tan solo no tiene en cuenta final phase, que se tiene en cuenta al pedir chunk y ver
    // cuanto queda.
    // Devuelve un 1 cuando se ha cambiado a la fase estable
    int recordGPUTh(unsigned int chunk, float time) {
        int retVal = 0;
        static int elbow;
        static bool notEnough; // indica si en la ultima vez no hubo iteraciones suficientes para la GPU
#ifdef H2D
            if (h2d) {
                h2d = false;
                //GPUTh = (float) chunk / (float) time;
#ifdef LDEBUG
        cerr << "H2D - No se registra GPUTh ("<< stageStrings[stage] <<"): chunk " << chunk << " TH " << (float) chunk / (float) time << endl;
#endif
                return retVal;
            }
#endif
        if (stage == EXPLORATION) { // estamos buscando aun el codo
            if (chunk == nextGPUChk) {// solo lo registramos si se ha enviado todo el chunk
                notEnough = false;  // ha habido iteraciones suficientes
                limits[actual] = chunk;
                chk[actual] = chunk;
                thr[actual] = GPUTh = (float) chunk / (float) time;
                actual++;
                limits[actual] = limits[actual-1]*2;
                lastLimit = actual;
                // comprobar si se ha alcanzado el codo
                if (!elbowCondition()) { // no se ha alcanzado el codo en el th de la GPU
                    nextGPUChk *= 2;    // volvemos a multiplicar por 2 el chunk
                } else { // se ha alcanzado el codo en el th de la GPU por primera vez, registramos th y pasamos a fase ELBOW
                    stage = ELBOW;
                    elbow = actual - 3;
                    nextGPUChk = chk[elbow];
                }
            } else if (notEnough) {  // es la segunda vez que no hay it suficientes, parar busqueda
                // registramos la nueva medida y damos por finalizada fase de busqueda del codo
                // se registra solo si el chunk es mayor que el ultimo registrado (no volvemos atras)
                chk[actual] = chunk;
                thr[actual] = GPUTh = (float) chunk / (float) time;
                npoints = actual;
                lastLimit = actual + 1;
                limits[lastLimit] = limits[lastLimit-1]*2;
                stage = STABLE;         // pasamos a fase estable
                retVal = 1;
                nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
            } else {    // es la primera vez que no hay it suficientes para la GPU
                notEnough = true;
            }
        } else if (stage == ELBOW) { // se ha detectado el codo, volver a comprobar los tres puntos, si vuelven a cumplir la condicion se ha encontrado codo, si no se continua
            if (chunk == nextGPUChk) {// solo lo registramos si se ha enviado todo el chunk
                notEnough = false;  // ha habido iteraciones suficientes
                chk[elbow] = chunk;
                thr[elbow] = GPUTh = (float) chunk / (float) time;
                elbow++;
                nextGPUChk = chk[elbow];
                if (elbow == actual) { // ya se han vuelto a medir los tres que antes detectaron codo
                    if (!elbowCondition()) { // no se ha alcanzado el codo en el th de la GPU
                        stage = EXPLORATION; // volvemos a la fase normal;
                        nextGPUChk = limits[actual];    // volvemos a multiplicar por 2 el chunk
                    } else { // se ha alcanzado el codo en el th de la GPU por segunda vez, este es el codo
                        npoints = actual - 3; // registramos el punto donde se alcanza el codo
                        actual = npoints;
                        lastLimit = actual + 1;
                        stage = STABLE;         // pasamos a fase estable
                        retVal = 1;
                        nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
                    }
                }
            } else if (notEnough) {  // es la segunda vez que no hay it suficientes, parar busqueda
                // registramos la nueva medida y damos por finalizada fase de busqueda del codo
                // se registra solo si el chunk es mayor que el ultimo registrado (no volvemos atras)
                chk[elbow] = chunk;
                thr[elbow] = GPUTh = (float) chunk / (float) time;
                npoints = actual;
                lastLimit = actual + 1;
                limits[lastLimit] = limits[lastLimit-1]*2;
                stage = STABLE;         // pasamos a fase estable
                retVal = 1;
                nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
            } else {    // es la primera vez que no hay it suficientes para la GPU
                notEnough = true;
            }
        } else if (stage == STABLE) {  // ya se encontro el codo, usamos el modelo
            float newGPUth = (float) chunk / (float) time;
            if (logFitMode == SCAN) { // antes disminuyo el TH de la GPU
#ifdef DEBUGSEC
        cerr << " Resultado segunda oportunidad: newTH " << newGPUth << " oldTH " << gpuThroughputCandidate << endl;
#endif
                if (newGPUth < gpuThroughputCandidate) { // si el nuevo TH es peor, consolidamos el anterior con el chunk anterior
                    newGPUth = gpuThroughputCandidate;
                    chunk = chunkGPUCandidate;
                } // si el nuevo TH es mejor, ha servido la segunda oportunidad, usamos el th y chunk que han mejorado
                logFitMode = NORMAL;
            }
            if (newGPUth >= GPUTh * LogFitTHProfit) {
                // si el th actual no empeora mucho, el nuevo chunk era adecuado
                // se registra y se calcula el nuevo chunk normalmente
                if (chunk <= limits[actual]) {
                    // el chunk esta por debajo del limite superior del intervalo actual
                    if (actual == 0 || chunk > limits[actual-1]) {
                        // el chunk pertence al intervalo actual, almacenarlo
                        chk[actual] = chunk;
                        thr[actual] = GPUTh = newGPUth;
                        nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
                    } else { // el chunk no pertenece al intervalo actual, es menor, buscar intervalo
                        int i = actual-1;
                        while (i > 0 && chunk <= limits[i])
                            i--;
                        actual = i;
                        chk[actual] = chunk;
                        thr[actual] = GPUTh = newGPUth;
                        nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
                    }
                } else { // el chunk esta por encima del limite superior del intervalo actual
                         // hay que buscar su intervalo, creando nuevos intervalos si fuera necesario
                    int i = actual+1;
                    while (i <= lastLimit && chunk > limits[i])
                        i++;
                    if (i <= lastLimit) {
                        actual = i;
                        chk[actual] = chunk;
                        thr[actual] = GPUTh = newGPUth;
                        if (actual == lastLimit) {
                            lastLimit++;
                            limits[lastLimit] = limits[lastLimit-1]*2;
                        }
                        nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
                    } else {
                        do {
                            limits[i] = limits[i-1]*2;
                            chk[i] = limits[i];
                            thr[i] = newGPUth;
                        } while (chunk > limits[i++]);
                        actual = i-1;
                        chk[actual] = chunk;
                        thr[actual] = GPUTh = newGPUth;
                        lastLimit = i;
                        limits[lastLimit] = limits[lastLimit-1]*2;
                        nextGPUChk = calculateLogarithmicModel();  // proximo chunk generado por logfit
                    }
                }
            } else {
                // el th actual ha empeorado con respecto al anterior, dar segunda oportunidad al chunk
                // cuyo th no ha sido mejorado
#ifdef DEBUGSEC
        cerr << " TH ha empeorado de " << GPUTh << " a " << newGPUth << ", segunda oportunidad para chunk " << chk[actual] << endl;
#endif
                logFitMode = SCAN;
                gpuThroughputCandidate = newGPUth;
                GPUTh = newGPUth;           // TODO: hay que actualizar el th de la GPU?
                chunkGPUCandidate = chunk;
                nextGPUChk = chk[actual];   // segunda oportunidad del chunk que tenia mejor th
            }
        }
#ifdef LDEBUG
        cerr << "--> Registrado GPUTh ("<< stageStrings[stage] <<"): chunk " << chunk << " TH " << GPUTh
                << " (CPU TH: " << CPUTh << ")" << endl;
#endif
        return retVal;
    }

    // devuelve el chunk de CPU a usar
    unsigned int getCPUChunk(unsigned int begin, unsigned int end) {
        unsigned int rem = end - begin;
        if (GPUTh == 0.0 || CPUTh == 0.0) { // si aun no hay th de GPU o CPU, devolvemos el chunk de CPU inicial

        } else { // ya hay th de ambos dispositivos y se puede calcular la velocidad relativa
            if (GPUstopped) { // si la GPU esta parada, repartimos entre los cores (guided)
                nextCPUChk = max(initCChunk, rem/(nCPUs+1));
            } else { // la GPU no esta parada
                float relSpeed = GPUTh / CPUTh;
                // el resto de iteraciones se reparte entre todos los disp. teniendo en cuenta su velocidad
                nextCPUChk = max((float)initCChunk, min((float)nextGPUChk/relSpeed, rem/(relSpeed+nCPUs)));
            }
        }
        return min(nextCPUChk, rem);
    }

    // devuelve el chunk de GPU a usar. Si el chunk es 0, no usar la GPU
    unsigned int getGPUChunk(unsigned int begin, unsigned int end) {
        if (lastChunk) return 0;
#ifdef H2D
        if (h2d)
            return initGChunk;
#endif
        unsigned int rem = end - begin;
        if (GPUTh == 0.0 || CPUTh == 0.0 || nCPUs == 0 || stage != STABLE) { // si aun no hay th de GPU o CPU, devolvemos el chunk de CPU inicial
                // se develve el nextGPUChk calculado previamente en el logfit
            return min(nextGPUChk, rem);
        } else { // ya hay th de ambos dispositivos y se puede calcular la velocidad relativa (fase final)
            float relSpeed = GPUTh / CPUTh;
#ifdef LDEBUG
                cerr << "rem " << rem << " GPUTH " << GPUTh << " CPUTH " << CPUTh << " relSpeed " << relSpeed
                        << " nextGPUChk " << nextGPUChk << " nextGPUChk / relSpeed " << nextGPUChk / relSpeed
                        << " (rem - nextGPUChk) / nCPUs " << (rem - nextGPUChk) / nCPUs << endl;
#endif
            if (rem < nextGPUChk || nextGPUChk / relSpeed > (rem - nextGPUChk) / nCPUs) {// No hay suficiente para repartir
                //lastChunk = true;  // ya es el ultimo reparto **** Cambiado por si erramos reparto
                // Hay suficiente para repartir y que la GPU no reciba menos que el chunk del codo?
                // tiempo en procesarlo todo en los cores
                float timeOnCPU = rem / (nCPUs * CPUTh);
                // calculamos tiempo en GPU solo
                float timeOnGPU;
                // calculamos el intervalo donde cae rem en los TH de la GPU
                int i = npoints;
                while (i >= 0 && chk[i] > rem) i--;
                if (i < 0) {
                    timeOnGPU = rem / thr[0];
                } else { // lo que queda esta por encima de chk[i]
                    if (i == npoints) { // cae por encima del codo
                        timeOnGPU = rem / thr[i];
                    } else {    // cae en un intervalo por debajo del codo
                        float slope = (thr[i+1] - thr[i])/(chk[i+1] - chk[i]);
                        float y = (rem - chk[i]) * slope + thr[i];
                        timeOnGPU = rem / y;
                    }
                }
                // estimamos tiempo para un reparto entre GPU y cores
                int x = rem;     // trozo a darle a la GPU en caso heterogeneo (comenzamos con el total)
                float timeHet = timeOnGPU;  // para el total el tiempo seria el de solo GPU
                // comprobamos repartiendo según todos los puntos registrados de TH de la GPU
                for (int i = npoints; i >= 0; i--) {
                    if (rem > chk[i]) {
                        float tg = chk[i] / thr[i];
                        float tc = (rem - chk[i]) / (nCPUs * CPUTh);
                        float tmax = (tg > tc) ? tg : tc;
                        if (tmax < timeHet) {
                            timeHet = tmax;
                            x = chk[i];
                        }
                    }
                }
                if (timeHet < timeOnCPU && timeHet < timeOnGPU) {
#ifdef LDEBUG
                    cerr << " --> " << x << " a GPU y " << (rem - x) << " a CPU " << endl;
#endif
                    return x;
                }
                else if (timeOnGPU < timeOnCPU) {
#ifdef LDEBUG
                    cerr << " --> Todo a GPU " << endl;
#endif
                    return rem;
                }
                else {
#ifdef LDEBUG
                    cerr << " --> Todo a CPU " << endl;
#endif
                    GPUstopped = true;
                    return 0;
                }
            }
        }
        return min(nextGPUChk, rem);
    }

private:

    // devuelve true si se cumple que hemos alcanzado el codo de la curva de th de la GPU
    bool elbowCondition() {
       if ((actual >= MINPOINTS + 2)
            && ((thr[actual-1]-thr[actual-3])/(chk[actual-1]-chk[actual-3]) <= GPUSLOPE)
            && ((thr[actual-2]-thr[actual-3])/(chk[actual-2]-chk[actual-3]) <= GPUSLOPE)) {
           // los dos ultimos puntos no superan al anterior con una pendiente suficiente
           // se ha encontrado en codo en el punto chk[actual-3]
           return true;
       }
       return false;
    }

    // devuelve true si se cumple parcialmente la condicion del codo en el punto dado
    bool elbowCondition(int pos) {
       if ((pos > 0)
            && ((thr[pos]-thr[pos-1])/(chk[pos]-chk[pos-1]) <= GPUSLOPE)) {
           // los dos ultimos puntos no superan al anterior con una pendiente suficiente
           // se ha encontrado en codo en el punto chk[actual-3]
           return true;
       }
       return false;
    }

#ifdef LDEBUG
//#define DEBUGLOG
#endif
    // calcula la aproximacion logaritmica a los puntos x,y medidos de chunk,thr de la
    // GPU, y devuevle el chunk siguiente de GPU adecuado para la ultima medida de througput
    // hace el fitting con todos los puntos y no se almacena nada ya que pueden cambiar los puntos
    unsigned int calculateLogarithmicModel() {
	float numerador, denominador;
	float sumatorio1 = 0.0, sumatorio2 = 0.0, sumatorio3 = 0.0;
	float sumatorio4 = 0.0, sumatorio5 = 0.0;

        for (int j = 0; j < lastLimit; j++) {
#ifdef DEBUGLOG
            cerr << " chk(" << chk[j] << ") = " << thr[j];
#endif
            float logaux = logf(chk[j]);
            sumatorio1 += thr[j] * logaux;
            sumatorio2 += logaux;
            sumatorio3 += thr[j];
            sumatorio4 += logaux*logaux;
        }
#ifdef DEBUGLOG
        cerr << endl;
#endif

	int np = lastLimit;
	sumatorio5 = sumatorio2*sumatorio2;
	sumatorio4 *= np;
	numerador = (np * sumatorio1) - (sumatorio3 * sumatorio2);
	denominador = sumatorio4 - sumatorio5;

	//Getting the value for a
	float a = numerador / denominador;
	//The threshold is set during first call
	if (threshold == 0.0) {
            threshold = a / chk[lastLimit-1];
	}
	//Just to get a multiple of computeUnit
	unsigned int calculatedChunk = ceil((a / threshold) / (float) initGChunk)*(float) initGChunk;
#ifdef DEBUGLOG
	cerr << "threshold " << threshold << " a " << a << " chunk " << calculatedChunk << endl;
#endif
	return (calculatedChunk < initGChunk) ? initGChunk : calculatedChunk;
    }

};

/*Bundle class: This class is used to store the information that items need while walking throught pipeline's stages.*/
class Bundle {
public:
	int begin;
	int end;
	int type; //GPU = 0, CPU=1

	Bundle() {
	};
};

template <class B> class MySerialFilter;
template <class B> class MyParallelFilter;
/**************************************************************************************/
/* Clase para ejecutar los TS: hacer logfit, medidas homogeneas, selecion dispositivo */
/**************************************************************************************/
#define NCKMINPROF 5    // numero minimo de chunks para medir TH
#define MSMINPROF 50    // numero minimo de ms para hacer profiling (anula NCKMINPROF si se define). Si no llega, entonces 1 TS

class LogFitDriver {
private:
    LogFitEngine * lfe;
    float GPUThHom = 0.0;     // throughput GPU en homogeneo
    float CPUThHom = 0.0;     // throughput CPU en homogeneo
    float HetTh = 0.0;        // throughput heterogeneo
#ifdef ENERGYPROF
public:
    PCM * pcm;
private:
    vector<CoreCounterState> cstates1ep, cstates2ep;
    vector<SocketCounterState> sktstate1ep, sktstate2ep;
    SystemCounterState sstate1ep, sstate2ep;
    double PwOnlyGPU;    // Power GPU only execution
    double PwOnlyCPU;    // Power CPU only execution
    double PwHet;        // Power Heterogeneous execution
#endif
    unsigned int nCPUs;
    unsigned int nGPUs;

public:
    enum {UNKP, HETP, CPUP, GPUP} policy;  // politica elegida: UNKP aun no se ha decidido
    enum {UNONE, UPROF} unkPhase;  // fase en la que se encuentra el proceso de decision de la politica

    int stopProf = 0;   // Indica cuando parar LogFit para hacer profiling a la fase serie del pipeline
    int recordTh = 1;   // Indica si actualizar Th de los dispositivos en LogFit (no hay que hacerlo cuando se hace profiling)

private:
    template <class B>
    void profGPU(int begin, int end, B* body) {  // medir TH de la ejecuicion homogenea solo GPU
#ifdef TRACER
        char version[100];
        sprintf(version, "ProfGPU");
        tr->newEvent(version);
#endif
        auto mp = global_control::max_allowed_parallelism;
        global_control gc(mp, 1);
#ifdef DRVDEBUG
        cerr << "LFDRV: profGPU-> Threads: " << global_control::active_value(mp) << endl;
#endif

        stopProf = 0;
        recordTh = 0;
        pipeline pipe;
        MySerialFilter<B> serial_filter(begin, end, 0, nGPUs, lfe, this);
        MyParallelFilter<B> parallel_filter(body, end - begin, lfe, this);
        pipe.add_filter(serial_filter);
        pipe.add_filter(parallel_filter);

#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate1ep, sktstate1ep, cstates1ep);
#endif
        tick_count tstart = tick_count::now();
        pipe.run(nGPUs);
        tick_count tend = tick_count::now();
        float time = (tend - tstart).seconds()*1000;
        GPUThHom = (end-begin) / time;

#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate2ep, sktstate2ep, cstates2ep);
        PwOnlyGPU = getConsumedJoules(sstate1ep, sstate2ep)*1000 / time;
#endif
        pipe.clear();

#ifdef DRVDEBUG
        cerr << "LFDRV: profGPU " << begin << ", " << end << " (" << end-begin << ") TH: " << GPUThHom
                << " (" << time << " ms)" << endl;
#ifdef ENERGYPROF
        cerr << "LFDRV: profGPU Power " << PwOnlyGPU << " (" << (time/1000)*PwOnlyGPU/(end-begin) << " Jul por item) " << endl;
#endif
#endif
#ifdef TRACER
        tr->newEvent(version);
#endif
    }

    template <class B>
    void profCPUs(int begin, int end, B* body) {  // medir TH de la ejecuicion homogenea solo CPUs
#ifdef TRACER
        char version[100];
        sprintf(version, "ProfCPUs");
        tr->newEvent(version);
#endif
        auto mp = global_control::max_allowed_parallelism;
        global_control gc(mp, nCPUs);
#ifdef DRVDEBUG
        cerr << "LFDRV: profCPUs-> Threads: " << global_control::active_value(mp) << endl;
#endif
#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate1ep, sktstate1ep, cstates1ep);
#endif

        tick_count tstart = tick_count::now();
        ParallelFORCPUsLF(begin, end, body);
        tick_count tend = tick_count::now();

        float time = (tend - tstart).seconds()*1000;
        CPUThHom = (end-begin) / (time*nCPUs);
#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate2ep, sktstate2ep, cstates2ep);
        PwOnlyCPU = getConsumedJoules(sstate1ep, sstate2ep)*1000 / time;
#endif
#ifdef DRVDEBUG
        cerr << "LFDRV: profCPUs " << begin << ", " << end << " (" << end-begin << ") TH: " << CPUThHom*nCPUs
                    << " (" << time << " ms)" << endl;
#ifdef ENERGYPROF
        cerr << "LFDRV: profCPU Power " << PwOnlyCPU << " (" << (time/1000)*PwOnlyCPU/(end-begin) << " Jul por item) " << endl;
#endif
#endif
#ifdef TRACER
        tr->newEvent(version);
#endif
    }

    template <class B>
    void profHet(int begin, int end, B* body) {  // medir TH de la ejecuicion heterogenea
#ifdef TRACER
        char version[100];
        sprintf(version, "ProfHet");
        tr->newEvent(version);
#endif
        auto mp = global_control::max_allowed_parallelism;
        global_control gc(mp, nCPUs + nGPUs);
#ifdef DRVDEBUG
        cerr << "LFDRV: profHet-> Threads: " << global_control::active_value(mp) << endl;
        //cerr << "LFDRV: profHet" << endl;
#endif

        stopProf = 0;
        recordTh = 0;
        pipeline pipe;
        MySerialFilter<B> serial_filter(begin, end, nCPUs, nGPUs, lfe, this);
        MyParallelFilter<B> parallel_filter(body, end - begin, lfe, this);
        pipe.add_filter(serial_filter);
        pipe.add_filter(parallel_filter);

#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate1ep, sktstate1ep, cstates1ep);
#endif
        tick_count tstart = tick_count::now();
        pipe.run(nCPUs + nGPUs);
        tick_count tend = tick_count::now();
        float time = (tend - tstart).seconds()*1000;
        HetTh = (end-begin) / time;

#ifdef ENERGYPROF
        pcm->getAllCounterStates(sstate2ep, sktstate2ep, cstates2ep);
        PwHet = getConsumedJoules(sstate1ep, sstate2ep)*1000 / time;
#endif
        pipe.clear();

#ifdef DRVDEBUG
        cerr << "LFDRV: profHet " << begin << ", " << end << " (" << end-begin << ") TH: " << HetTh
                << " (" << time << " ms)" << endl;
#ifdef ENERGYPROF
        cerr << "LFDRV: profHet Power " << PwHet << " (" << (time/1000)*PwHet/(end-begin) << " Jul por item) " << endl;
#endif
#endif
#ifdef TRACER
        tr->newEvent(version);
#endif
    }


#define INTERFERENCETHRESHOLD 0.75  // threshold para considerar que hay interferencia (si el throughput cae por debajo de eso %)
#define HETPENALTY 0.95     // penalti que se paga por la version heterogena (si la het es muy parecida a una homogenea se prima esta ultima
                            // por los posibles errores en la medidas heterogeneas)
    void setPolicy() {  // mira los throughputs y establece la politica
        // si hay mucha variacion en los throughputs de algun dispositivo
        // es que hay mucha interferencia y entonces le damos mas importancia
        // a los TH de los homogeneos

        float penalty = 1.0;
        //float HetTh = lfe->GPUTh + nCPUs*(lfe->CPUTh);
#ifdef DRVDEBUG
        cerr << "LFDRV setpolicy: HetTh " << HetTh << " GPUThHom " << GPUThHom << " CPUThHom " << CPUThHom*nCPUs << endl;
#endif
        float interference = HetTh / (GPUThHom + CPUThHom*nCPUs);
        if (interference < INTERFERENCETHRESHOLD)
            penalty = HETPENALTY;
#ifdef DRVDEBUG
        cerr << "LFDRV setpolicy: Interference " << interference << " HetTh " << penalty*HetTh << endl;
#endif
        if (penalty * HetTh > GPUThHom && penalty * HetTh > CPUThHom*nCPUs) {
#ifdef DRVDEBUG
            cerr << "LFDRV: setPolicy -> HETP" << endl;
#endif
            policy = HETP;
        } else if (GPUThHom > CPUThHom*nCPUs) {
#ifdef DRVDEBUG
            cerr << "LFDRV: setPolicy -> GPUP" << endl;
#endif
            policy = GPUP;
        } else {
#ifdef DRVDEBUG
            cerr << "LFDRV: setPolicy -> CPUP" << endl;
#endif
            policy = CPUP;
        }
    }

    bool enoughIterations(int begin, int end) {
        int chunkProfGPU = getChunkProfGPU();
        int chunkProfCPUs = getChunkProfCPU();
        int rem = end - begin;
        return (rem >= (chunkProfGPU + chunkProfCPUs));
    }

    bool enoughIterationsGPU(int begin, int end) {
        int chunkProfGPU = getChunkProfGPU();
        int rem = end - begin;
        return (rem >= chunkProfGPU);
    }

    bool enoughIterationsCPU(int begin, int end) {
        int chunkProfCPUs = getChunkProfCPU();
        int rem = end - begin;
        return (rem >= chunkProfCPUs);
    }

    int getChunkProfGPU() {
#ifndef MSMINPROF
        return NCKMINPROF * lfe->nextGPUChk;
#else
        // calculamos el tamaño de chunk necesario para mantener a la GPU durante
        // MSMINPROF ms activa. Calculamos el multiplo de (lfe->nextGPUChk) mas cercano
        // por encima, y si es mayor que NCKMINPROF * lfe->nextGPUChk, devolvemos el valor
        // calculado (nos aseguramos que al menos se miden NCKMINPROF)
        int profChunk = MSMINPROF * lfe->GPUTh;  // trozo para estar MSMINPROF ms haciendo profilig
        profChunk = ceil((float)(profChunk)/(float)lfe->nextGPUChk) * lfe->nextGPUChk;
        int minProfChunk = NCKMINPROF * lfe->nextGPUChk;
        return profChunk > minProfChunk ? profChunk : minProfChunk;
#endif
    }

    int getChunkProfCPU() {
#ifdef DRVDEBUG
        cerr << "LFDRV: getChunkProfCPU GPUTh: " << lfe->GPUTh << " CPUTh: " << lfe->CPUTh << endl;
#endif
        return getChunkProfGPU() * (nCPUs * lfe->CPUTh/lfe->GPUTh);
    }

public:
    LogFitDriver (LogFitEngine * _lfe) {
        lfe = _lfe;
        nGPUs = lfe->nGPUs;
        nCPUs = lfe->nCPUs;
        if (nGPUs < 1)
            policy = CPUP;
        else if (nCPUs < 1)
            policy = GPUP;
        else
            policy = UNKP;
        unkPhase = UNONE;
    }

    void stableReached() {  // se ha alcanzado la fase estable en logfit, se puede empezar a medir
        stopProf = 1;
    }

    template <class B>
    void runHet(int begin, int end, B *body) {
        if (nGPUs < 1) {  // solo CPUs
                ParallelFORCPUsLF(begin, end, body);
        } else {  // ejecucion con GPU
                pipeline pipe;
                MySerialFilter<B> serial_filter(begin, end, nCPUs, nGPUs, lfe);
                MyParallelFilter<B> parallel_filter(body, end - begin, lfe);
                pipe.add_filter(serial_filter);
                pipe.add_filter(parallel_filter);
#ifdef OVERHEAD_STUDY
                end_tc = tick_count::now();
#endif
                pipe.run(nCPUs + nGPUs);
                pipe.clear();
        }
    }

    template <class B>
    void run(int begin, int end, B *body) {
        int newbeg = begin;
        switch (policy) {
            case UNKP:  // aun no se ha medido y no se sabe la politica
                switch (unkPhase) {
                    case UNONE: { // no se ha empezado el proceso de medicion para tomar decision
#ifdef DRVDEBUG
                        cerr << "LFDRV: UNKP->UNONE " << begin << " -> " << end << endl;
#endif
                        pipeline pipe;
                        MySerialFilter<B> serial_filter(begin, end, nCPUs, nGPUs, lfe, this);
                        MyParallelFilter<B> parallel_filter(body, end - begin, lfe, this);
                        pipe.add_filter(serial_filter);
                        pipe.add_filter(parallel_filter);
#ifdef OVERHEAD_STUDY
                        end_tc = tick_count::now();
#endif
                        pipe.run(nCPUs + nGPUs);
                        newbeg = serial_filter.begin;
                        pipe.clear();
                        if (!(lfe->stablePhase())) break; // si no hemos alcanzado el codo en este TS seguimos en el siguiente
                        unkPhase = UPROF;
                    }
                    case UPROF: { // fase de profiling
                        // el pipe con un driver (this) termina cuando se llega a la fase stable
                        // ahora es el momento de hacer medidas para ver que politica
                        // de ejecucion es mejor (het, cpu, gpu)
                        int chunk, newend;
                        if (HetTh == 0.0 && (enoughIterations(newbeg, end) || (newbeg == begin))) { // quedan iteraciones suficientes para el prof het y no se ha hecho
                            chunk = getChunkProfCPU() + getChunkProfGPU();
                            newend = (newbeg + chunk > end) ? end : newbeg + chunk;
#ifdef DRVDEBUG
                            cerr << "LFDRV: UNKP->UPROF: HetTh " << newbeg << " -> " << newend << endl;
#endif
                            profHet(newbeg, newend, body);
                            newbeg = newend;
                        }
                        if (GPUThHom == 0.0 && (enoughIterationsGPU(newbeg, end) || (newbeg == begin))) { // quedan suficientes iteraciones en este TS para medir la GPU
                            chunk = getChunkProfGPU();
                            newend = (newbeg + chunk > end) ? end : newbeg + chunk;
#ifdef DRVDEBUG
                            cerr << "LFDRV: UNKP->UPROF: GPUThHom " << newbeg << " -> " << newend << endl;
#endif
                            profGPU(newbeg, newend, body);
                            newbeg = newend;
                        }
                        if (CPUThHom == 0.0 && (enoughIterationsCPU(newbeg, end) || (newbeg == begin))) {  // quedan suficientes iteraciones en este TS para medir las CPUs?
                            chunk = getChunkProfCPU();
                            newend = (newbeg + chunk > end) ? end : newbeg + chunk;
#ifdef DRVDEBUG
                            cerr << "LFDRV: UNKP->UPROF: CPUThHom " << newbeg << " -> " << newend << endl;
#endif
                            profCPUs(newbeg, newend, body);
                            newbeg = newend;
                        }
                        if (HetTh != 0.0 && GPUThHom != 0.0 && CPUThHom != 0.0) {  // se ha terminado profiling, terminamos TS con nueva politica
                            setPolicy();
                            this->run(newbeg, end, body);
                        } else if (newbeg < end) { // no hay suficientes para medir algun dispositivo, ejecutar en het hasta final y medir en sig TS
                            pipeline pipe;
                            MySerialFilter<B> serial_filter(newbeg, end, nCPUs, nGPUs, lfe);
                            MyParallelFilter<B> parallel_filter(body, end - newbeg, lfe);
                            pipe.add_filter(serial_filter);
                            pipe.add_filter(parallel_filter);
                            #ifdef OVERHEAD_STUDY
                            end_tc = tick_count::now();
                            #endif
                            pipe.run(nCPUs + nGPUs);
                            pipe.clear();
                        }
                        break;
                    }
                }
                break;
            case HETP: { // ejecutamos con el nCPUs y nGPUs indicado por el usuario
#ifdef DRVDEBUG
                cerr << "LFDRV: HETP " << begin << " -> " << end << endl;
#endif
                stopProf = 0;
                recordTh = 1;
                {
                    auto mp = global_control::max_allowed_parallelism;
                    global_control gc(mp, nCPUs+nGPUs);
                    pipeline pipe;
                    MySerialFilter<B> serial_filter(begin, end, nCPUs, nGPUs, lfe);
                    MyParallelFilter<B> parallel_filter(body, end - begin, lfe);
                    pipe.add_filter(serial_filter);
                    pipe.add_filter(parallel_filter);
#ifdef OVERHEAD_STUDY
                    end_tc = tick_count::now();
#endif
                    pipe.run(nCPUs + nGPUs);
                    pipe.clear();
                }
                break; }
            case CPUP: {
#ifdef DRVDEBUG
                cerr << "LFDRV: CPUP " << begin << " -> " << end << endl;
#endif
                {
                    auto mp = global_control::max_allowed_parallelism;
                    global_control gc(mp, nCPUs);

                    ParallelFORCPUsLF(begin, end, body);
                }
                break; }
            case GPUP: {
#ifdef DRVDEBUG
                cerr << "LFDRV: GPUP " << begin << " -> " << end << endl;
#endif
                stopProf = 0;
                recordTh = 1;
                {
                    auto mp = global_control::max_allowed_parallelism;
                    global_control gc(mp, 1);

                    pipeline pipe;
                    MySerialFilter<B> serial_filter(begin, end, 0, nGPUs, lfe);
                    MyParallelFilter<B> parallel_filter(body, end - begin, lfe);
                    pipe.add_filter(serial_filter);
                    pipe.add_filter(parallel_filter);
#ifdef OVERHEAD_STUDY
                    end_tc = tick_count::now();
#endif
                    pipe.run(nGPUs);
                    pipe.clear();
                }
                break; }
        }
    }
};


/*My serial filter class represents the partitioner of the engine. This class selects a device and a rubrange of iterations*/
template <class B>
class MySerialFilter : public filter {
public:
	int begin, _begin;
	int end;
	int nCPUs;
	int nGPUs;
        LogFitEngine *lf;
        LogFitDriver *lfd;
public:

	MySerialFilter(int b, int e, int nC, int nG, LogFitEngine *_lf) :
		filter(true) {
		begin = _begin = b;
		end = e;
		nCPUs = nC;
		nGPUs = nG;
                lf = _lf;
                lfd = NULL;
                lf->nCPUs = nC;
                lf->nGPUs = nG;
	}

	MySerialFilter(int b, int e, int nC, int nG, LogFitEngine *_lfe, LogFitDriver *_lfd) :
		filter(true) {
		begin = _begin = b;
		end = e;
		nCPUs = nC;
		nGPUs = nG;
                lf = _lfe;
                lfd = _lfd;
                lf->nCPUs = nC;
                lf->nGPUs = nG;
	}

	void * operator()(void *) {
            if (lfd && lfd->stopProf)  // si hay driver y me dice que pare, paro el pipeline
                return NULL;
            Bundle *bundle = new Bundle();
            //If there are remaining iterations
            if (begin < end) {
                    //Checking whether the GPU is idle or not.
                    if (--gpuStatus >= 0) {
                        // pedimos un trozo para la GPU entre begin y end
                        int ckgpu = lf->getGPUChunk(begin, end);
#ifdef LDEBUG
                            cerr << "Serial Filter GPU: " << ckgpu << " Begin = " << begin << ", End = " << end << endl;
#endif
                        if (ckgpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                                int auxEnd = begin + ckgpu;
                                auxEnd = (auxEnd > end) ? end : auxEnd;
                                bundle->begin = begin;
                                bundle->end = auxEnd;
                                begin = auxEnd;
                                bundle->type = GPU;
                                return bundle;
                        } else { // paramos la GPU si el trozo es <= 0, generamos token CPU
                                // no incrementamos gpuStatus para dejar pillada la GPU
                                nCPUs++;
                                int auxEnd = begin + lf->getCPUChunk(begin, end);
                                auxEnd = (auxEnd > end) ? end : auxEnd;
                                bundle->begin = begin;
                                bundle->end = auxEnd;
                                begin = auxEnd;
                                bundle->type = CPU;
                                return bundle;
                        }
                    } else {
                        //CPU WORK
                        gpuStatus++;
                        int auxEnd = begin + lf->getCPUChunk(begin, end);
                        auxEnd = (auxEnd > end) ? end : auxEnd;
                        bundle->begin = begin;
                        bundle->end = auxEnd;
                        begin = auxEnd;
                        bundle->type = CPU;
#ifdef LDEBUG
                        //cerr << "Serial Filter CPU: " << bundle->end-bundle->begin << " Begin = " << bundle->begin << ", End = " << bundle->end << endl;
#endif
                        return bundle;
                    }
            }
            return NULL;
    } // end operator
};

/*MyParallelFilter class is the executor component of the engine, it executes the subrange onto the device selected by SerialFilter*/
template <class B>
class MyParallelFilter : public filter {
private:
	B *body;
	int iterationSpace;
        LogFitEngine *lf;
        LogFitDriver *lfd;
public:
    	MyParallelFilter(B *b, int i, LogFitEngine *_lf) :
		filter(false) {
		body = b;
		iterationSpace = i;
                lf = _lf;
                lfd = NULL;
	}

    	MyParallelFilter(B *b, int i, LogFitEngine *_lf, LogFitDriver *_lfd) :
		filter(false) {
		body = b;
		iterationSpace = i;
                lf = _lf;
                lfd = _lfd;
	}

	void * operator()(void * item) {

		//variables
		Bundle *bundle = (Bundle*) item;

		if (bundle->type == GPU) {
			// GPU WORK
			executeOnGPU(bundle);
                        gpuStatus++;
		} else {
			// CPU WORK
			executeOnCPU(bundle);
		}
		delete bundle;
		return NULL;
	}

	void executeOnGPU(Bundle *bundle) {
		static bool firstmeasurement = true;
		tick_count start_tc = tick_count::now();
#ifdef TRACER
                tr->gpuStart();
#endif
#ifdef OVERHEAD_STUDY
		//Calculating partition scheduling overhead
		overhead_sp = overhead_sp + ((start_tc - end_tc).seconds()*1000);

		//Adding a marker in the command queue
		cl_event event_before_h2d;
		int error = clEnqueueMarker(command_queue, &event_before_h2d);
		if (error != CL_SUCCESS) {
			cerr << "Failed equeuing start event" << endl;
			exit(0);
		}
#endif

		body->sendObjectToGPU(bundle->begin, bundle->end, NULL);

#ifdef OVERHEAD_STUDY
		//Adding a marker in the command queue
		cl_event event_before_kernel;
		error = clEnqueueMarker(command_queue, &event_before_kernel);
		if (error != CL_SUCCESS) {
			cerr << "Failed equeuing start event" << endl;
			exit(0);
		}
		cl_event event_kernel;
		body->OperatorGPU(bundle->begin, bundle->end, &event_kernel);
#else
		body->OperatorGPU(bundle->begin, bundle->end, NULL);
#endif

#ifdef OVERHEAD_STUDY
		//Adding a marker in the command queue
		cl_event event_after_kernel;
		error = clEnqueueMarker(command_queue, &event_after_kernel);
		if (error != CL_SUCCESS) {
			cerr << "Failed equeuing start event" << endl;
			exit(0);
		}
#endif

		body->getBackObjectFromGPU(bundle->begin, bundle->end, NULL);

#ifdef OVERHEAD_STUDY
		//Adding a marker in the command queue
		cl_event event_after_d2h;
		error = clEnqueueMarker(command_queue, &event_after_d2h);
		if (error != CL_SUCCESS) {
			cerr << "Failed equeuing start event" << endl;
			exit(0);
		}
#endif

		clFinish(command_queue);

#ifdef OVERHEAD_STUDY
		//Adding a marker in the command queue to computer thread dispatch
		cl_event event_after_finish;
		error = clEnqueueMarker(command_queue, &event_after_finish);
		if (error != CL_SUCCESS) {
			cerr << "Failed equeuing start event" << endl;
			exit(0);
		}
#endif

		end_tc = tick_count::now();

#ifdef OVERHEAD_STUDY
		//Calculating host to device transfer overhead
		clGetEventProfilingInfo(event_before_h2d, CL_PROFILING_COMMAND_END, sizeof (cl_ulong), &tg1, NULL);
		clGetEventProfilingInfo(event_before_kernel, CL_PROFILING_COMMAND_START, sizeof (cl_ulong), &tg2, NULL);
		if (tg2 > tg1) {
			overhead_h2d = overhead_h2d + (tg2 - tg1) / 1000000.0; // ms
		}
		//cerr << "Overhead h2d: " << overhead_h2d << ": " << tg1 << ", " << tg2 << " = " << (tg2-tg1) <<endl;

		//Calculating kernel launch overheads
		clGetEventProfilingInfo(event_before_kernel, CL_PROFILING_COMMAND_END, sizeof (cl_ulong), &tg2, NULL);
		clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_START, sizeof (cl_ulong), &tg3, NULL);
		if (tg3 > tg2) {
			overhead_kl = overhead_kl + (tg3 - tg2) / 1000000.0; // ms
		}
		//Calculating kernel execution
		//clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &tg3, NULL);
		clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_END, sizeof (cl_ulong), &tg4, NULL);
		if (tg4 > tg3) {
			kernel_execution = kernel_execution + (tg4 - tg3) / 1000000.0; // ms
		}
		//Calculating device to host transfer overhead
		//clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_COMPLETE, sizeof(cl_ulong), &tg4, NULL);
		clGetEventProfilingInfo(event_after_d2h, CL_PROFILING_COMMAND_START, sizeof (cl_ulong), &tg5, NULL);
		if (tg5 > tg4) {
			overhead_d2h = overhead_d2h + (tg5 - tg4) / 1000000.0; // ms
		}
		//Calculating device overhead thread dispatch
		if (tg5 > tg1) {
			overhead_td = overhead_td + ((end_tc - start_tc).seconds()*1000) - ((tg5 - tg1) / 1000000.0); // ms
		}

		//Releasing event objects
		clReleaseEvent(event_before_h2d);
		clReleaseEvent(event_before_kernel);
		clReleaseEvent(event_kernel);
		clReleaseEvent(event_after_kernel);
		clReleaseEvent(event_after_d2h);
		clReleaseEvent(event_after_finish);
#endif
#ifdef TRACER
                tr->gpuStop();
#endif

		float time = (end_tc - start_tc).seconds()*1000;

		//Summarizing GPU Statistics
		totalIterationsGPU = totalIterationsGPU + (bundle->end - bundle->begin);
		itemsOnGPU = itemsOnGPU + 1;

#ifdef DEEP_GPU_REPORT
#ifdef H2D
                if (!(lf->h2d))
#endif
                    deep_gpu_report << bundle->end-bundle->begin << "\t" << (bundle->end - bundle->begin) / time << endl;
#endif

                if(!lfd) {
                    lf->recordGPUTh((bundle->end - bundle->begin), time);
                } else if (lfd->recordTh) {
                    int stable = lf->recordGPUTh((bundle->end - bundle->begin), time);
                    if (stable)
                        lfd->stableReached();  // indicamos al driver que se al alcanzado la fase estable para que pueda empezar a medir
                }

#ifdef LDEBUG
		cerr << "** Chunk GPU: " << bundle->end - bundle->begin << " TH: " << ((bundle->end - bundle->begin) / time) << " Time: " << time << endl;
#endif
	}

	void executeOnCPU(Bundle *bundle) {
#ifdef TRACER
                tr->cpuStart();
#endif
		tick_count start = tick_count::now();
		body->OperatorCPU(bundle->begin, bundle->end);
		tick_count end = tick_count::now();
#ifdef TRACER
                tr->cpuStop();
#endif
		float time = (end - start).seconds()*1000;

                if(!lfd || lfd->recordTh)
                    lf->recordCPUTh((bundle->end - bundle->begin), time);
#ifdef DEEP_CPU_REPORT
                {
                    myMutex_t::scoped_lock myLock(deepCPUMutex);
                    //cerr << bundle->end-bundle->begin << "\t" << (bundle->end - bundle->begin) / time << endl;
                    deep_cpu_report << bundle->end-bundle->begin << "\t" << (bundle->end - bundle->begin) / time << endl;
                }
#endif
#ifdef LDEBUG
		//cerr << "=== Chunk CPU: " << bundle->end - bundle->begin << " TH: " << (bundle->end - bundle->begin)/((end - start).seconds()*1000) << endl;
#endif
	}
};
//end class

/*****************************************************************************
 * Parallel For
 * **************************************************************************/
template<class T>
class ParallelForLF {
	T *body;
public:
	ParallelForLF(T *b){
		body = b;
	}

    void operator()( const blocked_range<int>& range ) const {
#ifdef TRACER
                tr->cpuStart();
#endif
        tick_count start = tick_count::now();
	body->OperatorCPU(range.begin(), range.end());
        tick_count end = tick_count::now();
#ifdef TRACER
                tr->cpuStop();
#endif
        float time = (end - start).seconds()*1000;

#ifdef DEEP_CPU_REPORT
        int rg = range.end() - range.begin();
        {
            myMutex_t::scoped_lock myLock(deepCPUMutex);
            //cerr << rg << "\t" << (rg) / time << endl;
            deep_cpu_report << rg << "\t" << (rg) / time << endl;
        }
#endif

    }
};

// Note: Reads input[0..n] and writes output[1..n-1].
template<class T>
void ParallelFORCPUsLF(size_t start, size_t end, T *body ) {
    ParallelForLF<T> pf(body);
	parallel_for( blocked_range<int>( start, end ), pf, auto_partitioner() );
}

/*****************************************************************************
 * LogFit scheduler
 * **************************************************************************/
class LogFit : public Scheduler<LogFit, Params> {
	Params *pars;
        LogFitEngine *lfe;
        LogFitDriver *lfd;
#ifdef TIMESTEP_PW_REPORT
        vector<CoreCounterState> cstates1ep, cstates2ep;
	vector<SocketCounterState> sktstate1ep, sktstate2ep;
	SystemCounterState sstate1ep, sstate2ep;
#endif
public:

	/*This constructor just call his parent's contructor*/
	LogFit(Params *p) : Scheduler<LogFit, Params>(p) {
		//Params * p = (Params*) params;
		pars = p;
                lfe = new LogFitEngine(nCPUs, nGPUs, computeUnits /*chunk minimo GPU*/, 10 /*chunk minimo cpu*/);
                lfd = new LogFitDriver(lfe);
#ifdef ENERGYPROF
                lfd->pcm = pcm;
#endif
		timeStep = 0;
		benchName = strdup(pars->benchName);
#ifdef OVERHEAD_STUDY
		sprintf(p->benchName, "%s_OVERHEAD", p->benchName);
#endif
#ifdef TRACER
                char version[100];
		sprintf(version, "%s_Logfit_Trace_%d_%d.json", pars->benchName, nCPUs, nGPUs);
                tr = new PFORTRACER(version);
#endif
	}

	/*The main function to be implemented*/
	template<class T>
	void heterogeneous_parallel_for(int begin, int end, T *body) {
#ifdef TRACER
                char version[100];
		sprintf(version, "TS%d", timeStep);
                tr->newEvent(version);
#endif
#ifdef DEEP_GPU_REPORT
		char versiongpu[100];
		sprintf(versiongpu, "%s_Logfit_deep_GPU_report_%d_%d.txt",pars->benchName, nCPUs, nGPUs);
		deep_gpu_report.open(versiongpu, ios::out | ios::app);
#endif
#ifdef DEEP_CPU_REPORT
		char versioncpu[100];
		sprintf(versioncpu, "%s_Logfit_deep_CPU_report_%d_%d.txt",pars->benchName, nCPUs, nGPUs);
		deep_cpu_report.open(versioncpu, ios::out | ios::app);
#endif
#ifdef TIMESTEP_THR_REPORT
		char versionttr[100];
		sprintf(versionttr, "%s_Logfit_timestep_throughput_report_%d_%d.txt",pars->benchName, nCPUs, nGPUs);
		timestep_thr_report.open(versionttr, ios::out | ios::app);
#endif
#ifdef TIMESTEP_THR_REPORT
#ifdef TIMESTEP_PW_REPORT
		pcm->getAllCounterStates(sstate1ep, sktstate1ep, cstates1ep);
#endif
                tick_count ttrbegin = tick_count::now();
#endif

		timeStep++;
#ifdef LDEBUG
                cout << " ---- Time step: " << timeStep << " ----" << endl;
#endif


		gpuStatus = nGPUs;
                totalIterations += (end - begin);

                lfe->reStart();
		body->firsttime = true;

#ifdef HOM_PROF
                lfd->run(begin, end, body);
#else
                lfd->runHet(begin, end, body);
                /*
		if (nGPUs < 1) {  // solo CPUs
			ParallelFORCPUsLF(begin, end, body);
		} else {  // ejecucion con GPU
                        pipeline pipe;
                        MySerialFilter<T> serial_filter(begin, end, nCPUs, nGPUs, lfe);
                        MyParallelFilter<T> parallel_filter(body, end - begin, lfe);
                        pipe.add_filter(serial_filter);
                        pipe.add_filter(parallel_filter);
#ifdef OVERHEAD_STUDY
                        end_tc = tick_count::now();
#endif
                        pipe.run(nCPUs + nGPUs);
			pipe.clear();
		}
                */
#endif

#ifdef TIMESTEP_THR_REPORT
                tick_count ttrend = tick_count::now();
                float time = (ttrend - ttrbegin).seconds()*1000;
                timestep_thr_report << (end - begin) << "\t" << (end-begin)/time;

#ifdef TIMESTEP_PW_REPORT
                pcm->getAllCounterStates(sstate2ep, sktstate2ep, cstates2ep);

                double CPUEnergy = getPP0ConsumedJoules(sstate1ep, sstate2ep);
                double GPUEnergy = getPP1ConsumedJoules(sstate1ep, sstate2ep);
                double TotalEnergy = getConsumedJoules(sstate1ep, sstate2ep);
                timestep_thr_report << "\t"
                        << CPUEnergy/time << "\t"
                        << GPUEnergy/time << "\t"
                        << TotalEnergy/time;
#endif
                timestep_thr_report << endl;
                timestep_thr_report.close();
#endif
#ifdef DEEP_GPU_REPORT
                deep_gpu_report.close();
#endif
#ifdef DEEP_CPU_REPORT
                deep_cpu_report.close();
#endif
#ifdef TRACER
                tr->newEvent(version);
#endif
	}

	/*this function print info to a Log file*/

    void saveResultsForBench() {

		char * execution_name = (char *) malloc(sizeof (char)*256);
		sprintf(execution_name, "_LOGFIT_%d_%d.txt", nCPUs, nGPUs);
		strcat(pars->benchName, execution_name);

		/*Checking if the file already exists*/
		bool fileExists = isFile(pars->benchName);
		ofstream file(pars->benchName, ios::out | ios::app);
		if (!fileExists) {
			printHeaderToFile(file);
		}
		cerr << "*************************" << endl;
		cerr << nCPUs << SEP << nGPUs << SEP << runtime;
                if (lfd->policy == 0) cerr << SEP << "UNKP" << endl;
                else if (lfd->policy == 1) cerr << SEP << " HETP" << endl;
                else if (lfd->policy == 2) cerr << SEP << "CPUP" << endl;
                else if (lfd->policy == 3) cerr << SEP << "GPUP" << endl;

		//Save information to file
		file << nCPUs << SEP << nGPUs << SEP << runtime << SEP
#ifdef OVERHEAD_STUDY
				<< kernel_execution << SEP << overhead_td << SEP << overhead_sp << SEP << overhead_h2d << SEP << overhead_kl << SEP << overhead_d2h << SEP
#endif
#ifdef ENERGYCOUNTERS
				<< getPP0ConsumedJoules(sstate1, sstate2) << SEP << getPP1ConsumedJoules(sstate1, sstate2) << SEP
				<< getConsumedJoules(sstate1, sstate2) - getPP0ConsumedJoules(sstate1, sstate2) - getPP1ConsumedJoules(sstate1, sstate2) << SEP << getConsumedJoules(sstate1, sstate2) << SEP
				<< getL2CacheHits(sktstate1[0], sktstate2[0]) << SEP << getL2CacheMisses(sktstate1[0], sktstate2[0]) << SEP << getL2CacheHitRatio(sktstate1[0], sktstate2[0]) << SEP
				<< getL3CacheHits(sktstate1[0], sktstate2[0]) << SEP << getL3CacheMisses(sktstate1[0], sktstate2[0]) << SEP << getL3CacheHitRatio(sktstate1[0], sktstate2[0]) << SEP
				<< getCyclesLostDueL3CacheMisses(sstate1, sstate2) << SEP
#endif
                                << (itemsOnGPU ? totalIterationsGPU / itemsOnGPU : 0) << SEP << itemsOnGPU << SEP
                                << lfd->policy << SEP
				<< endl;
		file.close();

		//Print information in console
#ifdef DEBUG
		cerr << nCPUs << SEP << nGPUs << SEP << runtime << SEP
#ifdef OVERHEAD_STUDY
				<< overhead_td << SEP << overhead_sp << SEP << overhead_h2d / 1000000.0 << SEP << overhead_kl << SEP << overhead_d2h << SEP
#endif
#ifdef ENERGYCOUNTERS
				<< getPP0ConsumedJoules(sstate1, sstate2) << SEP << getPP1ConsumedJoules(sstate1, sstate2) << SEP
				<< getConsumedJoules(sstate1, sstate2) - getPP0ConsumedJoules(sstate1, sstate2) - getPP1ConsumedJoules(sstate1, sstate2) << SEP << getConsumedJoules(sstate1, sstate2) << SEP
				<< getL2CacheHits(sktstate1[0], sktstate2[0]) << SEP << getL2CacheMisses(sktstate1[0], sktstate2[0]) << SEP << getL2CacheHitRatio(sktstate1[0], sktstate2[0]) << SEP
				<< getL3CacheHits(sktstate1[0], sktstate2[0]) << SEP << getL3CacheMisses(sktstate1[0], sktstate2[0]) << SEP << getL3CacheHitRatio(sktstate1[0], sktstate2[0]) << SEP
				<< getCyclesLostDueL3CacheMisses(sstate1, sstate2) << SEP
#endif
                                << (itemsOnGPU ? totalIterationsGPU / itemsOnGPU : 0) << SEP << itemsOnGPU << SEP
				<< endl;
#endif
    }

    void printHeaderToFile(ofstream &file) {
		file << "N. CPUs" << SEP << "N. GPUs" << SEP << "Time (ms)" << SEP

#ifdef OVERHEAD_STUDY
				<< "Kernel Execution" << SEP << "O. Th. Dispatch" << SEP << "O. sched. Partitioning" << SEP << "O. Host2Device" << SEP << "O. Kernel Launch" << SEP << "O. Device2Host" << SEP
#endif
#ifdef ENERGYCOUNTERS
				<< "CPU Energy(J)" << SEP << "GPU Enegy(J)" << SEP << "Uncore Energy(J)" << SEP << "Total Energy (J)" << SEP
				<< "L2 Cache Hits" << SEP << "L2 Cache Misses" << SEP << "L2 Cache Hit Ratio" << SEP
				<< "L3 Cache Hits" << SEP << "L3 Cache Misses" << SEP << "L3 Cache Hit Ratio" << SEP << "Cycles lost Due to L3 Misses" << SEP
#endif
                                << "Avg GPU chunkSize" << SEP << "items GPU" << SEP
                                << "LFD Policy" << SEP
				<< endl;
    }


#define PRINTCPUSGPUS
    void saveResultsForBenchDen(int inputId, int nrIter, int platformId, float rtHetFor){

        char * execution_name = (char *)malloc(sizeof(char)*100);
        sprintf(execution_name, "_GL-TBB_P.txt");
        strcat(pars->benchName, execution_name);

        /*Checking if the file already exists*/
        bool fileExists = isFile(pars->benchName);
        ofstream file(pars->benchName, ios::out | ios::app);
        if(!fileExists){
            printHeaderToFileDen(file);
        }
#ifdef ENERGYCOUNTERS
        double eG, eC, eM, eT;
        eG = getPP1ConsumedJoules(sstate1, sstate2);
        eC = getPP0ConsumedJoules(sstate1, sstate2);
        eT = getConsumedJoules(sstate1, sstate2);
        eM = eT - eG - eC;
#endif
        //print implementation id and size

        //file << (int)DYNAMIC << "\t" << inputId/2-1 << "\t" << platformId << "\t"
        file << (int)LOGFIT << "\t" << inputId/2-1 << "\t"
#ifdef PRINTCPUSGPUS
       << pars->numcpus << "\t"
       << pars->numgpus << "\t"
#endif
       << platformId << "\t"
       << float(totalIterationsGPU)/float(totalIterations) << "\t"
#ifdef ENERGYCOUNTERS
			 << eG << "\t"
			 << eC << "\t"
			 << eM << "\t"
			 << eT << "\t"
#endif
			 << runtime << "\t"
			 << nrIter << "\t"
                        << rtHetFor << "\t"
#ifdef OVERHEAD_STUDY
        		<< kernel_execution << "\t"
                        << overhead_td << "\t"
                        << overhead_sp << "\t"
                        << overhead_h2d << "\t"
                        << overhead_kl << "\t"
                        << overhead_d2h << "\t"
#endif
#ifdef CACHE_STUDY
                        << getL3CacheHits(sktstate1[0], sktstate2[0]) << "\t"
                        << getL3CacheMisses(sktstate1[0], sktstate2[0]) << "\t"
                        << getL3CacheHitRatio(sktstate1[0], sktstate2[0]) << "\t"
                        << getCyclesLostDueL3CacheMisses(sstate1, sstate2) << "\t"
#endif
                        << endl;
        file.close();

	  //cout << "\nDYNAMIC" << "\t" << inputId/2-1 << "\t" << platformId << "\t"
        cout << "\nLOGFIT" << "\t IN_SIZE: " << inputId << "\t PLATFORM_ID: " << platformId
                        << "\t ratioGPU: " << float(totalIterationsGPU)/float(totalIterations)
#ifdef ENERGYCOUNTERS
                        << "\t eT(Joule): " << eT
#endif
                        << "\t runtime(s): " << runtime
                        << "\t nrIter: " << nrIter
                        << "\t rtHetFor: " << rtHetFor
#ifdef OVERHEAD_STUDY
			<< endl
                        << "\t KernelEx(ms): " << kernel_execution
                        << "\t ThDispatch(ms): " << overhead_td
                        << "\t SchedPartitioning(ms): " << overhead_sp
                        << "\t H2D(ms): " << overhead_h2d
                        << "\t KernelLauch(ms): " << overhead_kl
                        << "\t D2H(ms): " << overhead_d2h
                        << "\t Total overhead(s): " << (kernel_execution+overhead_td+overhead_sp+overhead_h2d+overhead_kl+overhead_d2h)/1000
#endif
#ifdef CACHE_STUDY
        		<< "\t L3 Cache Hits: " << getL3CacheHits(sktstate1[0], sktstate2[0])
                        << "\t L3 Cache Misses: " << getL3CacheMisses(sktstate1[0], sktstate2[0])
                        << "\t L3 Cache Hit Ratio: " << getL3CacheHitRatio(sktstate1[0], sktstate2[0])
                        << "\t Cycles lost Due to L3 Misses: " << getCyclesLostDueL3CacheMisses(sstate1, sstate2)

#endif
                        << endl;
#ifdef LDEBUG
		cerr << nCPUs << "\t" << nGPUs << "\t"  << runtime << "\t"
#ifdef ENERGYCOUNTERS
		<< getPP0ConsumedJoules(sstate1, sstate2) << "\t" << getPP1ConsumedJoules(sstate1, sstate2) << "\t"
		<< getConsumedJoules(sstate1, sstate2) - getPP0ConsumedJoules(sstate1, sstate2) - getPP1ConsumedJoules(sstate1, sstate2) << "\t" <<  getConsumedJoules(sstate1, sstate2) << "\t"
		<< getL2CacheHits(sktstate1[0], sktstate2[0]) << "\t" << getL2CacheMisses(sktstate1[0], sktstate2[0]) << "\t" << getL2CacheHitRatio(sktstate1[0], sktstate2[0]) <<"\t"
		<< getL3CacheHits(sktstate1[0], sktstate2[0]) << "\t" << getL3CacheMisses(sktstate1[0], sktstate2[0]) << "\t" << getL3CacheHitRatio(sktstate1[0], sktstate2[0]) <<"\t"
		<< getCyclesLostDueL3CacheMisses(sstate1, sstate2)
#endif
		<< endl;

#endif
	}

	void printHeaderToFileDen(ofstream &file){
        file << "ALG_ID" << SEP << "INPUT_ID/2-1" << SEP
#ifdef PRINTCPUSGPUS
            << "N.CPUs" << "\t"
            << "N.GPUs" << "\t"
#endif
            << "PLATFORM_ID" << SEP
            << "GPU_RATIO" << SEP << "E_GPU(J)" << SEP << "E_CPU(J)" << SEP << "E_MEM(J)" << SEP << "E_TOT(J)" << SEP
            << "TIME(s)" << SEP
            << "NR_ITER" << SEP
            << "HET_FOR_TIME(s)" << SEP
#ifdef OVERHEAD_STUDY
	    << "Kernel Execution" << SEP << "O. Th. Dispatch" << SEP << "O. sched. Partitioning"
            << SEP << "O. Host2Device" << SEP << "O. Kernel Launch" << SEP << "O. Device2Host" << SEP
#endif
#ifdef CACHE_STUDY
            << "L3 Cache Hits" << SEP
            << "L3 Cache Misses" << SEP
            << "L3 Cache Hit Ratio" << SEP
            << "Cycles lost Due to L3 Misses" << SEP
#endif
            << endl;
	}
};
