#include "stdafx.h"
#include "PMIProblemV2A_PoG_v2.h"


PMIProblemV2A_PoG_v2::PMIProblemV2A_PoG_v2(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_PoG_v1(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_PoG_v2::~PMIProblemV2A_PoG_v2()
{
}

array<Int32, 2>^ PMIProblemV2A_PoG_v2::Assess_CreateParikhRulesStep(array<Int32>^ Solution)
{
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->constantIndex);

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->constantIndex; iSymbol++)
		{
			// TODO: this is really inefficient, I should precalculate which rule/symbol combinations need to be solved
			//if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
			if ((Solution != nullptr) && (this->ruleAndSymbol2Gene[iRule, iSymbol] != -1))
			{
				rules[iRule, iSymbol] = Solution[this->ruleAndSymbol2Gene[iRule, iSymbol]];
			}
			else
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
			}
		}
	}

#if _PMI_PROBLEM_CHEAT_
	rules = this->CheatParikh();
#endif

	return rules;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_PoG_v2::CreateGenome()
{
	Int32 numGenes = 0;
	// For module sub-problems, need a number of genes equal to 
	// the number of unsolved module -> module pairs.
	this->ruleAndSymbol2Gene = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd + this->constantIndex; iSymbol++)
		{
			// Only consider symbols which:
			// 1 - have not already been solved
			// 2 - Only those symbols which exist within the sub-problem
			if (!this->IsModuleSolvedForSymbol(iRule, iSymbol) && this->IsModuleInSet(iRule))
				//if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
			{
				numGenes += 1;
			}
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	Int32 iGene = 0;
	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd + this->constantIndex; iSymbol++)
		{
			// Only consider symbols which:
			// 1 - have not already been solved
			// 2 - Only those symbols which exist within the sub-problem
			if (!this->IsModuleSolvedForSymbol(iRule, iSymbol) && this->IsModuleInSet(iRule))
			{
				this->ruleAndSymbol2Gene[iRule, iSymbol] = iGene;
				min[iGene] = this->minPoG[iRule, iSymbol];
				max[iGene] = this->maxPoG[iRule, iSymbol];
				iGene++;
			}
			else
			{
				this->ruleAndSymbol2Gene[iRule, iSymbol] = -1;
			}
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}
