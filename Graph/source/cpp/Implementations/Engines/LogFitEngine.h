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

//#define DEEP_GPU_REPORT

//#define PJTRACER  // Traza visual de los chunks de CPU y GPU
//#define DEBUGLOG  // Depurar fitting y stop condition


#define H2D //no tener en cuenta el primer envio para logfit quitando así el efecto del H2D si solo se hace en exploracion
#define SLOPE  //criterio de parada de fase inicial basado en la pendiente de la recta de las dos ultimas medidas
#define SECONDCHANCE // version en la que LogFit da una segunda oportunidad a un chunk si el nuevo no ha mejorado su TH

#include <cstdlib>
#include <iostream> 
#include <fstream>
#include <list>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
#include "tbb/task.h"
#include "tbb/atomic.h"
#include "tbb/parallel_for.h"
#include "../../DataStructures/ProvidedDataStructures.h"
#include <math.h>

using namespace std;
using namespace tbb;


/*****************************************************************************
 * Defines
 * **************************************************************************/
#define GPU_OFF -100
#define MAXPOINTS 1024

/*****************************************************************************
 * Global variables
 * **************************************************************************/

#ifdef DEEP_GPU_REPORT
ofstream deep_gpu_report;
#endif

atomic<int> gpuStatus;

int timeStep;

char* benchName;

//Summarize GPU statistics
int itemsOnGPU = 0;
int totalIterationsGPU = 0;
tick_count end_tc;

/*****************************************************************************
 * Heterogeneous Scheduler
 * **************************************************************************/

#define MAXSEG 64           // numero maximo del array que almacena los segmentos
#define CHKTHRESHOLD 0.10   // tanto por ciento de las iteraciones restantes por debajo del cual no se reparte y se lo queda todo un dispositivo
#define MINPOINTS 4         // numero minimo de puntos para hacer logfit
#define GPUSLOPE 0.005      // pendiente del th para encontrar el codo
#define LogFitTHProfit 0.9  // tanto por ciento que se permite empeorar el th de la gpu antes de dar segunda oportunidad
class LogFitEngine : IEngine {
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

#ifdef DEBUG
    char* stageStrings[3] = { "EXPLORATION", "ELBOW", "STABLE" };
    char* logFitMode_strings[3] = { "NORMAL","SCAN","CONSOLIDATE" };
#endif 
    
    enum {NORMAL, SCAN, CONSOLIDATE} logFitMode;
    float gpuThroughputCandidate;   // para no olvidar el th de la segunda oportunidad
    unsigned int chunkGPUCandidate; // ni el chunk
    
    float CPUTh;                    
    float GPUTh;
    
    unsigned int nextGPUChk;        // siguiente chunk de GPU a asignar (calculado previamente)
    unsigned int nextCPUChk;        // siguiente chunk de CPU a asignar (calculado previamente)       
    
    bool GPUstopped;                // indica si la GPU esta parada o no
    unsigned int nCPUs;             // numero de cores
    unsigned int nGPUs;             // numero de GPUs
    
