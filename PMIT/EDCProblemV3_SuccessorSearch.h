#pragma once
#include "EDCProblemV3.h"

ref class EDCProblemV3;

public ref class EDCProblemV3_SuccessorSearch :
	public EDCProblemV3
{
public:
	EDCProblemV3_SuccessorSearch(ModelV3^ M, ProblemType T, array<List<SuccessorCount^>^>^ Successors);
	virtual ~EDCProblemV3_SuccessorSearch();

	void Initialize() override;

	virtual float Assess(array<Int32>^ RuleLengths) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual BruteForceSearchConfiguration^ CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig) override;

	virtual List<ProductionRuleV3^>^ SolveForRules() override;
	virtual SolutionType^ SolveProblemBF() override;
};

