#include "stdafx.h"
#include "EDCProblemV3.h"
#include "EDCProblemV3_SuccessorSearch.h"



EDCProblemV3::EDCProblemV3(ModelV3^ M, ProblemType T) : LSIProblemV3(M,T)
{
}


EDCProblemV3::~EDCProblemV3()
{
}

void EDCProblemV3::Initialize()
{
	this->Pr = 0.25;
	this->Pi = 0.0;
	this->Pd = 0.0;
	this->replacementAllowed = (this->Pr > 0);
	this->insertionAllowed = (this->Pi > 0);
	this->deletionAllowed = (this->Pd > 0);
	this->model->CreateError(1.0 - this->Pr - this->Pi - this->Pd, this->Pr, this->Pi, this->Pd, this->replacementAllowed, this->insertionAllowed, this->deletionAllowed);
	this->model->Filter();
	this->model->CreateOverlays();

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

	this->MAO->Display();
	this->MAO->ComputeSolutionSpaceSize();
	this->MAO->DisplaySolutionSpaces();
	//Console::WriteLine("Paused");
	//Console::ReadLine();
}

void EDCProblemV3::Analyze()
{
	// Analyze the alphabet
	this->ComputeSymbolLexicons();
	Int32 pass = 0;
	this->firstPass = true;
	do
	{
		this->MAO->ResetChangeFlag();

//		if (pass >= cPass_ComputeLength_TotalSymbolProduction)
//		{
//			// Rule #X - Infer min/max total length based on the total symbol production
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeLength_TotalSymbolProduction(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
//			}
//#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeTotalLength_TotalSymbolProduction)
//		{
//			// Rule #X - Infer total length from the total symbol production
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeTotalLength_TotalSymbolProduction(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
//			}
//#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeTotalLength_Symbiology)
//		{
//			this->MAO->partialTotalLengthFacts = gcnew List<TotalLengthFact^>; // used to store lengths for all the new SACS that showed in a generation
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeTotalLength_Symbiology(iGen);
//			}
//#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeTotalLength_Length)
//		{
//			// Rule #X - Infer total length from the individual min/max length
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeTotalLength_Length(iGen);
//			}
//#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeLength_TotalLength)
//		{
//			// Rule #X - Infer length based on the Min/Max Total Length
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeLength_TotalLength(iGen);
//			}
//#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeLength_Symbiology)
//		{
//			// Rule #X - Infer length based on the Symbiology
//			this->ComputeLength_Symbiology();
//#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeGrowth_Symbiology)
//		{
//			// Rule #X - Infer growth from symbiology
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeGrowth_Symbiology(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
//			}
//#if _PHASE3_COMPUTE_GROWTH_SYMBIOLOGY_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeGrowth_UnaccountedGrowth)
//		{
//			// Rule #X - Infer growth based on unaccounted symbols
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeGrowth_UnaccountedGrowth(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
//			}
//#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeGrowth_TotalGrowth)
//		{
//			// Rule #X - Infer growth based on total growth
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeGrowth_TotalGrowth(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
//			}
//#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeLength_Growth)
//		{
//			// Rule #X - Infer length based on growth
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeLength_Growth(iGen);
//			}
//#if	_PHASE3_COMPUTE_LENGTH_GROWTH_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeFragments_Position)
//		{
//			// Rule #X - Find fragments
//			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
//			{
//				this->ComputeFragments_Position(iGen, this->model->evidence[iGen], iGen + 1, this->model->evidence[iGen + 1]);
//			}
//#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
//			this->MAO->Display();
//#endif
//
//			//			// Rule #X - Find fragments
//			//			for (Int16 iGen = this->model->generations - 2; iGen >= 0; --iGen)
//			//			{
//			//				this->ComputeFragments_Position(iGen, this->model->evidence[iGen], iGen + 1, this->model->evidence[iGen + 1]);
//			//#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
//			//				this->MAO->Display();
//			//#endif
//			//			}
//		}
//
//		if (pass >= cPass_ComputeFragments_Cache)
//		{
//			this->ComputeBTF_Cache();
//#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 1
//			this->MAO->Display();
//#endif
//			this->ComputeLength_Fragment();
//			this->ComputeGrowth_Fragment();
//			//this->ComputeFragments_FactBTF();
//#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//		if (pass >= cPass_ComputeSymbolLocalization)
//		{
//			//this->MAO->Write();
//			this->ComputeSymbolLocalization();
//			this->MAO->Display();
//		}
//
//		//		this->ComputeLexicon();
//		//
//		//		// Rule # X - Compute the growth from the lexicon
//		//		this->ComputeGrowth_Lexicon();
//		//#if _PHASE3_COMPUTE_GROWTH_LEXICON_ >= 1
//		//		this->MAO->Display();
//		//#endif
//		//		// Rule # X - Compute the length from the fragments
//		//		this->ComputeLength_Lexicon();
//		//#if	_PHASE3_COMPUTE_LENGTH_LEXICON_ >= 1
//		//		this->MAO->Display();
//		//#endif
//
//		if (pass >= cPass_ComputeFragments_Revise)
//		{
//			this->ComputeFragments_Revise();
//#if _PHASE3_COMPUTE_FRAGMENTS_MARKERS_ >= 1
//			this->MAO->Display();
//#endif
//		}
//
//#if _PHASE3_VERBOSE_ >= 1
//		Console::WriteLine("Analysis Pass Complete");
//#endif
//		this->MAO->Display();
		pass++;
		firstPass = false;
	} while (this->MAO->HasChangeOccured() && pass <= cPass_Minimum);
}

float EDCProblemV3::Assess(array<Int32>^ RuleLengths)
{
	float fitness = 0.0;

#if _PMI_PROBLEM_CHEAT_ > 0
	RuleLengths = this->Cheat(RuleLengths);
#endif

	this->successors = this->CreateSuccessorsByScanning(RuleLengths);
	EDCProblemV3_SuccessorSearch^ p = gcnew EDCProblemV3_SuccessorSearch(this->model, LSIProblemV3::ProblemType::Master, this->successors);

	List<ProductionRuleV3^>^ modelRules = p->SolveForRules();

	if (modelRules != nullptr)
	{
#if _PMI_PROBLEM_VERBOSE_ >= 0
		Console::WriteLine(" Rules: ");
		for each (ProductionRuleV3^ r in modelRules)
		{
			r->Display();
		}
		Console::WriteLine();
#endif 
		fitness = this->AssessProductionRules(modelRules);
	}
	else
	{
		fitness = 999.0;
	}

	return fitness;
}

float EDCProblemV3::AssessProductionRules(List<ProductionRuleV3^>^ Rules)
{
	float result = 0.0;

	return result;
}

GenomeConfiguration<Int32>^ EDCProblemV3::CreateGenome()
{
	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(1,1,2);

	return result;
}

List<ProductionRuleV3^>^ EDCProblemV3::CreateRulesByScanning(array<Int32>^ RuleLengths)
{
	List<ProductionRuleV3^>^ result = gcnew List<ProductionRuleV3^>;

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		Int32 iScan = 0;
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 iTable = 0;
			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);
			Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);
			Int32 iFact = this->MAO->GetFactIndex(f);
			String^ successor = "";

			// if there are sufficient symbols left to form a successor grab it
			if (iScan + RuleLengths[this->rule2solution[iFact]] <= this->model->evidence[iGen + 1]->Length())
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1);
			}
			else
			{// otherwise grab the last N symbols
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1);
			}

			//Console::WriteLine("For symbol " + this->model->evidence[iGen]->GetSymbol(iPos) + " found successor " + successor);

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
					r->successor->Add(successor);
					result->Add(r);
				}
			}
			else
			{
				found = false;
				Int32 iSuccessor = 0;

				while (!found && iSuccessor < result[iRule]->successor->Count)
				{
					if (result[iRule]->successor[iSuccessor] == successor)
					{
						found = true;
					}
					else
					{
						iSuccessor++;
					}
				}

				if (!found)
				{
					result[iRule]->contextLeft->Add(this->model->alphabet->GetSymbol(iLeft));
					result[iRule]->contextRight->Add(this->model->alphabet->GetSymbol(iRight));
					result[iRule]->condition->Add("*");
					result[iRule]->odds->Add(100);
					if (iScan + RuleLengths[this->rule2solution[iFact]] - 1 <= this->model->evidence[iGen + 1]->Length() - 1)
					{
						result[iRule]->successor->Add(successor);
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
		}
	}

	result = this->SortProductionRules(result);

	return result;
}

