#include "stdafx.h"
#include "PMIProblemV2_V1Emulator.h"


PMIProblemV2_V1Emulator::PMIProblemV2_V1Emulator(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A(Model, ErrorRate, FileName)
{
}

PMIProblemV2_V1Emulator::~PMIProblemV2_V1Emulator()
{
}
