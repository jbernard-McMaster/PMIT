#pragma once
#include "PMIProblemV2A_PoG_v1.h"

ref class PMIProblemV2A_PoG_v2 :
	public PMIProblemV2A_PoG_v1
{
public:
	PMIProblemV2A_PoG_v2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_PoG_v2();

	array<Int32, 2>^ Assess_CreateParikhRulesStep(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
};

