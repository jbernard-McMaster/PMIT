#pragma once
#include "LSIProblemV3.h"

public ref class LSIProblemV3_Wolfram :
	public LSIProblemV3
{
public:
	LSIProblemV3_Wolfram(ModelV3^ M, ProblemType T);
	virtual ~LSIProblemV3_Wolfram();

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual BruteForceSearchConfiguration^ CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig);

	virtual void Solve() override;
	virtual SolutionType^ SolveProblemBF();

	Int32 tracker;
};

