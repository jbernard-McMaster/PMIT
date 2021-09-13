#include "stdafx.h"
#include "LiKProblem.h"


LiKProblem::LiKProblem(ModelV3^ M) : LSIProblemV3(M, LSIProblemV3::ProblemType::Master)
{
	this->model = M;
}

LiKProblem::~LiKProblem()
{
}

void LiKProblem::Initialize()
{
	// filter out an turtles from the model



	this->lengthMin = gcnew array<Int32>(this->model->alphabet->numNonTurtles);
	this->lengthMax = gcnew array<Int32>(this->model->alphabet->numNonTurtles);

	this->growthMin = gcnew array<Int32,2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	this->growthMax = gcnew array<Int32,2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);

	this->totalLengthMin = 0;
	this->totalLengthMax = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		this->totalLengthMin += this->model->productions[0]->rules[iSymbol]->successor[0]->Length;
		this->totalLengthMax += this->model->productions[0]->rules[iSymbol]->successor[0]->Length;

		this->lengthMin[iSymbol] = Math::Max(1, (int)(this->model->productions[0]->rules[iSymbol]->successor[0]->Length-this->lengthMinError));
		this->lengthMax[iSymbol] = this->model->productions[0]->rules[iSymbol]->successor[0]->Length+this->lengthMaxError;

		array<Int32>^ PV = this->model->alphabet->CreateParikhVector(this->model->productions[0]->rules[iSymbol]->successor[0]);

		for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
		{
			this->growthMin[iSymbol, jSymbol] = Math::Max(0, (int)(PV[jSymbol] - this->growthMinError));
			this->growthMax[iSymbol, jSymbol] = (PV[jSymbol] + this->growthMaxError);
		}
	}

	this->totalLengthMin -= this->totalLengthMinError;
	this->totalLengthMax += this->totalLengthMaxError;

	this->uaMaster = gcnew array<Int32, 2>(this->model->generations - 1, this->model->alphabet->numNonTurtles);

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			this->uaMaster[iGen, iSymbol] = this->model->evidence[iGen+1]->parikhVector[iSymbol];
			for (size_t kSymbol = 0; kSymbol < this->model->alphabet->numNonTurtles; kSymbol++)
			{
				this->uaMaster[iGen, iSymbol] -= this->growthMin[kSymbol, iSymbol] * this->model->evidence[iGen]->parikhVector[kSymbol];
			}
		}
	}

	this->ComputeSymbolLexicons();
}

Int32 LiKProblem::ChooseSymbolFromLexicon(Int32 iSymbol, Int32 Roll, bool BlankAllowed)
{
	Int32 result = this->symbolOddsWithBlank->GetUpperBound(1);
	float choice;
	Int32 idx = 0;
	float total = 0;
	bool done = false;
	if (BlankAllowed)
	{
		choice = (float)Roll / this->maxValue1;

		if (choice <= this->symbolOddsWithBlank[iSymbol, this->symbolOddsWithBlank->GetUpperBound(1)-1])
		{
			while (!done)
			{
				if (choice <= this->symbolOddsWithBlank[iSymbol, idx])
				{
					done = true;
				}
				else
				{
					idx++;
				}
			}
			result = idx;
		}
	}
	else
	{
		choice = (float)Roll / this->maxValue2;
		while (!done)
		{
			if (choice <= this->symbolOdds[iSymbol, idx])
			{
				done = true;
			}
			else
			{
				idx++;
			}
		}
		result = idx;
	}

	return result;
}

