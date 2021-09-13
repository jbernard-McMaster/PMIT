#pragma once
#include "PMIProblem.h"

public ref class PMIProblemV2 :
	public PMIProblem
{
public:
	PMIProblemV2(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2();

	TabuItem^ currentTabu;
	array<Int32,2>^ growthSymbolByGen;

	array<bool>^ solvedModules;
	array<bool>^ solvedModulesMaster;

	virtual array<Int32>^ ComputeRuleLengths(List<ProductionRule^>^ R);
	virtual void ComputeGrowthPattern() override;

	virtual bool EstimateRuleLengths(bool WithLexicon, bool WithInference) override;
	virtual bool EstimateRuleLengths_Fragments() override;
	virtual bool EstimateRuleLengths_Growth() override;
	virtual bool EstimateRuleLengths_Inference(Int32 Start, Int32 End) override;
	virtual bool EstimateRuleLengths_Lexicon() override;
	virtual bool EstimateRuleLengths_PoG() override;

	virtual bool IsModuleInSet(Int32 iRule);

	virtual void UpdateTotalRuleLengths() override;

protected:
};

