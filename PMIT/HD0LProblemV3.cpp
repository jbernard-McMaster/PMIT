#include "stdafx.h"
#include "HD0LProblemV3.h"


HD0LProblemV3::HD0LProblemV3(ModelV3^ M, ProblemType T) : S0LMProblemV3(M, T)
{
}


HD0LProblemV3::~HD0LProblemV3()
{
}

void HD0LProblemV3::Initialize()
{
	// Set the absolute lowest length any successor may have
	// Currently assuming a propagating L-system
	if (this->problemType == LSIProblemV3::ProblemType::Master)
	{
		this->absoluteMinLength = 1;
	}
	else if (this->problemType == LSIProblemV3::ProblemType::Primary)
	{
		this->absoluteMinLength = 0;
	}
	else if (this->problemType == LSIProblemV3::ProblemType::Secondary)
	{
		this->absoluteMinLength = 0;
	}
	this->absoluteMaxLength = Math::Max(10, this->model->successorLongest);

	//if (this->model->successorShortest == 1)
	//{
	//	this->absoluteMinLength = 1;
	//}
	//else
	//{
	//	this->absoluteMinLength = 2;
	//}

	//if (this->model->successorLongest - this->model->successorShortest >= 10)
	//{
	//	this->absoluteMinLength = this->model->successorShortest;
	//	this->absoluteMaxLength = this->model->successorLongest;
	//}

	this->hasSubProblems = false;
	this->solution = gcnew ModelV3();

	this->MAO = gcnew MasterAnalysisObject(this->model);

	this->analysisTime = 0;
	this->solveTime = 0;
	this->solved = false;
	this->matched = false;
	this->lastSymbolPos = gcnew array<Int32>(this->model->generations - 1);

	// Establish the base facts
	// Cannot compress the fact table yet until some raw facts are found by analysis
	if (this->model->parameters->Count == 0)
	{
		for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
		{
			//this->model->evidence[iGen]->Display();
			for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
			{
				Int32 iSymbol = this->model->alphabet->FindIndex(this->model->evidence[iGen]->GetSymbol(iPos));

				if (!this->model->alphabet->IsTurtle(iSymbol))
				{
					this->lastSymbolPos[iGen] = iPos;
					Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
					Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

					Fact^ f = this->CreateBaseFact(iSymbol, iLeft, iRight);

#if _PHASE3_VERBOSE_ >= 1
					Console::WriteLine("Gen " + iGen + ": Adding fact for SAC " + this->MAO->SymbolCondition(iSymbol, iLeft, iRight));
#endif
					if (!this->model->contextFree)
					{
						this->MAO->AddFact(iGen, 0, iSymbol, iLeft, iRight, f);
					}
					else
					{
						this->MAO->CopyFact(iGen, 0, iSymbol, -1, -1, iLeft, iRight, f);
					}
				}
			}
		}
	}
	else
	{
		// if there are any parameters then ... not sure yet
	}

	this->MAO->localization = gcnew array<array<Byte, 2>^>(this->model->generations - 1);
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		this->MAO->localization[iGen] = gcnew array<Byte, 2>(this->model->evidence[iGen + 1]->Length(), this->model->evidence[iGen]->Length());
		for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen + 1]->Length(); iPos1++)
		{
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen]->Length(); iPos2++)
			{
				this->MAO->localization[iGen][iPos1, iPos2] = MasterAnalysisObject::cLocalization_Unset;
			}
		}
	}

	auto startTime = Time::now();

	this->ComputeLength_AbsoluteMinMax(); // These only need to be done once at initialization, it will never produce a better value afterwards.
	this->ComputeGrowth_AbsoluteMinMax();

	this->Analyze();

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->analysisTime = ms.count();

#if _PHASE3_VERBOSE_ >= 1
	this->MAO->Display();
	this->MAO->ComputeSolutionSpaceSize();
	this->MAO->DisplaySolutionSpaces();
#endif

	//Console::WriteLine("Paused");
	//Console::ReadLine();
}

void HD0LProblemV3::Analyze()
{
	// Create the fact overlay
	this->factOverlay = gcnew array<List<Fact^>^>(this->model->generations - 1);
	this->factCount = gcnew array<Int32>(this->MAO->factsList->Count);

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		this->factOverlay[iGen] = gcnew List<Fact^>;
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				Int32 fIndex = this->MAO->GetFactIndex(f);
				factCount[fIndex]++;
				Fact^ tmp = this->CreateBaseFact(iSymbol, this->model->evidence[iGen]->GetLeftContext(iPos), this->model->evidence[iGen]->GetRightContext(iPos));
				this->factOverlay[iGen]->Add(tmp);
			}
			else
			{
				this->factOverlay[iGen]->Add(this->MAO->GetFact(iGen, 0, iSymbol, this->model->evidence[iGen]->GetLeftContext(iPos), this->model->evidence[iGen]->GetRightContext(iPos), nullptr));
			}
		}
	}

	// Analyze the alphabet
	this->ComputeSymbolLexicons();
	Int32 pass = 0;
	this->firstPass = true;
	do
	{
		this->MAO->ResetChangeFlag();

		this->ComputeLength_Production_S0LOnly();

#if _PHASE3_VERBOSE_ >= 1
		Console::WriteLine("Analysis Pass Complete");
		this->MAO->Display();
#endif
		pass++;
		firstPass = false;
	} while (this->MAO->HasChangeOccured() && pass <= cPass_Minimum);
}