float LiKProblem::AssessProductionRulesParikh(array<Int32, 2>^ Rules)
{
	//Rules[0, 0] = 2;
	//Rules[0, 1] = 1;
	//Rules[1, 0] = 0;
	//Rules[1, 1] = 3;

	float fitness = 999;

	Int32 errors = 0;
	Int32 total = 0;
	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		array<Int32>^ produced = gcnew array<Int32>(this->model->alphabet->numNonTurtles);
		total += this->model->evidence[iGen]->Length();

		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			for (size_t iRule = 0; iRule < this->model->alphabet->numNonTurtles; iRule++)
			{
				produced[iSymbol] += this->model->evidence[iGen]->parikhVector[iRule] * Rules[iRule, iSymbol];
			}
		}

		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			errors += Math::Abs(produced[iSymbol] - this->model->evidence[iGen+1]->parikhVector[iSymbol]);
		}
	}

	fitness = (float) errors / total;

	return fitness;
}

float LiKProblem::Assess_D(array<Int32>^ Solution)
{
	//Solution[0] = 0;
	//Solution[1] = 1;
	//Solution[2] = 1;
	//Solution[3] = 10;
	//Solution[4] = 400;
	//Solution[5] = 400;

	Solution[0] = 0;
	//Solution[1] = 150;
	//Solution[2] = 0;
	//Solution[3] = 150;
	//Solution[4] = 0;
	//Solution[5] = 2;
	//Solution[6] = 0;
	//Solution[7] = 300;
	Solution[4] = 2;
	//Solution[9] = 300;

	float fitness = 999;
	fitness = 0.0;
	List<ProductionRuleV3^>^ rules = gcnew List<ProductionRuleV3^>;
	Int32 idx = 0;
	Int32 numRules = this->model->alphabet->numNonTurtles;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		ProductionRuleV3^ r = gcnew ProductionRuleV3();
		r->condition->Add("*");
		r->predecessor = this->model->alphabet->GetSymbol(iSymbol);
		r->contextLeft->Add("*");
		r->contextRight->Add("*");
		r->condition->Add("*");
		r->odds->Add(100);
		r->parameters = this->model->parameters;

		for (size_t jSymbol = 0; jSymbol < this->lengthMin[iSymbol]; jSymbol++)
		{
			if (jSymbol == 0)
			{
				r->successor->Add(this->model->alphabet->GetSymbol(Solution[idx]));
				idx++;
			}
			else
			{
				Int32 last = this->model->alphabet->FindIndex(r->successor[0]->Substring(r->successor[0]->Length - 1, 1));
				Int32 kSymbol = this->ChooseSymbolFromLexicon(last, Solution[idx], false);
				r->successor[0] += this->model->alphabet->GetSymbol(kSymbol);
				idx++;
			}
		}

		rules->Add(r);
	}
	Int32 iRule;

	while (idx <= Solution->GetUpperBound(0))
	{
		iRule = Solution[idx] / this->maxValue1;
		ProductionRuleV3^ r = rules[iRule];
		Int32 last = this->model->alphabet->FindIndex(rules[iRule]->successor[0]->Substring(rules[iRule]->successor[0]->Length - 1, 1));
		Int32 kSymbol = this->ChooseSymbolFromLexicon(last, Solution[idx] - (this->maxValue1 * (iRule)), true);
		if (kSymbol < this->model->alphabet->numNonTurtles)
		{
			r->successor[0] += this->model->alphabet->GetSymbol(kSymbol);
		}
		idx++;
	}

	if (rules != nullptr)
	{
#if _PMI_PROBLEM_VERBOSE_ >= 1
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif 
		fitness = this->AssessProductionRules(rules);
	}
	return fitness;
}

float LiKProblem::Assess_G(array<Int32>^ Solution)
{
	float fitness = 999;

	array<Int32, 2>^ rulesParikh = gcnew array<Int32, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	Int32 idx = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
		{
			rulesParikh[iSymbol, jSymbol] = Solution[idx];
			idx++;
		}
	}

	fitness = 100 + this->AssessProductionRulesParikh(rulesParikh);

	if (fitness == 100)
	{
		array<Int32>^ ruleLengths = gcnew array<Int32>(this->model->alphabet->numNonTurtles);
		for (size_t iRule = 0; iRule < this->model->alphabet->numNonTurtles; iRule++)
		{
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
			{
				ruleLengths[iRule] += rulesParikh[iRule, iSymbol];
			}
		}

		List<ProductionRuleV3^>^ rules = this->Assess_CreateRulesStep_RuleLengths(ruleLengths);
		if (rules != nullptr)
		{
			fitness = this->AssessProductionRules(rules);
		}
	}
	
	return fitness;
}

