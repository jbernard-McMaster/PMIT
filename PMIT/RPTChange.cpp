#include "stdafx.h"
#include "RPTChange.h"


RPTChange::RPTChange(Int32 GenerationIndex, Int32 RuleIndex, Int32 SymbolIndex, Int32 OriginalValue)
{
	this->generationIndex = GenerationIndex;
	this->ruleIndex = RuleIndex;
	this->symbolIndex = SymbolIndex;
	this->originalValue = OriginalValue;
}


RPTChange::~RPTChange()
{
}
