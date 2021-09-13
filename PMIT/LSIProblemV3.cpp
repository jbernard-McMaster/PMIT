#include "stdafx.h"
#include "LSIProblemV3.h"


LSIProblemV3::LSIProblemV3(ModelV3^ M, ProblemType T)
{
	this->model = M;
	this->problemType = T;

	this->Initialize();
}

LSIProblemV3::~LSIProblemV3()
{
}

void LSIProblemV3::Initialize()
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

	// Establish the base facts
	// Cannot compress the fact table yet until some raw facts are found by analysis
	if (this->model->parameters->Count == 0)
	{
		for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
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

	// Display SACs
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		this->MAO->DisplayFact(this->MAO->factsList[iFact]);
	}
	Console::WriteLine("Paused");
	Console::ReadLine();

	// Check for ambiguitity
	this->isAmbiguous = false;
	this->IsAmbiguous();

	if (!isAmbiguous)
	{
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
}

void LSIProblemV3::Analyze()
{
	// Analyze the alphabet
	this->ComputeSymbolLexicons();
	Int32 pass = 0;
	this->firstPass = true;
	this->firstPassBTF = true;
	do
	{
		this->MAO->ResetChangeFlag();

		if (pass >= cPass_ComputeLength_TotalSymbolProduction)
		{
			// Rule #X - Infer min/max total length based on the total symbol production
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeLength_TotalSymbolProduction(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
			}
#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeTotalLength_TotalSymbolProduction)
		{
			// Rule #X - Infer total length from the total symbol production
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeTotalLength_TotalSymbolProduction(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
			}
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeTotalLength_Symbiology)
		{
			this->MAO->partialTotalLengthFacts = gcnew List<TotalLengthFact^>; // used to store lengths for all the new SACS that showed in a generation
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeTotalLength_Symbiology(iGen);
			}
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeTotalLength_Length)
		{
			// Rule #X - Infer total length from the individual min/max length
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeTotalLength_Length(iGen);
			}
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
		this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeLength_TotalLength)
		{
			// Rule #X - Infer length based on the Min/Max Total Length
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeLength_TotalLength(iGen);
			}
#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeLength_Symbiology)
		{
			// Rule #X - Infer length based on the Symbiology
			this->ComputeLength_Symbiology();
#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeGrowth_Symbiology)
		{
			// Rule #X - Infer growth from symbiology
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeGrowth_Symbiology(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
			}
#if _PHASE3_COMPUTE_GROWTH_SYMBIOLOGY_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeGrowth_UnaccountedGrowth)
		{
			// Rule #X - Infer growth based on unaccounted symbols
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeGrowth_UnaccountedGrowth(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
			}
#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeGrowth_TotalGrowth)
		{
			// Rule #X - Infer growth based on total growth
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeGrowth_TotalGrowth(iGen, this->model->evidence[iGen], this->model->evidence[iGen + 1]);
			}
#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeLength_Growth)
		{
			// Rule #X - Infer length based on growth
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeLength_Growth(iGen);
			}
#if	_PHASE3_COMPUTE_LENGTH_GROWTH_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeFragments_Position)
		{
			// Rule #X - Find fragments
			for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
			{
				this->ComputeFragments_Position(iGen, this->model->evidence[iGen], iGen + 1, this->model->evidence[iGen + 1]);
				this->MAO->Display();
			}
#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
			this->MAO->Display();
#endif

//			// Rule #X - Find fragments
//			for (Int16 iGen = this->model->generations - 2; iGen >= 0; --iGen)
//			{
//				this->ComputeFragments_Position(iGen, this->model->evidence[iGen], iGen + 1, this->model->evidence[iGen + 1]);
//#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
//				this->MAO->Display();
//#endif
//			}
		}

		if (pass >= cPass_ComputeFragments_Cache)
		{
			this->ComputeBTF_Cache();
#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 1
			this->MAO->Display();
#endif
			this->ComputeLength_Fragment();
			this->ComputeGrowth_Fragment();
			//this->ComputeFragments_FactBTF();
#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 1
			this->MAO->Display();
#endif
		}

		if (pass >= cPass_ComputeSymbolLocalization)
		{
			//this->MAO->Write();
			this->ComputeSymbolLocalization();
			this->MAO->Display();
		}

//		this->ComputeLexicon();
//
//		// Rule # X - Compute the growth from the lexicon
//		this->ComputeGrowth_Lexicon();
//#if _PHASE3_COMPUTE_GROWTH_LEXICON_ >= 1
//		this->MAO->Display();
//#endif
//		// Rule # X - Compute the length from the fragments
//		this->ComputeLength_Lexicon();
//#if	_PHASE3_COMPUTE_LENGTH_LEXICON_ >= 1
//		this->MAO->Display();
//#endif

		if (pass >= cPass_ComputeFragments_Revise)
		{
			this->ComputeFragments_Revise();
#if _PHASE3_COMPUTE_FRAGMENTS_MARKERS_ >= 1
			this->MAO->Display();
#endif
		}

#if _PHASE3_VERBOSE_ >= 1
		Console::WriteLine("Analysis Pass Complete");
#endif
		this->MAO->Display();
		pass++;
		firstPass = false;
	} while (this->MAO->HasChangeOccured() && pass <= cPass_Minimum);
}

float LSIProblemV3::Assess(array<Int32>^ RuleLengths)
{
	float fitness = 0.0;

#if _PMI_PROBLEM_CHEAT_ > 0
	RuleLengths = this->Cheat(RuleLengths);
#endif

	List<ProductionRuleV3^>^ modelRules;

	modelRules = this->Assess_CreateRulesStep_RuleLengths(RuleLengths);

	if (modelRules != nullptr)
	{
#if _PMI_PROBLEM_VERBOSE_ >= 1
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif 
		fitness = this->AssessProductionRules(modelRules);
	}
	else
	{
		fitness = 3.0;
	}

	return fitness;
}

List<ProductionRuleV3^>^ LSIProblemV3::Assess_CreateRulesStep_RuleLengths(array<Int32>^ Solution)
{
	List<ProductionRuleV3^>^ R = gcnew List<ProductionRuleV3^>;
	//Solution[0] = 2;
	//Solution[4] = 2;
	R = this->CreateRulesByScanning(Solution);
	return R;
}

float LSIProblemV3::AssessProductionRules(List<ProductionRuleV3^>^ Rules)
{
	float result = 0.0;

	List<WordXV3^>^ solutionWords = gcnew List<WordXV3^>;
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		WordXV3^ w = EvidenceFactoryV3::Process(this->model->evidence[iGen], Rules, iGen);
		w = this->model->ComputeWordX(w);
		solutionWords->Add(w);
	}
	result = this->CompareEvidence(solutionWords);

	return result;
}

float LSIProblemV3::CompareEvidence(List<WordXV3^>^ SolutionWords)
{
	float result = 0.0;
	Int32 errors = 0;
	Int32 total = 0;

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		total += this->model->evidence[iGen + 1]->Length();
		if (SolutionWords[iGen] != this->model->evidence[iGen + 1])
		{
			Int32 length = Math::Min(SolutionWords[iGen]->Length(), this->model->evidence[iGen + 1]->Length());
			errors += Math::Abs(this->model->evidence[iGen + 1]->Length() - SolutionWords[iGen]->Length());

			for (size_t iPos = 0; iPos < length; iPos++)
			{
				if (SolutionWords[iGen]->GetSymbols(iPos, 1) != this->model->evidence[iGen + 1]->GetSymbols(iPos, 1))
				{
					errors++;
				}
			}
		}
	}

	result = (float)errors / total;

	return result;
}

List<List<ProductionRuleV3^>^>^ LSIProblemV3::CreateModelFromSolution(SolutionType^ Solutions)
{
	List<List<ProductionRuleV3^>^>^ result = gcnew List<List<ProductionRuleV3^>^>;

	for (size_t iSolution = 0; iSolution <= Solutions->solutions->GetUpperBound(0); iSolution++)
	{
		List<ProductionRuleV3^>^ rules = this->CreateRulesByScanning(Solutions->solutions[iSolution]);
		result->Add(rules);
	}

	return result;
}

List<ProductionRuleV3^>^ LSIProblemV3::CreateRulesByScanning(array<Int32>^ RuleLengths)
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

List<ProductionRuleV3^>^ LSIProblemV3::SortProductionRules(List<ProductionRuleV3^>^ Rules)
{
	List<ProductionRuleV3^>^ result = Rules;

	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		ProductionRuleV3^ r = Rules[iRule];
		List<Int32>^ toRemove = gcnew List<Int32>;

		List<String^>^ contextLeft = gcnew List<String^>;
		List<String^>^ contextRight = gcnew List<String^>;
		List<Int32>^ odds = gcnew List<Int32>;
		List<String^>^ successor = gcnew List<String^>;
		List<String^>^ condition = gcnew List<String^>;

		bool contextFreeFound = false;
		String^ contextLeftFinal;
		String^ contextRightFinal;
		Int32 oddsFinal;
		String^ conditionFinal;
		String^ successorFinal;

		for (size_t jRule = 0; jRule < r->successor->Count; jRule++)
		{
			if (r->contextLeft[jRule] == "" && r->contextRight[jRule] == "")
			{
				contextFreeFound = true;
				contextLeftFinal = r->contextLeft[jRule];
				contextRightFinal = r->contextRight[jRule];
				oddsFinal = r->odds[jRule];
				conditionFinal = r->condition[jRule];
				successorFinal = r->successor[jRule];
				toRemove->Add(jRule);
			}
			else if (r->contextLeft[jRule] == "" || r->contextRight[jRule] == "")
			{
				contextLeft->Add(r->contextLeft[jRule]);
				contextRight->Add(r->contextRight[jRule]);
				odds->Add(r->odds[jRule]);
				condition->Add(r->condition[jRule]);
				successor->Add(r->successor[jRule]);
				toRemove->Add(jRule);
			}
		}

		toRemove->Sort();

		for (Int32 i = toRemove->Count - 1; i >= 0; --i)
		{
			r->contextLeft->RemoveAt(toRemove[i]);
			r->contextRight->RemoveAt(toRemove[i]);
			r->odds->RemoveAt(toRemove[i]);
			r->condition->RemoveAt(toRemove[i]);
			r->successor->RemoveAt(toRemove[i]);
		}

		for (size_t i = 0; i < contextLeft->Count; i++)
		{
			r->contextLeft->Add(contextLeft[i]);
			r->contextRight->Add(contextRight[i]);
			r->odds->Add(odds[i]);
			r->condition->Add(condition[i]);
			r->successor->Add(successor[i]);
		}

		if (contextFreeFound)
		{
			r->contextLeft->Add(contextLeftFinal);
			r->contextRight->Add(contextRightFinal);
			r->odds->Add(oddsFinal);
			r->condition->Add(conditionFinal);
			r->successor->Add(successorFinal);
		}
	}

	return result;
}

Fact^ LSIProblemV3::CreateBaseFact(Int32 iSymbol, Int32 iLeft, Int32 iRight)
{
	Fact^ f = gcnew Fact();
	f->sac = gcnew SAC();
	f->sac->iSymbol = iSymbol;
	f->sac->iLeft = iLeft;
	f->sac->iRight = iRight;
	f->condition = "*";
	f->head = gcnew MinMaxFragment();
	f->head->min = gcnew Fragment();
	f->head->max = gcnew Fragment();
	f->tail = gcnew MinMaxFragment();
	f->tail->min = gcnew Fragment();
	f->tail->max = gcnew Fragment();
	f->mid = gcnew Fragment();
	f->max = gcnew List<FragmentSigned^>;
	f->btf = gcnew List<FragmentSigned^>;
	f->cache = gcnew List<FragmentSigned^>;
	f->tabu = gcnew List<String^>;
	f->length = gcnew MinMax();
	f->length->min = this->absoluteMinLength; // this should be 1 when working with a complete alphabet, zero otherwise
	f->length->max = 999; // TODO: we can actually do better than this. It cannot be longer than the generation after it appears, but this will do for now.
	f->growth = gcnew array<Int32, 2>(this->model->alphabet->Size(), 2);
	// If there are no parameters, this is trivial
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		f->growth[iSymbol, 0] = 0;
		f->growth[iSymbol, 1] = 999; // TODO: we can do better than this, it cannot be larger than the growth in the generation after it appears
	}
	return f;
}

array<Int32>^ LSIProblemV3::CalculateGrowth(WordV3^ From, WordV3^ To)
{
	array<Int32>^ PV = gcnew array<Int32>(this->model->alphabet->symbols->Count);

	// if either word is not compressed, compute the Parikh vector for the word
	if (!To->isCompressed)
	{
		this->model->alphabet->CreateParikhVector(To);
	}

	if (!From->isCompressed)
	{
		this->model->alphabet->CreateParikhVector(From);
	}

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->symbols->Count; iSymbol++)
	{
		if (!this->model->alphabet->IsTurtle(iSymbol))
		{
			PV[iSymbol] = To->parikhVector[iSymbol];
		}
		else
		{ 
			PV[iSymbol] = To->parikhVector[iSymbol] - From->parikhVector[iSymbol];
		}
	}

	return PV;
}

FactCounter^ LSIProblemV3::ComputeFactCount(Int32 iGen, WordXV3^ W)
{
	FactCounter^ result = gcnew FactCounter();

	for (size_t iPos = 0; iPos < W->Length(); iPos++)
	{
		Int32 symbolIndex = W->GetSymbolIndex(iPos);
		Int32 iLeft = W->GetLeftContext(iPos);
		Int32 iRight = W->GetRightContext(iPos);

		Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
		result->Add(iGen, 0, symbolIndex, iLeft, iRight, f);
	}

	return result;
}

List<String^>^ LSIProblemV3::ComputeFragments_FragmentAndList(Fragment^ Fr, List<FragmentSigned^>^ L)
{
	List<String^>^ result = gcnew List<String^>;
	Int32 iLength = Fr->fragment->Length;
	array<bool>^ covered = gcnew array<bool>(iLength);

	bool done = false;
	while ((!done) && (iLength >= 1))
	{
		List<String^>^ subwords = this->MAO->GetSubwordsOfLength(Fr->fragment, iLength);

		// check to see if any of the subwords is a subword of all fragments in the cache
		Int32 iWord = 0;

		while (!done && iWord < subwords->Count)
		{
			bool isCommon = true;
			Int32 iFragment = 0;

			while (isCommon && iFragment < L->Count)
			{
				if (!this->MAO->IsSubword(L[iFragment]->fragment, subwords[iWord]))
				{
					isCommon = false;
				}
				iFragment++;
			}

			// if it is, then check to see if it is in a covered area
			if (isCommon)
			{
				// check to see if it is in previously covered area
				Int32 start = 0;
				Int32 index = -1;
				bool isCovered = true;

				// mark as covered the section of the shortest fragment by this subword
				do
				{
					index = Fr->fragment->IndexOf(subwords[iWord], start);
					if (index >= 0)
					{
						for (size_t i = index; i < index + subwords[iWord]->Length; i++)
						{
							if (!covered[i])
							{
								isCovered = false;
							}
							covered[i] = true;
						}
					}
					start = index + 2;
				} while (index >= 0);

				if (!result->Contains(subwords[iWord]) && !isCovered)
				{
					result->Add(subwords[iWord]);
				}



				// check to see if the shortest fragment is fully covered
				bool fullyCovered = true;
				Int32 iCovered = 0;
				while (fullyCovered && iCovered < iLength)
				{
					fullyCovered = fullyCovered && covered[iCovered];
					iCovered++;
				}

				done = fullyCovered;
			}
			iWord++;
		}
		iLength--;
	}

	return result;
}

// Every max fragment must be a common subword of the BTF list
void LSIProblemV3::ComputeFragments_FactBTF()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];
		List<Fragment^>^ toRemove = gcnew List<Fragment^>;
		for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
		{
			Fragment^ fr = f->max[iFragment];
			if (!this->MAO->IsCommonSubword(fr, f->btf))
			{
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " max fragment " + f->max[iFragment]->fragment + " is not common to all BTFs");
				toRemove->Add(f->max[iFragment]);
			}
		}

		List<String^>^ result = this->ComputeFragments_FragmentAndList(f->btf[f->btf->Count - 1], f->btf);

		this->MAO->RemoveFact_MaxFragmentOnly(f, toRemove, true);
	}

}

void LSIProblemV3::ComputeBTF_Cache()
{
	// Create an initial BTF on the 1st pass
	if (this->firstPassBTF)
	{
		this->firstPassBTF = false;
		this->ComputeBTF_Cache_Step1();
	}
	else
	{
		// Otherwise, add the existing BTFs to the cache
		for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
		{
			for (size_t iFragment = 0; iFragment < this->MAO->factsList[iFact]->btf->Count; iFragment++)
			{
				if (!this->MAO->IsBTFInList(this->MAO->factsList[iFact]->btf[iFragment], this->MAO->factsList[iFact]->cache))
				{
					this->MAO->factsList[iFact]->cache->Add(this->MAO->factsList[iFact]->btf[iFragment]);
				}
			}
		}
	}

	this->ComputeBTF_Cache_Step2(); // reconcile the fragments in the cache
	this->ComputeBTF_Cache_Step3(); // finalize the BTFs
	this->ComputeBTF_Cache_Step4(); // assign a BTF to each SAC instance

	// Clear the caches
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		this->MAO->factsList[iFact]->cache->Clear();
	}
}

// Compute a unique set of signed true fragments based on signature compatability
void LSIProblemV3::ComputeBTF_Cache_Step1()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];
		f->cache->Sort(gcnew BTFLengthComparer());

		// Find the set of true fragments such that no fragent is a subword of any other fragment in the set
		// And has incompatible signature
		List<Int32>^ toAdd = gcnew List<Int32>;
		for (size_t i = 0; i < f->cache->Count; i++)
		{
			bool found = false;
			Int32 j = i + 1;
			while (!found && j < f->cache->Count)
			{
				if (this->MAO->IsSubword(f->cache[i]->fragment, f->cache[j]->fragment) && (this->MAO->IsSignatureCompatible(f->cache[i]->leftSignature, f->cache[i]->rightSignature, f->cache[j]->leftSignature, f->cache[j]->rightSignature)))
				{
					found = true;
				}
				j++;
			}

			if (!found)
			{
				toAdd->Add(i);
			}
		}

		for (size_t i = 0; i < toAdd->Count; i++)
		{
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " found BTF " + f->cache[toAdd[i]]->fragment + " from the cache");
			this->MAO->SetFact_BTFOnly(f, f->cache[toAdd[i]]);
		}

		//f->cache->Clear();
	}
}