float LiKProblem::Assess_L(array<Int32>^ Solution)
{
	float fitness = 999;
	List<ProductionRuleV3^>^ rules = this->Assess_CreateRulesStep_RuleLengths(Solution);
	if (rules != nullptr)
	{
#if _PMI_PROBLEM_VERBOSE_ >= 1
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif 
		fitness = this->AssessProductionRules(rules);
	}

	return fitness;
}

float LiKProblem::Assess_PoG(array<Int32>^ Solution)
{
	float fitness = 999;

	array<Int32, 2>^ rulesParikh = gcnew array<Int32, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	array<Int32,2>^ ua = gcnew array<Int32,2>(this->model->generations-1, this->model->alphabet->numNonTurtles);

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
		{
			rulesParikh[iSymbol, jSymbol] = -1;
		}
	}

	Array::Copy(uaMaster, ua, uaMaster->Length);
	Int32 idx = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
		{
			// Calculate the fraction of the growth this symbol will be responsible for
			float fractionOfGrowth;
			// If there is a value in the solution compute a fraction of growth
			if (iSymbol < this->model->alphabet->numNonTurtles-1)
			{
				fractionOfGrowth = ((float)Solution[idx] / 999);
			}
			else
			{// otherwise it must be the last symbol and it gets all the growth remaining
				fractionOfGrowth = 1.0;
			}
			// Calculate the amount of growth this symbol is responsible for
			float growth = (float)(fractionOfGrowth * ua[ua->GetUpperBound(0), jSymbol]);
			// divide the amount of growth by the number of symbols in the evidence
			float parikhValue = this->growthMin[iSymbol, jSymbol] + (growth / (float) this->model->evidence[this->model->generations - 2]->parikhVector[iSymbol]);

			parikhValue = Math::Min(this->growthMax[iSymbol, jSymbol], (int)round(parikhValue));
			parikhValue = Math::Max(this->growthMin[iSymbol, jSymbol], (int)round(parikhValue));

			// reduce the unaccounted for growth by the computed growth for this symbol
			ua[ua->GetUpperBound(0), jSymbol] -= this->model->evidence[this->model->generations - 2]->parikhVector[iSymbol] * parikhValue;

			rulesParikh[iSymbol, jSymbol] = parikhValue;
			idx++;
		}
	}

	idx = 0;
	Int32 iGen = 0;

	fitness = 100 + this->AssessProductionRulesParikh(rulesParikh);

	if (fitness == 100)
	{
		array<Int32>^ ruleLengths = gcnew array<Int32>(this->model->alphabet->numNonTurtles);
		for (size_t iRule = 0; iRule < this->model->alphabet->numNonTurtles; iRule++)
		{
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
			{
				ruleLengths[iRule] += rulesParikh[iRule, iSymbol];
			}
		}

		List<ProductionRuleV3^>^ rules = this->Assess_CreateRulesStep_RuleLengths(ruleLengths);
		if (rules != nullptr)
		{
			fitness = this->AssessProductionRules(rules);
		}
	}

	return fitness;
}

float LiKProblem::Assess_PoL(array<Int32>^ Solution)
{
	float fitness = 999;
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->model->alphabet->numNonTurtles);

	Int32 t = 0;

	for (size_t i = 0; i < Solution->GetUpperBound(0); i++)
	{
		t += Solution[i];
	}

	for (size_t iRule = 0; iRule < Solution->GetUpperBound(0); iRule++)
	{
		float proportion = (float)Solution[iRule] / t;
		float length = Solution[Solution->GetUpperBound(0)] * proportion;
		Int32 tmp = (int)Math::Round(length);
		ruleLengths[iRule] = Math::Min(this->lengthMax[iRule], Math::Max(this->lengthMin[iRule], tmp));
	}

	List<ProductionRuleV3^>^ rules = this->Assess_CreateRulesStep_RuleLengths(ruleLengths);
	if (rules != nullptr)
	{
#if _PMI_PROBLEM_VERBOSE_ >= 1
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif 
		fitness = this->AssessProductionRules(rules);
	}

	return fitness;
}

