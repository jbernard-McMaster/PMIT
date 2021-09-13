#pragma once
#include "PMIProblem_Direct.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class PMIProblem_Directv2 :
	PMIProblem_Direct
{
public:
	PMIProblem_Directv2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_Directv2();

	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Direct(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;

	virtual bool SolveProblemSet() override;
	virtual bool SolveDirectSearch() override;
};

