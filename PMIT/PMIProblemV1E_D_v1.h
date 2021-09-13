#pragma once
#include "PMIProblemV2_V1Emulator.h"

public ref class PMIProblemV1E_D_v1 :
	public PMIProblemV2_V1Emulator
{
public:
	PMIProblemV1E_D_v1(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV1E_D_v1();

	virtual bool AnalyzeSolution(PMISolution^ s) override;

	virtual float AssessRuleLengths(array<Int32>^ Solution, bool ForceRuleLength) override;
	virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;

	virtual void ComputeFragmentsFromPartialSolution() override;
	virtual UInt64 ComputeSolutionSpaceSize() override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

};