float LiKProblem::Assess_Lex(array<Int32>^ Solution)
{
	float fitness = 999;
	return fitness;
}

float LiKProblem::Assess(array<Int32>^ Solution)
{
	float fitness = 999;
	if (this->problemType == LiKProblemType::D)
	{
		fitness = this->Assess_D(Solution);
	}
	else if (this->problemType == LiKProblemType::L)
	{
		fitness = this->Assess_L(Solution);
	}
	else if (this->problemType == LiKProblemType::PoL)
	{
		fitness = this->Assess_PoL(Solution);
	}
	else if (this->problemType == LiKProblemType::G)
	{
		fitness = this->Assess_G(Solution);
	}
	else if (this->problemType == LiKProblemType::PoG)
	{
		fitness = this->Assess_PoG(Solution);
	}
	else if (this->problemType == LiKProblemType::Lex)
	{
		fitness = this->Assess_Lex(Solution);
	}

	return fitness;
}

List<ProductionRuleV3^>^ LiKProblem::CreateRulesByScanning(array<Int32>^ RuleLengths)
{
	List<ProductionRuleV3^>^ result = gcnew List<ProductionRuleV3^>;
	List<Int32>^ completed = gcnew List<Int32>;
	Int32 iGen = 0;
	Int32 iPos = 0;
	Int32 iScan = 0;

	while (completed->Count < this->model->alphabet->numNonTurtles && iGen < this->model->generations - 1)
	{
		iPos = 0;
		iScan = 0;
		while (completed->Count < this->model->alphabet->numNonTurtles && iPos < this->model->evidence[iGen]->Length())
		{
			//Int32 iTable = 0;
			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

			// get the right fact
			//Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);
			//Int32 iFact = this->MAO->GetFactIndex(f);

			if (!completed->Contains(iSymbol) && iScan + RuleLengths[iSymbol] <= this->model->evidence[iGen + 1]->Length())
			{
				// check to see if a suitable rule already exists
				Int32 iRule = 0;
				bool found = false;

				while (!found && iRule < result->Count)
				{
					if (result[iRule]->predecessor == this->model->alphabet->GetSymbol(iSymbol))
					{
						found = true;
					}
					else
					{
						iRule++;
					}
				}


				if (!found)
				{
					ProductionRuleV3^ r = gcnew ProductionRuleV3();
					r->predecessor = this->model->alphabet->GetSymbol(iSymbol);
					r->contextLeft->Add("*");
					r->contextRight->Add("*");
					r->condition->Add("*");
					r->odds->Add(100);
					r->parameters = this->model->parameters;
					if (iScan + RuleLengths[iSymbol] - 1 <= this->model->evidence[iGen + 1]->Length() - 1)
					{
						r->successor->Add(this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[iSymbol] - 1));
						result->Add(r);
						completed->Add(iSymbol);
					}
				}
				else
				{
					result[iRule]->contextLeft->Add("*");
					result[iRule]->contextRight->Add("*");
					result[iRule]->condition->Add("*");
					result[iRule]->odds->Add(100);
					if (iScan + RuleLengths[iSymbol] - 1 <= this->model->evidence[iGen + 1]->Length() - 1)
					{
						result[iRule]->successor->Add(this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[iSymbol] - 1));
						completed->Add(iSymbol);
					}
				}
			}

			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				iScan += RuleLengths[iSymbol];
			}
			else
			{
				iScan++;
			}
			iPos++;
		}
		iGen++;
	}

	if (completed->Count < this->model->alphabet->numNonTurtles)
	{
		result = nullptr;
	}
	else
	{// Sort the Production Rules
		result = this->SortProductionRules(result);
	}

	return result;
}

