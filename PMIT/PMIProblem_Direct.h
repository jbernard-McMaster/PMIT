#pragma once
#include "PMIProblem_Search.h"

// Encoding scheme:
// Each represent a specific symbol in a specific rule.
// Gene values are 1 to S + 1 (for blank)

public ref class PMIProblem_Direct :
	public PMIProblem_Search
{
public:
	PMIProblem_Direct(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_Direct();

	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Direct(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;
	
	virtual bool SolveProblemSet() override;
	virtual bool SolveDirectSearch();
};

