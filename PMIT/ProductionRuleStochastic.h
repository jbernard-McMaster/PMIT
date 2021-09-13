#pragma once
#include "ProductionRule.h"
#include "Successor.h"

public ref class ProductionRuleStochastic :
	public ProductionRule
{
public:
	ProductionRuleStochastic();
	ProductionRuleStochastic(Symbol^ Predecessor, Symbol^ Operation, Successor^ Successor, List<IProductionRuleConstraint^>^ Constraints);
	virtual ~ProductionRuleStochastic();

	List<Successor^>^ successors;

	virtual List<Symbol^>^ Produce(Int32 T, Symbol^ L, Symbol^ R) override;
};

