#pragma once
#include "PMIProblemV2A_PoL_v1.h"

public ref class PMIProblemV2A_PoL_v1b :
	public PMIProblemV2A_PoL_v1
{
public:
	PMIProblemV2A_PoL_v1b(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_PoL_v1b();
};

