#include "stdafx.h"
#include "PMIProblemV2A.h"


PMIProblemV2A::PMIProblemV2A(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2(Model, ErrorRate, FileName)
{
	this->tabu = gcnew List<TabuItem^>;
}

PMIProblemV2A::~PMIProblemV2A()
{
}

void PMIProblemV2A::Initialize(PlantModel^ Model, Int32 StartGeneration, Int32 EndGeneration)
{
	this->model = Model;
	this->solution = gcnew PlantModel();
	this->startGeneration = StartGeneration;
	this->endGeneration = EndGeneration;
	this->beliefs = gcnew BeliefSystem();
	this->changes = gcnew List<RPTChange^>;
	this->equations = gcnew List<ParikhEquation^>;
	this->pathResult = PMIProblem::PathResult;
	this->fitnessOption = FitnessOption::Positional;
	this->constantIndex = -1;
	this->solutionSpaceSizes = gcnew List<UInt64>;
	this->produceFragments = true;

	// Make a fake current set for the initialization process
	this->currentSet = gcnew PMIProblemSet(this->startGeneration, this->endGeneration);

	// Initialize metrics
	this->solved = false;
	this->matched = false;
	this->analysisTime = 0;
	this->solveTime = 0;

	// Load the evidence from the model and extract vocabulary from evidence
	this->LoadEvidence();
	this->ExtractVocabulary();
	this->GenerateError();

	this->generationsSolved = gcnew array<bool>(this->generations);
	this->generationsSolvedModulesOnly = gcnew array<bool>(this->generations);
	this->numModules = gcnew array<Int32>(this->generations);
	this->numConstants = gcnew array<Int32>(this->generations);

	// Analysis time starts here
	auto startTime = Time::now();

	Console::WriteLine("Initial Analysis");

	bool newConstantDetected = false;

	do
	{
		newConstantDetected = false;
		this->CompressEvidence();
		this->InitializeAnalysisTables();
		// Always do this analysis
		if (!newConstantDetected)
		{
			this->ComputeMinHeadsTails();
			newConstantDetected = this->ComputeConstants();
		}

		// Always do this analysis
		if (!newConstantDetected)
		{
			this->InitializeGrowthPattern(); // this estimation needs to happen because rule lengths are critical to the fragment production
			newConstantDetected = this->ComputeConstants();
		}

		// Always do this analysis
		if (!newConstantDetected)
		{
			this->EstimateRuleLengths(false, false); // this estimation needs to happen because rule lengths are critical to the fragment production
			newConstantDetected = this->ComputeConstants();
		}

#if _PMI_PROBLEM_ANALYZE_FRAGMENTS_ > 0
		if (!newConstantDetected)
		{
			Console::WriteLine("Computing Fragments (this can take some time)");
			this->ComputeFragmentsFromEvidence();
			newConstantDetected = this->ComputeConstants();
		}
#endif

#if _PMI_PROBLEM_ANALYZE_UAG_ > 0
		if (!newConstantDetected)
		{
			this->EstimateGrowthPattern(); // this estimation needs to happen because rule lengths are critical to the lexicon production
			newConstantDetected = this->ComputeConstants();
		}
#endif

		// Always do this analysis
		if (!newConstantDetected)
		{
			this->EstimateRuleLengths(false, false); // this estimation needs to happen because rule lengths are critical to the lexicon production
			newConstantDetected = this->ComputeConstants();
		}

#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
		if (!newConstantDetected)
		{
			Console::WriteLine("Computing Lexicon (this can also take some time)");
			//this->ComputeLexicon();
			newConstantDetected = this->ComputeConstants();
		}
#endif

		//this->OutputAnalysis();

	} while (newConstantDetected);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->analysisTime = ms.count();

#if _PMI_PROBLEM_FRAGMENTS_SAVE_ > 0
	this->SaveFragments();
#endif

	// Scan the evidence and ensure any constants are marked as such
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->Count; iSymbol++)
		{
			Int32 symbolIndex = this->vocabulary->FindIndex(this->evidence[iGen]->symbols[iSymbol]);
			if (symbolIndex > this->vocabulary->IndexModulesEnd)
			{
				this->evidence[iGen]->symbols[iSymbol]->isModule = false;
			}
		}
	}


#if _PMI_PROBLEM_KNOWN_CONSTANTS_ == 0
	// If F and f are still modules set their min/max PoG and min/max rule length accordinging
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if ((this->vocabulary->Symbols[iRule]->Value() == "F") || (this->vocabulary->Symbols[iRule]->Value() == "f"))
		{
			for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
			{
				if (iSymbol == iRule)
				{
					this->minPoG[iRule, iSymbol] = 1;
					this->maxPoG[iRule, iSymbol] = 2;
				}
				else
				{
					this->minPoG[iRule, iSymbol] = 0;
					this->maxPoG[iRule, iSymbol] = 0;
				}
			}
			this->minRuleLengths[iRule] = this->minPoG[iRule, iRule];
			this->maxRuleLengths[iRule] = this->maxPoG[iRule, iRule];
		}
	}
#endif

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			if (this->minPoG[iRule, iSymbol] == this->maxPoG[iRule, iSymbol])
			{
				for (size_t iGen = 0; iGen < this->generations; iGen++)
				{
					this->UpdateRPT(iGen, iRule, iSymbol, this->minPoG[iRule, iSymbol]);
				}
			}
		}
	}

	for (size_t iRule = 0; iRule <= this->ruleFragmentsMaster->GetUpperBound(0); iRule++)
	{
		this->ruleFragmentsOriginal[iRule] = gcnew Word();
		this->ruleFragmentsOriginal[iRule]->symbols->AddRange(this->ruleFragmentsMaster[iRule]->symbols);
	}

	Console::WriteLine("Checking for sub-problems");
	this->FindSubProblems();
	this->CheckForSolvedGenerations(0, this->generations);

	this->rulesParikhTemplateMaster = gcnew array<Int32, 3>(this->generations, this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());
	for (size_t iGen = 0; iGen <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Gen); iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				this->rulesParikhTemplateMaster[iGen, iRule, iSymbol] = this->rulesParikhTemplate[iGen, iRule, iSymbol];
			}
		}
	}

	this->rulesSolution = gcnew List<ProductionRule^>;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		this->rulesSolution->Add(gcnew ProductionRule());
	}

	//Console::WriteLine("Base Solution Space = " + this->ComputeSolutionSpaceSize());
	this->OutputAnalysis();
}

void PMIProblemV2A::InitializeAnalysisTables()
{
	this->solvedModules = gcnew array<bool>(this->vocabularyMaster->numModules);
	this->solvedModulesMaster = gcnew array<bool>(this->vocabularyMaster->numModules);

	this->ruleModulesPool = gcnew array<List<Symbol^>^>(this->vocabulary->numModules);
	this->ruleConstantsPool = gcnew array<List<Symbol^>^>(this->vocabulary->numModules);

	this->ruleHead = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleTail = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleFragments = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleHeadMaster = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleTailMaster = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleFragmentsMaster = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleFragmentsOriginal = gcnew array<Word^>(this->vocabulary->numModules);

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		this->solvedModules[i] = false;
		this->solvedModulesMaster[i] = false;
		this->ruleHead[i] = gcnew Word();
		this->ruleTail[i] = gcnew Word();
		this->ruleFragments[i] = gcnew Word();
		this->ruleHeadMaster[i] = gcnew Word();
		this->ruleTailMaster[i] = gcnew Word();
		this->ruleFragmentsMaster[i] = gcnew Word();
		this->ruleModulesPool[i] = gcnew List<Symbol^>;
		this->ruleConstantsPool[i] = gcnew List<Symbol^>;
	}

	this->maxRuleLengthsMaster = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengthsMaster = gcnew array<Int32>(this->vocabulary->numModules);
	this->maxRuleLengthsWIP = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengthsWIP = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule <= this->minRuleLengthsMaster->GetUpperBound(0); iRule++)
	{
		this->minRuleLengthsMaster[iRule] = 0;
		this->maxRuleLengthsMaster[iRule] = 999999999;
		this->minRuleLengthsWIP[iRule] = 0;
		this->maxRuleLengthsWIP[iRule] = 999999999;
	}

	// Find the total minimum and maximum length of all rules based on inference
	this->totalMinRuleLength = 0;
	this->totalMaxRuleLength = 999999999;

	this->minRuleLengths = this->minRuleLengthsMaster;
	this->maxRuleLengths = this->maxRuleLengthsMaster;
	this->knownRuleLengthsIndexed = gcnew array<Int32, 2>(1 + this->vocabularyMaster->numConstants, this->vocabularyMaster->numModules);
	for (size_t i = 0; i <= this->knownRuleLengthsIndexed->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->knownRuleLengthsIndexed->GetUpperBound(1); j++)
		{
			this->knownRuleLengthsIndexed[i, j] = PMIProblem::cUnknownRuleLength;
		}
	}

	this->PoGMaster = gcnew array<Int32, 3>(this->generations - 1, this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());
	this->totalPoGMaster = gcnew array<Int32>(this->vocabularyMaster->NumSymbols());
	this->minPoGMaster = gcnew array<Int32, 2>(this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());
	this->maxPoGMaster = gcnew array<Int32, 2>(this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());

	this->minPoGWIP = gcnew array<Int32, 2>(this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());
	this->maxPoGWIP = gcnew array<Int32, 2>(this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());

	this->ComputeSymbiology();
	this->ComputeFinalGeneration();
	this->ComputeParikhVectors();
}

void PMIProblemV2A::InitializeGrowthPattern()
{
	for (size_t iRule = 0; iRule <= this->minPoGMaster->GetUpperBound(0); iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		array<Int32>^ metaFragmentPV = this->vocabularyMaster->CompressSymbols(metaFragment);
		for (size_t iSymbol = 0; iSymbol <= this->minPoGMaster->GetUpperBound(1); iSymbol++)
		{
			this->minPoGMaster[iRule, iSymbol] = metaFragmentPV[iSymbol];
			this->maxPoGMaster[iRule, iSymbol] = 999999;
			this->minPoGWIP[iRule, iSymbol] = metaFragmentPV[iSymbol];
			this->maxPoGWIP[iRule, iSymbol] = 999999;
		}
	}

	this->PoG = this->PoGMaster;
	this->minPoG = this->minPoGMaster;
	this->maxPoG = this->maxPoGMaster;
	this->totalPoG = this->totalPoGMaster;

#if _PMI_PROBLEM_ANALYZE_UAG_ > 0
	this->ComputeGrowthPattern_Unaccounted();
#endif
}


void PMIProblemV2A::AddTabuItem(TabuItem^ T)
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
		//if ((this->currentSet->startGeneration == 1) && (this->constantIndex == 0))
		//{
		//	Console::WriteLine("Adding Tabu Item");
		//	Console::WriteLine("Parikh Rules");
		//	Console::Write("  ");
		//	for (size_t iSymbol = 0; iSymbol < this->vocabulary->Symbols->Count; iSymbol++)
		//	{
		//		Console::Write(" " + this->vocabulary->Symbols[iSymbol] + "  ");
		//	}
		//	Console::WriteLine();
		//	for (size_t iRule = 0; iRule <= T->rules->GetUpperBound(0); iRule++)
		//	{
		//		Console::Write(this->vocabulary->Symbols[iRule]);
		//		for (size_t iSymbol = 0; iSymbol <= T->rules->GetUpperBound(1); iSymbol++)
		//		{
		//			Console::Write(" " + T->rules[iRule, iSymbol].ToString(L"D3"));
		//		}
		//		Console::WriteLine();
		//	}
		//	Console::WriteLine();
		//	Console::ReadLine();
		//}


		//if ((T->rules[0, 6] == 2) && (T->rules[2, 6] == 1) && (T->rules[3, 6] == 3) && (T->rules[4, 6] == 1))
		//{
		//	Console::WriteLine("AddTabuItem::Stop");
		//	Console::ReadLine();
		//}

		if (!this->IsTabu(T->index, T->rulesParikh))
		{
			this->tabu->Add(T);
		}
	}
}

void PMIProblemV2A::Analyze()
{
	this->flagAnalysisFailed = false;

	// Analysis time starts here
	auto startTime = Time::now();

	//this->ComputeNeighbouringSymbols();
	//this->CheckForSolvedGenerations(0, this->generations);
	//// TODO: this needs to be an estimation of the subproblems and then finalize the sub-problems after compute the feature vectors
	//// When that happens will need to add an interative estimate of the rule lengths and growth pattern before this step
	//this->FindSubProblems();

	// Loop through computing rule lengths and growth pattern until they converge and there is no update
	// This is because compute rule lengths can effect how growth patterns is computed
	// and computing the growth pattern can effect how the rule lengths are computed
	bool changes = true;
	while ((changes) && (!this->flagAnalysisFailed))
	{
		array<Int32>^ tmpMin = gcnew array<Int32>(this->minRuleLengths->GetUpperBound(0)+1);
		array<Int32>^ tmpMax = gcnew array<Int32>(this->maxRuleLengths->GetUpperBound(0)+1);

		this->minRuleLengths->CopyTo(tmpMin, 0);
		this->maxRuleLengths->CopyTo(tmpMax, 0);

		this->EstimateRuleLengths(true, true);

		this->SynchronizeAnalysis();
#if _PMI_PROBLEM_ANALYZE_UAG_ > 0
		if (!this->flagAnalysisFailed)
		{
			this->ComputeGrowthPattern();
		}
#endif
		this->SynchronizeAnalysis();

		changes = false;
		for (size_t iRule = 0; iRule <= this->minRuleLengths->GetUpperBound(0); iRule++)
		{
			if ((this->minRuleLengths[iRule] != tmpMin[iRule]) || (this->maxRuleLengths[iRule] != tmpMax[iRule]))
			{
				changes = true;
			}
		}
	}

	if (!this->flagAnalysisFailed)
	{
		this->UpdateKnownRuleLengths();
		this->CheckForSolvedGenerations(0, this->generations);
		this->UpdateCompleteProblems();
		this->UpdateSolvedProblems();
	}

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->analysisTime += ms.count();

	this->SynchronizeAnalysis();
	this->OutputAnalysis();
}

void PMIProblemV2A::AnalyzeMaxFragments(List<SymbolFragmentType^>^ Fragments)
{
	for (size_t iFragment = 0; iFragment < Fragments->Count; iFragment++)
	{
		Int32 ruleIndex = this->vocabularyMaster->FindIndex(Fragments[iFragment]->s);

		// Only capture a max fragment if it happens to be complete. This should pretty fairly rarely
		if (Fragments[iFragment]->isComplete)
		{
			this->CaptureFragment(ruleIndex, Fragments[iFragment]->fragment);
			this->ruleHeadMaster[ruleIndex] = Fragments[iFragment]->fragment;
		}
		
		//Console::WriteLine(Fragments[iFragment]->s->ToString() + " Max Length = " + Fragments[iFragment]->fragment->Count);
		this->UpdateMaxRuleLengths(ruleIndex, Fragments[iFragment]->fragment->Count);
		this->UpdateMaxPoG(ruleIndex, Fragments[iFragment]->fragment);
	}
}

void PMIProblemV2A::AnalyzeMinFragments(List<SymbolFragmentType^>^ Fragments)
{
	for (size_t iFragment = 0; iFragment < Fragments->Count; iFragment++)
	{
		Int32 ruleIndex = this->vocabularyMaster->FindIndex(Fragments[iFragment]->s);

#if _PMI_PROBLEM_DISPLAY_FRAGMENT_ANALYSIS_ >= 1
		Console::WriteLine("Tracker #" + this->tracker + " Fragment Found (" + Fragments[iFragment]->isComplete + ") " + this->vocabulary->Symbols[ruleIndex]->Value() + " => " + Fragments[iFragment]->fragment->ToString());
#endif // _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 2

		if (Fragments[iFragment]->isComplete)
		{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_ANALYSIS_ >= 2
			if ((Fragments[iFragment]->fragment->Count == 1) && (Fragments[iFragment]->fragment[0] == Fragments[iFragment]->s))
			{
				Console::WriteLine("Tracker #" + this->tracker + " Constant found " + Fragments[iFragment]->s->ToString() + " => " + Fragments[iFragment]->fragment->ToString());
				//Console::ReadLine();
			}
#endif
			this->CaptureFragment(ruleIndex, Fragments[iFragment]->fragment);
			this->ruleHeadMaster[ruleIndex] = Fragments[iFragment]->fragment;
		}
		else if ((Fragments[iFragment]->isHead) && (Fragments[iFragment]->fragment->Count > this->ruleHeadMaster[ruleIndex]->Count))
		{
			this->ruleHeadMaster[ruleIndex] = Fragments[iFragment]->fragment;
		}
		else if ((!Fragments[iFragment]->isHead) && (Fragments[iFragment]->fragment->Count > this->ruleTailMaster[ruleIndex]->Count))
		{
			this->ruleTailMaster[ruleIndex] = Fragments[iFragment]->fragment;
		}
	}
}

bool PMIProblemV2A::AnalyzeSolution(PMISolution^ s)
{
	return true;
}

float PMIProblemV2A::Assess(array<Int32>^ Solution)
{
	return 0.0;
}

bool PMIProblemV2A::Assess_BranchingSymbol()
{
	bool result = false;
	this->rule2solution = gcnew List<Int32>;

	array<Int32>^ ruleLengths = this->ComputeRuleLengthFromRPT(this->currentSet->startGeneration, this->vocabulary->numModules + this->constantIndex - 2);
	for (size_t iRule = 0; iRule <= ruleLengths->GetUpperBound(0); iRule++)
	{
		// Take the rule lengths and add the amount of the previous symbol to the length
		// E.g. if they previous symbol was "]" and there were two of them then the length with the "[" should be two longer
		ruleLengths[iRule] += this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, this->vocabulary->numModules + this->constantIndex - 2];
		this->rule2solution->Add(iRule);
	}

	float fitness = this->AssessRuleLengths(ruleLengths, true);

	if (fitness == 0)
	{
		List<ProductionRule^>^ rules = this->Assess_CreateRulesStep_RuleLengths(ruleLengths);

		// Make sure the brackets are in the right order, if not then this is not a valid solution
		Symbol^ Right  = this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2];
		Symbol^ Left = this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1];

		Int32 iRule = 0;
		bool valid = true;
		while ((valid) && (iRule < rules->Count))
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ > 0
			Console::WriteLine("Verifying : " + rules[iRule]->ToString());
#endif
			Int32 count = 0;
			Int32 iSymbol = 0;

			while ((count >= 0) && (iSymbol < rules[iRule]->successor->Count))
			{
				if (rules[iRule]->successor[iSymbol]->Value() == Left->Value())
				{
					count++;
				}
				else if (rules[iRule]->successor[iSymbol]->Value() == Right->Value())
				{
					count--;
				}
				iSymbol++;
			}

			if (count != 0)
			{
				valid = false;
			}

			iRule++;
		}

		if (valid)
		{
			this->CapturePartialSolution_Rules(rules);
			this->SynchronizeAnalysis();
			result = true;
		}
	}

	return result;
}

array<Int32>^ PMIProblemV2A::Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution)
{
	array<Int32>^ axiom = gcnew array<Int32>(Size);

	for (size_t i = 0; i < axiom->Length; i++)
	{
		axiom[i] = Solution[i];
	}

	return axiom;
}

List<ProductionRule^>^ PMIProblemV2A::Assess_CreateRulesStep(array<Int32>^ RuleLengths)
{
	return this->Assess_CreateRulesStep_RuleLengths(RuleLengths);
}

List<ProductionRule^>^ PMIProblemV2A::Assess_CreateRulesStep_RuleLengths(array<Int32>^ Solution)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	R = this->CreateRulesByScanning(Solution, this->currentSet->startGeneration, this->currentSet->endGeneration);
	return R;
}

float PMIProblemV2A::Assess_Modules(array<Int32>^ Solution)
{
	throw gcnew Exception("PMIProblemV2A::Assess_Modules not supported. Must be overriden by sub-class.");
}

float PMIProblemV2A::Assess_Constants(array<Int32>^ Solution)
{
	throw gcnew Exception("PMIProblemV2A::Assess_Constants not supported. Must be overriden by sub-class.");
}

float PMIProblemV2A::AssessRuleLengths(array<Int32>^ RuleLengths)
{
	return this->AssessRuleLengths(RuleLengths, false);
}

float PMIProblemV2A::AssessRuleLengths(array<Int32>^ RuleLengths, bool ForceRuleLength)
{
	float fitness = 0.0;

#if _PMI_PROBLEM_CHEAT_ > 0
	RuleLengths = this->Cheat(RuleLengths);
#endif

	List<ProductionRule^>^ modelRules;

	if (!this->IsTabu(this->constantIndex, RuleLengths))
	{
		// Sometimes need to force this to treat the incoming values as absolute rule lengths
		// For example, when assess branching symbols
		// Otherwise need to create the step using whatever custom method exists for creating them
		// The function "Assess_CreateRulesStep" originally only accepted ruleLengths but really this should be a solution
		if (ForceRuleLength)
		{
			modelRules = this->Assess_CreateRulesStep_RuleLengths(RuleLengths);
		}
		else
		{
			modelRules = this->Assess_CreateRulesStep(RuleLengths);
		}


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
			fitness = this->currentSet->endGeneration - this->startGeneration + 1;
		}
	}
	else
	{
		fitness = this->currentSet->endGeneration - this->startGeneration + 1;
	}

	return fitness;
}

float PMIProblemV2A::AssessProductionRules(List<ProductionRule^>^ Rules)
{
	float result = 0.0;

	Word^ a = gcnew Word(this->evidence[this->currentSet->startGeneration]->symbols);
	List<Word^>^ solutionAxioms = gcnew List<Word^>;
	solutionAxioms = EvidenceFactory::Process(a, Rules, this->currentSet->startGeneration, this->currentSet->endGeneration, true);
#if _PMI_PROBLEM_SEARCH_GA_ > 0
	result = this->CompareEvidence_Positional(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentSet->startGeneration, this->currentSet->endGeneration);
#else
	result = this->CompareEvidence_PositionalAbort(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentSet->startGeneration, this->currentSet->endGeneration);
#endif

	return result;
}

void PMIProblemV2A::CaptureFragment(Int32 iRule, Word^ W)
{
	// If the fragment has not been compressed, then compress it
	if (!W->compressed)
	{
		this->vocabularyMaster->CompressSymbols(W);
	}

	// Capture to the RPT
	for (size_t iGen = 0; iGen <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Gen); iGen++)
	{
		for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
		{
			this->rulesParikhTemplate[iGen, iRule, iSymbol] = W->parikhVector[iSymbol];
		}
	}

	// Update Min/Max Pog
	for (size_t iSymbol = 0; iSymbol < W->parikhVector->GetUpperBound(0); iSymbol++)
	{
		this->minPoGMaster[iRule, iSymbol] = W->parikhVector[iSymbol];
		this->maxPoGMaster[iRule, iSymbol] = W->parikhVector[iSymbol];
	}

	// Update Min/Max Rule Lengths

	this->minRuleLengthsMaster[iRule] = W->symbols->Count;
	this->maxRuleLengthsMaster[iRule] = W->symbols->Count;

	// Update solved modules
	this->solvedModules[iRule] = true;
}