// By comparing each signed true fragment to each other signed true fragment then additional signed true fragments may be found
void LSIProblemV3::ComputeBTF_Cache_Step2()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		bool added = false;
		List<FragmentSigned^>^ candidates = gcnew List<FragmentSigned^>;
		List<FragmentSigned^>^ associated = gcnew List<FragmentSigned^>;
		List<FragmentSigned^>^ uncertain = gcnew List<FragmentSigned^>;
		Fact^ f = this->MAO->factsList[iFact];
		f->cache->Reverse();

		do
		{
			added = false;
			associated->Clear();
			uncertain->Clear();
			for (size_t iCache = 0; iCache < f->cache->Count; iCache++)
			{
				FragmentSigned^ fr1 = f->cache[iCache];
				for (size_t jCache = 0; jCache < f->cache->Count; jCache++)
				{
					if (iCache != jCache)
					{
						FragmentSigned^ fr2 = f->cache[jCache];
						// TODO: improve the efficiency of this subroutine
						List<Fragment^>^ l = this->MAO->Compute_MaxFragmentsFromPairedFragments(fr1, fr2, f);

						if (l->Count == 1)
						{
							for (size_t i = 0; i < l->Count; i++)
							{
								if (!this->MAO->IsBTFInList(l[i], associated))
								{
									associated->Add(this->MAO->CreateBTF(l[i], f->cache[iCache]->leftSignature, f->cache[iCache]->rightSignature));
								}
							}
						}
						//else
						//{
						//	for (size_t i = 0; i < l->Count; i++)
						//	{
						//		if (!this->MAO->IsBTFInList(l[i], uncertain))
						//		{
						//			uncertain->Add(this->MAO->CreateBTF(l[i], f->cache[iCache]->leftSignature, f->cache[iCache]->rightSignature));
						//		}
						//	}
						//}
					}
				}
			}

			// Eliminate unnecessary ATFs
			// A. If any ATF is a subword with a compatible signature of another ATF, remove the longer ATF
			// B. If any BTF is a subword with a compatible signature of an ATF, remove the ATF
			List<FragmentSigned^>^ toRemove = gcnew List<FragmentSigned^>;
			associated->Sort(gcnew BTFLengthComparer());

			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight));
			for (size_t i = 0; i < associated->Count; i++)
			{
				Console::WriteLine("Found true fragment " + associated[i]->fragment + " & " + associated[i]->leftSignature + " / " + associated[i]->rightSignature);
			}
			Console::WriteLine();

			for (size_t iATF = 0; iATF < associated->Count; iATF++)
			{
				for (size_t jATF = 0; jATF < associated->Count; jATF++)
				{
					if (iATF != jATF)
					{
						// A. If any ATF is a subword with a compatible signature of another ATF, remove the longer ATF
						if (this->MAO->IsSubword(associated[jATF]->fragment, associated[iATF]->fragment) && (this->MAO->IsSignatureCompatible(associated[jATF]->leftSignature, associated[jATF]->rightSignature, associated[iATF]->leftSignature, associated[iATF]->rightSignature)))
						{
							if (!toRemove->Contains(associated[jATF]))
							{
								Console::WriteLine("Removing ATF " + associated[jATF]->fragment + " & " + associated[jATF]->leftSignature + " / " + associated[jATF]->rightSignature + " as it is a subword of " + associated[iATF]->fragment + " & " + associated[iATF]->leftSignature + " / " + associated[iATF]->rightSignature);
								toRemove->Add(associated[jATF]);
							}
						}
					}
				}

				// B. If any BTF is a subword with a compatible signature of an ATF, remove the ATF
				for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
				{
					if (this->MAO->IsSubword(associated[iATF]->fragment, f->btf[iBTF]->fragment) && (this->MAO->IsSignatureCompatible(associated[iATF]->leftSignature, associated[iATF]->rightSignature, f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature)))
					{
						if (!toRemove->Contains(associated[iATF]))
						{
							Console::WriteLine("Removing ATF " + associated[iATF]->fragment + " & " + associated[iATF]->leftSignature + " / " + associated[iATF]->rightSignature + " as it is a superstring of " + f->btf[iBTF]->fragment + " & " + f->btf[iBTF]->leftSignature + " / " + f->btf[iBTF]->rightSignature);
							toRemove->Add(associated[iATF]);
						}
					}
				}
			}

			for (size_t iRemove = 0; iRemove < toRemove->Count; iRemove++)
			{
				associated->Remove(toRemove[iRemove]);
			}

			f->cache->Clear();

			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight));
			for (size_t i = 0; i < associated->Count; i++)
			{
				Console::WriteLine("Found true fragment " + associated[i]->fragment + " & " + associated[i]->leftSignature + " / " + associated[i]->rightSignature);
				if (!this->MAO->IsBTFInList(associated[i], f->cache))
				{
					f->cache->Add(associated[i]);
					candidates->Add(associated[i]);
					added = true;
				}
			}
			Console::WriteLine();
		} while (added);

		// Update compatible BTFs or if none compatible add as a new BTF
		candidates->Sort(gcnew BTFLengthComparer());
		for (size_t iATF = 0; iATF < candidates->Count; iATF++)
		{
			Int32 iBTF = 0;
			bool found = false;
			while (!found && iBTF < f->btf->Count)
			{
				if (this->MAO->IsSubword(f->btf[iBTF]->fragment, candidates[iATF]->fragment) && (this->MAO->IsSignatureCompatible(f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature, candidates[iATF]->leftSignature, candidates[iATF]->rightSignature)))
				{
					this->MAO->SetFact_BTFOnly_Update(f->btf[iBTF], candidates[iATF], f);
					found = true;
				}
				iBTF++;
			}

			if (!found)
			{
				this->MAO->SetFact_BTFOnly_Direct(f, candidates[iATF]);
			}
		}

		//// Eliminate uncertain fragments
		//// A. If any BTF is a subword with compatible signature of an uncertain fragment, remove uncertain fragment
		//// B. If any associated fragment is a subword with compatible signature of an uncertain fragment, remove uncertain fragment
		//// C. If any uncertain fragment is a subword with compatible signature of an uncertain fragment, remove the longer fragment

		//// A. If any BTF is a subword with compatible signature of an uncertain fragment, remove uncertain fragment
		//List<FragmentSigned^>^ toRemove = gcnew List<FragmentSigned^>;
		//for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		//{
		//	for (size_t iUncertain = 0; iUncertain < uncertain->Count; iUncertain++)
		//	{
		//		if ((this->MAO->IsSubword(uncertain[iUncertain]->fragment, f->btf[iBTF]->fragment) || uncertain[iUncertain]->fragment->Length > f->btf[iBTF]->fragment->Length) && (this->MAO->IsSignatureCompatible(uncertain[iUncertain]->leftSignature, uncertain[iUncertain]->rightSignature, f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature)))
		//		{
		//			toRemove->Add(uncertain[iUncertain]);
		//		}
		//	}
		//}

		//for (size_t iRemove = 0; iRemove < toRemove->Count; iRemove++)
		//{
		//	uncertain->Remove(toRemove[iRemove]);
		//}

		//// B. If any associated fragment is a subword with compatible signature of an uncertain fragment, remove uncertain fragment
		//toRemove->Clear();
		//for (size_t iATF = 0; iATF < associated->Count; iATF++)
		//{
		//	for (size_t iUncertain = 0; iUncertain < uncertain->Count; iUncertain++)
		//	{
		//		if (this->MAO->IsSubword(uncertain[iUncertain]->fragment, associated[iATF]->fragment) && (this->MAO->IsSignatureCompatible(uncertain[iUncertain]->leftSignature, uncertain[iUncertain]->rightSignature, associated[iATF]->leftSignature, associated[iATF]->rightSignature)))
		//		{
		//			toRemove->Add(uncertain[iUncertain]);
		//		}
		//	}
		//}

		//for (size_t iRemove = 0; iRemove < toRemove->Count; iRemove++)
		//{
		//	uncertain->Remove(toRemove[iRemove]);
		//}

		//// C. If any uncertain fragment is a subword with compatible signature of an uncertain fragment, remove the longer fragment
		//toRemove->Clear();
		//for (size_t iUncertain = 0; iUncertain < uncertain->Count; iUncertain++)
		//{
		//	for (size_t jUncertain = 0; jUncertain < uncertain->Count; jUncertain++)
		//	{
		//		if (iUncertain != jUncertain)
		//		{
		//			if (this->MAO->IsSubword(uncertain[jUncertain]->fragment, uncertain[iUncertain]->fragment) && (this->MAO->IsSignatureCompatible(uncertain[jUncertain]->leftSignature, uncertain[jUncertain]->rightSignature, uncertain[iUncertain]->leftSignature, uncertain[iUncertain]->rightSignature)))
		//			{
		//				toRemove->Add(uncertain[iUncertain]);
		//			}
		//		}
		//	}
		//}

		//for (size_t iRemove = 0; iRemove < toRemove->Count; iRemove++)
		//{
		//	uncertain->Remove(toRemove[iRemove]);
		//}

		//uncertain->Sort(gcnew BTFLengthComparer());

		//for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		//{
		//	List<FragmentSigned^>^ toAdd = gcnew List<FragmentSigned^>;
		//	Console::WriteLine("For BTF " + f->btf[iBTF]->fragment + " & " + f->btf[iBTF]->leftSignature + " / " + f->btf[iBTF]->rightSignature);
		//	for (size_t iUncertain = 0; iUncertain < uncertain->Count; iUncertain++)
		//	{
		//		// Associate the uncertain fragments to the compatible BTF
		//		if (this->MAO->IsSubword(f->btf[iBTF]->fragment, uncertain[iUncertain]->fragment) && (this->MAO->IsSignatureCompatible(f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature, uncertain[iUncertain]->leftSignature, uncertain[iUncertain]->rightSignature)))
		//		{
		//			Console::WriteLine("Adding uncertain fragment " + uncertain[iUncertain]->fragment + " & " + uncertain[iUncertain]->leftSignature + " / " + uncertain[iUncertain]->rightSignature);
		//			//List<ValidatedFragment^>^ v = this->MAO->ReviseMaxFragmentWithCorrections(uncertain[iUncertain], f);
		//			toAdd->Add(uncertain[iUncertain]);
		//		}
		//	}

		//	if (toAdd->Count == 1)
		//	{
		//		this->MAO->SetFact_BTFOnly_Update(f->btf[iBTF], toAdd[0], f);
		//	}
		//	else
		//	{
		//		for (size_t i = 0; i < toAdd->Count; i++)
		//		{
		//			this->MAO->SetFact_BTFOnly_UncertainOnly(f, iBTF, toAdd[i]);
		//		}
		//	}
		//}


		//Console::WriteLine("Paused");
		//Console::ReadLine();
	}
}

// Make one final check to finalize BTFs
// A. If two BTFs have the same fragment, then their signatures can be compressed (and one of them can be removed)
// B. If two BTFs are signature compatible, then the longer one can be removed

void LSIProblemV3::ComputeBTF_Cache_Step3()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];
		// A. If two BTFs have the same fragment, then their signatures can be compressed (and one of them can be removed)
		List<FragmentSigned^>^ toRemove = gcnew List<FragmentSigned^>;
		for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		{
			for (size_t jBTF = iBTF+1; jBTF < f->btf->Count; jBTF++)
			{
				if (iBTF != jBTF && f->btf[iBTF]->fragment == f->btf[jBTF]->fragment)
				{
					Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " BTF " + f->btf[iBTF]->fragment + " is equal to " + f->btf[jBTF]->fragment);
					String^ right = this->MAO->Compute_PrefixOverlap(f->btf[iBTF]->rightSignature, f->btf[jBTF]->rightSignature);
					String^ left = this->MAO->Compute_SuffixOverlap(f->btf[iBTF]->leftSignature, f->btf[jBTF]->leftSignature);
					Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " new signatures for BTF " + f->btf[iBTF]->fragment + " & " + left + " / " + right);
					f->btf[iBTF]->leftSignature = left;
					f->btf[iBTF]->rightSignature = right;
					toRemove->Add(f->btf[jBTF]);
				}
			}
		}

		for (size_t i = 0; i < toRemove->Count; i++)
		{
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " removing BTF " + toRemove[i]->fragment + " & " + toRemove[i]->leftSignature + " / " + toRemove[i]->rightSignature);
			f->btf->Remove(toRemove[i]);
		}

		// B. Compress the BTF list
		toRemove->Clear();
		for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		{
			for (size_t jBTF = 0; jBTF < f->btf->Count; jBTF++)
			{
				if (iBTF != jBTF)
				if (this->MAO->IsSubword(f->btf[jBTF]->fragment, f->btf[iBTF]->fragment) && (this->MAO->IsSignatureCompatible(f->btf[jBTF]->leftSignature, f->btf[jBTF]->rightSignature, f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature)))
				{
					Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " BTF " + f->btf[iBTF]->fragment + " is signed compatible with " + f->btf[jBTF]->fragment);
					toRemove->Add(f->btf[jBTF]);
				}
			}
		}

		for (size_t i = 0; i < toRemove->Count; i++)
		{
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(f->sac->iSymbol, f->sac->iLeft, f->sac->iRight) + " removing BTF " + toRemove[i]->fragment + " & " + toRemove[i]->leftSignature + " / " + toRemove[i]->rightSignature);
			f->btf->Remove(toRemove[i]);
		}
	}
}

void LSIProblemV3::ComputeFragments_Localization(Int32 iGen)
{
	// B. if a set of symbols can only be produced by one symbol, then that set is a fragment
	// i. if the uniqueness is at the start of the production it is a head fragment 
	// ii. if the uniqueness is at the end of the production it is a tail fragment 
	// iii. if the uniqueness is at the middle of the production it is a mid fragment 
	// iv. if the uniqueness is at the start and end of the production it is a complete fragment
	// C. The BTF for a symbol is the start & end of the symbols it could produce, even if another symbol might produce them
	for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
	{
		// no need to process turtles
		if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos1)))
		{
			Int32 start = -1;
			Int32 end = -1;
			Int32 startBTF = -1;
			Int32 endBTF = -1;
			bool inFragment = false;
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				if ((inFragment) && ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Unset) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Never)))
				{// reach the end of true values
					end = iPos2 - 1;
					inFragment = false;
				}
				else if ((inFragment) && (iPos2 == this->model->evidence[iGen + 1]->Length() - 1))
				{// reached the end of the word
					end = iPos2;
					inFragment = false;
				}
				else if ((!inFragment) && ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)))
				{// found a true value
					start = iPos2;
					inFragment = true;
				}

				if ((start != -1) && (end != -1))
				{
					if (start < startBTF || startBTF == -1)
					{
						startBTF = start;
					}
					if (end > endBTF || endBTF == -1)
					{
						endBTF = end;
					}

					// process fragment
					FragmentSigned^ frMax = this->MAO->CreateSignedFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(start, end), false, true, "", "");

					Int32 startLocked = -1;
					Int32 endLocked = -1;
					bool inLocked = false;
					bool complete = false;

					for (size_t iPos3 = start; iPos3 <= end; iPos3++)
					{
						if ((inLocked) && (this->MAO->localization[iGen][iPos3, iPos1] == MasterAnalysisObject::cLocalization_Strong))
						{
							endLocked = iPos3 - 1;
							inLocked = false;
						}
						else if ((inLocked) && (iPos3 == end))
						{
							endLocked = iPos3;
							inLocked = false;
						}
						else if ((!inLocked) && (this->MAO->localization[iGen][iPos3, iPos1] == MasterAnalysisObject::cLocalization_Locked))
						{
							startLocked = iPos3;
							inLocked = true;
						}

						if ((startLocked != -1) && (endLocked != -1))
						{
							// If entire fragment is locked then it is a complete fragment
							if ((start == startLocked) && (end == endLocked))
							{
								Fragment^ fr = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startLocked, endLocked), true, true);
								this->MAO->SetFact_MaxFragmentOnly(iGen, this->model->evidence[iGen], iPos1, fr, this->model->evidence[iGen + 1], startLocked, endLocked);
								complete = true;
							}
							else if (start == startLocked)
							{// this is a head fragment
								Fragment^ frMinHead = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startLocked, endLocked), false, true);
								Fragment^ frMaxHead = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startLocked, end), false, true);
								this->MAO->SetFact_MinHeadFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMinHead, this->model->evidence[iGen + 1], startLocked, endLocked);
								this->MAO->SetFact_MaxHeadFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMaxHead, this->model->evidence[iGen + 1], startLocked, end);
							}
							else if (end == endLocked)
							{// this is a tail fragment
								Fragment^ frMinTail = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startLocked, endLocked), false, true);
								Fragment^ frMaxTail = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(start, endLocked), false, true);
								this->MAO->SetFact_MinTailFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMinTail, this->model->evidence[iGen + 1], startLocked, endLocked);
								this->MAO->SetFact_MaxTailFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMaxTail, this->model->evidence[iGen + 1], start, endLocked);
							}
							else
							{// this is a mid fragment
								Fragment^ frMid = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startLocked, endLocked), false, true);
								this->MAO->SetFact_MidFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMid, this->model->evidence[iGen + 1], startLocked, endLocked);
							}
						}
					}

					if (!complete)
					{
						// save the max fragment if the fragment was not complete
						this->MAO->SetFact_MaxFragmentOnly(iGen, this->model->evidence[iGen], iPos1, frMax, this->model->evidence[iGen + 1], start, end);
						Fragment^ tmp1 = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startBTF, endBTF), false, true);
						FragmentSigned^ tmp2 = this->MAO->CreateBTF(tmp1, this->ComputeBTF_LeftSignature(iGen, iPos1, this->model->evidence[iGen], tmp1->fragment), this->ComputeBTF_RightSignature(iGen, iPos1, this->model->evidence[iGen], tmp1->fragment));
						this->MAO->SetFact_BTFOnly(iGen, this->model->evidence[iGen], iPos1, tmp2);
					}

					// reset start/end to find another fragment
					start = -1;
					end = -1;
				}
			}
		}
	}
}


void LSIProblemV3::ComputeFragments_Markers(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_FRAGMENTS_MARKERS_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Fragment from Markers");
	Console::WriteLine("===============================");
#endif
}

void LSIProblemV3::ComputeFragments_Overlap()
{
#if _PHASE3_COMPUTE_FRAGMENTS_OVERLAP_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Fragment from Overlapping");
	Console::WriteLine("===================================");
#endif
}

void LSIProblemV3::ComputeFragments_PartialSolution(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_FRAGMENTS_PARTIALSOLUTION_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Fragment from Partial Solution");
	Console::WriteLine("========================================");
#endif
}

void LSIProblemV3::ComputeFragment_Position_HeadOnly(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 iPos2)
{
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	if (!f->head->min->isComplete)
	{// do not find a fragment for something that is already complete
		Int32 lengthMin = f->length->min;
		Int32 lengthMax = f->length->max;

		// if the head symbol is the last symbol, then this refines the fragment into a complete
		if (iPos1 == W1->Length() - 1)
		{
			lengthMin = W2->Length() - iPos2;
			lengthMax = W2->Length() - iPos2;
		}
		// if the right neighbour is a turtle then this refines the min/max fragment
		else if (iPos1 + 1 < W1->Length() - 1)
		{
			if (this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbol(iPos1 + 1)))
			{
				String^ rightMarker = "";
				Int32 iScan = iPos1 + 1;
				while (iScan < W1->Length() && this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbol(iScan)))
				{
					rightMarker += this->model->evidence[iGen]->GetSymbol(iScan);
					iScan++;
				}

				iScan = iPos2+1;
				bool found = false;

				while (!found && (iScan+rightMarker->Length < W2->Length()) && (iScan - iPos2) < f->length->max)
				{
					String^ marker = this->model->evidence[iGen + 1]->GetSymbols(iScan, rightMarker->Length);
					
					if (rightMarker == marker && this->model->evidence[iGen]->GetDepth(iPos1+1) == this->model->evidence[iGen+1]->GetDepth(iScan))
					{
						found = true;
					}
					else
					{
						iScan++;
					}
				}
				lengthMin = Math::Max(lengthMin, iScan - iPos2);

				iScan = iPos2 + f->length->max;
				found = false;

				while (!found && iScan < W2->Length() && (iScan - iPos2) < f->length->max)
				{
					String^ marker = this->model->evidence[iGen + 1]->GetSymbols(iScan, rightMarker->Length);

					if (rightMarker == marker && this->model->evidence[iGen]->GetDepth(iPos1 + 1) == this->model->evidence[iGen + 1]->GetDepth(iScan))
					{
						found = true;
					}
					else
					{
						iScan--;
					}
				}

				lengthMax = Math::Min(lengthMax, iScan - iPos2);
			}
		}

		String^ subwordMin = W2->GetSymbolsNoCorrection(iPos2, lengthMin);
		String^ subwordMax = W2->GetSymbolsNoCorrection(iPos2, lengthMax);

		ValidatedFragmentX^ subwordMinV = this->MAO->ReviseFragment_HeadTailFragmentOnly(subwordMin, f);
		ValidatedFragmentX^ subwordMaxV = this->MAO->ReviseFragment_HeadTailFragmentOnly(subwordMax, f);

		subwordMin = subwordMinV->vfr->fr->fragment;
		subwordMax = subwordMaxV->vfr->fr->fragment;

		this->MAO->SetFact_MinHeadFragmentOnly(iGen, W1, iPos1, this->MAO->CreateFragment(subwordMin, this->MAO->IsFragmentComplete(subwordMin, subwordMax), true), W2, iPos2 + subwordMinV->start, iPos2 + subwordMinV->end);
		this->MAO->SetFact_MaxHeadFragmentOnly(iGen, W1, iPos1, this->MAO->CreateFragment(subwordMax, this->MAO->IsFragmentComplete(subwordMin, subwordMax), true), W2, iPos2 + subwordMaxV->start, iPos2 + subwordMaxV->end);

#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
		Console::WriteLine("Found min/max fragment head for SAC " + this->MAO->SymbolCondition(iGen, W1, iPos1) + " => " + subwordMin + " / " + subwordMax);
#endif
	}
}

void LSIProblemV3::ComputeFragment_Position_TailOnly(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 iPos2)
{
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	if (!f->tail->min->isComplete)
	{// do not find a fragment for something that is already complete
		String^ subwordMin = W2->GetSymbolsFromPos(iPos2 - f->length->min, iPos2);
		String^ subwordMax = W2->GetSymbolsFromPos(iPos2 - f->length->max, iPos2);

		subwordMin = this->Reverse(subwordMin);
		subwordMax = this->Reverse(subwordMax);

		ValidatedFragmentX^ subwordMinV = this->MAO->ReviseFragment_HeadTailFragmentOnly(subwordMin, f);
		ValidatedFragmentX^ subwordMaxV = this->MAO->ReviseFragment_HeadTailFragmentOnly(subwordMax, f);

		subwordMin = this->Reverse(subwordMinV->vfr->fr->fragment);
		subwordMax = this->Reverse(subwordMaxV->vfr->fr->fragment);

		this->MAO->SetFact_MinTailFragmentOnly(iGen, W1, iPos1, this->MAO->CreateFragment(subwordMin, this->MAO->IsFragmentComplete(subwordMin, subwordMax), true), W2, iPos2 + subwordMinV->start, iPos2 + subwordMinV->end);
		this->MAO->SetFact_MaxTailFragmentOnly(iGen, W1, iPos1, this->MAO->CreateFragment(subwordMax, this->MAO->IsFragmentComplete(subwordMin, subwordMax), true), W2, iPos2 + subwordMaxV->start, iPos2 + subwordMaxV->end);

#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
		Console::WriteLine("Found min/max fragment tail for SAC " + this->MAO->SymbolCondition(iGen, W1, iPos1) + " => " + subwordMin + " / " + subwordMax);
#endif
	}
}

Fragment^ LSIProblemV3::ComputeFragment_Position_BTFOnly_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);
	Fragment^ result;

	// Compute the right reserve and the the effective length of the word
	Int32 reserveRight = 0;
	for (size_t iPos = iPos1 + 1; iPos < W1->Length(); iPos++)
	{
		Fact^ tmp = this->MAO->GetFact(iGen, W1, iPos);
		reserveRight += tmp->length->min;
	}

	Int32 length = W2->Length() - StartWake - reserveRight;
	length = Math::Min(length, (1 + EndWake - StartWake) + f->length->max);

	// Using the shorter length compute the new max fragment
	result = this->MAO->CreateFragment(W2->GetSymbols(StartWake, length), false, true);

	return result;
}

Fragment^ LSIProblemV3::ComputeFragment_Position_BTFOnly_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);
	Fragment^ result;

	// Compute the left reserve and the the effective length of the word
	Int32 reserveLeft = 0;
	for (size_t iPos = 0; iPos < iPos1; iPos++)
	{
		Fact^ tmp = this->MAO->GetFact(iGen, W1, iPos);
		reserveLeft += tmp->length->min;
	}

	Int32 length = EndWake - reserveLeft + 1;
	length = Math::Min(length, (1 + EndWake - StartWake) + f->length->max);

	// Using the shorter length compute the new max fragment
	result = this->MAO->CreateFragment(W2->GetSymbols(EndWake - length + 1, length), false, true);

	return result;
}

bool LSIProblemV3::SymbolCountsMatch(array<Int32>^ A, array<Int32>^ B)
{
	bool result = true;
	Int32 idx = 0;

	while (idx <= A->GetUpperBound(0) && result)
	{
		result = result && (A[idx] == B[idx]);
		idx++;
	}

	return result;
}

void LSIProblemV3::ComputeFragments_Position(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Fragment by Position");
	Console::WriteLine("==============================");
#endif

	// Find head fragment
	// Scan W1 to find the first unsolved SAC
	Int32 iPos = 0;
	bool found = false;
	Fact^ f;

	while ((!found) && (iPos < W1->Length()))
	{
		if (!this->model->alphabet->IsTurtle(W1->GetSymbol(iPos)))
		{
			found = true;
		}
		else
		{
			iPos++;
		}
	}

	if (found)
	{
		this->ComputeFragment_Position_HeadOnly(iGen1, W1, iPos, W2, iPos);
	}

	// Find tail fragment
	// Scan W1 to find the first unsolved SAC
	Int32 iScan = 0;
	iPos = W1->Length() - 1;
	found = false;

	while ((!found) && (iPos > 0))
	{
		// TODO: add that SAC is not known
		if (!this->model->alphabet->IsTurtle(W1->GetSymbol(iPos - iScan)))
		{
			found = true;
		}
		else
		{
			iScan++;
		}
	}

	if (found)
	{
		this->ComputeFragment_Position_TailOnly(iGen1, W1, iPos, W2, W2->Length()-iScan);
	}

	// For every other SAC, a max fragment can be found by reserving symbols in W2
	Int32 reserveRight = 0;
	Int32 reserveLeft = 0;
	array<Int32>^ reserveSymbol = gcnew array<Int32>(this->model->alphabet->Size());

	// Precompute the reserve right
	for (Int32 iPos1 = 0; iPos1 < W1->Length(); iPos1++)
	{
		Fact^ f = this->MAO->GetFact(iGen1, W1, iPos1);
		reserveRight += f->length->min;

		if (this->model->alphabet->IsTurtle(this->model->evidence[iGen1]->GetSymbol(iPos1)))
		{
			reserveSymbol[this->model->evidence[iGen1]->GetSymbolIndex(iPos1)]++;
		}
	}

	// compute absolute right limit based on reserved turtles
	Int32 absRightLimit = W2->Length();
	
	array<Int32>^ countSymbol = gcnew array<Int32>(this->model->alphabet->Size());

	while (!this->SymbolCountsMatch(reserveSymbol, countSymbol))
	{
		if (this->model->alphabet->IsTurtle(this->model->evidence[iGen2]->GetSymbol(absRightLimit-1)))
		{
			countSymbol[this->model->evidence[iGen2]->GetSymbolIndex(absRightLimit-1)]++;
			absRightLimit--;
		}
		else
		{
			absRightLimit--;
		}
	}

	for (Int32 iPos1 = 0; iPos1 < W1->Length(); iPos1++)
	{
		String^ subwordMax = "";
		if (!this->model->alphabet->IsTurtle(W1->GetSymbol(iPos1)))
		{
			Fact^ f = this->MAO->GetFact(iGen1, W1, iPos1);

			// reduce the reserve right by the min of the current symbol
			reserveRight -= f->length->min;
			
			// compute the fragment
			Int32 end = (W2->Length() - 1) - reserveRight;
			if (end > absRightLimit)
			{
				end = absRightLimit - 1;
			}

			//subwordMax = W2->GetSymbolsNoCorrection(reserveLeft, count);
			subwordMax = W2->GetSymbolsFromPos(reserveLeft, end);

			// this is not valid, I cannot guarantee that the superstring of the successor starts at this position
			//if (subwordMax->Length > f->length->max)
			//{
			//	subwordMax = subwordMax->Substring(0, f->length->max);
			//}

#if _PHASE3_COMPUTE_FRAGMENTS_POSITION_ >= 1
			Console::WriteLine("Found max fragment for SAC " + this->MAO->SymbolCondition(iGen1, W1, iPos1) + " => " + subwordMax + " by position");
#endif
			// Refine fragment based on neighbours
			Console::WriteLine("Refining max fragment for SAC " + this->MAO->SymbolCondition(iGen1, W1, iPos1) + " => " + subwordMax + " by neighbours");


			Int32 btwStart = reserveLeft;
			Int32 btwEnd = W2->Length() - reserveRight;
			W1->SetWTW(iPos1, btwStart, btwEnd);

			FragmentSigned^ btf = this->MAO->CreateBTF(this->MAO->CreateFragment(subwordMax, false, true), this->ComputeBTF_LeftSignature(iGen1, iPos1, W1, subwordMax), this->ComputeBTF_RightSignature(iGen1, iPos1, W1, subwordMax));
			W1->SetBTF(iPos1, btf);
			this->MAO->SetFact_CacheOnly(iGen1, W1, iPos1, btf, W2, reserveLeft, reserveLeft + W2->Length() - reserveLeft - reserveRight);

			// increase the reserve let by the min of the current symbol before moving on
			reserveLeft += f->length->min;
		}
		else
		{
			reserveSymbol[this->model->evidence[iGen1]->GetSymbolIndex(iPos1)]--;
			reserveRight--;
			reserveLeft++;
		}
	}
}

