﻿#include "SimulationData.cuh"

#include "GarbageCollectorKernels.cuh"

void SimulationData::init(int2 const& worldSize_, uint64_t timestep_)
{
    worldSize = worldSize_;
    timestep = timestep_;

    objects.init();
    tempObjects.init();
    preprocessedCellFunctionData.init(worldSize);
    cellMap.init(worldSize);
    particleMap.init(worldSize);

    CudaMemoryManager::getInstance().acquireMemory<double>(1, residualEnergy);
    CHECK_FOR_CUDA_ERROR(cudaMemset(residualEnergy, 0, sizeof(double)));
 
    processMemory.init();
    numberGen1.init(40312357);   //some array size for random numbers (~ 40 MB)
    numberGen2.init(1536941);  //some array size for random numbers (~ 1.5 MB)

    structuralOperations.init();
    for (int i = 0; i < CellFunction_WithoutNone_Count; ++i) {
        cellFunctionOperations[i].init();
    }
}

__device__ void SimulationData::prepareForNextTimestep()
{
    cellMap.reset();
    particleMap.reset();
    processMemory.reset();

    auto maxStructureOperations = 1000 + objects.cellPointers.getNumEntries() / 2; //heuristic
    auto maxCellFunctionOperations = objects.cellPointers.getNumEntries();  //heuristic

    structuralOperations.setMemory(processMemory.getTypedSubArray<StructuralOperation>(maxStructureOperations), maxStructureOperations);

    for (int i = 0; i < CellFunction_WithoutNone_Count; ++i) {
        cellFunctionOperations[i].setMemory(processMemory.getTypedSubArray<CellFunctionOperation>(maxCellFunctionOperations), maxCellFunctionOperations);
    }

    objects.saveNumEntries();
}

bool SimulationData::shouldResize(ArraySizes const& additionals)
{
    auto cellAndParticleArraySizeInc = std::max(additionals.cellArraySize, additionals.particleArraySize);
    return objects.cells.shouldResize_host(cellAndParticleArraySizeInc)
        || objects.cellPointers.shouldResize_host(cellAndParticleArraySizeInc * 10)
        || objects.particles.shouldResize_host(cellAndParticleArraySizeInc)
        || objects.particlePointers.shouldResize_host(cellAndParticleArraySizeInc * 10)
        || objects.auxiliaryData.shouldResize_host(additionals.auxiliaryDataSize);
}

void SimulationData::resizeTargetObjects(ArraySizes const& additionals)
{
    auto cellAndParticleArraySizeInc = std::max(additionals.cellArraySize, additionals.particleArraySize);

    resizeTargetIntern(objects.cells, tempObjects.cells, cellAndParticleArraySizeInc);
    resizeTargetIntern(objects.cellPointers, tempObjects.cellPointers, cellAndParticleArraySizeInc * 10);
    resizeTargetIntern(objects.particles, tempObjects.particles, cellAndParticleArraySizeInc);
    resizeTargetIntern(objects.particlePointers, tempObjects.particlePointers, cellAndParticleArraySizeInc * 10);
    resizeTargetIntern(objects.auxiliaryData, tempObjects.auxiliaryData, additionals.auxiliaryDataSize);
}

void SimulationData::resizeObjects()
{
    objects.cells.resize(tempObjects.cells.getSize_host());
    objects.cellPointers.resize(tempObjects.cellPointers.getSize_host());
    objects.particles.resize(tempObjects.particles.getSize_host());
    objects.particlePointers.resize(tempObjects.particlePointers.getSize_host());
    objects.auxiliaryData.resize(tempObjects.auxiliaryData.getSize_host());

    auto cellArraySize = objects.cells.getSize_host();
    cellMap.resize(cellArraySize);
    auto particleArraySize = objects.particles.getSize_host();
    particleMap.resize(particleArraySize);

    int upperBoundDynamicMemory = (sizeof(StructuralOperation) + sizeof(CellFunctionOperation) * CellFunction_Count + 200) * (cellArraySize + 1000); //heuristic
    processMemory.resize(upperBoundDynamicMemory);
}

bool SimulationData::isEmpty()
{
    return 0 == objects.cells.getNumEntries_host() && 0 == objects.particles.getNumEntries_host();
}

void SimulationData::free()
{
    objects.free();
    tempObjects.free();
    preprocessedCellFunctionData.free();
    cellMap.free();
    particleMap.free();
    numberGen1.free();
    numberGen2.free();
    processMemory.free();
    CudaMemoryManager::getInstance().freeMemory(residualEnergy);

    structuralOperations.free();
    for (int i = 0; i < CellFunction_WithoutNone_Count; ++i) {
        cellFunctionOperations[i].free();
    }
}

template <typename Entity>
void SimulationData::resizeTargetIntern(Array<Entity> const& sourceArray, Array<Entity>& targetArray, int additionalEntities)
{
    if (sourceArray.shouldResize_host(additionalEntities)) {
        auto newSize = (sourceArray.getSize_host() + additionalEntities) * 3;
        targetArray.resize(newSize);
    }
}
