#include "stdafx.h"
#include "S0LProblemV3.h"


S0LProblemV3::S0LProblemV3(ModelV3^ M, ProblemType T) : LSIProblemV3(M,T)
{
}

void S0LProblemV3::Initialize()
{
	// Set the absolute lowest length any successor may have
	// Currently assuming a propagating L-system
	if (this->problemType == LSIProblemV3::ProblemType::Master)
	{
		this->absoluteMinLength = 1;
	}
	//else if (this->problemType == LSIProblemV3::ProblemType::Generation)
	//{
	//	this->absoluteMinLength = 1;
	//}
	else if (this->problemType == LSIProblemV3::ProblemType::Primary)
	{
		this->absoluteMinLength = 0;
	}
	else if (this->problemType == LSIProblemV3::ProblemType::Secondary)
	{
		this->absoluteMinLength = 0;
	}

	this->hasSubProblems = false;
	this->solution = gcnew ModelV3();

	//this->model->ComputeGenerations();

	this->MAO = gcnew MasterAnalysisObject(this->model);
	//this->solutionSpaceSizes = gcnew List<UInt64>;

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

void S0LProblemV3::Analyze()
{
	this->absoluteMaxLength = 10;
	this->maxGreedySteps = 3;
	this->preferredLength = 6;

	// Create the fact overlay
	this->factOverlay = gcnew array<List<Fact^>^>(this->model->generations - 1);
	this->factCount = gcnew array<Int32>(this->MAO->factsList->Count);

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
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

	this->avgLength = 0;

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		avgLength += this->model->evidence[iGen + 1]->Length() / this->model->evidence[iGen]->Length();
	}

	this->avgLength /= this->model->generations - 1;
	this->lengthValues = gcnew array<Int32>(this->absoluteMaxLength + 1);

	for (Int16 iLength = 1; iLength <= this->absoluteMaxLength; iLength++)
	{
		this->lengthValues[iLength] = this->absoluteMaxLength - Math::Abs(iLength - this->preferredLength);
	}

	// Analyze the alphabet
	this->ComputeSymbolLexicons();
	Int32 pass = 0;
	this->firstPass = true;
	do
	{
		this->MAO->ResetChangeFlag();

		this->ComputeLength_Production_S0LOnly();
		this->histogram = this->ComputeSuccessorHistogram();

#if _PHASE3_VERBOSE_ >= 1
		Console::WriteLine("Analysis Pass Complete");
		this->MAO->Display();
#endif
		pass++;
		firstPass = false;
	} while (this->MAO->HasChangeOccured() && pass <= cPass_Minimum);
}

array<List<SuccessorCount^>^>^ S0LProblemV3::AddSuccessor(Int32 iGen, Int32 iPos, array<List<SuccessorCount^>^>^ Successors, String^ S)
{
	Fact^ f = this->MAO->GetFact(iGen, this->modelCurrent->evidence[iGen], iPos);

	if (f->ID != -1)
	{
		Int32 iFact = this->MAO->GetFactIndex(f);

		// Add the choosen successor to the successor list
		bool found = false;

		for (size_t i = 0; i < Successors[f->sac->iSymbol]->Count; i++)
		{
			if (Successors[f->sac->iSymbol][i]->successor == S)
			{
				Successors[f->sac->iSymbol][i]->count++;
				found = true;
#if _PHASE3_S0L_VERBOSE_ > 1
				Console::WriteLine("For symbol " + this->model->alphabet->GetSymbol(f->sac->iSymbol) + " -> " + S + " count = " + Successors[f->sac->iSymbol][i]->count);
#endif
		}
	}

		if (!found)
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Adding to symbol " + this->model->alphabet->GetSymbol(f->sac->iSymbol) + " -> " + S);
#endif
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = S;
			tmp->count = 1;
			tmp->odds = 1;
			Successors[f->sac->iSymbol]->Add(tmp);
		}
	}

	return Successors;
}

String^ S0LProblemV3::Assess_Dbl_ConsumeGeneStep_PoL(array<Int32>^ RuleLengths, Int32 iGene, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin)
{
	String^ successor = "";
	Int32 absMinLength = Math::Max(this->factOverlay[iGen][iPos]->length->min, ((this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin);
	float fraction;
	Int32 length;
	Int32 V = RuleLengths[iGene];
	this->flagSkipSet = false;

	if (iGene == this->aiBF->state->index)
	{
		do
		{
			fraction = (float)V / 100;
			length = absMinLength + (fraction * (absMaxLength - absMinLength));
			V++;
		} while ((length == this->lastLength));

		this->lastLength = length;
		//this->aiBF->Skip(iGene, V-1);
		//this->flagSkipSet = true;
	}
	else
	{
		fraction = (float)V / 100;
		length = absMinLength + (fraction * (absMaxLength - absMinLength));
	}

	if (length > 0)
	{
		successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (length - 1));
	}
	else
	{
		successor = "";
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine("By value " + RuleLengths[iGene] + " for " + this->model->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif

	return successor;
}

String^ S0LProblemV3::Assess_Dbl_ConsumeGeneStep_PoL_Old(array<Int32>^ RuleLengths, Int32 iGene, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin)
{
	String^ successor = "";
	Int32 absMinLength = Math::Max(this->factOverlay[iGen][iPos]->length->min, ((this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin);
	float fraction;
	Int32 length;
	Int32 V = Math::Ceiling((float)RuleLengths[iGene] / 2);
	this->flagSkipSet = false;

	if ((RuleLengths[iGene] % 2 == 1) && (iGene == this->aiBF->state->index))
	{
		do
		{
			fraction = (float)V / 100;
			length = absMinLength + (fraction * (absMaxLength - absMinLength));
			V++;
		} while ((length == this->lastLength));

		this->lastLength = length;
		this->aiBF->SetDelayedSkip(iGene, (V*2)-1);
		this->flagSkipSet = true;
	}
	else
	{
		fraction = (float)V / 100;
		length = absMinLength + (fraction * (absMaxLength - absMinLength));
	}

	if (length > 0)
	{
		successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (length - 1));
	}
	else
	{
		successor = "";
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine("By value " + RuleLengths[iGene] + " for " + this->model->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif

	return successor;
}

String^ S0LProblemV3::Assess_Dbl_ConsumeGeneStep_PoL2(Int32 V, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin)
{
	String^ successor = "";
	Int32 absMinLength = Math::Max(this->factOverlay[iGen][iPos]->length->min, ((this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin);
	//float fraction = RuleLengths[iGene] * ((float)this->factOverlay[iGen][iPos]->length->min / this->factOverlay[iGen][iPos]->length->max);
	float fraction = V * ((float) absMinLength / absMaxLength);
	Int32 length = fraction * (absMaxLength - absMinLength);
	length = Math::Max(length, 1);
	if (length > 0)
	{
		successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (length - 1));
	}
	else
	{
		successor = "";
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine("By value " + V + " for " + this->model->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif

	return successor;
}

String^ S0LProblemV3::Assess_Dbl_PeekAheadStep_L(array<List<SuccessorCount^>^>^ Successors, Int32 iSymbol, Int32 iGen, Int32 iPos, Int32 iScan, Int32 PeekCount)
{
	if (this->modelCurrent->alphabet->IsTurtle(iSymbol))
	{
		return "";
	}
	else
	{
		this->peekOverrideFlag = false;
		String^ successor = "";
		List<Int32>^ candidates = gcnew List<Int32>;

		for (size_t iSuccessor = 0; iSuccessor < Successors[iSymbol]->Count; iSuccessor++)
		{
			if (iPos < this->modelCurrent->evidence[iGen]->Length() - 1)
			{
				if (((iScan + (Successors[iSymbol][iSuccessor]->successor->Length - 1)) < this->modelCurrent->evidence[iGen + 1]->Length()) && (Successors[iSymbol][iSuccessor]->successor == this->modelCurrent->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (Successors[iSymbol][iSuccessor]->successor->Length - 1))))
				{
					candidates->Add(iSuccessor);
				}
			}
			else
			{
				if ((iScan < this->modelCurrent->evidence[iGen + 1]->Length() - 1) && (Successors[iSymbol][iSuccessor]->successor == this->modelCurrent->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->modelCurrent->evidence[iGen + 1]->Length() - 1)))
				{
					candidates->Add(iSuccessor);
				}
			}
		}

		if (candidates->Count == 0)
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("By peeking for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") no candidates found");
#endif
			return "";
		}
		else if (candidates->Count == 1)
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("By peeking for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") one candidate found " + Successors[iSymbol][candidates[0]]->successor);
#endif
			return Successors[iSymbol][candidates[0]]->successor;
		}
		else if ((iPos < this->pathEnd) && (this->longestPath[iPos] >= 0))
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("By peeking for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") candidate found on precomputed path " + Successors[iSymbol][longestPath[iPos]]->successor);
#endif
			this->peekOverrideFlag = true;
			return Successors[iSymbol][this->longestPath[iPos]]->successor;
		}
		else if (PeekCount >= this->absolutePeekLimit)
		{
			return "";
			}
		else
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("By peeking for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") multiple candidates found");
#endif

			Int32 longest = -1;
			Int32 choice = 0;
			for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("Computing path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") assessing candidate " + Successors[iSymbol][candidates[iCandidate]]->successor);
#endif
				Int32 length = PeekCount + 1;
				Int32 tmp = 1 + this->Assess_Dbl_PeekAheadStepWithRecursion_L(Successors, this->modelCurrent->evidence[iGen]->symbolIndices[iPos + 1], iGen, iPos + 1, iScan + Successors[iSymbol][candidates[iCandidate]]->successor->Length, length, 0);

				if (tmp > longest)
				{
					longest = tmp;
					choice = iCandidate;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("Computing path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") candidate " + Successors[iSymbol][candidates[iCandidate]]->successor + " has longest path at " + longest);
#endif
				}
				else if (tmp == longest)
				{
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("Computing path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") candidate " + Successors[iSymbol][candidates[iCandidate]]->successor + " has equal path at " + longest);
#endif
					choice = -1;
				}

				if (tmp >= this->absolutePeekLimit)
				{
					this->peekOverrideFlag = true;
					break;
				}
			}

			if (choice >= 0)
			{
				this->longestPath[iPos] = choice;
				return Successors[iSymbol][candidates[choice]]->successor;
			}
			else
			{
				return "";
			}
		}
	}
}

Int32 S0LProblemV3::Assess_Dbl_PeekAheadStepWithRecursion_L(array<List<SuccessorCount^>^>^ Successors, Int32 iSymbol, Int32 iGen, Int32 iPos, Int32 iScan, Int32 PeekCount, Int32 RecursionCount)
{
	if (iScan >= this->model->evidence[iGen + 1]->Length())
	{
		return 0;
	}
	if (this->modelCurrent->alphabet->IsTurtle(iSymbol))
	{
		if (iPos == this->modelCurrent->evidence[iGen]->Length() - 1 && (this->modelCurrent->evidence[iGen+1]->symbolIndices[iScan] == iSymbol))
		{
			return 1;
		}
		else if ((PeekCount >= this->absolutePeekLimit) || (RecursionCount >= this->absoluteRecursionLimit) && (this->modelCurrent->evidence[iGen+1]->symbolIndices[iScan] == iSymbol))
		{
			return this->absolutePeekLimit;
		}
		else if (this->modelCurrent->evidence[iGen+1]->symbolIndices[iScan] == iSymbol)
		{
			return 1 + this->Assess_Dbl_PeekAheadStepWithRecursion_L(Successors, this->modelCurrent->evidence[iGen]->symbolIndices[iPos + 1], iGen, iPos + 1, iScan + 1, 1, RecursionCount + 1);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		String^ successor = "";
		List<Int32>^ candidates = gcnew List<Int32>;

		for (size_t iSuccessor = 0; iSuccessor < Successors[iSymbol]->Count; iSuccessor++)
		{
			if (iPos < this->modelCurrent->evidence[iGen]->Length() - 1)
			{
				if (((iScan + (Successors[iSymbol][iSuccessor]->successor->Length - 1)) < this->modelCurrent->evidence[iGen + 1]->Length()) && (Successors[iSymbol][iSuccessor]->successor == this->modelCurrent->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (Successors[iSymbol][iSuccessor]->successor->Length - 1))))
				{
					candidates->Add(iSuccessor);
				}
			}
			else
			{
				if (Successors[iSymbol][iSuccessor]->successor == this->modelCurrent->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->modelCurrent->evidence[iGen + 1]->Length() - 1))
				{
					candidates->Add(iSuccessor);
				}
			}
				}

		if (candidates->Count == 0)
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Computing longest path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") no candidates found");
#endif
			this->pathEnd = iPos;
			return 0;
		}
		else if (iPos == this->modelCurrent->evidence[iGen]->Length() - 1)
		{
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Computing longest path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") last symbol using candidate " + candidates[0]);
#endif
			this->longestPath[iPos] = candidates[0];
			return 1;
		}
		else if ((PeekCount >= this->absolutePeekLimit) || (RecursionCount >= this->absoluteRecursionLimit))
		{
			return this->absolutePeekLimit;
		}
		else if (iPos + 1 < this->modelCurrent->evidence[iGen]->Length())
		{
			Int32 longest = -1;
			Int32 choice = 0;
			bool ambiguousFlag = false;

			for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("Computing longest path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") assessing candidate " + Successors[iSymbol][candidates[iCandidate]]->successor);
#endif
				Int32 length = PeekCount + 1;
				Int32 tmp = 1 + this->Assess_Dbl_PeekAheadStepWithRecursion_L(Successors, this->modelCurrent->evidence[iGen]->symbolIndices[iPos + 1], iGen, iPos + 1, iScan + Successors[iSymbol][candidates[iCandidate]]->successor->Length, length, RecursionCount + 1);
				if (tmp > longest)
				{
					longest = tmp;
					choice = iCandidate;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("Computing longest path for " + this->modelCurrent->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") candidate " + Successors[iSymbol][candidates[choice]]->successor + " has longest path so far at " + longest);
#endif
				}

				if (tmp >= this->absolutePeekLimit)
				{
					longest = this->absolutePeekLimit;
					this->longestPath[iPos] = choice;
					break;
				}
				else if (tmp == longest)
				{
					ambiguousFlag = true;
				}
			}

			if (!ambiguousFlag)
			{
				this->longestPath[iPos] = choice;
			}

			return longest;
		}
	}
}

