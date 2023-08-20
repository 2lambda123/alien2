#include "CudaSimulationFacade.cuh"

#include <functional>
#include <iostream>
#include <list>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <device_launch_parameters.h>
#include <cuda/helper_cuda.h>

#include "Base/Exceptions.h"
#include "Base/LoggingService.h"

#include "EngineInterface/InspectedEntityIds.h"
#include "EngineInterface/SimulationParameters.h"
#include "EngineInterface/GpuSettings.h"
#include "EngineInterface/SpaceCalculator.h"

#include "DataAccessKernels.cuh"
#include "TOs.cuh"
#include "Base.cuh"
#include "GarbageCollectorKernels.cuh"
#include "ConstantMemory.cuh"
#include "CudaMemoryManager.cuh"
#include "SimulationStatistics.cuh"
#include "Objects.cuh"
#include "Map.cuh"
#include "StatisticsKernels.cuh"
#include "EditKernels.cuh"
#include "RenderingKernels.cuh"
#include "SimulationData.cuh"
#include "SimulationKernelsLauncher.cuh"
#include "DataAccessKernelsLauncher.cuh"
#include "RenderingKernelsLauncher.cuh"
#include "EditKernelsLauncher.cuh"
#include "StatisticsKernelsLauncher.cuh"
#include "SelectionResult.cuh"
#include "RenderingData.cuh"
#include "TestKernelsLauncher.cuh"

namespace
{
    class CudaInitializer
    {
    public:
        static void init() { getInstance(); }
        static std::string getGpuName() { return getInstance()._gpuName; }

        CudaInitializer()
        {
            int deviceNumber = getDeviceNumberOfHighestComputeCapability();

            auto result = cudaSetDevice(deviceNumber);
            if (result != cudaSuccess) {
                throw SystemRequirementNotMetException("CUDA device could not be initialized.");
            }

            std::stringstream stream;
            stream << "device " << deviceNumber << " is set";
            log(Priority::Important, stream.str());
        }

        ~CudaInitializer() { cudaDeviceReset(); }

    private:
        static CudaInitializer& getInstance()
        {
            static CudaInitializer instance;
            return instance;
        }

        int getDeviceNumberOfHighestComputeCapability()
        {
            int result = 0;
            int numberOfDevices;
            CHECK_FOR_CUDA_ERROR(cudaGetDeviceCount(&numberOfDevices));
            if (numberOfDevices < 1) {
                throw SystemRequirementNotMetException("No CUDA device found.");
            }
            {
                std::stringstream stream;
                if (1 == numberOfDevices) {
                    stream << "1 CUDA device found";
                } else {
                    stream << numberOfDevices << " CUDA devices found";
                }
                log(Priority::Important, stream.str());
            }

            int highestComputeCapability = 0;
            for (int deviceNumber = 0; deviceNumber < numberOfDevices; ++deviceNumber) {
                cudaDeviceProp prop;
                CHECK_FOR_CUDA_ERROR(cudaGetDeviceProperties(&prop, deviceNumber));

                std::stringstream stream;
                stream << "device " << deviceNumber << ": " << prop.name << " with compute capability " << prop.major
                       << "." << prop.minor;
                log(Priority::Important, stream.str());

                int computeCapability = prop.major * 100 + prop.minor;
                if (computeCapability > highestComputeCapability) {
                    result = deviceNumber;
                    highestComputeCapability = computeCapability;
                    _gpuName = prop.name;
                }
            }
            if (highestComputeCapability < 600) {
                throw SystemRequirementNotMetException(
                    "No CUDA device with compute capability of 6.0 or higher found.");
            }

            return result;
        }

        std::string _gpuName;
    };
}

void _CudaSimulationFacade::initCuda()
{
    CudaInitializer::init();
}