    float threshold;                // threshold donde se encuentra el codo
    
#ifdef H2D
    bool h2d;
#endif    
public:
    LogFitEngine(unsigned int _nCPUs, unsigned int _nGPUs, unsigned int _initGChunk, unsigned int _initCChunk) : 
        initGChunk(_initGChunk), initCChunk(_initCChunk), 
        nextGPUChk(_initGChunk), nextCPUChk(_initCChunk), nCPUs(_nCPUs), nGPUs(_nGPUs) {
            actual = 0;
            lastLimit = 0;
            npoints = 0;
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
#ifdef DEBUG
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
    
#ifdef DEBUG
#define DEBUGSEC
#endif
    // registra un nuevo throughput de la GPU marcando la evolucion del algoritmo LogFit
    // cambia de estado del algoritmo y precalcula el chunk de gpu siguiente (adapta tamaño)
    // tan solo no tiene en cuenta final phase, que se tiene en cuenta al pedir chunk y ver 
    // cuanto queda.
    void recordGPUTh(unsigned int chunk, float time) {
        static int elbow;
        static bool notEnough; // indica si en la ultima vez no hubo iteraciones suficientes para la GPU
#ifdef H2D
            if (h2d) {
                h2d = false;
                //GPUTh = (float) chunk / (float) time;
#ifdef DEBUG
        cerr << "H2D - No se registra GPUTh ("<< stageStrings[stage] <<"): chunk " << chunk << " TH " << (float) chunk / (float) time << endl;
#endif
                return;
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
#ifdef DEBUG
        cerr << "--> Registrado GPUTh ("<< stageStrings[stage] <<"): chunk " << chunk << " TH " << GPUTh << endl;
#endif
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
        } else { // ya hay th de ambos dispositivos y se puede calcular la velocidad relativa (fase final)
            float relSpeed = GPUTh / CPUTh;
#ifdef DEBUG
                cerr << "rem " << rem << " nextGPUChk " << nextGPUChk << " nextGPUChk / relSpeed " << nextGPUChk / relSpeed 
                        << " (rem - nextGPUChk) / nCPUs " << (rem - nextGPUChk) / nCPUs << endl;
#endif
            if (rem < nextGPUChk || nextGPUChk / relSpeed > (rem - nextGPUChk) / nCPUs) {// No hay suficiente para repartir
                lastChunk = true;  // ya es el ultimo reparto
                // Hay suficiente para repartir y que la GPU no reciba menos que el chunk del codo?
                // tiempo en procesarlo todo en los cores
                float timeOnCPU = rem / (nCPUs * CPUTh);    
                // calculamos tiempo en GPU solo
                float timeOnGPU;
                if (rem >= chk[npoints]) { // el numero de iteraciones esta por encima del codo -> consideramos TH GPU constante, el actual
                    timeOnGPU = rem / (GPUTh);
                } else { // si esta por debajo, lo aproximamos por una unica recta entre el primer punto y el del codo
                         // y = (x-x1)*(y2 - y1)/(x2-x1) + y1
                    float y = (rem - chk[0]) * (GPUTh - thr[0]) / (chk[npoints-1] - chk[0]) + thr[0];
                    timeOnGPU = rem / y;
                }
                // estimamos tiempo para un reparto entre GPU y cores
                // Caso 1: Parte plana del codo de la GPU (x = chunk de corte para GPU)
                //  x / GPUth = (rem - x) / (nCPUs * CPUth)
                unsigned int x = GPUTh * rem / (nCPUs * CPUTh + GPUTh);
                
                // TODO: Caso2: parte ascendente del codo (como una recta)
                
                // si el trozo esta muy pegado a extremo de iteraciones (0 o rem), no repartirmos
                float timeHet = x / GPUTh;
#ifdef DEBUG
                cerr << "Fase final: rem " << rem << " timeGPU " << timeOnGPU 
                        << " timeCPU " << timeOnCPU << " timeHet " << timeHet 
                        << " (GPU chunk "<< x << ")";
#endif

                if (x < chk[npoints] || x > rem - CHKTHRESHOLD*rem) { // muy poco o mucho para GPU, todo para un dispositivo
                    if (timeOnGPU < timeOnCPU) {
#ifdef DEBUG
                        cerr << " --> Todo a GPU " << endl;
#endif
                        return rem; // todo para GPU
                    }
                    else {
#ifdef DEBUG
                        cerr << " --> Todo a CPU " << endl;
#endif
                        GPUstopped = true;  // todo para CPU y se para la GPU
                        return 0;
                    }
                } else {  // hay suficiente para repartir entre GPU y cores, ver el menor tiempo
                    if (timeHet < timeOnCPU && timeHet < timeOnGPU) {
#ifdef DEBUG
                        cerr << " --> " << x << " a GPU " << endl;
#endif
                        return x;
                    }
                    else if (timeOnGPU < timeOnCPU) {
#ifdef DEBUG
                        cerr << " --> Todo a GPU " << endl;
#endif
                        return rem;
                    }
                    else {
#ifdef DEBUG
                        cerr << " --> Todo a CPU " << endl;
#endif
                        GPUstopped = true;
                        return 0;
                    }
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
 
#ifdef DEBUG
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