String^ S0LProblemV3::Assess_Dbl_ConsumeGeneStep_L(Int32 V, Int32 iGen, Int32 iPos, Int32 iScan)
{
	String^ successor = "";
	//Int32 length = Math::Ceiling((float) V / 2);
	Int32 length = V;

	if (this->model->evidence[iGen + 1]->Length() - iScan >= length)
	{
		successor = this->model->evidence[iGen + 1]->GetSymbols(iScan, length);
	}
	else
	{
		successor = "";
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine("By value " + V + " (len = " + length + ") for " + this->model->alphabet->GetSymbol(this->factOverlay[iGen][iPos]->sac->iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif

	return successor;
}

double S0LProblemV3::Assess_Dbl(array<Int32>^ RuleLengths)
{
	this->modelCurrent = this->model;
	return this->Assess_Dbl_L(RuleLengths);
}

double S0LProblemV3::Assess_Dbl_PoL2(array<Int32>^ RuleLengths)
{
	double fitness = 0;

	//RuleLengths[0] = 5;
	//RuleLengths[1] = 5;
	//RuleLengths[2] = 2;
	//RuleLengths[3] = 4;

	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);

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
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		Int32 iSymbol;
		String^ successor = "";
		Int32 length = 0;
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

			successor = this->Assess_Dbl_PeekAheadStep_L(successors, iSymbol, iGen, iPos, iScan, 0);

			if (successor == "")
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") no successor found");
#endif
				successor = this->Assess_Dbl_ConsumeGeneStep_PoL2(RuleLengths[iGene], iGen, iPos, iScan, reserveRightMax, reserveRightMin);
				iGene++;
			}
			else
			{
				fitness++;
			}
		}
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Last symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") get successor " + successor);
#endif
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors = this->AddSuccessor(iGen, iPos, successors, successor);
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
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			Int32 iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

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
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors = this->AddSuccessor(iGen, iPos, successors, successor);
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
	}

	if ((!failed) && (!softFail))
	{
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
			Console::WriteLine("odds=" + fitness.ToString());

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
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::Write("Assessing: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}
	Console::WriteLine(fitness.ToString());
#endif

	return fitness;
}

double S0LProblemV3::Assess_Dbl_L(array<Int32>^ RuleLengths)
{
	double fitness = 0;
	this->iteration++;
	UInt64 placeInSearchSpace = 0;
	if (iteration % 1000000 == 0)
	{
		for (Int32 i = RuleLengths->GetUpperBound(0); i >= 0; --i)
		{
			if (i % 2 == 0)
			{
				UInt64 tmp = (RuleLengths[i]-1) * Math::Pow(this->aiBF->searchSpace[this->aiBF->searchSpace->GetUpperBound(0)]->max, RuleLengths->GetUpperBound(0) - i);
				placeInSearchSpace += tmp;
			}
			else if (i == 1)
			{
				UInt64 tmp = (RuleLengths[i]) * Math::Pow(this->aiBF->searchSpace[this->aiBF->searchSpace->GetUpperBound(0)]->max, RuleLengths->GetUpperBound(0) - i);
				placeInSearchSpace += tmp;
			}
			else if (i % 2 == 1)
			{
				UInt64 tmp = (RuleLengths[i]+1) * Math::Pow(this->aiBF->searchSpace[this->aiBF->searchSpace->GetUpperBound(0)]->max, RuleLengths->GetUpperBound(0) - i);
				placeInSearchSpace += tmp;
			}
		}

		Console::WriteLine("Iteration #" + iteration + " / " + this->maxSearchSpaceSize + " (" + (100 * ((float)iteration / this->maxSearchSpaceSize)) + "%)");
		Console::WriteLine("Iteration #" + placeInSearchSpace + " / " + this->maxSearchSpaceSize + " (" + (100 * ((float)placeInSearchSpace / this->maxSearchSpaceSize)) + "%)");
		Console::Write("Assessing: ");
		for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
		{
			Console::Write(RuleLengths[i] + " ");
		}
		Console::WriteLine();
		//Console::ReadLine();
	}

#if _PHASE3_S0L_VERBOSE_ > 0

	if (iteration == 1)
	{
		//RuleLengths[0]  = 17; // B
		//RuleLengths[1]  = 0;
		//RuleLengths[2]  = 1; // U & V
		//RuleLengths[3]  = 11; 
		//RuleLengths[4]  = 68; // W
		//RuleLengths[5]  = 0;
		//RuleLengths[6]  = 34; // K
		//RuleLengths[7]  = 0;
		////RuleLengths[8]  = 1; // V
		////RuleLengths[9]  = 64;
		//RuleLengths[8] = 10; // L
		//RuleLengths[9] = 65;
		//RuleLengths[10] = 21; // K
		//RuleLengths[11] = 13;
		//RuleLengths[12] = 8; // S
		//RuleLengths[13] = 328;

		RuleLengths[0] = 5;
		RuleLengths[1] = 0;
		RuleLengths[2] = 1;
		RuleLengths[3] = 7;
		RuleLengths[4] = 1;
		RuleLengths[5] = 0;
		RuleLengths[6] = 5;
		RuleLengths[7] = 5;
		RuleLengths[8] = 1;
		RuleLengths[9] = -1;
	}
#endif

	this->flagOverride = false;


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
	bool done = false;
	bool failed = false;
	this->nontrivialFlag = false;
	Int32 iSymbol;
	Int32 peekCount = 0;
	this->longestPath = gcnew array<Int32>(this->model->evidence[iGen]->Length());
	for (size_t i = 0; i <= this->longestPath->GetUpperBound(0); i++)
	{
		longestPath[i] = -1;
	}
	this->pathEnd = -1;

	Int32 reserveRightMin = 0;
	Int32 reserveRightMax = 0;
	if (!this->lengthBasedFlag)
	{

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
			reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
		}
	}

	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		iSymbol = this->model->evidence[iGen]->symbolIndices[iPos];
		String^ successor = "";
		Int32 length = 0;
		if ((iPos < this->lastSymbolPos[iGen]) && (!this->model->alphabet->IsTurtle(iSymbol)))
		{
			// Check to see if the current symbol matches an existing successor
			bool found = false;
			Int32 iSuccessor = 0;

			//Console::WriteLine(this->modelCurrent->evidence[2]->GetSymbolsFromPos(2550, this->modelCurrent->evidence[2]->Length()-1));

			if (peekCount < RuleLengths[iGene+1])
			{
				successor = this->Assess_Dbl_PeekAheadStep_L(successors, iSymbol, iGen, iPos, iScan, peekCount);
			}

			// if no successor found or if an override is required then consume a gene
			if (successor == "")
			{
				if (RuleLengths[iGene] > 1)
				{
					this->nontrivialFlag = true;
				}

				if ((peekCount < RuleLengths[iGene + 1]) && (this->aiBF->state->index == iGene + 1) && (!this->aiBF->flagSkip) && (!this->aiBF->flagOverride) && (!this->flagOverride))
				{
					this->overrideIndex = iGene;
					this->overrideValue = RuleLengths[iGene] + 1;
					this->flagOverride = true;
				}

				if ((RuleLengths[iGene + 1] > 0) && (peekCount < RuleLengths[iGene + 1]) && (this->aiBF->state->index != iGene + 1) && (RuleLengths[iGene + 1]+1 < this->aiBF->searchSpace[iGene+1]->max))
				{
					// create a delayed skip
#if _PHASE3_S0L_VERBOSE_ > 1
					Console::WriteLine("Setting delayed skip for " + (iGene + 1) + " at value " + (RuleLengths[iGene + 1] + 1));
#endif
					this->aiBF->SetDelayedSkip(iGene + 1, RuleLengths[iGene + 1]+1);
				}

				peekCount = 0;
				if (this->lengthBasedFlag)
				{
					successor = this->Assess_Dbl_ConsumeGeneStep_L(RuleLengths[iGene], iGen, iPos, iScan);
				}
				else
				{
					successor = this->Assess_Dbl_ConsumeGeneStep_PoL(RuleLengths, iGene, iGen, iPos, iScan, reserveRightMax, reserveRightMin);
				}

				iGene = iGene + 2;
				while (iGene + 1 <= RuleLengths->GetUpperBound(0) && RuleLengths[iGene + 1] == -1)
				{
					iGene = iGene + 2;
				}
			}
			else
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				if (successor != "")
				{
					Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
				}
#endif
				if (!this->peekOverrideFlag)
				{
					this->peekOverrideFlag = false;
					peekCount++;
				}
				fitness++;
			}
		}
		else if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			if (iScan <= this->model->evidence[iGen + 1]->Length())
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - (this->model->evidence[iGen]->Length() - this->lastSymbolPos[iGen]));
			}
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Last symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") get successor " + successor);
#endif
		}
		else
		{
			successor = this->model->alphabet->GetSymbol(iSymbol);
		}

		failed = failed || (successor == "");

		if (this->modelCurrent->alphabet->IsTurtle(iSymbol))
		{
			if (iScan < this->modelCurrent->evidence[iGen + 1]->Length())
			{

				failed = failed || (successor != this->modelCurrent->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
			else
			{
				failed = true;
			}
#if _PHASE3_S0L_VERBOSE_ > 0
			if (successor != this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan))
			{
				Console::WriteLine("*** FAILED *** Turtle symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") does not match " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
			else
			{
				Console::WriteLine("Turtle symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") matches current scan position " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
#endif
		}

		if (!failed)
		{
			if (!this->modelCurrent->alphabet->IsTurtle(iSymbol))
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
				this->longestPath = gcnew array<Int32>(this->model->evidence[iGen]->Length());
				this->pathEnd = -1;
				if ((!this->lengthBasedFlag) && (iGen <= this->factOverlay->GetUpperBound(0)))
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
			else if (!this->lengthBasedFlag)
			{
				reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
				reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			}

			if (iGen >= this->model->generations - 1)
			{
				done = true;
			}
		}
		else
		{
			// SKIP CONDITION
			if (((failed) || (peekCount < RuleLengths[iGene - 1])) && (!this->aiBF->flagSkip) && (!this->aiBF->flagOverride) && (!this->flagOverride))
			{
				Int32 jGene = iGene - 2;
				while (jGene >= 0 && RuleLengths[jGene + 1] == -1)
				{
					jGene = jGene - 2;
				}

				this->overrideIndex = jGene;
				this->overrideValue = RuleLengths[jGene] + 1;
				this->flagOverride = true;
			}

#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("**FAILED**" + successor);
			Console::WriteLine();
#endif
		}
	}

	//if ((RuleLengths[iGene - 1] == -1) && (!this->aiBF->flagSkip) && (!this->aiBF->overrideFlag) && (!this->flagOverride))
	//{
	//	this->overrideIndex = iGene-1;
	//	this->overrideValue = RuleLengths[iGene-1] + 1;
	//	this->flagOverride = true;
	//}

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
			successor = this->Assess_Dbl_PeekAheadStep_L(successors, iSymbol, iGen, iPos, iScan,0);

			if (successor != "")
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif
				fitness++;
			}
			else
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("For " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") no successor found");
#endif
				softFail = true;
			}
		}
		else if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			if (iScan <= this->model->evidence[iGen + 1]->Length() - 1)
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
			}
		}
		else
		{
			successor = this->model->alphabet->GetSymbol(iSymbol);
		}

		failed = failed || (successor == "");

		if (this->modelCurrent->alphabet->IsTurtle(iSymbol))
		{
			if (iScan < this->modelCurrent->evidence[iGen+1]->Length())
			{
				failed = failed || (successor != this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
			else
			{
				failed = true;
			}
#if _PHASE3_S0L_VERBOSE_ > 0
			if (successor != this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan))
			{
				Console::WriteLine("*** FAILED *** Turtle symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") does not match " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
			else
			{
				Console::WriteLine("Turtle symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") matches current scan position " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan));
			}
#endif
		}

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
				if (iScan >= this->model->evidence[iGen+1]->Length()-1)
				{
					iGen++;
					iPos = 0;
					iScan = 0;
					this->longestPath = gcnew array<Int32>(this->model->evidence[iGen]->Length());
					this->pathEnd = -1;
				}
				else
				{
					failed = true;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("*** FAILED *** Last symbol " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") scan position " + iScan + " not at end of line " + (this->model->evidence[iGen + 1]->Length() - 1));
#endif
				}
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
	//this->ComputeMetrics(successors, fitness);

	if (fitness > this->fitnessBest)
	{
		this->successorsBest = successors;
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	if ((fitness > this->fitnessBest) || ((this->wtp1 == 1.0) && (this->wtp2 == 1.0)))
	{
		if (fitness > this->fitnessBest)
		{
			this->fitnessBest = fitness;
			Console::Write("New Best : ");
		}
		else
		{
			Console::Write("WTP Match: ");
		}

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
	if ((done) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
#if _PHASE3_S0L_VERBOSE_ > 0
		Console::WriteLine("All genes were not consumed");
#endif
		this->OverrideSearchSpace(iGene, RuleLengths[iGene] + 1);
	}

	if (this->flagOverride)
	{
		this->OverrideSearchSpace(this->overrideIndex, this->overrideValue);
	}

	if (done)
	{
		this->doneFlag = true;
	}
	else
	{
		this->doneFlag = false;
	}

	if (done && this->nontrivialFlag)
	{
		this->solvedFlag = true;
		fitness = this->cFitnessTarget;
	}
	
	this->successorsCurrent = successors;

	return fitness;
}

double S0LProblemV3::Assess_Dbl_PoL(array<Int32>^ RuleLengths)
{
	double fitness = Double::MinValue;

	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);

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
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		Int32 iSymbol;
		String^ successor = "";
		Int32 length = 0;
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

			while ((!found) && (iSuccessor < successors[iSymbol]->Count))
			{
				if (successors[iSymbol][iSuccessor]->successor == this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (successors[iSymbol][iSuccessor]->successor->Length - 1)))
				{
					found = true;
					successor = successors[iSymbol][iSuccessor]->successor;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif
				}
				iSuccessor++;
			}

			if (!found)
			{
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("By peeking for " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") no successor found");
#endif
				successor = this->Assess_Dbl_ConsumeGeneStep_PoL(RuleLengths, iGene, iGen, iPos, iScan, reserveRightMax, reserveRightMin);
				iGene++;
			}
		}
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors = this->AddSuccessor(iGen, iPos, successors, successor);

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
				else
				{
					done = true;
				}
			}
		}
	}

	bool softFail = false;
	while ((!done) && (!failed) && (!softFail))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		String^ successor = "";
		Int32 length = 0;
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			Int32 iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

			while ((!found) && (iSuccessor < successors[iSymbol]->Count))
			{
				if (successors[iSymbol][iSuccessor]->successor == this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (successors[iSymbol][iSuccessor]->successor->Length - 1)))
				{
					found = true;
					successor = successors[iSymbol][iSuccessor]->successor;
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
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors = this->AddSuccessor(iGen, iPos, successors, successor);

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
				else
				{
					done = true;
				}
			}
		}
	}

	if (!failed)
	{
		// Compute the remaining successors using Greedy Algorithm
		if (softFail)
		{
			successors = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram, successors, iGen, iPos);
		}
		// Compute the fitness
		fitness = this->ComputeSuccessorsOdds(successors);
