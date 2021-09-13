#pragma once

#include "MasterAnalysisObject.h"

public ref class FactCounter
{
public:
	FactCounter();
	virtual ~FactCounter();

	List<FactCount^>^ counter;

	void Add(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F);
	Int32 Count(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F);
};

