#pragma once
#include "PMIProblem_Lengthv1.h"

ref class PMIProblem_LengthV2 :
	public PMIProblem_LengthV1
{
public:
	PMIProblem_LengthV2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_LengthV2();

	virtual float Assess_Length(array<Int32>^ Solution) override;
	virtual float Assess_Length_ModulesOnly(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;

	virtual bool SolveProblemSet() override;
	virtual bool SolveLengthSearch() override;
	virtual bool SolveLengthSearch_ModulesOnly() override;
};