void PMIProblemV2A::CapturePartialFragment(Int32 iRule, Word^ W)
{
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
		{
			for (size_t iFragmentSymbol = 0; iFragmentSymbol < W->symbols->Count; iFragmentSymbol++)
			{

			}
		}
	}
}

void PMIProblemV2A::CapturePartialSolution(array<Int32, 2>^ Rules)
{
	this->currentTabu = gcnew TabuItem(this->constantIndex, Rules);
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
		{
			if (this->IsModuleInSet(iRule))
			{
				Int32 sum = 0;
				for (size_t iSymbol = 0; iSymbol <= Rules->GetUpperBound(1); iSymbol++)
				{
					RPTChange^ C = gcnew RPTChange(iGen, iRule, iSymbol, this->rulesParikhTemplate[iGen, iRule, iSymbol]);
					this->changes->Add(C);
					this->UpdateRPT(iGen, iRule, iSymbol, Rules[iRule, iSymbol]);
					sum += Rules[iRule, iSymbol];
					//this->rulesParikhTemplate[iGen, iRule, iSymbol] = Rules[iRule, iSymbol];
				}
				this->knownRuleLengthsIndexed[this->constantIndex, iRule] = sum;
			}
		}
	}

	this->CheckForSolvedGenerations(this->currentSet->startGeneration, this->currentSet->endGeneration);
#if _PMI_PROBLEM_VERBOSE_ >= 3
	this->OutputRPT();
#endif
}

void PMIProblemV2A::CapturePartialSolution_Rules(List<ProductionRule^>^ Rules)
{
	// If the fitness is zero, it is possible that the model rules are correct by the RPT is wrong
	// E.g. if the RPT says 5 x A and 1 x B and the real answer is 3 x A and 3 x B
	// It will have found the right model because the lengths are the same
	// So convert the rules into Parkih vectors and save back to the RPT
	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		Int32 index = this->vocabulary->RuleIndex(Rules[iRule]->predecessor);

		//if (this->knownRuleLengths[index] == PMIProblem::cUnknownRuleLength)
		//{
		array<Int32>^ pv = this->vocabulary->CompressSymbols(Rules[iRule]->successor);
		Int32 length = this->vocabulary->ParikhVectorLength(pv);
		this->CaptureParikhVector(index, pv);
		//this->knownRuleLengths[index] = length;
		this->knownRuleLengthsIndexed[this->constantIndex, index] = length;
		this->solution->rules->Add(Rules[iRule]);
		//}

		// this needs to be a deep copy otherwise the tabu system fails
		this->rulesSolution[iRule] = gcnew ProductionRule();
		for (size_t iSymbol = 0; iSymbol < Rules[iRule]->successor->Count; iSymbol++)
		{
			this->rulesSolution[iRule]->successor->Add(Rules[iRule]->successor[iSymbol]);
		}
	}
}

array<Int32>^ PMIProblemV2A::Cheat(array<Int32>^ Solution)
{
	bool invalid = false;
	this->CreateGenome();

	for (size_t iValue = 0; iValue <= Solution->GetUpperBound(0); iValue++)
	{
		Int32 iRule = this->solution2rule[iValue];

		if (iRule == -1)
		{
			iRule = iValue;
		}

		if (this->knownRuleLengthsIndexed[this->constantIndex, iRule] == PMIProblem::cUnknownRuleLength)
		{
			Symbol^ s = this->vocabulary->Symbols[iRule];

			for (size_t idx = 0; idx < this->model->rules->Count; idx++)
			{
				if (this->model->rules[idx]->predecessor == s)
				{
					Solution[iValue] = this->ComputeActualLength(iRule);
				}
			}
		}
	}

	return Solution;
}

array<Int32, 2>^ PMIProblemV2A::CheatParikh()
{
	array<Int32, 2>^ result = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + this->constantIndex);

	for (size_t iRule = 0; iRule <= result->GetUpperBound(0); iRule++)
	{
		// find the right rule in the model
		for (size_t iModelRule = 0; iModelRule < this->model->rules->Count; iModelRule++)
		{
			if (this->model->rules[iModelRule]->predecessor == this->vocabularyMaster->Symbols[iRule])
			{
				for (size_t iSymbol = 0; iSymbol < this->model->rules[iModelRule]->successor->Count; iSymbol++)
				{
					Int32 symbolIndex = this->vocabularyMaster->FindIndex(this->model->rules[iModelRule]->successor[iSymbol]);
					if (symbolIndex <= result->GetUpperBound(1))
					{
						result[iRule, symbolIndex]++;
					}
				}
			}
		}
	}

	return result;
}

void PMIProblemV2A::CompressEvidence()
{
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		this->vocabularyMaster->CompressSymbols(this->evidence[iGen]);
	}
}

Lexicon^ PMIProblemV2A::CompressMarkerList(Lexicon^ Source, Lexicon^ Production, Int32 CountLowerBound, Int32 CountUpperBound)
{
	Lexicon^ result = gcnew Lexicon(Production->vocabulary, PMIProblemV2A::cMaxMarkerLength, Production->totalPoG);

	// Trim markers
	// 1 - eliminate any marker with a module, unless module is solved
	// 2 - eliminate any marker where the count in source is high
	// 3 - eliminate any marker where the count difference in source and production is large (not done here)
	for (size_t iLength = 0; iLength <= Source->subwordCounts->GetUpperBound(1); iLength++)
	{
		List<SubwordCountType^>^ replacement = gcnew List<SubwordCountType^>;
		for (size_t iWord = 0; iWord < Source->subwordCounts[0, iLength]->Count; iWord++)
		{
			bool allConstants = true;
			bool countsInRange = (Source->subwordCounts[0, iLength][iWord]->count >= CountLowerBound && Source->subwordCounts[0, iLength][iWord]->count <= CountUpperBound);
			Int32 iSymbol = 0;

			while ((allConstants) && (iSymbol < Source->subwordCounts[0, iLength][iWord]->subword->Count))
			{
				bool isModule = this->vocabulary->IsModule(Source->subwordCounts[0, iLength][iWord]->subword[iSymbol]);
				if (isModule)
				{
					Int32 ruleIndex = this->vocabulary->RuleIndex(Source->subwordCounts[0, iLength][iWord]->subword[iSymbol]);

					// if the modules is unsolved then this is a problem
					if ((this->minRuleLengths[ruleIndex] != this->maxRuleLengths[ruleIndex]) || (this->ruleHead[ruleIndex]->Count != this->minRuleLengths[ruleIndex]))
					{
						allConstants = false;
					}
				}
				iSymbol++;
			}

			// Find the source marker in the production lexicon
			if ((allConstants) && (countsInRange))
			{
				Word^ w = this->ConvertWord(Source->subwordCounts[0, iLength][iWord]->subword);
				Int32 idx = Production->FindSubwordCount(w);
				Int32 countDifference = 9999999;
				if (idx >= 0)
				{
					countDifference = (Production->subwordCounts[0, w->Count][idx]->count - Source->subwordCounts[0, iLength][iWord]->count);
				}

				if (countDifference <= PMIProblemV2A::cMaxMarkerFrequencyDifference)
				{
					replacement->Add(Source->subwordCounts[0, iLength][iWord]);
				}
			}
		}

		result->subwordCounts[0, iLength] = replacement;
	}

	return result;
}

// returns a list of words that are unique
List<Word^>^ PMIProblemV2A::CompressWordList(List<Word^>^ W)
{
	List<Word^>^ result = gcnew List<Word^>;

	for (size_t iWord = 0; iWord < W->Count; iWord++)
	{
		bool isUnique = true;
		Int32 idx = 0;

		while ((isUnique) && (idx < result->Count))
		{
			bool isMatch = true;
			
			// if they are the same length they could be the same word
			if (W[iWord]->Count == result[idx]->Count)
			{
				Int32 iSymbol = 0;
				do
				{
					if (W[iWord]->symbols[iSymbol] != result[idx]->symbols[iSymbol])
					{
						isMatch = false;
					}
					iSymbol++;
				} while ((isMatch) && (iSymbol < W[iWord]->Count));
				isUnique = (isUnique && (!isMatch));
			}

			idx++;
		}

		if (isUnique)
		{
			result->Add(W[iWord]);
		}
	}

	return result;
}


bool PMIProblemV2A::ComputeConstants()
{
	List<Symbol^>^ toProcess = gcnew List<Symbol^>;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if ((this->minRuleLengths[iRule] == 1) && (this->maxRuleLengths[iRule] == 1))
		{
			if ((this->minPoG[iRule, iRule] == 1) && (this->maxPoG[iRule, iRule] == 1))
			{
				toProcess->Add(this->vocabulary->Symbols[iRule]);
			}
		}
	}

	for (size_t iSymbol = 0; iSymbol < toProcess->Count; iSymbol++)
	{
		this->ModifyVocabulary(toProcess[iSymbol]);
	}

	return toProcess->Count > 0;
}

List<SymbolFragmentType^>^ PMIProblemV2A::ComputeConstantsFromEqualWords(Word^ W)
{
	List<SymbolFragmentType^>^ result = gcnew List<SymbolFragmentType^>;
	List<Symbol^>^ completed = gcnew List<Symbol^>;

	for (size_t i = 0; i < W->Count; i++)
	{
		if (this->vocabulary->IsModule(W->symbols[i]))
		{
			SymbolFragmentType^ m = gcnew SymbolFragmentType();
			m->s = W->symbols[i];
			m->fragment = gcnew Word();
			m->fragment->Add(W->symbols[i]);
			m->isComplete = true;
			m->isHead = true;
			result->Add(m);

			//Console::WriteLine("Rule #5: " + m->s->ToString() + " => " + m->fragment->ToString());
		}
	}

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 0
#endif

	return result;
}

List<Word^>^ PMIProblemV2A::ComputeEvidenceFromMarkers(List<MarkerMapType^>^ MarkerMap, Word^ E)
{
	List<Word^>^ result = gcnew List<Word^>;

	bool penDown = true;
	//Int32 iMarker = 0;

	if (MarkerMap[0]->start != 0)
	{
		Word^ tmp = gcnew Word();
		for (size_t iSymbol = 0; iSymbol <= MarkerMap[0]->start - 1; iSymbol++)
		{
			tmp->Add(E->symbols[iSymbol]);
		}
		//Console::WriteLine(tmp);
		result->Add(tmp);
	}

	for (size_t iMarker = 0; iMarker < MarkerMap->Count-1; iMarker++)
	{
		Word^ tmp = gcnew Word();

		for (size_t iSymbol = MarkerMap[iMarker]->end+1; iSymbol <= MarkerMap[iMarker+1]->start - 1; iSymbol++)
		{
			tmp->Add(E->symbols[iSymbol]);
		}

		//// special processing for the last marker and the last symbol is a module, basically include it in the subword
		//if ((iMarker == MarkerMap->Count - 2) && (this->vocabulary->IsModule(E->symbols[E->Count-1])))
		//{
		//	tmp->Add(E->symbols[E->Count - 1]);
		//}

		//Console::WriteLine(tmp);
		result->Add(tmp);
	}

	//for (size_t iSymbol = 0; iSymbol < E->Count; iSymbol++)
	//{
	//	// check to see if we should put the pen down
	//	if (iSymbol == MarkerMap[iMarker]->end + 1)
	//	{
	//		iMarker++;
	//		penDown = true;
	//		tmp = gcnew Word();
	//	}

	//	// check to see if we should put the pen up, note this should not be elsed with the preceding if as it contains some special process for the end of word
	//	if ((iSymbol == MarkerMap[iMarker]->start) || (iSymbol == E->Count - 1))
	//	{
	//		// in the special case of the pen down and reached the end of the word, include the last symbol
	//		if ((penDown) && (iSymbol == E->Count - 1))
	//		{
	//			tmp->Add(E->symbols[iSymbol]);
	//		}

	//		penDown = false;
	//		if (tmp->Count > 0)
	//		{
	//			//Console::WriteLine("New Subword = " + tmp);
	//			result->Add(tmp);
	//		}
	//	}

	//	if (penDown)
	//	{
	//		tmp->Add(E->symbols[iSymbol]);
	//	}
	//}

	return result;
}

bool PMIProblemV2A::ComputeFragments(array<Int32, 2>^ Rules)
{
	bool result = true;

	if (!this->IsSymbolAllConstant(this->currentSet->startGeneration))
	{
		Int32 idxScan;
		Int32 iSymbol;

		Int32 iGen = this->currentSet->startGeneration;
		List<Word^>^ filteredEvidence = this->evidenceMaster;
		List<Symbol^>^ completed = gcnew List<Symbol^>;

		do
		{
			idxScan = 0;
			iSymbol = 0;
			bool bypassToModule = false;
			bool previousModuleEmpty = false;
			bool previousActionBuildFragment = false;

			do
			{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
				Console::WriteLine();
				Console::WriteLine("Symbol " + filteredEvidence[iGen]->symbols[iSymbol]->ToString() + "(" + iSymbol + ") : " + filteredEvidence[iGen+1]->symbols[idxScan]->ToString() + "(" + idxScan + ")");
#endif

				// Step 1 - Find a module
				if ((this->vocabulary->IsModule(filteredEvidence[iGen]->symbols[iSymbol])) && (!completed->Contains(filteredEvidence[iGen]->symbols[iSymbol])))
				{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
					Console::WriteLine("Building fragment for symbol " + filteredEvidence[iGen]->symbols[iSymbol]->ToString());
#endif
					completed->Add(filteredEvidence[iGen]->symbols[iSymbol]);
					Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					array<Int32>^ pv;

					Int32 ruleLength = 0;
					//if (!bypassToModule)
					//{
					//	pv = gcnew array<Int32>(Rules->GetUpperBound(1) + 1);
					//	for (size_t iValue = 0; iValue <= Rules->GetUpperBound(1); iValue++)
					//	{
					//		pv[iValue] = Rules[ruleIndex, iValue];
					//		ruleLength += Rules[ruleIndex, iValue];
					//	}
					//}
					//else
					//{
						pv = gcnew array<Int32>(this->vocabulary->numModules);
						for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
						{
							pv[iValue] = Rules[ruleIndex, iValue];
							ruleLength += Rules[ruleIndex, iValue];
						}
					//}

					// advance the idxScan to the next module
					// but only if the module will be producing a module					
					if ((previousModuleEmpty) && (!this->IsParikhAllConstants(pv)))
					{
						while ((idxScan < filteredEvidence[iGen+1]->symbols->Count) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
						{
							idxScan++;
						}
						previousModuleEmpty = false;
					}

					// Step 2 - Scan the evidence and buuld the fragment
					// In the case where the length is set the flag 
					if (ruleLength == 0)
					{
						previousModuleEmpty = true;
					}
					else
					{
						// if the previous action was building a fragment it is possible it stopped short, so advance to the first symbol known to be
						// in the current module
						if (previousActionBuildFragment)
						{
							bool valid = false;
							while (!valid)
							{
								Int32 symbolIndex = this->vocabularyMaster->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);

								if ((symbolIndex <= pv->GetUpperBound(0)) && (pv[symbolIndex] > 0))
								{
									valid = true;
								}
								else
								{
									idxScan++;
								}
							}

						}

						List<Symbol^>^ fragment = gcnew List<Symbol^>;
						while (ruleLength > 0)
						{
							if (idxScan < filteredEvidence[iGen + 1]->symbols->Count)
							{
								fragment->Add(filteredEvidence[iGen + 1]->symbols[idxScan]);
								Int32 symbolIndex = this->vocabularyMaster->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);
								// This is needed because we're scanning the master index
								// But the rules (and hence the PV) were crafted with the local vocabulary
								// Which is smaller than the master
								// We're using the master vocabulary to build the fragment as we want it to be a master fragment
								if (symbolIndex <= pv->GetUpperBound(0))
								{
									pv[symbolIndex]--;
									ruleLength--;
								}
								idxScan++;
							}
						}
						bypassToModule = false;

						// it really should only ever be longer, but just in case
						if (fragment->Count > this->ruleFragments[ruleIndex]->Count)
						{
							Word^ w = gcnew Word(fragment);
							this->vocabularyMaster->CompressSymbols(w);
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1
							Console::WriteLine(this->vocabularyMaster->Symbols[ruleIndex]->ToString() + " => " + w->ToString());
#if _PMI_PROBLEM_BREAKS_ >= 1
							Console::ReadLine();
#endif
#endif
							//if (this->lexiconMaster->VerifyFragment(ruleIndex, w))
							//{
							this->ruleFragmentsMaster[ruleIndex] = w;
							//}
							//else
							//{
							//	Console::WriteLine("Invalid fragment");
							//	//this->lexiconMaster->VerifyFragment(ruleIndex, w);
							//	result = false;
							//}
						}
						previousActionBuildFragment = true;
					}
					iSymbol++;
				}
				else if ((filteredEvidence[iGen]->symbols[iSymbol]->isModule) && (completed->Contains(filteredEvidence[iGen]->symbols[iSymbol])))
				{// if it is a previously seen module advance the index scan until the expected fragment has been found
					// TODO: if the previous symbol and this symbol are the same then any symbols passed belong to the symbol
					previousActionBuildFragment = false;
					Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					// if the length is zero just pass
					bool fragmentPassed = (this->ruleFragmentsMaster[ruleIndex]->symbols->Count == 0);

					while (!fragmentPassed)
					{
						// 1st - Find a potential start of the fragment
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1
						Console::WriteLine();
						Console::WriteLine("Advancing idxScan (" + idxScan + ") to known fragment (" + this->ruleFragmentsMaster[ruleIndex]->ToString() + ") start");
#if _PMI_PROBLEM_BREAKS_ >= 1
						Console::ReadLine();
#endif
#endif
						Int32 startFragment = 0;

						// If a bypass has occured landing on a module, advance the fragment to the first module
						if (bypassToModule)
						{
							while ((startFragment < this->ruleFragmentsMaster[ruleIndex]->symbols->Count) && (!this->vocabulary->IsModule(this->ruleFragmentsMaster[ruleIndex]->symbols[startFragment])))
							{
								startFragment++;
							}
							bypassToModule = false;
						}
						else
						{
							while ((result) && (filteredEvidence[iGen + 1]->symbols[idxScan] != this->ruleFragmentsMaster[ruleIndex]->symbols[startFragment]))
							{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
								Console::Write(filteredEvidence[iGen + 1]->symbols[idxScan]);
#endif
								idxScan++;
								if (idxScan >= filteredEvidence[iGen + 1]->symbols->Count)
								{
									result = false;
								}
							}
						}

						// 2nd - Advance the index scan and make sure each symbol is correct
						Int32 lengthPassed = startFragment;
						bool failed = false;

#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 2
						Console::WriteLine();
						Console::WriteLine("Scanning to find and pass fragment");
#if _PMI_PROBLEM_BREAKS_ >= 1
						Console::ReadLine();
#endif
#endif
						Int32 tmp = idxScan;
						while ((result) && (!failed) && (lengthPassed < this->ruleFragmentsMaster[ruleIndex]->symbols->Count))
						{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
							Console::Write(filteredEvidence[iGen + 1]->symbols[tmp]);
#endif
							if (filteredEvidence[iGen + 1]->symbols[tmp] != this->ruleFragmentsMaster[ruleIndex]->symbols[lengthPassed])
							{
								failed = true;
							}
							else
							{
								tmp++;
								lengthPassed++;

								if (tmp >= filteredEvidence[iGen + 1]->symbols->Count)
								{
									result = false;
								}
							}
						}

						if (!failed)
						{
							idxScan = tmp;
							fragmentPassed = true;
#if _PMI_PROBLEM_BREAKS_ >= 1
							Console::ReadLine();
#endif
						}
						else
						{
							idxScan++;
						}
					}

					//Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					//idxScan += this->ruleFragmentsMaster[ruleIndex]->Count;
					iSymbol++;
				}
				else
				{
					previousActionBuildFragment = false;
					// Under no cirumstance is it permitted to advanced past a module
					// if constant, then advance index scan until the constant is found
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
					Console::WriteLine("Look for next instance of constant " + filteredEvidence[iGen]->symbols[iSymbol]->ToString() + "(" + iSymbol + ")");
#endif
					// TODO: stopping the can when it runs of symbols because I think it cannot desync when there's 
					// still modules left... might need to add a "last module position" and stop it there
					while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen]->symbols[iSymbol] != filteredEvidence[iGen + 1]->symbols[idxScan]) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
					{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
						Console::Write(filteredEvidence[iGen + 1]->symbols[idxScan]);
#endif
						idxScan++;
					};
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
					Console::WriteLine();
#endif

					if ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
					{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
						Console::WriteLine("Look for last instance of constant");
#endif
						Int32 tmp = idxScan + 1;
						// then advance index scan until the constant is not found
						while ((idxScan < filteredEvidence[iGen+1]->symbols->Count) && (tmp < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen + 1]->symbols[idxScan] == filteredEvidence[iGen + 1]->symbols[tmp]) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[tmp])))
						{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 3
							Console::Write(filteredEvidence[iGen + 1]->symbols[tmp]);
#endif
							tmp++;
						};
						idxScan = tmp;
					}
					else
					{
						bypassToModule = true;
					}

					iSymbol++;
				}
				// TODO: stopping processing when the idxScan is past the symbol count
				// I think this can only happen when the constants have desynced and there's no more modules
				// adding a little check for the moment to prove that's true
				// Might need to add a "stop when there's no more modules"
			} while ((result) && (idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (iSymbol < filteredEvidence[iGen]->symbols->Count) && (completed->Count < this->vocabularyMaster->numModules));

			if (completed->Count < this->vocabularyMaster->numModules)
			{
				for (size_t i = iSymbol; i < filteredEvidence[iGen]->symbols->Count; i++)
				{
					if (this->vocabulary->IsModule(filteredEvidence[iGen]->symbols[i]))
					{
						Console::WriteLine("Uh oh");
					}
				}
			}

			iGen++;
		} while ((result) && (iGen < this->currentSet->endGeneration) && (completed->Count < this->vocabularyMaster->numModules));

		if (!result)
		{
			this->flagAnalysisFailed = true;
		}
	}

	return result;
}

