#include "stdafx.h"
#include "PMIProblemV2A_Parikh.h"


PMIProblemV2A_Parikh::PMIProblemV2A_Parikh(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A(Model, ErrorRate, FileName)
{
}

PMIProblemV2A_Parikh::~PMIProblemV2A_Parikh()
{
}

bool PMIProblemV2A_Parikh::AnalyzeSolution(PMISolution^ s)
{
	bool result = false;

	array<Int32, 2>^ rules = this->Assess_CreateParikhRulesStep(s->solution);

	if (s->fitness == 0)
	{
		//this->OutputRules(rules);
		if (this->VerifyOrderScan(rules))
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
			Console::WriteLine("Verification: Success");
#endif
			this->CapturePartialSolution(rules);
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
		this->AddTabuItem(this->currentTabu);
	}

	if (!result)
	{
		this->AddTabuItem(gcnew TabuItem(this->constantIndex, rules));
	}

	this->OutputRules(rules);

	return result;
}

float PMIProblemV2A_Parikh::Assess(array<Int32>^ Solution)
{
	float result = this->currentSet->endGeneration - this->currentSet->startGeneration;

	array<Int32>^ axiom;
	array<Int32, 2>^ rules;

	axiom = this->Assess_CreateAxiomStep(this->vocabulary->numModules + this->constantIndex, this->evidence[this->currentSet->startGeneration]->parikhVector);
	rules = this->Assess_CreateParikhRulesStep(Solution);
	if (!this->IsTabu(this->constantIndex, rules))
	{
		result = this->fitnessFunction->Assess(axiom, rules, this->evidence, this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabulary->numModules + this->constantIndex, this->vocabulary->numModules);
		if (result == 0)
		{
#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
			if (!this->lexicon->VerifyRules(rules, this->symbolSubsets, this->currentSet->startGeneration, this->vocabulary->numModules + this->constantIndex))
			{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS >= 1
				Console::WriteLine("Non-verifiable solution");
#endif
				//result = this->fitnessFunction->Assess(axiom, rules, this->evidence, this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabulary->numModules + this->constantIndex, this->vocabulary->numModules);
				result = this->currentSet->endGeneration - this->currentSet->startGeneration + 1;
				this->AddTabuItem(gcnew TabuItem(this->constantIndex, rules));
			}	
#endif
		}
	}
	else
	{
		result += 1;
	}

	return result;
}

//bool PMIProblemV2A_Parikh::Assess_BranchingSymbol()
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
//	float fitness = this->AssessRuleLengths(ruleLengths);
//
//	if (fitness == 0)
//	{
//		result = true;
//	}
//
//	return result;
//}

array<Int32, 2>^ PMIProblemV2A_Parikh::Assess_CreateParikhRulesStep(array<Int32>^ Solution)
{
	return nullptr;
}

UInt64 PMIProblemV2A_Parikh::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		UInt64 value = 1;
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			value = value * (1 + this->maxPoG[iRule, iSymbol] - this->minPoG[iRule, iSymbol]);
		}
			UInt64 tmp = result * value;
			if (tmp > 0)
			{
				result *= value;
			}
			else
			{
				Console::WriteLine("Overflow on solution space size : " + result + " x " + value + " = 0?");
				result = UINT64_MAX;
			}
	}

	return result;
}