_CudaSimulationFacade::_CudaSimulationFacade(uint64_t timestep, Settings const& settings)
{
    CHECK_FOR_CUDA_ERROR(cudaGetLastError());

    _settings.generalSettings = settings.generalSettings;
    setSimulationParameters(settings.simulationParameters);
    setGpuConstants(settings.gpuSettings);

    log(Priority::Important, "initialize simulation");

    _cudaSimulationData = std::make_shared<SimulationData>();
    _cudaRenderingData = std::make_shared<RenderingData>();
    _cudaSelectionResult = std::make_shared<SelectionResult>();
    _cudaAccessTO = std::make_shared<DataTO>();
    _simulationStatistics = std::make_shared<SimulationStatistics>();

    _cudaSimulationData->init({settings.generalSettings.worldSizeX, settings.generalSettings.worldSizeY}, timestep);
    _cudaRenderingData->init();
    _simulationStatistics->init();
    _cudaSelectionResult->init();

    _simulationKernels = std::make_shared<_SimulationKernelsLauncher>();
    _dataAccessKernels = std::make_shared<_DataAccessKernelsLauncher>();
    _garbageCollectorKernels = std::make_shared<_GarbageCollectorKernelsLauncher>();
    _renderingKernels = std::make_shared<_RenderingKernelsLauncher>();
    _editKernels = std::make_shared<_EditKernelsLauncher>();
    _statisticsKernels = std::make_shared<_StatisticsKernelsLauncher>();

    CudaMemoryManager::getInstance().acquireMemory<uint64_t>(1, _cudaAccessTO->numCells);
    CudaMemoryManager::getInstance().acquireMemory<uint64_t>(1, _cudaAccessTO->numParticles);
    CudaMemoryManager::getInstance().acquireMemory<uint64_t>(1, _cudaAccessTO->numAuxiliaryData);

    //default array sizes for empty simulation (will be resized later if not sufficient)
    resizeArrays({100000, 100000, 100000});
}

_CudaSimulationFacade::~_CudaSimulationFacade()
{
    _cudaSimulationData->free();
    _cudaRenderingData->free();
    _simulationStatistics->free();
    _cudaSelectionResult->free();

    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->cells);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->particles);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->auxiliaryData);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->numCells);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->numParticles);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->numAuxiliaryData);

    log(Priority::Important, "close simulation");
}

void* _CudaSimulationFacade::registerImageResource(GLuint image)
{
    cudaGraphicsResource* cudaResource;

    CHECK_FOR_CUDA_ERROR(
        cudaGraphicsGLRegisterImage(&cudaResource, image, GL_TEXTURE_2D, cudaGraphicsMapFlagsReadOnly));

    return reinterpret_cast<void*>(cudaResource);
}

std::string _CudaSimulationFacade::getGpuName()
{
    return CudaInitializer::getGpuName();
}

void _CudaSimulationFacade::calcTimestep()
{
    checkAndProcessSimulationParameterChanges();

    Settings settings = [this] {
        std::lock_guard lock(_mutexForSimulationParameters);
        _simulationKernels->calcSimulationParametersForNextTimestep(_settings);
        CHECK_FOR_CUDA_ERROR(
            cudaMemcpyToSymbol(cudaSimulationParameters, &_settings.simulationParameters, sizeof(SimulationParameters), 0, cudaMemcpyHostToDevice));
        return _settings;
    }();

    _simulationKernels->calcTimestep(settings, getSimulationDataIntern(), *_simulationStatistics);
    syncAndCheck();

    automaticResizeArrays();

    std::lock_guard lock(_mutexForSimulationData);
    ++_cudaSimulationData->timestep;
}

void _CudaSimulationFacade::applyCataclysm(int power)
{
    for (int i = 0; i < power; ++i) {
        _editKernels->applyCataclysm(_settings.gpuSettings, getSimulationDataIntern());
        syncAndCheck();
        resizeArraysIfNecessary();
    }
}