void LSIProblemV3::ComputeFragments_Revise()
{
#if _PHASE3_COMPUTE_FRAGMENTS_REVISE_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Fragment by Revising");
	Console::WriteLine("==============================");
#endif

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		Int32 iTable = 0;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle()+1; iLeft++)
			{
				for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle()+1; iRight++)
				{
					Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft-1, iRight-1, nullptr);

					if (!this->MAO->IsNoFact(f))
					{
					}
				}
			}
		}
	}

}

// This function produces a fragment when no fragment could be localized
// It builds the fragment based on the wake start/end positions
// The fragment can be built in two ways, the shortest is taken
// A. Use the StartWake as the fragment start position. Compute a reserve right. Compute the end position by reserving from the EndWake position. The fragment is all symbols between those two positions.
// B. Use the StartWake as the fragment start position. Compute the end position as the EndWake + Successor Max Length. The fragment is all symbols between those two positions.
Range^ LSIProblemV3::ComputeFragment_Wake_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Range^ result = gcnew Range();
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	// Compute the right reserve and the the effective length of the word
	Int32 reserveRight = 0;
	for (size_t iPos = iPos1+1; iPos < W1->Length(); iPos++)
	{
		Fact^ tmp = this->MAO->GetFact(iGen, W1, iPos);
		reserveRight += tmp->length->min;
	}

	Int32 length = W2->Length() - StartWake - reserveRight;
	length = Math::Min(length, (1 + EndWake - StartWake) + f->length->max);
	
	// Using the shorter length compute the new max fragment
	FragmentSigned^ fr = this->MAO->CreateSignedFragment(W2->GetSymbols(StartWake, length), false, true, "", "");
	//this->MAO->SetFact_MaxFragmentOnly(iGen, W1, iPos1, fr, W2, StartWake, StartWake + length);
	this->MAO->SetFact_MaxFragmentOnly_Direct(f, fr);

	result->start = StartWake;
	result->end = StartWake + length;
	return result;
}

// This scans the fact table and finds the shortest signed true fragment with a compatible signature for the specific instance of a SAC
void LSIProblemV3::ComputeBTF_Cache_Step4()
{
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbol(iPos)))
			{
				Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				FragmentSigned^ btf = this->model->evidence[iGen]->GetBTF(iPos);

				Int32 shortest = 99999999;
				Int32 choice = -1;
				for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
				{
					if (f->btf[iBTF]->fragment->Length <= shortest && this->MAO->IsSignatureCompatible(f->btf[iBTF]->leftSignature, f->btf[iBTF]->rightSignature, btf->leftSignature, btf->rightSignature))
					{
						choice = iBTF;
						shortest = f->btf[iBTF]->fragment->Length;
					}
				}

				if (choice >= 0)
				{
#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 2
					Console::WriteLine("For Word " + iGen + " Symbol " + this->model->alphabet->GetSymbol(this->model->evidence[iGen]->GetSymbolIndex(iPos)) + "(" + iPos + ") BTF is compatible with " + f->btf[idx]->fragment + " with signature " + f->btf[idx]->leftSignature + "/" + f->btf[idx]->rightSignature);
#endif
					this->model->evidence[iGen]->SetBTF(iPos, f->btf[choice]);
			}
				else
				{
					this->model->evidence[iGen]->SetBTF(iPos, btf);
					this->MAO->SetFact_BTFOnly_Direct(f, btf);
				}
		}
		}
	}
}

// B. if a set of symbols can only be produced by one symbol, then that set is a fragment
// i. if the uniqueness is at the start of the production it is a head fragment 
// ii. if the uniqueness is at the end of the production it is a tail fragment 
// iii. if the uniqueness is at the middle of the production it is a mid fragment 
// iv. if the uniqueness is at the start and end of the production it is a complete fragment
// C. The BTF for a symbol is the start & end of the symbols it could produce, even if another symbol might produce them
void LSIProblemV3::ComputeBTF_Localization(Int32 iGen)
{
	for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
	{
		// no need to process turtles
		if (!this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos1)))
		{
			Int32 start = -1;
			Int32 end = -1;
			Int32 startBTF = -1;
			Int32 endBTF = -1;
			bool inFragment = false;
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				if ((inFragment) && ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Unset) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Never)))
				{// reach the end of true values
					end = iPos2 - 1;
					inFragment = false;
				}
				else if ((inFragment) && (iPos2 == this->model->evidence[iGen + 1]->Length() - 1))
				{// reached the end of the word
					end = iPos2;
					inFragment = false;
				}
				else if ((!inFragment) && ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)))
				{// found a true value
					start = iPos2;
					inFragment = true;
				}

				if ((start != -1) && (end != -1))
				{
					if (start < startBTF || startBTF == -1)
					{
						startBTF = start;
					}
					if (end > endBTF || endBTF == -1)
					{
						endBTF = end;
					}
				}
			}

			// reset start/end to find another fragment
			start = -1;
			end = -1;

			Fragment^ tmp1 = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(startBTF, endBTF), false, true);
			FragmentSigned^ tmp2 = this->MAO->CreateBTF(tmp1, this->ComputeBTF_LeftSignature(iGen, iPos1, this->model->evidence[iGen], tmp1->fragment), this->ComputeBTF_RightSignature(iGen, iPos1, this->model->evidence[iGen], tmp1->fragment));
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos1) + " by localization found BTF " + tmp2->fragment + " & " + tmp2->leftSignature + " / " + tmp2->rightSignature);
			this->MAO->SetFact_BTFOnly(iGen, this->model->evidence[iGen], iPos1, tmp2);
		}
	}
}

String^ LSIProblemV3::ComputeBTF_LeftSignature(Int32 iGen, Int32 iPos, WordXV3^ W, String^ Frag)
{
	Int32 iScan = 1;
	Int32 sigLengthLeft = 0;

	while (iPos - iScan >= 0 && sigLengthLeft < Frag->Length)
	{
		Fact^ f = this->MAO->GetFact(iGen, W, iPos - iScan);
		sigLengthLeft += f->length->min;
		iScan++;
	}

	String^ left = W->GetLeftSignature(iPos, sigLengthLeft);
	//Console::WriteLine("For Word " + W->GetSymbols(0, W->Length()) + " @ position " + iPos + " (" + W->GetSymbols(iPos, 1) + ") found left signature " + left);

	return left;
}

String^ LSIProblemV3::ComputeBTF_RightSignature(Int32 iGen, Int32 iPos, WordXV3^ W, String^ Frag)
{
	Int32 iScan = 1;
	Int32 sigLengthRight = 0;

	while (iPos + iScan < W->Length() && sigLengthRight < Frag->Length)
	{
		Fact^ f = this->MAO->GetFact(iGen, W, iPos + iScan);
		sigLengthRight += f->length->min;
		iScan++;
	}

	String^ right = W->GetRightSignature(iPos, sigLengthRight);
	//Console::WriteLine("For Word " + W->GetSymbols(0, W->Length()) + " @ position " + iPos + " (" + W->GetSymbols(iPos, 1) + ") found right signature " + right);

	return right;
}

Fragment^ LSIProblemV3::ComputeBTF_LocalizationCompatability_LeftToRight(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake)
{
	//Int32 iScan = Math::Max(1, Fr->fragment->Length-(1+EndWake-StartWake)); // This doesn't appear to work
	Int32 iScan = 1;
	bool isCompatible = false;
	Fragment^ result = gcnew Fragment;

	while (!isCompatible && iScan <= Fr->fragment->Length)
	{
		String^ subword = W->GetSymbols(Math::Max(0,StartWake-iScan), Fr->fragment->Length);
		if (subword == Fr->fragment)
		{
			isCompatible = true;
			result = this->MAO->CreateFragment(W->GetSymbolsFromPos(StartWake, StartWake + (Fr->fragment->Length - (iScan+1))), false, true);
		}
		iScan++;
	}

	return result;
}

Fragment^ LSIProblemV3::ComputeBTF_LocalizationCompatability_RightToLeft(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake)
{
	//Int32 iScan = Math::Max(1, Fr->fragment->Length-(1+EndWake-StartWake));
	Int32 iScan = 1;
	bool isCompatible = false;
	Fragment^ result = gcnew Fragment;

	while (!isCompatible && iScan <= Fr->fragment->Length)
	{
		Int32 end = Math::Min(W->Length() - 1, EndWake + iScan);
		String^ subword = W->GetSymbolsFromPos(Math::Max(0, end - Fr->fragment->Length), end-1);
		if (subword == Fr->fragment)
		{
			isCompatible = true;
			result = this->MAO->CreateFragment(subword->Substring(0,1+subword->Length-iScan), false, true);
		}
		iScan++;
	}

	return result;
}

// This function finds the longest BTF that will localize
Range^ LSIProblemV3::ComputeBTF_Wake_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Range^ result = gcnew Range();
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	Fragment^ wakeFr = this->MAO->CreateFragment(W2->GetSymbolsFromPos(StartWake, EndWake), false, false);
	for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
	{
		bool done = false;
		Int32 iLength = wakeFr->fragment->Length;
		Int32 index = 0;
		do
		{
			index = f->btf[iBTF]->fragment->IndexOf(wakeFr->fragment->Substring(0, iLength), 0);

			if (index >= 0)
			{
				done = true;
			}

			iLength--;
		} while ((!done) && (iLength >= 1));

		Fragment^ btf = this->MAO->CreateFragment(f->btf[iBTF]->fragment->Substring(index, f->btf[iBTF]->fragment->Length - index), false, true);
		f->cache->Add(this->MAO->CreateBTF(btf, "", ""));
	}

	//this->MAO->SetFact_BTFOnly(f, btf);

	result->start = StartWake;
	result->end = StartWake + f->cache[f->cache->Count-1]->fragment->Length;
	return result;
}

// This function finds the longest subword of a BTF that will localize
Range^ LSIProblemV3::ComputeBTF_Wake_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Range^ result = gcnew Range();
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	Fragment^ wakeFr = this->MAO->CreateFragment(W2->GetSymbolsFromPos(StartWake, EndWake), false, false);
	for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
	{
		bool done = false;
		Int32 iLength = 0;
		Int32 index = 0;

		do
		{
			String^ tmp = wakeFr->fragment->Substring(iLength, wakeFr->fragment->Length - iLength);
			index = f->btf[iBTF]->fragment->LastIndexOf(wakeFr->fragment->Substring(iLength, wakeFr->fragment->Length - iLength));

			if (index >= 0)
			{
				done = true;
			}

			iLength++;
		} while ((!done) && (iLength < wakeFr->fragment->Length));

		Fragment^ btf = this->MAO->CreateFragment(f->btf[iBTF]->fragment->Substring(0, index + wakeFr->fragment->Length), false, true);
		f->cache->Add(this->MAO->CreateBTF(btf, "", ""));
	}

	//this->MAO->SetFact_BTFOnly(f, btf);

	result->start = StartWake;
	result->end = StartWake + f->cache[f->cache->Count-1]->fragment->Length;
	return result;
}

Range^ LSIProblemV3::ComputeFragment_Wake_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake)
{
	Range^ result = gcnew Range();
	Fact^ f = this->MAO->GetFact(iGen, W1, iPos1);

	// Compute the left reserve and the the effective length of the word
	Int32 reserveLeft = 0;
	for (size_t iPos = 0; iPos < iPos1; iPos++)
	{
		Fact^ tmp = this->MAO->GetFact(iGen, W1, iPos);
		reserveLeft += tmp->length->min;
	}

	Int32 length = StartWake - reserveLeft + 1;
	length = Math::Min(length, (1 + StartWake - EndWake) + f->length->max);

	// Using the shorter length compute the new max fragment
	FragmentSigned^ fr = this->MAO->CreateSignedFragment(W2->GetSymbols(StartWake - length + 1, length), false, true, "", "");
	//this->MAO->SetFact_MaxFragmentOnly(iGen, W1, iPos1, fr, W2, StartWake - length + 1, StartWake);
	this->MAO->SetFact_MaxFragmentOnly_Direct(f, fr);

	result->start = StartWake;
	result->end = StartWake + length;
	return result;
}

void LSIProblemV3::ComputeLength_AbsoluteMinMax()
{
#if _PHASE3_VERBOSE_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Absolute Min/Max Length");
	Console::WriteLine("=================================");
#endif
	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		Int32 reserveRight = 0;
		Int32 reserveLeft = 0;

		for (size_t iPos = 1; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			if (!this->model->alphabet->IsTurtle(symbolIndex))
			{
				reserveRight += this->absoluteMinLength;
			}
			else
			{
				reserveRight++;
			}
		}

		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

			if (!this->model->alphabet->IsTurtle(symbolIndex))
			{
				this->MAO->SetFact_MinLengthOnly(iGen, 0, symbolIndex, iLeft, iRight, nullptr, this->absoluteMinLength);
				Int32 length = this->model->evidence[iGen + 1]->Length() - (reserveLeft + reserveRight);
				this->MAO->SetFact_MaxLengthOnly(iGen, 0, symbolIndex, iLeft, iRight, nullptr, length);
				reserveRight -= this->absoluteMinLength;
				reserveLeft += this->absoluteMinLength;
			}
			else
			{
				this->MAO->SetFact_MinLengthOnly(iGen, 0, symbolIndex, iLeft, iRight, nullptr, 1);
				this->MAO->SetFact_MaxLengthOnly(iGen, 0, symbolIndex, iLeft, iRight, nullptr, 1);
				reserveRight--;
				reserveLeft++;
			}
		}
	}
}

void LSIProblemV3::ComputeLength_Fragment()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];
		for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		{
			this->MAO->SetFact_MaxLengthOnly(f, f->btf[iBTF]->fragment->Length);
			if (f->btf[iBTF]->uncertain->Count)
			{
				Int32 longest = 0;
				for (size_t iUncertain = 0; iUncertain < f->btf[iBTF]->uncertain->Count; iUncertain++)
				{
					if (f->btf[iBTF]->uncertain[iUncertain]->fragment->Length > longest)
					{
						longest = f->btf[iBTF]->uncertain[iUncertain]->fragment->Length;
					}
				}
				this->MAO->SetFact_MaxLengthOnly(f, longest);
			}
		}
	}
}

void LSIProblemV3::ComputeLength_Growth(Int32 iGen)
{
#if	_PHASE3_COMPUTE_LENGTH_GROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length From Growth");
	Console::WriteLine("============================");
#endif

	Int32 iTable = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle(); iLeft++)
		{
			for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle(); iRight++)
			{
				Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);

				if (!this->MAO->IsNoFact(f))
				{ 
					MinMax^ length = gcnew MinMax();

					for (size_t i = 0; i <= f->growth->GetUpperBound(0); i++)
					{
						length->min += f->growth[i, 0];
						length->max += f->growth[i, 1];
					}

					this->MAO->SetFact_MinLengthOnly(iGen, iTable, iSymbol, iLeft, iRight, nullptr, length->min);
					this->MAO->SetFact_MaxLengthOnly(iGen, iTable, iSymbol, iLeft, iRight, nullptr, length->max);
#if	_PHASE3_COMPUTE_LENGTH_GROWTH_ >= 1
					Console::WriteLine("SAC " + this->MAO->SymbolCondition(iSymbol, iLeft, iRight) + " has min/max length " + length->min + " / " + length->max);
#endif
				}
			}
		}
	}
}

void LSIProblemV3::ComputeLength_Lexicon()
{
#if	_PHASE3_COMPUTE_LENGTH_LEXICON_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length from Lexicon");
	Console::WriteLine("=============================");
#endif
}

// TODO: there's a problem in here, I don't think the facts are being counted properly
void LSIProblemV3::ComputeLength_TotalSymbolProduction(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length from Symbol Production");
	Console::WriteLine("=======================================");
#endif

	FactCounter^ fc = this->ComputeFactCount(iGen, W1);
	Int32 total = W2->Length();

	for (size_t iFact1 = 0; iFact1 < fc->counter->Count; iFact1++)
	{
		if (!this->model->alphabet->IsTurtle(fc->counter[iFact1]->iSymbol))
		{
			Int32 producedMin = 0;
			Int32 producedMax = 0;

			for (size_t iFact2 = 0; iFact2 < fc->counter->Count; iFact2++)
			{
				if (iFact1 != iFact2)
				{
					producedMin += fc->counter[iFact2]->count * fc->counter[iFact2]->fact->length->min;
					producedMax += fc->counter[iFact2]->count * fc->counter[iFact2]->fact->length->max;
				}
			}

			Int32 remainderMin = total - producedMax;
			Int32 remainderMax = total - producedMin;

#if _PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >=1
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight) + " min/max remainder " + remainderMin + " / " + remainderMax + " = " + total + " / " + total + " - " + producedMax + " / " + producedMin);
#endif

			Int32 productionMax = remainderMax / fc->counter[iFact1]->count;
			Int32 productionMin = Math::Max(this->absoluteMinLength, remainderMin / fc->counter[iFact1]->count);

#if _PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >=1
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight) + " min/max length " + productionMin + " / " + productionMax + " = " + remainderMin + " / " + remainderMax + " div " + fc->counter[iFact1]->count + " / " + fc->counter[iFact1]->count);
			Console::WriteLine();
#endif

			this->MAO->SetFact_MaxLengthOnly(iGen, 0, fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight, nullptr, productionMax);
			this->MAO->SetFact_MinLengthOnly(iGen, 0, fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight, nullptr, productionMin);

		}
	}

}

void LSIProblemV3::ComputeLength_TotalSymbolProductionOLD(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length from Symbol Production");
	Console::WriteLine("=======================================");
#endif

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		if (this->MAO->symbolsByGeneration[iGen, iSymbol])
		{
			// Get the growth from iGen to iGen + 1
			array<Int32>^ PV = W2->parikhVector;

			// Compute the number of symbols produced
			Int32 total = 0;

			for (size_t i = 0; i <= PV->GetUpperBound(0); i++)
			{
				total += PV[i];
			}

			// 1st pass find the facts all symbols other than the symbol under consideration
			// E.g. if symbol is A, then find the facts for B,C,etc.
			// 2nd pass (below) will find the production by the same symbol under different conditions
			Int32 produced1stPassMin = 0; 
			Int32 produced1stPassMax = 0;  
			FactCounter^ fc = gcnew FactCounter();

			//this->model->evidence[iGen]->Display();
			//this->model->evidence[iGen + 1]->Display();

			// Scan the symbols in iGen, and compute the minimum at each symbol position
			for (size_t iPos = 0; iPos < W1->Length(); iPos++)
			{
				Int32 symbolIndex = W1->GetSymbolIndex(iPos);
				Int32 iLeft = W1->GetLeftContext(iPos);
				Int32 iRight = W1->GetRightContext(iPos);

				// TODO: need to get the proper table, proper left/right context and parameters
				if (symbolIndex != iSymbol)
				{
					Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
					produced1stPassMin += f->length->min;
					produced1stPassMax += f->length->max;
				}
				else
				{ // make a record that such a symbol exists and get its fact
					Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
					fc->Add(iGen, 0, symbolIndex, iLeft, iRight, f);
				}
			}

			// Compute how many symbols remain after reducing the total symbols in the generation by the min/max accounted for
			Int32 remainder1stPassMax = total - produced1stPassMin;
			Int32 remainder1stPassMin = total - produced1stPassMax;

			if (remainder1stPassMin < 0)
			{
				remainder1stPassMin = 0;
			}

#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
			Console::WriteLine("1st Pass For Symbol " + this->model->alphabet->symbols[iSymbol]);
			Console::WriteLine("Min / Max Remainder (" + remainder1stPassMin + " / " + remainder1stPassMax + ") = total (" + total + " / " + total + ") - produced (" + produced1stPassMax + "/" + produced1stPassMin + ")");
#endif
			// Now for the symbol + condition under consideration
			// compute the min/max produced by other symbols that do not share the condition
			// do this for every condition
			for (size_t i = 0; i < fc->counter->Count; i++)
			{
				Int32 remainder2ndPassMin = remainder1stPassMin;
				Int32 remainder2ndPassMax = remainder1stPassMax;
				Int32 produced2ndPassMin = 0;
				Int32 produced2ndPassMax = 0;
				for (size_t j = 0; j < fc->counter->Count; j++)
				{
					if (i != j)
					{
						produced2ndPassMin += fc->counter[j]->count * fc->counter[j]->fact->length->min;
						produced2ndPassMax += fc->counter[j]->count * fc->counter[j]->fact->length->max;
						//produced2ndPassMin += fc->counter[j]->fact->length->min;
						//produced2ndPassMax += fc->counter[j]->fact->length->max;
					}
				}

				// compute how many symbols remain
				remainder2ndPassMax -= produced2ndPassMin;
				remainder2ndPassMin -= produced2ndPassMax;

				if (remainder2ndPassMin < 0)
				{
					remainder2ndPassMin = 0;
				}
#if	_PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >= 1
				Console::WriteLine("2nd Pass for SAC " + this->MAO->SymbolCondition(fc->counter[i]->iSymbol, fc->counter[i]->iLeft, fc->counter[i]->iRight));
				Console::WriteLine("remainder (" + remainder2ndPassMin + " / " + remainder2ndPassMax + ") = total (" + total + ") - produced (" + produced2ndPassMax + "/" + produced2ndPassMin + ")");
#endif
				Int32 productionMax = remainder2ndPassMax / fc->counter[i]->count;
				Int32 productionMin = remainder2ndPassMin / fc->counter[i]->count;

				if (productionMin < this->absoluteMinLength)
				{
					productionMin = this->absoluteMinLength;
				}

#if _PHASE3_COMPUTE_LENGTH_TOTALPRODUCTION_ >=1
				Console::WriteLine("Found min/max length for SAC " + this->MAO->SymbolCondition(iSymbol, fc->counter[i]->iLeft, fc->counter[i]->iRight) + " of " + productionMin + " / " + productionMax);
				Console::WriteLine();
#endif
				this->MAO->SetFact_MaxLengthOnly(iGen, 0, iSymbol, fc->counter[i]->iLeft, fc->counter[i]->iRight, nullptr, productionMax);
				this->MAO->SetFact_MinLengthOnly(iGen, 0, iSymbol, fc->counter[i]->iLeft, fc->counter[i]->iRight, nullptr, productionMin);
			}
		}
	}
}