#if _PHASE3_S0L_VERBOSE_ >= 0
		if (fitness > this->fitnessBest)
		{
			this->fitnessBest = fitness;
			Console::Write("New Best: ");
			for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
			{
				Console::Write(RuleLengths[i] + " ");
			}
			Console::WriteLine("odds=" + fitness.ToString());

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
	}

#if _PHASE3_S0L_VERBOSE_ >= 0
	Console::Write("Assessing: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}
	Console::WriteLine(fitness.ToString());
	if (RuleLengths[0] == 2)
	{
		Console::ReadLine();
	}
#endif

	return fitness;
}

float S0LProblemV3::Assess(array<Int32>^ RuleLengths)
{
	float fitness = 5.0;
	bool failed = false;
	Int32 iGen = 0;
	Int32 iPos = 0;
	Int32 iScan = 0;
	array<List<SuccessorCount^>^>^ counts = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);

	for (size_t i = 0; i < this->MAO->factsList->Count; i++)
	{
		counts[i] = gcnew List<SuccessorCount^>;
	}

	Int32 idx = 0;
	do
	{
		//Console::WriteLine("Generation #" + iGen);
		iPos = 0;
		Int32 reserveRightMin = 0;
		Int32 reserveRightMax = 0;

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
			reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
		}

		do
		{
			//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));
			Int32 absMinLength = Math::Max(this->factOverlay[iGen][iPos]->length->min, ((this->model->evidence[iGen + 1]->Length() - iScan) - reserveRightMax));
			Int32 absMaxLength = (this->model->evidence[iGen + 1]->Length() - iScan) - reserveRightMin;
			float fraction = RuleLengths[idx] * ((float)this->factOverlay[iGen][iPos]->length->min / this->factOverlay[iGen][iPos]->length->max);
			Int32 length = absMinLength + (fraction * (absMaxLength - absMinLength));
			String^ successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (length-1));
			//Console::WriteLine(" -> " + successor);
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			Int32 fIndex = this->MAO->GetFactIndex(f);

			Int32 iCount = 0;
			bool found = false;
			while (!found && iCount < counts[fIndex]->Count)
			{
				if (counts[fIndex][iCount]->successor == successor)
				{
					counts[fIndex][iCount]->count++;
					found = true;
				}
				iCount++;
			}

			if (!found)
			{
				SuccessorCount^ tmp = gcnew SuccessorCount();
				tmp->successor = successor;
				tmp->count = 1;
				counts[fIndex]->Add(tmp);
			}

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			iPos++;
			iScan += length;
		} while (!failed && iPos < this->model->evidence[iGen]->Length());
		iGen++;
	} while (!failed && iGen < this->model->generations-1);

	if (failed)
	{
		fitness = 99999.0;
	}
	else
	{
		fitness = 1.0;
		for (size_t iFact = 0; iFact < counts->GetUpperBound(0); iFact++)
		{
			double odds = 1.0;
			for (size_t iCount = 0; iCount < counts[iFact]->Count; iCount++)
			{
				odds *= Math::Pow((float)counts[iFact][iCount]->count / this->factCount[iFact], counts[iFact][iCount]->count);
			}
			fitness *= odds;
		}

		fitness = 1 / fitness;
	}

	return fitness;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::CombineSuccessorCounts(array<List<SuccessorCount^>^>^ A, array<List<SuccessorCount^>^>^ B)
{
	array<List<SuccessorCount^>^>^ result = gcnew array<List<SuccessorCount^>^>(A->GetUpperBound(0)+1);
	for (size_t iGen = 0; iGen <= A->GetUpperBound(0); iGen++)
	{
		result[iGen] = gcnew List<SuccessorCount^>;

		for (size_t iCount1 = 0; iCount1 < A[iGen]->Count; iCount1++)
		{
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = A[iGen][iCount1]->successor;
			tmp->count = A[iGen][iCount1]->count;

			Int32 iCount2 = 0;
			bool found = false;

			while (!found && iCount2 < B[iGen]->Count)
			{
				if (tmp->successor == B[iGen][iCount2]->successor)
				{
					tmp->count = B[iGen][iCount2]->count;
					found = true;
				}
				iCount2++;
			}
			result[iGen]->Add(tmp);
		}

		// add any successor that is only in B
		for (size_t iCount1 = 0; iCount1 < B[iGen]->Count; iCount1++)
		{
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = B[iGen][iCount1]->successor;
			tmp->count = B[iGen][iCount1]->count;

			Int32 iCount2 = 0;
			bool found = false;

			while (!found && iCount2 < A[iGen]->Count)
			{
				if (tmp->successor == A[iGen][iCount2]->successor)
				{
					found = true;
				}
				iCount2++;
			}

			if (!found)
			{
				result[iGen]->Add(tmp);
			}
		}
	}

	return result;
}

