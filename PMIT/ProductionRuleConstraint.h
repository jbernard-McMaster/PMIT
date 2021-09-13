#pragma once
#include "Symbol.h"

using namespace System;
using namespace System::Collections::Generic;

public interface class IProductionRuleConstraint
{
public:

	bool Process(Int32 T, Symbol^ L, Symbol^ R);
};

public ref class ProductionRuleConstraint_Time :
	public IProductionRuleConstraint
{
public:
	ProductionRuleConstraint_Time(Int32 Start, Int32 End);
	virtual ~ProductionRuleConstraint_Time();

	virtual bool Process(Int32 T, Symbol^ L, Symbol^ R);

protected:
private:
	Int32 start;
	Int32 end;
};

public ref class ProductionRuleConstraint_TimeReoccuring :
	public IProductionRuleConstraint
{
public:
	ProductionRuleConstraint_TimeReoccuring(Int32 Start, Int32 Increment);
	virtual ~ProductionRuleConstraint_TimeReoccuring();

	virtual bool Process(Int32 T, Symbol^ L, Symbol^ R);

protected:
private:
	Int32 start;
	Int32 increment;
};

public ref class ProductionRuleConstraint_ContextLeft :
	public IProductionRuleConstraint
{
public:
	ProductionRuleConstraint_ContextLeft(Symbol^ S);
	virtual ~ProductionRuleConstraint_ContextLeft();

	virtual bool Process(Int32 T, Symbol^ L, Symbol^ R);

protected:
private:
	Symbol^ s;
};


public ref class ProductionRuleConstraint_ContextRight :
	public IProductionRuleConstraint
{
public:
	ProductionRuleConstraint_ContextRight(Symbol^ S);
	virtual ~ProductionRuleConstraint_ContextRight();

	virtual bool Process(Int32 T, Symbol^ L, Symbol^ R);

protected:
private:
	Symbol^ s;
};
