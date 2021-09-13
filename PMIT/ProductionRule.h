#pragma once
#include "Symbol.h"
#include "ProductionRuleConstraint.h"

using namespace System::Collections::Generic;

public interface class IProductionRule
{
};

public ref class ProductionRule :
	public IProductionRule
{
public:
	ProductionRule();
	ProductionRule(Symbol^ Predecessor, Symbol^ Operation);
	ProductionRule(Symbol^ Predecessor, Symbol^ Operation, List<Symbol^>^ Successor);
	ProductionRule(Symbol^ Predecessor, Symbol^ Operation, List<Symbol^>^ Successor, List<IProductionRuleConstraint^>^ Constraints);
	virtual ~ProductionRule();

	Symbol^ predecessor;
	List<Symbol^>^ successor;
	List<Symbol^>^ successorAbstract;
	Symbol^ operation;
	List<IProductionRuleConstraint^>^ constraints;

	void Display();

	bool IsMatch(List<Symbol^>^ X);
	bool IsMatch(Symbol^ X);

	virtual List<Symbol^>^ Produce(Int32 T, Symbol^ L, Symbol^ R);
	bool ShouldFire(Int32 T, Symbol^ L, Symbol^ R);

	bool operator==(ProductionRule^ R)
	{
		bool result;

		result = (this->successor->Count == R->successor->Count);

		if (result)
		{
			result = result && (this->predecessor == R->predecessor);
			Int32 iSymbol = 0;
			while ((result) && (iSymbol < this->successor->Count))
			{
				result == result && (this->successor[iSymbol] == R->successor[iSymbol]);
				iSymbol++;
			}
		}

		return result;
	}

	String^ ToString() override
	{
		String^ result = "";

		result += this->predecessor->ToString();

		result += " " + this->operation->ToString() + " ";

		for (size_t i = 0; i < this->successor->Count; i++)
		{
			result += this->successor[i]->ToString();
		}

		return result;
	}
	String^ ToStringAbstract() override
	{
		String^ result = "";
		result += this->predecessor->abstraction.ToString(L"G");

		result += " " + this->operation->ToString() + " ";

		for (size_t i = 0; i < this->successor->Count; i++)
		{
			result += this->successor[i]->abstraction.ToString(L"G");
		}

		return result;
	}

	static const Int32 cEndOfTime = 999999;
	static const UInt64 cMaxLength = 2500000000;

protected:

private:
};

