#pragma once
#include "PMIProblemV2A_Rule.h"

public ref class PMIProblemV2A_ML :
	public PMIProblemV2A_Rule
{
public:
	PMIProblemV2A_ML(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_ML();

	Int32 equationIndex;
	array<Int32>^ solutionInProgress;
	array<float, 2>^ solutionMatrix;

	virtual void AddTabuItem_Equation(TabuItem^ T);

	virtual float Assess(array<Int32>^ Solution) override;
	virtual float AssessLinearEquation(array<Int32>^ Solution);

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

	virtual bool IsTabu_Equation(array<Int32>^ Solution);

	virtual bool ModifySolutionMatrix(PMISolution^ s);

	//virtual SolutionType^ SolveLinearAlgebra();
	virtual bool SolveProblem() override;

};

