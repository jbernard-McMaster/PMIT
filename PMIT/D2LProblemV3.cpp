#include "stdafx.h"
#include "D2LProblemV3.h"


D2LProblemV3::D2LProblemV3(ModelV3^ M, ProblemType T) : S0LProblemV3(M, T)
{
}


D2LProblemV3::~D2LProblemV3()
{
}

void D2LProblemV3::Initialize()
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
		this->absoluteMinLength = 1;
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
					Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
					Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

					Fact^ f = this->CreateBaseFact(iSymbol, iLeft, iRight);

#if _PHASE3_VERBOSE_ >= 1
					Console::WriteLine("Gen " + iGen + ": Adding fact for SAC " + this->MAO->SymbolCondition(iSymbol, iLeft, iRight));
#endif
					if (this->model->alphabet->successors[iSymbol] != AlphabetV3::Deterministic)
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

	//this->MAO->localization = gcnew array<array<Byte, 2>^>(this->model->generations - 1);
	//for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	//{
	//	this->MAO->localization[iGen] = gcnew array<Byte, 2>(this->model->evidence[iGen + 1]->Length(), this->model->evidence[iGen]->Length());
	//	for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen + 1]->Length(); iPos1++)
	//	{
	//		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen]->Length(); iPos2++)
	//		{
	//			this->MAO->localization[iGen][iPos1, iPos2] = MasterAnalysisObject::cLocalization_Unset;
	//		}
	//	}
	//}

	//this->MAO->Display();
	//this->MAO->ComputeSolutionSpaceSize();
	//this->MAO->DisplaySolutionSpaces();
	//Console::WriteLine("Paused");
	//Console::ReadLine();
}

void D2LProblemV3::Analyze()
{
	this->absoluteMaxLength = 10;
	this->maxGreedySteps = 3;
	this->preferredLength = 6;

	// Create the fact overlay
	this->factOverlay = gcnew array<List<Fact^>^>(this->model->generations - 1);
	this->factIndexOverlay = gcnew array<array<Int32>^>(this->model->generations - 1);
	this->factCount = gcnew array<Int32>(this->MAO->factsList->Count);
	this->fitnessBase = 0;

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		this->factOverlay[iGen] = gcnew List<Fact^>;
		this->factIndexOverlay[iGen] = gcnew array<Int32>(this->model->evidence[iGen]->Length());
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			this->fitnessBase++;
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			Int32 fIndex = this->MAO->GetFactIndex(f);
			factCount[fIndex]++;

			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				Fact^ f = this->CreateBaseFact(iSymbol, this->model->evidence[iGen]->GetLeftContext(iPos), this->model->evidence[iGen]->GetRightContext(iPos));
				this->factOverlay[iGen]->Add(f);
				this->factIndexOverlay[iGen][iPos] = fIndex;
			}
			else
			{
				this->factOverlay[iGen]->Add(this->MAO->GetFact(iGen, 0, iSymbol, this->model->evidence[iGen]->GetLeftContext(iPos), this->model->evidence[iGen]->GetRightContext(iPos), nullptr));
				this->factIndexOverlay[iGen][iPos] = fIndex;
			}
		}
	}

	this->avgLength = 0;

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

float D2LProblemV3::Assess(array<Int32>^ RuleLengths)
{
	return this->Assess_L(RuleLengths);
}

