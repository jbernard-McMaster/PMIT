#pragma once
#include "PMIProblemV2A_Rule.h"

public ref class PMIProblemV2A_PoL_v1 :
	public PMIProblemV2A_Rule
{
public:
	PMIProblemV2A_PoL_v1(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_PoL_v1();

	//virtual float Assess(array<Int32>^ Solution) override;
	virtual array<Int32>^ Assess_CreateRuleLengths(array<Int32>^ Solution) override;

	virtual UInt64 ComputeSolutionSpaceSize() override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual BruteForceSearchConfiguration^ CreateSearchSpace() override;

};

