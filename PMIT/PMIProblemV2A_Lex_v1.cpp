#include "stdafx.h"
#include "PMIProblemV2A_Lex_v1.h"


PMIProblemV2A_Lex_v1::PMIProblemV2A_Lex_v1(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Parikh(Model, ErrorRate, FileName)
{
}

PMIProblemV2A_Lex_v1::~PMIProblemV2A_Lex_v1()
{
}

array<Int32, 2>^ PMIProblemV2A_Lex_v1::Assess_CreateParikhRulesStep(array<Int32>^ Solution)
{
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->constantIndex);

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		Int32 iSymbol = 0;

		if ((Solution != nullptr) && (this->rule2Gene[iRule] != -1))
		{
			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			{
				rules[iRule, iSymbol] = this->lexicon->symbolSubwords[iRule][Solution[this->rule2Gene[iRule]]]->parikhVector[iSymbol];
			}
		}
		else
		{
			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
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

UInt64 PMIProblemV2A_Lex_v1::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			result *= this->lexicon->symbolSubwords[iRule]->Count;
		}
	}

	return result;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_Lex_v1::CreateGenome()
{
	Int32 numGenes = 0;
	// For module sub-problems, need a number of genes equal to 
	// the number of unsolved module -> module pairs.
	this->rule2Gene = gcnew array<Int32>(this->vocabulary->numModules);
	
	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		Int32 iSymbol = 0;

		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			this->rule2Gene[iRule] = numGenes;
			numGenes++;
		}
		else
		{
			this->rule2Gene[iRule] = -1;
		}
	}
	
	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	Int32 iGene = 0;
	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			min[iGene] = 0;
			max[iGene] = this->lexicon->symbolSubwords[iRule]->Count - 1;
			iGene++;
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}