void LSIProblemV3::ComputeLength_Symbiology()
{
#if _PHASE3_COMPUTE_LENGTH_SYMBIOLOGY_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length from Symbiology");
	Console::WriteLine("================================");
#endif

	for (size_t i = 0; i < this->MAO->partialTotalLengthFacts->Count; i++)
	{
		for (size_t iFact1 = 0; iFact1 < this->MAO->partialTotalLengthFacts[i]->facts->Count; iFact1++)
		{
			Int32 remainderMin = this->MAO->partialTotalLengthFacts[i]->length->min;
			Int32 remainderMax = this->MAO->partialTotalLengthFacts[i]->length->max;
			for (size_t iFact2 = 0; iFact2 < this->MAO->partialTotalLengthFacts[i]->facts->Count; iFact2++)
			{
				if (iFact1 != iFact2)
				{
					remainderMin -= this->MAO->partialTotalLengthFacts[i]->facts[iFact2]->length->max;
					remainderMax -= this->MAO->partialTotalLengthFacts[i]->facts[iFact2]->length->min;
				}
			}

			SACX^ s = this->MAO->GetSAC(this->MAO->partialTotalLengthFacts[i]->facts[iFact1]);
#if _PHASE3_COMPUTE_LENGTH_SYMBIOLOGY_ >= 1
			Console::WriteLine("For SAC " + this->MAO->SymbolCondition(s->iSymbol, s->iLeft, s->iRight) + " min/max length = " + remainderMin + " / " + remainderMax);
#endif
			Int32 iTable = 0;

			this->MAO->SetFact_MinLengthOnly(s->iGen, iTable, s->iSymbol, s->iLeft, s->iRight, nullptr, remainderMin);
			this->MAO->SetFact_MaxLengthOnly(s->iGen, iTable, s->iSymbol, s->iLeft, s->iRight, nullptr, remainderMax);
		}
	}
}

// TODO: if all the lengths are known except one, the remaining must produce the remaining length
void LSIProblemV3::ComputeLength_TotalLength(Int32 iGen)
{
#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Length from Total Length");
	Console::WriteLine("==================================");
#endif

#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
		Console::WriteLine();
		Console::WriteLine("Processing G" + iGen);
#endif
	MinMax^ totalLengthAllSuccessors = this->MAO->GetTotalLengthAllSuccessors(iGen);

	Int32 iTable = 0;

	FactCounter^ fcAll = gcnew FactCounter();

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle()+1; iLeft++)
		{
			for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle()+1; iRight++)
			{
				// Capture all of the facts in this generation
				Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft-1, iRight-1, nullptr);

				if (f->ID != MasterAnalysisObject::NoFactID)
				{
					fcAll->Add(iGen, iTable, iSymbol, iLeft-1, iRight-1, f);
				}
			}
		}
	}

	Int32 remainderMax = 0;
	Int32 remainderMin = 0;

	// Compute the min/max for each symbol in the FactCounters
	// Based on if all other symbols are their min/max length, implies that the remaining symbol must have the remainder
#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
	Console::WriteLine("Based on Min/Max Total of All Successors");
#endif
	for (size_t iFact1 = 0; iFact1 < fcAll->counter->Count; iFact1++)
	{
		remainderMax = totalLengthAllSuccessors->max;
		remainderMin = totalLengthAllSuccessors->min;

		for (size_t iFact2 = 0; iFact2 < fcAll->counter->Count; iFact2++)
		{
			if (iFact1 != iFact2)
			{
				remainderMax -= fcAll->counter[iFact2]->fact->length->min;
				remainderMin -= fcAll->counter[iFact2]->fact->length->max;
			}
		}

#if _PHASE3_COMPUTE_LENGTH_TOTALLENGTH_ >= 1
		Console::WriteLine("Total Min/Max Length All Successors: " + totalLengthAllSuccessors->min + " / " + totalLengthAllSuccessors->max);
		Console::WriteLine("Symbol " + this->MAO->SymbolCondition(fcAll->counter[iFact1]->iSymbol, fcAll->counter[iFact1]->iLeft, fcAll->counter[iFact1]->iRight) + " produces at min/max " + remainderMin + " / " +  remainderMax);
#endif
		this->MAO->SetFact_MinLengthOnly(iGen, iTable, fcAll->counter[iFact1]->iSymbol, fcAll->counter[iFact1]->iLeft, fcAll->counter[iFact1]->iRight, nullptr, remainderMin);
		this->MAO->SetFact_MaxLengthOnly(iGen, iTable, fcAll->counter[iFact1]->iSymbol, fcAll->counter[iFact1]->iLeft, fcAll->counter[iFact1]->iRight, nullptr, remainderMax);
	}
}


// Find the SAC that appear least and most frequently
// the total min/max is equal to the sum of the individual lengths but this should be a worse result generally
void LSIProblemV3::ComputeTotalLength_Length(Int32 iGen)
{
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Total Length from Length");
	Console::WriteLine("==================================");
#endif

	Int32 iTable = 0;
	Int32 totalMin = 0;
	Int32 totalMax = 0;

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle()+1; iLeft++)
		{
			for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle() + 1; iRight++)
			{
				Fact^ f = this->MAO->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);

				if (!this->MAO->IsNoFact(f))
				{
					totalMin += f->length->min;
					totalMax += f->length->max;
				}
			}
		}
	}

#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine("For Gen #" + iGen + " min/max total successor length = " + totalMin + " / " + totalMax);
#endif
	this->MAO->SetTotalLengthSuccessors_MinOnly(iGen, totalMin);
	this->MAO->SetTotalLengthSuccessors_MaxOnly(iGen, totalMax);
}

// Find the SAC that appear least and most frequently
// Max total length is the sum of the least frequent handling the most # of symbols + min of the rest
// Min total length is the sum of the most frequent handling the most # of symbols + min of the rest
void LSIProblemV3::ComputeTotalLength_TotalSymbolProduction(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Total Length from Total Symbol Production");
	Console::WriteLine("===================================================");
#endif

	Int32 iTable = 0;
	Int32 totalMin = 0;
	Int32 totalMax = 0;
	Int32 totalMinCurrentGenOnly = 0;
	Int32 totalMaxCurrentGenOnly = 0;
	FactCounter^ fc = gcnew FactCounter();

	// Get a count of all SACs
	for (size_t iPos = 0; iPos < W1->Length(); iPos++)
	{
		Int32 symbolIndex = W1->GetSymbolIndex(iPos);
		Int32 iLeft = W1->GetLeftContext(iPos);
		Int32 iRight = W1->GetRightContext(iPos);

		if (!this->model->alphabet->IsTurtle(symbolIndex))
		{
			Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
			if (!this->MAO->IsNoFact(f))
			{
				fc->Add(iGen, 0, symbolIndex, iLeft, iRight, f);
			}
		}
	}

	Int32 most = 0;
	Int32 least = 999999;
	Int32 mostIndex = 0;
	Int32 leastIndex = 0;

	// Find the SACs that occur most/least frequently
	for (size_t iFact = 0; iFact < fc->counter->Count; iFact++)
	{
		if (fc->counter[iFact]->count > most)
		{
			most = fc->counter[iFact]->count;
			mostIndex = iFact;
		}

		if (fc->counter[iFact]->count < least)
		{
			least = fc->counter[iFact]->count;
			leastIndex = iFact;
		}
	}

	// Get the total growth from iGen to iGen + 1
	array<Int32>^ growth = this->CalculateGrowth(W1, W2);
	Int32 productionTotal = 0;
	for (size_t i = 0; i <= growth->GetUpperBound(0); i++)
	{
		productionTotal += growth[i];
	}

	Int32 remainderMax = productionTotal;
	Int32 remainderMin = productionTotal;

	for (size_t iFact = 0; iFact < fc->counter->Count; iFact++)
	{
		if (iFact != leastIndex)
		{
			remainderMax -= fc->counter[iFact]->count * fc->counter[iFact]->fact->length->min;
			totalMax += fc->counter[iFact]->fact->length->min;
		}

		if (iFact != mostIndex)
		{
			remainderMin -= fc->counter[iFact]->count * fc->counter[iFact]->fact->length->min;
			totalMin += fc->counter[iFact]->fact->length->min;
		}
	}

	totalMax += remainderMax / fc->counter[leastIndex]->count;
	totalMin += remainderMin / fc->counter[mostIndex]->count;

#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine("For Gen #" + iGen + " min/max total successor length = " + totalMin + " / " + totalMax);
#endif

	this->MAO->SetTotalLengthSuccessors_MinOnly(iGen, totalMin);
	this->MAO->SetTotalLengthSuccessors_MaxOnly(iGen, totalMax);
}

// The total length of generation N can be reduced by the lengths of the SACs from previous generations
// Thus the effect is a length equation for only those SACs that appear in this generation
void LSIProblemV3::ComputeTotalLength_Symbiology(Int32 iGen)
{
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Total Length from Symbiology");
	Console::WriteLine("======================================");
#endif

	Int32 iTable = 0;
	Int32 remainderMin = this->MAO->GetTotalLengthAllSuccessors(iGen)->min;
	Int32 remainderMax = this->MAO->GetTotalLengthAllSuccessors(iGen)->max;
	Int32 totalMinCurrentGenOnly = 0;
	Int32 totalMaxCurrentGenOnly = 0;

	// Compute the total length of the appearing SACs
	List<SACX^>^ appearing = this->MAO->GetAppearingSACs(iGen - 1, iGen);
	List<Fact^>^ facts = gcnew List<Fact^>;

#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine(" For SACs ");
#endif
	for (size_t iSAC = 0; iSAC < appearing->Count; iSAC++)
	{
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
		Console::WriteLine(this->MAO->SymbolCondition(appearing[iSAC]->iSymbol, appearing[iSAC]->iLeft, appearing[iSAC]->iRight));
#endif
		facts->Add(this->MAO->GetFact(appearing[iSAC]));
	}
	TotalLengthFact^ f = gcnew TotalLengthFact;
	f->facts = facts;

	// get all of the non-appearing facts
	List<Fact^>^ remaining = gcnew List<Fact^>;
	for (size_t iSAC = 0; iSAC < this->MAO->sacsByGeneration[iGen]->Count; iSAC++)
	{
		Fact^ f = this->MAO->GetFact(this->MAO->sacsByGeneration[iGen][iSAC]);
		if (!facts->Contains(f))
		{
			remaining->Add(f);
		}
	}

	// for each partial length fact in the table
	for (size_t iFact1 = 0; iFact1 < this->MAO->partialTotalLengthFacts->Count; iFact1++)
	{
		bool match = true;
		Int32 iFact2 = 0;
		// check to see if it matches a subset of facts in the remaining list
		while ((match) && (iFact2 < this->MAO->partialTotalLengthFacts[iFact1]->facts->Count))
		{
			if (!remaining->Contains(this->MAO->partialTotalLengthFacts[iFact1]->facts[iFact2]))
			{
				match = false;
			}
			else
			{
				iFact2++;
			}
		}

		if (match)
		{// this partial total length matches so deduct the length from the total and remove the facts from the remaining list
			remainderMin -= this->MAO->partialTotalLengthFacts[iFact1]->length->max;
			remainderMax -= this->MAO->partialTotalLengthFacts[iFact1]->length->min;
			for (size_t i = 0; i < this->MAO->partialTotalLengthFacts[iFact1]->facts->Count; i++)
			{
				remaining->Remove(this->MAO->partialTotalLengthFacts[iFact1]->facts[i]);
			}
		}
	}

	for (size_t i = 0; i < remaining->Count; i++)
	{
		remainderMin -= remaining[i]->length->max;
		remainderMax -= remaining[i]->length->min;
	}

	// for the remaining facts, see if there is a set in the total length fact table 
	f->length = gcnew MinMax;
	f->length->min = remainderMin;
	f->length->max = remainderMax;
#if _PHASE3_COMPUTE_TOTALLENGTH_LENGTH_ >= 1
	Console::WriteLine("Min/max total length = " + f->length->min + " / " + f->length->max);
#endif
	this->MAO->partialTotalLengthFacts->Add(f);
}

// Look at the growth
// For max, no symbol can produce more than the complete growth from one generation to another
// For min, no growth at all
void LSIProblemV3::ComputeGrowth_AbsoluteMinMax()
{
#if _PHASE3_VERBOSE_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Absolute Min/Max Growth");
	Console::WriteLine("=================================");
#endif
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		FactCounter^ fc = gcnew FactCounter();

#if _PHASE3_VERBOSE_ >= 2
		this->model->evidence[iGen]->Display();
		this->model->evidence[iGen + 1]->Display();
#endif
		array<Int32>^ growth = this->CalculateGrowth(this->model->evidence[iGen], this->model->evidence[iGen + 1]);
#if _PHASE3_VERBOSE_ >= 1
		this->DisplayParikhVector(growth, "Growth: ");
#endif
		// Scan the evidence, capture how often each symbol + condition exists, reduce growth for the turtles
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);
			Int32 iLeft = this->model->evidence[iGen]->GetLeftContext(iPos);
			Int32 iRight = this->model->evidence[iGen]->GetRightContext(iPos);

			if (!this->model->alphabet->IsTurtle(symbolIndex))
			{
				Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
				fc->Add(iGen, 0, symbolIndex, iLeft, iRight, f);
			}
		}

		// The maximum growth for each symbol, is the min growth + unaccounted for growth divided by the count of number of symbols
		for (size_t iFact = 0; iFact < fc->counter->Count; iFact++)
		{
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
			{
				Int32 maxGrowth = fc->counter[iFact]->fact->growth[fc->counter[iFact]->iSymbol, 0] + floor(growth[iSymbol] / fc->counter[iFact]->count);
#if _PHASE3_VERBOSE_ >= 1
				Console::WriteLine("Max Growth: Symbol " + this->MAO->SymbolCondition(fc->counter[iFact]->iSymbol, fc->counter[iFact]->iLeft, fc->counter[iFact]->iRight) + " -> " + maxGrowth + this->model->alphabet->symbols[iSymbol]);
#endif
				this->MAO->SetFact_MaxGrowthOnly(fc->counter[iFact]->iGen, fc->counter[iFact]->iTable, fc->counter[iFact]->iSymbol, fc->counter[iFact]->iLeft, fc->counter[iFact]->iRight, nullptr, iSymbol, maxGrowth);
			}
		}
	}
}

void LSIProblemV3::ComputeGrowth_Fragment()
{
	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ f = this->MAO->factsList[iFact];

		for (size_t iBTF = 0; iBTF < f->btf->Count; iBTF++)
		{
			array<Int32>^ PV = this->model->alphabet->CreateParikhVector(f->btf[iBTF]->fragment);
			this->MAO->SetFact_MaxGrowthOnly(f, PV);
			if (f->btf[iBTF]->uncertain->Count)
			{
				PV = gcnew array<Int32>(PV->Length);
				for (size_t iUncertain = 0; iUncertain < f->btf[iBTF]->uncertain->Count; iUncertain++)
				{
					array<Int32>^ tmp = this->model->alphabet->CreateParikhVector(f->btf[iBTF]->fragment);
					for (size_t i = 0; i < tmp->GetUpperBound(0); i++)
					{
						if (tmp[i] > PV[i])
						{
							PV[i] = tmp[i];
						}
					}
				}
				this->MAO->SetFact_MaxGrowthOnly(f, PV);
			}
		}
	}
}

// Computes the growth based on the lexicon for the symbol
void LSIProblemV3::ComputeGrowth_Lexicon()
{
#if _PHASE3_COMPUTE_GROWTH_LEXICON_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Growth from Lexicon");
	Console::WriteLine("=============================");
#endif
}

// Analyze growth pattern
// A. a SAC does not produce a symbol S if the growth of S = 0 in the generation immediately following it
// B. if growth is constant for a symbol S1, than any symbols that appeared in the previous generation did not produce the symbol S1 unless a symbol has vanished which may have produced S1
// Example, if Growth of A is 4 for G1 => G2 and is still 4 for G2 => G3 then if SAC B appeared in G2 it does not produce A unless another SAC appeared in G2 with G,max(A) > 0
void LSIProblemV3::ComputeGrowth_Symbiology(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_GROWTH_SYMBIOLOGY_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Growth from Symbiology");
	Console::WriteLine("================================");
#endif
	FactCounter^ fc = gcnew FactCounter();

	//this->model->evidence[iGen]->Display();
	//this->model->evidence[iGen + 1]->Display();
	array<Int32>^ growthT0 = this->CalculateGrowth(W1, W2);

#if _PHASE3_COMPUTE_GROWTH_SYMBIOLOGY_ >= 1
	this->DisplayParikhVector(growthT0, "Total Growth G" + iGen + "=>G"+(iGen+1)+": ");
#endif
	// Check to see if the growth is zero for any symbol
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->numNonTurtles; iSymbol++)
	{
		// A. a SAC does not produce a symbol S if the growth of S = 0 in the generation immediately following it
		if (growthT0[iSymbol] == 0)
		{
			this->MAO->SetFact_MinGrowthOnly(iGen, iSymbol, 0);
			this->MAO->SetFact_MaxGrowthOnly(iGen, iSymbol, 0);
		}
	}

	if (iGen < this->model->generations - 2)
	{
		//this->model->evidence[iGen + 2]->Display();
		List<SACX^>^ appeared = this->MAO->GetAppearingSACs(iGen, iGen + 1);

		if (appeared->Count > 0)
		{
			Int32 iTable = 0;
			array<Int32>^ growthT1 = this->CalculateGrowth(this->model->evidence[iGen + 1], this->model->evidence[iGen + 2]);
#if _PHASE3_COMPUTE_GROWTH_SYMBIOLOGY_ >= 1
			this->DisplayParikhVector(growthT1, "Total Growth G" + (iGen + 1) + "=>G" + (iGen + 2) + ": ");
#endif
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
			{
				// B. if growth is constant for a symbol S1, than any SAC that appeared in the previous generation did not produce the symbol S1 unless a SAC has vanished which may have produced S1
				// Example, if Growth of A is 4 for G1 => G2 and is still 4 for G2 => G3, if SAC C appears in G2 then it does not produce A unless a SAC in G2 disappeared with G,max(A) > 0
				if (growthT0[iSymbol] == growthT1[iSymbol])
				{
					// Get any SACs that disappeared in G2
					List<SACX^>^ disappeared = this->MAO->GetDisappearingSACs(iGen, iGen + 1);

					bool ruleInvalid = false;
					Int32 iDisappeared = 0;

					// Check to see if any SAC that disappeared has G,max(iSymbol) > 0
					while ((!ruleInvalid) && (iDisappeared < disappeared->Count))
					{
						Fact^ f = this->MAO->GetFact(disappeared[iDisappeared]->iGen, iTable, disappeared[iDisappeared]->iSymbol, disappeared[iDisappeared]->iLeft, disappeared[iDisappeared]->iRight, nullptr);
						if (f->growth[iSymbol, 1] > 0)
						{
							ruleInvalid = true;
						}
						iDisappeared++;
					}

					if (!ruleInvalid)
					{
						//Fact^ f = this->MAO->GetFact(disappeared[iDisappeared]->iGen, iTable, disappeared[iDisappeared]->iSymbol, disappeared[iDisappeared]->iLeft, disappeared[iDisappeared]->iRight, nullptr);

						// Change the growth iSymbol for any SAC that appeared to 0
						for (size_t iSAC = 0; iSAC < appeared->Count; iSAC++)
						{
							this->MAO->SetFact_MinGrowthOnly(appeared[iSAC]->iGen, iTable, appeared[iSAC]->iSymbol, appeared[iSAC]->iLeft, appeared[iSAC]->iRight, nullptr, iSymbol, 0);
							this->MAO->SetFact_MaxGrowthOnly(appeared[iSAC]->iGen, iTable, appeared[iSAC]->iSymbol, appeared[iSAC]->iLeft, appeared[iSAC]->iRight, nullptr, iSymbol, 0);
						}
					}
				}
			}
		}
	}
}

// works similar to total length
// if a total of 7As are produced and 4As are produced max by all but one symbol then the remaining symbol produces 3
// also, if all the growths are known except one then the remaining symbol must produce everything else
void LSIProblemV3::ComputeGrowth_TotalGrowth(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Growth from Total Growth");
	Console::WriteLine("==================================");
#endif

	array<Int32>^ growth = this->CalculateGrowth(W1, W2);
#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
	this->DisplayParikhVector(growth, "Total Growth: ");
#endif
	// Get a count of all the facts
	FactCounter^ fc = this->ComputeFactCount(iGen, W1);

	array<Int32>^ remainderMin = gcnew array<Int32>(growth->Length);
	array<Int32>^ remainderMax = gcnew array<Int32>(growth->Length);

	// Compute the min/max for each symbol in the Factor
	// Based on if all other symbols are their min/max length, implies that the remaining symbol must have the remainder
#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
	Console::WriteLine("Based on Min/Max Growth of All Successors Except One");
#endif
	for (size_t iFact1 = 0; iFact1 < fc->counter->Count; iFact1++)
	{
		if (!this->model->alphabet->IsTurtle(fc->counter[iFact1]->iSymbol))
		{
			Array::Copy(growth, remainderMin, growth->Length);
			Array::Copy(growth, remainderMax, growth->Length);

			for (size_t iFact2 = 0; iFact2 < fc->counter->Count; iFact2++)
			{
				if ((iFact1 != iFact2) && (!this->model->alphabet->IsTurtle(fc->counter[iFact2]->iSymbol)))
				{
					for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
					{
#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
						Console::WriteLine("Reducing remainderMax[" + this->model->alphabet->symbols[iSymbol] + "] = " + remainderMax[iSymbol] + " by " + this->MAO->SymbolCondition(fc->counter[iFact2]->iSymbol, fc->counter[iFact2]->iLeft, fc->counter[iFact2]->iRight) + " min growth of " + fc->counter[iFact2]->fact->growth[iSymbol, 0] + " x " + fc->counter[iFact2]->count);
						//Console::WriteLine("Reducing remainderMin[" + this->model->alphabet->symbols[iSymbol] + "] = " + remainderMin[iSymbol] + " by " + this->MAO->SymbolCondition(fc->counter[iFact2]->iSymbol, fc->counter[iFact2]->iLeft, fc->counter[iFact2]->iRight) + " max growth of " + fc->counter[iFact2]->fact->growth[iSymbol, 1] + " x " + fc->counter[iFact2]->count);
#endif
						remainderMax[iSymbol] -= fc->counter[iFact2]->count * fc->counter[iFact2]->fact->growth[iSymbol, 0];
						remainderMin[iSymbol] -= fc->counter[iFact2]->count * fc->counter[iFact2]->fact->growth[iSymbol, 1];
					}
				}
			}

			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
			{
				remainderMin[iSymbol] /= fc->counter[iFact1]->count;
				remainderMax[iSymbol] /= fc->counter[iFact1]->count;
			}

#if	_PHASE3_COMPUTE_GROWTH_TOTALGROWTH_ >= 1
			this->DisplayParikhVector(remainderMin, this->MAO->SymbolCondition(fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight) + " has min growth:");
			this->DisplayParikhVector(remainderMax, this->MAO->SymbolCondition(fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight) + " has max growth:");
#endif
			Int32 iTable = 0;
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
			{
				this->MAO->SetFact_MinGrowthOnly(iGen, iTable, fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight, nullptr, iSymbol, remainderMin[iSymbol]);
				this->MAO->SetFact_MaxGrowthOnly(iGen, iTable, fc->counter[iFact1]->iSymbol, fc->counter[iFact1]->iLeft, fc->counter[iFact1]->iRight, nullptr, iSymbol, remainderMax[iSymbol]);
			}
		}
	}
}