bool PMIProblemV2A::ComputeFragments_v2(array<Int32, 2>^ Rules)
{
	bool result = true;

	if (!this->IsSymbolAllConstant(this->currentSet->startGeneration))
	{
		Int32 idxScan;
		Int32 iSymbol;

		Int32 iGen = this->currentSet->startGeneration;
		List<Word^>^ filteredEvidence = this->evidenceMaster;
		List<Symbol^>^ completed = gcnew List<Symbol^>;

		do
		{
#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1
			Console::WriteLine("Starting Generation " + iGen);
#endif

			idxScan = 0;
			iSymbol = 0;
			bool bypassToModule = false;
			bool previousModuleEmpty = false;
			bool previousActionBuildFragment = false;

			do
			{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
				Console::WriteLine();
				Console::WriteLine("Symbol " + filteredEvidence[iGen]->symbols[iSymbol]->ToString() + "(" + iSymbol + ") : " + filteredEvidence[iGen + 1]->symbols[idxScan]->ToString() + "(" + idxScan + ")");
#endif

				// Step 1 - Find a module
				if ((this->vocabulary->IsModule(filteredEvidence[iGen]->symbols[iSymbol])) && (!completed->Contains(filteredEvidence[iGen]->symbols[iSymbol])))
				{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 3
					Console::WriteLine("Building fragment for symbol " + filteredEvidence[iGen]->symbols[iSymbol]->ToString());
#endif
					completed->Add(filteredEvidence[iGen]->symbols[iSymbol]);
					Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					array<Int32>^ pv;

					Int32 ruleLength = 0;
					//if (!bypassToModule)
					//{
					//	pv = gcnew array<Int32>(Rules->GetUpperBound(1) + 1);
					//	for (size_t iValue = 0; iValue <= Rules->GetUpperBound(1); iValue++)
					//	{
					//		pv[iValue] = Rules[ruleIndex, iValue];
					//		ruleLength += Rules[ruleIndex, iValue];
					//	}
					//}
					//else
					//{
					pv = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);
					for (size_t iValue = 0; iValue < this->vocabulary->numModules + this->constantIndex; iValue++)
					{
						pv[iValue] = Rules[ruleIndex, iValue];
						ruleLength += Rules[ruleIndex, iValue];
					}
					//}

					// advance the idxScan to the next module
					// but only if the module will be producing a module					
					if ((previousModuleEmpty) && (!this->IsParikhAllConstants(pv)))
					{
						//while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
						while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) 
							&& ((this->vocabulary->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]) > 0) &&
							    (pv[this->vocabulary->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan])] == 0)))
						{
							idxScan++;
						}
						previousModuleEmpty = false;
					}

					// Step 2 - Scan the evidence and build the fragment
					// In the case where the length is set the flag 
					if (ruleLength == 0)
					{
						previousModuleEmpty = true;
					}
					else
					{
						// if the previous action was building a fragment it is possible it stopped short, so advance to the first symbol known to be
						// in the current module
						if (previousActionBuildFragment)
						{
							bool valid = false;
							while (!valid)
							{
								Int32 symbolIndex = this->vocabularyMaster->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);

								if ((symbolIndex <= pv->GetUpperBound(0)) && (pv[symbolIndex] > 0))
								{
									valid = true;
								}
								else
								{
									idxScan++;
								}
							}
						}

						bool misaligned = false;
						//// if there is a partial solution and
						//// if the first symbol is a constant and currently misaligned
						//// then roll back the idxScan to that constant
						//// NOTE: misalignment should only occur when the solution is invalid, so this process should always fail
						//if ((this->rulesSolution[ruleIndex]->successor->Count > 0) && 
						//	((this->vocabulary->IsConstant(this->rulesSolution[ruleIndex]->successor[0]) && (this->rulesSolution[ruleIndex]->successor[0] != filteredEvidence[iGen + 1]->symbols[idxScan])))
						//	)
						//{
						//	// Look forward through the successor and find the first module
						//	// While scanning forward count the number of times the first symbol is passed
						//	// Must rollback that many copies of the symbol
						//	Int32 count = 0;
						//	Int32 iSymbol = 0;
						//	do
						//	{
						//		if (this->rulesSolution[ruleIndex]->successor[iSymbol] == this->rulesSolution[ruleIndex]->successor[0])
						//		{
						//			count++;
						//			iSymbol++;
						//		}
						//	} while ((iSymbol < this->rulesSolution[ruleIndex]->successor->Count) && (!this->vocabulary->IsModule(this->rulesSolution[ruleIndex]->successor[iSymbol])));

						//	while ((idxScan > 0) && (count > 0))
						//	{
						//		if (filteredEvidence[iGen + 1]->symbols[idxScan] == this->rulesSolution[ruleIndex]->successor[0])
						//		{
						//			count--;
						//		}
						//		idxScan--;
						//	}

						//	if (count > 0)
						//	{
						//		Console::WriteLine("Misalignment detected");
						//		misaligned = true;
						//		result = false;
						//	}
						//}
					 
						if (!misaligned)
						{
							List<Symbol^>^ fragment = gcnew List<Symbol^>;
							while (ruleLength > 0)
							{
								if (idxScan < filteredEvidence[iGen + 1]->symbols->Count)
								{
									fragment->Add(filteredEvidence[iGen + 1]->symbols[idxScan]);
									Int32 symbolIndex = this->vocabularyMaster->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);
									// This is needed because we're scanning the master index
									// But the rules (and hence the PV) were crafted with the local vocabulary
									// Which is smaller than the master
									// We're using the master vocabulary to build the fragment as we want it to be a master fragment
									if (symbolIndex <= pv->GetUpperBound(0))
									{
										pv[symbolIndex]--;
										ruleLength--;
									}
									idxScan++;
								}
							}
							bypassToModule = false;

							// it really should only ever be longer, but just in case
							if (fragment->Count > this->ruleFragments[ruleIndex]->Count)
							{
								Word^ w = gcnew Word(fragment);
								this->vocabularyMaster->CompressSymbols(w);
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 1
								Console::WriteLine(this->vocabularyMaster->Symbols[ruleIndex]->ToString() + " => " + w->ToString());
#if _PMI_PROBLEM_BREAKS_ >= 1
								if (this->constantIndex >= 0)
								{
									Console::ReadLine();
								}
#endif
#endif
								//if (this->lexiconMaster->VerifyFragment(ruleIndex, w))
								//{
								this->ruleFragmentsMaster[ruleIndex] = w;
								//}
								//else
								//{
								//	Console::WriteLine("Invalid fragment");
								//	//this->lexiconMaster->VerifyFragment(ruleIndex, w);
								//	result = false;
								//}
							}
							previousActionBuildFragment = true;
						}
					}
					iSymbol++;
				}
				else if ((filteredEvidence[iGen]->symbols[iSymbol]->isModule) && (completed->Contains(filteredEvidence[iGen]->symbols[iSymbol])))
				{// if it is a previously seen module advance the index scan until the expected fragment has been found
				 // TODO: if the previous symbol and this symbol are the same then any symbols passed belong to the symbol
					previousActionBuildFragment = false;
					Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					// if the length is zero just pass
					bool fragmentPassed = (this->ruleFragmentsMaster[ruleIndex]->symbols->Count == 0);

					while (!fragmentPassed)
					{
						// 1st - Find a potential start of the fragment
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
						Console::WriteLine();
						Console::WriteLine("Advancing idxScan (" + idxScan + ") to known fragment (" + this->ruleFragmentsMaster[ruleIndex]->ToString() + ") start");
#if _PMI_PROBLEM_BREAKS_ >= 1
						Console::ReadLine();
#endif
#endif
						Int32 startFragment = 0;

						// If a bypass has occured landing on a module, advance the fragment to the first module
						if (bypassToModule)
						{
							while ((startFragment < this->ruleFragmentsMaster[ruleIndex]->symbols->Count) && (!this->vocabulary->IsModule(this->ruleFragmentsMaster[ruleIndex]->symbols[startFragment])))
							{
								startFragment++;
							}
							bypassToModule = false;
						}
						else
						{
							while ((result) && (filteredEvidence[iGen + 1]->symbols[idxScan] != this->ruleFragmentsMaster[ruleIndex]->symbols[startFragment]))
							{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
								Console::Write(filteredEvidence[iGen + 1]->symbols[idxScan]);
#endif
								idxScan++;
								if (idxScan >= filteredEvidence[iGen + 1]->symbols->Count)
								{
									result = false;
								}
							}
						}

						// 2nd - Advance the index scan and make sure each symbol is correct
						Int32 lengthPassed = startFragment;
						bool failed = false;

#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
						Console::WriteLine();
						Console::WriteLine("Scanning to find and pass fragment");
#if _PMI_PROBLEM_BREAKS_ >= 1
						Console::ReadLine();
#endif
#endif
						Int32 tmp = idxScan;
						while ((result) && (!failed) && (lengthPassed < this->ruleFragmentsMaster[ruleIndex]->symbols->Count))
						{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
							Console::Write(filteredEvidence[iGen + 1]->symbols[tmp]);
#endif
							if (filteredEvidence[iGen + 1]->symbols[tmp] != this->ruleFragmentsMaster[ruleIndex]->symbols[lengthPassed])
							{
								failed = true;
							}
							else
							{
								tmp++;
								lengthPassed++;

								if (tmp >= filteredEvidence[iGen + 1]->symbols->Count)
								{
									result = false;
								}
							}
						}

						if (!failed)
						{
							idxScan = tmp;
							fragmentPassed = true;
#if _PMI_PROBLEM_BREAKS_ >= 1
							Console::ReadLine();
#endif
						}
						else
						{
							idxScan++;
						}
					}

					//Int32 ruleIndex = this->vocabularyMaster->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
					//idxScan += this->ruleFragmentsMaster[ruleIndex]->Count;
					iSymbol++;
				}
				else
				{
					previousActionBuildFragment = false;
					// Under no cirumstance is it permitted to advance past a module
					// if constant, then advance index scan until the constant is found
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
					Console::WriteLine("Look for next instance of constant " + filteredEvidence[iGen]->symbols[iSymbol]->ToString() + "(" + iSymbol + ")");
#endif
					// if the symbols match simply advance by 1
					// TODO: might need to make this only if they match AND are contained in the vocabulary
					if (filteredEvidence[iGen + 1]->symbols[idxScan] == filteredEvidence[iGen]->symbols[iSymbol])
					{
						idxScan++;
					}
					else
					{

						// changed from IsModule to vocabulary contains, basically it is not allowed to advance past 
						// a symbol in the alphabet with new fragment finder unless
						// the symbols match
						//while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen]->symbols[iSymbol] != filteredEvidence[iGen + 1]->symbols[idxScan]) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
						while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen]->symbols[iSymbol] != filteredEvidence[iGen + 1]->symbols[idxScan]) && (!this->vocabulary->Contains(filteredEvidence[iGen + 1]->symbols[idxScan])))
						{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
							Console::Write(filteredEvidence[iGen + 1]->symbols[idxScan]);
#endif
							idxScan++;
						};
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
						Console::WriteLine();
#endif

						//if ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan])))
						if ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (!this->vocabulary->Contains(filteredEvidence[iGen + 1]->symbols[idxScan])))
						{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
							Console::WriteLine("Look for last instance of constant " + filteredEvidence[iGen]->symbols[iSymbol]->ToString());
#endif
							Int32 tmp = idxScan + 1;
							// then advance index scan until the constant is not found
							//while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (tmp < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen + 1]->symbols[idxScan] == filteredEvidence[iGen + 1]->symbols[tmp]) && (!this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[tmp])))
							while ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (tmp < filteredEvidence[iGen + 1]->symbols->Count) && (filteredEvidence[iGen + 1]->symbols[idxScan] == filteredEvidence[iGen + 1]->symbols[tmp]) && (!this->vocabulary->Contains(filteredEvidence[iGen + 1]->symbols[tmp])))
							{
#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 2
								Console::Write(filteredEvidence[iGen + 1]->symbols[tmp]);
#endif
								tmp++;
							};
							idxScan = tmp;
						}
						else if (this->vocabulary->IsModule(filteredEvidence[iGen + 1]->symbols[idxScan]))
						{
							bypassToModule = true;
						}
					}
	
					// if the symbol is a member of the alphabet than advance past it as they should be in aligned
					if ((idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (this->vocabulary->Contains(filteredEvidence[iGen + 1]->symbols[idxScan])) && (filteredEvidence[iGen + 1]->symbols[idxScan] == filteredEvidence[iGen]->symbols[iSymbol]))
					{
						idxScan++;
					}


#if _PMI_PROBLEM_BREAKS_ >= 1
					Console::ReadLine();
#endif
					iSymbol++;
				}
				// TODO: stopping processing when the idxScan is past the symbol count
				// I think this can only happen when the constants have desynced and there's no more modules
				// adding a little check for the moment to prove that's true
				// Might need to add a "stop when there's no more modules"
				// TODO: note misalignment can now be detected during fragment generation for invalid solutions only
				// so be careful if making any changes here. There could be modules left after a detected misalignment
			} while ((result) && (idxScan < filteredEvidence[iGen + 1]->symbols->Count) && (iSymbol < filteredEvidence[iGen]->symbols->Count) && (completed->Count < this->vocabularyMaster->numModules));

			//if (completed->Count < this->vocabularyMaster->numModules)
			//{
			//	for (size_t i = iSymbol; i < filteredEvidence[iGen]->symbols->Count; i++)
			//	{
			//		if (this->vocabulary->IsModule(filteredEvidence[iGen]->symbols[i]))
			//		{
			//			Console::WriteLine("Uh oh");
			//		}
			//	}
			//}

			iGen++;
		} while ((result) && (iGen < this->currentSet->endGeneration) && (completed->Count < this->vocabularyMaster->numModules));

		if (!result)
		{
			this->flagAnalysisFailed = true;
		}
	}

	return result;
}

void PMIProblemV2A::ComputeFragmentsFromEvidence()
{
#if _PMI_PROBLEM_FRAGMENTS_LOAD_ > 0
	if (!this->LoadFragments())
	{
#endif
		if (this->vocabularyMaster->numConstants >= 2)
		{
			if ((this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules]->Value() == "]") && (this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 1]->Value() == "["))
			{
				for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
				{
					// check to make sure at least one module in this generation
					if (!this->IsGenerationAllConstants(iGen))
					{
						this->ComputeFragmentsUsingSymbolMarkers(this->evidenceMaster[iGen], this->evidenceMaster[iGen + 1], this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 1], this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules]);
					}
				}
			}
		}

		if (this->vocabularyMaster->numConstants >= 4)
		{
			if ((this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 2]->Value() == "}") && (this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 3]->Value() == "{"))
			{
				for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
				{
					if (!this->IsGenerationAllConstants(iGen))
					{
						this->ComputeFragmentsUsingSymbolMarkers(this->evidenceMaster[iGen], this->evidenceMaster[iGen + 1], this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 3], this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + 2]);
					}
				}
			}
		}

		if (this->vocabularyMaster->numConstants >= 1)
		{
			//for (size_t iGen = 0; iGen < 8; iGen++)
			for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
			{
				if (!this->IsGenerationAllConstants(iGen))
				{
					if ((this->evidence[iGen]->Count < 20000) && (this->evidence[iGen + 1]->Count < 40000))
					{
						this->ComputeFragmentsUsingMarkerMap(this->evidence[iGen], this->evidence[iGen + 1]);
					}
				}
			}
		}
#if _PMI_PROBLEM_FRAGMENTS_LOAD_ > 0
	}
#endif
}

// Must be override by algorithms that make absolute rules such as PMIT-D
void PMIProblemV2A::ComputeFragmentsFromPartialSolution()
{
}

List<SymbolFragmentType^>^ PMIProblemV2A::ComputeMaxFragmentsFromEvidence(Word^ E1, Word^ E2)
{
	List<SymbolFragmentType^>^ result = gcnew List<SymbolFragmentType^>;

	array<Int32>^ moduleCounts = gcnew array<Int32>(this->vocabularyMaster->numModules);
	Int32 count = 0;
	for (size_t iSymbol = 0; iSymbol < E1->symbols->Count; iSymbol++)
	{
		if (this->vocabularyMaster->IsModule(E1->symbols[iSymbol]))
		{
			Int32 ruleIndex = this->vocabularyMaster->RuleIndex(E1->symbols[iSymbol]);
			moduleCounts[ruleIndex]++;
			count++;
		}
	}

	if (count > 0)
	{
		// Strip lead and trailing constants
		this->StripConstants(E1, E2);

		if (count == 1)
		{
			SymbolFragmentType^ sol = gcnew SymbolFragmentType();

			sol->s = E1->symbols[0];
			sol->fragment = E2;
			sol->isHead = true;
			sol->isComplete = true;

			result->Add(sol);
		}
		else
		{
			this->vocabulary->CompressSymbols(E1);
			this->vocabulary->CompressSymbols(E2);
			result->Add(this->ComputeMaxHead(E1, E2));
			result->Add(this->ComputeMaxTail(E1, E2));
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0
			if (count > 2)
			{
				result->AddRange(this->ComputeMaxFragment(E1, E2));
			}
#endif // _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0
			// TODO: build a lexicon from the production, if word count == module count possible solution, if unique at that count it is the solution
		}
	}

	return result;
}

List<SymbolFragmentType^>^ PMIProblemV2A::ComputeMinFragmentsFromEvidence(Word^ E1, Word^ E2)
{
	List<SymbolFragmentType^>^ result = gcnew List<SymbolFragmentType^>;

	// get the number of modules in the source (E1)
	Int32 count = 0;
	for (size_t iSymbol = 0; iSymbol < E1->symbols->Count; iSymbol++)
	{
		if (this->vocabularyMaster->IsModule(E1->symbols[iSymbol]))
		{
			count++;
		}
	}

	if (count > 0)
	{
		// Strip lead and trailing constants
		this->StripConstants(E1, E2);

		if (count == 1)
		{
			SymbolFragmentType^ sol = gcnew SymbolFragmentType();

			sol->s = E1->symbols[0];
			sol->fragment = E2;
			sol->isHead = true;
			sol->isComplete = true;

			result->Add(sol);
		}
		else
		{
			result->Add(this->ComputeMinHead(E1, E2));
			result->Add(this->ComputeMinTail(E1, E2));
		}
	}

#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 2
	for (size_t iFragment = 0; iFragment < result->Count; iFragment++)
	{
		Console::WriteLine(result[iFragment]->s->ToString() + " => " + result[iFragment]->fragment->ToString() + " (" + result[iFragment]->isComplete + ")");
	}
#endif

	return result;
}

//void PMIProblemV2A::ComputeFragmentsByScanWithMarkers(Word^ W0, Word^ W1, Lexicon^ Markers)
//{
//
//}

void PMIProblemV2A::ComputeFragmentsUsingMarkerMap(Word^ E0, Word^ E1)
{
	Word^ W0 = gcnew Word(E0);
	Word^ W1 = gcnew Word(E1);
	bool shouldProcess = false;
	Int32 iSymbol = 0;

	while ((!shouldProcess) && (iSymbol < E0->Count))
	{
		if (this->vocabulary->IsModule(E0[iSymbol]))
		{
			Int32 ruleIndex = this->vocabulary->FindIndex(E0[iSymbol]);

			if (this->minRuleLengths[ruleIndex] != this->maxRuleLengths[ruleIndex])
			{
				shouldProcess = true;
			}
		}
		iSymbol++;
	}

	if (shouldProcess)
	{
		this->AnalyzeMinFragments(this->ComputeMinFragmentsFromEvidence(W0, W1));
		this->AnalyzeMaxFragments(this->ComputeMaxFragmentsFromEvidence(W0, W1));

		Lexicon^ sourceMarkers;
		Lexicon^ productionMarkers;
		Lexicon^ markers;

		sourceMarkers = this->ComputeLexiconForWord(E0, PMIProblemV2A::cMaxMarkerLength);
		productionMarkers = this->ComputeLexiconForWord(E1, PMIProblemV2A::cMaxMarkerLength);

		this->tracker++;
		markers = this->CompressMarkerList(sourceMarkers, productionMarkers, PMIProblemV2A::cMinMarkerFrequency, PMIProblemV2A::cMaxMarkerFrequency);

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
		Console::WriteLine("Tracker = " + this->tracker);
		Console::WriteLine();
		Console::WriteLine("Source    : " + E0->ToString());
		Console::WriteLine("Production: " + E1->ToString());
#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 2
		Console::WriteLine("Candidate Markers");
		markers->DisplayCounts();
#endif
#endif

		// Convert the marker lexicon into a map
		List<MarkerMapType^>^ sourceMarkerMap = this->ComputeMarkerMap(markers, E0);
		List<MarkerMapType^>^ convertedMarkerMap = this->ConvertMarkerMap(sourceMarkerMap); // converts any modules into their successor
		List<MarkerMapType^>^ productionMarkerMap = this->ComputeMarkerMap(convertedMarkerMap, E1);

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
		for (size_t iMarker = 0; iMarker < sourceMarkerMap->Count; iMarker++)
		{
			Console::WriteLine("Tracker ID#" + this->tracker + " " + sourceMarkerMap[iMarker]->start + " " + sourceMarkerMap[iMarker]->end + " : " + productionMarkerMap[iMarker]->start + " " + productionMarkerMap[iMarker]->end);
		}
#endif

		bool shouldTerminate = false;

		shouldTerminate = (sourceMarkerMap->Count <= 1);

		if (!shouldTerminate)
		{
			// Convert marker map into subwords
			List<Word^>^ sourceSubwords = this->ComputeEvidenceFromMarkers(sourceMarkerMap, E0);
			List<Word^>^ productionSubwords = this->ComputeEvidenceFromMarkers(productionMarkerMap, E1);

			for (size_t iWord = 0; iWord < sourceSubwords->Count; iWord++)
			{
#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
				Console::WriteLine("Source Subword #" + iWord + " : " + sourceSubwords[iWord]->ToString());
				Console::WriteLine("Corresponding Production #" + iWord + " : " + productionSubwords[iWord]->ToString());
#endif
				if (!this->IsWordAllConstants(sourceSubwords[iWord]))
				{
					this->ComputeFragmentsUsingMarkerMap(sourceSubwords[iWord], productionSubwords[iWord]);
				}
			}
		}
	}
}

