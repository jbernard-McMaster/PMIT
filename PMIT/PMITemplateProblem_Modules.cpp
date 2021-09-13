#include "stdafx.h"
#include "PMITemplateProblem_Modules.h"

PMITemplateProblem_Modules::PMITemplateProblem_Modules(PMIProblem^ Problem) : PMITemplateProblem(Problem)
{
	this->tabu = gcnew List<array<Int32>^>;
}

PMITemplateProblem_Modules::~PMITemplateProblem_Modules()
{
}

array<Int32, 2>^ PMITemplateProblem_Modules::Assess_CreateRulesStep(array<Int32>^ Solution)
{
	Int32 index1 = 0;
	Int32 index2 = 0;

	array<Int32>^ t = gcnew array<Int32>(this->vocabulary->numModules);
	array<Int32>^ s = gcnew array<Int32>(this->vocabulary->numModules);
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules);

	/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
	but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
		{
			// TODO: How should this be working??
			if (this->rulesParikhTemplate[0, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
			{
				t[iSymbol] += Solution[index1];
				index1++;
			}
		}
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
		{
			// TODO: How should this be working??
			if (this->rulesParikhTemplate[0, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
			{
				//rules[iRule, iSymbol] = round(((float)Solution[index2] / t[iSymbol]) * this->maxOccuranceSymbolRule[iRule, iSymbol]);
				float x = ((float)Solution[index2] / t[iSymbol]);
				float y = (x * this->growthSymbol[iSymbol]);
				// TODO: Account for the ParikhVector being zero
				//float z = y / this->evidence[this->vocabulary->numModules - 1]->parikhVector[iSymbol]; // TODO: currently -1 because CreateParikhVectors is using this->vocabulary->numModules, if change to this-numModules + 1, this needs to change to this->vocabulary->numModules
				float z = y / this->evidence[this->evidence->Count-2]->parikhVector[iSymbol]; // TODO: currently -2 because CreateParikhVectors using last evidence, so needs to be the one before it
				float q = round(z);

				//rules[iRule, iSymbol] = Math::Min((int)this->maxOccuranceSymbolRule[iRule, iSymbol], (int)round((((float)Solution[index2] / t[iSymbol]) * this->growthSymbol[iSymbol]) / this->evidence[this->vocabulary->numModules]->parikhVector[iSymbol]));
				rules[iRule, iSymbol] = Math::Min(this->maxPoG[iRule, iSymbol], (int)q);
				index2++;
			}
			else
			{
				// TODO: How should this be working??
				rules[iRule, iSymbol] = this->rulesParikhTemplate[0, iRule, iSymbol];
			}
		}
	}

	return rules;
}

float PMITemplateProblem_Modules::Assess(array<Int32>^ Solution)
{
	float result = 999.0;
	Int32 index = 0;
	Int32 total = 0;
	Int32 errors = 0;
	float errorsValue = 0.0;

	array<Int32>^ axiom;
	array<Int32, 2>^ rules;

	axiom = this->Assess_CreateAxiomStep_Modules(this->vocabulary->numModules, this->evidence[0]->parikhVector);
	rules = this->Assess_CreateRulesStep(Solution);

	array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules);
	array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		last[i] = axiom[i];
	}

	for (size_t iGen = 1; iGen < this->generations; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
		{
			Int32 count = 0;
			for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
			{
				count += last[iRule] * rules[iRule, iSymbol];
			}

			current[iSymbol] = count;
		}

		for (size_t i = 0; i < current->Length; i++)
		{
			errors += Math::Abs(current[i] - this->evidence[iGen]->parikhVector[i]);
			total += this->evidence[iGen]->parikhVector[i];

			last[i] = current[i];
		}
#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 2
		if (((errors == 0) && (iGen >= 2) && (!this->solveForConstants)) || ((errors == 0) && (iGen >= 0) && (this->solveForConstants)))
		{
			Console::WriteLine("Candidate");
			for (size_t i = 0; i < axiom->Length; i++)
			{
				Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
			}
			Console::WriteLine();
			Console::WriteLine(" Rules: ");
			for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
			{
				Console::Write(this->vocabulary->Symbols[i] + " => ");
				for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
				{
					Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
				}
				Console::WriteLine();
			}

			for (size_t i = 0; i < current->Length; i++)
			{
				Console::WriteLine("S (" + this->vocabulary->Symbols[i] + "): Actual " + this->evidence[iGen]->compressed[i] + ", Calculated " + current[i]);
			}
			Console::WriteLine();
		}
#endif
	}

	result = (float)errors / total;

#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 1
	if ((result == 0))
	{
		Console::WriteLine("Candidate");
		for (size_t i = 0; i < axiom->Length; i++)
		{
			Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
		}
		Console::WriteLine();
		Console::WriteLine(" Rules: ");
		for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
		{
			Console::Write(this->vocabulary->Symbols[i] + " => ");
			for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
			{
				Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
			}
			Console::WriteLine();
		}
		Console::WriteLine();
	}
#endif
	//}

	return result;
}

GenomeConfiguration<Int32>^ PMITemplateProblem_Modules::CreateGenome(Int32 IndexStart, Int32 IndexEnd)
{
	// Determine how many genes are needed
	// One gene is needed for each unknown value in the template
	Int32 numGenes = 0;
	for (size_t iRule = 0; iRule < (this->vocabulary->numModules); iRule++)
	{
		for (size_t iSymbol = IndexStart; iSymbol <= IndexEnd; iSymbol++)
		{
			// TODO: How should this be working??
			if (this->rulesParikhTemplate[0, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
			{
				numGenes++;
			}
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Create genes to represent the symbols in the Rules
	for (size_t i = 0; i < min->Length; i++)
	{
		min[i] = 1;
		max[i] = 99;
	}

	//int i = 0;
	//for (size_t iRule = 0; iRule < (this->vocabulary->numModules); iRule++)
	//{
	//	for (size_t iSymbol = IndexStart; iSymbol <= IndexEnd; iSymbol++)
	//	{
	//		min[i] = 1;
	//		max[i] = 99;
	//		i++;
	//	}
	//}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}
