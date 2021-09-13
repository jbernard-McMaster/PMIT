#pragma once
#include "ProductionRuleV3.h"

public ref class ProductionTableV3
{
public:
	ProductionTableV3();
	ProductionTableV3(Int32 Odds);
	virtual ~ProductionTableV3();

	Int32 odds;

	List<ProductionRuleV3^>^ rules;

	bool ShouldFire(Int32 O);
};

