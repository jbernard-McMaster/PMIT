#pragma once

#include <math.h>

#include "PMIProblem.h"

#define _PMI_TEMPLATE_PROBLEM_VERBOSE_ 0

public ref class PMITemplateProblem : public PMIProblem
{
public:
	PMITemplateProblem(PMIProblem^ Problem);
	virtual ~PMITemplateProblem();

	virtual float Assess(array<Int32>^ Solution) override;
	//virtual array<Int32>^ Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution);
	virtual array<Int32, 2>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;
	//virtual void CreateParikhVectors(Int32 IndexStart, Int32 IndexEnd);
	virtual GenomeConfiguration<Int32>^ CreateGenome(Int32 IndexStart, Int32 IndexEnd);
};

