#pragma once
#include "S0LProblemV3.h"

public ref class D2LProblemV3 :
	public S0LProblemV3
{
public:
	D2LProblemV3(ModelV3^ M, ProblemType T);
	virtual ~D2LProblemV3();

	virtual void Initialize() override;

	virtual void Analyze() override;

	virtual float Assess(array<Int32>^ RuleLengths) override;
	virtual float Assess_L(array<Int32>^ RuleLengths) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome_GreedyBF() override;

	virtual void Decode(array<Int32>^ RuleLengths) override;
	virtual void Decode_L(array<Int32>^ RuleLengths) override;

	virtual void Solve() override;
	virtual SolutionType^ SolveProblemGreedyBF() override;
	virtual SolutionType^ SolveProblemGreedyGA() override;

	float fitnessBase;
};

