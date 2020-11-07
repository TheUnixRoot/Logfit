
#ifndef LIBRARY_ENGINEFACTORY_H
#define LIBRARY_ENGINEFACTORY_H

#include "../../lib/Implementations/Engines/LogFitEngine.cpp"
#include "../../lib/Implementations/Engines/DynamicEngine.cpp"

namespace HelperFactories {
    enum EngineType : int {
        Logfit = 0, Dynamic = 1
    };

    struct EngineFactory {
    public:
        template<typename TEngine>
        static inline typename std::enable_if<std::is_same<TEngine, DynamicEngine>::value, TEngine *>::type
        getInstance(unsigned int _nCPUs, unsigned int _nGPUs, unsigned int _initGChunk, unsigned int _initCChunk = 0) {
            return new DynamicEngine(_nCPUs, _nGPUs, _initGChunk);
        }

        template<typename TEngine>
        static inline typename std::enable_if<std::is_same<TEngine, LogFitEngine>::value, TEngine *>::type
        getInstance(unsigned int _nCPUs, unsigned int _nGPUs, unsigned int _initGChunk, unsigned int _initCChunk = 0) {
            return new LogFitEngine(_nCPUs, _nGPUs, _initGChunk, _initCChunk);
        }

        template<typename TEngine>
        static inline void deleteInstance(TEngine *instance) {
            delete (instance);
        }
    };

}
#endif //LIBRARY_ENGINEFACTORY_H