void PMIProblemV2A::ComputeFragmentsUsingSymbolMarkers(Word^ W0, Word^ W1, Symbol^ Left, Symbol^ Right)
{
	List<Word^>^ sources = gcnew List<Word^>;
	List<Word^>^ productions = gcnew List<Word^>;
	List<Word^>^ inProgress = gcnew List<Word^>;

	// find all sources
	Word^ e0 = gcnew Word(W0);
	Word^ e1 = gcnew Word(W1);

	bool accepted = true;

	this->tracker++;
	// Find all sources
	//Console::Write("E0: ");
	for (size_t iSymbol	= 0; iSymbol < e0->symbols->Count; iSymbol++)
	{
		//Console::Write(e0->symbols[iSymbol]->ToString());

		// start a new source
		if (e0->symbols[iSymbol] == Left)
		{
			Word^ source = gcnew Word;
			inProgress->Add(source);
		}

		for (size_t iFragment = 0; iFragment < inProgress->Count; iFragment++)
		{
			inProgress[iFragment]->Add(e0->symbols[iSymbol]);
		}

		// stop the last source
		if (e0->symbols[iSymbol] == Right)
		{
			if (inProgress->Count == 1)
			{
				sources->Add(inProgress[inProgress->Count - 1]);
			}
			inProgress->RemoveAt(inProgress->Count - 1);
		}
	}

	//Console::WriteLine();

	// Find all productions
	//Console::Write("E1: ");
	for (size_t iSymbol = 0; iSymbol < e1->symbols->Count; iSymbol++)
	{
		//Console::Write(e1->symbols[iSymbol]->ToString());

		// start a new source
		if (e1->symbols[iSymbol] == Left)
		{
			Word^ production = gcnew Word;
			inProgress->Add(production);
		}

		for (size_t iFragment = 0; iFragment < inProgress->Count; iFragment++)
		{
			inProgress[iFragment]->Add(e1->symbols[iSymbol]);
		}

		// stop the last source
		if (e1->symbols[iSymbol] == Right)
		{
			// Check to see if it is unique
			if (inProgress->Count == 1)
			{
				productions->Add(inProgress[inProgress->Count - 1]);
			}
			inProgress->RemoveAt(inProgress->Count - 1);
		}
	}

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
	Console::WriteLine();

	Console::WriteLine("Tracker ID#" + this->tracker);
	for (size_t iSource = 0; iSource < sources->Count; iSource++)
	{
		Console::WriteLine("Source     #" + iSource + ": " + sources[iSource]);
	}

	for (size_t iProduction = 0; iProduction < productions->Count; iProduction++)
	{
		Console::WriteLine("Production #" + iProduction + ": " + productions[iProduction]);
	}
#endif

	if (sources->Count == 0)
	{// problem has been reduced to a word with no markers so just analyze
		this->AnalyzeMinFragments(this->ComputeMinFragmentsFromEvidence(e0, e1));
		this->AnalyzeMaxFragments(this->ComputeMaxFragmentsFromEvidence(e0, e1));
	}
	else
	{
		// Apply Rule #2 : Check if all preceding or following symbols are constants or accounted for
		/*Rule #3: Match based on order when preceding / following constants.If all of the symbol before a marker are constants
		then it can be matched in order of occurance

		Example #1: Preceding constants

		++++++[x]abc
		++++++[y]...

		x = > y

		Example #2: Following constants
		....abc[x]++ +
		...abc[y]++ +

		x = > y
		*/

		// 1. Check symbols until a left marker is found or a module
		Int32 iSymbol = 0;
		bool markerFound = false;
		bool moduleFound = false;

		while ((!markerFound) && (!moduleFound) && (iSymbol < e0->symbols->Count))
		{
			if (e0->symbols[iSymbol]->Value() == Left->Value())
			{
				markerFound = true;
			}
			else if (!this->vocabulary->IsConstant(e0->symbols[iSymbol]))
			{
				moduleFound = true;
			}
			else
			{
				iSymbol++;
			}
		}

		if (markerFound)
		{
			// 2. If left marker is found, find the closing marker
			// Build the predecessor and successor subwords at the same time.
			Word^ predecessor = gcnew Word();
			Word^ successor = gcnew Word();

			Int32 count = 1; // keeps a count of the open left markers
			iSymbol++;
			Int32 tmp = iSymbol;

			// Build the predecessor
			while (count != 0)
			{
				predecessor->Add(e0->symbols[iSymbol]);

				// Add the current symbol to the predecessor and successor
				if (e0->symbols[iSymbol]->Value() == Left->Value())
				{
					count++;
				}
				else if (e0->symbols[iSymbol]->Value() == Right->Value())
				{
					count--;
				}

				iSymbol++;
			}

			count = 1;
			iSymbol = tmp;
			// Build the predecessor
			while (count != 0)
			{
				successor->Add(e1->symbols[iSymbol]);

				// Add the current symbol to the predecessor and successor
				if (e1->symbols[iSymbol]->Value() == Left->Value())
				{
					count++;
				}
				else if (e1->symbols[iSymbol]->Value() == Right->Value())
				{
					count--;
				}

				iSymbol++;
			}

			// remove the last symbols as that will be the closing marker
			predecessor->symbols->RemoveAt(predecessor->symbols->Count - 1);
			successor->symbols->RemoveAt(successor->symbols->Count - 1);
#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
			Console::WriteLine("Rule 2: " + predecessor->ToString() + " =>" + successor->ToString());
#endif // _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
			this->ComputeFragmentsUsingSymbolMarkers(predecessor, successor, Left, Right);
		}

		// Apply Rule #1: Match if the number of source/productions is the same
		if (sources->Count == productions->Count)
		{
			for (size_t iEvidence = 0; iEvidence < sources->Count; iEvidence++)
			{
				this->AnalyzeMinFragments(this->ComputeMinFragmentsFromEvidence(sources[iEvidence], productions[iEvidence]));
				this->AnalyzeMaxFragments(this->ComputeMaxFragmentsFromEvidence(sources[iEvidence], productions[iEvidence]));
			}
		}
		else
		{
			sources = this->CompressWordList(sources);
			productions = this->CompressWordList(productions);

			// Apply Rule #3 : Check for unique markers on head and tail
			for (size_t iSource = 0; iSource < sources->Count; iSource++)
			{
#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
				Console::WriteLine("S: " + sources[iSource]->ToString());
#endif
				List<Word^>^ candidates = gcnew List<Word^>;

				for (size_t iProduction = 0; iProduction < productions->Count; iProduction++)
				{
#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
					Console::WriteLine("P: " + productions[iProduction]->ToString());
#endif				// check head
					Int32 iSymbol = 0;
					bool isMatch = true;

					// production cannot be shorter than source
					if (sources[iSource]->Count <= productions[iProduction]->Count)
					{
						while ((isMatch) && (iSymbol < sources[iSource]->Count) && (iSymbol < productions[iProduction]->Count) && (!this->vocabularyMaster->IsModule(sources[iSource]->symbols[iSymbol])))
						{
							//Console::Write(sources[iSource]->symbols[iSymbol]);
							if (sources[iSource]->symbols[iSymbol] != productions[iProduction]->symbols[iSymbol])
							{
								isMatch = false;
							}
							iSymbol++;
						}

						//Console::WriteLine();

						if (isMatch)
						{
							// check tail
							iSymbol = 1;
							while ((isMatch) && (iSymbol < sources[iSource]->Count) && (iSymbol < productions[iProduction]->Count) && (!this->vocabularyMaster->IsModule(sources[iSource]->symbols[sources[iSource]->Count - iSymbol])))
							{
								//Console::Write(sources[iSource]->symbols[sources[iSource]->Count - iSymbol]);
								if (sources[iSource]->symbols[sources[iSource]->Count - iSymbol] != productions[iProduction]->symbols[productions[iProduction]->Count - iSymbol])
								{
									isMatch = false;
								}
								iSymbol++;
							}

							//Console::WriteLine();
						}

						if (isMatch)
						{
							// Check to make sure that the source and production are not identical
							if (!sources[iSource]->IsMatch(productions[iProduction]))
							{
								//Console::WriteLine("Matched");
								candidates->Add(productions[iProduction]);
							}
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ == 0
							else
							{
								candidates->Add(productions[iProduction]);
							}
#endif
						}
					}
				}

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
				for (size_t iCandidate = 0; iCandidate < candidates->Count; iCandidate++)
				{
					Console::WriteLine("C: " + candidates[iCandidate]->ToString());
				}
#endif
				if (candidates->Count == 1)
				{
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ == 0
					if (sources[iSource]->IsMatch(candidates[0]))
					{
						this->AnalyzeMinFragments(this->ComputeConstantsFromEqualWords(sources[iSource]));
					}
					else
					{
						//Console::WriteLine(sources[iSource]->ToString() + " " + candidates[0]->ToString());
						this->ComputeFragmentsUsingSymbolMarkers(sources[iSource], candidates[0], Left, Right);
						List<SymbolFragmentType^>^ fragments;
						fragments = this->ComputeMinFragmentsFromEvidence(sources[iSource], candidates[0]);
						this->AnalyzeMinFragments(fragments);
						fragments = this->ComputeMaxFragmentsFromEvidence(sources[iSource], candidates[0]);
						this->AnalyzeMaxFragments(fragments);
					}
#else
					//Console::WriteLine(sources[iSource]->ToString() + " " + candidates[0]->ToString());
					this->ComputeFragmentsUsingSymbolMarkers(sources[iSource], candidates[0], Left, Right);
					List<SymbolFragmentType^>^ fragments;
					fragments = this->ComputeMinFragmentsFromEvidence(sources[iSource], candidates[0]);
					this->AnalyzeMinFragments(fragments);
					fragments = this->ComputeMaxFragmentsFromEvidence(sources[iSource], candidates[0]);
					this->AnalyzeMaxFragments(fragments);
#endif
				}
			}


			// Apply Rule #4 : Check for sets of constants that are unique markers next to module e.g. ))))U///////[!!
			// Recursive call with these markers
			// This is now in function ComputeFragmentsUsingMarkerMap
		}
	}
}

bool PMIProblemV2A::ComputeFragmentsFromRPT()
{
	return this->ComputeFragments_v2(this->CreateParikhRuleFromRPT());
}

void PMIProblemV2A::ComputeLexicon()
{
	this->lexiconMaster = gcnew Lexicon(this->generations, this->vocabulary, this->minRuleLengths, this->maxRuleLengths, this->ruleHead, this->ruleTail, this->ruleFragments, this->totalPoG, this->minPoG, this->maxPoG);

#if _PMI_PROBLEM_LEXICON_LOAD_ > 0
	if (!this->lexiconMaster->Load(this->lexiconFileName))
	{
#endif
#if _PMI_PROBLEM_MT_LEXICON_ > 0
		Thread^ t1 = gcnew Thread(gcnew ParameterizedThreadStart(this, &PMIProblem::ComputeLexiconProcessT));
		LexiconThreadParams^ p1 = gcnew LexiconThreadParams(this->evidence, this->lexicon, this->maxRuleLengths);
		t1->Start(p1);
		//Thread^ t2 = gcnew Thread(gcnew ParameterizedThreadStart(this, &PMIProblem::ComputeLexiconProcessT));
		//LexiconThreadParams^ p2 = gcnew LexiconThreadParams(this->evidenceModuleOnly, this->lexiconModuleOnly, this->maxRuleLengthsModulesOnly);
		//t2->Start(p2);
		t1->Join();
		//t2->Join();
		this->lexicon->CompileLexicon(this->evidence);
		//this->lexiconModuleOnly->CompileLexiconT(this->evidenceModuleOnly);
#else
		Int32 longest = 0;
		Int32 shortest = 99999999;

		for (size_t iRule = 0; iRule <= this->maxRuleLengths->GetUpperBound(0); iRule++)
		{
			if (this->minRuleLengths[iRule] < shortest)
			{
				shortest = this->minRuleLengths[iRule];
			}

			if (this->maxRuleLengths[iRule] > longest)
			{
				longest = this->maxRuleLengths[iRule];
			}
		}

		this->processMin = shortest;
		this->processMax = longest;

		Console::WriteLine("Building Lexicon");
		this->ComputeLexiconProcess(this->evidenceMaster, this->lexiconMaster, this->maxRuleLengths);
		Console::WriteLine("Compiling Lexicon");
		this->lexiconMaster->CompileLexiconT(this->evidenceMaster);
		Console::WriteLine("Compressing Lexicon");
		this->lexiconMaster->Compress();
#if _PMI_PROBLEM_LEXICON_SAVE_ > 0
		this->lexiconMaster->Save(this->lexiconFileName);
#endif
#endif
#if _PMI_PROBLEM_LEXICON_LOAD_ > 0
}
	else
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			this->lexiconMaster->UpdateLexicon_Length(iRule, this->minRuleLengths[iRule], this->maxRuleLengths[iRule]);
		}
#if _PMI_PROBLEM_LEXICON_SAVE_ > 0
		this->lexiconMaster->Save(this->lexiconFileName);
#endif
	}
#endif

	//	if (!this->lexiconModuleOnly->Load(this->lexiconModuleOnlyFileName))
	//	{
	//		Console::WriteLine("Compiling Module Only Lexicon");
	//		this->lexiconModuleOnly->Filter(this->lexicon);
	//#if _PMI_PROBLEM_LEXICON_MO_SAVE_ > 0
	//		this->lexiconModuleOnly->Save(this->lexiconModuleOnlyFileName);
	//#endif
	//	}
}

Lexicon^ PMIProblemV2A::ComputeLexiconForWord(Word^ W, Int32 SizeLimit)
{
	Lexicon^ result = gcnew Lexicon(this->vocabulary, SizeLimit, this->totalPoG);

	Int32 iLength = 1;
	bool stop = false;
	Int32 count = 0;
	Int32 highestCount = 0;

	while ((!stop) && (iLength <= SizeLimit) && (iLength <= W->symbols->Count))
	{
		Int32 start = 0;
		Int32 end = start + iLength;
		highestCount = 0;
		Word^ sw = gcnew Word();

		// produce the first word
		if ((end - 1) < W->symbols->Count)
		{
			for (size_t iSymbol = start; iSymbol < end; iSymbol++)
			{
				sw->Add(W->symbols[iSymbol]);
			}
			result->FoundSubword(sw, 0);
		}

		start++;
		end = start + iLength;

		// for every other word, remove the first symbol and add the next symbol over
		while ((end - 1) < W->symbols->Count)
		{
			sw->symbols->RemoveAt(0);
			sw->symbols->Add(W->symbols[end - 1]);
			count = result->FoundSubword(sw, 0);

			if (count > highestCount)
			{
				highestCount = count;
			}
			start++;
			end = start + iLength;
		}

		iLength++;
	}

	return result;
}

List<MarkerMapType^>^ PMIProblemV2A::ComputeMarkerMap(Lexicon^ Markers, Word^ E)
{
	List<MarkerMapType^>^ result = gcnew List<MarkerMapType^>;

	for (size_t iSymbol = 0; iSymbol < E->Count; iSymbol++)
	{
		// Check to see if current symbol is the 1st symbol in any marker
		Word^ tmp = gcnew Word();
		tmp->Add(E->symbols[iSymbol]);
		bool isSymbolMarkerLead = Markers->IsSubwordOverlap(tmp);

		// If so add symbols until the candidate is no longer a potential marker
		if (isSymbolMarkerLead)
		{
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 2
			Console::WriteLine("Potential Source marker detected @ " + iSymbol);
#endif

			Int32 iScan = iSymbol;
			Word^ w = gcnew Word();
			bool markerFound = false;
			bool stop = false;

			while ((!stop) && (iScan < E->Count))
			{
				w->Add(E->symbols[iScan]);
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 3
				Console::WriteLine(w->ToString());
#endif
				if (Markers->IsSubwordOverlap(w))
				{
					iScan++;

					if (Markers->FindSubwordCount(w) >= 0)
					{
						// record that a marker has been found but keep adding symbols as 
						// we want to find the longest possible marker from this position
						markerFound = true;
					}
				}
				else // potential marker, w, is not in the lexicon so stop adding symbols
				{
					stop = true;
					w->symbols->RemoveAt(w->symbols->Count - 1); // remove the last symbol, i.e. the one most recently added as it is not part of the marker
				}
			}

			if (markerFound)
			{
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
				Console::WriteLine("Marker " + w->ToString() + " found @ " + iSymbol + " to " + (iScan - 1));
#endif
				MarkerMapType^ m = gcnew MarkerMapType;
				m->marker = w;
				m->start = iSymbol;
				m->end = iScan - 1;
				result->Add(m);
				iSymbol = iScan; // if this is every modified to not ignore the character following the maker, make sure to remove the +1 in the production side
			}
		}
	}

	if ((result->Count == 0) || (result[result->Count - 1]->end != E->symbols->Count - 1))
	{
		// force a 'marker' to be at the end of the word so subword can be computed properly
		MarkerMapType^ m = gcnew MarkerMapType;
		m->marker = gcnew Word();
		m->start = E->symbols->Count;
		m->end = E->symbols->Count;
		result->Add(m);
	}

	return result;
}

// This computes a marker map while being forced to used an existing ordered set of markers
List<MarkerMapType^>^ PMIProblemV2A::ComputeMarkerMap(List<MarkerMapType^>^ Markers, Word^ E)
{
	List<MarkerMapType^>^ result = gcnew List<MarkerMapType^>;

	Int32 iMarker = 0;
	for (size_t iSymbol = 0; iSymbol < E->Count; iSymbol++)
	{
		// Check to see if current symbol is the 1st symbol of the Nth marker
		Word^ tmp = gcnew Word();
		tmp->Add(E->symbols[iSymbol]);
		bool isSymbolMarkerLead = false;

		// the last marker is always the end of the word. Since that marker is blank, do not do this check
		if (Markers[iMarker]->marker->Count > 0)
		{
			isSymbolMarkerLead = (E->symbols[iSymbol] == Markers[iMarker]->marker[0]);
		}

		// If so add symbols until the candidate is no longer a potential marker
		if (isSymbolMarkerLead)
		{
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 2
			Console::WriteLine("Potential Production marker detected @ " + iSymbol);
#endif

			Word^ w = gcnew Word();
			for (size_t iScan = 0; iScan < Markers[iMarker]->marker->Count; iScan++)
			{
				w->Add(E->symbols[iSymbol + iScan]);
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 3
				Console::WriteLine(w->ToString());
#endif
			}

			if (w->IsMatch(Markers[iMarker]->marker))
			{
				Int32 iScan = iSymbol + (Markers[iMarker]->marker->Count - 1);
#if	_PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
				Console::WriteLine("Tracker #" + this->tracker + ": Marker " + w->ToString() + " found @ " + iSymbol + " to " + (iScan));
#endif
				MarkerMapType^ m = gcnew MarkerMapType;
				m->marker = w;
				m->start = iSymbol;
				m->end = iScan;
				result->Add(m);
				iSymbol = iScan + 1; // since the markers in the source ignore the next character, the production marker needs to ignore it as well
				iMarker++;
			}
		}
	}

	if ((result->Count == 0) || (result[result->Count - 1]->end != E->symbols->Count - 1))
	{
		// force a 'marker' to be at the end of the word so subword can be computed properly
		MarkerMapType^ m = gcnew MarkerMapType;
		m->marker = gcnew Word();
		m->start = E->symbols->Count;
		m->end = E->symbols->Count;
		result->Add(m);
	}

	return result;
}

List<SymbolFragmentType^>^ PMIProblemV2A::ComputeMaxFragment(Word^ Source, Word^ Production)
{
	//Console::WriteLine(Source->ToString());
	//Console::WriteLine(Production->ToString());

	List<SymbolFragmentType^>^ result = gcnew List<SymbolFragmentType^>;
	Int32 reservedHead = 0;
	Int32 reservedTail = 0;
	bool pastFirst = false;

	for (size_t iSymbol = 0; iSymbol < Source->symbols->Count; iSymbol++)
	{
		// Ignore the first module
		if (this->vocabulary->IsModule(Source->symbols[iSymbol]) && (!pastFirst))
		{
			Int32 iRule = this->vocabulary->RuleIndex(Source->symbols[iSymbol]);
			pastFirst = true;
			reservedHead += this->minRuleLengths[iRule];
		}
		else if (this->vocabulary->IsModule(Source->symbols[iSymbol]) && (pastFirst))
		{ // else look ahead to compute the reservedTail
			reservedTail = 0;
			Word^ fragment = gcnew Word();
			for (size_t iSymbolInner = iSymbol + 1; iSymbolInner < Source->symbols->Count; iSymbolInner++)
			{
				if (this->vocabulary->IsModule(Source->symbols[iSymbolInner]))
				{
					Int32 iRule = this->vocabulary->RuleIndex(Source->symbols[iSymbolInner]);
					reservedTail += this->minRuleLengths[iRule];
				}
				else // otherwise this is a constant
				{
					reservedTail++; // add a reserved symbol for the constant
				}
			}

			// After computing the reservedTail capture the fragment
			if (Production->symbols->Count - reservedTail - reservedHead <= 2 * PMIProblem::cMaxFragmentLength)
			{
				for (size_t iSymbolInner = reservedHead; iSymbolInner < Production->symbols->Count - reservedTail; iSymbolInner++)
				{
					fragment->Add(Production->symbols[iSymbolInner]);
				}

				SymbolFragmentType^ m = gcnew SymbolFragmentType();
				m->fragment = fragment;
				m->s = Source->symbols[iSymbol];
				m->isHead = false;
				m->isComplete = false;
				result->Add(m);
			}

			//Console::WriteLine(Source->symbols[iSymbol]->ToString() + " => " + fragment->ToString());
		}
		else // otherwise this is a constant
		{
			reservedHead++; // add a reserved symbol for the constant
		}
	}

	return result;
}

SymbolFragmentType^ PMIProblemV2A::ComputeMaxHead(Word^ Source, Word^ Production)
{
	SymbolFragmentType^ result = gcnew SymbolFragmentType();

	// Find first module in generation
	// and compute the # reserved symbols
	Int32 iFirst = -1;
	Int32 reserved = 0;
	Word^ rightMarker = gcnew Word();
	bool capture = false;
	Int32 moduleCount = 0;

	for (size_t iSymbol = 0; iSymbol < Source->symbols->Count; iSymbol++)
	{
		if (this->vocabulary->IsModule(Source->symbols[iSymbol]) && iFirst == -1)
		{
			// record the location of the first module and start capturing the right marker
			iFirst = iSymbol;
			capture = true;
			moduleCount++;
		}
		else if (this->vocabulary->IsModule(Source->symbols[iSymbol])) 
		{
			// add a # of reserved symbols equal to the min length of the module
			Int32 iRule = this->vocabulary->RuleIndex(Source->symbols[iSymbol]);
			reserved = this->minRuleLengths[iRule];
			capture = false; // stop capturing the right marker once the 2nd module is hit
			moduleCount++;
		}
		else // otherwise this is a constant
		{
			reserved++; // add a reserved symbol for the constant
			if (capture)
			{
				rightMarker->Add(Source->symbols[iSymbol]); // add to the right marker
			}
		}
	}

	Word^ head = gcnew Word();

	//Console::WriteLine("Right Marker = " + rightMarker->ToString());

	Int32 end = reserved + rightMarker->Count;

	// Find the last instance of the right marker before the reserved symbols
	// Finding the last instance is that same as finding the first instance starting from the end
	Int32 iSymbol = end;
	bool stop = false;
	while ((!stop) && (iSymbol < Production->Count))
	{
		bool isMatch = true;
		Int32 iMarkerSymbol = 0;

		while ((isMatch) && (iMarkerSymbol < rightMarker->Count))
		{
			if (rightMarker[iMarkerSymbol] != Production[Production->Count - iSymbol + iMarkerSymbol])
			{
				isMatch = false;
			}
			iMarkerSymbol++;
		}

		if (isMatch)
		{
			end = Production->Count - iSymbol;
			stop = true;
		}
		iSymbol++;
	}

	// add all symbols from the start to the computed end
	for (size_t iSymbol = iFirst; iSymbol < end; iSymbol++)
	{
		head->Add(Production[iSymbol]);
	}

	//Console::WriteLine("Max Head = " + head->ToString());
	result->s = Source->symbols[iFirst];
	result->fragment = head;
	result->isHead = true;
	result->isComplete = false;

	return result;
}

