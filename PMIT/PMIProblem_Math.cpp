#include "stdafx.h"
#include "PMIProblem_Math.h"


PMIProblem_Math::PMIProblem_Math(PlantModel^ Model) : PMIProblem(Model)
{
}


PMIProblem_Math::~PMIProblem_Math()
{
}

bool PMIProblem_Math::SolveProblemSet()
{
	// get the current problem
	this->currentProblem = this->currentSet->moduleProblem;

	if (!this->currentProblem->solved)
	{
		this->currentProblem->solved = this->SolveModulesLinearAlgebra();

		if ((this->currentProblem->solved) && (this->currentProblem->vocabulary->numConstants > 0))
		{
			this->currentProblem = this->currentSet->constantProblem;
			this->currentProblem->solved = this->SolveConstantsLinearAlgebra();
		}

		if (this->currentProblem->solved)
		{
			this->currentProblem = this->currentSet->orderProblem;
			if (this->equations->Count == 0)
			{
				this->currentProblem->solved = this->SolveOrderScan();
			}
			else
			{
				//this->currentProblem->solved = this->SolveOrderScanWithVariables();
				this->currentProblem->solved = false;
			}
		}
	}

	return this->currentProblem->solved;
}

bool PMIProblem_Math::SolveModulesLinearAlgebra()
{
	bool result = true;

	array<float, 2>^ M = gcnew array<float, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
	array<float, 2>^ S = gcnew array<float, 2>(this->vocabulary->numModules, 1);
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules);

	for (size_t iCol = 0; iCol <= M->GetUpperBound(1); iCol++)
	{
		for (size_t iRow = 0; iRow <= M->GetUpperBound(0); iRow++)
		{
			// Cannot assume 1st generation
			if (this->rulesParikhTemplate[0, iRow, iCol] == PMIProblem::cUnknownParikhValue)
			{
				M[iRow, iCol] = this->evidence[iRow]->parikhVector[iCol];
			}
			else
			{
				M[iRow, iCol] = 0;
			}
		}
	}

	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	{
		for (size_t iRow = 0; iRow <= S->GetUpperBound(0); iRow++)
		{
			S[iRow, 0] = this->evidence[iRow + 1]->parikhVector[iSymbol];
		}

		MatrixMath^ calc = gcnew MatrixMath();
		array<float, 2>^ solution = calc->GaussJordanElimination(M, S);

		// Copy values from solution into the rules
		for (size_t iRow = 0; iRow <= solution->GetUpperBound(0); iRow++)
		{
			ParikhEquation^ eq = gcnew ParikhEquation();
			eq->value = round(solution[iRow, solution->GetUpperBound(1)]);

			for (size_t iCol = 0; iCol <= solution->GetUpperBound(1) - 1; iCol++)
			{
				if ((iCol == iRow) && (round(solution[iRow, iCol]) == 1))
				{
					rules[iCol, iSymbol] = round(solution[iRow, solution->GetUpperBound(1)]);
					eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, 1));
				}
				else if (round(solution[iRow, iCol]) > 0)
				{
					rules[iCol, iSymbol] = PMIProblem::cVariableParikhValue;
					eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, round(solution[iRow, iCol])));
				}
			}

			if (eq->variables->Count > 1)
			{
				this->equations->Add(eq);
			}
		}

		Console::Write(this->vocabulary->Symbols[iSymbol]->ToString() + ": ");
		for (size_t iRule = 0; iRule <= solution->GetUpperBound(0); iRule++)
		{
			if (rules[iRule, iSymbol] != PMIProblem::cVariableParikhValue)
			{
				Console::Write(rules[iRule, iSymbol] + " ");
			}
			else
			{
				Console::Write("* ");
			}
		}
		Console::WriteLine();
	}

	//if (this->VerifyOrderScan_Modules(rules))
	//{
	this->CapturePartialSolution(rules);
	result = true;
	//}
	//else
	//{
	//	result = false;
	//}

	return result;
}

bool PMIProblem_Math::SolveConstantsLinearAlgebra()
{
	bool result = true;

	if (this->vocabulary->numConstants > 0)
	{
		array<float, 2>^ M = gcnew array<float, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
		array<float, 2>^ S = gcnew array<float, 2>(this->vocabulary->numModules, 1);
		array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + 1);
		MatrixMath^ calc = gcnew MatrixMath();

		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol];
			}
		}

		// For each constant
		for (size_t iSymbol = this->vocabulary->IndexConstantsStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
		{
			// Create the matrix representing the linear equations
			for (size_t iCol = 0; iCol <= M->GetUpperBound(1); iCol++)
			{
				for (size_t iRow = 0; iRow <= M->GetUpperBound(0); iRow++)
				{
					// Cannot assume 1st generation
					if (!this->IsModuleSolvedForSymbol(iCol, iSymbol))
					{
						M[iRow, iCol] = this->evidence[iRow]->parikhVector[iCol];
					}
					else
					{
						M[iRow, iCol] = 0;
					}
				}
			}


			// Create the solution matrix, i.e. the number of symbols which exist in each generation
			for (size_t iRow = 0; iRow <= S->GetUpperBound(0); iRow++)
			{
				S[iRow, 0] = this->evidence[iRow + 1]->parikhVector[iSymbol] - this->evidence[iRow]->parikhVector[iSymbol];
			}

			S = calc->RemoveEmptyRows(M, S);
			M = calc->RemoveEmptyRows(M);

			// Find a solution to the linear equations
			array<float, 2>^ solution = calc->GaussJordanElimination(M, S);

			// Copy values from solution into the rules
			for (size_t iRow = 0; iRow <= solution->GetUpperBound(0); iRow++)
			{
				ParikhEquation^ eq = gcnew ParikhEquation();
				eq->value = round(solution[iRow, solution->GetUpperBound(1)]);

				for (size_t iCol = 0; iCol <= solution->GetUpperBound(1) - 1; iCol++)
				{
					if ((iCol == iRow) && (round(solution[iRow, iCol]) == 1))
					{
						//rules[iRule, rules->GetUpperBound(1)] = round(solution[iRule, 0]); // in the rules matrix the last column is always the constants column
						rules[iCol, rules->GetUpperBound(1)] = round(solution[iRow, solution->GetUpperBound(1)]);
						eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, 1));
					}
					else if (round(solution[iRow, iCol]) > 0)
					{
						rules[iCol, rules->GetUpperBound(1)] = PMIProblem::cVariableParikhValue;
						eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, round(solution[iRow, iCol])));
					}
				}

				if (eq->variables->Count > 1)
				{
					this->equations->Add(eq);
				}
			}

			this->CapturePartialSolution(rules);

			Console::Write(this->vocabulary->Symbols[iSymbol]->ToString() + ": ");
			for (size_t iRule = 0; iRule <= solution->GetUpperBound(0); iRule++)
			{
				if (rules[iRule, rules->GetUpperBound(1)] != PMIProblem::cVariableParikhValue)
				{
					Console::Write(rules[iRule, rules->GetUpperBound(1)] + " ");
				}
				else
				{
					Console::Write("* ");
				}
			}
			Console::WriteLine();
			this->currentProblem->constantIndex++;
		}
	}

	return result;
}

