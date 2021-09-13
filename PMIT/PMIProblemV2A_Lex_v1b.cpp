#include "stdafx.h"
#include "PMIProblemV2A_Lex_v1b.h"


PMIProblemV2A_Lex_v1b::PMIProblemV2A_Lex_v1b(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Parikh(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_Lex_v1b::~PMIProblemV2A_Lex_v1b()
{
}

array<Int32, 2>^ PMIProblemV2A_Lex_v1b::Assess_CreateParikhRulesStep(array<Int32>^ Solution)
{
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->constantIndex);

	// Initialize the growth array
	array<Int32>^ ua = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);

	for (size_t iSymbol = 0; iSymbol <= ua->GetUpperBound(0); iSymbol++)
	{
		ua[iSymbol] = this->growthSymbolByGen[this->currentSet->endGeneration, iSymbol];
	}

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		Int32 iSymbol = 0;

		if ((Solution != nullptr) && (this->rule2Gene[iRule] != -1))
		{
			if (this->rule2Gene[iRule] <= Solution->GetUpperBound(0))
			{
				for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
				{
					rules[iRule, iSymbol] = this->lexicon->symbolSubwords[iRule][Solution[this->rule2Gene[iRule]]]->parikhVector[iSymbol];
					ua[iSymbol] -= this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule] * rules[iRule, iSymbol];
				}
			}
			else
			{// the last rule gets the remaining growth
				for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
				{
					rules[iRule, iSymbol] = ua[iSymbol] / this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule];
					ua[iSymbol] -= this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule] * rules[iRule, iSymbol];
				}
			}
		}
		else
		{
			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
				ua[iSymbol] -= this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule] * rules[iRule, iSymbol];
			}
		}
	}


#if _PMI_PROBLEM_CHEAT_
	rules = this->CheatParikh();
#endif

	return rules;
}

UInt64 PMIProblemV2A_Lex_v1b::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules-1; iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			result *= this->lexicon->symbolSubwords[iRule]->Count;
		}
	}

	return result;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_Lex_v1b::CreateGenome()
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

	array<Int32>^ min = gcnew array<Int32>(numGenes-1);
	array<Int32>^ max = gcnew array<Int32>(numGenes-1);

	Int32 iGene = 0;
	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1) && (iGene <= min->GetUpperBound(0)))
		{
			min[iGene] = 0;
			max[iGene] = this->lexicon->symbolSubwords[iRule]->Count - 1;
			iGene++;
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes-1, min, max);

	return result;
}
