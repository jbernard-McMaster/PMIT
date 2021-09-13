#pragma once
#include "PMIFitnessFunction.h"

public ref class PMIFitnessFunction_Positional:
	public IPMIFitnessFunction
{
public:
	PMIFitnessFunction_Positional();
	virtual ~PMIFitnessFunction_Positional();

	virtual float Assess(array<Int32>^ A, array<Int32, 2>^ R, List<Word^>^ E, Int32 Start, Int32 End, Int32 NumSymbols, Int32 NumModules);
	virtual float Assess(List<ProductionRule^>^ Rules);
};

