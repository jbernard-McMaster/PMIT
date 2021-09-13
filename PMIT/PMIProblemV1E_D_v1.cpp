#include "stdafx.h"
#include "PMIProblemV1E_D_v1.h"


PMIProblemV1E_D_v1::PMIProblemV1E_D_v1(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2_V1Emulator(Model, ErrorRate, FileName)
{
}


PMIProblemV1E_D_v1::~PMIProblemV1E_D_v1()
{
}

bool PMIProblemV1E_D_v1::AnalyzeSolution(PMISolution^ s)
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
		if (this->currentTabu->index >= 0)
		{
			this->RollbackAnalysisForTabu(this->currentTabu);
		}
	}

	if (!result)
	{
		TabuItem^ t = gcnew TabuItem(this->constantIndex, rules);
		t->ruleLengths = s->solution;
		this->AddTabuItem(t);
	}

#if _PMI_PROBLEM_DISPLAY_RULES_ > 0
	this->OutputRules(rules);
#endif // _PMI_PROBLEM_DISPLAY_RULES_ > 0

	return result;
}

float PMIProblemV1E_D_v1::AssessRuleLengths(array<Int32>^ RuleLengths, bool ForceRuleLength)
{
	float fitness = 0.0;
	bool tabu = false;

#if _PMI_PROBLEM_CHEAT_ > 0
	RuleLengths = this->Cheat(RuleLengths);
#endif

	List<ProductionRule^>^ modelRules;

	if ((ForceRuleLength && !this->IsTabu(this->constantIndex, RuleLengths)) || (!ForceRuleLength))
	{
		// Sometimes need to force this to treat the incoming values as absolute rule lengths
		// For example, when assess branching symbols
		// Otherwise need to create the step using whatever custom method exists for creating them
		// The function "Assess_CreateRulesStep" originally only accepted ruleLengths but really this should be a solution
		if (ForceRuleLength)
		{
			modelRules = this->Assess_CreateRulesStep_RuleLengths(RuleLengths);
		}
		else
		{
			modelRules = this->Assess_CreateRulesStep(RuleLengths);
			tabu = this->IsTabu(this->constantIndex, modelRules);
		}


		if ((modelRules != nullptr) && (!tabu))
		{
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine(" Rules: ");
			for each (ProductionRule^ r in modelRules)
			{
				Console::WriteLine("   " + r->ToString());
			}
			Console::WriteLine();
#endif 
			fitness = this->AssessProductionRules(modelRules);
		}
		else
		{
			fitness = this->currentSet->endGeneration - this->startGeneration + 1;
		}
	}
	else
	{
		fitness = this->currentSet->endGeneration - this->startGeneration + 1;
	}

	return fitness;
}

List<ProductionRule^>^ PMIProblemV1E_D_v1::Assess_CreateRulesStep(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;

	return rules;
}

void PMIProblemV1E_D_v1::ComputeFragmentsFromPartialSolution()
{
	//for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//{
	//	List<Symbol^>^ toAdd = gcnew List<Symbol^>;

	//	Int32 iSymbol = 0;
	//	bool done = false;

	//	// find all the symbols before the first module
	//	while ((!done) && (iSymbol < this->rulesSolution[iRule]->successor->Count))
	//	{
	//		if (this->ruleFragmentsMaster[iRule]->symbols->Count > 0)
	//		{
	//			if (this->rulesSolution[iRule]->successor[iSymbol] != this->ruleFragmentsMaster[iRule][0])
	//			{
	//				//Console::Write(this->rulesSolution[iRule]->successor[iSymbol]);
	//				toAdd->Add(this->rulesSolution[iRule]->successor[iSymbol]);
	//				iSymbol++;
	//			}
	//			else
	//			{
	//				done = true;
	//			}
	//		}
	//		else
	//		{
	//			toAdd->AddRange(this->rulesSolution[iRule]->successor);
	//			done = true;
	//		}
	//	}

	//	for (size_t i = 0; i < toAdd->Count; i++)
	//	{
	//		this->ruleFragmentsMaster[iRule]->symbols->Insert(i, toAdd[i]);
	//	}

	//	//Console::WriteLine();

	//	toAdd->Clear();
	//	iSymbol = this->rulesSolution[iRule]->successor->Count-1;
	//	done = false;

	//	// find all the symbols before the first module
	//	while ((!done) && (iSymbol > 0))
	//	{
	//		if (this->rulesSolution[iRule]->successor[iSymbol] != this->ruleFragmentsMaster[iRule][this->ruleFragmentsMaster[iRule]->symbols->Count-1])
	//		{
	//			//Console::Write(this->rulesSolution[iRule]->successor[iSymbol]);
	//			toAdd->Add(this->rulesSolution[iRule]->successor[iSymbol]);
	//			iSymbol--;
	//		}
	//		else
	//		{
	//			done = true;
	//		}
	//	}

	//	toAdd->Reverse();
	//	//Console::WriteLine();

	//	for (size_t i = 0; i < toAdd->Count; i++)
	//	{
	//		this->ruleFragmentsMaster[iRule]->symbols->Add(toAdd[i]);
	//	}
	//}
}

UInt64 PMIProblemV1E_D_v1::ComputeSolutionSpaceSize()
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

GenomeConfiguration<Int32>^ PMIProblemV1E_D_v1::CreateGenome()
{
	Int32 numGenes = 0;

	for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
		{
			numGenes += this->maxPoG[iRule, iSymbol];
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Gene 0..R ... represent the 1st gene in each rule, since every rule must have 1 gene
	// Values: 0..S, where S is the number of possible symbols
	// Gene R..N ... represent the symbols that cannot be blank due to minimum/maximums
	// Values: 0..S, where S is the number of possible symbols
	// Gene N..M represent the symbols that may be blank
	// Values: 0..S+1 where S is the number of possible symbols, value S+1 indicates blank

	// Example: if there is a maximum of 6 symbols (total) and a minimum of 5 symbols and two rules.
	// Genes 1..2 represent the first symbol in each rule
	// Genes 3..5 represent the symbols that cannot be blank
	// Gene 6 represent a possible blank symbol

	Int32 iGene = 0;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		Int32 idx = 0;
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			for (size_t i = 0; i < this->maxPoG[iRule, iSymbol]; i++)
			{
				if (idx == 0)
				{
					min[iGene] = 0;
					max[iGene] = this->vocabulary->NumSymbols() - 1; // the first symbol of each rule cannot be blank
				}
				else
				{
					min[iGene] = 0;
					max[iGene] = this->vocabulary->NumSymbols(); // don't need the plus 1 for the blank as this is zero-indexed
				}
				iGene++;
				idx++;
			}
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

