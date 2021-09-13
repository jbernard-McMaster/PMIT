#pragma once
#include "ProductionRule.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class TabuItem
{
public:
	TabuItem(Int32 Index);
	TabuItem(Int32 Index, array<Int32, 2>^ Rules);
	TabuItem(Int32 Index, array<Int32>^ Lengths);
	TabuItem(Int32 Index, List<ProductionRule^>^ Rules);
	virtual ~TabuItem();

	Int32 index;
	Int32 indexSecondary;
	array<Int32, 2>^ rulesParikh;
	List<ProductionRule^>^ rules;
	array<Int32>^ ruleLengths;
	array<Int32>^ solution;

	static const Int32 ModulesOnlyIndex = -1;
};

public ref class TabuSolution
{
public:
	TabuSolution(List<Int32>^ Solution);
	virtual ~TabuSolution();

	List<Int32>^ values;

	bool IsTabu(List<Int32>^ S);
};
