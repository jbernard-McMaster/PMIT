#include "stdafx.h"
#include "PMIProblemV2A_L_v2.h"


PMIProblemV2A_L_v2::PMIProblemV2A_L_v2(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Rule(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_L_v2::~PMIProblemV2A_L_v2()
{
}

//List<ProductionRule^>^ PMIProblemV2A_L_v2::Assess_CreateRulesStep(array<Int32>^ Solution)
//{
//	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
//	R = this->CreateRulesByScanning(Solution, this->currentSet->startGeneration, this->currentSet->endGeneration);
//	return R;
//}

GenomeConfiguration<Int32>^ PMIProblemV2A_L_v2::CreateGenome()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;
	// Need a number of genes equal to the number of rules in the sub-problem

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		this->rule2solution->Add(-1);
		this->solution2rule->Add(-1);

		if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
		{
			this->rule2solution[iRule] = numGenes;
			this->solution2rule[numGenes] = iRule;
			numGenes += 1;
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t iGene = 0; iGene < min->Length; iGene++)
	{
		Int32 iRule = this->solution2rule[iGene];
		min[iGene] = this->minRuleLengths[iRule];
		max[iGene] = this->maxRuleLengths[iRule];
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

BruteForceSearchConfiguration^ PMIProblemV2A_L_v2::CreateSearchSpace()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;
	// Need a number of genes equal to the number of rules in the sub-problem

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		this->rule2solution->Add(-1);
		this->solution2rule->Add(-1);

		if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
		{
			this->rule2solution[iRule] = numGenes;
			this->solution2rule[numGenes] = iRule;
			numGenes += 1;
		}
	}

	//array<Int32>^ min = gcnew array<Int32>(numGenes);
	//array<Int32>^ max = gcnew array<Int32>(numGenes);
	array<DimensionSpecType^>^ searchSpace = gcnew array<DimensionSpecType^>(numGenes);

	for (size_t iRule = 0; iRule < searchSpace->Length; iRule++)
	{
		searchSpace[iRule] = gcnew DimensionSpecType();
		searchSpace[iRule]->min = this->minRuleLengths[iRule];
		searchSpace[iRule]->max = this->maxRuleLengths[iRule];
	}

	BruteForceSearchConfiguration^ result = gcnew BruteForceSearchConfiguration(searchSpace);

	return result;
}

