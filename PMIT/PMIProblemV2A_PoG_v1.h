#pragma once
#include "PMIProblemV2A_Parikh.h"

public ref class PMIProblemV2A_PoG_v1 :
	public PMIProblemV2A_Parikh
{
public:
	PMIProblemV2A_PoG_v1(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_PoG_v1();

	array<Int32, 2>^ ruleAndSymbol2Gene;

	virtual array<Int32, 2>^ Assess_CreateParikhRulesStep(array<Int32>^ Solution) override;
	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
};