void S0LProblemV3::ComputeMetrics(array<List<SuccessorCount^>^>^ Successors, double Odds)
{
	// compute match metric
	this->wtp1 = 0.0f;
	this->wtp2 = 0.0f;
	this->wfp = 0.0f;
	this->wfn = 0.0f;
	this->errorAvg = 0.0f;
	this->errorTotal = 0.0f;
	bool found = false;
	Int32 jSuccessor = 0;
	Int32 totalSuccessorsC = 0;
	Int32 totalSuccessorsM = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		totalSuccessorsC += this->model->productions[0]->rules[iSymbol]->successor->Count;
		totalSuccessorsM += Successors[iSymbol]->Count;

		// find successors in the original that are in the candidate solution
		for (size_t iSuccessor = 0; iSuccessor < this->model->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			found = false;
			jSuccessor = 0;

			if ((Successors[iSymbol]->Count > 0) && (Successors[iSymbol] != nullptr && Successors[iSymbol][jSuccessor]->odds >= 1.000))
			{
				found = true;
			}

			while ((!found) && (jSuccessor < Successors[iSymbol]->Count))
			{
				if (Successors[iSymbol][jSuccessor]->successor == this->model->productions[0]->rules[iSymbol]->successor[iSuccessor])
				{
					found = true;
				}
				else
				{
					jSuccessor++;
				}
			}

			if (!found && this->model->productions[0]->rules[iSymbol]->count[iSuccessor] == 0)
			{
				this->wtp1 += (float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100;
			}

			if (found)
			{
				this->wtp1 += (float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100;
				if (iSuccessor == 0 && Successors[iSymbol][jSuccessor]->odds == 1.000)
				{
					this->wtp2 += (float)Successors[iSymbol][jSuccessor]->odds;
					this->errorTotal += Math::Abs(1.00 - ((float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100));
				}
				else if (Successors[iSymbol][jSuccessor]->odds < 1.000)
				{
					this->wtp2 += (float)Successors[iSymbol][jSuccessor]->odds;
					this->errorTotal += Math::Abs(Successors[iSymbol][jSuccessor]->odds - ((float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100));
				}
			}
		}

		// find successors in the original that are NOT in the candidate solution
		for (size_t iSuccessor = 0; iSuccessor < this->model->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			found = false;
			jSuccessor = 0;

			if ((Successors[iSymbol]->Count > 0) && (Successors[iSymbol][jSuccessor]->odds >= 1.000))
			{
				found = true;
			}

			while ((!found) && (jSuccessor < Successors[iSymbol]->Count))
			{
				if (Successors[iSymbol][jSuccessor]->successor == this->model->productions[0]->rules[iSymbol]->successor[iSuccessor])
				{
					found = true;
				}
				jSuccessor++;
			}

			if (!found)
			{
				this->wfn += (float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100;
				this->errorTotal += (float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100;
			}
		}

		// find successors in the candidate solution that are NOT in the original
		for (size_t iSuccessor = 0; iSuccessor < Successors[iSymbol]->Count; iSuccessor++)
		{
			found = false;
			jSuccessor = 0;

			if (Successors[iSymbol][iSuccessor]->odds >= 1.000)
			{
				found = true;
			}

			while ((!found) && (jSuccessor < this->model->productions[0]->rules[iSymbol]->successor->Count))
			{
				if (Successors[iSymbol][iSuccessor]->successor == this->model->productions[0]->rules[iSymbol]->successor[jSuccessor])
				{
					found = true;
				}
				jSuccessor++;
			}

			if (!found)
			{
				this->wfp += (float)Successors[iSymbol][iSuccessor]->odds;
				this->errorTotal += (float)Successors[iSymbol][iSuccessor]->odds;
			}
		}
	}

	this->successorCountDiff = totalSuccessorsC - totalSuccessorsM;

	if (this->successorCountDiff != 0)
	{
		this->successorCountMM = true;
	}
	else
	{
		this->successorCountMM = false;
	}

	this->wtp1 /= this->model->alphabet->numNonTurtles;
	this->wtp2 /= this->model->alphabet->numNonTurtles;
	this->wfn /= this->model->alphabet->numNonTurtles;
	this->wfp /= this->model->alphabet->numNonTurtles;
	this->errorAvg = this->errorTotal / this->model->alphabet->numNonTurtles;

	if (this->wtp1 >= 0.995)
	{
		this->wtp1 = 1.00;
	}

	if (this->wtp2 >= 0.995)
	{
		this->wtp2 = 1.00;
	}

	// compute success metric
	if (this->wtp1 == 1)
	{
		this->success = true;
		Console::WriteLine("SUCCESS: " + Odds + " >= " + this->model->oddsProduced + " overridden ");
	}
	else if (Odds >= this->model->oddsProduced)
	{
		this->success = true;
		Console::WriteLine("SUCCESS: " + Odds + " >= " + this->model->oddsProduced);
	}
	else
	{
		this->success = false;
		Console::WriteLine("FAILED: " + Odds + " < " + this->model->oddsProduced);
	}


	Console::WriteLine("* RESULTS FOR PROBLEM *");
	Console::WriteLine("weighted true positive (M2C) = " + this->wtp1.ToString(L"F4"));
	Console::WriteLine("weighted true positive (C2M) = " + this->wtp2.ToString(L"F4"));
	Console::WriteLine("weighted false positive      = " + this->wfp.ToString(L"F4"));
	Console::WriteLine("weighted false negative      = " + this->wfn.ToString(L"F4"));
	Console::WriteLine("error (avg)                  = " + this->errorAvg.ToString(L"F4"));
	Console::WriteLine("error (total)                = " + this->errorTotal.ToString(L"F4"));
	Console::WriteLine();
}

void S0LProblemV3::ComputeLength_Production_S0LOnly()
{
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		List<Fact^>^ l = this->factOverlay[iGen];
		Int32 reserveLeft = 0;
		Int32 reserveRight = 0;

		for (size_t iPos = 1; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			reserveRight = this->factOverlay[iGen][iPos]->length->min;
		}

		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Fact^ f = l[iPos];
			if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen + 1]->GetSymbolIndex(iPos)))
			{
				f->length->max = Math::Min(this->absoluteMaxLength, this->model->evidence[iGen + 1]->Length() - reserveLeft - reserveRight);
			}

			reserveLeft += f->length->min;
			reserveRight -= f->length->min;
		}
	}
}

double S0LProblemV3::ComputeGreedySuccessor(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep)
{
	double result = 0.0;

	Int32 absMinLength = Math::Max(this->absoluteMinLength, Math::Max(this->factOverlay[iGen][iPos]->length->min, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->absoluteMaxLength, Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin));
	String^ choice = "";
	Int32 iChoice = 0;
	Int32 iChoiceLength = 0;
	double highest = 0;
	double tiebreaker = 0;
	double odds = 0.0;
	bool found = false;

	for (size_t iLength = absMinLength; iLength <= absMaxLength; iLength++)
	{
		String^ tmp = this->model->evidence[iGen + 1]->GetSymbols(iScan, iLength);
		for (size_t iWord = 0; iWord < H[iLength]->Count; iWord++)
		{
			if (H[iLength][iWord]->successor == tmp)
			{
				// Compute how this effects the overall likelihood of occuring
				Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				Int32 iFact = this->MAO->GetFactIndex(f);
				odds = O * this->ComputeSuccessorOdds(H[iLength][iWord]->successor, Successors, iFact);

				if (iPos + 1 < this->model->evidence[iGen]->Length() && iStep+1 <= this->maxGreedySteps)
				{
					odds = this->ComputeGreedySuccessor(iGen, iPos + 1, iScan + H[iLength][iWord]->successor->Length, odds, H, Successors, ReserveRightMin - this->factOverlay[iGen][iPos]->length->min, ReserveRightMax - this->factOverlay[iGen][iPos]->length->max, iStep + 1);
				}

				if (odds > highest || (odds == highest && H[iLength][iWord]->odds > tiebreaker))
				{
					highest = odds;
					tiebreaker = H[iLength][iWord]->odds;
					choice = H[iLength][iWord]->successor;
					iChoice = iWord;
					iChoiceLength = iLength;
					found = true;
					//Console::WriteLine("Step #" + iStep + " Update choice to " + choice);
				}
			}
		}
	}

	if (!found)
	{
		odds = this->ComputeGreedySuccessor_SharedHistogram(iGen, iPos, iScan, odds, this->histogram, Successors, ReserveRightMin, ReserveRightMax, 1);
	}

	if (iStep == 1 && found)
	{
		Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
		Int32 iFact = this->MAO->GetFactIndex(f);

		// Add the choosen successor to the successor list
		bool found = false;

		for (size_t i = 0; i < Successors[iFact]->Count; i++)
		{
			if (Successors[iFact][i]->successor == choice)
			{
				Successors[iFact][i]->count++;
				found = true;
			}
		}

		if (!found)
		{
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = choice;
			tmp->count = 1;
			tmp->odds = 1;
			Successors[iFact]->Add(tmp);
		}

#if _PHASE3_S0L_VERBOSE_ > 1
		Console::WriteLine("For " + this->model->alphabet->GetSymbol(f->sac->iSymbol) + " (" + iGen + "," + iPos + ") final choice is " + choice);
#endif

		result = choice->Length;
	}
	else if (iStep == 1 && !found)
	{
		result = odds;
	}
	else
	{
		result = highest;
	}

	return result;
}

