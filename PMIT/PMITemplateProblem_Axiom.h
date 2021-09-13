#pragma once

#include <math.h>

#include "PMITemplateProblem.h"

public ref class PMITemplateProblem_Axiom : public PMITemplateProblem
{
public:
	PMITemplateProblem_Axiom(PMIProblem^ Problem);
	virtual ~PMITemplateProblem_Axiom();

	virtual float Assess(array<Int32>^ Solution) override;
	virtual GenomeConfiguration<Int32>^ CreateGenome(Int32 IndexStart, Int32 IndexEnd) override;

	array<Int32, 2>^ rules;

	List<array<Int32>^>^ tabu;

protected:
private:
};