SymbolFragmentType^ PMIProblemV2A::ComputeMinHead(Word^ Source, Word^ Production)
{
	SymbolFragmentType^ result = gcnew SymbolFragmentType();

	// Find first module in generation
	Int32 iFirst = 0;

	while ((iFirst < Source->symbols->Count) && (!this->vocabulary->IsModule(Source->symbols[iFirst])))
	{
		iFirst++;
	}

	Word^ head = gcnew Word();
	if (iFirst < Source->symbols->Count)
	{
		Int32 ruleIndex = this->vocabulary->RuleIndex(Source->symbols[iFirst]);
		result->s = Source->symbols[iFirst];

		// Find the right neighbour to the 1st symbol
		head->Add(Production->symbols[iFirst]);
		Symbol^ right = Source->symbols[iFirst + 1];

		// A module is not allowed to produce only itself, so add the next symbol
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
		if (head[0] == this->vocabulary->Symbols[ruleIndex])
		{
			iFirst++;
			head->Add(Production->symbols[iFirst]);
		}
#endif
		// Check to make sure the module is not the last symbol
		if (this->vocabulary->IsConstant(right))
		{
			// If the right neighbour is a constant we can add additional symbols by scanning until we hit the constant
			Int32 idx = iFirst + 1;
			while (Production->symbols[idx] != right)
			{
				head->Add(Production->symbols[idx]);
				idx++;
			}

			// Keep scanning until the 2nd time the right neighbour is found, if it is found then it is not guaranteed complete
			result->isComplete = true;
			while ((result->isComplete) && (idx < Production->symbols->Count))
			{
				if ((Production->symbols[idx] == right))
				{
					result->isComplete = false;
				}
				idx++;
			}
		}
	}

#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1 || _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
	Console::WriteLine("Min Head for " + result->s->ToString() + " => " + head->ToString());
#endif
	result->fragment = head;
	result->isHead = true;

	return result;
}

SymbolFragmentType^ PMIProblemV2A::ComputeMaxTail(Word^ Source, Word^ Production)
{
	//Console::WriteLine(Source->ToString() + " " + Production->ToString());

	SymbolFragmentType^ result = gcnew SymbolFragmentType();

	// Find last module in generation and compute the # reserved symbols
	Int32 iModule = -1;
	Int32 reserved = 0;
	Word^ leftMarker = gcnew Word();
	bool capture = false;
	Int32 moduleCount = 0;

	for (size_t iSymbol = 0; iSymbol < Source->symbols->Count; iSymbol++)
	{
		if (this->vocabulary->IsModule(Source->symbols[Source->symbols->Count - iSymbol - 1]) && iModule == -1)
		{
			// record the location of the first module and start capturing the right marker
			iModule = iSymbol;
			capture = true;
			moduleCount++;
		}
		else if (this->vocabulary->IsModule(Source->symbols[Source->symbols->Count - iSymbol - 1]))
		{
			// add a # of reserved symbols equal to the min length of the module
			Int32 iRule = this->vocabulary->RuleIndex(Source->symbols[Source->symbols->Count - iSymbol - 1]);
			reserved = this->minRuleLengths[iRule];
			capture = false; // stop capturing the right marker once the 2nd module is hit
			moduleCount++;
		}
		else // otherwise this is a constant
		{
			reserved++; // add a reserved symbol for the constant
			if (capture)
			{
				leftMarker->Add(Source->symbols[Source->symbols->Count - iSymbol - 1]); // add to the right marker
			}
		}
	}

	leftMarker->symbols->Reverse();

	Word^ tail = gcnew Word();

	//Console::WriteLine("Left Marker = " + leftMarker->ToString());

	Int32 start = reserved;

	// Find the last instance of the right marker before the reserved symbols
	// Finding the last instance is that same as finding the first instance starting from the end
	Int32 iSymbol = start;
	bool stop = false;
	while ((!stop) && (iSymbol < Production->Count))
	{
		bool isMatch = true;
		Int32 iMarkerSymbol = 0;

		while ((isMatch) && (iMarkerSymbol < leftMarker->Count))
		{
			if (leftMarker[iMarkerSymbol] != Production[iSymbol + iMarkerSymbol])
			{
				isMatch = false;
			}
			iMarkerSymbol++;
		}

		if (isMatch)
		{
			start = iSymbol + leftMarker->Count;
			stop = true;
		}
		iSymbol++;
	}

	// add all symbols from the start to the computed end
	for (size_t iSymbol = start; iSymbol < Production->Count - iModule; iSymbol++)
	{
		tail->Add(Production[iSymbol]);
	}

	//Console::WriteLine("Max Tail = " + tail->ToString());
	result->s = Source->symbols[Source->symbols->Count - iModule - 1];
	result->fragment = tail;
	result->isHead = false;
	result->isComplete = false;

	return result;
}

SymbolFragmentType^ PMIProblemV2A::ComputeMinTail(Word^ Source, Word^ Production)
{
	// Find last module in generation
	SymbolFragmentType^ result = gcnew SymbolFragmentType();
	Int32 iLast = Source->symbols->Count - 1;
	Int32 tailSkips = 0;

	while ((!this->vocabulary->IsModule(Source->symbols[iLast])) && (iLast > 0))
	{
		iLast--;
		tailSkips++;
	}

	result->s = Source->symbols[iLast];
	Int32 ruleIndex = this->vocabulary->RuleIndex(Source->symbols[iLast]);

	Word^ tail = gcnew Word();
	// the index is the length of the evidence - the number of symbols skipped from the back
	Int32 idx = Production->symbols->Count - (tailSkips)-1;
	tail->Add(Production->symbols[idx]);
	Symbol^ left = Source->symbols[iLast - 1];

	// A rule is not allowed to produce only itself
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
	if (tail[0] == this->vocabulary->Symbols[ruleIndex])
	{
		idx--;
		tail->Add(Production->symbols[idx]);
	}
#endif
	if (this->vocabulary->IsConstant(left))
	{
		idx--;
		while (Production->symbols[idx] != left)
		{
			tail->Add(Production->symbols[idx]);
			idx--;
		}
	}

	tail->symbols->Reverse();

#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1 || _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
	Console::WriteLine("Min Tail for " + result->s->ToString() + " => " + tail->ToString());
#endif
	result->fragment = tail;
	result->isHead = false;
	result->isComplete = false;

	return result;
}

void PMIProblemV2A::ComputeMinHeadsTails()
{
	for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
	{
		// Find first module in generation
		Int32 iFirst = 0;
		Int32 iLast = this->evidence[iGen]->symbols->Count - 1;
		Int32 tailSkips = 0;
		Word^ head = gcnew Word();
		Word^ tail = gcnew Word();

		while ((iFirst < this->evidence[iGen]->symbols->Count) && (!this->vocabulary->IsModule(this->evidence[iGen]->symbols[iFirst])))
		{
			iFirst++;
		}

		while ((!this->vocabulary->IsModule(this->evidence[iGen]->symbols[iLast])) && (iLast > 0))
		{
			iLast--;
			tailSkips++;
		}

		Int32 ruleIndex1 = -1;
		Int32 ruleIndex2 = -1;

		if (iFirst < this->evidence[iGen]->symbols->Count)
		{
			ruleIndex1 = this->vocabulary->RuleIndex(this->evidence[iGen]->symbols[iFirst]);
		}

		if (this->vocabulary->IsModule(this->evidence[iGen]->symbols[iLast]))
		{
			ruleIndex2 = this->vocabulary->RuleIndex(this->evidence[iGen]->symbols[iLast]);
		}

		if (iFirst < this->evidence[iGen]->symbols->Count - 1)
		{
			// Find the right neighbour to the 1st symbol and left neighbour of the last symbol
			head->Add(this->evidence[iGen + 1]->symbols[iFirst]);
			Symbol^ right = this->evidence[iGen]->symbols[iFirst + 1];

			// A module is not allowed to produce only itself, so add the next symbol
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
			if (head[0] == this->vocabulary->Symbols[ruleIndex1])
			{
				iFirst++;
				head->Add(this->evidence[iGen + 1]->symbols[iFirst]);
			}
#endif
			if (this->vocabulary->IsConstant(right))
			{
				// If the right neighbour is a constant we can add additional symbols by scanning until we hit the constant
				Int32 idx = iFirst + 1;
				while (this->evidence[iGen + 1]->symbols[idx] != right)
				{
					head->Add(this->evidence[iGen + 1]->symbols[idx]);
					idx++;
				}
			}
		}
		else
		{// if the first module is the last symbol then it produces everything after stripping off the constants in front
			Int32 idx = iFirst + 1;
			while (idx < this->evidence[iGen + 1]->symbols->Count)
			{
				head->Add(this->evidence[iGen + 1]->symbols[idx]);
				idx++;
			}
		}

		if ((ruleIndex1 != -1) && (head->Count > this->ruleHeadMaster[ruleIndex1]->Count))
		{
			this->ruleHeadMaster[ruleIndex1] = head;
		}

		if (ruleIndex2 != -1)
		{
			// the index is the length of the evidence - the number of symbols skipped from the back
			Int32 idx = this->evidence[iGen + 1]->symbols->Count - (tailSkips)-1;
			tail->Add(this->evidence[iGen + 1]->symbols[idx]);

			if (iLast > 0)
			{
				Symbol^ left = this->evidence[iGen]->symbols[iLast - 1];
				// A rule is not allowed to produce only itself
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
				if (tail[0] == this->vocabulary->Symbols[ruleIndex2])
				{
					idx--;
					tail->Add(this->evidence[iGen + 1]->symbols[idx]);
				}
#endif
				if (this->vocabulary->IsConstant(left))
				{
					idx--;
					while (this->evidence[iGen + 1]->symbols[idx] != left)
					{
						tail->Add(this->evidence[iGen + 1]->symbols[idx]);
						idx--;
					}
				}

				if (tail->Count > this->ruleTailMaster[ruleIndex2]->Count)
				{
					this->ruleTailMaster[ruleIndex2] = tail;
				}
			}
			else
			{
				idx--;
				while (idx < 0)
				{
					tail->Add(this->evidence[iGen + 1]->symbols[idx]);
					idx--;
				}
			}
		}
	}

	for (size_t iRule = 0; iRule <= this->ruleTailMaster->GetUpperBound(0); iRule++)
	{
		this->ruleTailMaster[iRule]->symbols->Reverse();
	}


	this->ruleHead = this->ruleHeadMaster;
	this->ruleTail = this->ruleTailMaster;
}

array<Int32>^ PMIProblemV2A::ComputeRuleLengthFromRPT(Int32 iGen, Int32 SymbolIndex)
{
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= SymbolIndex; iSymbol++)
		{
			ruleLengths[iRule] += this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
		}
	}

	return ruleLengths;
}

void PMIProblemV2A::ComputeSymbiology()
{
	this->symbolSubsets = gcnew array<bool, 2>(this->generations, this->vocabulary->IndexModulesEnd - this->vocabulary->IndexModulesStart + 1);
	this->lastGenerationSymbol = gcnew array<Int32>(this->vocabulary->NumSymbols());

	// Step 1 - Find the subset of symbols at each generation
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Int32 modulesFound = 0;
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
		{
			bool found = false;
			Int32 index = 0;

			while ((!found) && (index < this->evidence[iGen]->symbols->Count))
			{
				if (this->vocabulary->Symbols[iSymbol] == this->evidence[iGen]->symbols[index])
				{
					found = true;
					this->symbolSubsets[iGen, this->vocabulary->RuleIndex(this->evidence[iGen]->symbols[index])] = true;
					modulesFound++;
				}
				index++;
			}
		}
		this->numModules[iGen] = modulesFound;
	}

	// Step 2 - Determine if any are vanishing
	for (size_t iSymbol = 0; iSymbol <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Sym); iSymbol++)
	{
		bool isVanishing = false;

		// 1st pass - Determine if the symbol is a vanishing symbol
		for (size_t iGen = 1; iGen <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Gen); iGen++)
		{
			if ((this->symbolSubsets[iGen, iSymbol] == false) && (this->symbolSubsets[iGen - 1, iSymbol] == true))
			{
				isVanishing = true;
			}
			else if ((this->symbolSubsets[iGen, iSymbol] == true) && (this->symbolSubsets[iGen, iSymbol] == false) && (isVanishing))
			{
				isVanishing = false;
			}
		}

		// 2nd pass - Find the point it vanishes
		if (isVanishing)
		{
			for (size_t iGen = 1; iGen <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Gen); iGen++)
			{
				if ((this->symbolSubsets[iGen, iSymbol] == false) && (this->symbolSubsets[iGen - 1, iSymbol] == true))
				{
					this->lastGenerationSymbol[iSymbol] = iGen - 1;
				}
			}
		}
		else
		{
			this->lastGenerationSymbol[iSymbol] = this->generations - 1;
		}
	}

	// Find last symbol for each constant
	for (size_t iSymbol = this->vocabularyMaster->IndexConstantsStart; iSymbol <= this->vocabularyMaster->IndexConstantsEnd; iSymbol++)
	{
		this->lastGenerationSymbol[iSymbol] = this->generations - 1;
	}

#if _PMI_PROBLEM_VERBOSE_ >= 1
	Console::WriteLine("Symbol Subsets");

	Console::Write("Gen   ");
	for (size_t iGen = 0; iGen <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Gen); iGen++)
	{
		Console::Write(iGen + "  ");
	}
	Console::WriteLine();

	for (size_t iSymbol = 0; iSymbol <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Sym); iSymbol++)
	{
		Console::Write(this->vocabulary->Symbols[this->vocabulary->IndexModulesStart + iSymbol] + "(" + this->lastGenerationSymbol[iSymbol] + ")");
		for (size_t iGen = 0; iGen <= this->symbolSubsets->GetUpperBound(PMIProblem::cDim_SymSub_Gen); iGen++)
		{
			if (this->symbolSubsets[iGen, iSymbol])
			{
				Console::Write("  T");
			}
			else
			{
				Console::Write("  F");
			}
		}
		Console::WriteLine("");
	}

	Console::Write("#Mod  ");
	for (size_t iGen = 0; iGen <= this->numModules->GetUpperBound(0); iGen++)
	{
		Console::Write(this->numModules[iGen] + "  ");
	}
	Console::WriteLine();
	//Console::WriteLine("Paused");
	//Console::ReadLine();
#endif
}

List<MarkerMapType^>^ PMIProblemV2A::ConvertMarkerMap(List<MarkerMapType^>^ Markers)
{
	List<MarkerMapType^>^ result = gcnew List<MarkerMapType^>;

	for (size_t iMarker = 0; iMarker < Markers->Count; iMarker++)
	{
		MarkerMapType^ m = gcnew MarkerMapType();
		m->start = Markers[iMarker]->start;
		m->end = Markers[iMarker]->end;

		Word^ w = this->ConvertWord(Markers[iMarker]->marker);
		m->marker = w;

#if _PMI_PROBLEM_DISPLAY_MARKERS_ >= 1
		Console::WriteLine(Markers[iMarker]->marker->ToString() + " => " + m->marker->ToString());
#endif

		result->Add(m);
	}

	return result;
}

Word^ PMIProblemV2A::ConvertWord(Word^ W)
{
	Word^ w = gcnew Word(); // make a copy so as to not change the original
	for (size_t iSymbol = 0; iSymbol < W->symbols->Count; iSymbol++)
	{
		if (!this->vocabulary->IsModule(W->symbols[iSymbol]))
		{
			w->Add(W->symbols[iSymbol]);
		}
		else
		{
			Int32 ruleIndex = this->vocabulary->RuleIndex(W->symbols[iSymbol]);

			if ((this->minRuleLengths[ruleIndex] != this->maxRuleLengths[ruleIndex]) || (this->minRuleLengths[ruleIndex] != this->ruleHead[ruleIndex]->symbols->Count))
			{
				throw gcnew Exception("Rule lengths do not match fragment length");
			}

			for (size_t iFragmentSymbol = 0; iFragmentSymbol < this->ruleHead[ruleIndex]->Count; iFragmentSymbol++)
			{
				w->Add(this->ruleHead[ruleIndex]->symbols[iFragmentSymbol]);
			}
		}
	}

	return w;
}

// TODO: Need to extend to take into account that the RPT values may change from generation to generation
void PMIProblemV2A::CreateModelFromRPT()
{
	Int32 iGen = 0;
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	// if the problem is fully solved by analysis set the constant index to 0
	if (this->constantIndex == -1)
	{
		this->constantIndex = 0;
	}

	for (size_t iRule = 0; iRule <= ruleLengths->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
		{
			ruleLengths[iRule] += this->rulesParikhTemplate[iGen, iRule, iSymbol];
		}

		this->knownRuleLengthsIndexed[this->constantIndex, iRule] = ruleLengths[iRule];
	}

	this->solution->rules = this->CreateRulesByScanning(ruleLengths, 0, this->generations, true);
}

// TODO: Need to extend to take into account that the RPT values may change from generation to generation
List<ProductionRule^>^ PMIProblemV2A::CreateRulesFromRPT()
{
	List<ProductionRule^>^ result = gcnew List<ProductionRule^>;
	Int32 iGen = 0;
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	// if the problem is fully solved by analysis set the constant index to 0
	if (this->constantIndex == -1)
	{
		this->constantIndex = 0;
	}

	for (size_t iRule = 0; iRule <= ruleLengths->GetUpperBound(0); iRule++)
	{
		if (this->IsModuleInSet(iRule))
		{
			for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + this->constantIndex; iSymbol++)
			{
				ruleLengths[iRule] += this->rulesParikhTemplate[iGen, iRule, iSymbol];
			}
		}
	}

	result = this->CreateRulesByScanning(ruleLengths, this->currentSet->startGeneration, this->currentSet->endGeneration, false);

	return result;
}

array<Int32, 2>^ PMIProblemV2A::CreateParikhRuleFromRPT()
{
	array<Int32, 2>^ result = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= result->GetUpperBound(1); iSymbol++)
		{
			result[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
		}
	}

	return result;
}

List<ProductionRule^>^ PMIProblemV2A::CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration)
{
	return this->CreateRulesByScanning(RuleLengths, StartGeneration, EndGeneration, false);
}

List<ProductionRule^>^ PMIProblemV2A::CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration, bool Finalize)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	List<Symbol^>^ completed = gcnew List<Symbol^>;
	Int32 modulesNeeded = 0;

	if (!Finalize)
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			if (this->IsModuleInSet(iRule))
			{
				modulesNeeded++;
			}
		}
	}
	else
	{
		modulesNeeded = this->vocabulary->numModules;
	}

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
	Console::WriteLine("Evidence");

	Console::WriteLine("E1: " + E1->ToString());
	Console::WriteLine("E2: " + E2->ToString());
	Console::WriteLine();
#endif

	Int32 idx;
	bool failed = false;
	Int32 iGen = 0;

	do
	{
		//Console::WriteLine(iGen);
		Word^ E1 = this->evidence[StartGeneration + iGen];
		Word^ E2 = this->evidence[StartGeneration + 1 + iGen];
		idx = 0;

		for each (Symbol^ s in E1->symbols)
		{
			// If this is a modules which has not been process, then process it
			if ((this->vocabulary->IsModule(s)) && (!completed->Contains(s)))
			{
				//Console::WriteLine(s->ToString());
				Symbol^ predecessor;
				List<Symbol^>^ successor = gcnew List<Symbol^>;
				predecessor = s;

				Int32 iRule = this->vocabulary->RuleIndex(s);
				Int32 ruleLength;

				if (this->knownRuleLengthsIndexed[this->constantIndex, iRule] != PMIProblem::cUnknownRuleLength)
				{
					ruleLength = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
				}
				else
				{
					ruleLength = RuleLengths[this->rule2solution[iRule]];
				}

				//Console::WriteLine(s->ToString() + " " + ruleLength + " " + idx);
				failed = false;
				for (size_t iSymbol = 0; iSymbol < ruleLength; iSymbol++)
				{
					if (idx < E2->symbols->Count)
					{
						successor->Add(E2->symbols[idx]);
						idx++;
					}
					else
					{
						failed = true;
					}
				}

				if (!failed)
				{

					// Build constraint(s) based on the sub-problem
					List<IProductionRuleConstraint^>^ C = gcnew List<IProductionRuleConstraint^>;
					ProductionRuleConstraint_Time^ C1 = gcnew ProductionRuleConstraint_Time(StartGeneration, EndGeneration);
					C->Add(C1);
					ProductionRule^ r = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor, C);
					R->Add(r);
					completed->Add(s);
				}
			}
			// If is a module that has been processed then skip the length of its rules
			else if ((this->vocabulary->IsModule(s)) && (completed->Contains(s)))
			{
				Int32 iRule = this->vocabulary->RuleIndex(s);
				Int32 ruleLength;
				if (this->knownRuleLengthsIndexed[this->constantIndex, iRule] != PMIProblem::cUnknownRuleLength)
				{
					ruleLength = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
				}
				else
				{
					ruleLength = RuleLengths[this->rule2solution[iRule]];
				}
				idx += ruleLength;
				//Console::WriteLine(s->ToString() + " " + ruleLength + " " + idx + " " + E2->symbols[idx]->ToString());
			}
			// If it is a constant, then skip one symbol
			else
			{
				idx++;
				//Console::WriteLine(s->ToString() + " " + idx);
			}
		}

		iGen++;
	//} while ((!failed) && (R->Count < modulesNeeded) && (StartGeneration + 1 + iGen < this->evidence->Count));
	} while ((R->Count < modulesNeeded) && (StartGeneration + 1 + iGen < this->evidence->Count));

	if (R->Count < modulesNeeded)
	{
		return nullptr;
	}
	else
	{
		return R;
	}
}

