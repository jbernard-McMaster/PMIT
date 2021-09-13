#pragma once
#include "PMIProblemV2.h"

// Level 1 Problems - D0L System with only complete sub-problems
// Algae - Solved
// Cantor Dust - Solved
// Koch Curve - Solved
// Dragon Curve - Solved
// Fractal Plant - Solved
// Pythogoras Tree - Solved
// Sierpenski Triangle 1 - Solved
// Sierpenski Triangle 2 - Solved
// Gosper Curve 1 - Solved
// Gosper Curve 2 - Solved
// E-Curve - Solved
// Peano - Solved
// Hilbert - Out of memory

// Level 2 Problems - D0L System with incomplete sub-problems
// Apple Twig - Fails
// Bush-like - Fails
// Aphanocladia stichidiosa - Fails
// Dipterosuphonia - Fails
// Ditria Zonaricola - Fails
// Heroposiphonia Zonaricola - Fails


public ref class PMIProblem_LengthV1 :
	public PMIProblemV2
{
public:
	PMIProblem_LengthV1(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_LengthV1();

	virtual float Assess_Length(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;

	virtual bool SolveProblemSet() override;
	virtual bool SolveLengthSearch();

protected:

private:

};

