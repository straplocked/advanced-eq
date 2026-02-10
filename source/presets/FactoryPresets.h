#pragma once

#include "PresetData.h"
#include <vector>

namespace FactoryPresets
{
    std::vector<Preset> getInstrumentPresets();
    std::vector<Preset> getBusPresets();
    std::vector<Preset> getMasteringPresets();
    std::vector<Preset> getAll();
}
