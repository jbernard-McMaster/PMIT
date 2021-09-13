#include "stdafx.h"
#include "PMIFitnessFunction_Parikh.h"


PMIFitnessFunction_Parikh::PMIFitnessFunction_Parikh()
{
}


PMIFitnessFunction_Parikh::~PMIFitnessFunction_Parikh()
{
}

float PMIFitnessFunction_Parikh::Assess(array<Int32>^ A, array<Int32, 2>^ R, List<Word^>^ E, Int32 Start, Int32 End, Int32 NumSymbols, Int32 NumModules)
{
	Int32 generations = End - Start;
	float result = generations;
	
	array<Int32, 2>^ solution = EvidenceFactory::Process(A, R, generations, NumSymbols, NumModules);

	Int32 error = 0;
	Int32 errorGen = 0;
	Int32 total = 0;

	for (size_t iGen = 1; iGen <= solution->GetUpperBound(0); iGen++)
	{
		for (size_t iSymbol = 0; iSymbol <= solution->GetUpperBound(1); iSymbol++)
		{
			errorGen += Math::Abs(solution[iGen, iSymbol] - E[Start+iGen]->parikhVector[iSymbol]);
			total += E[Start+iGen]->parikhVector[iSymbol];
		}

		error += errorGen;

		// if no errors in a generation deduct one from the fitness
		if (errorGen == 0)
		{
			result--;
		}
	}

	result += (float)error / total;

	if (result == 0.0)
	{
		Console::Write("");
	}

	return result;
}

float PMIFitnessFunction_Parikh::Assess(List<ProductionRule^>^ Rules)
{
	return 0.0;
}
