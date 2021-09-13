#include "stdafx.h"
#include "PMIProblemV2A_Rule.h"


PMIProblemV2A_Rule::PMIProblemV2A_Rule(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_Rule::~PMIProblemV2A_Rule()
{
}

void PMIProblemV2A_Rule::AddTabuItem(TabuItem^ T)
{
	if (T != nullptr)
	{
#if _PMI_PROBLEM_DISPLAY_TABU_ >= 0
		Console::WriteLine("Adding Tabu Item");
		this->OutputRules(T->rules);
#if _PMI_PROBLEM_BREAKS_ >= 1
		Console::ReadLine();
#endif
#endif
		if (!this->IsTabu(T->index, T->rules))
		{
			this->tabu->Add(T);
		}
	}
}

bool PMIProblemV2A_Rule::AnalyzeSolution(PMISolution^ s)
{
	bool result = false;
	array<Int32>^ ruleLengths = this->Assess_CreateRuleLengths(s->solution);
	List<ProductionRule^>^ rules = this->Assess_CreateRulesStep(ruleLengths);
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
			this->currentTabu->ruleLengths = ruleLengths;
			//this->currentTabu->solution = s->solution;
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

float PMIProblemV2A_Rule::Assess(array<Int32>^ Solution)
{
	if (Solution != nullptr)
	{
		return this->AssessRuleLengths(this->Assess_CreateRuleLengths(Solution));
	}
	else // this gets called when there is no solution to assess but rather when all the rule lengths are known
	{
		return this->AssessRuleLengths(Solution);
	}
}

//bool PMIProblemV2A_Rule::Assess_BranchingSymbol()
//{
//	bool result = false;
//	this->rule2solution = gcnew List<Int32>;
//
//	array<Int32>^ ruleLengths = this->ComputeRuleLengthFromRPT(this->currentSet->startGeneration, this->vocabulary->numModules + this->constantIndex - 2);
//	for (size_t iRule = 0; iRule <= ruleLengths->GetUpperBound(0); iRule++)
//	{
//		// Take the rule lengths and add the amount of the previous symbol to the length
//		// E.g. if they previous symbol was "]" and there were two of them then the length with the "[" should be two longer
//		ruleLengths[iRule] += this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, this->vocabulary->numModules + this->constantIndex - 2];
//		this->rule2solution->Add(iRule);
//	}
//
//	float fitness = this->Assess(ruleLengths);
//
//	if (fitness == 0)
//	{
//		result = true;
//	}
//
//	return result;
//}

//List<ProductionRule^>^ PMIProblemV2A_Rule::Assess_CreateRulesStep(array<Int32>^ Solution)
//{
//	return nullptr;
//}
//
//float PMIProblemV2A_Rule::AssessProductionRules(List<ProductionRule^>^ Rules)
//{
//	float result = 0.0;
//
//	Word^ a = gcnew Word(this->evidence[this->currentSet->startGeneration]->symbols);
//	List<Word^>^ solutionAxioms = gcnew List<Word^>;
//	solutionAxioms = EvidenceFactory::Process(a, Rules, this->currentSet->startGeneration, this->currentSet->endGeneration, true);
//	result = this->CompareEvidence_Positional(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentSet->startGeneration, this->currentSet->endGeneration);
//
//	return result;
//}

array<Int32>^ PMIProblemV2A_Rule::Assess_CreateRuleLengths(array<Int32>^ Solution)
{
	return Solution;
}

UInt64 PMIProblemV2A_Rule::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		result = result * (1 + this->maxRuleLengths[iRule] - this->minRuleLengths[iRule]);
	}
	return result;
}
