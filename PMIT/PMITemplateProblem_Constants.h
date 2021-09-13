#pragma once

#include <math.h>

#include "PMITemplateProblem.h"

public ref class PMITemplateProblem_Constants : public PMITemplateProblem
{
public:
	PMITemplateProblem_Constants(PMIProblem^ Problem);
	virtual ~PMITemplateProblem_Constants();

	virtual float Assess(array<Int32>^ Solution) override;
	virtual array<Int32>^ Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution) override;
	virtual array<Int32, 2>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;
	virtual GenomeConfiguration<Int32>^ CreateGenome(Int32 IndexStart, Int32 IndexEnd) override;

	List<array<Int32>^>^ tabu;
	array<Int32, 2>^ rulesModules;

	Int32 constantIndex = 0;

	bool solveForConstants;

protected:
private:
};

