#pragma once
#include "S0LMProblemV3.h"

ref class HD0LProblemV3 :
	public S0LMProblemV3
{
public:
	HD0LProblemV3(ModelV3^ M, ProblemType T);
	virtual ~HD0LProblemV3();

	virtual void Initialize() override;

	virtual void Analyze() override;

	virtual double Assess_Dbl_Old(array<Int32>^ RuleLengths) override;
	virtual double Assess_Dbl_PoL_Old(array<Int32>^ RuleLengths) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome_GreedyBF() override;

	virtual void Decode2(array<Int32>^ RuleLengths) override;

};