float D2LProblemV3::Assess_L(array<Int32>^ RuleLengths)
{
#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine();
#endif

	RuleLengths[0] = 3;
	RuleLengths[1] = 3;
	RuleLengths[2] = 3;
	RuleLengths[3] = 3;
	RuleLengths[4] = 3;
	RuleLengths[5] = 3;
	RuleLengths[6] = 3;

	float fitness = this->fitnessBase;
	
	array<String^>^ successors = gcnew array<String^>(this->MAO->factsList->Count);

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = "";
	}

	// Consume the genes for the first N symbols
	Int32 iPos = 0;
	Int32 iScan = 0;
	Int32 iGen = 0;
	Int32 iGene = 0;
	bool done = false;
	bool failed = false;
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		Int32 iSymbol;
		Int32 iLeft;
		Int32 iRight;
		Int32 iFact;
		String^ successor = "";
		Int32 length = 0;
		// Check to see if current successor is set
		iFact = this->factIndexOverlay[iGen][iPos];

		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			if (successors[iFact] == "")
			{
				Fact^ f = this->MAO->factsList[iFact];
				// If not consume a gene to set it
				successor = this->Assess_Dbl_ConsumeGeneStep_L(RuleLengths[iGene], iGen, iPos, iScan);
				fitness--;
				iGene++;
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("By value " + RuleLengths[iGene] + " for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor found " + successor);
#endif
			}
			else
			{
				// If so then check to see if the current symbol matches an the successor
				successor = this->Assess_Dbl_ConsumeGeneStep_L(successors[iFact]->Length, iGen, iPos, iScan);
				if (successor != successors[iFact])
				{
					// if not then fail out
					failed = true;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor mismatch " + successor + " != " + successors[iFact]);
#endif
				}
				else
				{
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor found " + successor);
#endif
					fitness--;
				}
			}
		}
		else
		{
			if (successors[iFact] == "")
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
				fitness--;
			}
			else
			{
				// If so then check to see if the current symbol matches an the successor
				successor = this->Assess_Dbl_ConsumeGeneStep_L(successors[iFact]->Length, iGen, iPos, iScan);
				if (successor != successors[iFact])
				{
					// if not then fail out
					failed = true;
				}
				else
				{
					fitness--;
				}
			}
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Last symbol " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") get successor " + successor);
#endif
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors[iFact] = successor;
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

	if (failed)
	{
		fitness *= 2;
	}

#if _PHASE3_S0L_VERBOSE_ >= 0
	if (fitness < this->fitnessBest)
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
			Console::WriteLine(this->MAO->SymbolCondition(this->MAO->factsList[i]->sac->iSymbol, this->MAO->factsList[i]->sac->iLeft, this->MAO->factsList[i]->sac->iRight) + " -> " + successors[i]->PadRight(11));
		}

		Console::WriteLine("Paused");
		Console::ReadLine();
	}
#endif

	return fitness;
}

GenomeConfiguration<Int32>^ D2LProblemV3::CreateGenome_GreedyBF()
{
	Int32 numGenes = this->MAO->factsList->Count;
	Int32 iGene = 0;
	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	while (iGene < numGenes)
	{
		min[iGene] = this->absoluteMinLength;
		max[iGene] = this->absoluteMaxLength;

		this->rule2solution->Add(iGene);
		this->solution2rule->Add(iGene);
		iGene++;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);
	return result;
}

void D2LProblemV3::Decode(array<Int32>^ RuleLengths)
{
	return this->Decode_L(RuleLengths);
}

void D2LProblemV3::Decode_L(array<Int32>^ RuleLengths)
{
#if _PHASE3_S0L_VERBOSE_ > 0
	Console::WriteLine();
#endif

	float fitness = this->fitnessBase;

	array<String^>^ successors = gcnew array<String^>(this->MAO->factsList->Count);

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = "";
	}

	// Consume the genes for the first N symbols
	Int32 iPos = 0;
	Int32 iScan = 0;
	Int32 iGen = 0;
	Int32 iGene = 0;
	bool done = false;
	bool failed = false;
	while ((!done) && (!failed) && (iGene <= RuleLengths->GetUpperBound(0)))
	{
		//Console::Write("Symbol #" + iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos));

		Int32 iSymbol;
		Int32 iLeft;
		Int32 iRight;
		Int32 iFact;
		String^ successor = "";
		Int32 length = 0;
		// Check to see if current successor is set
		iFact = this->factIndexOverlay[iGen][iPos];

		if (iPos < this->model->evidence[iGen]->Length() - 1)
		{
			if (successors[iFact] == "")
			{
				Fact^ f = this->MAO->factsList[iFact];
				// If not consume a gene to set it
				successor = this->Assess_Dbl_ConsumeGeneStep_L(RuleLengths[iGene], iGen, iPos, iScan);
				fitness--;
				iGene++;
#if _PHASE3_S0L_VERBOSE_ > 0
				Console::WriteLine("By value " + RuleLengths[iGene] + " for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor found " + successor);
#endif
			}
			else
			{
				// If so then check to see if the current symbol matches an the successor
				successor = this->Assess_Dbl_ConsumeGeneStep_L(successors[iFact]->Length, iGen, iPos, iScan);
				if (successor != successors[iFact])
				{
					// if not then fail out
					failed = true;
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor mismatch " + successor + " != " + successors[iFact]);
#endif
				}
				else
				{
#if _PHASE3_S0L_VERBOSE_ > 0
					Console::WriteLine("By peeking for " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") successor found " + successor);
#endif
					fitness--;
				}
			}
		}
		else
		{
			if (successors[iFact] == "")
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, this->model->evidence[iGen + 1]->Length() - 1);
				fitness--;
			}
			else
			{
				// If so then check to see if the current symbol matches an the successor
				successor = this->Assess_Dbl_ConsumeGeneStep_L(successors[iFact]->Length, iGen, iPos, iScan);
				if (successor != successors[iFact])
				{
					// if not then fail out
					failed = true;
				}
				else
				{
					fitness--;
				}
			}