double S0LProblemV3::ComputeGreedySuccessor_Right2Left(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep)
{
	double result = 0.0;

	Int32 absMinLength = Math::Max(this->absoluteMinLength, Math::Max(this->factOverlay[iGen][iPos]->length->min, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->absoluteMaxLength, Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin));
	String^ choice = "";
	Int32 iChoice = 0;
	Int32 iChoiceLength = 0;
	double highest = 0;
	double tiebreaker = 0;
	double odds = 0.0;
	bool found = false;

	for (size_t iLength = absMinLength; iLength <= absMaxLength; iLength++)
	{
		for (size_t iWord = 0; iWord < H[iLength]->Count; iWord++)
		{
			if (H[iLength][iWord]->successor == this->model->evidence[iGen + 1]->GetSymbols(this->model->evidence[iGen + 1]->Length() - iScan - iLength, iLength))
			{
				// Compute how this effects the overall likelihood of occuring
				Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				Int32 iFact = this->MAO->GetFactIndex(f);
				odds = O * this->ComputeSuccessorOdds(H[iLength][iWord]->successor, Successors, iFact);

				//Console::WriteLine("Step #" + iStep + " Successor " + H[iLength][iWord]->successor + " matches and changes odds to " + odds);

				if (iPos + 1 < this->model->evidence[iGen]->Length() && iStep + 1 <= this->maxGreedySteps)
				{
					odds = this->ComputeGreedySuccessor(iGen, iPos + 1, iScan + H[iLength][iWord]->successor->Length, odds, H, Successors, ReserveRightMin - this->factOverlay[iGen][iPos]->length->min, ReserveRightMax - this->factOverlay[iGen][iPos]->length->max, iStep + 1);
				}

				if (odds > highest || (odds == highest && H[iLength][iWord]->odds > tiebreaker))
				{
					highest = odds;
					tiebreaker = H[iLength][iWord]->odds;
					choice = H[iLength][iWord]->successor;
					iChoice = iWord;
					iChoiceLength = iLength;
					found = true;
					//Console::WriteLine("Step #" + iStep + " Update choice to " + choice);
				}
			}
		}
	}

	if (!found)
	{
		odds = this->ComputeGreedySuccessor_SharedHistogram(iGen, iPos, iScan, odds, this->histogram, Successors, ReserveRightMin, ReserveRightMax, 1);
	}

	if (iStep == 1 && found)
	{
		Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
		Int32 iFact = this->MAO->GetFactIndex(f);

		// Add the choosen successor to the successor list
		bool found = false;

		for (size_t i = 0; i < Successors[iFact]->Count; i++)
		{
			if (Successors[iFact][i]->successor == choice)
			{
				Successors[iFact][i]->count++;
				found = true;
			}
		}

		if (!found)
		{
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = choice;
			tmp->count = 1;
			tmp->odds = 1;
			Successors[iFact]->Add(tmp);
		}

		//Console::WriteLine("Finalize choice is " + choice);

		result = choice->Length;
	}
	else if (iStep == 1 && !found)
	{
		result = odds;
	}
	else
	{
		result = odds;
	}

	return result;
}

double S0LProblemV3::ComputeGreedySuccessor_SharedHistogram(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep)
{
	double result = 0.0;

	Int32 absMinLength = Math::Max(this->absoluteMinLength, Math::Max(this->factOverlay[iGen][iPos]->length->min, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMax));
	Int32 absMaxLength = Math::Min(this->absoluteMaxLength, Math::Min(this->factOverlay[iGen][iPos]->length->max, (this->model->evidence[iGen + 1]->Length() - iScan) - ReserveRightMin));
	String^ choice = "";
	Int32 iChoice = 0;
	Int32 iChoiceLength = 0;
	double highest = 0;
	double tiebreaker = 0;
	double odds = 0.0;
	bool found = false;

	for (size_t iLength = absMinLength; iLength <= absMaxLength; iLength++)
	{
		for (size_t iWord = 0; iWord < H[iLength]->Count; iWord++)
		{
			if (H[iLength][iWord]->successor == this->model->evidence[iGen + 1]->GetSymbols(iScan, iLength))
			{
				// Compute how this effects the overall likelihood of occuring
				Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				Int32 iFact = this->MAO->GetFactIndex(f);
				odds = O * this->ComputeSuccessorOdds(H[iLength][iWord]->successor, Successors, iFact);

				//Console::WriteLine("Step #" + iStep + " Successor " + H[iLength][iWord]->successor + " matches and changes odds to " + odds);

				if (iPos + 1 < this->model->evidence[iGen]->Length() && iStep + 1 <= this->maxGreedySteps)
				{
					odds = this->ComputeGreedySuccessor(iGen, iPos + 1, iScan + H[iLength][iWord]->successor->Length, odds, H, Successors, ReserveRightMin - this->factOverlay[iGen][iPos]->length->min, ReserveRightMax - this->factOverlay[iGen][iPos]->length->max, iStep + 1);
				}

				if (odds > highest || (odds == highest && H[iLength][iWord]->odds > tiebreaker))
				{
					highest = odds;
					tiebreaker = H[iLength][iWord]->odds;
					choice = H[iLength][iWord]->successor;
					iChoice = iWord;
					iChoiceLength = iLength;
					found = true;
					//Console::WriteLine("Step #" + iStep + " Update choice to " + choice);
				}
			}
		}
	}

	if (iStep == 1 && found)
	{
		Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
		Int32 iFact = this->MAO->GetFactIndex(f);

		// Add the choosen successor to the successor list
		bool found = false;

		for (size_t i = 0; i < Successors[iFact]->Count; i++)
		{
			if (Successors[iFact][i]->successor == choice)
			{
				Successors[iFact][i]->count++;
				found = true;
			}
		}

		if (!found)
		{
			SuccessorCount^ tmp = gcnew SuccessorCount();
			tmp->successor = choice;
			tmp->count = 1;
			tmp->odds = 1;
			Successors[iFact]->Add(tmp);
		}

		result = choice->Length;
	}
	else if (iStep == 1 && !found)
	{
		result = -1;
	}
	else
	{
		result = odds;
	}

	return result;
}

