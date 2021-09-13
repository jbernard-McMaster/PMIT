#pragma once
#include "PMIFitnessFunction.h"

public ref class PMIFitnessFunction_Parikh :
	public IPMIFitnessFunction
{
public:
	PMIFitnessFunction_Parikh();
	virtual ~PMIFitnessFunction_Parikh();

	virtual float Assess(array<Int32>^ A, array<Int32, 2>^ R, List<Word^>^ E, Int32 Start, Int32 End, Int32 NumSymbols, Int32 NumModules);
	virtual float Assess(List<ProductionRule^>^ Rules);
};

