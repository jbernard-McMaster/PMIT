#pragma once

#include <math.h>

#include "PMITemplateProblem.h"

public ref class PMITemplateProblem_Modules : public PMITemplateProblem
{
public:
	PMITemplateProblem_Modules(PMIProblem^ Problem);
	virtual ~PMITemplateProblem_Modules();

	virtual float Assess(array<Int32>^ Solution) override;
	virtual array<Int32, 2>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;
	virtual GenomeConfiguration<Int32>^ CreateGenome(Int32 IndexStart, Int32 IndexEnd) override;

	List<array<Int32>^>^ tabu;

protected:
private:
};