List<SuccessorCount^>^ S0LProblemV3::ComputeSuccessorList(array<List<SuccessorCount^>^>^ H)
{
	List<SuccessorCount^>^ result = gcnew List<SuccessorCount^>;
	for (size_t i = 1; i < H->GetUpperBound(0); i++)
	{
		Console::WriteLine("For length = " + i);
		Int32 total = 0;
		for (size_t j = 0; j < H[i]->Count; j++)
		{
			total += H[i][j]->count;
			Console::WriteLine(H[i][j]->successor + " " + H[i][j]->count);
		}

		for (size_t j = 0; j < H[i]->Count; j++)
		{
			H[i][j]->odds = this->absoluteMaxLength - Math::Abs(i - this->avgLength) + (float)H[i][j]->count / total;
			//H[i][j]->odds = (float)H[i][j]->count / total;
			//double tmp = 100.0 / (1 + (this->absoluteMaxLength - Math::Abs(i - this->avgLength)));
			//H[i][j]->odds *= tmp;
			result->Add(H[i][j]);
			Console::WriteLine("Adding " + H[i][j]->successor + " to list with value " + H[i][j]->odds);
		}
	}

	for (size_t i = 1; i < H->GetUpperBound(0); i++)
	{
		H[i]->Sort(gcnew SuccessorOddsComparer());
	}

	result->Sort(gcnew SuccessorOddsComparer());

	return result;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessorHistogram()
{
	array<List<SuccessorCount^>^>^ result = gcnew array<List<SuccessorCount^>^>(this->absoluteMaxLength + 1);

	for (size_t iLength = 0; iLength <= this->absoluteMaxLength; iLength++)
	{
		result[iLength] = gcnew List<SuccessorCount^>;
	}

	for (size_t iLength = 1; iLength <= this->absoluteMaxLength; iLength++)
	{
		for (size_t iGen = 1; iGen < this->model->generations; iGen++)
		{
			if (iLength <= this->model->evidence[iGen]->Length())
			{
				List<String^>^ subwords = gcnew List<String^>;

				for (size_t iStart = 0; iStart <= this->model->evidence[iGen]->Length() - iLength; iStart++)
				{
					String^ tmp = this->model->evidence[iGen]->GetSymbols(iStart, iLength);
					subwords->Add(tmp);
				}

				bool found = false;
				Int32 iWord = 0;

				for (size_t jWord = 0; jWord < subwords->Count; jWord++)
				{
					found = false;
					iWord = 0;
					while (!found && iWord < result[iLength]->Count)
					{
						if (result[iLength][iWord]->successor == subwords[jWord])
						{
							found = true;
							result[iLength][iWord]->count++;
							//Console::WriteLine("Incrementing " + subwords[jWord] + " to " + histogram[iLength][iWord]->count);
						}
						else
						{
							iWord++;
						}
					}

					if (!found)
					{
						SuccessorCount^ tmp = gcnew SuccessorCount();
						tmp->successor = subwords[jWord];
						tmp->count = 1;
						result[iLength]->Add(tmp);
						//Console::WriteLine("Adding " + tmp->successor + " to " + tmp->count);
					}
				}
			}
		}
	}

	for (size_t i = 1; i <= result->GetUpperBound(0); i++)
	{
		//Console::WriteLine("For length = " + i);
		Int32 total = 0;
		for (size_t j = 0; j < result[i]->Count; j++)
		{
			total += result[i][j]->count;
			//Console::WriteLine(result[i][j]->successor + " " + result[i][j]->count);
		}

		for (size_t j = 0; j < result[i]->Count; j++)
		{
			result[i][j]->odds = this->lengthValues[i] + (float)result[i][j]->count / total;
		}
	}

	for (size_t i = 1; i <= result->GetUpperBound(0); i++)
	{
		result[i]->Sort(gcnew SuccessorOddsComparer());
	}

	return result;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessorHistogram(array<List<SuccessorCount^>^>^ Successors)
{
	array<List<SuccessorCount^>^>^ result = gcnew array<List<SuccessorCount^>^>(this->absoluteMaxLength + 1);

	for (size_t iLength = 0; iLength <= this->absoluteMaxLength; iLength++)
	{
		result[iLength] = gcnew List<SuccessorCount^>;
	}

	for (size_t iSymbol = 0; iSymbol <= Successors->GetUpperBound(0); iSymbol++)
	{
		for (size_t iWord = 0; iWord < Successors[iSymbol]->Count; iWord++)
		{
			Int32 iLength = Successors[iSymbol][iWord]->successor->Length;
			bool found = false;
			Int32 jWord = 0;

			while (!found && jWord < result[iLength]->Count)
			{
				if (result[iLength][jWord]->successor == Successors[iSymbol][iWord]->successor)
				{
					found = true;
				}
				else
				{
					jWord++;
				}
			}

			if (!found)
			{
				SuccessorCount^ tmp = gcnew SuccessorCount();
				tmp->successor = Successors[iSymbol][iWord]->successor;
				tmp->count = Successors[iSymbol][iWord]->count;
				result[iLength]->Add(tmp);
			}
		}
	}

	for (size_t i = 1; i <= result->GetUpperBound(0); i++)
	{
		Int32 total = 0;
		for (size_t j = 0; j < result[i]->Count; j++)
		{
			total += result[i][j]->count;
		}

		for (size_t j = 0; j < result[i]->Count; j++)
		{
			result[i][j]->odds = this->lengthValues[i] + (float)result[i][j]->count / total;
		}
	}

	for (size_t i = 1; i < result->GetUpperBound(0); i++)
	{
		result[i]->Sort(gcnew SuccessorOddsComparer());
	}


	return result;
}

double S0LProblemV3::ComputeSuccessorsOdds(array<List<SuccessorCount^>^>^ Successors)
{
	// compute the odds of these successors
	double X = 1.0;
	Int32 tmp = 1;
	for (size_t iFact = 0; iFact <= Successors->GetUpperBound(0); iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];
		Int32 iSymbol = f->sac->iSymbol;

		for (size_t j = 0; j < Successors[iSymbol]->Count; j++)
		{
			Successors[iSymbol][j]->odds = (float)Successors[iSymbol][j]->count / this->factCount[iFact];
			X *= Math::Pow((double)Successors[iSymbol][j]->odds, Successors[iSymbol][j]->count);
			tmp++;
		}
	}

	if (X == 0)
	{
		X = Double::MinValue;
	}

#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine("The odds of this S0L/S2L occuring is " + X);
#endif

	if (X == Double::MinValue)
	{
		X = 0;
	}

	return X;
}

double S0LProblemV3::ComputeSuccessorsOdds_SingleGenerationOnly(array<List<SuccessorCount^>^>^ Successors)
{
	// compute the odds of these successors
	double X = 1.0;
	Int32 tmp = 1;
	for (size_t i = 0; i <= Successors->GetUpperBound(0); i++)
	{
		Int32 total = 0;
		for (size_t j = 0; j < Successors[i]->Count; j++)
		{
			total += Successors[i][j]->count;
		}

		for (size_t j = 0; j < Successors[i]->Count; j++)
		{
			X *= Math::Pow((double)Successors[i][j]->count / total, Successors[i][j]->count);
			tmp++;
		}
	}

	Console::WriteLine("The odds of this S0L/S2L occuring is " + X);

	return X;
}

double S0LProblemV3::ComputeSuccessorOdds(String^ S, array<List<SuccessorCount^>^>^ Counts, Int32 iFact)
{
	double odds = 1.0;
	bool found = false;

	for (size_t iCount = 0; iCount < Counts[iFact]->Count; iCount++)
	{
		Int32 count = 0;
		
		if (S == Counts[iFact][iCount]->successor)
		{
			count = Counts[iFact][iCount]->count + 1;
			found = true;
		}
		else
		{
			count = Counts[iFact][iCount]->count;
		}

		odds *= Math::Pow((double)count / this->factCount[iFact], count);
	}

	if (!found)
	{
		odds *= Math::Pow((double)1 / this->factCount[iFact], 1);
	}

	return odds;
}

GenomeConfiguration<Int32>^ S0LProblemV3::CreateGenome()
{
	Int32 numGenes = 0;

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos)))
			{
				numGenes++;
			}
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	Int32 iGene = 0;
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos)))
			{
				min[iGene] = this->factOverlay[iGen][iPos]->length->min;
				max[iGene] = this->factOverlay[iGen][iPos]->length->max;
				this->rule2solution->Add(iGene);
				this->solution2rule->Add(iGene);
				iGene++;
			}
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);
	return result;
}

GenomeConfiguration<Int32>^ S0LProblemV3::CreateGenome_GreedyBF()
{
	this->maxSearchSpaceSize = 1;
	this->absoluteMaxLength = 3;
	this->absolutePeekLimit = 4;
	this->absoluteRecursionLimit = 5;

	//this->numGenes = this->model->alphabet->numNonTurtles + (this->model->alphabet->numNonTurtles * 0.75) + (this->model->alphabet->numNonTurtles * 0.5);
	//this->numGenes = Math::Max(5, Math::Min(12, numGenes));

	this->numGenes = 0;
	for (size_t i = 0; i < this->model->productions[0]->rules->Count; i++)
	{
		this->numGenes += this->model->productions[0]->rules[i]->successor->Count;
	}

	this->numGenes = 16;
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

BruteForceSearchConfiguration^ S0LProblemV3::CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig)
{
	array<DimensionSpecType^>^ searchSpace = gcnew array<DimensionSpecType^>(GenomeConfig->numGenes);

	for (size_t iRule = 0; iRule < searchSpace->Length; iRule++)
	{
		searchSpace[iRule] = gcnew DimensionSpecType();
		searchSpace[iRule]->min = GenomeConfig->min[iRule];
		searchSpace[iRule]->max = GenomeConfig->max[iRule];
	}

	BruteForceSearchConfiguration^ result = gcnew BruteForceSearchConfiguration(searchSpace);

	return result;
}

List<ProductionRuleV3^>^ S0LProblemV3::CreateRulesByScanning(array<Int32>^ RuleLengths)
{
	List<ProductionRuleV3^>^ result = gcnew List<ProductionRuleV3^>;
	List<Fact^>^ completed = gcnew List<Fact^>;
	Int32 iGen = 0;
	Int32 iPos = 0;
	Int32 iScan = 0;

	while (completed->Count < this->MAO->factsList->Count && iGen < this->model->generations - 1)
	{
		iPos = 0;
		iScan = 0;
		while (completed->Count < this->MAO->factsList->Count && iPos < this->model->evidence[iGen]->Length())
		{
			Int32 iTable = 0;
			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

			// get the right fact
			Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);
			Int32 iFact = this->MAO->GetFactIndex(f);

			if (!completed->Contains(f) && iScan + RuleLengths[this->rule2solution[iFact]] <= this->model->evidence[iGen + 1]->Length())
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
					r->contextLeft->Add(this->model->alphabet->GetSymbol(iLeft));
					r->contextRight->Add(this->model->alphabet->GetSymbol(iRight));
					r->condition->Add("*");
					r->odds->Add(100);
					r->parameters = this->model->parameters;
					if (iScan + RuleLengths[this->rule2solution[iFact]] - 1 <= this->model->evidence[iGen + 1]->Length() - 1)
					{
						r->successor->Add(this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1));
						result->Add(r);
						completed->Add(f);
					}
				}
				else
				{
					result[iRule]->contextLeft->Add(this->model->alphabet->GetSymbol(iLeft));
					result[iRule]->contextRight->Add(this->model->alphabet->GetSymbol(iRight));
					result[iRule]->condition->Add("*");
					result[iRule]->odds->Add(100);
					if (iScan + RuleLengths[this->rule2solution[iFact]] - 1 <= this->model->evidence[iGen + 1]->Length() - 1)
					{
						result[iRule]->successor->Add(this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1));
						completed->Add(f);
					}
				}
			}

			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				iScan += RuleLengths[this->rule2solution[iFact]];
			}
			else
			{
				iScan++;
			}
			iPos++;
		}
		iGen++;
	}

	if (completed->Count < this->MAO->factsList->Count)
	{
		result = nullptr;
	}
	else
	{// Sort the Production Rules
		result = this->SortProductionRules(result);
	}

	return result;
}

void S0LProblemV3::Decode2(array<Int32>^ RuleLengths)
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

	this->ComputeMetrics(this->successorsBest, odds);

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

void S0LProblemV3::Decode(array<Int32>^ RuleLengths)
{
	this->Decode_L(RuleLengths);
}

void S0LProblemV3::Decode_PoL(array<Int32>^ RuleLengths)
{
	double fitness = 0.0;

	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);

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
	while ((!done) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		String^ successor = "";
		Int32 length = 0;
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			Int32 iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

			while ((!found) && (iSuccessor < successors[iSymbol]->Count))
			{
				if (successors[iSymbol][iSuccessor]->successor == this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (successors[iSymbol][iSuccessor]->successor->Length - 1)))
				{
					found = true;
					successor = successors[iSymbol][iSuccessor]->successor;
#if _PHASE3_S0L_DECODE_ > 1
					Console::WriteLine("For " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif
				}
				iSuccessor++;
			}

			if (!found)
			{
				successor = this->Assess_Dbl_ConsumeGeneStep_PoL2(RuleLengths[iGene], iGen, iPos, iScan, reserveRightMax, reserveRightMin);
				//successor = this->Assess_Dbl_ConsumeGeneStep_L(RuleLengths[iGene], iGen, iPos, iScan);
				iGene++;
			}
		}
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}

		successors = this->AddSuccessor(iGen, iPos, successors, successor);

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

	bool softFail = false;
	while ((!done) && (!softFail))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		String^ successor = "";
		Int32 length = 0;
		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;
			Int32 iSymbol = this->factOverlay[iGen][iPos]->sac->iSymbol;

			while ((!found) && (iSuccessor < successors[iSymbol]->Count))
			{
				if (successors[iSymbol][iSuccessor]->successor == this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + (successors[iSymbol][iSuccessor]->successor->Length - 1)))
				{
					found = true;
					successor = successors[iSymbol][iSuccessor]->successor;
#if _PHASE3_S0L_VERBOSE_ > 1
					Console::WriteLine("For " + this->model->alphabet->GetSymbol(iSymbol) + " (" + iGen + "," + iPos + ") Successor found " + successor);
#endif
				}
				iSuccessor++;
			}

			if (!found)
			{
				softFail = true;
			}
		}
		else
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
		}

		successors = this->AddSuccessor(iGen, iPos, successors, successor);

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

	// Compute the remaining successors using Greedy Algorithm
	if (softFail)
	{
		successors = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram, successors, iGen, iPos);
	}

	double odds = this->ComputeSuccessorsOdds(successors);

	Console::Write("Decoding: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}

	this->ComputeMetrics(successors, odds);

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

	Console::WriteLine();

#if _PHASE3_S0L_DECODE_ > 0
	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}