/* PMIT-M VERSION 2 */

PMIProblem_Mathv2::PMIProblem_Mathv2(PlantModel^ Model) : PMIProblem_Math(Model)
{
}

PMIProblem_Mathv2::~PMIProblem_Mathv2()
{
}

bool PMIProblem_Mathv2::SolveProblemSet()
{
	// get the current problem
	this->currentProblem = this->currentSet->otherProblems[0];

	if (!this->currentProblem->solved)
	{
		this->currentProblem->solved = this->SolveLengthLinearAlgebra();
	}

	return this->currentProblem->solved;
}

bool PMIProblem_Mathv2::SolveLengthLinearAlgebra()
{
	bool result = true;

	//array<float, 2>^ M = gcnew array<float, 2>(this->vocabulary->numModules, this->vocabulary->numModules);
	//array<float, 2>^ S = gcnew array<float, 2>(this->vocabulary->numModules, 1);
	//array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules);

	//for (size_t iCol = 0; iCol <= M->GetUpperBound(1); iCol++)
	//{
	//	for (size_t iRow = 0; iRow <= M->GetUpperBound(0); iRow++)
	//	{
	//		if (this->rulesParikhTemplate[0, iRow, iCol] == PMIProblem::cUnknownParikhValue)
	//		{
	//			M[iRow, iCol] = this->evidence[iRow]->parikhVector[iCol];
	//		}
	//		else
	//		{
	//			M[iRow, iCol] = 0;
	//		}
	//	}
	//}

	//	for (size_t iRow = 0; iRow <= S->GetUpperBound(0); iRow++)
	//	{
	//		Int32 length = 0;
	//		for (size_t iSymbol = 0; iSymbol <= this->evidence[iRow]->parikhVector->GetUpperBound(0); iSymbol++)
	//		{
	//			length += this->evidence[iRow]->parikhVector[iSymbol];
	//		}
	//		S[iRow, 0] = length;
	//	}

	//	MatrixMath^ calc = gcnew MatrixMath();
	//	array<float, 2>^ solution = calc->GaussJordanElimination(M, S);

	//	// Copy values from solution into the rules
	//	for (size_t iRow = 0; iRow <= solution->GetUpperBound(0); iRow++)
	//	{
	//		ParikhEquation^ eq = gcnew ParikhEquation();
	//		eq->value = round(solution[iRow, solution->GetUpperBound(1)]);

	//		for (size_t iCol = 0; iCol <= solution->GetUpperBound(1) - 1; iCol++)
	//		{
	//			if ((iCol == iRow) && (round(solution[iRow, iCol]) == 1))
	//			{
	//				rules[iCol, iSymbol] = round(solution[iRow, solution->GetUpperBound(1)]);
	//				eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, 1));
	//			}
	//			else if (round(solution[iRow, iCol]) > 0)
	//			{
	//				rules[iCol, iSymbol] = PMIProblem::cVariableParikhValue;
	//				eq->variables->Add(gcnew RPTCoordinate(this->currentProblem->startGeneration, this->currentProblem->endGeneration, iCol, iSymbol, round(solution[iRow, iCol])));
	//			}
	//		}

	//		if (eq->variables->Count > 1)
	//		{
	//			this->equations->Add(eq);
	//		}
	//	}

	//	Console::Write(this->vocabulary->Symbols[iSymbol]->ToString() + ": ");
	//	for (size_t iRule = 0; iRule <= solution->GetUpperBound(0); iRule++)
	//	{
	//		if (rules[iRule, iSymbol] != PMIProblem::cVariableParikhValue)
	//		{
	//			Console::Write(rules[iRule, iSymbol] + " ");
	//		}
	//		else
	//		{
	//			Console::Write("* ");
	//		}
	//	}
	//	Console::WriteLine();
	//}

	//array<Int32>^ solution = gcnew array<Int32>(this->vocabulary->numModules);

	//List<ProductionRule^>^ modelRules = this->CreateRulesByScanning(solution);
	//result = true;

	return result;
}