void PMIProblemV2A::CreateSubProblems_Generations(List<Int32>^ Cutoffs)
{
	// Create sub-problems based on the generation cutoffs
	Int32 start = 0;
	Int32 end = 0;

	for (size_t iProblem = 1; iProblem < Cutoffs->Count; iProblem++)
	{
		start = Cutoffs[iProblem - 1];
		end = Cutoffs[iProblem];

		PMIProblemSet^ S1 = gcnew PMIProblemSet(start, end);

		//// Check to see if the modules at this generation produce only constants, if so don't make a module problem
		//if (this->numModules[end] > 0)
		//{
		//	PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Modules, PMIProblemDescriptor::PriorityModuleProblem);
		//	this->ComputeModulesToSolve(P1);
		//	P1->unset = false;
		//	S1->moduleProblem = P1;

		//}

		//// Check to see if the modules at this generation produce no constants, if so don't make a constant problem
		//if (this->numModules[start] > 0)
		//{
		//	PMIProblemDescriptor^ P2 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Constants, PMIProblemDescriptor::PriorityConstantProblem);
		//	PMIProblemDescriptor^ P3 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Order, PMIProblemDescriptor::PriorityOrderProblem);
		//	this->ComputeModulesToSolve(P2);
		//	this->ComputeModulesToSolve(P3);
		//	P2->unset = false;
		//	P3->unset = false;
		//	S1->constantProblem = P2;
		//	S1->orderProblem = P3;
		//}

		this->problemSets->Add(S1);
	}
}

// TODO: this shouldn't override the best known information
void PMIProblemV2A::EstimateGrowthPattern()
{
	for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		array<Int32>^ metaFragmentPV = this->vocabulary->CompressSymbols(metaFragment);
		for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
		{
			this->UpdateMinPoG(iRule, iSymbol, metaFragmentPV[iSymbol]);
		}
	}

	this->ComputeGrowthPattern_Unaccounted();
}

bool PMIProblemV2A::EstimateRuleLengths_Fragments()
{
	bool result = false;
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Rule lengths based on Fragments");
#endif
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		result = this->UpdateMinRuleLengths(iRule, metaFragment->Count) || result;
	}

#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	this->OutputRuleLengths();
#endif
	return result;
}

bool PMIProblemV2A::EstimateRuleLengths_Growth()
{
	bool result = false;

#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Update Lengths Based on Actual Production to Estimated Production");
#endif
	for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
	{
		// Total production is the number of symbols in the next generation minus the number of constants in this generation
		// # of constants is equal to the total number of symbols minus the number of modules
		Int32 totalProduction = this->evidence[iGen + 1]->symbols->Count - (this->evidence[iGen]->symbols->Count - this->evidenceModuleOnly[iGen]->symbols->Count);
		Int32 maxProduction = 0;
		Int32 minProduction = 0;
		Int32 avgProduction = 0;
		Int32 sumMax = 0;
		Int32 sumMin = 0;
		Int32 sumAvg = 0;
		for (size_t iRule = 0; iRule <= this->maxRuleLengths->GetUpperBound(0); iRule++)
		{
			sumMax = this->maxRuleLengths[iRule];
			sumMin = this->minRuleLengths[iRule];
			sumMin = ((this->minRuleLengths[iRule] + this->maxRuleLengths[iRule]) / 2);
			maxProduction += this->maxRuleLengths[iRule] * this->evidence[iGen]->parikhVector[iRule];
			minProduction += this->minRuleLengths[iRule] * this->evidence[iGen]->parikhVector[iRule];
			avgProduction += ((this->minRuleLengths[iRule] + this->maxRuleLengths[iRule]) / 2) * this->evidence[iGen]->parikhVector[iRule];
		}

		// Compute the error
		Int32 errorMax = maxProduction - totalProduction;
		Int32 errorMin = totalProduction - minProduction;
		Int32 errorAvg = Math::Abs(totalProduction - avgProduction);
		//Console::WriteLine(totalProduction + " : " + minProduction + " : " +  avgProduction + " : " + maxProduction);
		//Console::WriteLine(totalProduction + " : " + minProduction + " : " + maxProduction);

		this->UpdateTotalMinRuleLengths(sumMax - errorMax);
		this->UpdateTotalMaxRuleLengths(sumMin + errorMin);

		//// For any symbols in this generation, the difference between the min/max cannot be greater than the error
		//for (size_t iRule = 0; iRule <= this->maxRuleLengths->GetUpperBound(0); iRule++)
		//{
		//	if (this->symbolSubsets[iGen, iRule])
		//	{
		//		//result = this->UpdateMinRuleLengths(iRule, this->maxRuleLengths[iRule] - errorMax) || result;
		//		//result = this->UpdateMaxRuleLengths(iRule, this->minRuleLengths[iRule] + errorMin) || result;
		//		//result = this->UpdateMinRuleLengths(iRule, ((this->maxRuleLengths[iRule] + this->minRuleLengths[iRule]) / 2) - errorAvg) || result;
		//		//result = this->UpdateMaxRuleLengths(iRule, ((this->maxRuleLengths[iRule] + this->minRuleLengths[iRule]) / 2) + errorAvg) || result;
		//	}
		//}
	}
	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2A::EstimateRuleLengths_Lexicon()
{
	bool result = false;

	// Update Min/Max Lengths based on Lexicon
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Rule lengths based on Lexicon");
#endif
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if (this->lexicon->symbolSubwords[iRule]->Count > 1)
		{
			result = this->UpdateMinRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			result = this->UpdateMaxRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][this->lexicon->symbolSubwords[iRule]->Count - 1]->symbols->Count) || result;
		}
		else
		{// if the lexicon has a single word then this is the solution
			this->solvedModules[iRule] = true;
			result = this->UpdateMinRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			result = this->UpdateMaxRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			this->UpdatePoGForCompleteFragment(iRule, this->lexicon->symbolSubwords[iRule][0]);
		}
	}
	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2A::EstimateRuleLengths_Inference(Int32 Start, Int32 End)
{
	bool result = false;

	for (size_t iGen = Start; iGen <= End - 1; iGen++)
	{
		// Ensure that the modules have reached a steady state and there's at least 1 module
		if (!this->IsSymbolSubsetChange(iGen, iGen + 1) && (!this->IsSymbolAllConstant(iGen)))
		{
			// Get the two generations of evidence
			Word^ e2 = this->evidence[iGen + 1];
			Word^ e1 = this->evidence[iGen];

			// The number of symbols in the generation minus the number of constants in the previous generation
			Int32 min = 0;
			Int32 countMin = e2->symbols->Count - this->vocabulary->CountConstants(e1);
			Int32 idxMin = 0;

			Int32 max = 0;
			Int32 countMax = countMin;
			Int32 idxMax = 0;

			// Find the most occuring symbol in y
			Int32 highest = 0;
			Int32 lowest = 99999999;

			if (this->vocabulary->numModules > 1)
			{
				// Step 1 - Find the modules that exist the least and most
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
				{
					if (this->symbolSubsets[iGen, iSymbol])
					{
						Int32 tmp = idxMin;
						// Find the symbol which occur most often
						// 1 - Prefer a symbol with a higher minimum length than the current highest
						// 2 - Do not use a symbol which has a known length
						if ((this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]) && ((e1->parikhVector[iSymbol] > highest) || ((e1->parikhVector[iSymbol] == highest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp]))))
						{
							idxMin = iSymbol;
							highest = e1->parikhVector[iSymbol];
						}

						if ((this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]) && ((e1->parikhVector[iSymbol] < lowest) || ((e1->parikhVector[iSymbol] == lowest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp]) && (this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]))))
						{
							idxMax = iSymbol;
							lowest = e1->parikhVector[iSymbol];
						}
					}
				}

				// Assume every other symbol only produces their minimum
				// if the minimum is unknown assume it is 1
				// if the minimum is unknown or modules only assume it is 0
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
				{
					if (this->symbolSubsets[iGen, iSymbol])
					{
						if (iSymbol != idxMin)
						{
							min += this->minRuleLengths[iSymbol];
							countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
							//countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
						}

						if (iSymbol != idxMax)
						{
							countMax -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
							max += this->minRuleLengths[iSymbol];
						}
					}
				}

				if (e1->parikhVector[idxMin] != 0)
				{
					countMin /= e1->parikhVector[idxMin];
				}

				if (e1->parikhVector[idxMax] != 0)
				{
					countMax /= e1->parikhVector[idxMax];
				}

				min += countMin;
				max += countMax;

				result = this->UpdateTotalMaxRuleLengths(max) || result;
				result = this->UpdateTotalMinRuleLengths(min) || result;
			}
		}
	}

	// Given the total maximum rule length, the minimum individual rule length for each rule can be computed by
	// TotalMin - Sum(Maximums except iRule)
	// And
	// TotalMax - Sum(Minimum except iRule)
	// Repeat this process until convergence
	bool valueChanged;
	do
	{
		valueChanged = false;
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			Int32 sum = 0;

			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
			{
				if (iValue != iRule)
				{
					sum += this->maxRuleLengths[iValue];
				}
			}

			Int32 tmp = this->minRuleLengths[iRule]; // capture the current value

			result = this->UpdateMinRuleLengths(iRule, this->totalMinRuleLength - sum) || result;

			if (tmp != this->minRuleLengths[iRule])
			{
				valueChanged = true;
			}
		}

		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			Int32 sum = 0;

			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
			{
				if (iValue != iRule)
				{
					sum += this->minRuleLengths[iValue];
				}
			}

			Int32 tmp = this->maxRuleLengths[iRule]; // capture the current value
			result = this->UpdateMaxRuleLengths(iRule, this->totalMaxRuleLength - sum) || result;

			if (tmp != this->maxRuleLengths[iRule])
			{
				valueChanged = true;
			}
		}
	} while (valueChanged);

	this->OutputRuleLengths();

	return result;
}

// TODO: This function doesn't work. Do not use!!
bool PMIProblemV2A::EstimateRuleLengths_Inference(Word^ Source, Word^ Production)
{
	bool result = false;

	// Get the two generations of evidence
	Word^ e2 = Production;
	Word^ e1 = Source;

	// The number of symbols in the generation minus the number of constants in the previous generation
	Int32 min = 0;
	Int32 countMin = e2->symbols->Count - this->vocabulary->CountConstants(e1);
	Int32 idxMin = 0;

	Int32 max = 0;
	Int32 countMax = countMin;
	Int32 idxMax = 0;

	// Find the most occuring symbol in y
	Int32 highest = 0;
	Int32 lowest = 99999999;

	bool unset = true;
	if (this->vocabulary->numModules > 1)
	{
		// Step 1 - Find the modules that exist the least and most
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
		{
			Int32 tmp = idxMin;
			// Find the symbol which occur most often
			// 1 - Prefer a symbol with a higher minimum length than the current highest
			// 2 - Do not use a symbol which has a known length
			if ((e1->parikhVector[iSymbol] != 0 && this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]) && ((e1->parikhVector[iSymbol] != 0 && e1->parikhVector[iSymbol] > highest) || ((e1->parikhVector[iSymbol] != 0 && e1->parikhVector[iSymbol] == highest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp]))))
			{
				idxMin = iSymbol;
				highest = e1->parikhVector[iSymbol];
				unset = false;
			}

			if ((e1->parikhVector[iSymbol] != 0 && this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]) && ((e1->parikhVector[iSymbol] != 0 && e1->parikhVector[iSymbol] < lowest) || ((e1->parikhVector[iSymbol] != 0 && e1->parikhVector[iSymbol] == lowest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp]) && (this->minRuleLengths[iSymbol] != this->maxRuleLengths[iSymbol]))))
			{
				idxMax = iSymbol;
				lowest = e1->parikhVector[iSymbol];
				unset = false;
			}
		}
	}

	if (!unset)
	{
		// Assume every other symbol only produces their minimum
		// if the minimum is unknown assume it is 1
		// if the minimum is unknown or modules only assume it is 0
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
		{
			if (iSymbol != idxMin)
			{
				min += this->minRuleLengths[iSymbol];
				countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
			}

			if (iSymbol != idxMax)
			{
				countMax -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
				max += this->minRuleLengths[iSymbol];
			}
		}

		if (e1->parikhVector[idxMin] != 0)
		{
			countMin /= e1->parikhVector[idxMin];
		}

		if (e1->parikhVector[idxMax] != 0)
		{
			countMax /= e1->parikhVector[idxMax];
		}

		min += countMin;
		max += countMax;
	}

	//// Given the total maximum rule length, the minimum individual rule length for each rule can be computed by
	//// TotalMin - Sum(Maximums except iRule)
	//// And
	//// TotalMax - Sum(Minimum except iRule)
	//// Repeat this process until convergence
	//bool valueChanged;
	//do
	//{
	//	valueChanged = false;
	//	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//	{
	//		Int32 sum = 0;

	//		for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
	//		{
	//			if (iValue != iRule)
	//			{
	//				sum += this->maxRuleLengths[iValue];
	//			}
	//		}

	//		Int32 tmp = this->minRuleLengths[iRule]; // capture the current value

	//		result = this->UpdateMinRuleLengths(iRule, this->totalMinRuleLength - sum) || result;

	//		if (tmp != this->minRuleLengths[iRule])
	//		{
	//			valueChanged = true;
	//		}
	//	}

	//	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//	{
	//		Int32 sum = 0;

	//		for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
	//		{
	//			if (iValue != iRule)
	//			{
	//				sum += this->minRuleLengths[iValue];
	//			}
	//		}

	//		Int32 tmp = this->maxRuleLengths[iRule]; // capture the current value
	//		result = this->UpdateMaxRuleLengths(iRule, this->totalMaxRuleLength - sum) || result;

	//		if (tmp != this->maxRuleLengths[iRule])
	//		{
	//			valueChanged = true;
	//		}
	//	}
	//} while (valueChanged);

	//this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2A::EstimateRuleLengths_PoG()
{
	bool result = false;
	// Find the maximum rule lengths for each individual rule by summing the proportion of growth values
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Rule lengths based on PoG");
#if _PMI_PROBLEM_VERBOSE_ >= 0
	this->OutputAnalysis();
#endif
#endif
	if (this->vocabulary->numModules > 1)
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			result = this->UpdateMinRuleLengths(iRule, Math::Max(this->minRuleLengths[iRule], this->absoluteMinRuleLength)) || result;

			Int32 max = 0; // maximum length of the complete rule
			Int32 min = 0; // maximum length of the complete rule

			for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
			{
				min += this->minPoG[iRule, iSymbol];
				max += this->maxPoG[iRule, iSymbol];
			}

			result = this->UpdateMinRuleLengths(iRule, min) || result;
			result = this->UpdateMaxRuleLengths(iRule, max) || result;
		}
	}
	else
	{
		// Will be equal to the sum of PoG for the symbol 0 since there is only one module
		Int32 sum = 0;
		for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
		{
			sum += this->maxPoG[0, iSymbol];
		}

		result = this->UpdateMinRuleLengths(0, sum) || result;
		result = this->UpdateMaxRuleLengths(0, sum) || result;
	}
	this->OutputRuleLengths();

	return result;
}

void PMIProblemV2A::ExtractVocabulary()
{
	List<Symbol^>^ added = gcnew List<Symbol^>;
	List<Symbol^>^ modules = gcnew List<Symbol^>;
	List<Symbol^>^ constants = gcnew List<Symbol^>;

	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		for each (Symbol^ s in this->evidence[iGen]->symbols)
		{
			if (!added->Contains(s))
			{
				added->Add(s);

				if (this->useStandardVocuabularySymbols)
				{
					// TODO:
					// Need to produce a standard vocbulary of symbols
					// I.e. A,B,C,D,E,F are modules
					// +,-,/,[,] are constants
					// In this way, depending on the symbol it will be added as a module or constant
				}
				else
				{	// Determine if this is a module or constant from the plant model
					// Note, this is peeking across the veil but it is justified by 
					// each test model having their own standard vocabulary
					// It does not reveal anything about the proper model


					if (iGen == this->evidence->Count - 1)
					{ // If a symbol is appearing for the first time in the last piece of evidence treat it as a constant
					  // This is mainly for sub-problems which have a module which appears at the very end
					  // Such a 'module' will never produce anything so should be treated as a constant
						constants->Add(s);
					}
					else if (this->model->vocabulary->IsModule(s))
					{
						modules->Add(s);
					}
					else
					{
						constants->Add(s);
					}
				}
			}
		}
	}

	this->vocabularyMaster = gcnew Vocabulary();

	// Add all of the modules
	for each (Symbol^ s in modules)
	{
		this->vocabularyMaster->AddSymbol(s->Value(), true);
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "]")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "[")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "}")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "{")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "+")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	for each (Symbol^ s in constants)
	{
		if (s->Value() == "-")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	// Add all of the constants except F
	for each (Symbol^ s in constants)
	{
		if ((s->Value() != "[") && (s->Value() != "]") && (s->Value() != "{") && (s->Value() != "}") && (s->Value() != "+") && (s->Value() != "-") && (s->Value() != "F"))
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	// Add F
	for each (Symbol^ s in constants)
	{
		if (s->Value() == "F")
		{
			this->vocabularyMaster->AddSymbol(s->Value(), false);
		}
	}

	//for each (Symbol^ s in constants)
	//{
	//	if ((s->Value() == "[") || (s->Value() == "]"))
	//	{
	//		this->vocabularyMaster->AddSymbol(s->Value(), false);
	//	}
	//}

	this->vocabularyMaster->IndexModulesStart = 0;
	this->vocabularyMaster->IndexModulesEnd = modules->Count - 1;
	this->vocabularyMaster->IndexConstantsStart = this->vocabularyMaster->IndexModulesEnd + 1;
	this->vocabularyMaster->IndexConstantsEnd = this->vocabularyMaster->IndexConstantsStart + (constants->Count - 1);
	this->vocabularyMaster->IndexSymbolsStart = this->vocabularyMaster->IndexModulesStart;
	this->vocabularyMaster->IndexSymbolsEnd = this->vocabularyMaster->IndexConstantsEnd;
	this->vocabularyMaster->IndexAlpha = this->vocabularyMaster->IndexConstantsEnd + 1;

	// Re-compute the Parikh vectors based on the problem's vocabulary
	for each (Word^ a in this->evidence)
	{
		this->vocabularyMaster->CompressSymbols(a);
	}

	this->vocabulary = this->vocabularyMaster;
}

bool PMIProblemV2A::IsGenerationAllConstants(Int32 Generation)
{
	Int32 iRule = 0;
	bool result = false;
	do
	{
		result = this->symbolSubsets[Generation, iRule];
		iRule++;
	} while ((!result) && (iRule < this->vocabulary->numModules));

	return (!result);
}

bool PMIProblemV2A::IsModuleInGeneration(Int32 Generation, Int32 iRule)
{
	Int32 iGen = Generation;
	bool result = this->symbolSubsets[iGen, iRule];
	return result;
}

bool PMIProblemV2A::IsModuleSolved(Int32 RuleIndex)
{
	bool result = true;

	for (size_t iGen = this->currentSet->startGeneration; iGen < this->currentSet->endGeneration; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			result = result && (this->rulesParikhTemplate[iGen, RuleIndex, iSymbol] != PMIProblem::cUnknownParikhValue);
		}
	}

	return result;
}

bool PMIProblemV2A::IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex)
{
	return this->IsModuleSolvedForSymbol(RuleIndex, SymbolIndex, this->currentSet->startGeneration, this->currentSet->endGeneration);
}

bool PMIProblemV2A::IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex, Int32 Start, Int32 End)
{
	bool result = true;

	for (size_t iGen = Start; iGen < End; iGen++)
	{
		result = result && (this->rulesParikhTemplate[iGen, RuleIndex, SymbolIndex] != PMIProblem::cUnknownParikhValue);
	}

	return result;
}

bool PMIProblemV2A::IsParikhAllConstants(array<Int32>^ PV)
{
	bool result = true;
	Int32 iSymbol = 0;

	while ((result) && (iSymbol < this->vocabulary->numModules))
	{
		result = result && (PV[iSymbol] == 0);
		iSymbol++;
	}

	return result;
}

bool PMIProblemV2A::IsProblemSetComplete(PMIProblemSet^ P)
{
	bool result = true;

	Int32 neededGen = 1;
	Int32 missingGen = 0;

	for (size_t iSymbol = 0; iSymbol < this->vocabularyMaster->numModules; iSymbol++)
	{
		if (this->symbolSubsets[P->startGeneration, iSymbol])
		{
			neededGen++;
		}
	}

	missingGen = neededGen - (P->endGeneration - P->startGeneration + 1);

	// Not enough generations in the sub-problem, so check to see if enough generations immediately following the endGeneration are completed
	if (missingGen > 0)
	{
		for (size_t iGen = P->endGeneration; iGen < P->endGeneration + missingGen; iGen++)
		{
			if (iGen <= this->generationsSolved->GetUpperBound(0))
			{
				result = result && this->generationsSolved[iGen];
			}
		}
	}
	else
	{
		result = true;
	}

	return result;
}

// Scan through the RPT and see if all the symbols are solved in the given generations
bool PMIProblemV2A::IsProblemSolved(Int32 StartGen, Int32 EndGen, Int32 StartSymbolIndex, Int32 EndSymbolIndex)
{
	bool result = true;
	for (size_t iGen = StartGen; iGen < EndGen; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = StartSymbolIndex; iSymbol <= EndSymbolIndex; iSymbol++)
			{
				result = result && (this->rulesParikhTemplate[iGen, iRule, iSymbol] != PMIProblem::cUnknownParikhValue);
			}
		}
	}

	return result;
}

bool PMIProblemV2A::IsTabu(array<Int32, 2>^ Rules)
{
	return this->IsTabu(TabuItem::ModulesOnlyIndex, Rules);
}

bool PMIProblemV2A::IsTabu(Int32 Index, array<Int32, 2>^ Rules)
{
	bool result = false;
	Int32 idx = 0;
	//this->OutputRules(Rules);

	while ((!result) && (idx < this->tabu->Count))
	{
		if (Index == this->tabu[idx]->index)
		{
			array<Int32, 2>^ tabuRules = this->tabu[idx]->rulesParikh;
			//this->OutputRules(tabuRules);
			Int32 iRule = 0;
			bool isMatch = true;
			while ((isMatch) && (iRule <= Rules->GetUpperBound(0)))
			{
				Int32 iSymbol = 0;

				while ((isMatch) && (iSymbol <= Rules->GetUpperBound(1)))
				{
					isMatch = (Rules[iRule, iSymbol] == tabuRules[iRule, iSymbol]);
					iSymbol++;
				}
				iRule++;
			}
			//Console::WriteLine("isMatch = " + isMatch);
			//Console::ReadLine();
			result = isMatch;
		}
		idx++;
	}
	return result;
}

