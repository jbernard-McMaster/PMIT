#pragma once
#include "ProductionRule.h"



public ref class ConsistencyItem
{
public:
	ConsistencyItem(ProductionRule^ R);
	virtual ~ConsistencyItem();

	ProductionRule^ rule;
	Int32 count;
};

