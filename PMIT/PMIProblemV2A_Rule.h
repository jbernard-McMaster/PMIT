#pragma once
#include "PMIProblemV2A.h"
public ref class PMIProblemV2A_Rule :
	public PMIProblemV2A
{
public:
	PMIProblemV2A_Rule(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_Rule();

	virtual void AddTabuItem(TabuItem^ T) override;

	virtual bool AnalyzeSolution(PMISolution^ s) override;

	virtual float Assess(array<Int32>^ Solution) override;
	virtual array<Int32>^ Assess_CreateRuleLengths(array<Int32>^ Solution);
	//virtual bool Assess_BranchingSymbol() override;
	//virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution);
	//virtual float AssessProductionRules(List<ProductionRule^>^ Rules) override;

	virtual UInt64 ComputeSolutionSpaceSize() override;
};