double HD0LProblemV3::Assess_Dbl_Old(array<Int32>^ RuleLengths)
{
	this->modelCurrent = this->model;
	double fitness = this->Assess_Dbl_L(RuleLengths);
	if (fitness > this->fitnessBest)
	{
		this->fitnessBest = fitness;
		this->successorsBest = this->successorsCurrent;
	}
	return fitness;
}

double HD0LProblemV3::Assess_Dbl_PoL_Old(array<Int32>^ RuleLengths)
{
	double fitness = 0;

	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->model->alphabet->numNonTurtles);

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	// Consume the genes for the first N symbols
	Int32 iPos = 0;
	Int32 iScan = 0;
	Int32 iGen = 0;
	Int32 iGene = 0;
	Int32 reserveRightMin = 0;
	Int32 reserveRightMax = 0;

	for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
	{
		reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
		reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
	}

	bool done = false;
	bool failed = false;
	Int32 iSymbol;
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		iSymbol = this->model->evidence[iGen]->symbolIndices[iPos];
		String^ successor = "";
		Int32 length = 0;
		if ((iPos < this->lastSymbolPos[iGen]) && (!this->model->alphabet->IsTurtle(iSymbol)))
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;

			if (RuleLengths[iGene] % 2 == 1)
			{
				successor = this->Assess_Dbl_PeekAheadStep_L(successors, iSymbol, iGen, iPos, iScan,0);
			}

			if (successor == "")
			{
				successor = this->Assess_Dbl_ConsumeGeneStep_PoL(RuleLengths, iGene, iGen, iPos, iScan, reserveRightMax, reserveRightMin);

				if ((iGene == this->aiBF->state->index) && (RuleLengths[iGene] % 2 == 1) && (!this->flagSkipSet))
				{
					this->aiBF->SetDelayedSkip(iGene, RuleLengths[iGene] + 2);
				}

				iGene++;
			}
			else
			{
				fitness++;
			}
		}
		else if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - (this->model->evidence[iGen]->Length() - this->lastSymbolPos[iGen]));
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Last symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") get successor " + successor);
#endif
		}
		else
		{
			successor = this->model->alphabet->GetSymbol(iSymbol);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				successors = this->AddSuccessor(iGen, iPos, successors, successor);
			}

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;

			iPos++;
			iScan += successor->Length;

			if (iPos >= this->model->evidence[iGen]->Length())
			{
				iGen++;
				iPos = 0;
				iScan = 0;

				if (iGen < this->model->generations - 1)
				{
					reserveRightMin = 0;
					reserveRightMax = 0;
					for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
					{
						reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
						reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
					}
				}
			}

			if (iGen >= this->model->generations - 1)
			{
				done = true;
			}
		}
		else
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("**FAILED**" + successor);
			Console::WriteLine();
#endif
		}
	}

	// move this to its own function

	bool softFail = false;
	while ((!done) && (!failed) && (!softFail))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		String^ successor = "";
		Int32 length = 0;
		iSymbol = this->model->evidence[iGen]->symbolIndices[iPos];
		if ((iPos < this->model->evidence[iGen]->Length() - 1) && (!this->model->alphabet->IsTurtle(iSymbol)))
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;

			while ((!found) && (iSuccessor < successors[iSymbol]->Count))
			{
				if (successors[iSymbol][iSuccessor]->successor == this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (successors[iSymbol][iSuccessor]->successor->Length - 1)))
				{
					found = true;
					successor = successors[iSymbol][iSuccessor]->successor;
					fitness++;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif
				}
				iSuccessor++;
			}

			if (!found)
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("For " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") no successor found");
#endif
				softFail = true;
			}
		}
		else if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}
		else
		{
			successor = this->model->alphabet->GetSymbol(iSymbol);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				successors = this->AddSuccessor(iGen, iPos, successors, successor);
			}

			iPos++;
			iScan += successor->Length;

			if (iPos >= this->model->evidence[iGen]->Length())
			{
				iGen++;
				iPos = 0;
				iScan = 0;
			}

			if (iGen >= this->model->generations - 1)
			{
				done = true;
			}
		}
	}

	Int32 t = 1;
	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		t += Math::Max(0, successors[i]->Count - 1);
	}

	fitness /= t;

	if (failed)
	{
		fitness /= 2;
	}

	// Compute the remaining successors using Greedy Algorithm
	//if (softFail)
	//{
	//	successors = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram, successors, iGen, iPos);
	//}
	// Compute the fitness

	fitness += this->ComputeSuccessorsOdds(successors);
