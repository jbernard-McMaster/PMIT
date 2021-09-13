#pragma once
#include "PMIProblemV2A.h"

public ref class PMIProblemV2A_Parikh :
	public PMIProblemV2A
{
public:
	PMIProblemV2A_Parikh(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_Parikh();

	virtual bool AnalyzeSolution(PMISolution^ s) override;

	virtual float Assess(array<Int32>^ Solution) override;
	//virtual bool Assess_BranchingSymbol() override;
	virtual array<Int32, 2>^ Assess_CreateParikhRulesStep(array<Int32>^ Solution);

	virtual UInt64 ComputeSolutionSpaceSize() override;
};

