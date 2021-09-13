#pragma once
#include "ProductionRule.h"
#include "EvidenceFactory.h"

using namespace System;
using namespace System::Collections::Generic;

public interface class IPMIFitnessFunction
{
	float Assess(array<Int32>^ A, array<Int32,2>^ R, List<Word^>^ E, Int32 Start, Int32 End, Int32 NumSymbols, Int32 NumModules);
	float Assess(List<ProductionRule^>^ Rules);
};

