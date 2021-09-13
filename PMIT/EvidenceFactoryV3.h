#pragma once
#include "ProductionRule.h"
#include "ProductionRuleV3.h"
#include "WordV3.h"

using namespace System;

public ref class EvidenceFactoryV3
{
public:
	EvidenceFactoryV3();
	virtual ~EvidenceFactoryV3();

	static array<Int32, 2>^ Process(array<Int32>^ A, array<Int32, 2>^ R, Int32 Generations, Int32 NumSymbols, Int32 NumModules);
	static array<WordXV3^>^ Process(WordXV3^ A, List<ProductionRuleV3^>^ R, Int32 Start, Int32 End, bool AxiomFirst);
	static WordXV3^ Process(WordXV3^ A, List<ProductionRuleV3^>^ R, Int32 T);
};

