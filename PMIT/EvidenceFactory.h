#pragma once
#include "Word.h"
#include "ProductionRule.h"
#include "ProductionRuleStochastic.h"

public ref class EvidenceFactory
{
public:
	EvidenceFactory();
	virtual ~EvidenceFactory();

	static array<Int32, 2>^ Process(array<Int32>^ A, array<Int32, 2>^ R, Int32 Generations, Int32 NumSymbols, Int32 NumModules);
	static List<Word^>^ Process(Word^ A, List<ProductionRule^>^ R, Int32 Start, Int32 End, bool AxiomFirst);
	static List<Word^>^ Process(Word^ A, List<ProductionRuleStochastic^>^ R, Int32 Start, Int32 End, bool AxiomFirst);
	static Word^ Process(Word^ A, List<ProductionRule^>^ R);

private:
};

