#include "stdafx.h"
#include "ProductionRuleConstraint.h"


ProductionRuleConstraint_Time::ProductionRuleConstraint_Time(Int32 Start, Int32 End)
{
	this->start = Start;
	this->end = End;
}

ProductionRuleConstraint_Time::~ProductionRuleConstraint_Time()
{
}

bool ProductionRuleConstraint_Time::Process(Int32 T, Symbol^ L, Symbol^ R)
{
	if ((T >= this->start) && (T < this->end))
	{
		return true;
	}
	else
	{
		return false;
	}
}

ProductionRuleConstraint_TimeReoccuring::ProductionRuleConstraint_TimeReoccuring(Int32 Start, Int32 Increment)
{
	this->start = Start;
	this->increment = Increment;
}


ProductionRuleConstraint_TimeReoccuring::~ProductionRuleConstraint_TimeReoccuring()
{
}

bool ProductionRuleConstraint_TimeReoccuring::Process(Int32 T, Symbol^ L, Symbol^ R)
{
	Int32 _T = T - this->start;

	if (_T % this->increment == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* CONTEXT LEFT */

ProductionRuleConstraint_ContextLeft::ProductionRuleConstraint_ContextLeft(Symbol^ L)
{
	this->s = L;
}

ProductionRuleConstraint_ContextLeft::~ProductionRuleConstraint_ContextLeft()
{
}

bool ProductionRuleConstraint_ContextLeft::Process(Int32 T, Symbol^ L, Symbol^ R)
{
	if (L == this->s)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/* CONTEXT RIGHT */

ProductionRuleConstraint_ContextRight::ProductionRuleConstraint_ContextRight(Symbol^ S)
{
	this->s = S;
}

ProductionRuleConstraint_ContextRight::~ProductionRuleConstraint_ContextRight()
{
}

bool ProductionRuleConstraint_ContextRight::Process(Int32 T, Symbol^ L, Symbol^ R)
{
	if (R == this->s)
	{
		return true;
	}
	else
	{
		return false;
	}
}