void LSIProblemV3::ComputeGrowth_UnaccountedGrowth(Int32 iGen, WordXV3^ W1, WordXV3^ W2)
{
#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Growth from UAG");
	Console::WriteLine("=========================");
#endif

	FactCounter^ fc = gcnew FactCounter();

	//this->model->evidence[iGen]->Display();
	//this->model->evidence[iGen + 1]->Display();
	array<Int32>^ growth = this->CalculateGrowth(W1, W2);
#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
	this->DisplayParikhVector(growth, "Total Growth: ");
#endif
	// Scan the evidence, capture how often each symbol + condition exists, reduce growth for the turtles
	for (size_t iPos = 0; iPos < W1->Length(); iPos++)
	{
		Int32 symbolIndex = W1->GetSymbolIndex(iPos);
		Int32 iLeft = W1->GetLeftContext(iPos);
		Int32 iRight = W1->GetRightContext(iPos);

		if (!this->model->alphabet->IsTurtle(symbolIndex))
		{
			Fact^ f = this->MAO->GetFact(iGen, 0, symbolIndex, iLeft, iRight, nullptr);
			fc->Add(iGen, 0, symbolIndex, iLeft, iRight, f);
		}
	}

	// For each symbol, reduce the growth by the minimum
	for (size_t iFact = 0; iFact < fc->counter->Count; iFact++)
	{
		growth[fc->counter[iFact]->iSymbol] -= fc->counter[iFact]->fact->growth[fc->counter[iFact]->iSymbol, 0];
	}

#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
	this->DisplayParikhVector(growth, "Unaccounted Growth: ");
	Console::WriteLine("Gen " + iGen + " has " + fc->counter->Count + " Unique Facts ");
#endif
	// The maximum growth for each symbol, is the min growth + unaccounted for growth divided by the count of number of symbols
	for (size_t iFact = 0; iFact < fc->counter->Count; iFact++)
	{
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
		{
			Int32 maxGrowth = fc->counter[iFact]->fact->growth[fc->counter[iFact]->iSymbol, 0] + floor(growth[iSymbol] / fc->counter[iFact]->count);
#if _PHASE3_COMPUTE_GROWTH_UAG_ >= 1
			Console::WriteLine("Max Growth: Symbol " + this->MAO->SymbolCondition(fc->counter[iFact]->iSymbol, fc->counter[iFact]->iLeft, fc->counter[iFact]->iRight) + " -> " + maxGrowth + this->model->alphabet->symbols[iSymbol]);
#endif
			this->MAO->SetFact_MaxGrowthOnly(fc->counter[iFact]->iGen, fc->counter[iFact]->iTable, fc->counter[iFact]->iSymbol, fc->counter[iFact]->iLeft, fc->counter[iFact]->iRight, nullptr, iSymbol, maxGrowth);
		}
	}
}

void LSIProblemV3::ComputeLexicon()
{
}

void LSIProblemV3::ComputeSymbolLexicons()
{
	array<Int32, 3>^ symbolRightLexicon = gcnew array<Int32, 3>(this->model->generations, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count);
	array<Int32, 4>^ symbolDoubleRightLexicon = gcnew array<Int32, 4>(this->model->generations, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count);
	array<Int32, 3>^ symbolLeftLexicon = gcnew array<Int32, 3>(this->model->generations, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count);
	array<Int32, 4>^ symbolDoubleLeftLexicon = gcnew array<Int32, 4>(this->model->generations, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count);
	array<Int32, 4>^ symbolLeftRightLexicon = gcnew array<Int32, 4>(this->model->generations, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count, this->model->alphabet->symbols->Count);

	for (size_t iGen = 0; iGen <= this->model->evidence->GetUpperBound(0); iGen++)
	{
		String^ current = "";
		String^ previous = "";
		String^ left = "";
		String^ right = "";

		Int32 currentSymbolID = -1;
		Int32 previousSymbolID = -1;
		Int32 leftSymbolID = -1;
		Int32 rightSymbolID = -1;

		//this->model->evidence[iGen]->Display();

		for (size_t iSymbol = 0; iSymbol < this->model->evidence[iGen]->Length(); iSymbol++)
		{
			// copy current symbols into previous before getting the current symbol
			previous = current;
			previousSymbolID = currentSymbolID;
			// reset the left and right IDs
			leftSymbolID = -1;
			rightSymbolID = -1;

			// get the current symbol
			current = this->model->evidence[iGen]->GetSymbol(iSymbol);
			currentSymbolID = this->model->alphabet->FindIndex(current);

			// Get Left Neighbour
			if (iSymbol > 0)
			{
				left = this->model->evidence[iGen]->GetSymbol(iSymbol - 1);
				leftSymbolID = this->model->alphabet->FindIndex(left);
			}

			// Get Right Neighbour
			if (iSymbol < this->model->evidence[iGen]->Length()-1)
			{
				right = this->model->evidence[iGen]->GetSymbol(iSymbol + 1);
				rightSymbolID = this->model->alphabet->FindIndex(right);
			}

			if ((currentSymbolID != -1) && (leftSymbolID != -1))
			{
				symbolLeftLexicon[iGen, currentSymbolID, leftSymbolID]++;
			}

			if ((currentSymbolID != -1) && (rightSymbolID != -1))
			{
				symbolRightLexicon[iGen, currentSymbolID, rightSymbolID]++;
			}

			if ((currentSymbolID != -1) && (leftSymbolID != -1) && (rightSymbolID != -1))
			{
				symbolLeftRightLexicon[iGen, currentSymbolID, leftSymbolID, rightSymbolID]++;
			}

			if ((currentSymbolID != -1) && (previousSymbolID != -1) && (leftSymbolID != -1))
			{
				symbolDoubleLeftLexicon[iGen, currentSymbolID, previousSymbolID, leftSymbolID]++;
			}

			if ((currentSymbolID != -1) && (previousSymbolID != -1) && (rightSymbolID != -1))
			{
				symbolDoubleRightLexicon[iGen, currentSymbolID, previousSymbolID, rightSymbolID]++;
			}
		}
	}

	this->MAO->SetSymbolLexicons(symbolRightLexicon, symbolDoubleRightLexicon, symbolLeftLexicon, symbolDoubleLeftLexicon, symbolLeftRightLexicon);
}

void LSIProblemV3::ComputeSymbolLocalization_Analyze(Int32 iGen)
{
	// LOCALIZATION ANALYSIS
	// STEP 1 - This step remove reduces the number of localizations.
	// A1. If a turtle only options is a position, then no other symbol can produce that turtle
	// A2. If a turtle can only be produced in a single spot it must be produced in that spot
	// Example:  +: X(4) +(5) <---- remove this
	//           +: +(5)   <---- because this must the localization
	// B1. A symbol cannot be localized to a position if the next position does not have a localization equal or greater than the current symbol
	// Example: Sn   : 4,5,6 <---- remove the 6
	//          Sn+1 : 4,5
	// then Sn cannot be localized to symbol 6
	// B2. A symbol cannot be localized to a position if the previous position does not have a localization equal or greater than the current symbol
	// Example: Sn   : 5,6
	//          Sn+1 : 4,5,6 <--- remove the 4
	// C1. No symbol can be localized after the last localization of the previous symbol
	// C2. No symbol can be localized before the first localization of the next symbol

	// STEP 2 - these steps should finalize the localization settings, i.e. converting 1s to 2s or 1s to 0s
	// A1. If a symbol has any value is locked then remove the remainder
	// Example: Sn: 4,5*,6  <---- remove the 4 and 6
	// A2. If a symbol has only a single precedessor set it to 2

	// Repeat Steps 1 & 2 until convergence

	// STEP 3 - these steps discovery facts about the L-system, such as fragments
	// A. if a set of symbols can only be produced by one symbol, then that set is a fragment
	// i. if the uniqueness is at the start of the production it is a head fragment 
	// ii. if the uniqueness is at the end of the production it is a tail fragment 
	// iii. if the uniqueness is at the middle of the production it is a mid fragment 
	// iv. if the uniqueness is at the start and end of the production it is a complete fragment
	// B. Remove any fragment which could not be localized into a wake, mark as tabu		

	// STEP 1 - This step remove reduces the number of localizations repeat until convergence
	bool change = false;

	do
	{
		change = false;

		// A1. If a turtle only options is a position, then no other symbol can produce that turtle
		// Example: +: 4
		// Example: +: 4,5
		// Example: +: 5,6  <--- if this is the only option for 6 then 6 must produce this, so remove 5
		for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
		{
			// Do process for A1
			if (this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos1)))
			{
				Int32 count = 0;
				Int32 idx = 0;
				Int32 iPos2 = 0;
				while ((count < 2) && (iPos2 < this->model->evidence[iGen + 1]->Length()))
				{
					if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
					{
						count++;
						idx = iPos2;
					}
					iPos2++;
				}

				if (count == 1)
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
					{
						if ((this->MAO->localization[iGen][idx, iPos3] == MasterAnalysisObject::cLocalization_Strong) && (iPos1 != iPos3))
						{
							//Console::WriteLine("Setting to false " + this->model->evidence[iGen + 1]->GetSymbol(iPos3) + " : " + this->model->evidence[iGen]->GetSymbol(idx) + "(" + idx+ ")");
							this->MAO->SetLocalization(iGen, idx, iPos3, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
						else if ((this->MAO->localization[iGen][idx, iPos3] == MasterAnalysisObject::cLocalization_Locked) && (iPos1 != iPos3))
						{
							Console::WriteLine("Uh oh!");
						}
					}
				}
			}

			// Do process for C1 & C2. Find the first and last occurance for each symbol
			Int32 first = -1;
			Int32 last = 0;
			Int32 iPos2 = 0;
			bool locked = false;

			while ((!locked) && (iPos2 < this->model->evidence[iGen + 1]->Length()))
			{
				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) && (first == -1) && (!locked))
				{
					first = iPos2;
				}

				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) && (!locked))
				{
					last = iPos2;
				}

				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					first = iPos2;
					last = iPos2;
					locked = true;
				}
				iPos2++;
			}

			// C1. No symbol can be localized after the last localization of the previous symbol
			// Example:  Sn: 3,4 <--- if this is the last instance of 4
			//           Sn+1: 3,5 <--- remove the 3
			for (size_t iPos2 = last + 1; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				for (Int32 iPos3 = 0; iPos3 < Math::Max(0, iPos1 - 1); iPos3++)
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
					//this->MAO->localization[iGen][iPos2, iPos3] == cLocalization_Unset;
				}
			}

			// C2. No symbol can be localized before the first localization of the next symbol
			// Example:  Sn: 3,5 <--- remove the 5
			//           Sn+1: 3,4 <--- if this is the first instance of 4
			for (size_t iPos2 = 0; iPos2 < Math::Max(0, first - 1); iPos2++)
			{
				for (size_t iPos3 = iPos1 + 1; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
					//this->MAO->localization[iGen][iPos2, iPos3] == cLocalization_Unset;
				}
			}
		}

		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			// A2. If a turtle can only be produced by a turtle, then it must be produced by that turtle
			if (this->model->alphabet->IsTurtle(this->model->evidence[iGen + 1]->GetSymbolIndex(iPos2)))
			{
				Int32 count = 0;
				Int32 idx = 0;
				Int32 iPos1 = 0;
				while ((count < 2) && (iPos1 < this->model->evidence[iGen]->Length()))
				{
					if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
					{
						count++;
						idx = iPos1;
					}
					iPos1++;
				}

				// if the turtle can only be produced by a single turtle then eliminate all other possibilities
				if ((count == 1) && (this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(idx))))
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen + 1]->Length(); iPos3++)
					{
						if ((this->MAO->localization[iGen][iPos3, idx] == MasterAnalysisObject::cLocalization_Strong) && (iPos2 != iPos3))
						{
							this->MAO->SetLocalization(iGen, iPos3, idx, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
						else if ((this->MAO->localization[iGen][iPos3, idx] == MasterAnalysisObject::cLocalization_Locked) && (iPos2 != iPos3))
						{
							Console::WriteLine("Uh oh!");
						}
					}
				}
			}

			// B1. A symbol cannot be localized to a position if the next position does not have a localization equal or greater than the current symbol
			Int32 largest = 0;
			Int32 smallest = 999999;
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if ((iPos2 < this->model->evidence[iGen + 1]->Length() - 1) && ((this->MAO->localization[iGen][iPos2 + 1, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2 + 1, iPos1] == MasterAnalysisObject::cLocalization_Locked)) && (iPos1 > largest))
				{
					largest = iPos1;
				}

				if ((iPos2 > 0) && ((this->MAO->localization[iGen][iPos2 - 1, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2 - 1, iPos1] == MasterAnalysisObject::cLocalization_Locked)) && (iPos1 < smallest))
				{
					smallest = iPos1;
				}
			}

			// Nothing larger than the largest in the next symbol can produce the current symbol
			if (iPos2 < this->model->evidence[iGen + 1]->Length() - 1)
			{
				for (size_t iPos1 = largest + 1; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
					{
						this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
						change = true;
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::WriteLine("Uh oh!");
					}
				}
			}

			// Nothing smaller than the smallest in the previous symbol can produce the current symbol
			if (iPos2 > 0)
			{
				for (size_t iPos1 = 0; iPos1 < Math::Max(0, smallest - 1); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
					{
						this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
						change = true;
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::WriteLine("Uh oh!");
					}
				}
			}

			// STEP 2 - these steps should convert finalize the localization settings, i.e. converting 1s to 2s or 1s to 0s
			// A1. If a symbol has any value is locked then remove the remainder
			// A2. If a symbol has only a single precedessor set it to 2
			Int32 count = 0;
			Int32 idx = 0;
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
					{
						if ((iPos1 != iPos3) && (this->MAO->localization[iGen][iPos2, iPos3] != MasterAnalysisObject::cLocalization_Never))
						{
							this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
					}
				}
				else if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
				{
					count++;
					idx = iPos1;
				}
			}

			if (count == 1)
			{
				this->MAO->SetLocalization(iGen, iPos2, idx, MasterAnalysisObject::cLocalization_Locked);
				change = true;
			}
		}
	} while (change);
}

void LSIProblemV3::ComputeSymbolLocalization_Analyze_BTFOnly(Int32 iGen)
{
	// LOCALIZATION ANALYSIS
	// STEP 1 - This step remove reduces the number of localizations.
	// A1. If a turtle only options is a position, then no other symbol can produce that turtle
	// A2. If a turtle can only be produced in a single spot it must be produced in that spot
	// Example:  +: X(4) +(5) <---- remove this
	//           +: +(5)   <---- because this must the localization
	// B1. A symbol cannot be localized to a position if the next position does not have a localization equal or greater than the current symbol
	// Example: Sn   : 4,5,6 <---- remove the 6
	//          Sn+1 : 4,5
	// then Sn cannot be localized to symbol 6
	// B2. A symbol cannot be localized to a position if the previous position does not have a localization equal or greater than the current symbol
	// Example: Sn   : 5,6
	//          Sn+1 : 4,5,6 <--- remove the 4
	// C1. No symbol can be localized after the last localization of the previous symbol
	// C2. No symbol can be localized before the first localization of the next symbol

	// STEP 2 - these steps should finalize the localization settings, i.e. converting 1s to 2s or 1s to 0s
	// A1. If a symbol has any value is locked then remove the remainder
	// Example: Sn: 4,5*,6  <---- remove the 4 and 6
	// A2. If a symbol has only a single precedessor set it to 2

	// Repeat Steps 1 & 2 until convergence

	// STEP 3 - these steps discovery facts about the L-system, such as fragments
	// A. if a set of symbols can only be produced by one symbol, then that set is a fragment
	// i. if the uniqueness is at the start of the production it is a head fragment 
	// ii. if the uniqueness is at the end of the production it is a tail fragment 
	// iii. if the uniqueness is at the middle of the production it is a mid fragment 
	// iv. if the uniqueness is at the start and end of the production it is a complete fragment
	// B. Remove any fragment which could not be localized into a wake, mark as tabu		

	// STEP 1 - This step remove reduces the number of localizations repeat until convergence
	bool change = false;

	do
	{
		change = false;

		// A1. If a turtle only options is a position, then no other symbol can produce that turtle
		// Example: +: 4
		// Example: +: 4,5
		// Example: +: 5,6  <--- if this is the only option for 6 then 6 must produce this, so remove 5
		for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
		{
			// Do process for A1
			if (this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(iPos1)))
			{
				Int32 count = 0;
				Int32 idx = 0;
				Int32 iPos2 = 0;
				while ((count < 2) && (iPos2 < this->model->evidence[iGen + 1]->Length()))
				{
					if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
					{
						count++;
						idx = iPos2;
					}
					iPos2++;
				}

				if (count == 1)
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
					{
						if ((this->MAO->localization[iGen][idx, iPos3] == MasterAnalysisObject::cLocalization_StrongBTF) && (iPos1 != iPos3))
						{
							this->MAO->SetLocalization(iGen, idx, iPos3, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
						else if ((this->MAO->localization[iGen][idx, iPos3] == MasterAnalysisObject::cLocalization_Locked) && (iPos1 != iPos3))
						{
							Console::WriteLine("Uh oh!");
						}
					}
				}
			}

			// Do process for C1 & C2. Find the first and last occurance for each symbol
			Int32 first = -1;
			Int32 last = 0;
			Int32 iPos2 = 0;
			bool locked = false;

			while ((!locked) && (iPos2 < this->model->evidence[iGen + 1]->Length()))
			{
				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) && (first == -1) && (!locked))
				{
					first = iPos2;
				}

				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) && (!locked))
				{
					last = iPos2;
				}

				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					first = iPos2;
					last = iPos2;
					locked = true;
				}
				iPos2++;
			}

			// C1. No symbol can be localized after the last localization of the previous symbol
			// Example:  Sn: 3,4 <--- if this is the last instance of 4
			//           Sn+1: 3,5 <--- remove the 3
			for (size_t iPos2 = last + 1; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				for (Int32 iPos3 = 0; iPos3 < Math::Max(0, iPos1 - 1); iPos3++)
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
				}
			}

			// C2. No symbol can be localized before the first localization of the next symbol
			// Example:  Sn: 3,5 <--- remove the 5
			//           Sn+1: 3,4 <--- if this is the first instance of 4
			for (size_t iPos2 = 0; iPos2 < Math::Max(0, first - 1); iPos2++)
			{
				for (size_t iPos3 = iPos1 + 1; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
				}
			}
		}

		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			// A2. If a turtle can only be produced by a turtle, then it must be produced by that turtle
			if (this->model->alphabet->IsTurtle(this->model->evidence[iGen + 1]->GetSymbolIndex(iPos2)))
			{
				Int32 count = 0;
				Int32 idx = 0;
				Int32 iPos1 = 0;
				while ((count < 2) && (iPos1 < this->model->evidence[iGen]->Length()))
				{
					if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
					{
						count++;
						idx = iPos1;
					}
					iPos1++;
				}

				// if the turtle can only be produced by a single turtle then eliminate all other possibilities
				if ((count == 1) && (this->model->alphabet->IsTurtle(this->model->evidence[iGen]->GetSymbolIndex(idx))))
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen + 1]->Length(); iPos3++)
					{
						if ((this->MAO->localization[iGen][iPos3, idx] == MasterAnalysisObject::cLocalization_StrongBTF) && (iPos2 != iPos3))
						{
							this->MAO->SetLocalization(iGen, iPos3, idx, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
						else if ((this->MAO->localization[iGen][iPos3, idx] == MasterAnalysisObject::cLocalization_Locked) && (iPos2 != iPos3))
						{
							Console::WriteLine("Uh oh!");
						}
					}
				}
			}

			// B1. A symbol cannot be localized to a position if the next position does not have a localization equal or greater than the current symbol
			Int32 largest = 0;
			Int32 smallest = 999999;
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if ((iPos2 < this->model->evidence[iGen + 1]->Length() - 1) && ((this->MAO->localization[iGen][iPos2 + 1, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2 + 1, iPos1] == MasterAnalysisObject::cLocalization_Locked)) && (iPos1 > largest))
				{
					largest = iPos1;
				}

				if ((iPos2 > 0) && ((this->MAO->localization[iGen][iPos2 - 1, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2 - 1, iPos1] == MasterAnalysisObject::cLocalization_Locked)) && (iPos1 < smallest))
				{
					smallest = iPos1;
				}
			}

			// Nothing larger than the largest in the next symbol can produce the current symbol
			if (iPos2 < this->model->evidence[iGen + 1]->Length() - 1)
			{
				for (size_t iPos1 = largest + 1; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF)
					{
						this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
						change = true;
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::WriteLine("Uh oh!");
					}
				}
			}

			// Nothing smaller than the smallest in the previous symbol can produce the current symbol
			if (iPos2 > 0)
			{
				for (size_t iPos1 = 0; iPos1 < Math::Max(0, smallest - 1); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF)
					{
						this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
						change = true;
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::WriteLine("Uh oh!");
					}
				}
			}

			// STEP 2 - these steps should convert finalize the localization settings, i.e. converting 1s to 2s or 1s to 0s
			// A1. If a symbol has any value is locked then remove the remainder
			// A2. If a symbol has only a single precedessor set it to 2
			Int32 count = 0;
			Int32 idx = 0;
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					for (size_t iPos3 = 0; iPos3 < this->model->evidence[iGen]->Length(); iPos3++)
					{
						if ((iPos1 != iPos3) && (this->MAO->localization[iGen][iPos2, iPos3] != MasterAnalysisObject::cLocalization_Never))
						{
							this->MAO->SetLocalization(iGen, iPos2, iPos3, MasterAnalysisObject::cLocalization_Never);
							change = true;
						}
					}
				}
				else if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked))
				{
					count++;
					idx = iPos1;
				}
			}

			if (count == 1)
			{
				this->MAO->SetLocalization(iGen, iPos2, idx, MasterAnalysisObject::cLocalization_Locked);
				change = true;
			}
		}
	} while (change);
}