GenomeConfiguration<Int32>^ LiKProblem::CreateGenome()
{
	Int32 numGenes;
	array<Int32>^ min;
	array<Int32>^ max;
	UInt64 solutionSpaceSize = 1;

	if (this->problemType == LiKProblemType::D)
	{
		numGenes = this->totalLengthMax;

		min = gcnew array<Int32>(numGenes);
		max = gcnew array<Int32>(numGenes);

		Int32 idx = 0;

		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			for (size_t iGene = 0; iGene < this->lengthMin[iSymbol]; iGene++)
			{
				if (iGene == 0)
				{
					min[idx] = 0;
					max[idx] = this->model->alphabet->numNonTurtles-1;
					idx++;
					solutionSpaceSize *= (this->model->alphabet->numNonTurtles);
				}
				else
				{
					min[idx] = 0;
					max[idx] = (this->wheelSize * this->model->alphabet->numNonTurtles)-1;
					idx++;
					solutionSpaceSize *= (this->model->alphabet->numNonTurtles);
				}
			}
		}

		Int32 tmp = idx;

		for (size_t iGene = 0; iGene < (this->totalLengthMax - tmp); iGene++)
		{
			min[idx] = 0;
			max[idx] = (this->model->alphabet->numNonTurtles * (this->wheelSize * (this->model->alphabet->numNonTurtles + 1))) - 1;
			idx++;
			solutionSpaceSize *= (this->model->alphabet->numNonTurtles+1);
		}

		this->maxValue1 = this->wheelSize * (this->model->alphabet->numNonTurtles + 1);
		this->maxValue2 = this->wheelSize * (this->model->alphabet->numNonTurtles);
	}
	else if (this->problemType == LiKProblemType::L)
	{
		numGenes = this->lengthMin->GetUpperBound(0)+1;
		min = gcnew array<Int32>(numGenes);
		max = gcnew array<Int32>(numGenes);

		for (size_t iGene = 0; iGene < numGenes; iGene++)
		{
			min[iGene] = this->lengthMin[iGene];
			max[iGene] = this->lengthMax[iGene];
			solutionSpaceSize *= 1 + (this->lengthMax[iGene] - this->lengthMin[iGene]);
		}
	}
	else if (this->problemType == LiKProblemType::PoL)
	{
		numGenes = this->model->alphabet->numNonTurtles+1;
		min = gcnew array<Int32>(numGenes);
		max = gcnew array<Int32>(numGenes);

		for (size_t iGene = 0; iGene < numGenes; iGene++)
		{
			if (iGene == numGenes - 1)
			{
				min[iGene] = totalLengthMin;
				max[iGene] = totalLengthMax;
				solutionSpaceSize *= 1 + (totalLengthMax - totalLengthMin);
			}
			else
			{
				min[iGene] = 0;
				max[iGene] = 999;
				solutionSpaceSize *= 1 + (totalLengthMax - totalLengthMin);
			}
		}
	}
	else if (this->problemType == LiKProblemType::G)
	{
		numGenes = (this->growthMin->GetUpperBound(0)+1) * (this->growthMin->GetUpperBound(1)+1);
		min = gcnew array<Int32>(numGenes);
		max = gcnew array<Int32>(numGenes);
		Int32 idx = 0;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
			{
				min[idx] = this->growthMin[iSymbol, jSymbol];
				max[idx] = this->growthMax[iSymbol, jSymbol];
				idx++;
				solutionSpaceSize *= 1 + (this->growthMax[iSymbol,jSymbol] - this->growthMin[iSymbol, jSymbol]);
			}
		}
	}
	else if (this->problemType == LiKProblemType::PoG)
	{
		numGenes = this->model->alphabet->numNonTurtles * (this->model->alphabet->numNonTurtles-1);
		min = gcnew array<Int32>(numGenes);
		max = gcnew array<Int32>(numGenes);
		Int32 idx = 0;

		for (size_t iGene = 0; iGene < numGenes; iGene++)
		{
			min[iGene] = 0;
			max[iGene] = 999;
		}

		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
			{
				float fractionOfGrowth = 1.0;
				// Calculate the amount of growth this symbol is responsible for
				float growth = (float)(fractionOfGrowth * uaMaster[uaMaster->GetUpperBound(0), jSymbol]);
				// divide the amount of growth by the number of symbols in the evidence
				float parikhValue = (growth / (float) this->model->evidence[this->model->generations - 2]->parikhVector[iSymbol]);
				solutionSpaceSize *= parikhValue;
			}
		}
	}
	else if (this->problemType == LiKProblemType::Lex)
	{

	}


	for (size_t iGene = 0; iGene < numGenes; iGene++)
	{
		solutionSpaceSize *= (max[iGene] - min[iGene])+1;
	}

	Console::WriteLine("Size = " + solutionSpaceSize);

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

