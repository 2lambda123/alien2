#ifndef FACTORYFACADE_H
#define FACTORYFACADE_H

#include <QtGlobal>
#include <QVector2D>

#include "model/Entities/CellTO.h"
#include "model/Features/CellFeatureEnums.h"
#include "model/Entities/Descriptions.h"

#include "Definitions.h"

class BuilderFacade
{
public:
	virtual ~BuilderFacade() = default;

	virtual SimulationContextApi* buildSimulationContext(int maxRunngingThreads, IntVector2D gridSize, SpaceMetric* metric
		, SymbolTable* symbolTable, SimulationParameters* parameters) const = 0;
	virtual SimulationAccess* buildSimulationAccess(SimulationContextApi* context) const = 0;
	virtual SimulationController* buildSimulationController(SimulationContextApi* context) const = 0;
	virtual SpaceMetric* buildSpaceMetric(IntVector2D universeSize) const = 0;
	virtual SymbolTable* buildDefaultSymbolTable() const = 0;
	virtual SimulationParameters* buildDefaultSimulationParameters() const = 0;
};

#endif // FACTORYFACADE_H