List<ProductionRuleV3^>^ EDCProblemV3::CreateRulesByScenario(array<List<SuccessorCount^>^>^ Successors, array<Int32>^ Scenario)
{
	List<ProductionRuleV3^>^ result = gcnew List<ProductionRuleV3^>;

	result = this->SortProductionRules(result);

	return result;

}

array<List<SuccessorCount^>^>^ EDCProblemV3::CreateSuccessorsByScanning(array<Int32>^ RuleLengths)
{
	array<List<SuccessorCount^>^>^ successors = gcnew array<List<SuccessorCount^>^>(this->MAO->factsList->Count);
	for (size_t i = 0; i <= successors->GetUpperBound(0); i++)
	{
		successors[i] = gcnew List<SuccessorCount^>;
	}

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Int32 iScan = 0;
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 iTable = 0;
			Int32 iSymbol = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);
			Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);
			Int32 iFact = this->MAO->GetFactIndex(f);
			String^ successor = "";

			// if there are sufficient symbols left to form a successor grab it
			if (iScan + RuleLengths[this->rule2solution[iFact]] <= this->model->evidence[iGen + 1]->Length())
			{
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1);
			}
			else
			{// otherwise grab the last N symbols
				successor = this->model->evidence[iGen + 1]->GetSymbolsFromPos(iScan, iScan + RuleLengths[this->rule2solution[iFact]] - 1);
			}

			//Console::WriteLine("For symbol " + this->model->evidence[iGen]->GetSymbol(iPos) + " found successor " + successor);

			// check to see if a suitable rule already exists
			Int32 iRule = 0;
			bool found = false;

			while (!found && iRule < successors[this->rule2solution[iFact]]->Count)
			{
				if (successors[this->rule2solution[iFact]][iRule]->successor == successor)
				{
					found = true;
					successors[this->rule2solution[iFact]][iRule]->count++;
				}
				else
				{
					iRule++;
				}
			}

			if (!found)
			{
				SuccessorCount^ tmp = gcnew SuccessorCount();
				tmp->successor = successor;
				tmp->count = 1;
				successors[this->rule2solution[iFact]]->Add(tmp);
			}

			if (!this->model->alphabet->IsTurtle(iSymbol))
			{
				iScan += RuleLengths[this->rule2solution[iFact]];
			}
			else
			{
				iScan++;
			}
		}
	}

	for (size_t i = 1; i < successors->GetUpperBound(0); i++)
	{
		successors[i]->Sort(gcnew SuccessorOddsComparer());
	}

	return successors;
}

void EDCProblemV3::Solve()
{
	auto startTime = Time::now();
	SolutionType^ solution;

	array<Int32>^ RuleLengths = gcnew array<Int32>(2);
	RuleLengths[0] = 3;
	RuleLengths[1] = 3;
	this->rule2solution = gcnew List<Int32>;
	this->rule2solution->Add(0);
	this->rule2solution->Add(1);

	float fitness = this->Assess(RuleLengths);

	//SolutionType^ solution = this->SolveProblemGreedyNash_V1();

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	List<List<ProductionRuleV3^>^>^ ruleSets = this->CreateModelFromSolution(solution);

	for (size_t iSet = 0; iSet < ruleSets->Count; iSet++)
	{
		Console::WriteLine("Solution #" + iSet + 1);
		List<ProductionRuleV3^>^ rules = ruleSets[iSet];
		for (size_t iRule = 0; iRule < rules->Count; iRule++)
		{
			rules[iRule]->Display();
		}
	}

	Console::WriteLine("MTTS: " + this->analysisTime + this->solveTime + " Analzye Time: " + this->analysisTime + " Search Time: " + this->solveTime);
	System::IO::StreamWriter^ sw;

	sw->Close();

	//Console::WriteLine("Paused");
	//Console::ReadLine();
}
