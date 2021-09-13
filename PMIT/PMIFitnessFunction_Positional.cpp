#include "stdafx.h"
#include "PMIFitnessFunction_Positional.h"


PMIFitnessFunction_Positional::PMIFitnessFunction_Positional()
{
}


PMIFitnessFunction_Positional::~PMIFitnessFunction_Positional()
{
}

float PMIFitnessFunction_Positional::Assess(array<Int32>^ A, array<Int32, 2>^ R, List<Word^>^ E, Int32 Start, Int32 End, Int32 NumSymbols, Int32 NumModules)
{
	Int32 generations = End - Start;
	array<Int32, 2>^ solution = EvidenceFactory::Process(A, R, generations, NumSymbols, NumModules);

	return 0.0;
}

float PMIFitnessFunction_Positional::Assess(List<ProductionRule^>^ Rules)
{
	return 0.0;
}
