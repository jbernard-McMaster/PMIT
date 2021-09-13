#include "stdafx.h"
#include "FactCounter.h"


FactCounter::FactCounter()
{
	this->counter = gcnew List<FactCount^>;
}


FactCounter::~FactCounter()
{
}

void FactCounter::Add(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
{
	// Check to see if we already have this fact
	bool found = false;
	Int32 iFact = 0;

	while ((!found) && (iFact < this->counter->Count))
	{
		if (F->ID == this->counter[iFact]->fact->ID)
		{
			found = true;
		}
		else
		{
			iFact++;
		}
	}

	if (!found)
	{
		FactCount^ fCount = gcnew FactCount();
		fCount->iGen = iGen;
		fCount->iTable = iTable;
		fCount->iSymbol = iSymbol;
		fCount->iLeft = iLeft;
		fCount->iRight = iRight;
		fCount->fact = F;
		fCount->count = 1;
		this->counter->Add(fCount);
	}
	else
	{
		this->counter[iFact]->count++;
	}
}

Int32 FactCounter::Count(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
{
	// Check to see if we already have this fact
	bool found = false;
	Int32 iFact = 0;

	while ((!found) && (iFact < this->counter->Count))
	{
		if (F->ID == this->counter[iFact]->fact->ID)
		{
			found = true;
		}
		else
		{
			iFact++;
		}
	}

	if (!found)
	{
		return 0;
	}
	else
	{
		return this->counter[iFact]->count;
	}
}
