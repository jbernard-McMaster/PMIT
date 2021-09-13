#pragma once
#include "PMIProblemV2A_Rule.h"

public ref class PMIProblemV2A_L_v2 :
	public PMIProblemV2A_Rule
{
public:
	PMIProblemV2A_L_v2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_L_v2();

	//virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;
	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual BruteForceSearchConfiguration^ CreateSearchSpace() override;
};

