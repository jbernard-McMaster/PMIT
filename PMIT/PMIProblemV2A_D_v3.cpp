#include "stdafx.h"
#include "PMIProblemV2A_D_v3.h"


PMIProblemV2A_D_v3::PMIProblemV2A_D_v3(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Rule(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_D_v3::~PMIProblemV2A_D_v3()
{
}

bool PMIProblemV2A_D_v3::AnalyzeSolution(PMISolution^ s)
{
	bool result = false;
	List<ProductionRule^>^ rules = this->Assess_CreateRulesStep(s->solution);
	if (s->fitness == 0)
	{
		//this->OutputRules(rules);
		//if (this->VerifyOrderScan(rules))
		bool verified = true;

		if (verified)
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
			Console::WriteLine("Verification: Success");
#endif
			this->currentTabu = gcnew TabuItem(this->constantIndex, rules);
			this->CapturePartialSolution_Rules(rules);
			result = true;
		}
		else
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
			Console::WriteLine("Verification: Failure");
#endif
		}
	}
	else
	{// if there is no solution than the previous solution is no good and should be marked tabu
	 //this->AddTabuItem(this->currentTabu);
		this->RollbackAnalysisForTabu(this->currentTabu);
	}

	if (!result)
	{
		TabuItem^ t = gcnew TabuItem(this->constantIndex, rules);
		t->ruleLengths = s->solution;
		this->AddTabuItem(t);
	}

	this->OutputRules(rules);

	return result;
}

List<ProductionRule^>^ PMIProblemV2A_D_v3::Assess_CreateRulesStep(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;

	Int32 iGene = this->vocabulary->numModules;

	// Make a set of rules with blank successors
	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;
		rules->Add(gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s));
	}

	for (size_t iGene = this->vocabulary->numModules; iGene <= Solution->GetUpperBound(0); iGene++)
	{
		Int32 iRule = Solution[iGene] / this->vocabulary->numConstants;
		Int32 iSymbol = Solution[iGene] % this->vocabulary->numConstants;

		for (size_t j = this->vocabulary->IndexModulesStart; j <= this->vocabulary->IndexModulesEnd; j++)
		{
			if (Solution[j] == iGene)
			{
				rules[j]->successor->AddRange(this->ruleFragments[j]->symbols);
			}
		}

		if (iRule < this->vocabulary->numModules)
		{
			rules[iRule]->successor->Add(this->vocabulary->Symbols[this->vocabulary->IndexConstantsStart + iSymbol]);
		}
	}

	// Check to see if the fragment should be added at the end
	for (size_t j = this->vocabulary->IndexModulesStart; j <= this->vocabulary->IndexModulesEnd; j++)
	{
		if (Solution[j] == Solution->Length)
		{
			rules[j]->successor->AddRange(this->ruleFragments[j]->symbols);
		}
	}
	
	Int32 index = 0;

	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;
		s->Add(this->vocabulary->Symbols[Solution[iRule]]);
		rules->Add(gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s));
		index++;
	}



	for (size_t iValue = index; iValue <= Solution->GetUpperBound(0); iValue++)
	{
		Int32 iRule = Solution[iValue] / this->vocabulary->numModules;
		Int32 iSymbol = Solution[iValue] % this->vocabulary->numModules;

		// if the division causes a value greater than the number of rules treat as a blank or unassigned symbol
		if (iRule < this->vocabulary->numModules)
		{
			rules[iRule]->successor->Add(this->vocabulary->Symbols[iSymbol]);
		}
	}

	return rules;
}

UInt64 PMIProblemV2A_D_v3::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules - 1; iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			result *= this->lexicon->symbolSubwords[iRule]->Count;
		}
	}

	return result;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_D_v3::CreateGenome()
{
	Int32 numGenes = 0;
	Int32 totalFragmentLength = 0;
	Int32 numBlanks = this->totalMaxRuleLength - this->totalMinRuleLength;

	numGenes = this->vocabulary->numModules; // one gene for each modules to show where the fragment will go
	for (size_t iRule = 0; iRule <= this->ruleFragments->GetUpperBound(0); iRule++)
	{
		totalFragmentLength += this->ruleFragments[iRule]->Count;
	}
	numGenes += this->totalMaxRuleLength - totalFragmentLength;

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Gene 0..R ... represent the 1st gene in each rule, since every rule must have 1 gene
	// Values: 0..S, where S is the number of possible symbols
	// TODO: current assumes every symbol is possible, this should only be the symbol under consideration
	// Gene R..N ... represent the symbols that cannot be blank due to minimum/maximums
	// Values: 0..S, where S is the number of possible symbols
	// Gene N..M represent the symbols that may be blank
	// Values: 0..S+1 where S is the number of possible symbols, value S+1 indicates blank

	// Example: if there is a maximum of 6 symbols (total) and a minimum of 5 symbols and two rules.
	// Genes 1..2 represent the first symbol in each rule
	// Genes 3..5 represent the symbols that cannot be blank
	// Gene 6 represent a possible blank symbol

	for (size_t iGene = 0; iGene < min->Length; iGene++)
	{
		if (iGene < this->vocabulary->numModules)
		{
			min[iGene] = this->vocabulary->numModules;
			max[iGene] = min->Length; // represent at what point to insert the fragment based on how many genes processed so far
		}
		else if (iGene - this->vocabulary->numModules >= numBlanks)
		{
			min[iGene] = 0;
			max[iGene] = (this->vocabulary->numModules * this->vocabulary->numConstants) - 1;
		}
		else
		{
			min[iGene] = 0;
			max[iGene] = (this->vocabulary->numModules * this->vocabulary->numConstants);
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

