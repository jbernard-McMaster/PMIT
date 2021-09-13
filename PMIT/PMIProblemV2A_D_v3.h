#pragma once
#include "PMIProblemV2A_Rule.h"

ref class PMIProblemV2A_D_v3 :
	public PMIProblemV2A_Rule
{
public:
	PMIProblemV2A_D_v3(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_D_v3();

	virtual bool AnalyzeSolution(PMISolution^ s) override;

	virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;

	virtual UInt64 ComputeSolutionSpaceSize() override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
};

