#include "stdafx.h"
#include "PMIProblemV2A_PoG_v1b.h"


PMIProblemV2A_PoG_v1b::PMIProblemV2A_PoG_v1b(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_PoG_v1(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_PoG_v1b::~PMIProblemV2A_PoG_v1b()
{
}

array<Int32, 2>^ PMIProblemV2A_PoG_v1b::Assess_CreateParikhRulesStep(array<Int32>^ Solution)
{
	Int32 index1 = 0;
	Int32 index2 = 0;

	array<Int32>^ t = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->constantIndex);

	array<Int32>^ ua = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);

	for (size_t iSymbol = 0; iSymbol <= ua->GetUpperBound(0); iSymbol++)
	{
		ua[iSymbol] = this->growthSymbolByGen[this->currentSet->endGeneration, iSymbol];
	}


	if (Solution != nullptr)
	{

		/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
		but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->constantIndex; iSymbol++)
			{
				// TODO: this is really inefficient, I should precalculate which rule/symbol combinations need to be solved
				//if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
				if (this->ruleAndSymbol2Gene[iRule, iSymbol] != -1)
				{
					t[iSymbol] += Solution[index1];
					index1++;
				}
			}
		}
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->constantIndex; iSymbol++)
		{
			// TODO: this is really inefficient, I should precalculate which rule/symbol combinations need to be solved
			if (this->IsModuleSolvedForSymbol(iRule, iSymbol))
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
				ua[iSymbol] -= this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule] * rules[iRule, iSymbol];
			}
		}
	}

	if (Solution != nullptr)
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->constantIndex; iSymbol++)
			{
				//if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
				// if the module is not solved for this symbol, solve for it
				if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
				{
					// Calculate the fraction of the growth this symbol will be responsible for
					float fractionOfGrowth;
					// If there is a value in the solution compute a fraction of growth
					if (this->ruleAndSymbol2Gene[iRule, iSymbol] != -1)
					{
						fractionOfGrowth = ((float)Solution[index2] / PMIProblem::cGene_Max);
					}
					else
					{// otherwise it must be the last symbol and it gets all the growth remaining
						fractionOfGrowth = 1.0;
					}
					// Calculate the amount of growth this symbol is responsible for
					//float growth = (float)(fractionOfGrowth * this->growthSymbol[iSymbol]);
					//float growth = (float)(fractionOfGrowth * this->growthSymbolByGen[this->currentSet->endGeneration, iSymbol]);
					float growth = (float)(fractionOfGrowth * ua[iSymbol]);
					// divide the amount of growth by the number of symbols in the evidence
					//float parikhValue = growth / (float) this->evidence[this->lastGenerationSymbol[iSymbol] - 1]->parikhVector[iRule];
					float parikhValue = growth / (float) this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule];
					parikhValue = Math::Min(this->maxPoG[iRule, iSymbol], (int)round(parikhValue));
					parikhValue = Math::Max(this->minPoG[iRule, iSymbol], (int)round(parikhValue));

					// reduce the unaccounted for growth by the computed growth for this symbol
					ua[iSymbol] -= this->evidence[this->currentSet->endGeneration - 1]->parikhVector[iRule] * parikhValue;

					rules[iRule, iSymbol] = parikhValue;
					index2++;
				}
			}
		}
	}

#if _PMI_PROBLEM_CHEAT_
	rules = this->CheatParikh();
#endif

	return rules;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_PoG_v1b::CreateGenome()
{
	Int32 numGenes = 0;
	// For module sub-problems, need a number of genes equal to 
	// the number of unsolved module -> module pairs.
	this->ruleAndSymbol2Gene = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd + this->constantIndex; iSymbol++)
	{
		bool first = true;
		for (int iRule = this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule >= 0; iRule--)
		{
			// Only consider symbols which:
			// 1 - have not already been solved
			// 2 - Only those symbols which exist within the sub-problem
			// 3 - The growth is dynamically created so the number of genes needed for each symbol is reduced by 1
			
			if ((!first) && (!this->IsModuleSolvedForSymbol(iRule, iSymbol)) && (this->IsModuleInSet(iRule)))
			{
				this->ruleAndSymbol2Gene[iRule, iSymbol] = numGenes;
				numGenes += 1;
			}
			else
			{
				first = false;
				this->ruleAndSymbol2Gene[iRule, iSymbol] = -1;
			}
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t i = 0; i < min->Length; i++)
	{
		min[i] = PMIProblem::cGene_Min;
		max[i] = PMIProblem::cGene_Max;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

