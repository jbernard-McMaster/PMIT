#pragma once
#include "PMIProblem_Search.h"

public ref class PMIProblem_PoG :
	public PMIProblem_Search
{
public:
	PMIProblem_PoG(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_PoG();

	//virtual array<Int32, 2>^ Assess_CreateParikhRulesStep_Constants(array<Int32>^ Solution) override;
	//virtual array<Int32, 2>^ Assess_CreateParikhRulesStep_Modules(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
};

