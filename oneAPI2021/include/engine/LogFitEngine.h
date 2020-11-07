//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_LOGFITENGINE_H
#define HETEROGENEOUS_PARALLEL_FOR_LOGFITENGINE_H

#include "../../lib/Interfaces/Engines/IEngine.cpp"

#define H2D //no tener en cuenta el primer envio para logfit quitando así el efecto del H2D si solo se hace en exploracion
#define SLOPE  //criterio de parada de fase inicial basado en la pendiente de la recta de las dos ultimas medidas
#define SECONDCHANCE // version en la que LogFit da una segunda oportunidad a un chunk si el nuevo no ha mejorado su TH

#define MAXSEG 64           // numero maximo del array que almacena los segmentos
#define CHKTHRESHOLD 0.10   // tanto por ciento de las iteraciones restantes por debajo del cual no se reparte y se lo queda todo un dispositivo
#define MINPOINTS 4         // numero minimo de puntos para hacer logfit
#define GPUSLOPE 0.005      // pendiente del th para encontrar el codo
#define LogFitTHProfit 0.9  // tanto por ciento que se permite empeorar el th de la gpu antes de dar segunda oportunidad

class LogFitEngine : public IEngine {
    unsigned int limits[MAXSEG];    // array con los limites de los segmentos del logfit
    unsigned int actual;            // indice al limite actual para hacer una busqueda mas rapida
    unsigned int lastLimit;         // indice al ultimo limite inicializado en el codigo actual
    unsigned int npoints;           // indice del punto donde se alcanza el codo
    unsigned int initGChunk;        // chunk inicial para la GPU (y minimo tambien)
    unsigned int initCChunk;        // chunk inicial para la CPU (y minimo tambien)
    unsigned int chk[MAXSEG];       // array con los chunks de GPU utilizados para el ajuste
    float thr[MAXSEG];              // array con los throughputs registrados para los chunks de GPU usados
    enum {
        EXPLORATION, ELBOW, STABLE
    } stage; // etapa en la que se encuentra el algoritmo logfit
    bool lastChunk;                 // ultimo chunk para la GPU

    enum {
        NORMAL, SCAN, CONSOLIDATE
    } logFitMode;
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
    LogFitEngine(unsigned int _nCPUs, unsigned int _nGPUs, unsigned int _initGChunk, unsigned int _initCChunk);

    // inicializacion para un TS nuevo
    void reStart();

    // registra un nuevo throughput de un core
    void recordCPUTh(unsigned int chunk, float time);

    bool stablePhase();

    // registra un nuevo throughput de la GPU marcando la evolucion del algoritmo LogFit
    // cambia de estado del algoritmo y precalcula el chunk de gpu siguiente (adapta tamaño)
    // tan solo no tiene en cuenta final phase, que se tiene en cuenta al pedir chunk y ver
    // cuanto queda.
    void recordGPUTh(unsigned int chunk, float time);

    // devuelve el chunk de CPU a usar
    unsigned int getCPUChunk(unsigned int begin, unsigned int end);

    // devuelve el chunk de GPU a usar. Si el chunk es 0, no usar la GPU
    unsigned int getGPUChunk(unsigned int begin, unsigned int end);

private:

    // devuelve true si se cumple que hemos alcanzado el codo de la curva de th de la GPU
    bool elbowCondition();

    // devuelve true si se cumple parcialmente la condicion del codo en el punto dado
    bool elbowCondition(int pos);

    // calcula la aproximacion logaritmica a los puntos x,y medidos de chunk,thr de la
    // GPU, y devuevle el chunk siguiente de GPU adecuado para la ultima medida de througput
    // hace el fitting con todos los puntos y no se almacena nada ya que pueden cambiar los puntos
    unsigned int calculateLogarithmicModel();
};

#endif //HETEROGENEOUS_PARALLEL_FOR_LOGFITENGINE_H
