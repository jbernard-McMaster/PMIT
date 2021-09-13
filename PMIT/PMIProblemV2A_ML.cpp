#include "stdafx.h"
#include "PMIProblemV2A_ML.h"


PMIProblemV2A_ML::PMIProblemV2A_ML(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Rule(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_ML::~PMIProblemV2A_ML()
{
}

void PMIProblemV2A_ML::AddTabuItem_Equation(TabuItem^ T)
{
	if (T != nullptr)
	{
#if _PMI_PROBLEM_DISPLAY_TABU_ >= 1
		Console::WriteLine("Adding Tabu Item");
		this->OutputRules(T->rules);
#if _PMI_PROBLEM_BREAKS_ >= 1
		Console::ReadLine();
#endif
#endif
		T->indexSecondary = this->constantIndex;
		if (!this->IsTabu_Equation(T->solution))
		{
			this->tabu->Add(T);
		}
	}
}

float PMIProblemV2A_ML::Assess(array<Int32>^ Solution)
{
	if (this->equationIndex != -1)
	{
		return this->AssessLinearEquation(Solution);
	}
	else
	{
		if (Solution != nullptr)
		{
			//return this->AssessRuleLengths(this->Assess_CreateRuleLengths(Solution));
		}
		else // this gets called when there is no solution to assess but rather when all the rule lengths are known
		{
			return this->AssessRuleLengths(Solution);
		}
	}
}

float PMIProblemV2A_ML::AssessLinearEquation(array<Int32>^ Solution)
{
	float value = 0.0;
	float result = 0.0;
	Int32 idx = 0;

	// TODO: need to check to make sure solution isn't tabu
	if (!this->IsTabu_Equation(Solution))
	{
		for (size_t iVariable = 0; iVariable <= this->solutionMatrix->GetUpperBound(0); iVariable++)
		{
			if (this->solutionMatrix[this->equationIndex, iVariable] != 0)
			{
				float tmp = Math::Round(Solution[idx] * this->solutionMatrix[this->equationIndex, iVariable],3);
				value += tmp;
				idx++;
			}
		}
		result = Math::Abs(value - this->solutionMatrix[this->equationIndex, this->solutionMatrix->GetUpperBound(1)]);
		if (result < 0.09)
		{
			result = 0;
		}
		else if (this->constantIndex == 6 && result < 0.13)
		{
			result = 0;
		}
		//Console::WriteLine("result = " + result.ToString(L"F4"));
	}
	else
	{
		result = 99999;
	}

	return result;
}


GenomeConfiguration<Int32>^ PMIProblemV2A_ML::CreateGenome()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	for (size_t iRule = 0; iRule < this->solutionMatrix->GetUpperBound(1); iRule++)
	{
		this->rule2solution->Add(-1);
		this->solution2rule->Add(-1);

		if (this->solutionMatrix[this->equationIndex, iRule] != 0)
		{
			this->rule2solution[iRule] = numGenes;
			this->solution2rule[numGenes] = iRule;
			numGenes++;
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t iGene = 0; iGene < min->Length; iGene++)
	{
		Int32 iRule = this->solution2rule[iGene];
		min[iGene] = this->minRuleLengths[iRule];
		max[iGene] = this->maxRuleLengths[iRule];
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

// TODO: this is overly aggressive. It needs take into consideration the ruleLengths found so far
// For example, if the Solution is 0,0,0,0,8, but the length so far are 2,15,?,?,8 then need to also compared the 2 and 15
bool PMIProblemV2A_ML::IsTabu_Equation(array<Int32>^ Solution)
{
	bool result = false;
	Int32 iTabu = 0;

	while ((!result) && (iTabu < this->tabu->Count))
	{
		if ((this->tabu[iTabu]->index == 100 + this->equationIndex) && (this->tabu[iTabu]->indexSecondary == this->constantIndex))
		{
			bool isMatch = true;

			for (size_t iRule = 0; iRule <= this->solutionInProgress->GetUpperBound(0); iRule++)
			{
				if (this->solutionInProgress[iRule] != -1)
				{
					isMatch = isMatch && (this->tabu[iTabu]->ruleLengths[iRule] == this->solutionInProgress[iRule]);
				}
				else
				{
					if (this->rule2solution[iRule] != -1)
					{
						isMatch = isMatch && (this->tabu[iTabu]->ruleLengths[iRule] == Solution[this->rule2solution[iRule]]);
					}
				}

			}

			//for (size_t iRule = 0; iRule <= Solution->GetUpperBound(0); iRule++)
			//{
			//	// TODO: really not sure this is correct
			//	isMatch = isMatch && (this->tabu[iTabu]->ruleLengths[this->solution2rule[iRule]] == Solution[iRule]);
			//}

			result = isMatch;
		}

		iTabu++;
	}

	return result;
}

bool PMIProblemV2A_ML::ModifySolutionMatrix(PMISolution^ s)
{
	bool result = true;
	// copy the solution matrix over to a temporary just in case the solution is bad
	array<float, 2>^ tmp = gcnew array<float,2>(this->solutionMatrix->GetUpperBound(0)+1, this->solutionMatrix->GetUpperBound(1) + 1);
	Array::Copy(this->solutionMatrix, tmp, this->solutionMatrix->Length);

	for (size_t iRow = 0; iRow <= tmp->GetUpperBound(0); iRow++)
	{
		bool allZero = true;
		bool allPositive = true;
		for (size_t iCol = 0; iCol < tmp->GetUpperBound(1); iCol++)
		{
			allZero = allZero && (tmp[iRow, iCol] == 0);
			allPositive = allPositive && (tmp[iRow, iCol] > 0);
			if (this->rule2solution[iCol] != -1)
			{
				tmp[iRow, tmp->GetUpperBound(1)] -= tmp[iRow, iCol] * s->solution[this->rule2solution[iCol]];
				tmp[iRow, iCol] = 0;
			}

		}

		if ((!allZero && tmp[iRow, tmp->GetUpperBound(1)] != 0) && (allPositive && tmp[iRow, tmp->GetUpperBound(1)] < 0))
		{
			result = false;
		}
	}

	// if the solution is valid, finalize modifying the solution matrix
	if (result)
	{
		Array::Copy(tmp, this->solutionMatrix, this->solutionMatrix->Length);
	}

	return result;
}

bool PMIProblemV2A_ML::SolveProblem()
{
	bool result = false;

	array<float, 2>^ M0;
	array<float, 2>^ M;
	array<float, 2>^ S0;
	array<float, 2>^ S;
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
	float fitness;
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	SolutionType^ equationSolution;
	bool success;
	Int32 lastEquationIndex;
	bool failed = false;
	bool noSolution = false;

	M0 = gcnew array<float, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
	S0 = gcnew array<float, 2>(this->vocabulary->numModules, 1);
	Word^ E0;
	Word^ E1;

	for (size_t iRow = 0; iRow <= M0->GetUpperBound(0); iRow++)
	{
		if (iRow >= this->evidence->Count-1)
		{
			E1 = EvidenceFactory::Process(E0, this->model->rules);
			this->vocabulary->CompressSymbols(E1);
		}
		else
		{
			E0 = this->evidence[iRow];
			E1 = this->evidence[iRow+1];
		}

		for (size_t iCol = 0; iCol <= M0->GetUpperBound(1); iCol++)
		{
			M0[iRow, iCol] = E0->parikhVector[iCol];
		}

		array<Int32>^ PV = this->ComputeProduction(E0, E1);
		for (size_t i = 0; i <= PV->GetUpperBound(0); i++)
		{
			S0[iRow, 0] += PV[i];
		}

		E0 = E1;
	}

	do
	{
		failed = false;
		M = gcnew array<float, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
		S = gcnew array<float, 2>(this->vocabulary->numModules, 1);

		Array::Copy(M0, M, M0->Length);
		Array::Copy(S0, S, S0->Length);

		MatrixMath^ calc = gcnew MatrixMath();
		this->solutionMatrix = calc->GaussJordanElimination(M, S);
		this->solutionInProgress = gcnew array<Int32>(this->vocabulary->numModules);

		for (size_t i = 0; i <= this->solutionInProgress->GetUpperBound(0); i++)
		{
			this->solutionInProgress[i] = -1;
		}

		// TODO: Re-assess min/max length values based on matrix

		// Find the rule lengths that solve the matrix
		//if (this->currentTabu != nullptr)
		//{
		//	this->currentTabu->solution = gcnew array<Int32>(this->vocabulary->numModules);
		//}

#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
		Console::WriteLine();
		Console::WriteLine("Equations");
		Console::WriteLine("*********");
#endif
		for (size_t iEquation = 0; iEquation <= this->solutionMatrix->GetUpperBound(0); iEquation++)
		{
			this->equationIndex = iEquation;
			Int32 count = 0;

			for (size_t iVariable = 0; iVariable < this->solutionMatrix->GetUpperBound(1); iVariable++)
			{
#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
				if (this->solutionMatrix[iEquation, iVariable] >= 0)
				{
					Console::Write(" + ");
				}
				else
				{
					Console::Write(" - ");
				}

				Console::Write(Math::Abs(this->solutionMatrix[iEquation, iVariable]).ToString(L"F2") + " * L(" + this->vocabulary->Symbols[iVariable]->Value() + ")");
#endif
				if (this->solutionMatrix[iEquation, iVariable] != 0)
				{
					count++;
				}
			}

#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
			Console::Write(" = " + this->solutionMatrix[iEquation, this->solutionMatrix->GetUpperBound(1)] + "   ");
#endif

			// Solve the equation
			if (count > 1)
			{
				lastEquationIndex = iEquation;
#if _PMI_PROBLEM_SEARCH_GA_ > 0
				equationSolution = this->SolveProblemGA();
#else
				equationSolution = this->SolveProblemBF();
#endif

				PMISolution^ s;
				Int32 iSolution = 0;
				success = false;

				if (this->IsTabu_Equation(equationSolution->solutions[0]))
				{
					noSolution = true;
				}

				if (equationSolution->fitnessValues[0] == 0)
				{
					do
					{
						s = gcnew PMISolution(equationSolution->solutions[iSolution], equationSolution->fitnessValues[iSolution]);
						success = this->ModifySolutionMatrix(s);
						iSolution++;
					} while ((!success) && (iSolution <= equationSolution->solutions->GetUpperBound(0)) && (equationSolution->fitnessValues[iSolution] == 0));
				}
				else
				{
					Int32 idx = Math::Min(100, 100 + this->equationIndex - 1);

					TabuItem^ t = gcnew TabuItem(idx);
					t->ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);
					t->solution = gcnew array<Int32>(this->vocabulary->numModules);
					for (size_t i = 0; i <= this->solutionInProgress->GetUpperBound(0); i++)
					{
						t->ruleLengths[i] = this->solutionInProgress[i];
						t->solution[i] = this->solutionInProgress[i];
					}
					this->AddTabuItem_Equation(t);
				}

				// Capture the solution to the ruleLengths
				if (success)
				{
#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
					for (size_t i = 0; i <= s->solution->GetUpperBound(0); i++)
					{
						Console::Write(s->solution[i] + ",");
		}
#endif

					for (size_t iVariable = 0; iVariable <= s->solution->GetUpperBound(0); iVariable++)
					{
						ruleLengths[this->solution2rule[iVariable]] = s->solution[iVariable];
						solutionInProgress[this->solution2rule[iVariable]] = s->solution[iVariable];
					}
				}
				else
				{
					failed = true;
					Int32 idx = Math::Min(100, 100 + this->equationIndex);

					TabuItem^ t = gcnew TabuItem(idx);
					t->ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);
					t->solution = gcnew array<Int32>(this->vocabulary->numModules);
					for (size_t i = 0; i <= equationSolution->solutions[0]->GetUpperBound(0); i++)
					{
						t->ruleLengths[this->solution2rule[i]] = equationSolution->solutions[0][i];
						t->solution[this->solution2rule[i]] = equationSolution->solutions[0][i];
					}
					this->AddTabuItem_Equation(t);
					break;
				}
			}
			else if (count == 1)
			{
#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
				Console::Write(this->solutionMatrix[iEquation, this->solutionMatrix->GetUpperBound(1)]);
#endif
				Int32 l = 0;
				for (size_t i = 0; i < this->solutionMatrix->GetUpperBound(1); i++)
				{
					if (this->solutionMatrix[iEquation, i] != 0)
					{
						l = Math::Round(this->solutionMatrix[iEquation, this->solutionMatrix->GetUpperBound(1)] / this->solutionMatrix[iEquation, i]);
					}
				}

				ruleLengths[iEquation] = l;
				solutionInProgress[iEquation] = ruleLengths[iEquation];
			}

#if _PMI_PROBLEM_DISPLAY_EQUATIONS_ > 0
			Console::WriteLine();
#endif
		}

		if (!failed)
		{
			this->equationIndex = -1; // indicate that now any assessment should be on the stored rule lengths

									  // set the rule2solution, it is trivially direct 0=>0, 1=>1, etc.
			this->rule2solution = gcnew List<Int32>;
			this->solution2rule = gcnew List<Int32>;

			for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
			{
				this->rule2solution->Add(iRule);
				this->solution2rule->Add(iRule);
			}

			// Analyze the ruleLengths
			List<ProductionRule^>^ R = this->Assess_CreateRulesStep_RuleLengths(ruleLengths);
			if (R != nullptr)
			{
				fitness = this->AssessProductionRules(R);

				if (fitness != 0)
				{
					TabuItem^ t = gcnew TabuItem(100 + lastEquationIndex);
					t->ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);
					t->solution = gcnew array<Int32>(this->vocabulary->numModules);

					for (size_t i = 0; i <= ruleLengths->GetUpperBound(0); i++)
					{
						t->ruleLengths[i] = ruleLengths[i];
						t->solution[i] = ruleLengths[i];
					}
					this->AddTabuItem_Equation(t);
				}
			}
			else
			{
				fitness = 1.0;

				TabuItem^ t = gcnew TabuItem(100 + lastEquationIndex);
				t->ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);
				t->solution = gcnew array<Int32>(this->vocabulary->numModules);

				for (size_t i = 0; i <= ruleLengths->GetUpperBound(0); i++)
				{
					t->ruleLengths[i] = ruleLengths[i];
					t->solution[i] = ruleLengths[i];
				}
				this->AddTabuItem_Equation(t);
			}
		}
	} while ((failed && !noSolution) || (!noSolution && (fitness != 0) && (equationSolution->fitnessValues[0] == 0)));

	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		this->rule2solution->Add(iRule);
		this->solution2rule->Add(iRule);
	}

	PMISolution^ s = gcnew PMISolution(ruleLengths, fitness);
	result = this->AnalyzeSolution(s);

	if (!result)
	{
		this->RollbackProblemSet();
	}

	return result;
}