#pragma once
#include "PMIProblemV2A.h"

public ref class PMIProblemV2_V1Emulator :
	public PMIProblemV2A
{
public:
	PMIProblemV2_V1Emulator(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2_V1Emulator();
};