array<Int32>^ LiKProblem::ConvertSolutionToRuleLengths(array<Int32>^ Solution)
{
	array<Int32, 2>^ rulesParikh = gcnew array<Int32, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	Int32 idx = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		for (size_t jSymbol = 0; jSymbol < this->model->alphabet->numNonTurtles; jSymbol++)
		{
			rulesParikh[iSymbol, jSymbol] = Solution[idx];
			idx++;
		}
	}

	array<Int32>^ ruleLengths = gcnew array<Int32>(this->model->alphabet->numNonTurtles);
	for (size_t iRule = 0; iRule < this->model->alphabet->numNonTurtles; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
		{
			ruleLengths[iRule] += rulesParikh[iRule, iSymbol];
		}
	}

	return ruleLengths;
}

void LiKProblem::ComputeSymbolLexicons()
{
	this->symbolLexicon = gcnew array<Int32, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	this->symbolOdds= gcnew array<float, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles);
	this->symbolOddsWithBlank = gcnew array<float, 2>(this->model->alphabet->numNonTurtles, this->model->alphabet->numNonTurtles+1);
	this->symbolOptions = gcnew array<Int32>(this->model->alphabet->numNonTurtles);

	for (size_t iGen = 0; iGen <= this->model->evidence->GetUpperBound(0); iGen++)
	{
		String^ current = "";
		String^ previous = "";

		Int32 currentSymbolID = -1;
		Int32 previousSymbolID = -1;

		//this->model->evidence[iGen]->Display();

		for (size_t iSymbol = 0; iSymbol < this->model->evidence[iGen]->Length(); iSymbol++)
		{
			// copy current symbols into previous before getting the current symbol
			previous = current;
			previousSymbolID = currentSymbolID;

			// get the current symbol
			current = this->model->evidence[iGen]->GetSymbol(iSymbol);
			currentSymbolID = this->model->alphabet->FindIndex(current);

			if (currentSymbolID != -1 && previousSymbolID != -1)
			{
				this->symbolLexicon[currentSymbolID, previousSymbolID]++;
			}
		}
	}

	for (size_t iSymbol = 0; iSymbol <= this->symbolLexicon->GetUpperBound(0); iSymbol++)
	{
		Int32 total = 0;
		Int32 count = 0;
		for (size_t jSymbol = 0; jSymbol <= this->symbolLexicon->GetUpperBound(1); jSymbol++)
		{
			total += this->symbolLexicon[iSymbol, jSymbol];
			if (this->symbolLexicon[iSymbol, jSymbol] > 0)
			{
				count++;
			}
		}

		float odds = 0;
		for (size_t jSymbol = 0; jSymbol <= this->symbolLexicon->GetUpperBound(1); jSymbol++)
		{
			odds += (float)this->symbolLexicon[iSymbol, jSymbol] / total;
			this->symbolOdds[iSymbol, jSymbol] = odds;
		}

		odds = 0;
		for (size_t jSymbol = 0; jSymbol <= this->symbolOddsWithBlank->GetUpperBound(1); jSymbol++)
		{
			if (jSymbol < this->symbolOddsWithBlank->GetUpperBound(1))
			{
				odds += (float)this->symbolLexicon[iSymbol, jSymbol] / (2 * total);
				this->symbolOddsWithBlank[iSymbol, jSymbol] = odds;
			}
			else
			{
				this->symbolOddsWithBlank[iSymbol, jSymbol] = 1;
			}
		}

		this->symbolOptions[iSymbol] = count;
	}
}