bool PMIProblemV2A::IsTabu(Int32 Index, array<Int32>^ Lengths)
{
	bool result = false;
	if (Lengths!= nullptr)
	{
		Int32 idx = 0;
		//this->OutputRules(Rules);

		while ((!result) && (idx < this->tabu->Count))
		{
			if (Index == this->tabu[idx]->index)
			{
				array<Int32>^ tabuLengths = this->tabu[idx]->ruleLengths;
				Int32 iRule = 0;
				bool isMatch = true;
				while ((isMatch) && (iRule <= Lengths->GetUpperBound(0)))
				{
					isMatch = (Lengths[iRule] == tabuLengths[iRule]);
					iRule++;
				}
				//Console::WriteLine("isMatch = " + isMatch);
				//Console::ReadLine();
				result = isMatch;
			}
			idx++;
		}
	}
	return result;
}

bool PMIProblemV2A::IsTabu(Int32 Index, List<ProductionRule^>^ Rules)
{
	bool result = false;
	Int32 idx = 0;
	//this->OutputRules(Rules);

	while ((!result) && (idx < this->tabu->Count))
	{
		if (Index == this->tabu[idx]->index)
		{
			Int32 iRule = 0;
			bool isMatch = true;

			do
			{
				isMatch = (Rules[iRule] == this->tabu[idx]->rules[iRule]);
				iRule++;
			} while ((isMatch) && (iRule < Rules->Count));

			//Console::WriteLine("isMatch = " + isMatch);
			//Console::ReadLine();
			result = isMatch;
		}
		idx++;
	}
	return result;
}

//bool PMIProblemV2A::IsTabu(array<Int32>^ Solution)
//{
//	Int32 iTabu = 0;
//	bool result = false;
//
//	while ((iTabu < this->tabuSolutions->Count) && (!result))
//	{
//		Int32 iValue = 0;
//		bool isMatch = true;
//		array<Int32>^ tabuSolution = this->tabuSolutions[iTabu];
//
//		do
//		{
//			if (Solution[iValue] != tabuSolution[iValue])
//			{
//				isMatch = false;
//			}
//			iValue++;
//		} while ((iValue <= Solution->GetUpperBound(0)) && (isMatch));
//
//		if (isMatch)
//		{
//			result = true;
//		}
//
//		iTabu++;
//	}
//
//	return result;
//}

bool PMIProblemV2A::IsWordAllConstants(Word^ W)
{
	bool result = true;
	Int32 iSymbol = 0;

	while ((result) && (iSymbol < W->Count))
	{
		if (!this->vocabulary->IsConstant(W[iSymbol]))
		{
			result = false;
		}

		iSymbol++;
	}

	return result;
}

void PMIProblemV2A::ModifyProblem(Int32 ConstantIndex)
{
#if _PMI_PROBLEM_VERBOSE_ >= 1
	Console::WriteLine();
	Console::WriteLine("*** MODIFYING PROBLEM SPACE ***");
#endif

	// Modify the vocabulary
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Modifying Vocabulary");
#endif
	this->vocabulary = gcnew Vocabulary();
	List<Symbol^>^ filter = gcnew List<Symbol^>;
	
	for (size_t iSymbol = 0; iSymbol < this->vocabularyMaster->NumSymbols(); iSymbol++)
	{
		if (iSymbol > this->vocabularyMaster->IndexModulesEnd + ConstantIndex)
		{
			filter->Add(this->vocabularyMaster->Symbols[iSymbol]);
		}
		else
		{
			if (iSymbol < this->vocabularyMaster->numModules)
			{
				this->vocabulary->AddSymbol(this->vocabularyMaster->Symbols[iSymbol], true);
			}
			else
			{
				this->vocabulary->AddSymbol(this->vocabularyMaster->Symbols[iSymbol], false);
			}
		}
	}

	this->vocabulary->IndexModulesStart = 0;
	this->vocabulary->IndexModulesEnd = this->vocabularyMaster->numModules - 1;
	this->vocabulary->IndexConstantsStart = this->vocabulary->IndexModulesEnd + 1;
	this->vocabulary->IndexConstantsEnd = this->vocabulary->IndexConstantsStart + (ConstantIndex - 1);
	this->vocabulary->IndexSymbolsStart = this->vocabulary->IndexModulesStart;
	this->vocabulary->IndexSymbolsEnd = this->vocabulary->IndexConstantsEnd;
	this->vocabulary->IndexAlpha = this->vocabulary->IndexConstantsEnd + 1;

	// Modify the evidence
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Modifying Evidence");
#endif
	this->evidence = gcnew List<Word^>;
	for (size_t iGen = 0; iGen < this->evidenceMaster->Count; iGen++)
	{
		Word^ w = gcnew Word(this->evidenceMaster[iGen]);
		w->FilterOutSymbol(filter);
		this->vocabulary->CompressSymbols(w);
		this->evidence->Add(w);
	}

	// Modify the min/max PoG
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Modify PoG");
#endif
	this->PoG = gcnew array<Int32, 3>(this->evidence->Count - 1, this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->totalPoG = gcnew array<Int32>(this->vocabulary->NumSymbols());
	this->minPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->maxPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	if (this->constantIndex == 0)
	{
		for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
		{
			this->totalPoG[iRule] = this->totalPoGMaster[iRule];
			for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
			{
				//if (this->IsModuleInSet(iRule))
				//{
					this->minPoG[iRule, iSymbol] = this->minPoGMaster[iRule, iSymbol];
					this->maxPoG[iRule, iSymbol] = this->maxPoGMaster[iRule, iSymbol];
				//}
				//else
				//{
				//	this->minPoG[iRule, iSymbol] = 0;
				//	this->maxPoG[iRule, iSymbol] = 0;
				//}
			}
		}
	}
	else
	{
		for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
		{
			this->totalPoG[iRule] = this->totalPoGMaster[iRule];
			for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
			{
				//if (this->IsModuleInSet(iRule))
				//{
				//	this->minPoG[iRule, iSymbol] = 0;
				//	this->maxPoG[iRule, iSymbol] = 0;
				//}
				if (this->IsModuleSolvedForSymbol(iRule, iSymbol, 0, this->generations))
				{// plus since the right side is a count and the left is an index
					this->minPoG[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
					this->maxPoG[iRule, iSymbol] = this->rulesParikhTemplate[this->currentSet->startGeneration, iRule, iSymbol];
				}
				else
				{
					this->minPoG[iRule, iSymbol] = this->minPoGMaster[iRule, iSymbol];
					this->maxPoG[iRule, iSymbol] = this->maxPoGMaster[iRule, iSymbol];
				}
			}
		}
	}

	// Modify the Min/Max Rule Lengths
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Modify Min/Max Rule Lengths");
#endif
	this->maxRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	if (this->constantIndex == 0)
	{
		for (size_t iRule = 0; iRule <= this->minRuleLengths->GetUpperBound(0); iRule++)
		{
			//if (this->IsModuleInSet(iRule))
			//{
			//	this->minRuleLengths[iRule] = 0;
			//	this->maxRuleLengths[iRule] = 0;
			//}
			if (this->knownRuleLengthsIndexed[this->constantIndex, iRule] == PMIProblem::cUnknownRuleLength)
			{
				this->minRuleLengths[iRule] = 0;
				this->maxRuleLengths[iRule] = 999999999;
			}
			else
			{
				this->minRuleLengths[iRule] = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
				this->maxRuleLengths[iRule] = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
			}
		}

		this->totalMinRuleLength = 0;
		this->totalMaxRuleLength = 999999999;
	}
	else
	{
		this->totalMinRuleLength = 0;
		this->totalMaxRuleLength = 0;
		for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
		{
			if (this->knownRuleLengthsIndexed[this->constantIndex, iRule] == PMIProblem::cUnknownRuleLength)
			{
				for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
				{
					this->minRuleLengths[iRule] += this->minPoG[iRule, iSymbol];
					this->maxRuleLengths[iRule] += this->maxPoG[iRule, iSymbol];
				}
			}
			else
			{// TODO: Add master knownlengthsindexed list
				this->minRuleLengths[iRule] = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
				this->maxRuleLengths[iRule] = this->knownRuleLengthsIndexed[this->constantIndex, iRule];
			}

			this->totalMinRuleLength += this->minRuleLengths[iRule];
			this->totalMaxRuleLength += this->maxRuleLengths[iRule];
		}
	}

	// Modify the Head/Tail/Fragments
	// TODO: Should I be recomputing the head/tail???
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		Word^ w = gcnew Word(this->ruleHeadMaster[iRule]->symbols);
		w->FilterOutSymbol(filter);
		this->vocabulary->CompressSymbols(w);
		this->ruleHead[iRule] = w;

		w = gcnew Word(this->ruleTailMaster[iRule]->symbols);
		w->FilterOutSymbol(filter);
		this->vocabulary->CompressSymbols(w);
		this->ruleTail[iRule] = w;

		w = gcnew Word(this->ruleFragmentsMaster[iRule]->symbols);
		w->FilterOutSymbol(filter);
		this->vocabulary->CompressSymbols(w);
		this->ruleFragments[iRule] = w;
	}

	this->EstimateRuleLengths(false, false); // this estimation needs to happen because rule lengths are critical to the lexicon production
	this->SynchronizeAnalysis();

#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
	if (!this->flagAnalysisFailed)
	{
		// Modify the lexicon
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
		Console::WriteLine("Modifying Lexicon");
#endif	
		this->lexicon = gcnew Lexicon(this->generations, this->vocabulary, this->minRuleLengths, this->maxRuleLengths, this->ruleHead, this->ruleTail, this->ruleFragments, this->totalPoG, this->minPoG, this->maxPoG);
		this->lexicon->FilterMT(this->lexiconMaster);
	}
#endif

}

void PMIProblemV2A::ModifyVocabulary(Symbol^ S)
{
	this->vocabulary->SwitchSymbol(S);
}

void PMIProblemV2A::OutputRules(array<Int32, 2>^ R)
{
#if _PMI_PROBLEM_DISPLAY_RULES_ >= 1
	Console::WriteLine("Parikh Rules");
	Console::Write("  ");
	for (size_t iSymbol = 0; iSymbol < this->vocabulary->Symbols->Count; iSymbol++)
	{
		Console::Write(" " + this->vocabulary->Symbols[iSymbol] + "  ");
	}
	Console::WriteLine();
	for (size_t iRule = 0; iRule <= R->GetUpperBound(0); iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule]);
		for (size_t iSymbol = 0; iSymbol <= R->GetUpperBound(1); iSymbol++)
		{
			Console::Write(" " + R[iRule, iSymbol].ToString(L"D3"));
		}
		Console::WriteLine();
	}
	Console::WriteLine();

#if _PMI_PROBLEM_BREAKS_ >= 1
	Console::WriteLine("Press enter to continue.");
	Console::ReadLine();
#endif
#endif
}

void PMIProblemV2A::OutputRules(List<ProductionRule^>^ R)
{
	for (size_t iRule = 0; iRule < R->Count; iRule++)
	{
		Console::Write(R[iRule]->predecessor->ToString() + " => ");

		for (size_t iSymbol = 0; iSymbol < R[iRule]->successor->Count; iSymbol++)
		{
			Console::Write(R[iRule]->successor[iSymbol]);
		}
		Console::WriteLine();
	}
	Console::WriteLine();

#if _PMI_PROBLEM_BREAKS_ >= 1
	Console::WriteLine("Press enter to continue.");
	Console::ReadLine();
#endif
}

void PMIProblemV2A::RollbackAnalysisForTabu(TabuItem^ T)
{
	this->AddTabuItem(T);
	this->RollbackKnownRuleLengths(T->index);
}

void PMIProblemV2A::RollbackKnownRuleLengths(Int32 Index)
{
#if _PMI_PROBLEM_PARTIAL_ROLLBACK_ > 0
	for (size_t idx = Index; idx <= this->knownRuleLengthsIndexed->GetUpperBound(0); idx++)
	{
		for (size_t iRule = 0; iRule <= this->knownRuleLengthsIndexed->GetUpperBound(1); iRule++)
		{
			this->knownRuleLengthsIndexed[idx, iRule] = -1;
		}
	}
#else
	for (size_t idx = 0; idx <= this->knownRuleLengthsIndexed->GetUpperBound(0); idx++)
	{
		for (size_t iRule = 0; iRule <= this->knownRuleLengthsIndexed->GetUpperBound(1); iRule++)
		{
			this->knownRuleLengthsIndexed[idx, iRule] = -1;
		}
	}
#endif // _PMI_PROBLEM_PARTIAL_ROLLBACK_ > 0

}

void PMIProblemV2A::RollbackPartialSolution()
{
	for (size_t iGen = this->currentSet->startGeneration; iGen < this->currentSet->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol];
			}
		}
	}

	this->CheckForSolvedGenerations(this->currentSet->startGeneration, this->currentSet->endGeneration);
}

void PMIProblemV2A::RollbackProblemSet()
{
	for (size_t iRule = 0; iRule <= this->solvedModules->GetUpperBound(0); iRule++)
	{
		this->solvedModules[iRule] = this->solvedModulesMaster[iRule];
	}

	for (size_t iRule = 0; iRule <= this->ruleFragmentsMaster->GetUpperBound(0); iRule++)
	{
		this->ruleFragmentsMaster[iRule]->symbols->Clear();
		this->ruleFragmentsMaster[iRule]->symbols->AddRange(this->ruleFragmentsOriginal[iRule]->symbols);
	}

#if _PMI_PROBLEM_PARTIAL_ROLLBACK_ == 0
	for (size_t iGen = this->currentSet->startGeneration; iGen <= this->currentSet->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			if ((this->IsModuleInSet(iRule)) && (!this->solvedModules[iRule]))
			{
				for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
				{
					this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol];
				}
			}
		}
	}
#elif _PMI_PROBLEM_PARTIAL_ROLLBACK_ == 1
	for (size_t iGen = this->currentSet->startGeneration; iGen <= this->currentSet->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			if ((this->IsModuleInSet(iRule)) && (!this->solvedModules[iRule]))
			{
				if (this->constantIndex == 0)
				{
					for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
					{
						this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol];
					}
				}
				else
				{
					Int32 iSymbol = this->vocabulary->numModules + this->constantIndex - 1;
					this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol];

					if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "[") &&
						(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "]"))
					{
						this->rulesParikhTemplate[iGen, iRule, iSymbol-1] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol-1];
					}
					else if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "{") &&
						(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "}"))
					{
						this->rulesParikhTemplate[iGen, iRule, iSymbol-1] = this->rulesParikhTemplateMaster[iGen, iRule, iSymbol-1];
					}
				}
			}
		}
	}
#endif

	this->RollbackSpecial();

	//this->OutputRPT();
	this->CheckForSolvedGenerations(this->currentSet->startGeneration, this->currentSet->endGeneration);
}

void PMIProblemV2A::RollbackSpecial()
{
}

void PMIProblemV2A::Solve()
{
	bool done;
	bool change;
	List<Int32>^ toRemove = gcnew List<Int32>;
	Int32 attempts = 1;

	auto startTime = Time::now();

	bool incompleteRestricted = true;
	do
	{
		Int32 priority = 0;
		bool found = false;
		bool anySetSolved = false;
		bool anySetAttempted = false;

		this->UpdateSolvedProblems();
		this->UpdateCompleteProblems();

		// Loop through every problem set
		for (int iSet = this->problemSets->Count - 1; iSet >= 0; iSet--)
		{
			this->currentSet = this->problemSets[iSet];

			if (this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabularyMaster->IndexSymbolsStart, this->vocabularyMaster->IndexSymbolsEnd))
			{
				Console::WriteLine("Problem set #" + iSet + " is solved.");
			}
			else if (!this->IsProblemSetComplete(this->currentSet) && (iSet != this->problemSets->Count - 1) && (incompleteRestricted))
			{ // the last problem set is always considered complete
				Console::WriteLine("Problem set #" + iSet + " is incomplete");
			}
			else
			{
				anySetAttempted = true;
				Console::WriteLine("Problem set #" + iSet + " is complete and unsolved.");
				this->tabu->Clear();
				bool solved = this->SolveProblemSet();
				if (solved)
				{// any modules in the problem set are now solved and should not be modified by other problem sets
					this->UpdateSolvedModules();
				}
				anySetSolved = anySetSolved || solved;
			}
		}
		// allow incomplete problems to be attempted when no attempt was made on any set
		// TODO: examine the definition of 'incomplete'
		if (!anySetAttempted)
		{
			if (incompleteRestricted)
			{
				incompleteRestricted = false;
				anySetSolved = true;
			}
		}

		done = ((this->IsSolved()) || (!anySetSolved));
	} while (!done);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	if (this->IsSolved())
	{
		this->solved = true;
		this->CreateModelFromRPT();
		this->CompressSolution();
		this->matched = this->model->IsMatch(this->solution);
		this->OutputModel();
	}
	else
	{
		this->solved = false;
		this->matched = false;
		Console::WriteLine("*** No model found ***");
		Console::WriteLine();
	}
}

void PMIProblemV2A::Solve_Special()
{
	bool done;
	bool change;
	List<Int32>^ toRemove = gcnew List<Int32>;
	Int32 attempts = 1;

	auto startTime = Time::now();

	bool incompleteRestricted = true;
	do
	{
		Int32 priority = 0;
		bool found = false;
		bool anySetSolved = false;
		bool anySetAttempted = false;

		this->UpdateSolvedProblems();
		this->UpdateCompleteProblems();

		// Loop through every problem set
		for (int iSet = this->problemSets->Count - 1; iSet >= 0; iSet--)
		{
			this->currentSet = this->problemSets[iSet];

			if (this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabularyMaster->IndexSymbolsStart, this->vocabularyMaster->IndexSymbolsEnd))
			{
				Console::WriteLine("Problem set #" + iSet + " is solved.");
			}
			else if (!this->IsProblemSetComplete(this->currentSet) && (iSet != this->problemSets->Count - 1) && (incompleteRestricted))
			{ // the last problem set is always considered complete
				Console::WriteLine("Problem set #" + iSet + " is incomplete");
			}
			else
			{
				anySetAttempted = true;
				Console::WriteLine("Problem set #" + iSet + " is complete and unsolved.");
				this->tabu->Clear();
				bool solved = this->SolveProblemSet_Special();
				if (solved)
				{// any modules in the problem set are now solved and should not be modified by other problem sets
					this->UpdateSolvedModules();
				}
				anySetSolved = anySetSolved || solved;
			}
		}
		// allow incomplete problems to be attempted when no attempt was made on any set
		// TODO: examine the definition of 'incomplete'
		if (!anySetAttempted)
		{
			if (incompleteRestricted)
			{
				incompleteRestricted = false;
				anySetSolved = true;
			}
		}

		done = ((this->IsSolved()) || (!anySetSolved));
	} while (!done);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

//	this->solveTime = ms.count();

	if (this->IsSolved())
	{
		this->solved = true;
		this->CreateModelFromRPT();
		this->CompressSolution();
		this->matched = this->model->IsMatch(this->solution);
		this->OutputModel();
	}
	else
	{
		this->solved = false;
		this->matched = false;
		Console::WriteLine("*** No model found ***");
		Console::WriteLine();
	}
}

bool PMIProblemV2A::SolveProblemSet()
{
	Int32 attempts = 0;
	bool done = false;
	this->constantIndex = 0;
	this->currentTabu = gcnew TabuItem(-1);
	bool totalFailure = false;

	for (size_t iRule = 0; iRule <= this->solvedModules->GetUpperBound(0); iRule++)
	{
		this->solvedModulesMaster[iRule] = this->solvedModules[iRule];
	}

	do
	{
		// If is a modules only problem then the minimum length is 0
		// Otherwise 1
		if (this->constantIndex == this->vocabularyMaster->numConstants)
		{
			this->absoluteMinRuleLength = 1;
		}
		else
		{
			this->absoluteMinRuleLength = 0;
		}

		if (this->constantIndex == 0)
		{
			// Really this means it will only record the solution space sizes of the last problem solved but whatever
			this->solutionSpaceSizes = gcnew List<UInt64>;
		}

		// Modify the Vocabulary, Lexicon and Evidence for the simulated problem
		this->flagAnalysisFailed = false;
		this->ModifyProblem(this->constantIndex);
		//this->OutputAnalysis();

		bool code = false;
		bool problemSolved = true;

#if _PMI_PROBLEM_CHEAT_ == 0
		if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "[") &&
			(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "]"))
		{
			this->OutputAnalysis();
			problemSolved = this->Assess_BranchingSymbol();
		}
		else if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "{") &&
			(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "}"))
		{
			problemSolved = this->Assess_BranchingSymbol();
		}
		// Run the analysis on the simulated problem
		else if (!this->flagAnalysisFailed)
		{
			this->Analyze();
			this->OutputAnalysis();
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Analysis step complete. Flag = " + this->flagAnalysisFailed);
#endif
		}

#else
		// Run the analysis on the simulated problem
		if (!this->flagAnalysisFailed)
		{
			this->Analyze();
			this->OutputAnalysis();
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Analysis step complete. Flag = " + this->flagAnalysisFailed);
#endif
		}
#endif

		if ((this->flagAnalysisFailed) || (!problemSolved))
		{
			code = false;
			this->flagAnalysisFailed = true;
		}
		else
		{
			code = true;
			// Compute solution space size
			UInt64 tmp = this->ComputeSolutionSpaceSize();
			Console::WriteLine("Solution Space Size = " + tmp);
			this->solutionSpaceSizes->Add(tmp);
		}
		
		// Update whether any problems are fully solved by the analysis
		if ((code) && (!this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, 0, this->vocabularyMaster->numModules + this->constantIndex - 1)))
		{
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Problem is unsolved. Searching for solution");
#endif
			//this->OutputRPT();
			this->OutputAnalysis();
 			code = this->SolveProblem();
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Search complete. Code = " + code);
#endif
		}
#if _PMI_PROBLEM_KNOWN_CONSTANTS == 0
		else if ((code) && (this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, 0, this->vocabularyMaster->numModules + this->constantIndex - 1)))
		{
			Console::WriteLine("Problem is supposedly solved.");
			//this->OutputRPT();
			float fitness;
			List<ProductionRule^>^ rules = this->CreateRulesFromRPT();
			fitness = this->AssessProductionRules(rules);

			//fitness = this->Assess(nullptr);
			
			if (fitness != 0)
			{
				code = false;
				this->flagAnalysisFailed = true;
			}
		}
