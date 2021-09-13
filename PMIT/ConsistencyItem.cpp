#include "stdafx.h"
#include "ConsistencyItem.h"


ConsistencyItem::ConsistencyItem(ProductionRule^ R)
{
	this->rule = R;
	this->count = 1;
}


ConsistencyItem::~ConsistencyItem()
{
}
