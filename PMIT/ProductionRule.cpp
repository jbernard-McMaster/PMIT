#include "stdafx.h"
#include "ProductionRule.h"

ProductionRule::ProductionRule()
{
	this->successor = gcnew List<Symbol^>;
	this->constraints = gcnew List<IProductionRuleConstraint^>;
}

ProductionRule::ProductionRule(Symbol^ Precursor, Symbol^ Operation)
{
	this->predecessor = Precursor;
	this->operation = Operation;
}

ProductionRule::ProductionRule(Symbol^ Precursor, Symbol^ Operation, List<Symbol^>^ Successor)
{
	this->predecessor = Precursor;
	this->operation = Operation;
	this->successor = Successor;
	this->constraints = gcnew List<IProductionRuleConstraint^>;
}

ProductionRule::ProductionRule(Symbol^ Precursor, Symbol^ Operation, List<Symbol^>^ Successor, List<IProductionRuleConstraint^>^ Constraints)
{
	this->predecessor = Precursor;
	this->operation = Operation;
	this->successor = Successor;
	this->constraints = Constraints;
}

ProductionRule::~ProductionRule()
{
}

void ProductionRule::Display()
{
	Console::Write(this->predecessor->Value() + " => ");
	for (size_t iSymbol = 0; iSymbol < this->successor->Count; iSymbol++)
	{
		Console::Write(this->successor[iSymbol]);
	}
	Console::WriteLine();
}

bool ProductionRule::IsMatch(List<Symbol^>^ X)
{
	bool result = true;
	Int32 index = 0;

	do
	{
		if (X[index] != this->predecessor)
		{
			result = false;
		}

		index++;
	} while ((index < X->Count) && (result));


	return result;
}

bool ProductionRule::IsMatch(Symbol^ X)
{
	bool result = true;

	if (X != this->predecessor)
	{
		result = false;
	}

	return result;
}

List<Symbol^>^ ProductionRule::Produce(Int32 T, Symbol^ L, Symbol^ R)
{
	if (this->ShouldFire(T, L, R))
	{
		return this->successor;
	}
	else
	{
		return nullptr;
	}
}

bool ProductionRule::ShouldFire(Int32 T, Symbol^ L, Symbol^ R)
{
	bool result = true;

	for each (IProductionRuleConstraint^ c in this->constraints)
	{
		result = result && c->Process(T, L, R);
	}

	return result;
}