MaxFragmentCandidates^ LSIProblemV3::ComputeSymbolLocalization_FindBTFCandidates_LeftToRight(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F)
{
	MaxFragmentCandidates^ result = gcnew MaxFragmentCandidates();
	List<Int32>^ candidates = gcnew List<Int32>;
	List<Int32>^ btfIndex = gcnew List<Int32>;

	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < this->model->evidence[iGen + 1]->Length())
		{
			for (size_t iBTF = 0; iBTF < F->cache->Count; iBTF++)
			{
				String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, F->cache[iBTF]->fragment->Length);
				if (subwordMax == F->cache[iBTF]->fragment)
				{
					if (candidates->Count == 0 || btfIndex[0] == iBTF)
					{
						candidates->Add(iScan);
						btfIndex->Add(iBTF);
					}
				}
			}
		}
	}

	result->candidates = candidates;
	result->fragmentIndex = btfIndex;

	return result;
}

MaxFragmentCandidates^ LSIProblemV3::ComputeSymbolLocalization_FindBTFCandidates_RightToLeft(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F)
{
	MaxFragmentCandidates^ result = gcnew MaxFragmentCandidates();
	List<Int32>^ candidates = gcnew List<Int32>;
	List<Int32>^ btfIndex = gcnew List<Int32>;

	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < this->model->evidence[iGen + 1]->Length())
		{
			for (size_t iBTF = 0; iBTF < F->cache->Count; iBTF++)
			{
				Int32 start = iScan - F->cache[iBTF]->fragment->Length + 1;
				if (start >= 0)
				{
					String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(start, F->cache[iBTF]->fragment->Length);
					if (subwordMax == F->cache[iBTF]->fragment)
					{
						if (candidates->Count == 0 || btfIndex[0] == iBTF)
						{
							candidates->Add(iScan);
							btfIndex->Add(iBTF);
						}
					}
				}
			}
		}
	}

	result->candidates = candidates;
	result->fragmentIndex = btfIndex;

	return result;
}

MaxFragmentCandidates^ LSIProblemV3::ComputeSymbolLocalization_FindMaxFragmentCandidates_LeftToRight(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F)
{
	MaxFragmentCandidates^ result = gcnew MaxFragmentCandidates();
	List<Int32>^ candidates = gcnew List<Int32>;
	List<Int32>^ fragmentIndex = gcnew List<Int32>;

	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < this->model->evidence[iGen + 1]->Length())
		{
			// Starting from iScan and scanning right, every right neighbour must be valid to at least one max fragments
			for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
			{
				String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, F->max[iFragment]->fragment->Length);
				if (subwordMax == F->max[iFragment]->fragment)
				{
					candidates->Add(iScan);
					fragmentIndex->Add(iFragment);
				}
			}
		}
	}

	result->candidates = candidates;
	result->fragmentIndex = fragmentIndex;

	return result;
}

MaxFragmentCandidates^ LSIProblemV3::ComputeSymbolLocalization_FindMaxFragmentCandidates_RightToLeft(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F)
{
	MaxFragmentCandidates^ result = gcnew MaxFragmentCandidates();
	List<Int32>^ candidates = gcnew List<Int32>;
	List<Int32>^ fragmentIndex = gcnew List<Int32>;

	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < this->model->evidence[iGen + 1]->Length())
		{
			// Starting from iScan and scanning right, every right neighbour must be valid to at least one max fragments
			for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
			{
				Int32 start = iScan - F->max[iFragment]->fragment->Length + 1;
				if (start >= 0)
				{
					String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(start, F->max[iFragment]->fragment->Length);
					if (subwordMax == F->max[iFragment]->fragment)
					{
						candidates->Add(iScan);
						fragmentIndex->Add(iFragment);
					}
				}
			}
		}
	}

	result->candidates = candidates;
	result->fragmentIndex = fragmentIndex;

	return result;
}

// LEFT TO RIGHT LOCALIZATION
void LSIProblemV3::ComputeSymbolLocalization_LeftToRightLocalization(Int32 iGen)
{
	Console::WriteLine("Left to Right Localization");
	Console::WriteLine("==========================");
	Int32 prevStart = 0;
	Int32 prevEnd = 0;
	bool prevIsTurtle = false;
	bool prevSingleCandidate = false;
	bool prevComplete = false;

	Console::WriteLine();

	for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
	{
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine(iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos) + " in wake from " + prevStart + " to " + prevEnd);
#endif
		Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);

		List<Int32>^ candidates = gcnew List<Int32>;

		if (this->model->alphabet->IsTurtle(symbolIndex))
		{
			prevIsTurtle = true;
			// All turtle symbols must be in the prev range or the right neighbour of the range
			// Look to find all the places where the symbol matches
			for (size_t iScan = prevStart; iScan <= prevEnd; iScan++)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length()) // if the previous symbols range already extends to the end then don't scan outside the word
				{
					if ((symbolIndex == this->model->evidence[iGen + 1]->GetSymbolIndex(iScan)) && (this->ValidateRightNeighbourTurtle(this->model->evidence[iGen], this->model->evidence[iGen + 1], iPos + 1, iScan + 1)))
					{
						candidates->Add(iScan);
					}
				}
			}

			if (candidates->Count == 1)
			{
				prevSingleCandidate = true;
				this->MAO->SetLocalization(iGen, candidates[0], iPos, MasterAnalysisObject::cLocalization_Locked);
			}
			else
			{
				prevSingleCandidate = false;
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					this->MAO->SetLocalization(iGen, candidates[iCandidate], iPos, MasterAnalysisObject::cLocalization_Weak);
				}
			}

			prevStart = candidates[0] + 1;
			prevEnd = candidates[candidates->Count - 1] + 1;

		}
		else
		{
			// if the prev symbol is a turtle, AND only had a single candidate
			// then this create a head fragment for the current symbol
			if ((prevIsTurtle) && (prevSingleCandidate))
			{
				prevSingleCandidate = true;
				this->ComputeFragment_Position_HeadOnly(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart);
			}

			// Set the localization for the head fragments
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			// Adjust the wake based on the WTW
			Range^ wtw = this->model->evidence[iGen]->GetWTW(iPos);
			if (prevStart < wtw->start)
			{
				prevStart = wtw->start;
			}
			if (prevEnd > wtw->end)
			{
				prevEnd = wtw->end;
			}

			List<Int32>^ candidatesBTF = gcnew List<Int32>;
			List<Int32>^ candidatesHead = gcnew List<Int32>;
			List<Int32>^ candidatesTail = gcnew List<Int32>;
			List<Int32>^ candidatesMid = gcnew List<Int32>;
			List<Int32>^ fragmentIndex = gcnew List<Int32>;
			List<Int32>^ btfIndex = gcnew List<Int32>;
			bool localizeHeadFragment = true;
			bool localizeTailFragment = true;
			bool localizeMaxFragment = true;
			bool localizeMidFragment = true;
			Int32 startWake = 99999;
			Int32 endWake = 0;
			Int32 startLocalizationLimit = 99999;
			Int32 endLocalizationLimit = 0;
			Int32 startWakeAbs = 99999;
			Int32 endWakeAbs = 0;
			prevIsTurtle = false;
			FragmentSigned^ btf;

			if (f->btf->Count == 1)
			{
				btf = f->btf[0];
				this->model->evidence[iGen]->SetBTF(iPos, f->btf[0]);
			}
			else
			{
				btf = this->model->evidence[iGen]->GetBTF(iPos);
			}

			// if the head fragment is complete only localize the head fragment
			if (f->head->min->isComplete)
			{
				localizeTailFragment = false;
				localizeMidFragment = false;
				localizeMaxFragment = false;
			}

			for (size_t iScan = prevStart; iScan <= prevEnd; iScan++)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length())
				{
					String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, btf->fragment->Length);
					if (subwordMax == btf->fragment && (iScan >= wtw->start && iScan <= wtw->end))
					{
						candidatesBTF->Add(iScan);
					}

					if (f->head->min->fragment != nullptr)
					{
						String^ subwordHeadMin = this->model->evidence[iGen + 1]->GetSymbols(iScan, f->head->min->fragment->Length);
						String^ subwordHeadMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, f->head->max->fragment->Length);
						if ((subwordHeadMin == f->head->min->fragment) && (subwordHeadMax == f->head->max->fragment))
						{
							candidatesHead->Add(iScan);
						}
					}

					if (f->tail->min->fragment != nullptr)
					{
						String^ subwordTailMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, f->tail->max->fragment->Length);
						String^ subwordTailMin = subwordTailMax->Substring(subwordTailMax->Length - f->tail->min->fragment->Length, f->tail->min->fragment->Length);
						if ((subwordTailMin == f->tail->min->fragment) && (subwordTailMax == f->tail->max->fragment))
						{
							candidatesTail->Add(iScan);
						}
					}

					if (f->mid->fragment != nullptr)
					{
						String^ subwordMid = this->model->evidence[iGen + 1]->GetSymbols(iScan, f->mid->fragment->Length);
						if (subwordMid == f->mid->fragment)
						{
							candidatesMid->Add(iScan);
						}
					}
				}
			}

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 2
			Console::Write("Head Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesHead->Count; iCandidate++)
			{
				Console::Write(candidatesHead[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Mid Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesMid->Count; iCandidate++)
			{
				Console::Write(candidatesMid[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Tail Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesTail->Count; iCandidate++)
			{
				Console::Write(candidatesTail[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Max Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
			{
				Console::Write(candidates[iCandidate] + " ");
			}
			Console::WriteLine();
#endif
			// If not BTF fragment could be placed in the wake, then create a new BTF and localize it
			if (candidatesBTF->Count == 0)
			{
				Fragment^ toCache;
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " cannot be localized in wake " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(prevStart, prevEnd));

				// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				toCache = this->ComputeBTF_LocalizationCompatability_LeftToRight(btf, this->model->evidence[iGen + 1], prevStart, prevEnd);

				//// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				//// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				//Int32 iBTF = 0;
				//bool btfFound = false;
				//do
				//{
				//	btf = this->ComputeBTF_LocalizationCompatability_LeftToRight(f->btf[iBTF], this->model->evidence[iGen + 1], prevStart, prevEnd);
				//	if (btf->fragment != nullptr)
				//	{
				//		btfFound = true;
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + btf->fragment);
				//	}
				//	else
				//	{
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + f->btf[iBTF]->fragment + " is not locally compatible.");
				//	}
				//	iBTF++;
				//} while (!btfFound && iBTF < f->btf->Count);

				////// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
				////if (btf->fragment == nullptr && candidates->Count > 0)
				////{
				////	Int32 btfStart = prevStart;
				////	Int32 btfEnd = 0;
				////	for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				////	{
				////		if (btfEnd < candidates[iCandidate] + f->max[fragmentIndex[iCandidate]]->fragment->Length)
				////		{
				////			btfEnd = Math::Min(this->model->evidence[iGen + 1]->Length()-1, candidates[iCandidate] + f->max[fragmentIndex[iCandidate]]->fragment->Length);
				////		}
				////	}
				////	btf = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(btfStart, btfEnd), false, true);
				////	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found from max fragments BTF " + btf->fragment);
				////}

				//// C. Create a BTF using the reserve algorithm
				//if (btf->fragment == nullptr)
				//{
				//	btf = this->ComputeFragment_Position_BTFOnly_LeftToRight(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart, prevEnd);
				//	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by position reserve BTF " + btf->fragment);
				//}

				f->cache->Add(this->MAO->CreateBTF(toCache, btf->leftSignature, btf->rightSignature));
				this->MAO->SetFact_BTFOnly_Update(btf, f->cache[f->cache->Count - 1], f);
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + toCache->fragment);
				candidatesBTF = this->ComputeSymbolLocalization_MaxFragment_LeftToRight(btf, this->model->evidence[iGen + 1], prevStart, prevEnd);
			}

			for (size_t iCandidate = 0; iCandidate < candidatesBTF->Count; iCandidate++)
			{
				Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " @ " + candidatesBTF[iCandidate] + ".." + (candidatesBTF[iCandidate] + btf->fragment->Length - 1));
				Console::WriteLine("Finding superstring candidates");
				for (size_t iScan = prevStart; iScan <= prevEnd; iScan++)
				{
					if (iScan < this->model->evidence[iGen + 1]->Length())
					{
						for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
						{
							// Signature must match BTF or be blank
							bool signatureMatch = btf->leftSignature == f->max[iFragment]->leftSignature && btf->rightSignature == f->max[iFragment]->rightSignature;

							if (((f->max[iFragment]->leftSignature == "" && f->max[iFragment]->rightSignature == "")) || signatureMatch)
							{
								String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(iScan, f->max[iFragment]->fragment->Length);
								if (subwordMax == f->max[iFragment]->fragment)
								{
									candidates->Add(iScan);
									fragmentIndex->Add(iFragment);
								}
							}
						}
					}
				}

				if (candidatesBTF[iCandidate] + f->length->min < startWakeAbs)
				{
					startWakeAbs = candidatesBTF[iCandidate] + f->length->min;
				}
				if (candidatesBTF[iCandidate] < startLocalizationLimit)
				{
					startLocalizationLimit = candidatesBTF[iCandidate];
				}

				if (candidatesBTF[iCandidate] + btf->fragment->Length > endWakeAbs)
				{
					endWakeAbs = candidatesBTF[iCandidate] + btf->fragment->Length;
				}
				if (candidatesBTF[iCandidate] + btf->fragment->Length - 1 > endLocalizationLimit)
				{
					endLocalizationLimit = candidatesBTF[iCandidate] + btf->fragment->Length - 1;
				}

				for (size_t iLength = 0; iLength < btf->fragment->Length; iLength++)
				{
					this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_WeakBTF);
					// if there are no max fragments localilzed then upgrade to weak localization
					if (candidates->Count == 0)
					{
						this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
					}
				}
			}

			startLocalizationLimit = Math::Max(prevStart, startLocalizationLimit);
			endLocalizationLimit = Math::Min(this->model->evidence[iGen + 1]->Length() - 1, Math::Min(prevEnd + f->length->max - 1, endLocalizationLimit));

			Console::WriteLine("Absolute localization limit is " + startLocalizationLimit + " / " + endLocalizationLimit);

			//// If no fragment could be placed in the wake, then
			//// 1. tabu all of the current max fragments
			//// 2. construct a new max fragment from the wake and add it as a candidate
			//if ((candidates->Count == 0) && (candidatesHead->Count == 0) && (candidatesTail->Count == 0))
			//{
			//	List<Fragment^>^ toRemove = gcnew List<Fragment^>;
			//	for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
			//	{
			//		toRemove->Add(f->max[iFragment]);
			//	}

			//	for (size_t iFragment = 0; iFragment < toRemove->Count; iFragment++)
			//	{
			//		this->MAO->RemoveFact_MaxFragmentOnly(f, toRemove[iFragment], true);
			//	}

			//	Range^ r = this->ComputeFragment_Wake_LeftToRight(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart, prevEnd);
			//	MaxFragmentCandidates^ x = this->ComputeSymbolLocalization_FindMaxFragmentCandidates_LeftToRight(iGen, prevStart, prevEnd, f);
			//	candidates = x->candidates;
			//	fragmentIndex = x->fragmentIndex;
			//}

			if ((localizeHeadFragment) && (candidatesHead->Count > 0))
			{
				localizeMaxFragment = false; // if the head fragment gets set then there is no need to localize the max fragment
				for (size_t iCandidate = 0; iCandidate < candidatesHead->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Head Fragment " + f->head->min->fragment + " / " + f->head->max->fragment + " @ " + candidatesHead[iCandidate]);

					if (candidatesHead[iCandidate] + f->length->min < startWake)
					{
						startWake = candidatesHead[iCandidate] + f->length->min;
					}
					if (candidatesHead[iCandidate] + f->head->max->fragment->Length > endWake)
					{
						endWake = candidatesHead[iCandidate] + f->head->max->fragment->Length;
					}

					for (size_t iLength = 0; iLength < f->head->max->fragment->Length; iLength++)
					{
						if (candidatesHead[iCandidate] + iLength >= startLocalizationLimit && candidatesHead[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
					}
					for (size_t iLength = 0; iLength < f->head->min->fragment->Length; iLength++)
					{
						if (candidatesHead->Count == 1 && candidatesHead[iCandidate] + iLength >= startLocalizationLimit && candidatesHead[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Locked);
						}
						else if (candidatesHead[iCandidate] + iLength >= startLocalizationLimit && candidatesHead[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
					}
				}
			}

			// Set the localization for the tail fragments
			if ((localizeTailFragment) && (candidatesTail->Count > 0))
			{
				localizeMaxFragment = false; // if the head fragment gets set then there is no need to localize the max fragment
				for (size_t iCandidate = 0; iCandidate < candidatesTail->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Tail Fragment " + f->tail->min->fragment + " / " + f->tail->max->fragment + " @ " + candidatesTail[iCandidate]);

					if (candidatesTail[iCandidate] + f->length->min < startWake)
					{
						startWake = candidatesTail[iCandidate] + f->length->min;
					}
					if (candidatesTail[iCandidate] + f->tail->max->fragment->Length > endWake)
					{
						endWake = candidatesTail[iCandidate] + f->tail->max->fragment->Length;
					}

					for (size_t iLength = 0; iLength < f->tail->max->fragment->Length; iLength++)
					{
						if (candidatesTail[iCandidate] + iLength >= startLocalizationLimit && candidatesTail[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
					}

					for (size_t iLength = 1; iLength <= f->tail->min->fragment->Length; iLength++)
					{
						if (candidatesTail->Count > 1 && candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength >= startLocalizationLimit && candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
						else if (candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength >= startLocalizationLimit && candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] + f->tail->max->fragment->Length - iLength, iPos, MasterAnalysisObject::cLocalization_Locked);
						}
					}
				}
			}

			// Set the localization for the max fragments
			if (localizeMaxFragment)
			{
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Max Fragment " + f->max[fragmentIndex[iCandidate]]->fragment + " @ " + candidates[iCandidate] + ".." + (candidates[iCandidate] + f->max[fragmentIndex[iCandidate]]->fragment->Length - 1));
					if (candidates[iCandidate] + f->length->min < startWake)
					{
						startWake = candidates[iCandidate] + f->length->min;
					}
					if (candidates[iCandidate] + f->max[fragmentIndex[iCandidate]]->fragment->Length > endWake)
					{
						endWake = candidates[iCandidate] + f->max[fragmentIndex[iCandidate]]->fragment->Length;
					}

					for (size_t iLength = 0; iLength < f->max[fragmentIndex[iCandidate]]->fragment->Length; iLength++)
					{
						if (candidates[iCandidate] + iLength >= startLocalizationLimit && candidates[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidates[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
					}
				}
			}

			// Set the localization for the mid fragments
			if ((localizeMidFragment) && (candidatesMid->Count > 0))
			{
				for (size_t iCandidate = 0; iCandidate < candidatesMid->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Mid Fragments " + f->mid->fragment + " @ " + candidatesMid[iCandidate]);
					for (size_t iLength = 0; iLength < f->mid->fragment->Length; iLength++)
					{
						if (candidatesMid->Count > 1 && candidatesMid[iCandidate] + iLength >= startLocalizationLimit && candidatesMid[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesMid[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Weak);
						}
						else if (candidatesMid[iCandidate] + iLength >= startLocalizationLimit && candidatesMid[iCandidate] + iLength <= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesMid[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_Locked);
						}
					}
				}
			}

//			// Any max fragment that could not be placed in the wake is tabu
//			List<Fragment^>^ toRemove = gcnew List<Fragment^>;
//			for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
//			{
//				if (!fragmentIndex->Contains(iFragment))
//				{
//#if _PHASE3_COMPUTE_LOCALIZATION_ >= 2
//					Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " max fragment " + f->max[iFragment]->fragment + " is tabu");
//#endif
//					toRemove->Add(f->max[iFragment]);
//					// remove any max fragment that could not be placed in the wake as false
//				}
//			}
//			this->MAO->RemoveFact_MaxFragmentOnly(f, toRemove, true);

			if ((startWakeAbs > startWake) || (startWake == 99999))
			{
				startWake = startWakeAbs;
			}

			if (startLocalizationLimit > startWake) 
			{
				startWake = startLocalizationLimit;
			}

			if ((endWakeAbs < endWake) || (endWake == 0))
			{
				endWake = endWakeAbs;
			}

			if (endLocalizationLimit+1 < endWake)
			{
				endWake = endLocalizationLimit+1;
			}

			prevStart = startWake;
			prevEnd = endWake;
		}
	}
}

// LEFT TO RIGHT LOCALIZATION
void LSIProblemV3::ComputeSymbolLocalization_LeftToRightLocalization_BTFOnly(Int32 iGen)
{
	Console::WriteLine("Left to Right Localization BTF Only");
	Console::WriteLine("===================================");
	Int32 prevStart = 0;
	Int32 prevEnd = 0;
	bool prevIsTurtle = false;
	bool prevSingleCandidate = false;
	bool prevComplete = false;

	Console::WriteLine();

	for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
	{
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine(iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos) + " in wake from " + prevStart + " to " + prevEnd);
#endif
		Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);

		List<Int32>^ candidates = gcnew List<Int32>;

		if (this->model->alphabet->IsTurtle(symbolIndex))
		{
			prevIsTurtle = true;
			// All turtle symbols must be in the prev range or the right neighbour of the range
			// Look to find all the places where the symbol matches
			for (size_t iScan = prevStart; iScan <= prevEnd; iScan++)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length()) // if the previous symbols range already extends to the end then don't scan outside the word
				{
					if ((symbolIndex == this->model->evidence[iGen + 1]->GetSymbolIndex(iScan)) && (this->ValidateRightNeighbourTurtle(this->model->evidence[iGen], this->model->evidence[iGen + 1], iPos + 1, iScan + 1)))
					{
						candidates->Add(iScan);
					}
				}
			}

			if (candidates->Count == 1)
			{
				prevSingleCandidate = true;
				this->MAO->SetLocalization(iGen, candidates[0], iPos, MasterAnalysisObject::cLocalization_Locked);
			}
			else
			{
				prevSingleCandidate = false;
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					this->MAO->SetLocalization(iGen, candidates[iCandidate], iPos, MasterAnalysisObject::cLocalization_Weak);
				}
			}

			prevStart = candidates[0] + 1;
			prevEnd = candidates[candidates->Count - 1] + 1;

		}
		else
		{
			// Set the localization for the head fragments
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			// Adjust the wake based on the WTW
			Range^ wtw = this->model->evidence[iGen]->GetWTW(iPos);
			if (prevStart < wtw->start)
			{
				prevStart = wtw->start;
			}
			if (prevEnd > wtw->end)
			{
				prevEnd = wtw->end;
			}

			List<Int32>^ candidatesBTF = gcnew List<Int32>;
			Int32 startWake = 99999;
			Int32 endWake = 0;
			Int32 startLocalizationLimit = 99999;
			Int32 endLocalizationLimit = 0;
			prevIsTurtle = false;
			FragmentSigned^ btf;

			if (f->btf->Count == 1)
			{
				btf = f->btf[0];
				this->model->evidence[iGen]->SetBTF(iPos, f->btf[0]);
			}
			else
			{
				btf = this->model->evidence[iGen]->GetBTF(iPos);
			}

			for (size_t iScan = prevStart; iScan <= prevEnd; iScan++)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length())
				{
					String^ subword = this->model->evidence[iGen + 1]->GetSymbols(iScan, btf->fragment->Length);
					if (subword == btf->fragment && (iScan >= wtw->start && iScan <= wtw->end))
					{
						candidatesBTF->Add(iScan);
					}
				}
			}

			// If not BTF fragment could be placed in the wake, then create a new BTF and localize it
			if (candidatesBTF->Count == 0)
			{
				Fragment^ toCache;
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " cannot be localized in wake " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(prevStart, prevEnd));

				// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				Fragment^ tmp1 = this->ComputeBTF_LocalizationCompatability_LeftToRight(btf, this->model->evidence[iGen + 1], prevStart, prevEnd);
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + tmp1->fragment);

				// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
				Int32 btfStart = prevStart;
				Int32 btfEnd = prevEnd + f->length->max;
				Fragment^ tmp2 = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(btfStart, btfEnd), false, true);
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by wake + max length BTF " + tmp2->fragment);

				//// C. Create a BTF using the reserve algorithm
				//Fragment^ tmp3 = this->ComputeFragment_Position_BTFOnly_LeftToRight(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart, prevEnd);
				//Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by position reserve BTF " + btf->fragment);

				if (tmp1->fragment == nullptr || tmp2->fragment->Length < tmp1->fragment->Length)
				{
					toCache = tmp2;
				}
				else
				{
					toCache = tmp1;
				}

				f->cache->Add(this->MAO->CreateBTF(toCache, btf->leftSignature, btf->rightSignature));
				this->MAO->SetFact_BTFOnly_Update(btf, f->cache[f->cache->Count - 1], f);
				candidatesBTF = this->ComputeSymbolLocalization_MaxFragment_LeftToRight(btf, this->model->evidence[iGen + 1], prevStart, prevEnd);
			}

			for (size_t iCandidate = 0; iCandidate < candidatesBTF->Count; iCandidate++)
			{
				Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " @ " + candidatesBTF[iCandidate] + ".." + (candidatesBTF[iCandidate] + btf->fragment->Length - 1));

				if (candidatesBTF[iCandidate] + f->length->min < startWake)
				{
					startWake = candidatesBTF[iCandidate] + f->length->min;
				}
				if (candidatesBTF[iCandidate] < startLocalizationLimit)
				{
					startLocalizationLimit = candidatesBTF[iCandidate];
				}

				if (candidatesBTF[iCandidate] + btf->fragment->Length > endWake)
				{
					endWake = candidatesBTF[iCandidate] + btf->fragment->Length;
				}
				if (candidatesBTF[iCandidate] + btf->fragment->Length - 1 > endLocalizationLimit)
				{
					endLocalizationLimit = candidatesBTF[iCandidate] + btf->fragment->Length - 1;
				}

				for (size_t iLength = 0; iLength < btf->fragment->Length; iLength++)
				{
					this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] + iLength, iPos, MasterAnalysisObject::cLocalization_WeakBTF);
				}
			}

			startLocalizationLimit = Math::Max(prevStart, startLocalizationLimit);
			endLocalizationLimit = Math::Min(this->model->evidence[iGen + 1]->Length() - 1, Math::Min(prevEnd + f->length->max - 1, endLocalizationLimit));

			Console::WriteLine("Absolute localization limit is " + startLocalizationLimit + " / " + endLocalizationLimit);

			if (startLocalizationLimit > startWake)
			{
				startWake = startLocalizationLimit;
			}

			if (endLocalizationLimit + 1 < endWake)
			{
				endWake = endLocalizationLimit + 1;
			}

			prevStart = startWake;
			prevEnd = endWake;

			if (iGen >= 999)
			{
				Console::ReadLine();
			}
		}
	}
}

