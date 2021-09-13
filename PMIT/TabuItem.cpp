#include "stdafx.h"
#include "TabuItem.h"

TabuItem::TabuItem(Int32 RuleIndex)
{
	this->index = RuleIndex;
}

TabuItem::TabuItem(Int32 RuleIndex, array<Int32, 2>^ Rules)
{
	this->index= RuleIndex;
	this->rulesParikh = Rules;
}

TabuItem::TabuItem(Int32 RuleIndex, array<Int32>^ Lengths)
{
	this->index = RuleIndex;
	this->ruleLengths = Lengths;
}

TabuItem::TabuItem(Int32 RuleIndex, List<ProductionRule^>^ Rules)
{
	this->index = RuleIndex;
	this->rules = Rules;
}

TabuItem::~TabuItem()
{
}

TabuSolution::TabuSolution(List<Int32>^ Solution)
{
	this->values = Solution;
}

TabuSolution::~TabuSolution()
{
}

bool TabuSolution::IsTabu(List<Int32>^ S)
{
	bool result = true;
	Int32 idx = 0;

	while ((result) && (idx < this->values->Count))
	{
		result = result && (this->values[idx] == S[idx]);
		idx++;
	}

	return result;
}