#if _PHASE3_S0L_VERBOSE_ > 0
	if (fitness > this->fitnessBest)
	{
		this->fitnessBest = fitness;
		Console::Write("New Best: ");
		for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
		{
			Console::Write(RuleLengths[i] + " ");
		}
		Console::WriteLine(", F = " + fitness.ToString());

		for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
		{
			Int32 total = 0;
			for (size_t j = 0; j < successors[i]->Count; j++)
			{
				total += successors[i][j]->count;
			}

			for (size_t j = 0; j < successors[i]->Count; j++)
			{
				Console::WriteLine(this->model->alphabet->GetSymbol(i) + " -> " + successors[i][j]->successor->PadRight(11) + ", p=" + ((float)successors[i][j]->count / total).ToString(L"F6"));
			}
		}

		Console::WriteLine("Paused");
		Console::ReadLine();
	}
#endif

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::Write("Assessing: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}
	Console::WriteLine(fitness.ToString());

	Console::WriteLine("Paused");
	Console::ReadLine();
#endif

	//if (iGene < RuleLengths->GetUpperBound(0))
	//{
	//	this->aiBF->Skip(iGene);
	//}

	this->successorsCurrent = successors;

	return fitness;
}

GenomeConfiguration<Int32>^ HD0LProblemV3::CreateGenome_GreedyBF()
{
	this->maxSearchSpaceSize = 1;
	this->absolutePeekLimit = 15;
	this->absoluteRecursionLimit = 10;

	this->numGenes = this->model->alphabet->numNonTurtles + (this->model->alphabet->numNonTurtles * 0.75) + (this->model->alphabet->numNonTurtles * 0.5);
	this->numGenes = Math::Max(5, Math::Min(12, numGenes));

	this->numGenes = 0;
	for (size_t i = 0; i < this->model->productions[0]->rules->Count; i++)
	{
		this->numGenes += this->model->productions[0]->rules[i]->successor->Count;
	}

	this->numGenes = 2 * this->numGenes;

	//Int32 numGenes = this->model->alphabet->numNonTurtles * 3;

	Int32 iGene = 0;
	Int32 iGen = 0;
	Int32 iPos = 0;
	array<Int32>^ min = gcnew array<Int32>(this->numGenes);
	array<Int32>^ max = gcnew array<Int32>(this->numGenes);
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	while (iGene < this->numGenes)
	{
		if (iGene % 2 == 0)
		{
			min[iGene] = this->absoluteMinLength;
			max[iGene] = this->absoluteMaxLength;
			this->rule2solution->Add(iGene);
			this->solution2rule->Add(iGene);
			this->maxSearchSpaceSize *= (1 + max[iGene] - min[iGene]);
		}
		else if (iGene == 1)
		{
			min[iGene] = 0;
			max[iGene] = 0;
		}
		else
		{
			min[iGene] = -1;
			max[iGene] = this->absolutePeekLimit;
			this->maxSearchSpaceSize *= (2 + max[iGene]);
		}

		iGene++;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(this->numGenes, min, max);
	return result;
}

void HD0LProblemV3::Decode2(array<Int32>^ RuleLengths)
{
	// Compute the remaining successors using Greedy Algorithm
	double odds = this->ComputeSuccessorsOdds(this->successorsBest);

	// recompute best successors odds
	for (size_t iRule = 0; iRule <= this->successorsBest->GetUpperBound(0); iRule++)
	{
		Int32 total = 0;
		for (size_t iSuccessor = 0; iSuccessor < this->successorsBest[iRule]->Count; iSuccessor++)
		{
			total += this->successorsBest[iRule][iSuccessor]->count;
		}

		for (size_t iSuccessor = 0; iSuccessor < this->successorsBest[iRule]->Count; iSuccessor++)
		{
			this->successorsBest[iRule][iSuccessor]->odds = (double)this->successorsBest[iRule][iSuccessor]->count / total;
		}
	}

	Console::Write("Decoding: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}

	if (this->solvedFlag)
	{
		this->success = true;
	}

	//this->ComputeMetrics(this->successorsBest, odds);
	Console::WriteLine();
	for (size_t i = 0; i <= this->successorsBest->GetUpperBound(0); i++)
	{
		Int32 total = 0;
		for (size_t j = 0; j < this->successorsBest[i]->Count; j++)
		{
			total += this->successorsBest[i][j]->count;
		}

		for (size_t j = 0; j < this->successorsBest[i]->Count; j++)
		{
			Console::WriteLine(this->model->alphabet->GetSymbol(i) + " -> " + this->successorsBest[i][j]->successor->PadRight(11) + ", p=" + ((float)this->successorsBest[i][j]->count / total).ToString(L"F6"));
		}
	}

	Console::WriteLine();

#if _PHASE3_S0L_DECODE_ > 1
	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}