void S0LProblemV3::Decode_L(array<Int32>^ RuleLengths)
{
	double fitness = 0.0;

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
	bool done = false;
	bool failed = false;
	Int32 iSymbol;
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		iSymbol = this->model->evidence[iGen]->symbolIndices[iPos];
		String^ successor = "";
		Int32 length = 0;
		if ((iPos < this->model->evidence[iGen]->Length() - 1) && (!this->model->alphabet->IsTurtle(iSymbol)))
		{
			// Check to see if the current symbol matches an existing successor
			// Maybe look ahead too?
			bool found = false;
			Int32 iSuccessor = 0;

			successor = this->Assess_Dbl_PeekAheadStep_L(successors, iSymbol, iGen, iPos, iScan,0);

			if (successor == "")
			{
				successor = this->Assess_Dbl_ConsumeGeneStep_L(RuleLengths[iGene], iGen, iPos, iScan);
				iGene++;
			}
			else
			{
				fitness++;
			}
		}
		else if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
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
		else
		{
#if _PHASE3_S0L_VERBOSE_ > 1
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

	// Compute the remaining successors using Greedy Algorithm
	double odds = this->ComputeSuccessorsOdds(successors);

	Console::Write("Decoding: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}

	this->ComputeMetrics(successors, odds);

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

	Console::WriteLine();

#if _PHASE3_S0L_DECODE_ > 1
	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}

void S0LProblemV3::ComputeModelOdds()
{
	double result = 1.0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		Int32 total = 0;

		for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
		{
			total += this->model->evidence[iGen]->parikhVector[iSymbol];
		}

		for (size_t iSuccessor = 0; iSuccessor < this->model->productions[0]->rules[iSymbol]->odds->Count; iSuccessor++)
		{
			float o = (float)this->model->productions[0]->rules[iSymbol]->odds[iSuccessor] / 100;
			result *= Math::Pow(o, total*o);
		}
	}

#if _PHASE3_VERBOSE_ >= 1
	Console::WriteLine("Model odds ~= " + result);
#endif
	this->successTarget = result;
}

double S0LProblemV3::EstimateTerminationFitness()
{
	double result = 1.0;
	float odds2 = 0.01;
	float odds3 = 0.01;
	float odds1 = 1.00 - odds2 - odds3;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		Int32 total = 0;

		for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
		{
			total += this->model->evidence[iGen]->parikhVector[iSymbol];
		}

		result *= Math::Pow(odds1, total*odds1) * Math::Pow(odds2, total*odds2) * Math::Pow(odds3, total*odds3);
	}

	return result;
}

void S0LProblemV3::OverrideSearchSpace(Int32 Index, Int32 Value)
{
	//Console::WriteLine("Skipping Index " + Index + " to " + Value);
	//Console::WriteLine("Paused");
	//Console::ReadLine();

	this->aiBF->Override(Index, Value);
	//if (Value <= this->absoluteMaxLength)
	//{
	//	this->aiBF->Override(Index, Value);
	//	this->aiBF->Override(Index + 1, -1);
	//}
	//else
	//{
	//	this->aiBF->Override(Index - 2, 0);
	//	this->aiBF->Override(Index - 1, -1);
	//}

	//for (size_t i = Index + 2; i < this->numGenes; i++)
	//{
	//	if (i % 2 == 0)
	//	{
	//		this->aiBF->Override(i, 0);
	//	}
	//	else
	//	{
	//		this->aiBF->Override(i, -1);
	//	}
	//}
}

void S0LProblemV3::Solve()
{
	this->ComputeModelOdds();
	this->fitnessTarget = this->EstimateTerminationFitness();

	auto startTime = Time::now();
	SolutionType^ solution = this->SolveProblemGreedyBF();
	//SolutionType^ solution = this->SolveProblemGreedyGA();

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	this->Decode2(solution->solutions[0]);
}

SolutionType^ S0LProblemV3::SolveProblemBF()
{
	BruteForceSearch^ ai;
	GenomeConfiguration<Int32>^ genomeConfig;
	genomeConfig = this->CreateGenome();
	BruteForceSearchConfiguration^ configuration = this->CreateSearchSpace(genomeConfig);

	ai = gcnew BruteForceSearch(configuration);
	ai->problem = this;

	SolutionType^ S = gcnew SolutionType();
	S->solutions = gcnew array<array<Int32>^>(1);
	S->fitnessValues = gcnew array<float>(1);

	S->solutions[0] = ai->Solve();
	//S->solutions[0] = ai->SolveAllSolutions();
	S->fitnessValues[0] = this->Assess(S->solutions[0]);

	//for (size_t i = 0; i <= S->solutions[0]->GetUpperBound(0); i++)
	//{
	//	Console::Write(S->solutions[0][i] + ",");
	//}
	//Console::WriteLine();
	//Console::ReadLine();

	return S;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_Down_Left2Right(array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 StartGen, Int32 StartPos)
{
	array<List<SuccessorCount^>^>^ successors = Successors;
	float oddsCurrent = 1.0;

	Int32 iGen = StartGen;
	Int32 iPos = StartPos;

	while (iGen < this->model->generations - 1)
	{
		Int32 reserveRightMin = 0;
		Int32 reserveRightMax = 0;
		Int32 iScan = 0;

		for (size_t jPos = iPos+1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
			reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
		}

		while (iPos < this->model->evidence[iGen]->Length())
		{
			Int32 length = this->ComputeGreedySuccessor(iGen, iPos, iScan, 1.0, H, successors, reserveRightMin, reserveRightMax, 1);
			//Console::WriteLine();

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
			iPos++;
		}

		iGen++;
		iPos = 0;
	}

	return successors;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_Down_Left2Right(array<List<SuccessorCount^>^>^ H)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	float oddsCurrent = 1.0;

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Int32 reserveRightMin = 0;
		Int32 reserveRightMax = 0;
		Int32 iScan = 0;

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
			reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
		}

		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 length = this->ComputeGreedySuccessor(iGen, iPos, iScan, 1.0, H, successors, reserveRightMin, reserveRightMax, 1);
			//Console::WriteLine();

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
		}
	}

	return successors;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_Up_Left2Right(array<List<SuccessorCount^>^>^ H)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	float oddsCurrent = 1.0;

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	for (size_t iGenX = 1; iGenX < this->model->generations - 1; iGenX++)
	{
		Int32 iGen = this->model->generations - 1 - iGenX;
		Int32 reserveRightMin = 0;
		Int32 reserveRightMax = 0;
		Int32 iScan = 0;

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
			reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
		}

		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 length = this->ComputeGreedySuccessor(iGen, iPos, iScan, 1.0, H, successors, reserveRightMin, reserveRightMax, 1);
			//Console::WriteLine();

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
		}
	}

	return successors;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_Down_Right2Left(array<List<SuccessorCount^>^>^ H)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	float oddsCurrent = 1.0;

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Int32 reserveLeftMin = 0;
		Int32 reserveLeftMax = 0;
		Int32 iScan = 0;

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveLeftMin += this->factOverlay[iGen][jPos]->length->min;
			reserveLeftMax += this->factOverlay[iGen][jPos]->length->max;
		}

		for (size_t iPosX = 0; iPosX < this->model->evidence[iGen]->Length(); iPosX++)
		{
			Int32 iPos = this->model->evidence[iGen]->Length() - 1 - iPosX;
			Int32 length = this->ComputeGreedySuccessor_Right2Left(iGen, iPos, iScan, 1.0, H, successors, reserveLeftMin, reserveLeftMax, 1);
			//Console::WriteLine();

			reserveLeftMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveLeftMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
		}
	}

	return successors;
}

array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_Up_Right2Left(array<List<SuccessorCount^>^>^ H)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	float oddsCurrent = 1.0;

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	for (size_t iGenX = 1; iGenX < this->model->generations - 1; iGenX++)
	{
		Int32 iGen = this->model->generations - 1 - iGenX;
		Int32 reserveLeftMin = 0;
		Int32 reserveLeftMax = 0;
		Int32 iScan = 0;

		for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
		{
			reserveLeftMin += this->factOverlay[iGen][jPos]->length->min;
			reserveLeftMax += this->factOverlay[iGen][jPos]->length->max;
		}

		for (size_t iPosX = 0; iPosX < this->model->evidence[iGen]->Length(); iPosX++)
		{
			Int32 iPos = this->model->evidence[iGen]->Length() - 1 - iPosX;
			Int32 length = this->ComputeGreedySuccessor_Right2Left(iGen, iPos, iScan, 1.0, H, successors, reserveLeftMin, reserveLeftMax, 1);
			//Console::WriteLine();

			reserveLeftMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveLeftMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
		}
	}

	return successors;
}


array<List<SuccessorCount^>^>^ S0LProblemV3::ComputeSuccessors_Greedy_SingleGeneration(array<List<SuccessorCount^>^>^ H, Int32 iGen)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	float oddsCurrent = 1.0;

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	Int32 reserveRightMin = 0;
	Int32 reserveRightMax = 0;
	Int32 iScan = 0;
	bool failed = false;

	for (size_t jPos = 1; jPos < this->model->evidence[iGen]->Length(); jPos++)
	{
		reserveRightMin += this->factOverlay[iGen][jPos]->length->min;
		reserveRightMax += this->factOverlay[iGen][jPos]->length->max;
	}

	for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
	{
		Int32 length = this->ComputeGreedySuccessor(iGen, iPos, iScan, 1.0, H, successors, reserveRightMin, reserveRightMax, 1);

		if (length != -1)
		{
			//Console::WriteLine();

			reserveRightMin -= this->factOverlay[iGen][iPos]->length->min;
			reserveRightMax -= this->factOverlay[iGen][iPos]->length->max;
			iScan += length;
		}
		else
		{
			Console::WriteLine("No successor found");
			failed = true;
			break;
		}
	}

	if (failed)
	{
		successors = nullptr;
	}

	return successors;
}

