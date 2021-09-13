#include "stdafx.h"
#include "ProductionRuleStochastic.h"


ProductionRuleStochastic::ProductionRuleStochastic() : ProductionRule()
{
	this->successors = gcnew List<Successor^>;
}

ProductionRuleStochastic::ProductionRuleStochastic(Symbol^ Predecessor, Symbol^ Operation, Successor^ S, List<IProductionRuleConstraint^>^ Constraints) : ProductionRule(Predecessor, Operation, S->symbols, Constraints)
{
	this->successors = gcnew List<Successor^>;
	this->successors->Add(S);
}

ProductionRuleStochastic::~ProductionRuleStochastic()
{
}


List<Symbol^>^ ProductionRuleStochastic::Produce(Int32 T, Symbol^ L, Symbol^ R)
{
	if (this->ShouldFire(T, L, R))
	{
		float dieRoll = CommonGlobals::PoolFloat();
		bool found = false;
		Int32 idx = 0;
		float total = 0;

		while (!found)
		{
			total += this->successors[idx]->chance;

			if (total > dieRoll)
			{
				found = true;
			}
			else
			{
				idx++;
			}
		}

		return this->successors[idx]->symbols;
	}
	else
	{
		return nullptr;
	}
}