void _CudaSimulationFacade::drawVectorGraphics(
    float2 const& rectUpperLeft,
    float2 const& rectLowerRight,
    void* cudaResource,
    int2 const& imageSize,
    double zoom)
{
    checkAndProcessSimulationParameterChanges();

    auto cudaResourceImpl = reinterpret_cast<cudaGraphicsResource*>(cudaResource);
    CHECK_FOR_CUDA_ERROR(cudaGraphicsMapResources(1, &cudaResourceImpl));

    cudaArray* mappedArray;
    CHECK_FOR_CUDA_ERROR(cudaGraphicsSubResourceGetMappedArray(&mappedArray, cudaResourceImpl, 0, 0));

    _cudaRenderingData->resizeImageIfNecessary(imageSize);

    _renderingKernels->drawImage(
        _settings.gpuSettings, rectUpperLeft, rectLowerRight, imageSize, static_cast<float>(zoom), getSimulationDataIntern(), *_cudaRenderingData);
    syncAndCheck();

    const size_t widthBytes = sizeof(uint64_t) * imageSize.x;
    CHECK_FOR_CUDA_ERROR(cudaMemcpy2DToArray(
        mappedArray,
        0,
        0,
        _cudaRenderingData->imageData,
        widthBytes,
        widthBytes,
        imageSize.y,
        cudaMemcpyDeviceToDevice));

    CHECK_FOR_CUDA_ERROR(cudaGraphicsUnmapResources(1, &cudaResourceImpl));
}

void _CudaSimulationFacade::getSimulationData(
    int2 const& rectUpperLeft,
    int2 const& rectLowerRight,
    DataTO const& dataTO)
{
    _dataAccessKernels->getData(_settings.gpuSettings, getSimulationDataIntern(), rectUpperLeft, rectLowerRight, *_cudaAccessTO);
    syncAndCheck();

    copyDataTOtoHost(dataTO);
}

void _CudaSimulationFacade::getSelectedSimulationData(bool includeClusters, DataTO const& dataTO)
{
    _dataAccessKernels->getSelectedData(_settings.gpuSettings, getSimulationDataIntern(), includeClusters, *_cudaAccessTO);
    syncAndCheck();

    copyDataTOtoHost(dataTO);
}

void _CudaSimulationFacade::getInspectedSimulationData(std::vector<uint64_t> entityIds, DataTO const& dataTO)
{
    InspectedEntityIds ids;
    if (entityIds.size() > Const::MaxInspectedObjects) {
        return;
    }
    for (int i = 0; i < entityIds.size(); ++i) {
        ids.values[i] = entityIds.at(i);
    }
    if (entityIds.size() < Const::MaxInspectedObjects) {
        ids.values[entityIds.size()] = 0;
    }
    _dataAccessKernels->getInspectedData(_settings.gpuSettings, getSimulationDataIntern(), ids, *_cudaAccessTO);
    syncAndCheck();
    copyDataTOtoHost(dataTO);
}

void _CudaSimulationFacade::getOverlayData(int2 const& rectUpperLeft, int2 const& rectLowerRight, DataTO const& dataTO)
{
    _dataAccessKernels->getOverlayData(_settings.gpuSettings, getSimulationDataIntern(), rectUpperLeft, rectLowerRight, *_cudaAccessTO);
    syncAndCheck();

    copyToHost(dataTO.numCells, _cudaAccessTO->numCells);
    copyToHost(dataTO.numParticles, _cudaAccessTO->numParticles);
    copyToHost(dataTO.cells, _cudaAccessTO->cells, *dataTO.numCells);
    copyToHost(dataTO.particles, _cudaAccessTO->particles, *dataTO.numParticles);
}

void _CudaSimulationFacade::addAndSelectSimulationData(DataTO const& dataTO)
{
    copyDataTOtoDevice(dataTO);
    _editKernels->removeSelection(_settings.gpuSettings, getSimulationDataIntern());
    _dataAccessKernels->addData(_settings.gpuSettings, getSimulationDataIntern(), *_cudaAccessTO, true, true);
    syncAndCheck();
}