SolutionType^ S0LProblemV3::SolveProblemGreedyBF()
{
	auto startTime = Time::now();

	GenomeConfiguration<Int32>^ genomeConfig = this->CreateGenome_GreedyBF();
	BruteForceSearchConfiguration^ config = this->CreateSearchSpace(genomeConfig);

	this->aiBF = gcnew BruteForceSearchR_Dbl_PMIT(config);
	this->aiBF->problem = this;
	this->aiBF->preferHighFitness = true;
	this->aiBF->fitnessTarget = this->cFitnessTarget;

	SolutionType^ S = gcnew SolutionType();
	S->solutions = gcnew array<array<Int32>^>(1);
	S->fitnessValues = gcnew array<float>(1);

	S->solutions[0] = this->aiBF->Solve();
	S->fitnessValues[0] = this->Assess_Dbl(S->solutions[0]);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

#if _PHASE3_S0L_VERBOSE_ >= 1
	Console::WriteLine("Solve time = " + this->solveTime + " ms");
#endif

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedyGA()
{
	GeneticAlgorithmP_Dbl<Int32>^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

	genomeConfig = this->CreateGenome_GreedyBF();
	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
	configuration->selection = SelectionType::RouletteWheel;
	configuration->survival = SurvivalType::UniqueElite;
	configuration->populationSizeInit = 100;
	configuration->populationSizeMax = 100;
	configuration->crossoverWeight = 0.80;
	configuration->mutationWeight = 0.01;
	configuration->preferHighFitness = true;

	ai = gcnew GeneticAlgorithmP_Dbl<Int32>(configuration);
	ai->problem = this;
	Int32 limit = (genomeConfig->numGenes+1) * 2000;
	//Int32 limit = ((Math::Pow(10, genomeConfig->numGenes)) / configuration->populationSizeInit) * 0.75;

	//ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(limit, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementStrict<GeneticAlgorithmState^>(limit, 2));
	//ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitnessHigh<GeneticAlgorithmState^>(this->fitnessTarget));
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(2500000));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;

	SolutionType^ S = gcnew SolutionType();

	auto startTime = Time::now();
	S->solutions = ai->SolveAllSolutions();
	S->fitnessValues = ai->FitnessValues();
	auto endTime = Time::now();

	Console::WriteLine("Terminated after " + ai->state->generationCurrent + " generations.");

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();
	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedy_V1()
{
	SolutionType^ S = gcnew SolutionType;

	auto startTime = Time::now();

	array<List<SuccessorCount^>^>^ successors = this->ComputeSuccessors_Greedy_Up_Left2Right(this->histogram);
	this->ComputeSuccessorsOdds_SingleGenerationOnly(successors);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();
	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedy_V2()
{
	SolutionType^ S = gcnew SolutionType;

	auto startTime = Time::now();
	array<array<List<SuccessorCount^>^>^>^ successorsByGen = gcnew array<array<List<SuccessorCount^>^>^>(this->model->generations - 1);

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		successorsByGen[iGen] = this->ComputeSuccessors_Greedy_SingleGeneration(this->histogram, iGen);
		this->ComputeSuccessorsOdds_SingleGenerationOnly(successorsByGen[iGen]);
	}

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();
	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedy_V1_1()
{
	SolutionType^ S = gcnew SolutionType;

	auto startTime = Time::now();

	this->preferredLength = 5;
	array<List<SuccessorCount^>^>^ successors = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram);
	this->ComputeSuccessorsOdds_SingleGenerationOnly(successors);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();
	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedyNash_V1()
{
	SolutionType^ S = gcnew SolutionType();

	auto startTime = Time::now();
	double odds = 0.0;
	bool stop = false;

	array<List<SuccessorCount^>^>^ bestULR;
	array<List<SuccessorCount^>^>^ bestURL;
	array<List<SuccessorCount^>^>^ bestDLR;
	array<List<SuccessorCount^>^>^ bestDRL;
	double oddsULR = 0.0;
	double oddsURL = 0.0;
	double oddsDLR = 0.0;
	double oddsDRL = 0.0;

	array<List<SuccessorCount^>^>^ combined;
	
	for (Int16 iLength = 2; iLength <= this->absoluteMaxLength; iLength++)
	{
		this->preferredLength = iLength;
		Console::WriteLine("Preferred Length = " + this->preferredLength);

		for (Int16 jLength = 1; jLength <= this->absoluteMaxLength; jLength++)
		{
			this->lengthValues[jLength] = this->absoluteMaxLength - Math::Abs(jLength - this->preferredLength);
		}

		this->histogram = this->ComputeSuccessorHistogram();
		array<List<SuccessorCount^>^>^ successorsULR = this->ComputeSuccessors_Greedy_Up_Left2Right(this->histogram);
		array<List<SuccessorCount^>^>^ successorsURL = this->ComputeSuccessors_Greedy_Up_Right2Left(this->histogram);
		array<List<SuccessorCount^>^>^ successorsDLR = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram);
		array<List<SuccessorCount^>^>^ successorsDRL = this->ComputeSuccessors_Greedy_Down_Right2Left(this->histogram);
		double tmp = this->ComputeSuccessorsOdds(successorsULR);
		if (tmp > oddsULR)
		{
			bestULR = successorsULR;
			oddsULR = tmp;
		}

		tmp = this->ComputeSuccessorsOdds(successorsURL);
		if (tmp > oddsURL)
		{
			bestURL = successorsURL;
			oddsURL = tmp;
		}

		tmp = this->ComputeSuccessorsOdds(successorsDLR);
		if (tmp > oddsDLR)
		{
			bestDLR = successorsDLR;
			oddsDLR = tmp;
		}

		tmp = this->ComputeSuccessorsOdds(successorsDRL);
		if (tmp > oddsDRL)
		{
			bestDRL = successorsDRL;
			oddsDRL = tmp;
		}

		if (combined == nullptr)
		{
			combined = successorsULR;
		}
		else
		{
			combined = this->CombineSuccessorCounts(combined, successorsULR);
		}

		combined = this->CombineSuccessorCounts(combined, successorsURL);
		combined = this->CombineSuccessorCounts(combined, successorsDLR);
		combined = this->CombineSuccessorCounts(combined, successorsDRL);
	}

	//combined = this->CombineSuccessorCounts(bestULR, bestURL);
	//combined = this->CombineSuccessorCounts(combined, bestDLR);
	//combined = this->CombineSuccessorCounts(combined, bestDRL);

	for (size_t i = 0; i <= this->lengthValues->GetUpperBound(0); i++)
	{
		this->lengthValues[i] = this->absoluteMaxLength;
	}

	this->histogram = this->ComputeSuccessorHistogram(combined);

	array<List<SuccessorCount^>^>^ best;

	while (!stop)
	{
		array<List<SuccessorCount^>^>^ successorsULR = this->ComputeSuccessors_Greedy_Up_Left2Right(this->histogram);
		array<List<SuccessorCount^>^>^ successorsURL = this->ComputeSuccessors_Greedy_Up_Right2Left(this->histogram);
		array<List<SuccessorCount^>^>^ successorsDLR = this->ComputeSuccessors_Greedy_Down_Left2Right(this->histogram);
		array<List<SuccessorCount^>^>^ successorsDRL = this->ComputeSuccessors_Greedy_Down_Right2Left(this->histogram);
		array<List<SuccessorCount^>^>^ combined = this->CombineSuccessorCounts(successorsULR, successorsURL);
		combined = this->CombineSuccessorCounts(combined, successorsDLR);
		combined = this->CombineSuccessorCounts(combined, successorsDRL);
		stop = true;
		double tmp = this->ComputeSuccessorsOdds(successorsULR);
		if (tmp > odds)
		{
			odds = tmp;
			stop = false;
			best = successorsULR;
		}
		tmp = this->ComputeSuccessorsOdds(successorsURL);
		if (tmp > odds)
		{
			odds = tmp;
			stop = false;
			best = successorsURL;
		}
		tmp = this->ComputeSuccessorsOdds(successorsDLR);
		if (tmp > odds)
		{
			odds = tmp;
			stop = false;
			best = successorsDLR;
		}
		tmp = this->ComputeSuccessorsOdds(successorsDRL);
		if (tmp > odds)
		{
			odds = tmp;
			stop = false;
			best = successorsDRL;
		}

		if (!stop)
		{
			this->histogram = this->ComputeSuccessorHistogram(combined);
		}
	}

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	this->ComputeMetrics(best, odds);

	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}

SolutionType^ S0LProblemV3::SolveProblemGreedyNash_V2()
{
	SolutionType^ S = gcnew SolutionType;

	auto startTime = Time::now();
	array<array<array<List<SuccessorCount^>^>^>^>^ successorsByLengthGen = gcnew array<array<array<List<SuccessorCount^>^>^>^>(this->absoluteMaxLength);
	array<double,2>^ oddsByLengthGen = gcnew array<double,2>(this->absoluteMaxLength, this->model->generations-1);
	array<double>^ oddsByGen = gcnew array<double>(this->model->generations - 1);

	for (size_t iLength = 2; iLength <= this->absoluteMaxLength; iLength++)
	{
		Console::WriteLine("For preferred length = " + iLength);
		this->preferredLength = iLength;
		this->histogram = this->ComputeSuccessorHistogram();
		array<array<List<SuccessorCount^>^>^>^ successorsByGen = gcnew array<array<List<SuccessorCount^>^>^>(this->model->generations - 1);
		for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
		{
			array<List<SuccessorCount^>^>^ tmp = this->ComputeSuccessors_Greedy_SingleGeneration(this->histogram, iGen);
			if (tmp != nullptr)
			{
				successorsByGen[iGen] = tmp;
				oddsByLengthGen[iLength - 1, iGen] = this->ComputeSuccessorsOdds_SingleGenerationOnly(successorsByGen[iGen]);
			}

			//if (odds > oddsByGen[iGen])
			//{
			//	successorsByGen[iGen] = tmp;
			//	oddsByGen[iGen] = odds;
			//}
		}
		successorsByLengthGen[iLength-1] = successorsByGen;
	}

	Console::WriteLine();

	//for (size_t iGen1 = 0; iGen1 < this->model->generations - 1; iGen1++)
	//{
	//	for (size_t iGen2 = 0; iGen2 < this->model->generations - 1; iGen2++)
	//	{
	//		array<List<SuccessorCount^>^>^ H = this->ComputeSuccessorHistogram(successorsByGen[iGen1]);
	//		array<List<SuccessorCount^>^>^ adjusted = this->ComputeSuccessors_Greedy_SingleGeneration(H, iGen2);
	//		this->ComputeSuccessorsOdds_SingleGenerationOnly(adjusted);
	//	}
	//}

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();
	Console::WriteLine("Solve time = " + this->solveTime + " ms");

	return S;
}
