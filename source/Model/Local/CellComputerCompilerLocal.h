﻿#pragma once

#include "Model/Api/CellComputerCompiler.h"
#include "Model/Api/Definitions.h"

class CellComputerCompilerLocal
	: public CellComputerCompiler
{
	Q_OBJECT
public:
	CellComputerCompilerLocal(QObject * parent = nullptr);
	virtual ~CellComputerCompilerLocal() = default;

	virtual void init(SymbolTable* symbols, SimulationParameters* parameters) = 0;
};