void _CudaSimulationFacade::setSimulationData(DataTO const& dataTO)
{
    copyDataTOtoDevice(dataTO);
    _dataAccessKernels->clearData(_settings.gpuSettings, getSimulationDataIntern());
    _dataAccessKernels->addData(_settings.gpuSettings, getSimulationDataIntern(), *_cudaAccessTO, false, false);
    syncAndCheck();
}

void _CudaSimulationFacade::removeSelectedObjects(bool includeClusters)
{
    _editKernels->removeSelectedObjects(_settings.gpuSettings, getSimulationDataIntern(), includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::relaxSelectedObjects(bool includeClusters)
{
    _editKernels->relaxSelectedObjects(_settings.gpuSettings, getSimulationDataIntern(), includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::uniformVelocitiesForSelectedObjects(bool includeClusters)
{
    _editKernels->uniformVelocities(_settings.gpuSettings, getSimulationDataIntern(), includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::makeSticky(bool includeClusters)
{
    _editKernels->makeSticky(_settings.gpuSettings, getSimulationDataIntern(), includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::removeStickiness(bool includeClusters)
{
    _editKernels->removeStickiness(_settings.gpuSettings, getSimulationDataIntern(), includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::setBarrier(bool value, bool includeClusters)
{
    _editKernels->setBarrier(_settings.gpuSettings, getSimulationDataIntern(), value, includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::changeInspectedSimulationData(DataTO const& changeDataTO)
{
    copyDataTOtoDevice(changeDataTO);
    _editKernels->changeSimulationData(_settings.gpuSettings, getSimulationDataIntern(), *_cudaAccessTO);
    syncAndCheck();

    resizeArraysIfNecessary();
}

void _CudaSimulationFacade::applyForce(ApplyForceData const& applyData)
{
    _editKernels->applyForce(_settings.gpuSettings, getSimulationDataIntern(), applyData);
    syncAndCheck();
}

void _CudaSimulationFacade::switchSelection(PointSelectionData const& pointData)
{
    _editKernels->switchSelection(_settings.gpuSettings, getSimulationDataIntern(), pointData);
    syncAndCheck();
}

void _CudaSimulationFacade::swapSelection(PointSelectionData const& pointData)
{
    _editKernels->swapSelection(_settings.gpuSettings, getSimulationDataIntern(), pointData);
    syncAndCheck();
}

void _CudaSimulationFacade::setSelection(AreaSelectionData const& selectionData)
{
    _editKernels->setSelection(_settings.gpuSettings, getSimulationDataIntern(), selectionData);
}

 SelectionShallowData _CudaSimulationFacade::getSelectionShallowData()
{
    _editKernels->getSelectionShallowData(_settings.gpuSettings, getSimulationDataIntern(), *_cudaSelectionResult);
    syncAndCheck();
    return _cudaSelectionResult->getSelectionShallowData();
}

void _CudaSimulationFacade::shallowUpdateSelectedObjects(ShallowUpdateSelectionData const& shallowUpdateData)
{
    _editKernels->shallowUpdateSelectedObjects(_settings.gpuSettings, getSimulationDataIntern(), shallowUpdateData);
    syncAndCheck();
}

void _CudaSimulationFacade::removeSelection()
{
    _editKernels->removeSelection(_settings.gpuSettings, getSimulationDataIntern());
    syncAndCheck();
}

void _CudaSimulationFacade::updateSelection()
{
    _editKernels->updateSelection(_settings.gpuSettings, getSimulationDataIntern());
    syncAndCheck();
}

void _CudaSimulationFacade::colorSelectedObjects(unsigned char color, bool includeClusters)
{
    _editKernels->colorSelectedCells(_settings.gpuSettings, getSimulationDataIntern(), color, includeClusters);
    syncAndCheck();
}

void _CudaSimulationFacade::reconnectSelectedObjects()
{
    _editKernels->reconnect(_settings.gpuSettings, getSimulationDataIntern());
    syncAndCheck();
}

void _CudaSimulationFacade::setDetached(bool value)
{
    _editKernels->setDetached(_settings.gpuSettings, getSimulationDataIntern(), value);
    syncAndCheck();
}

void _CudaSimulationFacade::setGpuConstants(GpuSettings const& gpuConstants)
{
    _settings.gpuSettings = gpuConstants;

    CHECK_FOR_CUDA_ERROR(
        cudaMemcpyToSymbol(cudaThreadSettings, &gpuConstants, sizeof(GpuSettings), 0, cudaMemcpyHostToDevice));
}

SimulationParameters _CudaSimulationFacade::getSimulationParameters() const
{
    std::lock_guard lock(_mutexForSimulationParameters);
    return _newSimulationParameters ? *_newSimulationParameters : _settings.simulationParameters;
}

void _CudaSimulationFacade::setSimulationParameters(SimulationParameters const& parameters)
{
    std::lock_guard lock(_mutexForSimulationParameters);
    _newSimulationParameters = parameters;
}

auto _CudaSimulationFacade::getArraySizes() const -> ArraySizes
{
    return {
        _cudaSimulationData->objects.cells.getSize_host(),
        _cudaSimulationData->objects.particles.getSize_host(),
        _cudaSimulationData->objects.auxiliaryData.getSize_host()
    };
}

StatisticsData _CudaSimulationFacade::getStatistics()
{
    _statisticsKernels->updateStatistics(_settings.gpuSettings, getSimulationDataIntern(), *_simulationStatistics);
    syncAndCheck();
    
    return _simulationStatistics->getStatistics();
}

void _CudaSimulationFacade::resetTimeIntervalStatistics()
{
    _simulationStatistics->resetAccumulatedStatistics();
}

uint64_t _CudaSimulationFacade::getCurrentTimestep() const
{
    std::lock_guard lock(_mutexForSimulationData);
    return _cudaSimulationData->timestep;
}

void _CudaSimulationFacade::setCurrentTimestep(uint64_t timestep)
{
    std::lock_guard lock(_mutexForSimulationData);
    _cudaSimulationData->timestep = timestep;
}

void _CudaSimulationFacade::clear()
{
    _dataAccessKernels->clearData(_settings.gpuSettings, getSimulationDataIntern());
    syncAndCheck();
}

void _CudaSimulationFacade::resizeArraysIfNecessary(ArraySizes const& additionals)
{
    if (_cudaSimulationData->shouldResize(additionals)) {
        resizeArrays(additionals);
    }
}

void _CudaSimulationFacade::testOnly_mutate(uint64_t cellId, MutationType mutationType)
{
    _testKernels->testOnly_mutate(_settings.gpuSettings, getSimulationDataIntern(), cellId, mutationType);
    syncAndCheck();

    resizeArraysIfNecessary();
}

void _CudaSimulationFacade::syncAndCheck()
{
    cudaDeviceSynchronize();
    CHECK_FOR_CUDA_ERROR(cudaGetLastError());
}

void _CudaSimulationFacade::copyDataTOtoDevice(DataTO const& dataTO)
{
    copyToDevice(_cudaAccessTO->numCells, dataTO.numCells);
    copyToDevice(_cudaAccessTO->numParticles, dataTO.numParticles);
    copyToDevice(_cudaAccessTO->numAuxiliaryData, dataTO.numAuxiliaryData);

    copyToDevice(_cudaAccessTO->cells, dataTO.cells, *dataTO.numCells);
    copyToDevice(_cudaAccessTO->particles, dataTO.particles, *dataTO.numParticles);
    copyToDevice(_cudaAccessTO->auxiliaryData, dataTO.auxiliaryData, *dataTO.numAuxiliaryData);
}

void _CudaSimulationFacade::copyDataTOtoHost(DataTO const& dataTO)
{
    copyToHost(dataTO.numCells, _cudaAccessTO->numCells);
    copyToHost(dataTO.numParticles, _cudaAccessTO->numParticles);
    copyToHost(dataTO.numAuxiliaryData, _cudaAccessTO->numAuxiliaryData);

    copyToHost(dataTO.cells, _cudaAccessTO->cells, *dataTO.numCells);
    copyToHost(dataTO.particles, _cudaAccessTO->particles, *dataTO.numParticles);
    copyToHost(dataTO.auxiliaryData, _cudaAccessTO->auxiliaryData, *dataTO.numAuxiliaryData);
}

void _CudaSimulationFacade::automaticResizeArrays()
{
    uint64_t timestep;
    {
        std::lock_guard lock(_mutexForSimulationData);
        timestep = _cudaSimulationData->timestep;
    }
    //make check after every 10th time step
    if (timestep % 10 == 0) {
        resizeArraysIfNecessary();
    }
}

void _CudaSimulationFacade::resizeArrays(ArraySizes const& additionals)
{
    log(Priority::Important, "resize arrays");

    _cudaSimulationData->resizeTargetObjects(additionals);
    if (!_cudaSimulationData->isEmpty()) {
        _garbageCollectorKernels->copyArrays(_settings.gpuSettings, getSimulationDataIntern());
        syncAndCheck();

        _cudaSimulationData->resizeObjects();

        _garbageCollectorKernels->swapArrays(_settings.gpuSettings, getSimulationDataIntern());
        syncAndCheck();
    } else {
        _cudaSimulationData->resizeObjects();
    }

    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->cells);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->particles);
    CudaMemoryManager::getInstance().freeMemory(_cudaAccessTO->auxiliaryData);

    auto cellArraySize = _cudaSimulationData->objects.cells.getSize_host();
    CudaMemoryManager::getInstance().acquireMemory<CellTO>(cellArraySize, _cudaAccessTO->cells);
    auto particleArraySize = _cudaSimulationData->objects.particles.getSize_host();
    CudaMemoryManager::getInstance().acquireMemory<ParticleTO>(particleArraySize, _cudaAccessTO->particles);
    auto auxiliaryDataSize = _cudaSimulationData->objects.auxiliaryData.getSize_host();
    CudaMemoryManager::getInstance().acquireMemory<uint8_t>(auxiliaryDataSize, _cudaAccessTO->auxiliaryData);

    CHECK_FOR_CUDA_ERROR(cudaGetLastError());

    log(Priority::Unimportant, "cell array size: " + std::to_string(cellArraySize));
    log(Priority::Unimportant, "particle array size: " + std::to_string(particleArraySize));
    log(Priority::Unimportant, "auxiliary data size: " + std::to_string(auxiliaryDataSize));

    auto const memorySizeAfter = CudaMemoryManager::getInstance().getSizeOfAcquiredMemory();
    log(Priority::Important, std::to_string(memorySizeAfter / (1024 * 1024)) + " MB GPU memory acquired");
}

void _CudaSimulationFacade::checkAndProcessSimulationParameterChanges()
{
    std::lock_guard lock(_mutexForSimulationParameters);
    if (_newSimulationParameters) {
        _settings.simulationParameters = *_newSimulationParameters;
        CHECK_FOR_CUDA_ERROR(cudaMemcpyToSymbol(cudaSimulationParameters, &*_newSimulationParameters, sizeof(SimulationParameters), 0, cudaMemcpyHostToDevice));
        _newSimulationParameters.reset();

        if (_cudaSimulationData) {
            _simulationKernels->prepareForSimulationParametersChanges(_settings, getSimulationDataIntern());
        }
    }
}

SimulationData _CudaSimulationFacade::getSimulationDataIntern() const
{
    std::lock_guard lock(_mutexForSimulationData);
    return *_cudaSimulationData;
}
