#include "stdafx.h"
#include "PMITemplateProblem.h"

PMITemplateProblem::PMITemplateProblem(PMIProblem^ Problem) : PMIProblem(Problem)
{
}

PMITemplateProblem::~PMITemplateProblem()
{
}

//array<Int32>^ PMITemplateProblem::Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution)
//{
//	array<Int32>^ axiom = gcnew array<Int32>(Size);
//
//	for (size_t i = 0; i < axiom->Length; i++)
//	{
//		axiom[i] = Solution[i];
//	}
//
//	return axiom;
//}

array<Int32, 2>^ PMITemplateProblem::Assess_CreateRulesStep(array<Int32>^ Solution)
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
			t[iSymbol] += Solution[index1];
			index1++;
		}
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
		{
			//rules[iRule, iSymbol] = round(((float)Solution[index2] / t[iSymbol]) * this->maxOccuranceSymbolRule[iRule, iSymbol]);
			rules[iRule, iSymbol] = round((((float)Solution[index2] / t[iSymbol]) * this->growthSymbol[iSymbol]) / this->evidence[this->vocabulary->numModules]->parikhVector[iSymbol]);
			index2++;
		}
	}

	return rules;
}

float PMITemplateProblem::Assess(array<Int32>^ Solution)
{
	float result = 999.0;
	Int32 index = 0;
	Int32 total = 0;
	Int32 errors = 0;
	float errorsValue = 0.0;

	array<Int32>^ axiom;
	array<Int32,2>^ rules;

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

//void PMITemplateProblem::CreateParikhVectors(Int32 IndexStart, Int32 IndexEnd)
//{
//	for each (Axiom^ a in this->evidence)
//	{
//		this->vocabulary->CompressAxiom(a);
//	}
//
//	this->minOccuranceSymbolRule = gcnew array<Int32, 2>(this->vocabulary->numModules, IndexEnd - IndexStart + 1);
//	this->maxOccuranceSymbolRule = gcnew array<Int32, 2>(this->vocabulary->numModules, IndexEnd - IndexStart + 1);
//	this->growthSymbol = gcnew array<Int32>(IndexEnd - IndexStart + 1);
//
//	for (size_t i = 0; i <= this->maxOccuranceSymbolRule->GetUpperBound(0); i++)
//	{
//		for (size_t j = 0; j <= this->maxOccuranceSymbolRule->GetUpperBound(1); j++)
//		{
//			this->maxOccuranceSymbolRule[i, j] = 99999;
//		}
//	}
//
//	/* TODO: 
//	2 - Find the set of symbols in each generation. Look for where the symbols repeat.
//		Can solving the repeating part independently of the non-repeating part.
//	*/
//
//	for (size_t iGen = 0; iGen < this->evidence->Count-1; iGen++)
//	{
//		// Get the two generations of evidence
//		Axiom^ x = this->evidence[iGen+1];
//		Axiom^ y = this->evidence[iGen];
//
//		// For each symbol figure out the maximum growth by symbol
//		for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
//		{
//			for (size_t iSymbol = IndexStart; iSymbol <= IndexEnd; iSymbol++)
//			{
//				if (y->parikhVector[iRule] > 0)
//				{
//					if (iSymbol < this->vocabulary->numModules)
//					{
//						//this->maxOccuranceSymbolRule[iRule, iSymbol] = Math::Floor((x->parikhVector[iRule] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]) + (1 * (iRule == iSymbol));
//						this->maxOccuranceSymbolRule[iRule, iSymbol] = Math::Min(this->maxOccuranceSymbolRule[iRule, iSymbol], (int)Math::Floor((x->parikhVector[iSymbol]) / y->parikhVector[iRule]));
//					}
//					else
//					{
//						this->maxOccuranceSymbolRule[iRule, iSymbol - IndexStart] = Math::Min(this->maxOccuranceSymbolRule[iRule, iSymbol - IndexStart], (int)Math::Floor((x->parikhVector[iSymbol] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]));
//					}
//				}
//			}
//		}
//	}
//
//	/* TODO:
//
//	This can be used to solve systems which change over time by solving for the evidence at each generation instead of just the N+1 generation
//	
//	*/
//
//	// Get the growth for the N+1 generation
//	for (size_t iSymbol = IndexStart; iSymbol <= IndexEnd; iSymbol++)
//	{
//		//this->growthSymbol[iSymbol] = this->evidence[this->vocabulary->numModules + 1]->parikhVector[iSymbol];
//		this->growthSymbol[iSymbol - IndexStart] = this->evidence[this->vocabulary->numModules]->parikhVector[iSymbol];
//	}
//}

GenomeConfiguration<Int32>^ PMITemplateProblem::CreateGenome(Int32 IndexStart, Int32 IndexEnd)
{
	Int32 numGenes = (this->vocabulary->numModules) * (IndexEnd - IndexStart);
	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Create genes to represent the symbols in the Rules
	int i = 0;
	for (size_t iRule = 0; iRule < (this->vocabulary->numModules); iRule++)
	{
		for (size_t iSymbol = IndexStart; iSymbol < IndexEnd; iSymbol++)
		{
			min[i] = this->minPoG[iRule, iSymbol];
			max[i] = this->maxPoG[iRule, iSymbol];
			i++;
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

/* THE OLD METHOD OF DOING ASSESS */

//float PMITemplateProblem::Assess(array<Int32>^ Solution)
//{
//	float result = 999.0;
//	Int32 index = 0;
//	Int32 total = 0;
//	Int32 errors = 0;
//	float errorsValue = 0.0;
//
//	array<Int32>^ axiom;
//	array<Int32, 2>^ rules;
//
//	axiom = this->Assess_CreateAxiomStep(this->vocabulary->numModules, this->evidence[0]->parikhVector);
//	rules = this->Assess_CreateRulesStep(Solution);
//
//	array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules);
//	array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules);
//
//	for (size_t i = 0; i < this->vocabulary->numModules; i++)
//	{
//		last[i] = axiom[i];
//	}
//
//	for (size_t iGen = 1; iGen < this->generations; iGen++)
//	{
//		for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
//		{
//			Int32 count = 0;
//			for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
//			{
//				count += last[iRule] * rules[iRule, iSymbol];
//			}
//
//			current[iSymbol] = count;
//		}
//
//		for (size_t i = 0; i < current->Length; i++)
//		{
//			errors += Math::Abs(current[i] - this->evidence[iGen]->parikhVector[i]);
//			total += this->evidence[iGen]->parikhVector[i];
//
//			last[i] = current[i];
//		}
//#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 2
//		if (((errors == 0) && (iGen >= 2) && (!this->solveForConstants)) || ((errors == 0) && (iGen >= 0) && (this->solveForConstants)))
//		{
//			Console::WriteLine("Candidate");
//			for (size_t i = 0; i < axiom->Length; i++)
//			{
//				Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
//			}
//			Console::WriteLine();
//			Console::WriteLine(" Rules: ");
//			for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
//			{
//				Console::Write(this->vocabulary->Symbols[i] + " => ");
//				for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
//				{
//					Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
//				}
//				Console::WriteLine();
//			}
//
//			for (size_t i = 0; i < current->Length; i++)
//			{
//				Console::WriteLine("S (" + this->vocabulary->Symbols[i] + "): Actual " + this->evidence[iGen]->compressed[i] + ", Calculated " + current[i]);
//			}
//			Console::WriteLine();
//		}
//#endif
//	}
//
//	result = (float)errors / total;
//
//#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 1
//	if ((result == 0))
//	{
//		Console::WriteLine("Candidate");
//		for (size_t i = 0; i < axiom->Length; i++)
//		{
//			Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
//		}
//		Console::WriteLine();
//		Console::WriteLine(" Rules: ");
//		for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
//		{
//			Console::Write(this->vocabulary->Symbols[i] + " => ");
//			for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
//			{
//				Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
//			}
//			Console::WriteLine();
//		}
//		Console::WriteLine();
//	}
//#endif
//	//}
//
//	return result;
//}
