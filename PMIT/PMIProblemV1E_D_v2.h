#pragma once
#include "PMIProblemV2_V1Emulator.h"

public ref class PMIProblemV1E_D_v2 :
	public PMIProblemV2_V1Emulator
{
public:
	PMIProblemV1E_D_v2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV1E_D_v2();
};

