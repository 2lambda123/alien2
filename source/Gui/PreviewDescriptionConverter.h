#pragma once

#include "EngineInterface/GenomeDescriptions.h"
#include "EngineInterface/SimulationParameters.h"
#include "PreviewDescriptions.h"


class PreviewDescriptionConverter
{
public:
    static PreviewDescription convert(GenomeDescription const& genome, SimulationParameters const& parameters);
};

