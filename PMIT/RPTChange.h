#pragma once

using namespace System;

public ref class RPTChange
{
public:
	RPTChange(Int32 GenerationIndex, Int32 RuleIndex, Int32 SymbolIndex, Int32 OriginalValue);
	virtual ~RPTChange();

	Int32 generationIndex;
	Int32 ruleIndex;
	Int32 symbolIndex;
	Int32 originalValue;
};