void LiKProblem::Generate()
{
	this->lengthDistribution = gcnew array<float>(10);
	this->symbolDistribution = gcnew array<float>(6);

	this->lengthDistribution[0] = 0.04; // 0.04
	this->lengthDistribution[1] = 0.20; // 0.16
	this->lengthDistribution[2] = 0.40; // 0.20
	this->lengthDistribution[3] = 0.60; // 0.20
	this->lengthDistribution[4] = 0.80; // 0.20
	this->lengthDistribution[5] = 0.92; // 0.12
	this->lengthDistribution[6] = 0.96; // 0.04
	this->lengthDistribution[7] = 0.98; // 0.2
	this->lengthDistribution[8] = 0.99; // 0.1
	this->lengthDistribution[9] = 1.00; // 0.1

	this->symbolDistribution[0] = 0.20; // 0.20
	this->symbolDistribution[1] = 0.70; // 0.50
	this->symbolDistribution[2] = 0.90; // 0.20
	this->symbolDistribution[3] = 0.95; // 0.05
	this->symbolDistribution[4] = 0.98; // 0.03
	this->symbolDistribution[5] = 1.00; // 0.02

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		Int32 length = 0;
		Int32 numSymbols = 0;

		float roll = CommonGlobals::PoolFloat();
		Int32 idx = 0;

		while (roll > this->lengthDistribution[idx])
		{
			idx++;
		}
		length = idx+1;

		idx = 0;
		roll = CommonGlobals::PoolFloat();
		while (roll > this->symbolDistribution[idx])
		{
			idx++;
		}
		numSymbols = idx+1;

		array<Int32>^ symbols = gcnew array<Int32>(numSymbols);
		for (size_t j = 0; j < numSymbols; j++)
		{
			symbols[j] = CommonGlobals::PoolInt(0, this->model->alphabet->numNonTurtles);
		}

		String^ successor = "";
		for (size_t j = 0; j < length; j++)
		{
			successor += this->model->alphabet->GetSymbol(CommonGlobals::PoolInt(0, symbols->GetUpperBound(0)));
		}

		Console::WriteLine(successor);
		this->model->productions[0]->rules[iSymbol]->successor[0] = successor;
	}
}