#endif
		if (this->vocabularyMaster->numModules + this->constantIndex < this->vocabularyMaster->NumSymbols())
		{
			// For now code is boolean, so if true then proceed
			// TODO: only compute fragments for PMITs that do not produce explicit rules
			// I.e. PMIT-D produces its own fragments
			if (code)
			{
				// Compute fragments
				code = this->ComputeFragmentsFromRPT();

#if _PMI_PROBLEM_DISPLAY_FRAGMENT_PARTIAL_SOLUTION_ >= 0
				for (size_t iRule = 0; iRule <= this->ruleFragmentsMaster->GetUpperBound(0); iRule++)
				{
					Console::Write(this->vocabulary->Symbols[iRule] + " -> ");
					Console::Write(this->ruleFragmentsMaster[iRule]->ToString());
					Console::WriteLine();
				}
				//Console::ReadLine();
#endif
				//code = true;
				//this->OutputRPT();
			}

			// Align partial solution when absolutely rule-based (i.e. PMIT-D... PMIT-Lex?) with master fragment
			if (code)
			{
				this->ComputeFragmentsFromPartialSolution();
			}
		}

		if (!code)
		{
#if _PMI_PROBLEM_VERBOSE_ >= 0
			Console::WriteLine("Search failed. Resetting.");
			this->OutputAnalysis();
			//this->OutputRPT();
#endif
			// failing on the first search
			if (this->currentTabu->index == -1)
			{
				totalFailure = true;
			}
			// If the analysis failed then the previous solution is tabu
			//if ((this->flagAnalysisFailed) && (this->constantIndex > 0))
			else if ((this->flagAnalysisFailed) && (this->currentTabu != nullptr) && (this->currentTabu->index >= 0))
			{
				this->RollbackAnalysisForTabu(this->currentTabu);
				//this->AddTabuItem(this->currentTabu);
			}

			this->RollbackProblemSet();
			this->constantIndex = 0;
			attempts++;
			this->flagAnalysisFailed = false;
		}
		else
		{
			if (this->vocabularyMaster->numModules + this->constantIndex < this->vocabularyMaster->NumSymbols())
			{
#if _PMI_PROBLEM_DISPLAY_RULES_ >= 2
				this->OutputRPT();
#endif
#if _PMI_PROBLEM_VERBOSE_ >= 0
				Console::WriteLine("Problem solved. Advancing to Symbol " + this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + this->constantIndex]->ToString() + " (" + (this->constantIndex + 1) + ")");
#endif
				this->constantIndex++;
				attempts = 0;
			}
			else
			{
#if _PMI_PROBLEM_VERBOSE_ >= 0
				Console::WriteLine("Final problem solved.");
#endif
				done = true;
			}
		}
	} while ((!done) && (attempts < PMIProblemV2A::cMaxAttempts) && (!totalFailure));

	return (this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabularyMaster->IndexSymbolsStart, this->vocabularyMaster->IndexSymbolsEnd));
}

bool PMIProblemV2A::SolveProblemSet_Special()
{
	Int32 attempts = 0;
	bool done = false;
	this->constantIndex = 0;
	this->currentTabu = gcnew TabuItem(-1);
	bool totalFailure = false;

	for (size_t iRule = 0; iRule <= this->solvedModules->GetUpperBound(0); iRule++)
	{
		this->solvedModulesMaster[iRule] = this->solvedModules[iRule];
	}

	do
	{
		// If is a modules only problem then the minimum length is 0
		// Otherwise 1
		if (this->constantIndex == this->vocabularyMaster->numConstants)
		{
			this->absoluteMinRuleLength = 1;
		}
		else
		{
			this->absoluteMinRuleLength = 0;
		}

		if (this->constantIndex == 0)
		{
			// Really this means it will only record the solution space sizes of the last problem solved but whatever
			this->solutionSpaceSizes = gcnew List<UInt64>;
		}

		// Modify the Vocabulary, Lexicon and Evidence for the simulated problem
		this->flagAnalysisFailed = false;
		this->ModifyProblem(this->constantIndex);
		//this->OutputAnalysis();

		bool code = false;
		bool problemSolved = true;

#if _PMI_PROBLEM_CHEAT_ == 0
		if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "[") &&
			(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "]"))
		{
			this->OutputAnalysis();
			problemSolved = this->Assess_BranchingSymbol();
		}
		else if ((this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]->Value() == "{") &&
			(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 2]->Value() == "}"))
		{
			problemSolved = this->Assess_BranchingSymbol();
		}
		// Run the analysis on the simulated problem
		else if (!this->flagAnalysisFailed)
		{
			this->Analyze();
			this->OutputAnalysis();

#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Analysis step complete. Flag = " + this->flagAnalysisFailed);
#endif
		}

#else
		// Run the analysis on the simulated problem
		if (!this->flagAnalysisFailed)
		{
			this->Analyze();
			this->OutputAnalysis();
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine("Analysis step complete. Flag = " + this->flagAnalysisFailed);
#endif
		}
#endif

		if ((this->flagAnalysisFailed) || (!problemSolved))
		{
			code = false;
			this->flagAnalysisFailed = true;
		}
		else
		{
			code = true;
			// Compute solution space size
			UInt64 tmp = this->ComputeSolutionSpaceSize();
			Console::WriteLine("Solution Space Size = " + tmp);
			this->solutionSpaceSizes->Add(tmp);
		}

		//for (size_t j = 0; j < this->model->rules->Count; j++)
		//{
		//	for (size_t iRule = 0; iRule < this->vocabulary->NumSymbols(); iRule++)
		//	{
		//		if (this->vocabulary->Symbols[iRule]->Value() == this->model->rules[j]->predecessor->Value())
		//		{
		//			if (j < 4)
		//			{
		//				this->minRuleLengths[iRule] = 1;
		//				this->maxRuleLengths[iRule] = this->model->rules[j]->successor->Count * 2;
		//				this->knownRuleLengths[iRule] = -1;
		//				this->knownRuleLengthsIndexed[0, iRule] = -1;
		//			}
		//			else
		//			{
		//				this->minRuleLengths[iRule] = 1;
		//				this->maxRuleLengths[iRule] = 1;
		//				this->knownRuleLengths[iRule] = 1;
		//				this->knownRuleLengthsIndexed[0, iRule] = 1;
		//			}

		//		}
		//	}
		//}

			//this->OutputRPT();
			auto startTime = Time::now();
		
			this->OutputAnalysis();
			code = this->SolveProblem();

			auto endTime = Time::now();

			std::chrono::duration<double> duration = endTime - startTime;
			std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

			this->solveTime = ms.count();
			this->analysisTime = 0;


		// For now code is boolean, so if true then proceed
		//if (code)
		//{
		//	// Compute fragments
		//	code = this->ComputeFragmentsFromRPT();
		//	//code = true;
		//	//this->OutputRPT();
		//}

		if (!code)
		{
#if _PMI_PROBLEM_VERBOSE_ >= 0
			Console::WriteLine("Search failed. Resetting.");
			this->OutputAnalysis();
			//this->OutputRPT();
#endif
			// failing on the first search
			if (this->currentTabu->index == -1)
			{
				totalFailure = true;
			}
			// If the analysis failed then the previous solution is tabu
			//if ((this->flagAnalysisFailed) && (this->constantIndex > 0))
			else if ((this->flagAnalysisFailed) && (this->currentTabu != nullptr) && (this->currentTabu->index >= 0))
			{
				this->RollbackAnalysisForTabu(this->currentTabu);
				//this->AddTabuItem(this->currentTabu);
			}

			this->RollbackProblemSet();
			this->constantIndex = 0;
			attempts++;
			this->flagAnalysisFailed = false;
		}
		else
		{
			if (this->vocabularyMaster->numModules + this->constantIndex < this->vocabularyMaster->NumSymbols())
			{
#if _PMI_PROBLEM_DISPLAY_RULES_ >= 2
				this->OutputRPT();
#endif
#if _PMI_PROBLEM_VERBOSE_ >= 0
				Console::WriteLine("Problem solved. Advancing to Symbol " + this->vocabularyMaster->Symbols[this->vocabularyMaster->numModules + this->constantIndex]->ToString() + " (" + (this->constantIndex + 1) + ")");
#endif
				this->constantIndex++;
				attempts = 0;
			}
			else
			{
#if _PMI_PROBLEM_VERBOSE_ >= 0
				Console::WriteLine("Final problem solved.");
#endif
				done = true;
			}
		}
	} while ((!done) && (attempts < PMIProblemV2A::cMaxAttempts) && (!totalFailure));

	return (this->IsProblemSolved(this->currentSet->startGeneration, this->currentSet->endGeneration, this->vocabularyMaster->IndexSymbolsStart, this->vocabularyMaster->IndexSymbolsEnd));
}

bool PMIProblemV2A::SolveProblem()
{
	bool result = true;
#if _PMI_PROBLEM_SEARCH_GA_ > 0
	SolutionType^ S = this->SolveProblemGA();
#else
	SolutionType^ S = this->SolveProblemBF();
#endif

	// Check the top solution.
	// If fitness = 0, then verify it
	Int32 iSolution = 0;

	do
	{
		PMISolution^ s = gcnew PMISolution(S->solutions[iSolution], S->fitnessValues[iSolution]);
		result = this->AnalyzeSolution(s);
		iSolution++;
	} while ((!result) && (iSolution <= S->solutions->GetUpperBound(0)) && (S->fitnessValues[iSolution] == 0));

	if (!result)
	{
		this->RollbackProblemSet();
	}

	return result;
}

bool PMIProblemV2A::SolveForModules()
{
	bool result = false;
	float fitness = 0.0;

	SolutionType^ S = this->SolveProblemGA();

	array<Int32>^ solution;
	Int32 iSolution = 0;
	bool done = false;

	do
	{
		// TODO: Rather than storing the raw solution store it as rules
		// TODO: Only store unique rule solutions
		PMISolution^ s = gcnew PMISolution(S->solutions[iSolution], S->fitnessValues[iSolution]);

		if ((S->fitnessValues[iSolution] == 0) || (iSolution == 0))
		{
			array<Int32, 2>^ rules = this->Assess_CreateParikhRulesStep_Modules(s->solution);

			// If the top solution does not have zero fitness, then perform error analysis
			if (S->fitnessValues[iSolution] > 0)
			{
				this->ErrorAnalysis(rules);
			}

			if (this->VerifyOrderScan_Modules(rules))
			{
				this->currentProblem->fullSolutions->Add(s);
				result = true;
				done = true;
			}
			else
			{
				this->currentProblem->tabu->Add(gcnew TabuItem(TabuItem::ModulesOnlyIndex, rules));
				result = false;
			}
		}
		else if (S->fitnessValues[iSolution] < this->currentProblem->NumGenerations())
		{
			this->currentProblem->partialSolutions->Add(s);
			done = true;
		}

		iSolution++;
	} while ((!done) && (iSolution < S->solutions->GetUpperBound(0)));

	return result;
}

bool PMIProblemV2A::SolveForConstants()
{
	float fitness = 0.0;
	bool result = true;

	if (this->vocabulary->numConstants > 0)
	{
		array<Int32>^ solution;
		GeneticAlgorithmPMIT^ ai;
		GeneticAlgorithmConfiguration<Int32>^ configuration;
		GenomeConfiguration<Int32>^ genomeConfig;
		GenomeFactoryInt^ factory;

		Int32 iSymbol = 0;

		while ((iSymbol < this->vocabulary->numConstants) && (result))
		{
			this->currentProblem->constantIndex = iSymbol;

			if (!this->RPT_IsConstantSolved(this->currentProblem->startGeneration, this->currentProblem->endGeneration, this->currentProblem->constantIndex + this->vocabulary->IndexConstantsStart))
			{
				genomeConfig = this->CreateGenome();
				configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
				configuration->populationSizeInit = 20;
				configuration->populationSizeMax = 20;
				configuration->crossoverWeight = 0.80;
				configuration->mutationWeight = 0.05;
				configuration->preferHighFitness = false;
				ai = gcnew GeneticAlgorithmPMIT(configuration);
				ai->problem = this;
				ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(100000, 2));
				ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0.0));
				ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(50000));
				factory = gcnew GenomeFactoryInt(genomeConfig);
				ai->factory = factory;

				array<array<Int32>^>^ solutions = ai->SolveAllSolutions();
				array<float>^ fitnessValues = ai->FitnessValues();

				PMISolution^ s = gcnew PMISolution(solutions[0], fitnessValues[0]);
				array<Int32, 2>^ rules = this->Assess_CreateParikhRulesStep_Constants(s->solution);

				if (s->fitness == 0.0)
				{
					//Console::WriteLine("(Constants Template) F = " + s->fitness.ToString(L"F5"));
					if (this->VerifyOrderScan_Constants(rules))
					{
						//Console::WriteLine("Constants Template Verified");
						this->CapturePartialSolution(rules);
						result = true;
					}
					else
					{
						this->currentProblem->tabu->Add(gcnew TabuItem(this->currentProblem->constantIndex, rules));
						result = false;
					}
				}
				else if (fitness < this->generations - 1)
				{
					this->currentProblem->tabu->Add(gcnew TabuItem(this->currentProblem->constantIndex, rules));
					result = false;
				}
				else
				{
					this->currentProblem->tabu->Add(gcnew TabuItem(this->currentProblem->constantIndex, rules));
					result = false;
				}
			}
			iSymbol++;
		}
	}

	return result;
}

bool PMIProblemV2A::SolveForOrder()
{
	return true;
}

void PMIProblemV2A::StripConstants(Word^ W)
{
	this->StripConstants(W, nullptr);
}

void PMIProblemV2A::StripConstants(Word^ E1, Word^ E2)
{
	Int32 idx = 0;
	while (!this->vocabularyMaster->IsModule(E1->symbols[idx]))
	{
		idx++;
	}
	E1->symbols->RemoveRange(0, idx);
	if (E2 != nullptr)
	{
		E2->symbols->RemoveRange(0, idx);
	}

	idx = E1->symbols->Count - 1;
	Int32 count = 0;
	while (!this->vocabularyMaster->IsModule(E1->symbols[idx]))
	{
		idx--;
		count++;
	}
	E1->symbols->RemoveRange(idx+1, count);
	if (E2 != nullptr)
	{
		E2->symbols->RemoveRange(E2->symbols->Count - count, count);
	}

	//Console::WriteLine(E1->ToString());
	//if (E2 != nullptr)
	//{
	//	Console::WriteLine(E2->ToString());
	//}
}

void PMIProblemV2A::SynchronizeAnalysis()
{
	Int32 symbolEnd;
	Int32 genStart;
	Int32 genEnd;

	if (this->constantIndex >= 0)
	{
		symbolEnd = this->vocabulary->numModules + this->constantIndex - 1;
		genStart = this->currentSet->startGeneration;
		genEnd = this->currentSet->endGeneration;
	}
	else
	{
		symbolEnd = this->vocabulary->NumSymbols() - 1;
		genStart = 0;
		genEnd = this->generations;
	}

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		Int32 ruleLength = 0;
		List<Int32>^ PoG = gcnew List<Int32>;

		Int32 iGen = genStart;
		bool isSolved = true;

		while ((isSolved) && (iGen < genEnd))
		{
			Int32 iSymbol = 0;
			while ((isSolved) && (iSymbol <= symbolEnd))
			{
				if (this->rulesParikhTemplate[iGen, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
				{
					isSolved = false;
				}

				if (iGen == genStart)
				{
					ruleLength += this->rulesParikhTemplate[iGen, iRule, iSymbol];
					PoG->Add(this->rulesParikhTemplate[iGen, iRule, iSymbol]);
				}

				iSymbol++;
			}
			iGen++;
		}

		if ((isSolved) && (this->IsModuleInSet(iRule)))
		{
			if (this->constantIndex >= 0)
			{
				this->minRuleLengths[iRule] = ruleLength;
				this->maxRuleLengths[iRule] = ruleLength;
				this->knownRuleLengthsIndexed[this->constantIndex, iRule] = ruleLength;
			}

			for (size_t iSymbol = 0; iSymbol < PoG->Count; iSymbol++)
			{
				this->minPoG[iRule, iSymbol] = PoG[iSymbol];
				this->maxPoG[iRule, iSymbol] = PoG[iSymbol];
			}
		}
	}
}

void PMIProblemV2A::UpdateKnownRuleLengths()
{
	if (this->constantIndex >= 0)
	{
		// Set known rule lengths for any rule where the min equals the max
		for (size_t iRule = 0; iRule <= this->knownRuleLengthsIndexed->GetUpperBound(1); iRule++)
		{
			if (this->minRuleLengths[iRule] == this->maxRuleLengths[iRule])
			{
				this->knownRuleLengthsIndexed[this->constantIndex, iRule] = this->minRuleLengths[iRule];
			}
			else
			{
				this->knownRuleLengthsIndexed[this->constantIndex, iRule] = PMIProblem::cUnknownRuleLength;
			}
		}
	}
}

void PMIProblemV2A::UpdateKnownRuleLengths(Word^ W)
{

}

bool PMIProblemV2A::UpdateMaxPoG(Int32 iRule, Word^ Fragment)
{
	this->vocabulary->CompressSymbols(Fragment);

	for (size_t iSymbol = 0; iSymbol <= Fragment->parikhVector->GetUpperBound(0); iSymbol++)
	{
		this->UpdateMaxPoG(iRule, iSymbol, Fragment->parikhVector[iSymbol]);
	}

	return false;
}

void PMIProblemV2A::UpdateSolvedModules()
{
	for (size_t iGen = this->currentSet->startGeneration; iGen <= this->currentSet->endGeneration-1; iGen++)
	{
		for (size_t iRule = 0; iRule < this->vocabularyMaster->numModules; iRule++)
		{
			if (this->symbolSubsets[iGen, iRule])
			{
				this->solvedModules[iRule] = true;
			}
		}
	}
}

void PMIProblemV2A::UpdateTotalRuleLengths()
{
	bool result = false;

	Int32 totalMin = 0;
	Int32 totalMax = 0;
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Update Total Min/Max Based on Rule Min/Max Length");
#endif
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		totalMin += this->minRuleLengths[iRule];
		totalMax += this->maxRuleLengths[iRule];
	}

	result = this->UpdateTotalMinRuleLengths(totalMin) || result;
	result = this->UpdateTotalMaxRuleLengths(totalMax) || result;

	this->OutputRuleLengths();
}

void PMIProblemV2A::UpdateWIP(array<Int32, 2>^ Rules)
{
	// Update min/max PoG with latest solution
	for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= Rules->GetUpperBound(1); iSymbol++)
		{
			this->minPoG[iRule, iSymbol] = Rules[iRule, iSymbol];
			this->maxPoG[iRule, iSymbol] = Rules[iRule, iSymbol];
			this->minRuleLengthsWIP[iRule] += Rules[iRule, iSymbol];
			this->maxRuleLengthsWIP[iRule] += Rules[iRule, iSymbol];
		}
	}
}

void PMIProblemV2A::UpdateWIP(List<ProductionRule^>^ Rules)
{
	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		this->minRuleLengthsWIP[iRule] = Rules[iRule]->successor->Count;
		this->maxRuleLengthsWIP[iRule] = Rules[iRule]->successor->Count;
		for (size_t iSymbol = 0; iSymbol < Rules[iRule]->successor->Count; iSymbol++)
		{
			Int32 idx = this->vocabularyMaster->FindIndex(Rules[iRule]->successor[iSymbol]);
			this->minPoGWIP[iRule, iSymbol]++;
			this->maxPoGWIP[iRule, iSymbol]++;
		}
	}
}

bool PMIProblemV2A::VerifyOrderScan(array<Int32, 2>^ Rules)
{
	bool result = true;

#ifdef _PMI_PROBLEM_VERBOSE >= 3
	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
#endif

	Int32 idxScan;
	Int32 iSymbol;

	Int32 iGen = this->currentSet->startGeneration;
	List<Word^>^ filteredEvidence = this->evidence;
	//List<Symbol^>^ toRemove = gcnew List<Symbol^>;
	//for (size_t iSymbol = this->vocabularyMaster->numModules+this->constantIndex; iSymbol < this->vocabularyMaster->NumSymbols(); iSymbol++)
	//{
	//	toRemove->Add(this->vocabularyMaster->Symbols[iSymbol]);
	//}
	//List<Word^>^ filteredEvidence = gcnew List<Word^>;
	//// Make a copy of all the evidence while removing the constants
	//for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	//{
	//	Word^ a = gcnew Word(this->evidence[iGen]);
	//	a->FilterOutSymbol(toRemove);
	//	filteredEvidence->Add(a);
	//}

	do
	{
		idxScan = 0;
		iSymbol = 0;

		do
		{
			// Step 1 - Find a module
			if (filteredEvidence[iGen]->symbols[iSymbol]->isModule)
			{
				Int32 ruleIndex = this->vocabulary->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);
				//Word^ fragment = gcnew Word();

				array<Int32>^ pv = gcnew array<Int32>(Rules->GetUpperBound(1) + 1);
				Int32 ruleLength = 0;
				for (size_t iValue = 0; iValue <= Rules->GetUpperBound(1); iValue++)
				{
					pv[iValue] = Rules[ruleIndex, iValue];
					ruleLength += Rules[ruleIndex, iValue];
				}

				// Step 2 - Scan the evidence and ensure that any order is possible
				while ((ruleLength > 0) && (result))
				{
					if (idxScan < filteredEvidence[iGen + 1]->symbols->Count)
					{
						Int32 symbolIndex = this->vocabulary->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);
						//fragment->Add(filteredEvidence[iGen + 1]->symbols[idxScan]);

						//Console::WriteLine(fragment->ToString());
						pv[symbolIndex]--;
						ruleLength--;
						result = result && (pv[symbolIndex] >= 0);
						idxScan++;
					}
					else
					{
						result = false;
					}
				}

				//Console::WriteLine(fragment->ToString());
				//Console::WriteLine();

				iSymbol++;
				//// Since the verification can be cut short of the entire length of a rule
				//// if the current symbol (iSymbol) does not match the target (idxScan) then advanced
				//// the scan until they match to pass the previous rule
				//if ((iSymbol <this->model->evidence[iGen]->symbols->Count) && (!this->model->evidence[iGen]->symbols[iSymbol]->isModule))
				//{
				//	while ((idxScan < this->model->evidence[iGen + 1]->symbols->Count) && (this->model->evidence[iGen + 1]->symbols[idxScan] != this->model->evidence[iGen]->symbols[iSymbol]))
				//	{
				//		idxScan++;
				//	}
				//}
			}
			else
			{
				//Console::WriteLine(idxScan + ": " + iSymbol + " " + filteredEvidence[iGen + 1]->symbols[idxScan]);
				iSymbol++;
				idxScan++;
			}

			//} while ((result) && (iSymbol < this->model->evidence[iGen]->symbols->Count));
		} while ((result) && (iSymbol < filteredEvidence[iGen]->symbols->Count));

		iGen++;

	} while ((result) && (iGen < this->currentSet->endGeneration));

	//Console::WriteLine("Verification (Constants): " + result);
	return result;
}