void LSIProblemV3::ComputeSymbolLocalization_RightToLeftLocalization(Int32 iGen)
{
	Console::WriteLine("Right to Left Localization");
	Console::WriteLine("==========================");
	Int32 prevStart = this->model->evidence[iGen + 1]->Length() - 1;
	Int32 prevEnd = this->model->evidence[iGen + 1]->Length() - 1;
	bool prevIsTurtle = false;
	bool prevSingleCandidate = false;
	bool prevComplete = false;

	Console::WriteLine();
	//this->MAO->localization[iGen] = gcnew array<Byte, 2>(this->model->evidence[iGen + 1]->Length(), this->model->evidence[iGen]->Length());

	for (Int16 iPos = this->model->evidence[iGen]->Length()-1; iPos >= 0; --iPos)
	{
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine(iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos) + " in wake from " + prevStart + " to " + prevEnd);
#endif
		Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);

		List<Int32>^ candidates = gcnew List<Int32>;

		if (this->model->alphabet->IsTurtle(symbolIndex))
		{
			prevIsTurtle = true;
			// All turtle symbols must be in the prev range or the right neighbour of the range
			// Look to find all the places where the symbol matches
			for (Int16 iScan = prevStart; iScan >= prevEnd; --iScan)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length()) // if the previous symbols range already extends to the end then don't scan outside the word
				{
					if ((symbolIndex == this->model->evidence[iGen + 1]->GetSymbolIndex(iScan)) && (this->ValidateLeftNeighbourTurtle(this->model->evidence[iGen], this->model->evidence[iGen + 1], iPos - 1, iScan - 1)))
					{
						candidates->Add(iScan);
					}
				}
			}

			if (candidates->Count == 1)
			{
				prevSingleCandidate = true;
				this->MAO->SetLocalization(iGen, candidates[0], iPos, MasterAnalysisObject::cLocalization_Locked);
				//this->MAO->localization[iGen][candidates[0], iPos] = cLocalization_Locked;
			}
			else
			{
				prevSingleCandidate = false;
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					// Upgrade the setting from weak to strong
					this->MAO->SetLocalization(iGen, candidates[iCandidate], iPos, MasterAnalysisObject::cLocalization_Strong);
					//if (this->MAO->localization[iGen][candidates[iCandidate], iPos] == cLocalization_Weak)
					//{
					//	this->MAO->localization[iGen][candidates[iCandidate], iPos] = cLocalization_Strong;
					//}
				}
			}

			prevStart = candidates[0] - 1;
			prevEnd = candidates[candidates->Count - 1] - 1;

		}
		else
		{
			// if the prev symbol is a turtle, AND only had a single candidate
			// then this create a head fragment for the current symbol
			if ((prevIsTurtle) && (prevSingleCandidate))
			{
				prevSingleCandidate = true;
				this->ComputeFragment_Position_TailOnly(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart);
			}

			// Set the localization for the head fragments
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			// Adjust the wake based on the WTW
			Range^ wtw = this->model->evidence[iGen]->GetWTW(iPos);
			if (prevStart < wtw->start)
			{
				prevStart = wtw->start;
			}
			if (prevEnd > wtw->end)
			{
				prevEnd = wtw->end;
			}


			List<Int32>^ candidatesBTF = gcnew List<Int32>;
			List<Int32>^ candidatesHead = gcnew List<Int32>;
			List<Int32>^ candidatesTail = gcnew List<Int32>;
			List<Int32>^ candidatesMid = gcnew List<Int32>;
			List<Int32>^ fragmentIndex = gcnew List<Int32>;
			List<Int32>^ btfIndex = gcnew List<Int32>;
			bool localizeHeadFragment = true;
			bool localizeTailFragment = true;
			bool localizeMaxFragment = true;
			bool localizeMidFragment = true;
			Int32 startWake = 0;
			Int32 endWake = 99999;
			Int32 startLocalizationLimit = 0;
			Int32 endLocalizationLimit = 99999;
			Int32 startWakeAbs = 0;
			Int32 endWakeAbs = 99999;
			prevIsTurtle = false;
			FragmentSigned^ btf;

			if (f->btf->Count == 1)
			{
				btf = f->btf[0];
				this->model->evidence[iGen]->SetBTF(iPos, f->btf[0]);
			}
			else
			{
				btf = this->model->evidence[iGen]->GetBTF(iPos);
			}

			// if the head fragment is complete only localize the head fragment
			if (f->head->min->isComplete)
			{
				localizeTailFragment = false;
				localizeMidFragment = false;
				localizeMaxFragment = false;
			}

			for (Int16 iScan = prevStart; iScan >= prevEnd; --iScan)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length())
				{
					Int32 start = iScan - btf->fragment->Length + 1;
					Int32 end = start + btf->fragment->Length;
					if (start >= 0)
					{
						String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(start, btf->fragment->Length);
						if (subwordMax == btf->fragment && (iScan <= wtw->end && iScan >= wtw->start))
						{
							candidatesBTF->Add(iScan);
						}
					}

					if (f->head->min->fragment != nullptr)
					{
						Int32 start = iScan - f->head->max->fragment->Length + 1;
						if (start >= 0)
						{
							String^ subwordHeadMin = this->model->evidence[iGen + 1]->GetSymbols(start, f->head->min->fragment->Length);
							String^ subwordHeadMax = this->model->evidence[iGen + 1]->GetSymbols(start, f->head->max->fragment->Length);
							if ((subwordHeadMin == f->head->min->fragment) && (subwordHeadMax == f->head->max->fragment))
							{
								candidatesHead->Add(iScan);
							}
						}
					}

					if (f->tail->min->fragment != nullptr)
					{
						Int32 start = iScan - f->tail->max->fragment->Length + 1;
						if (start >= 0)
						{
							String^ subwordTailMin = this->model->evidence[iGen + 1]->GetSymbols(iScan - f->tail->min->fragment->Length + 1, f->tail->min->fragment->Length);
							String^ subwordTailMax = this->model->evidence[iGen + 1]->GetSymbols(start, f->tail->max->fragment->Length);
							if ((subwordTailMin == f->tail->min->fragment) && (subwordTailMax == f->tail->max->fragment))
							{
								candidatesTail->Add(iScan);
							}
						}
					}

					if (f->mid->fragment != nullptr)
					{
						Int32 start = iScan - f->mid->fragment->Length + 1;
						if (start >= 0)
						{
							String^ subwordMid = this->model->evidence[iGen + 1]->GetSymbols(start, f->mid->fragment->Length);
							if (subwordMid == f->mid->fragment)
							{
								candidatesMid->Add(iScan);
							}
						}
					}
				}
			}

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 2
			Console::Write("Head Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesHead->Count; iCandidate++)
			{
				Console::Write(candidatesHead[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Mid Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesMid->Count; iCandidate++)
			{
				Console::Write(candidatesMid[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Tail Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidatesTail->Count; iCandidate++)
			{
				Console::Write(candidatesTail[iCandidate] + " ");
			}
			Console::WriteLine();

			Console::Write("Max Fragment:");
			for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
			{
				Console::Write(candidates[iCandidate] + " ");
			}
			Console::WriteLine();
#endif
			// If no BTF could be placed in the wake, then create a new BTF and localize it
			// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
			// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
			// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
			// C. Create a BTF using the reserve algorithm
			if (candidatesBTF->Count == 0)
			{
				Fragment^ toCache;
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " cannot be localized in wake " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(prevEnd, prevStart));

				// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				toCache = this->ComputeBTF_LocalizationCompatability_RightToLeft(btf, this->model->evidence[iGen + 1], prevEnd, prevStart);

				//// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				//// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				//Int32 iBTF = 0;
				//bool btfFound = false;
				//do
				//{
				//	btf = this->ComputeBTF_LocalizationCompatability_RightToLeft(f->btf[iBTF], this->model->evidence[iGen + 1], prevEnd, prevStart);
				//	if (btf->fragment != nullptr)
				//	{
				//		btfFound = true;
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + btf->fragment);
				//	}
				//	else
				//	{
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + f->btf[iBTF]->fragment + " is not locally compatible.");
				//	}
				//	iBTF++;
				//} while (!btfFound && iBTF < f->btf->Count);

				//// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
				//if (btf->fragment == nullptr && candidates->Count > 0)
				//{
				//	Int32 btfStart = 99999;
				//	Int32 btfEnd = prevStart;
				//	for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				//	{
				//		if (btfStart > candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length)
				//		{
				//			btfStart = Math::Max(0, candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length);
				//		}
				//	}
				//	btf = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(btfStart, btfEnd),false,true);
				//	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found from max fragments BTF " + btf->fragment);
				//}

				// C. Create a BTF using the reserve algorithm
				//if (toCache->fragment == nullptr)
				//{
				//	toCache = this->ComputeFragment_Position_BTFOnly_RightToLeft(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevEnd,prevStart);
				//	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by position reserve BTF " + toCache->fragment);
				//}

				f->cache->Add(this->MAO->CreateBTF(toCache, btf->leftSignature, btf->rightSignature));
				btf->fragment = toCache->fragment;
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + btf->fragment);
				candidatesBTF = this->ComputeSymbolLocalization_MaxFragment_RightToLeft(btf, this->model->evidence[iGen + 1], prevEnd, prevStart);
			}

			for (size_t iCandidate = 0; iCandidate < candidatesBTF->Count; iCandidate++)
			{
				Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " @ " + candidatesBTF[iCandidate] + ".." + (1 + candidatesBTF[iCandidate] - btf->fragment->Length));
				Console::WriteLine("Finding superstring localizations");

				// Starting from iScan and scanning right, every right neighbour must be valid to at least one max fragments
				for (Int16 iScan = prevStart; iScan >= prevEnd; --iScan)
				{
					if (iScan < this->model->evidence[iGen + 1]->Length())
					{
						for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
						{
							Int32 start = iScan - f->max[iFragment]->fragment->Length + 1;
							bool signatureMatch = btf->leftSignature == f->max[iFragment]->leftSignature && btf->rightSignature == f->max[iFragment]->rightSignature;

							if ((start >= 0) && ((f->max[iFragment]->leftSignature == "" && f->max[iFragment]->rightSignature == "") || signatureMatch))
							{
								String^ subwordMax = this->model->evidence[iGen + 1]->GetSymbols(start, f->max[iFragment]->fragment->Length);
								if (subwordMax == f->max[iFragment]->fragment)
								{
									candidates->Add(iScan);
									fragmentIndex->Add(iFragment);
								}
							}
						}
					}
				}


				if (candidatesBTF[iCandidate] - f->length->min > startWakeAbs)
				{
					startWakeAbs = candidatesBTF[iCandidate] - f->length->min;
				}
				if (candidatesBTF[iCandidate] > startLocalizationLimit)
				{
					startLocalizationLimit = candidatesBTF[iCandidate];
				}

				if (candidatesBTF[iCandidate] - btf->fragment->Length < endWakeAbs)
				{
					endWakeAbs = candidatesBTF[iCandidate] - btf->fragment->Length;
				}
				if (candidatesBTF[iCandidate] - btf->fragment->Length + 1 < endLocalizationLimit)
				{
					endLocalizationLimit = candidatesBTF[iCandidate] - btf->fragment->Length + 1;
				}

				for (size_t iLength = 0; iLength < btf->fragment->Length; iLength++)
				{
					this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_StrongBTF);
					if (candidates->Count == 0)
					{
						this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
					}
				}
			}

			startLocalizationLimit = Math::Min(prevStart, startLocalizationLimit);
			endLocalizationLimit = Math::Max(0, Math::Max(prevEnd - f->length->max + 1, endLocalizationLimit));

			Console::WriteLine("Absolute localization limit is " + startLocalizationLimit + " / " + endLocalizationLimit);

			//// If no fragment could be placed in the wake, then
			//// 1. tabu all of the current max fragments
			//// 2. construct a new max fragment from the wake and add it as a candidate
			//if ((candidates->Count == 0) && (candidatesHead->Count == 0) && (candidatesTail->Count == 0))
			//{
			//	List<Fragment^>^ toRemove = gcnew List<Fragment^>;
			//	for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
			//	{
			//		toRemove->Add(f->max[iFragment]);
			//	}

			//	for (size_t iFragment = 0; iFragment < toRemove->Count; iFragment++)
			//	{
			//		this->MAO->RemoveFact_MaxFragmentOnly(f, toRemove[iFragment], true);
			//	}

			//	Range^ r = this->ComputeFragment_Wake_RightToLeft(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart, prevEnd);
			//	MaxFragmentCandidates^ x = this->ComputeSymbolLocalization_FindMaxFragmentCandidates_RightToLeft(iGen, prevEnd, prevStart, f);
			//	candidates = x->candidates;
			//	fragmentIndex = x->fragmentIndex;
			//}

			if ((localizeHeadFragment) && (candidatesHead->Count > 0))
			{
				localizeMaxFragment = false; // if the head fragment gets set then there is no need to localize the max fragment
				for (size_t iCandidate = 0; iCandidate < candidatesHead->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Head Fragment " + f->head->min->fragment + " / " + f->head->max->fragment + " @ " + candidatesHead[iCandidate]);

					if (candidatesHead[iCandidate] - f->length->min > startWake)
					{
						startWake = Math::Max(0,candidatesHead[iCandidate] - f->length->min);
					}
					if (candidatesHead[iCandidate] - f->head->max->fragment->Length < endWake)
					{
						endWake = Math::Max(0, candidatesHead[iCandidate] - f->head->max->fragment->Length); // no +1 because we want to include the NEXT symbol
					}

					for (size_t iLength = 0; iLength < f->head->max->fragment->Length; iLength++)
					{
						if (candidatesHead[iCandidate] - iLength <= startLocalizationLimit && candidatesHead[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}

					for (size_t iLength = 1; iLength <= f->head->min->fragment->Length; iLength++)
					{
						if (candidatesHead->Count == 1 && candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength <= startLocalizationLimit && candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength, iPos, MasterAnalysisObject::cLocalization_Locked);
						}
						else if (candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength <= startLocalizationLimit && candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesHead[iCandidate] - f->head->max->fragment->Length + iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}
				}
			}

			// Set the localization for the tail fragments
			if ((localizeTailFragment) && (candidatesTail->Count > 0))
			{
				localizeMaxFragment = false; // if the head fragment gets set then there is no need to localize the max fragment
				for (size_t iCandidate = 0; iCandidate < candidatesTail->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Tail Fragment " + f->tail->min->fragment + " / " + f->tail->max->fragment + " @ " + candidatesTail[iCandidate]);

					if (candidatesTail[iCandidate] - f->length->min > startWake)
					{
						startWake = Math::Max(0, candidatesTail[iCandidate] - f->length->min);
					}
					if (candidatesTail[iCandidate] - f->tail->max->fragment->Length < endWake)
					{
						endWake = Math::Max(0, candidatesTail[iCandidate] - f->tail->max->fragment->Length); // no +1 because we want to include the NEXT symbol
					}

					for (size_t iLength = 0; iLength < f->tail->max->fragment->Length; iLength++)
					{
						if (candidatesTail[iCandidate] - iLength <= startLocalizationLimit && candidatesTail[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}

					for (size_t iLength = 0; iLength < f->tail->min->fragment->Length; iLength++)
					{
						if (candidatesTail->Count == 1 && candidatesTail[iCandidate] - iLength <= startLocalizationLimit && candidatesTail[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Locked);
						}
						else if (candidatesTail[iCandidate] - iLength <= startLocalizationLimit && candidatesTail[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesTail[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}
				}
			}

			// Set the localization for the max fragments
			if (localizeMaxFragment)
			{
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					if (candidates[iCandidate] - f->length->min > startWake)
					{
						startWake = Math::Max(0, candidates[iCandidate] - f->length->min);
					}
					if (candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length < endWake)
					{
						endWake = Math::Max(0, candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length); // no +1 as we want to include the NEXT symbol
					}
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Max Fragment " + f->max[fragmentIndex[iCandidate]]->fragment + " @ " + candidates[iCandidate] + ".." + (endWake+1));

					for (size_t iLength = 0; iLength < f->max[fragmentIndex[iCandidate]]->fragment->Length; iLength++)
					{
						if (candidates[iCandidate] - iLength <= startLocalizationLimit && candidates[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidates[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}
				}
			}

			// Set the localization for the mid fragments
			if ((localizeMidFragment) && (candidatesMid->Count > 0))
			{
				for (size_t iCandidate = 0; iCandidate < candidatesMid->Count; iCandidate++)
				{
					Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " Mid Fragments " + f->mid->fragment + " @ " + candidatesMid[iCandidate]);
					for (size_t iLength = 0; iLength < f->mid->fragment->Length; iLength++)
					{
						if (candidatesMid->Count > 1 && candidatesMid[iCandidate] - iLength <= startLocalizationLimit && candidatesMid[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesMid[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
						else if (candidatesMid->Count == 1 && candidatesMid[iCandidate] - iLength <= startLocalizationLimit && candidatesMid[iCandidate] - iLength >= endLocalizationLimit)
						{
							this->MAO->SetLocalization(iGen, candidatesMid[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_Strong);
						}
					}
				}
			}

//			// Any max fragment that could not be placed in the wake is tabu
//			List<Fragment^>^ toRemove = gcnew List<Fragment^>;
//			for (size_t iFragment = 0; iFragment < f->max->Count; iFragment++)
//			{
//				// remove any max fragment that could not be placed in the wake as false
//				if (!fragmentIndex->Contains(iFragment))
//				{
//#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
//					Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " max fragment " + f->max[iFragment]->fragment + " is tabu");
//#endif
//					toRemove->Add(f->max[iFragment]);
//				}
//				Console::Write("");
//			}
//			this->MAO->RemoveFact_MaxFragmentOnly(f, toRemove, true);

			if ((startWakeAbs < startWake) || (startWake == 0))
			{
				startWake = startWakeAbs;
			}

			if (startLocalizationLimit < startWake)
			{
				startWake = startLocalizationLimit;
			}

			if ((endWakeAbs > endWake) || (endWake == 99999))
			{
				endWake = endWakeAbs;
			}

			if (endLocalizationLimit > endWake)
			{
				endWake = endLocalizationLimit;
			}

			prevStart = startWake;
			prevEnd = endWake;
		}
	}
}

void LSIProblemV3::ComputeSymbolLocalization_RightToLeftLocalization_BTFOnly(Int32 iGen)
{
	Console::WriteLine("Right to Left Localization BTF Only");
	Console::WriteLine("===================================");
	Int32 prevStart = this->model->evidence[iGen + 1]->Length() - 1;
	Int32 prevEnd = this->model->evidence[iGen + 1]->Length() - 1;
	bool prevIsTurtle = false;
	bool prevSingleCandidate = false;
	bool prevComplete = false;

	Console::WriteLine();
	//this->MAO->localization[iGen] = gcnew array<Byte, 2>(this->model->evidence[iGen + 1]->Length(), this->model->evidence[iGen]->Length());

	for (Int16 iPos = this->model->evidence[iGen]->Length() - 1; iPos >= 0; --iPos)
	{
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine(iPos + ": " + this->model->evidence[iGen]->GetSymbol(iPos) + " in wake from " + prevStart + " to " + prevEnd);
#endif
		Int32 symbolIndex = this->model->evidence[iGen]->GetSymbolIndex(iPos);

		List<Int32>^ candidates = gcnew List<Int32>;

		if (this->model->alphabet->IsTurtle(symbolIndex))
		{
			prevIsTurtle = true;
			// All turtle symbols must be in the prev range or the right neighbour of the range
			// Look to find all the places where the symbol matches
			for (Int16 iScan = prevStart; iScan >= prevEnd; --iScan)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length()) // if the previous symbols range already extends to the end then don't scan outside the word
				{
					if ((symbolIndex == this->model->evidence[iGen + 1]->GetSymbolIndex(iScan)) && (this->ValidateLeftNeighbourTurtle(this->model->evidence[iGen], this->model->evidence[iGen + 1], iPos - 1, iScan - 1)))
					{
						candidates->Add(iScan);
					}
				}
			}

			if (candidates->Count == 1)
			{
				prevSingleCandidate = true;
				this->MAO->SetLocalization(iGen, candidates[0], iPos, MasterAnalysisObject::cLocalization_Locked);
			}
			else
			{
				prevSingleCandidate = false;
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					// Upgrade the setting from weak to strong
					this->MAO->SetLocalization(iGen, candidates[iCandidate], iPos, MasterAnalysisObject::cLocalization_Strong);
				}
			}

			prevStart = candidates[0] - 1;
			prevEnd = candidates[candidates->Count - 1] - 1;
		}
		else
		{
			// if the prev symbol is a turtle, AND only had a single candidate
			// then this create a head fragment for the current symbol
			if ((prevIsTurtle) && (prevSingleCandidate))
			{
				prevSingleCandidate = true;
				this->ComputeFragment_Position_TailOnly(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevStart);
			}

			// Set the localization for the head fragments
			Fact^ f = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			// Adjust the wake based on the WTW
			Range^ wtw = this->model->evidence[iGen]->GetWTW(iPos);
			if (prevStart < wtw->start)
			{
				prevStart = wtw->start;
			}
			if (prevEnd > wtw->end)
			{
				prevEnd = wtw->start;
			}

			List<Int32>^ candidatesBTF = gcnew List<Int32>;
			Int32 startWake = 0;
			Int32 endWake = 99999;
			Int32 startLocalizationLimit = 0;
			Int32 endLocalizationLimit = 99999;
			prevIsTurtle = false;
			FragmentSigned^ btf;

			if (f->btf->Count == 1)
			{
				btf = f->btf[0];
				this->model->evidence[iGen]->SetBTF(iPos, f->btf[0]);
			}
			else
			{
				btf = this->model->evidence[iGen]->GetBTF(iPos);
			}

			for (Int16 iScan = prevStart; iScan >= prevEnd; --iScan)
			{
				if (iScan < this->model->evidence[iGen + 1]->Length())
				{
					Int32 start = iScan - btf->fragment->Length + 1;
					Int32 end = start + btf->fragment->Length;
					if (start >= 0)
					{
						String^ subword = this->model->evidence[iGen + 1]->GetSymbols(start, btf->fragment->Length);
						if (subword == btf->fragment && (iScan <= wtw->end && iScan >= wtw->start))
						{
							candidatesBTF->Add(iScan);
						}
					}
				}
			}

			// If no BTF could be placed in the wake, then create a new BTF and localize it
			// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
			// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
			// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
			// C. Create a BTF using the reserve algorithm
			if (candidatesBTF->Count == 0)
			{
				Fragment^ toCache;
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " cannot be localized in wake " + this->model->evidence[iGen + 1]->GetSymbolsFromPos(prevEnd, prevStart));

				// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				toCache = this->ComputeBTF_LocalizationCompatability_RightToLeft(btf, this->model->evidence[iGen + 1], prevEnd, prevStart);

				//// A. Try to check the existing BTF for localization compatability. Extend the wake and see if it localizes.
				//// -- if this fails then it means the BTF comes from an instance with a non-compatible signature
				//Int32 iBTF = 0;
				//bool btfFound = false;
				//do
				//{
				//	btf = this->ComputeBTF_LocalizationCompatability_RightToLeft(f->btf[iBTF], this->model->evidence[iGen + 1], prevEnd, prevStart);
				//	if (btf->fragment != nullptr)
				//	{
				//		btfFound = true;
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + btf->fragment);
				//	}
				//	else
				//	{
				//		Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + f->btf[iBTF]->fragment + " is not locally compatible.");
				//	}
				//	iBTF++;
				//} while (!btfFound && iBTF < f->btf->Count);

				//// B. Create a BTF by considering the max fragments. A BTF is formed from the start of the wake to the end of the next wake.
				//if (btf->fragment == nullptr && candidates->Count > 0)
				//{
				//	Int32 btfStart = 99999;
				//	Int32 btfEnd = prevStart;
				//	for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				//	{
				//		if (btfStart > candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length)
				//		{
				//			btfStart = Math::Max(0, candidates[iCandidate] - f->max[fragmentIndex[iCandidate]]->fragment->Length);
				//		}
				//	}
				//	btf = this->MAO->CreateFragment(this->model->evidence[iGen + 1]->GetSymbolsFromPos(btfStart, btfEnd),false,true);
				//	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found from max fragments BTF " + btf->fragment);
				//}

				// C. Create a BTF using the reserve algorithm
				//if (toCache->fragment == nullptr)
				//{
				//	toCache = this->ComputeFragment_Position_BTFOnly_RightToLeft(iGen, this->model->evidence[iGen], iPos, this->model->evidence[iGen + 1], prevEnd,prevStart);
				//	Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by position reserve BTF " + toCache->fragment);
				//}

				f->cache->Add(this->MAO->CreateBTF(toCache, btf->leftSignature, btf->rightSignature));
				this->MAO->SetFact_BTFOnly_Update(btf, f->cache[f->cache->Count - 1], f);
				Console::WriteLine("For SAC " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " found by localization compatibility BTF " + btf->fragment);
				candidatesBTF = this->ComputeSymbolLocalization_MaxFragment_RightToLeft(btf, this->model->evidence[iGen + 1], prevEnd, prevStart);
			}

			for (size_t iCandidate = 0; iCandidate < candidatesBTF->Count; iCandidate++)
			{
				Console::WriteLine("Localizing " + this->MAO->SymbolCondition(iGen, this->model->evidence[iGen], iPos) + " BTF " + btf->fragment + " @ " + candidatesBTF[iCandidate] + ".." + (1 + candidatesBTF[iCandidate] - btf->fragment->Length));

				if (candidatesBTF[iCandidate] - f->length->min > startWake)
				{
					startWake = candidatesBTF[iCandidate] - f->length->min;
				}
				if (candidatesBTF[iCandidate] > startLocalizationLimit)
				{
					startLocalizationLimit = candidatesBTF[iCandidate];
				}

				if (candidatesBTF[iCandidate] - btf->fragment->Length < endWake)
				{
					endWake = candidatesBTF[iCandidate] - btf->fragment->Length;
				}
				if (candidatesBTF[iCandidate] - btf->fragment->Length + 1 < endLocalizationLimit)
				{
					endLocalizationLimit = candidatesBTF[iCandidate] - btf->fragment->Length + 1;
				}

				for (size_t iLength = 0; iLength < btf->fragment->Length; iLength++)
				{
					this->MAO->SetLocalization(iGen, candidatesBTF[iCandidate] - iLength, iPos, MasterAnalysisObject::cLocalization_StrongBTF);
				}
			}

			startLocalizationLimit = Math::Min(prevStart, startLocalizationLimit);
			endLocalizationLimit = Math::Max(0, Math::Max(prevEnd - f->length->max + 1, endLocalizationLimit));

			Console::WriteLine("Absolute localization limit is " + startLocalizationLimit + " / " + endLocalizationLimit);

			if (startLocalizationLimit < startWake)
			{
				startWake = startLocalizationLimit;
			}

			if (endLocalizationLimit - 1 > endWake)
			{
				endWake = endLocalizationLimit - 1;
			}

			prevStart = startWake;
			prevEnd = endWake;

			if (iGen >= 999)
			{
				Console::ReadLine();
			}
		}
	}
}

List<Int32>^ LSIProblemV3::ComputeSymbolLocalization_MaxFragment_LeftToRight(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake)
{
	List<Int32>^ result = gcnew List<Int32>;
	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < W->Length())
		{
			// find a candidate for the BTF
			String^ subwordMax = W->GetSymbols(iScan, Fr->fragment->Length);
			if (subwordMax == Fr->fragment)
			{
				result->Add(iScan);
			}
		}
	}

	return result;
}

List<Int32>^ LSIProblemV3::ComputeSymbolLocalization_MaxFragment_RightToLeft(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake)
{
	List<Int32>^ result = gcnew List<Int32>;
	for (size_t iScan = StartWake; iScan <= EndWake; iScan++)
	{
		if (iScan < W->Length())
		{
			Int32 start = iScan - Fr->fragment->Length + 1;
			if (start >= 0)
			{
				String^ subwordMax = W->GetSymbols(start, Fr->fragment->Length);
				if (subwordMax == Fr->fragment)
				{
					result->Add(iScan);
				}
			}
		}
	}

	return result;
}

void LSIProblemV3::ComputeSymbolLocalization()
{
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
	Console::WriteLine();
	Console::WriteLine("Computing Symbol Localization");
	Console::WriteLine("=============================");
#endif
	// A. Perform BTF Only localization until convergence
	do
	{
		this->MAO->ResetBTFChangeFlag();
		for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
		{
			// Step 1. Perform left to right localization
			this->ComputeSymbolLocalization_LeftToRightLocalization_BTFOnly(iGen);
#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
			Console::WriteLine("Before Right to Left");
			Console::WriteLine("********************");
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
				for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
					}
				}
				Console::WriteLine();
			}
			Console::WriteLine();
#endif
			// Step 2. Perform right to left localization
			this->ComputeSymbolLocalization_RightToLeftLocalization_BTFOnly(iGen);

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
			Console::WriteLine("Before Weak Removal");
			Console::WriteLine("*******************");
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
				for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
					}
				}
				Console::WriteLine();
			}
			Console::WriteLine();
