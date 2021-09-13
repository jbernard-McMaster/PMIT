#include "stdafx.h"
#include "PMITemplateProblem_Constants.h"

PMITemplateProblem_Constants::PMITemplateProblem_Constants(PMIProblem^ Problem) : PMITemplateProblem(Problem)
{
	this->tabu = gcnew List<array<Int32>^>;
}

PMITemplateProblem_Constants::~PMITemplateProblem_Constants()
{
}

array<Int32, 2>^ PMITemplateProblem_Constants::Assess_CreateRulesStep(array<Int32>^ Solution)
{
	Int32 index1 = 0;
	Int32 index2 = 0;

	Int32 t = 0;
	//array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->vocabulary->numConstants);
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + 1);

	/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
	but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		t += Solution[index1];
		index1++;
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + 1; iSymbol++)
		{
			if (iSymbol < this->vocabulary->numModules)
			{
				rules[iRule, iSymbol] = this->rulesModules[iRule, iSymbol];
			}
			else
			{
				//rules[iRule, iSymbol] = round(((float)Solution[index2] / t) * this->maxOccuranceSymbolRule[iRule, iSymbol - this->vocabulary->IndexConstantsStart]);
				rules[iRule, iSymbol] = round(((float)Solution[index2] / t) * this->maxPoG[iRule, this->constantIndex]);
				index2++;
			}
		}
	}

	return rules;
}

array<Int32>^ PMITemplateProblem_Constants::Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution)
{
	array<Int32>^ axiom = gcnew array<Int32>(Size);

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		axiom[i] = Solution[i];
	}

	axiom[Size - 1] = Solution[this->constantIndex];

	return axiom;
}

//array<Int32, 2>^ PMITemplateProblem_Constants::Assess_CreateRulesStepORIGINAL(array<Int32>^ Solution)
//{
//	Int32 index1 = 0;
//	Int32 index2 = 0;
//
//	array<Int32>^ t = gcnew array<Int32>(this->vocabulary->numConstants);
//	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->vocabulary->numConstants);
//
//	/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
//	but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
//	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
//	{
//		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numConstants; iSymbol++)
//		{
//			t[iSymbol] += Solution[index1];
//			index1++;
//		}
//	}
//
//	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
//	{
//		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->vocabulary->numConstants; iSymbol++)
//		{
//			if (iSymbol < this->vocabulary->numModules)
//			{
//				rules[iRule, iSymbol] = this->rulesModules[iRule,iSymbol];
//			}
//			else
//			{
//				rules[iRule, iSymbol] = round(((float)Solution[index2] / t[iSymbol - this->vocabulary->IndexConstantsStart]) * this->maxOccuranceSymbolRule[iRule, iSymbol - this->vocabulary->IndexConstantsStart]);
//				index2++;
//			}
//		}
//	}
//
//	return rules;
//
//	//array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, (this->vocabulary->numModules + this->vocabulary->numConstants));
//	//Int32 index1 = 0;
//	//Int32 index2 = 0;
//
//	//for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
//	//{
//	//	for (size_t iSymbol = 0; iSymbol < (this->vocabulary->numModules + this->vocabulary->numConstants); iSymbol++)
//	//	{
//	//		if (iSymbol < this->vocabulary->numModules)
//	//		{
//	//			rules[iRule, iSymbol] = this->moduleParikhVector[index2];
//	//			index2++;
//	//		}
//	//		else
//	//		{
//	//			rules[iRule, iSymbol] = Solution[index1];
//	//			index1++;
//	//		}
//	//	}
//	//}
//
//	//return rules;
//}

float PMITemplateProblem_Constants::Assess(array<Int32>^ Solution)
{
	float result = 999.0;
	// check to see if the solution is on the tabu list
	bool disallowed = false;
	Int32 iTabu = 0;

	//while ((!disallowed) && (iTabu < this->tabu->Count))
	//{
	//	array<Int32>^ tmp = this->tabu[iTabu];
	//	Int32 index = 0;
	//	bool matches = true;

	//	do
	//	{
	//		matches = matches && (Solution[index] == tmp[index]);
	//		index++;
	//	} while ((index < Solution->Length) && (matches));

	//	disallowed = disallowed || matches;
	//	iTabu++;
	//}

	//if (disallowed)
	//{
	//	Console::WriteLine("Solution on tabu list");
	//}
	if (!disallowed)
	{
		Int32 index1 = 0;
		Int32 index2 = 0;
		Int32 total = 0;
		Int32 errors = 0;
		float errorsValue = 0.0;
		Int32 idx = this->vocabulary->IndexConstantsStart + this->constantIndex;

		array<Int32>^ axiom;
		array<Int32, 2>^ rules;

		//axiom = this->Assess_CreateAxiomStep(this->vocabulary->numModules + this->vocabulary->numConstants, this->evidence[0]->parikhVector);
		axiom = this->Assess_CreateAxiomStep(this->vocabulary->numModules + 1, this->evidence[0]->parikhVector);
		rules = this->Assess_CreateRulesStep(Solution);

		array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules + 1);
		array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules + 1);
		//array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules + this->vocabulary->numConstants);
		//array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules + this->vocabulary->numConstants);

		//for (size_t i = 0; i < this->vocabulary->numModules + this->vocabulary->numConstants; i++)
		for (size_t i = 0; i < this->vocabulary->numModules + 1; i++)
			{
			last[i] = axiom[i];
			if (i >= this->vocabulary->numModules)
			{
				current[i] = axiom[i]; // for constants need to prepopulate the current number so it will be tracked properly
			}
		}

		for (size_t iGen = 1; iGen < this->generations; iGen++)
		{
			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			{
				Int32 count = 0;
				for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
				{
					if (iSymbol < this->vocabulary->numModules)
					{
						count += last[iRule] * rules[iRule, iSymbol];
					}
					else
					{
						current[iSymbol] += last[iRule] * rules[iRule, iSymbol];
					}
				}

				if (iSymbol < this->vocabulary->numModules)
				{
					current[iSymbol] = count;
				}
			}

			for (size_t i = 0; i < current->Length; i++)
			{
				if (i >= this->vocabulary->IndexConstantsStart)
				{
					errors += Math::Abs(current[i] - this->evidence[iGen]->parikhVector[this->constantIndex]);
					total += this->evidence[iGen]->parikhVector[this->constantIndex];
				}

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
		if ((result == 0) && (this->solveForConstants))
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
	}

	return result;
}

GenomeConfiguration<Int32>^ PMITemplateProblem_Constants::CreateGenome(Int32 IndexStart, Int32 IndexEnd)
{
	//Int32 numGenes = (this->vocabulary->numModules) * (1 + IndexEnd - IndexStart);
	Int32 numGenes = (this->vocabulary->numModules) * 1; // doing only one constant at a time
	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Create genes to represent the symbols in the Rules
	for (size_t iGene = 0; iGene < numGenes; iGene++)
	{
		min[iGene] = 1;
		max[iGene] = 99;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}