#if _PHASE3_S0L_VERBOSE_ > 0
			Console::WriteLine("Last symbol " + this->MAO->SymbolCondition(this->MAO->factsList[iFact]->sac->iSymbol, this->MAO->factsList[iFact]->sac->iLeft, this->MAO->factsList[iFact]->sac->iRight) + " (" + iGen + "," + iPos + ") get successor " + successor);
#endif
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			successors[iFact] = successor;
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

	if (failed)
	{
		fitness *= 2;
	}

#if _PHASE3_S0L_VERBOSE_ >= 0
	Console::Write("Decoded: ");
	for (size_t i = 0; i <= RuleLengths->GetUpperBound(0); i++)
	{
		Console::Write(RuleLengths[i] + " ");
	}
	Console::WriteLine(", F = " + fitness.ToString());

	if (fitness == 0)
	{
		this->success = true;
		Console::WriteLine("SUCCESS");
	}
	else
	{
		this->success = false;
		Console::WriteLine("FAILED");
	}

	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		Console::WriteLine(this->MAO->SymbolCondition(this->MAO->factsList[i]->sac->iSymbol, this->MAO->factsList[i]->sac->iLeft, this->MAO->factsList[i]->sac->iRight) + " -> " + successors[i]->PadRight(11));
	}

	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}

void D2LProblemV3::Solve()
{
	this->fitnessBest = float::MaxValue;

	auto startTime = Time::now();

	this->ComputeLength_AbsoluteMinMax(); // These only need to be done once at initialization, it will never produce a better value afterwards.
	this->ComputeGrowth_AbsoluteMinMax();

	this->Analyze();

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->analysisTime = ms.count();

	// If a master problem, create sub-problems
	if (this->problemType == ProblemType::Master)
	{
		// Adjust the model for the primary problem
		ModelV3^ adjusted = this->model->Adjust(this->model->alphabet->numNonTurtles);

		D2LProblemV3^ p = gcnew D2LProblemV3(this->model, ProblemType::Primary);

		// For each secondary problem (graphical symbol) produce a secondary problem
		for (size_t iSymbol = this->model->alphabet->numNonTurtles; iSymbol < this->model->alphabet->numSymbols; iSymbol++)
		{
			// Adjust the model for the next graphical symbol
			D2LProblemV3^ p = gcnew D2LProblemV3(this->model, ProblemType::Secondary);
		}

		// Solve sub-problems

		// Re-analyze

	}

	// Solve master problem

	startTime = Time::now();
	//SolutionType^ solution = this->SolveProblemGreedyBF();
	SolutionType^ solution = this->SolveProblemGreedyGA();

	endTime = Time::now();

	duration = endTime - startTime;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	this->Decode(solution->solutions[0]);
}

SolutionType^ D2LProblemV3::SolveProblemGreedyBF()
{
	auto startTime = Time::now();

	GenomeConfiguration<Int32>^ genomeConfig = this->CreateGenome_GreedyBF();
	BruteForceSearchConfiguration^ config = this->CreateSearchSpace(genomeConfig);

	BruteForceSearch^ ai;
	ai = gcnew BruteForceSearch(config);
	ai->problem = this;
	ai->preferHighFitness = true;

	SolutionType^ S = gcnew SolutionType();
	S->solutions = gcnew array<array<Int32>^>(1);
	S->fitnessValues = gcnew array<float>(1);

	S->solutions[0] = ai->Solve();
	S->fitnessValues[0] = this->Assess(S->solutions[0]);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

#if _PHASE3_S0L_VERBOSE_ >= 1
	Console::WriteLine("Solve time = " + this->solveTime + " ms");
#endif

	return S;
}


SolutionType^ D2LProblemV3::SolveProblemGreedyGA()
{
	GeneticAlgorithmP<Int32>^ ai;
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
	configuration->preferHighFitness = false;

	ai = gcnew GeneticAlgorithmP<Int32>(configuration);
	ai->problem = this;
	Int32 limit = (genomeConfig->numGenes + 1) * 2000;

	//ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(limit, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementStrict<GeneticAlgorithmState^>(limit, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0));
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