#endif

			// Step 3. Remove all weak localizations
			for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				for (Int32 iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
				{
					if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Unset))
					{
						this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
					}
				}
			}

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
			Console::WriteLine("Before Analysis");
			Console::WriteLine("***************");
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
				for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
					}
				}
				Console::WriteLine();
			}
			Console::WriteLine();
#endif
			// Step 4. Analyze
			this->ComputeSymbolLocalization_Analyze_BTFOnly(iGen);

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
			Console::WriteLine("After Analysis");
			Console::WriteLine("**************");
			for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
				for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
					}
					else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
					{
						Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
					}
				}
				Console::WriteLine();
			}
			Console::WriteLine();
#endif

			// Step 5. Build a new set of BTFs
			this->ComputeBTF_Localization(iGen);
			// ??? Should this only be running after all the generations are completed?
			this->ComputeBTF_Cache();
#if _PHASE3_COMPUTE_FRAGMENTS_CACHE_ >= 1
			this->MAO->Display();
#endif
		}

		// Reset all the localizations to unset
		if (this->MAO->HasBTFChangeOccured())
		{
			for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
			{
				for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
				{
					for (Int32 iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
					{
						if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF && this->MAO->localization[iGen][iPos2, iPos1] != MasterAnalysisObject::cLocalization_Locked)
						{
							this->MAO->SetLocalization_Direct(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Unset);
						}
						else if (this->MAO->localization[iGen][iPos2, iPos1] != MasterAnalysisObject::cLocalization_Locked)
						{
							this->MAO->SetLocalization_Direct(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
						}
					}
				}
			}
		}
	} while (this->MAO->HasBTFChangeOccured());

	// TODO: tomorrow morning... need to create the max fragments here

	// B. Perform head/tail/max/mid fragment localization
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		this->ComputeSymbolLocalization_LeftToRightLocalization(iGen);

		// Remove all BTF localizations
		for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
		{
			for (Int32 iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_WeakBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_StrongBTF) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Unset))
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
				}
			}
		}

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine("Before Right to Left");
		Console::WriteLine("********************");
		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Weak)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
				}
			}
			Console::WriteLine();
		}
		Console::WriteLine();
#endif
		
		this->ComputeSymbolLocalization_RightToLeftLocalization(iGen);

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine("Before Weak Removal");
		Console::WriteLine("*******************");
		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Weak)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
				}
			}
			Console::WriteLine();
		}
		Console::WriteLine();
#endif

		// Remove all weak localizations
		for (Int32 iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
		{
			for (Int32 iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
			{
				if ((this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Weak) || (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Unset))
				{
					this->MAO->SetLocalization(iGen, iPos2, iPos1, MasterAnalysisObject::cLocalization_Never);
				}
			}
		}

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine("Before Analysis");
		Console::WriteLine("***************");
		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Weak)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
				}
			}
			Console::WriteLine();
		}
		Console::WriteLine();
#endif

		this->ComputeSymbolLocalization_Analyze(iGen);

#if _PHASE3_COMPUTE_LOCALIZATION_ >= 1
		Console::WriteLine("After Analysis");
		Console::WriteLine("***************");
		for (size_t iPos2 = 0; iPos2 < this->model->evidence[iGen + 1]->Length(); iPos2++)
		{
			Console::Write(iPos2 + " : " + this->model->evidence[iGen + 1]->GetSymbol(iPos2) + " : ");
			for (size_t iPos1 = 0; iPos1 < this->model->evidence[iGen]->Length(); iPos1++)
			{
				if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Weak)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + ")");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Strong)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "*)");
				}
				else if (this->MAO->localization[iGen][iPos2, iPos1] == MasterAnalysisObject::cLocalization_Locked)
				{
					Console::Write(this->model->evidence[iGen]->GetSymbol(iPos1) + "(" + iPos1 + "!)");
				}
			}
			Console::WriteLine();
		}
		Console::WriteLine();
#endif

		this->ComputeFragments_Localization(iGen);
	}
}

float LSIProblemV3::CompareEvidence_Positional(List<WordV3^>^ E1, Int32 E1_Start, Int32 E1_End, List<WordV3^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	int errors = 0;
	float errorValue = 0.0;
	int total = 0;
	float result = 0.0;
	int generationsSolved = 0;

	for (size_t iGen = 0; iGen < E1->Count; iGen++)
	{
		total += Math::Max(E1[E1_Start + iGen]->Length(), E2[E2_Start + iGen]->Length());

		// Check results versus evidence
		Int32 iSymbol = 0;

		// TODO: removed the solution limiter, might need to re-add it
		// TODO: convert to some kind of LCS-like process
		Int32 stop = Math::Min(E1[E1_Start + iGen]->Length(), E2[E2_Start + iGen]->Length());
		errors += Math::Max(E1[E1_Start + iGen]->Length(), E2[E2_Start + iGen]->Length()) - stop;
		errorValue += Math::Max(E1[E1_Start + iGen]->Length(), E2[E2_Start + iGen]->Length()) - stop * (1 / 1 + iGen);

		while (iSymbol < stop)
		{
			if (E1[E1_Start + iGen]->GetSymbol(iSymbol) != E2[E2_Start + iGen]->GetSymbol(iSymbol))
			{
				errorValue += (float)1 / (1 + iGen);
				errors++; 
			}

			iSymbol++;
		}
	}

	result = (float)errorValue / total;

	return result;
}

void LSIProblemV3::DisplayParikhVector(array<Int32>^ PV, String^ msg)
{
	Console::Write(msg + "< ");
	for (size_t iSymbol = 0; iSymbol <= PV->GetUpperBound(0); iSymbol++)
	{
		Console::Write(PV[iSymbol] + this->model->alphabet->symbols[iSymbol] + " ");
	}
	Console::WriteLine(">");
}

void LSIProblemV3::FindSubProblems()
{

}

void LSIProblemV3::IsAmbiguous()
{
	array<bool, 2>^ lexiconSACRight = gcnew array<bool, 2>(this->MAO->factsList->Count, this->MAO->factsList->Count);

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Fact^ prev;
		Fact^ curr;
		for (size_t iPos = 0; iPos < this->model->evidence[iGen]->Length(); iPos++)
		{
			if (iPos == 0)
			{
				prev = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
			}
			else if (prev->ID != 0)
			{
				curr = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				Int32 indexCurr = this->MAO->GetFactIndex(curr);
				Int32 indexPrev = this->MAO->GetFactIndex(prev);
				if (indexCurr != -1 && indexPrev != -1)
				{
					lexiconSACRight[indexPrev, indexCurr] = true;
				}
				prev = curr;
			}
			else
			{
				curr = this->MAO->GetFact(iGen, this->model->evidence[iGen], iPos);
				prev = curr;
			}
		}
	}

	for (size_t iFact = 0; iFact < this->MAO->factsList->Count; iFact++)
	{
		Fact^ curr = this->MAO->factsList[iFact];
		Console::WriteLine("Right Neighbours for SAC " + this->MAO->SymbolCondition(curr->sac->iSymbol, curr->sac->iLeft, curr->sac->iRight));
		for (size_t jFact = 0; jFact < this->MAO->factsList->Count; jFact++)
		{
			if (lexiconSACRight[iFact, jFact])
			{
				Fact^ prev = this->MAO->factsList[jFact];
				Console::WriteLine(this->MAO->SymbolCondition(prev->sac->iSymbol, prev->sac->iLeft, prev->sac->iRight));
			}
		}
	}
}

void LSIProblemV3::SimplifyModel()
{
	// If a non-turtle symbol is guaranteed to be terminal or a drawing symbol it can be treated as terminal
}

void LSIProblemV3::Solve()
{
	// Check solution size estimate, if it is small just search for a solution
	if (this->MAO->solutionSpaceSize_Length <= LSIProblemV3::cSmallSolutionSpace)
	{

	}
	else
	{	// Find any subproblems


	}

	// Check if there are no subproblems, if not then search for a solution to the current problem
	if (!this->hasSubProblems)
	{

	}
	else
	{// instantiate the first subproblem and solve it

	}
}

void LSIProblemV3::SolveGenerationSubProblem()
{

}

void LSIProblemV3::SolveSymbolSubProblem()
{

}

GenomeConfiguration<Int32>^ LSIProblemV3::CreateGenome()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	// Need a number of genes equal to the number of rules in the problem
	for (size_t iSAC = 0; iSAC < this->MAO->sacs->Count; iSAC++)
	{

	}

	//for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	//{
	//	this->rule2solution->Add(-1);
	//	this->solution2rule->Add(-1);

	//	if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
	//	{
	//		this->rule2solution[iRule] = numGenes;
	//		this->solution2rule[numGenes] = iRule;
	//		numGenes += 1;
	//	}
	//}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	//for (size_t iGene = 0; iGene < min->Length; iGene++)
	//{
	//	Int32 iRule = this->solution2rule[iGene];
	//	min[iGene] = this->minRuleLengths[iRule];
	//	max[iGene] = this->maxRuleLengths[iRule];
	//}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

SolutionType^ LSIProblemV3::SolveProblemGA()
{
	GeneticAlgorithmPMIT^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

	genomeConfig = this->CreateGenome();
	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
	configuration->populationSizeInit = 100;
	configuration->populationSizeMax = 100;
	configuration->crossoverWeight = 0.85;
	configuration->mutationWeight = 0.15;
	configuration->preferHighFitness = false;

	ai = gcnew GeneticAlgorithmPMIT(configuration);
	ai->problem = this;

	Int32 tmp1 = Math::Max((UInt64)250, Math::Min(this->MAO->solutionSpaceSize_Length * 10, (UInt64)30000));
	Int32 tmp2 = Math::Max((UInt64)250, Math::Min(this->MAO->solutionSpaceSize_Length * 10, (UInt64)250000));

	//tmp1 *= this->modifierGenerations;
	//tmp2 *= this->modifierGenerations;

	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(tmp1, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0.0));
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(tmp2));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;

	SolutionType^ S = gcnew SolutionType();
	S->solutions = ai->SolveAllSolutions();
	S->fitnessValues = ai->FitnessValues();

	return S;
}


bool LSIProblemV3::ValidateLeftNeighbourTurtle(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2)
{
	bool result = false;

	if (Pos1 < 0 && Pos2 < 0)
	{// if they're both outside the word, then this is valid. This is an exit condition for recursion
		result = true;
	}
	else if (Pos1 < 0 || Pos2 < 0)
	{// if one is outside the word and the other isn't, this is not valid and is an exit condition.
		result = false;
	}
	else if ((this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1))) && (W1->GetSymbolIndex(Pos1) != W2->GetSymbolIndex(Pos2)))
	{// if a turtle is in the predecesor word but is a non-matching symbol is in the successor, this is invalid and is an exit condition
		result = false;
	}
	else if (!this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1)))
	{// if the symbol in the predecessor is a non-turtle, this is valid and is an exit condition
		result = true;
	}
	else if ((W1->GetSymbolIndex(Pos1) == W2->GetSymbolIndex(Pos2)) && (this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1))) && (this->model->alphabet->IsTurtle(W2->GetSymbolIndex(Pos2))))
	{// if they are both the same turtle graphic, this is valid but check the right neighbour recursively
		result = this->ValidateLeftNeighbourTurtle(W1, W2, Pos1 - 1, Pos2 - 1);
	}

	return result;
}

// Looks like this isn't needed
bool LSIProblemV3::ValidateRightNeighbour(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2, Fragment^ Fr)
{
	bool result = false;

	return result;
}

bool LSIProblemV3::ValidateRightNeighbourTurtle(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2)
{
	bool result = false;

	if (Pos1 >= W1->Length() && Pos2 >= W2->Length())
	{// if they're both outside the word, then this is valid. This is an exit condition for recursion
		result = true;
	}
	else if (Pos1 >= W1->Length() || Pos2 >= W2->Length())
	{// if one is outside the word and the other isn't, this is not valid and is an exit condition.
		result = false;
	}
	else if ((this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1))) && (W1->GetSymbolIndex(Pos1) != W2->GetSymbolIndex(Pos2)))
	{// if a turtle is in the predecesor word but is a non-matching symbol is in the successor, this is invalid and is an exit condition
		result = false;
	}
	else if (!this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1)))
	{// if the symbol in the predecessor is a non-turtle, this is valid and is an exit condition
		result = true;
	}
	else if ((W1->GetSymbolIndex(Pos1) == W2->GetSymbolIndex(Pos2)) && (this->model->alphabet->IsTurtle(W1->GetSymbolIndex(Pos1))) && (this->model->alphabet->IsTurtle(W2->GetSymbolIndex(Pos2))))
	{// if they are both the same turtle graphic, this is valid but check the right neighbour recursively
		result = this->ValidateRightNeighbourTurtle(W1, W2, Pos1 + 1, Pos2 + 1);
	}

	return result;
}

//void LSIProblemV3::ComputeSymbolFacts()
//{
//	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->symbols->Count; iSymbol++)
//	{
//		String^ s = this->model->alphabet->symbols[iSymbol];
//		SymbolFact^ fact = gcnew SymbolFact();
//
//		if (this->model->alphabet->IsTurtle(s))
//		{
//			fact->isGraphical = true;
//			fact->isKnown = true;
//			fact->isProducer = false;
//			fact->isTerminal = true;
//			this->MAO->SetSymbolFact(iSymbol, fact);
//		}
//		else
//		{
//			fact->isGraphical = false;
//			fact->isKnown = false;
//			fact->isProducer = true;
//			fact->isTerminal = false;
//			this->MAO->SetSymbolFact(iSymbol, fact);
//		}
//	}
//}
