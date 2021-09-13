#include "stdafx.h"
#include "ProductionTableV3.h"


ProductionTableV3::ProductionTableV3()
{
	this->rules = gcnew List<ProductionRuleV3^>;
	this->odds = 100;
}

ProductionTableV3::ProductionTableV3(Int32 Odds)
{
	this->rules = gcnew List<ProductionRuleV3^>;
	this->odds = Odds;
}

ProductionTableV3::~ProductionTableV3()
{
}

bool ProductionTableV3::ShouldFire(Int32 O)
{
	if (O <= this->odds)
	{
		return true;
	}
	return false;
}