void LiKProblem::Solve()
{
	System::IO::StreamWriter^ sw;
	std::chrono::duration<double> duration;
	std::chrono::milliseconds ms;
	auto startTime = Time::now();
	auto endTime = Time::now();

	//auto startTime = Time::now();
	//Console::WriteLine("Solving PMIT-D");
	//this->problemType = LiKProblemType::D;
	//sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_D.csv", false);
	//this->SolveProblemGA(sw);
	//auto endTime = Time::now();
	//std::chrono::duration<double> duration = endTime - startTime;
	//std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	//Console::WriteLine("MTTS: " + ms);
	//Console::WriteLine("Paused");
	//Console::ReadLine();

	startTime = Time::now();
	Console::WriteLine("Solving PMIT-G");
	this->problemType = LiKProblemType::G;
	sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_G.csv", false);
	this->SolveProblemGA(sw, 0.0);
	endTime = Time::now();
	duration = endTime - startTime;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	Console::WriteLine("MTTS: " + ms.count());
	Console::WriteLine();

	startTime = Time::now();
	Console::WriteLine("Solving PMIT-L");
	this->problemType = LiKProblemType::L;
	sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_L.csv", false);
	this->SolveProblemGA(sw, 0.0);
	endTime = Time::now();
	duration = endTime - startTime;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	Console::WriteLine("MTTS: " + ms.count());
	Console::WriteLine();

	startTime = Time::now();
	Console::WriteLine("Solving PMIT-G2L");
	this->problemType = LiKProblemType::G;
	sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_G.csv", false);
	SolutionType^ sol = this->SolveProblemGA(sw, 100.5);
	endTime = Time::now();
	duration = endTime - startTime;
	std::chrono::milliseconds ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	SolutionType^ converted = gcnew SolutionType();
	converted->solutions = gcnew array<array<Int32>^>(sol->solutions->GetUpperBound(0)+1);

	for (size_t i = 0; i <= sol->solutions->GetUpperBound(0); i++)
	{
		converted->solutions[i] = this->ConvertSolutionToRuleLengths(sol->solutions[i]);
	}

	startTime = Time::now();
	this->problemType = LiKProblemType::L;
	sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_L.csv", false);
	this->SolveProblemGA2(sw, 0.0, converted);
	endTime = Time::now();
	duration = endTime - startTime;
	std::chrono::milliseconds ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	Console::WriteLine("MTTS: " + (ms1.count() + ms2.count()));
	Console::WriteLine("Paused");
	Console::ReadLine();

	//startTime = Time::now();
	//Console::WriteLine("Solving PMIT-PoG");
	//this->problemType = LiKProblemType::PoG;
	//sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_PoG.csv", false);
	//this->SolveProblemGA(sw);
	//endTime = Time::now();
	//duration = endTime - startTime;
	//ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	//Console::WriteLine("MTTS: " + ms.count());
	//Console::WriteLine("Paused");
	//Console::ReadLine();

	//startTime = Time::now();
	//Console::WriteLine("Solving PMIT-PoL");
	//this->problemType = LiKProblemType::PoL;
	//sw = gcnew System::IO::StreamWriter("D://data//fitnesshistory_PoL.csv", false);
	//this->SolveProblemGA(sw);
	//endTime = Time::now();
	//duration = endTime - startTime;
	//ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	//Console::WriteLine("MTTS: " + ms.count());
	//Console::WriteLine("Paused");
	//Console::ReadLine();
}

SolutionType^ LiKProblem::SolveProblemGA(System::IO::StreamWriter^ SW, float Target)
{
	GeneticAlgorithmPMIT^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

	genomeConfig = this->CreateGenome();
	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
	configuration->populationSizeInit = 10;
	configuration->populationSizeMax = 10;
	configuration->crossoverWeight = 0.85;
	configuration->mutationWeight = 0.15;
	configuration->preferHighFitness = false;

	ai = gcnew GeneticAlgorithmPMIT(configuration);
	ai->problem = this;
	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(30000, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(Target));
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(250000));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;
	ai->sw = SW;

	SolutionType^ S = gcnew SolutionType();
	S->solutions = ai->SolveAllSolutions();
	S->fitnessValues = ai->FitnessValues();

	return S;
}

SolutionType^ LiKProblem::SolveProblemGA2(System::IO::StreamWriter^ SW, float Target, SolutionType^ Solution)
{
	GeneticAlgorithmPMIT^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

	genomeConfig = this->CreateGenome();
	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
	configuration->populationSizeInit = 10;
	configuration->populationSizeMax = 10;
	configuration->crossoverWeight = 0.85;
	configuration->mutationWeight = 0.15;
	configuration->preferHighFitness = false;

	ai = gcnew GeneticAlgorithmPMIT(configuration);
	ai->problem = this;
	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(30000, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(Target));
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(250000));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;
	ai->sw = SW;

	SolutionType^ S = gcnew SolutionType();
	ai->InitializePopulation(Solution->solutions);
	S->solutions = ai->SolveAllSolutions();
	S->fitnessValues = ai->FitnessValues();

	return S;
}
