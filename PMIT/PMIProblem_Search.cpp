#include "stdafx.h"
#include "PMIProblem_Search.h"


PMIProblem_Search::PMIProblem_Search(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2(Model, ErrorRate, FileName)
{
}


PMIProblem_Search::~PMIProblem_Search()
{
}

bool PMIProblem_Search::SolveProblemSet()
{
	Int32 attempts = 0;

	while ((!this->currentSet->IsSolved()) && (!this->currentSet->disabled))
	{
		attempts++;
		// TODO: Add a check here that prevents a search if it has been searched many times already
		// If there are no full solutions go try to find one
		this->currentProblem = this->currentSet->moduleProblem;
		if ((!this->currentProblem->solved) && (!this->currentProblem->unset) && (this->currentProblem->fullSolutions->Count == 0))
		{
			Int32 tabuCount;
			do
			{
				tabuCount = this->currentSet->moduleProblem->tabu->Count;
				this->currentProblem->solved = this->SolveModulesSearch();
			} while ((!this->currentProblem->solved) && (this->currentProblem->tabu->Count > tabuCount));
		}

		// There is a full solution to try
		if ((this->currentProblem->unset) || (this->currentProblem->fullSolutions->Count > 0) || (this->currentProblem->solved))
		{
			if ((!this->currentProblem->unset) && (this->currentProblem->fullSolutions->Count > 0))
			{
				// Pop the top solution from the list
				PMISolution^ s = this->currentProblem->fullSolutions[0];
				this->currentSet->moduleProblem->fullSolutions->RemoveAt(0);
				// Create a rule set based on the solution
				array<Int32, 2>^ rules = this->Assess_CreateParikhRulesStep_Modules(s->solution);
				// Update the RPT
				this->CapturePartialSolution(rules);
			}

			this->currentProblem = this->currentSet->constantProblem;
			this->currentProblem->solved = this->SolveConstantsSearch();

			if (this->currentProblem->solved)
			{
				this->currentProblem = this->currentSet->orderProblem;
				this->currentProblem->solved = this->SolveOrderScan();
			}
			//else
			//{
			//	this->currentSet->moduleProblem->tabu->Add(gcnew TabuItem(TabuItem::ModulesOnlyIndex, rules));
			//}

			if (this->currentProblem->solved)
			{// commit changes
				this->changes->RemoveRange(0, this->changes->Count);
			}
			else
			{
				this->RollbackPartialSolution();
				this->changes->RemoveRange(0, this->changes->Count);

				this->currentSet->moduleProblem->Reset();
				this->currentSet->constantProblem->Reset();
				this->currentSet->orderProblem->Reset();
				for (size_t iProblem = 0; iProblem < this->currentSet->otherProblems->Count; iProblem++)
				{
					this->currentSet->otherProblems[iProblem]->Reset();
				}
				this->UpdateSolvedProblems();
			}
		}
		else if (this->currentSet->moduleProblem->partialSolutions->Count > 0)
		{// No full solutions create a new set of sub-problems

			// Pop the top solution from the list
			PMISolution^ s = this->currentProblem->partialSolutions[0];

			// Create subproblems based on the partial solution
			// Mark the current problem set as disabled

			this->currentSet->disabled = true;
		}
		else
		{// There is no solution at all
			this->currentSet->disabled = true;
		}

		if ((attempts > 10) && (!this->currentSet->IsSolved()))
		{
			this->currentSet->disabled = true;
		}
	}

	return this->currentSet->IsSolved();
}

//
//void PMIProblem_Search::Solve_GetModuleSolutions()
//{
//	this->currentProblem = this->currentSet->moduleProblem;
//	this->SolveModulesSearch();
//}

//				while (iProblem < this->currentSet->problems->Count)
//				{
//					// get the current problem
//					this->currentProblem = this->currentSet->problems[iProblem];
//
//					if (!this->currentProblem->solved)
//					{
//						Console::WriteLine("Problem set #" + iSet + ": Solving Problem #" + iProblem + " for " + this->currentProblem->ToString());
//
//						if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Modules)
//						{
//							this->currentProblem->solved = this->SolveModulesSearch();
//						}
//						else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Constants)
//						{
//							this->currentProblem->solved = this->SolveConstantsSearch();
//						}
//						else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Order)
//						{
//							// Are there any variables that need solving?
//							if (this->equations->Count > 0)
//							{
//								this->currentProblem->solved = this->SolveOrderScan();
//							}
//							else
//							{
//								this->currentProblem->solved = this->SolveOrderScan();
//							}
//							//if (!this->currentProblem->solved)
//							//{
//							//	this->currentProblem->solved = this->SolveOrderSearch();
//							//}
//
//							if (this->currentProblem->solved)
//							{// commit changes
//								this->changes->RemoveRange(0, this->changes->Count);
//							}
//
//						}
//
//						// If not solved, then rollback the partial solutions and set the problems to be unsolved
//						if (!this->currentProblem->solved)
//						{
//							this->PartialSolution();
//							this->changes->RemoveRange(0, this->changes->Count);
//							iProblem = 0;
//							for (size_t iProblem = 0; iProblem < this->currentSet->problems->Count; iProblem++)
//							{
//								this->currentSet->problems[iProblem]->Reset();
//							}
//						}
//						else
//						{
//							iProblem++;
//#if _PMI_PROBLEM_VERBOSE_ >= 3
//							this->OutputRPT();
//#endif
//						}
//					}
//					else
//					{
//						iProblem++;
//					}
//				}
