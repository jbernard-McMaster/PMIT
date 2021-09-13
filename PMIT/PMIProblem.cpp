#include "stdafx.h"
#include "PMIProblem.h"

#include <algorithm>

/* PMI PROBLEM */

PMIProblem::PMIProblem(PMIProblem^ Problem)
{
	this->generations = Problem->generations;
	this->vocabulary = Problem->vocabulary;
	this->evidence = Problem->evidence;
	this->evidenceModuleOnly = Problem->evidenceModuleOnly;
}

PMIProblem::PMIProblem(PlantModel^ Model)
{
	this->Initialize(Model, 0, Model->evidence->Count - 1);
}

PMIProblem::PMIProblem(PlantModel^ Model, float ErrorRate, String^ FileName)
{
	this->errorRate = ErrorRate;
	this->modelFileName = FileName;
	this->fragmentFileName = FileName->Replace("problem", "fragment");
	this->lexiconFileName = FileName->Replace("problem", "lexicon");
	this->lexiconModuleOnlyFileName = FileName->Replace("problem", "lexiconMO");
	this->Initialize(Model, 0, Model->evidence->Count - 1);
}

PMIProblem::PMIProblem(PlantModel^ Model, Int32 StartGeneration, Int32 EndGeneration)
{
	this->Initialize(Model, StartGeneration, EndGeneration);
}

void PMIProblem::Initialize(PlantModel^ Model, Int32 StartGeneration, Int32 EndGeneration)
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

	// Initialize metrics
	this->solved = false;
	this->matched = false;
	this->analysisTime = 0;
	this->solveTime = 0;

	// Load the evidence from the model and extract vocabulary from evidence
	this->LoadEvidence();
	this->ExtractVocabulary();

	this->ruleHead = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleTail = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleFragments = gcnew array<Word^>(this->vocabulary->numModules);
	this->ruleModulesPool = gcnew array<List<Symbol^>^>(this->vocabulary->numModules);
	this->ruleConstantsPool = gcnew array<List<Symbol^>^>(this->vocabulary->numModules);

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		this->ruleHead[i] = gcnew Word();
		this->ruleTail[i] = gcnew Word();
		this->ruleFragments[i] = gcnew Word();
		this->ruleModulesPool[i] = gcnew List<Symbol^>;
		this->ruleConstantsPool[i] = gcnew List<Symbol^>;
	}

	this->maxRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	this->maxRuleLengthsModulesOnly = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengthsModulesOnly = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule <= this->minRuleLengths->GetUpperBound(0); iRule++)
	{
		this->minRuleLengths[iRule] = 0;
		this->minRuleLengthsModulesOnly[iRule] = 0;
		this->maxRuleLengths[iRule] = 999999999;
		this->maxRuleLengthsModulesOnly[iRule] = 99999999;
	}

	// Find the total minimum and maximum length of all rules based on inference
	this->totalMinRuleLength = 0;
	this->totalMaxRuleLength = 999999999;
	this->totalMinRuleLengthModulesOnly = 0;
	this->totalMaxRuleLengthModulesOnly = 999999999;

	this->GenerateError();
	// Analysis time starts here
	auto startTime = Time::now();
	this->generationsSolved = gcnew array<bool>(this->generations);
	this->generationsSolvedModulesOnly = gcnew array<bool>(this->generations);
	this->numModules = gcnew array<Int32>(this->generations);
	this->numConstants = gcnew array<Int32>(this->generations);

	this->ComputeSymbiology();
	this->ComputeFinalGeneration();
	this->ComputeParikhVectors();
	this->ComputeRuleHeadsTails();
	this->EstimateGrowthPattern(); // this estimation needs to happen because rule lengths are critical to the lexicon production
	this->EstimateRuleLengths(false, false); // this estimation needs to happen because rule lengths are critical to the lexicon production
	this->ComputeLexicon();
	//this->ComputeNeighbouringSymbols();
	this->CheckForSolvedGenerations(0, this->generations);
	// TODO: this needs to be an estimation of the subproblems and then finalize the sub-problems after compute the feature vectors
	// When that happens will need to add an interative estimate of the rule lengths and growth pattern before this step
	this->FindSubProblems();

	// Loop through computing rule lengths and growth pattern until they converge and there is no update
	// This is because compute rule lengths can effect how growth patterns is computed
	// and computing the growth pattern can effect how the rule lengths are computed
	bool changes = true;
	while (changes)
	{
		changes = this->EstimateRuleLengths(true, true);
		this->ComputeGrowthPattern();
	}

	this->CheckForSolvedGenerations(0, this->generations);

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->analysisTime = ms.count();

	this->OutputAnalysis();
}

PMIProblem::~PMIProblem()
{
}

array<Int32, 3>^ PMIProblem::AdjustRPT(array<Int32, 3>^ ARPT, List<Int32>^ SolutionIndex)
{
	array<Int32, 3>^ result = ARPT;

	for (size_t iEquation = 0; iEquation < this->equations->Count; iEquation++)
	{
		List<Int32>^ s = this->equations[iEquation]->solutions[SolutionIndex[iEquation]];
		for (size_t iVariable = 0; iVariable < this->equations[iEquation]->variables->Count; iVariable++)
		{
			for (size_t iGen = 0; iGen < this->generations; iGen++)
			{
				result[iGen, this->equations[iEquation]->variables[iVariable]->rule, this->equations[iEquation]->variables[iVariable]->symbol] = s[iVariable];
			}
		}
	}

	return result;
}

List<Symbol^>^ PMIProblem::AssembleFragments(Int32 RuleIndex)
{
	List<Symbol^>^ result = gcnew List<Symbol^>;

	bool matchHeadTail = true;
	// Step 1 - Check to see if head and tail overlap, if they do they both cannot be included
	if (this->ruleHead[RuleIndex]->symbols->Count < this->ruleTail[RuleIndex]->symbols->Count)
	{
		for (size_t iStart = 0; iStart < this->ruleHead[RuleIndex]->Count; iStart++)
		{
			Int32 idx = 0;
			bool isMatch = true;
			for (size_t iSymbol = iStart; iSymbol < this->ruleHead[RuleIndex]->Count; iSymbol++)
			{
				if (this->ruleTail[RuleIndex][idx] != this->ruleHead[RuleIndex][iSymbol])
				{
					isMatch == false;
				}
				idx++;
			}

			matchHeadTail = matchHeadTail || isMatch;
		}
	}
	else
	{
		for (size_t iEnd = 0; iEnd < this->ruleTail[RuleIndex]->Count; iEnd++)
		{
			Int32 idx = this->ruleHead[RuleIndex]->Count - this->ruleTail[RuleIndex]->Count + iEnd;
			bool isMatch = true;
			for (size_t iSymbol = 0; iSymbol < this->ruleTail[RuleIndex]->Count - iEnd; iSymbol++)
			{
				if (this->ruleTail[RuleIndex][iSymbol] != this->ruleHead[RuleIndex][idx])
				{
					isMatch == false;
				}
				idx++;
			}

			matchHeadTail = matchHeadTail || isMatch;
		}
	}

	bool matchHeadFragment = false;
	Int32 headFragmentIndex = 0;

	bool matchTailFragment = false;
	Int32 tailFragmentIndex = 0;

	// Check for overlap between head & tail with the fragment if there is one
	if (this->ruleFragments[RuleIndex]->Count > 0)
	{
		// Step 2 - Check to see if head and fragment overlap if there is a head fragment
		if (this->ruleHead[RuleIndex]->Count > 0)
		{
			Int32 iStart = 0;
			while ((iStart < this->ruleHead[RuleIndex]->Count) && (!matchHeadFragment))
			{
				Int32 idx = 0;
				bool isMatch = true;
				for (size_t iSymbol = iStart; iSymbol < this->ruleHead[RuleIndex]->Count; iSymbol++)
				{
					if (this->ruleHead[RuleIndex][iSymbol] != this->ruleFragments[RuleIndex][idx])
					{
						isMatch == false;
					}
					idx++;
				}

				matchHeadFragment = matchHeadFragment || isMatch;

				if (isMatch)
				{
					headFragmentIndex = iStart;
				}

				iStart++;
			}
		}

		// Step 3 - Check to see if head and fragment overlap
		if (this->ruleTail[RuleIndex]->Count > 0)
		{
			Int32 iEnd = this->ruleTail[RuleIndex]->Count - 1;
			while ((iEnd < this->ruleTail[RuleIndex]->Count) && (!matchTailFragment))
			{
				Int32 idx = this->ruleFragments[RuleIndex]->Count - 1 - iEnd;
				bool isMatch = true;
				for (size_t iSymbol = 0; iSymbol < iEnd; iSymbol++)
				{
					if (this->ruleTail[RuleIndex][iSymbol] != this->ruleFragments[RuleIndex][idx])
					{
						isMatch == false;
					}
					idx++;
				}

				matchTailFragment = matchTailFragment || isMatch;

				if (isMatch)
				{
					tailFragmentIndex = iEnd;
				}

				iEnd--;
			}
		}
	}

	// If there is a fragment it will be included as it will always be longer than the head/tail
	// Non-overlapping poritions of the head/tail fragments can also be added
	if (this->ruleFragments[RuleIndex]->Count > 0)
	{
		for (size_t iSymbol = 0; iSymbol < this->ruleFragments[RuleIndex]->Count; iSymbol++)
		{
			result->Add(this->ruleFragments[RuleIndex][iSymbol]);
		}

		// if there is no overlap then add the whole head/tail
		if (!matchHeadFragment)
		{
			for (size_t iSymbol = 0; iSymbol < this->ruleHead[RuleIndex]->Count; iSymbol++)
			{
				result->Add(this->ruleHead[RuleIndex][iSymbol]);
			}
		}
		else
		{// otherwise add the non-overlapping portion
			for (size_t iSymbol = headFragmentIndex-1; iSymbol < 0; iSymbol--)
			{
				result->Add(this->ruleHead[RuleIndex][iSymbol]);
			}
		}


		// if there is no overlap then add the whole head/tail
		if (!matchTailFragment)
		{
			for (size_t iSymbol = 0; iSymbol < this->ruleTail[RuleIndex]->Count; iSymbol++)
			{
				result->Add(this->ruleTail[RuleIndex][iSymbol]);
			}
		}
		else
		{// otherwise add the non-overlapping portion
			for (size_t iSymbol = tailFragmentIndex+1; iSymbol < this->ruleTail[RuleIndex]->Count; iSymbol++)
			{
				result->Add(this->ruleTail[RuleIndex][iSymbol]);
			}
		}

		//Console::WriteLine(this->ruleHead[RuleIndex]->ToString() + " " + this->ruleFragments[RuleIndex]->ToString() + " " + matchHeadFragment.ToString() + " " + headFragmentIndex);
		//Console::WriteLine(this->ruleTail[RuleIndex]->ToString() + " " + this->ruleFragments[RuleIndex]->ToString() + " " + matchTailFragment.ToString() + " " + tailFragmentIndex);
		//for (size_t i = 0; i < result->Count; i++)
		//{
		//	Console::Write(result[i]->ToString());
		//}
		//Console::WriteLine();
		//Console::ReadLine();
	}
	else
	{
		if (!matchHeadTail)
		{
			for (size_t iSymbol = 0; iSymbol < this->ruleHead[RuleIndex]->Count; iSymbol++)
			{
				result->Add(this->ruleHead[RuleIndex][iSymbol]);
			}

			for (size_t iSymbol = 0; iSymbol < this->ruleTail[RuleIndex]->Count; iSymbol++)
			{
				result->Add(this->ruleTail[RuleIndex][iSymbol]);
			}
		}
		else
		{
			if (this->ruleHead[RuleIndex]->Count >= this->ruleTail[RuleIndex]->Count)
			{
				for (size_t iSymbol = 0; iSymbol < this->ruleHead[RuleIndex]->Count; iSymbol++)
				{
					result->Add(this->ruleHead[RuleIndex][iSymbol]);
				}
			}
			else
			{
				for (size_t iSymbol = 0; iSymbol < this->ruleTail[RuleIndex]->Count; iSymbol++)
				{
					result->Add(this->ruleTail[RuleIndex][iSymbol]);
				}
			}
		}
	}

	return result;
}

float PMIProblem::Assess(array<Int32>^ Solution)
{
	float result = 0.0;

	// Generate Axiom
	//Word^ a = gcnew Word(this->evidence[0]->symbols);
	//List<Word^>^ solutionAxioms = gcnew List<Word^>(0);

	//solutionAxioms->Add(gcnew Word(a->symbols));

	if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Modules)
	{
		return this->Assess_Modules(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Length)
	{
		return this->Assess_Length(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Constants)
	{
		return this->Assess_Constants(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Fragment)
	{
		return this->Assess_Fragment(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::DirectModulesOnly)
	{
		return this->Assess_Direct_ModulesOnly(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Direct)
	{
		return this->Assess_Direct(Solution);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Order)
	{
		List<ProductionRule^>^ R = this->CreateRulesFromSolution(Solution);
		result = this->AssessProductionRules(R);

#if _PMI_PROBLEM_VERBOSE_ >= 2
		Console::WriteLine("Candidate");
		Console::WriteLine(" Axiom = " + a->ToString());
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in R)
		{
			Console::WriteLine("   " + r->ToString());
			//Console::WriteLine("   " + r->ToStringAbstract());
		}

		Console::WriteLine();
#endif

#if _PMI_PROBLEM_VERBOSE_ >= 2

		for (size_t iGen = 0; iGen < this->generations; iGen++)
		{
			Console::WriteLine("Evidence #" + iGen.ToString(L"G") + " : " + this->evidence[iGen]->ToString());
			Console::WriteLine("Solution #" + iGen.ToString(L"G") + " : " + solutionAxioms[iGen]->ToString());
		}

		Console::WriteLine("F = " + result.ToString(L"F5"));
		Console::WriteLine("Paused");
		Console::ReadLine();
#endif
	}

	return result;
}

float PMIProblem::Assess_Constants(array<Int32>^ Solution)
{
	// check to see if the solution is on the tabu list
	float result = this->generations-1;
	bool disallowed = false;

	Int32 index1 = 0;
	Int32 index2 = 0;
	Int32 total = 0;
	Int32 errors = 0;
	float errorsValue = 0.0;
	Int32 idx = this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex;

	array<Int32>^ axiom;
	array<Int32, 2>^ rules;

	axiom = this->Assess_CreateAxiomStep_Constants(this->vocabulary->numModules + 1, this->evidence[this->currentProblem->startGeneration]->parikhVector);
	rules = this->Assess_CreateParikhRulesStep_Constants(Solution);

	array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules + 1);
	array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules + 1);

	if (!this->currentProblem->IsTabu(this->currentProblem->constantIndex, rules))
	{
		for (size_t i = 0; i < this->vocabulary->numModules + 1; i++)
		{
			last[i] = axiom[i];
			if (i >= this->vocabulary->numModules)
			{
				current[i] = axiom[i]; // for constants need to prepopulate the current number so it will be tracked properly
			}
		}

		for (size_t iGen = this->currentProblem->startGeneration; iGen < this->currentProblem->endGeneration; iGen++)
		{
			Int32 genErrors = 0;

			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			{
				Int32 count = 0;
				for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
				{
					if (iSymbol < this->vocabulary->numModules)
					{
						count += last[iRule] * rules[iRule, iSymbol];
					}
					else
					{
						current[iSymbol] += last[iRule] * rules[iRule, iSymbol];
					}
				}

				if (iSymbol < this->vocabulary->numModules)
				{
					current[iSymbol] = count;
				}
			}

			for (size_t i = 0; i < current->Length; i++)
			{
				if (i >= this->vocabulary->IndexConstantsStart)
				{
					genErrors += Math::Abs(current[i] - this->evidence[iGen + 1]->parikhVector[this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex]);
					total += this->evidence[iGen + 1]->parikhVector[this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex];
				}

				last[i] = current[i];
			}

			if (genErrors == 0)
			{
				result -= 1;
			}
			
			errors += genErrors;

#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 2
			if (((errors == 0) && (iGen >= 2) && (!this->solveForConstants)) || ((errors == 0) && (iGen >= 0) && (this->solveForConstants)))
			{
				Console::WriteLine("Candidate");
				for (size_t i = 0; i < axiom->Length; i++)
				{
					Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
				}
				Console::WriteLine();
				Console::WriteLine(" Rules: ");
				for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
				{
					Console::Write(this->vocabulary->Symbols[i] + " => ");
					for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
					{
						Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
					}
					Console::WriteLine();
				}

				for (size_t i = 0; i < current->Length; i++)
				{
					Console::WriteLine("S (" + this->vocabulary->Symbols[i] + "): Actual " + this->evidence[iGen]->compressed[i] + ", Calculated " + current[i]);
				}
				Console::WriteLine();
			}
#endif
		}

		result = (float)errors / total;
	}
	else
	{// Tabu solution
		result = 0.001;
	}

#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 1
	if ((result == 0) && (this->solveForConstants))
	{
		Console::WriteLine("Candidate");
		for (size_t i = 0; i < axiom->Length; i++)
		{
			Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
		}
		Console::WriteLine();
		Console::WriteLine(" Rules: ");
		for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
		{
			Console::Write(this->vocabulary->Symbols[i] + " => ");
			for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
			{
				Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
			}
			Console::WriteLine();
		}
		Console::WriteLine();
	}
#endif

	return result;
}

float PMIProblem::Assess_Direct(array<Int32>^ Solution)
{
	float fitness = this->generations+1;

	if (!this->currentProblem->IsTabu(Solution))
	{
		List<ProductionRule^>^ modelRules = this->Assess_CreateRulesStep_Direct(Solution);

#if _PMI_PROBLEM_VERBOSE_ >= 5
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
	}
		Console::WriteLine();
#endif

		if (modelRules != nullptr)
		{
			fitness = this->AssessProductionRules(modelRules);
		}
		else
		{// invalid solution
			fitness = 1.0;
		}
	}
	else
	{
		fitness = 0.1;
	}

	return fitness;
}

float PMIProblem::Assess_Direct_ModulesOnly(array<Int32>^ Solution)
{
	float fitness = this->generations + 1;
	if (!this->currentProblem->IsTabu(Solution))
	{
		List<ProductionRule^>^ modelRules = this->Assess_CreateRulesStep_Modules(Solution);


#if _PMI_PROBLEM_VERBOSE_ >= 5
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif

		if (modelRules != nullptr)
		{
			fitness = this->AssessProductionRules_ModulesOnly(modelRules);
		}
		else
		{// invalid solution
			fitness = 1.0;
		}
	}
	else
	{
		fitness = 0.1;
	}

	return fitness;
}

float PMIProblem::Assess_Fragment(array<Int32>^ Solution)
{
	float fitness = this->generations + 1;

	if (!this->currentProblem->IsTabu(Solution))
	{
		List<ProductionRule^>^ modelRules = this->Assess_CreateRulesStep_Fragment(Solution);

#if _PMI_PROBLEM_VERBOSE_ >= 5
		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in modelRules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();
#endif

		if (modelRules != nullptr)
		{
			fitness = this->AssessProductionRules(modelRules);
		}
		else
		{// invalid solution
			fitness = 1.0;
		}
	}
	else
	{
		fitness = 0.1;
	}

	return fitness;
}

float PMIProblem::Assess_Length(array<Int32>^ Solution)
{
	return 1.0;
}

float PMIProblem::Assess_Modules(array<Int32>^ Solution)
{
	float result = this->currentProblem->endGeneration-this->currentProblem->startGeneration;
	Int32 index = 0;
	Int32 total = 0;
	Int32 errors = 0;
	float errorsValue = 0.0;

	array<Int32>^ axiom;
	array<Int32, 2>^ rules;

	axiom = this->Assess_CreateAxiomStep_Modules(this->vocabulary->numModules, this->evidence[this->currentProblem->startGeneration]->parikhVector);
	rules = this->Assess_CreateParikhRulesStep_Modules(Solution);

	if (!this->currentProblem->IsTabu(rules))
	{
		array<Int32>^ last = gcnew array<Int32>(this->vocabulary->numModules);
		array<Int32>^ current = gcnew array<Int32>(this->vocabulary->numModules);

		for (size_t i = 0; i < this->vocabulary->numModules; i++)
		{
			last[i] = axiom[i];
		}

		for (size_t iGen = this->currentProblem->startGeneration; iGen < this->currentProblem->endGeneration; iGen++)
		{
			for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			{
				Int32 count = 0;
				for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
				{
					count += last[iRule] * rules[iRule, iSymbol];
				}

				current[iSymbol] = count;
			}

			Int32 genError = 0;

			for (size_t i = 0; i < current->Length; i++)
			{
				genError += Math::Abs(current[i] - this->evidence[iGen + 1]->parikhVector[i]);
				total += this->evidence[iGen + 1]->parikhVector[i];

				last[i] = current[i];
			}

			if (genError == 0)
			{
				result -= 1;
			}

			errors += genError;

#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 2
			if (((errors == 0) && (iGen >= 2) && (!this->solveForConstants)) || ((errors == 0) && (iGen >= 0) && (this->solveForConstants)))
			{
				Console::WriteLine("Candidate");
				for (size_t i = 0; i < axiom->Length; i++)
				{
					Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
				}
				Console::WriteLine();
				Console::WriteLine(" Rules: ");
				for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
				{
					Console::Write(this->vocabulary->Symbols[i] + " => ");
					for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
					{
						Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
					}
					Console::WriteLine();
				}

				for (size_t i = 0; i < current->Length; i++)
				{
					Console::WriteLine("S (" + this->vocabulary->Symbols[i] + "): Actual " + this->evidence[iGen]->compressed[i] + ", Calculated " + current[i]);
				}
				Console::WriteLine();
			}
#endif
		}
#if _PMI_TEMPLATE_PROBLEM_VERBOSE_ >= 1
		if ((result == 0))
		{
			Console::WriteLine("Candidate");
			for (size_t i = 0; i < axiom->Length; i++)
			{
				Console::Write(this->vocabulary->Symbols[i] + "(" + axiom[i] + ")");
			}
			Console::WriteLine();
			Console::WriteLine(" Rules: ");
			for (size_t i = 0; i <= rules->GetUpperBound(0); i++)
			{
				Console::Write(this->vocabulary->Symbols[i] + " => ");
				for (size_t j = 0; j <= rules->GetUpperBound(1); j++)
				{
					Console::Write(" " + this->vocabulary->Symbols[j] + "(" + rules[i, j] + ")");
				}
				Console::WriteLine();
			}
			Console::WriteLine();
		}
#endif

		result += (float)errors / total;
	}
	else
	{// tabu solution
		result += 0.001;
	}

	return result;
}

float PMIProblem::AssessProductionRules(List<ProductionRule^>^ Rules)
{
	float result = 0.0;

	if ((this->fitnessOption == FitnessOption::Positional) || (this->fitnessOption == FitnessOption::Pattern) || (this->fitnessOption == FitnessOption::Indexed))
	{
		Word^ a = gcnew Word(this->evidence[this->currentProblem->startGeneration]->symbols);
		List<Word^>^ solutionAxioms = gcnew List<Word^>;
		solutionAxioms = EvidenceFactory::Process(a, Rules, this->currentProblem->startGeneration, this->currentProblem->endGeneration, true);
		
		switch (this->fitnessOption)
		{
		case FitnessOption::Positional:
			result = this->CompareEvidence_Positional(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
			break;
		case FitnessOption::Pattern:
			//	solutionAxioms = this->SymbolSubstitution(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
			result = 1.0;
			break;
		case FitnessOption::Indexed:
			result = this->CompareEvidence_Indexed(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
			break;
		default:
			break;
		}
	}
	else if (this->fitnessOption == FitnessOption::Consistency)
	{
		result = this->CompareEvidence_Consistency(Rules, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	}

	return result;
}

float PMIProblem::AssessProductionRules_ModulesOnly(List<ProductionRule^>^ Rules)
{
	Word^ a = gcnew Word(this->evidenceModuleOnly[this->currentProblem->startGeneration]->symbols);
	List<Word^>^ solutionAxioms = gcnew List<Word^>;

	solutionAxioms = EvidenceFactory::Process(a, Rules, this->currentProblem->startGeneration, this->currentProblem->endGeneration, true);

	float result = 0.0;

	//// For pattern matching fitness, first do a symbol substitution
	//if (this->fitnessOption == FitnessOption::Pattern)
	//{
	//	solutionAxioms = this->SymbolSubstitution(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	//}

	if ((this->fitnessOption == FitnessOption::Positional) || (this->fitnessOption == FitnessOption::Pattern))
	{
		result = this->CompareEvidence_Positional(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidenceModuleOnly, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	}
	else if (this->fitnessOption == FitnessOption::Indexed)
	{// I don't remember what indexed fitness 
		result = this->CompareEvidence_Indexed(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidenceModuleOnly, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	}

	return result;
}

float PMIProblem::AssessProductionRules_Parikh(List<ProductionRule^>^ Rules)
{
	float result = this->generations+1;
	Word^ a = gcnew Word(this->evidence[this->currentProblem->startGeneration]->symbols);
	List<Word^>^ solutionAxioms = gcnew List<Word^>;

	solutionAxioms = EvidenceFactory::Process(a, Rules, this->currentProblem->startGeneration, this->currentProblem->endGeneration, true);

	// Compress the axioms into Parikh vectors
	Int32 errors = 0;
	Int32 total = 0;

	for (size_t iAxiom = 0; iAxiom < solutionAxioms->Count; iAxiom++)
	{
		// Compute the Parikh vector for the axiom
		this->vocabulary->CompressSymbols(solutionAxioms[iAxiom]);

		// Compare it to the Parikh vector of the evidence
		for (size_t iValue = 0; iValue <= this->evidence[iAxiom]->parikhVector->GetUpperBound(0); iValue++)
		{
			errors += Math::Abs(this->evidence[iAxiom]->parikhVector[iValue] - solutionAxioms[iAxiom]->parikhVector[iValue]);
			total += this->evidence[iAxiom]->parikhVector[iValue];
		}
	}

	if (errors == 0)
	{
		result = this->CompareEvidence_Positional(solutionAxioms, 0, solutionAxioms->Count - 1, this->evidence, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	}
	else
	{
		result = (float)errors / total;
	}

	return result;
}

array<Int32>^ PMIProblem::Assess_CreateAxiomStep_Constants(Int32 Size, array<Int32>^ Solution)
{
	array<Int32>^ axiom = gcnew array<Int32>(Size);

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		axiom[i] = Solution[i];
	}

	axiom[Size - 1] = Solution[this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex];

	return axiom;
}

array<Int32>^ PMIProblem::Assess_CreateAxiomStep_Modules(Int32 Size, array<Int32>^ Solution)
{
	array<Int32>^ axiom = gcnew array<Int32>(Size);

	for (size_t i = 0; i < axiom->Length; i++)
	{
		axiom[i] = Solution[i];
	}

	return axiom;
}

array<Int32, 2>^ PMIProblem::Assess_CreateParikhRulesStep_Constants(array<Int32>^ Solution)
{
	Int32 index1 = 0;
	Int32 index2 = 0;

	Int32 t = 0;

	// Need enough space to load the Parikh values for the modules and then for the 1 constant
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules + 1);

	/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
	but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
	for (size_t iRule = 0; iRule <= Solution->GetUpperBound(0); iRule++)
	{
		t += Solution[index1];
		index1++;
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules + 1; iSymbol++)
		{
			if (iSymbol < this->vocabulary->numModules)
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol];
			}
			else
			{
				//if (this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, ] == PMIProblem::cUnknownParikhValue)
				if (!this->IsModuleSolvedForSymbol(iRule, iSymbol + this->currentProblem->constantIndex))
				{
					rules[iRule, iSymbol] = round(((float)Solution[index2] / PMIProblem::cGene_Max) * this->maxPoG[iRule, iSymbol + this->currentProblem->constantIndex]);
					index2++;
				}
				else
				{
					rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol + this->currentProblem->constantIndex];
				}
			}
		}
	}

	return rules;
}

array<Int32, 2>^ PMIProblem::Assess_CreateParikhRulesStep_Modules(array<Int32>^ Solution)
{
	Int32 index1 = 0;
	Int32 index2 = 0;

	array<Int32>^ t = gcnew array<Int32>(this->vocabulary->numModules);
	array<Int32>^ s = gcnew array<Int32>(this->vocabulary->numModules);
	array<Int32, 2>^ rules = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->numModules);

	/* This is really inefficient. The problem is that everything I've coded so far process each different symbol in a rule, then the next rule and so on
	but the genome is coded as Symbol A in Rule 1, Symbol A in Rule 2, ..., Symbol B in Rule 1, ... */
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
		{
			//if (this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
			// TODO: this is really inefficient, I should precalculate which rule/symbol combinations need to be solved
			if (!this->IsModuleSolvedForSymbol(iRule,iSymbol))
			{
				t[iSymbol] += Solution[index1];
				index1++;
			}
		}
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
		{
			// TODO: again this should be pre-calculated
			if (!this->IsModuleSolvedForSymbol(iRule, iSymbol))
			{
			//	if (this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
			//{
				// TODO: Account for the ParikhVector being zero

				// Calculate the fraction of the growth this symbol will be responsible for
				float fractionOfGrowth = ((float)Solution[index2] / t[iSymbol]);
				// Calculate the amount of growth this symbol is responsible for
				float growth = (float)(fractionOfGrowth * this->growthSymbol[iSymbol]);
				// divide the amount of growth by the number of symbols in the evidence
				float parikhValue = growth / (float) this->evidence[this->lastGenerationSymbol[iSymbol] - 1]->parikhVector[iRule];

				rules[iRule, iSymbol] = Math::Min(this->maxPoG[iRule, iSymbol], (int)round(parikhValue));
				index2++;
			}
			else
			{
				rules[iRule, iSymbol] = this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol];
			}
		}
	}

	return rules;
}

List<ProductionRule^>^ PMIProblem::Assess_CreateRulesStep_Direct(array<Int32>^ Solution)
{
	return nullptr;
}

List<ProductionRule^>^ PMIProblem::Assess_CreateRulesStep_Fragment(array<Int32>^ Solution)
{
	return nullptr;
}

List<ProductionRule^>^ PMIProblem::Assess_CreateRulesStep_Modules(array<Int32>^ Solution)
{
	return nullptr;
}

void PMIProblem::CapturePartialSolution(array<Int32, 2>^ Rules)
{
	if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Modules)
	{
		this->CapturePartialSolution_ParikhModules(Rules);
	}
	else if (this->currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Constants)
	{
		this->CapturePartialSolution_ParikhConstants(Rules);
	}

	this->CheckForSolvedGenerations(this->currentProblem->startGeneration, this->currentProblem->endGeneration);
#if _PMI_PROBLEM_VERBOSE_ >= 3
	this->OutputRPT();
#endif
}

void PMIProblem::CapturePartialSolution_ParikhConstants(array<Int32, 2>^ Rules)
{
	// TODO: Need to change this so that it choose the generations based on the belief system
	//for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
		{
			Int32 iSymbol = this->vocabulary->numModules; // Because everything is zero indexed, this points to the symbol
			RPTChange^ C = gcnew RPTChange(iGen, iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex, this->rulesParikhTemplate[iGen, iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex]);
			this->changes->Add(C);
			this->rulesParikhTemplate[iGen, iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex] = Rules[iRule, iSymbol];
		}
	}
}

void PMIProblem::CapturePartialSolution_ParikhModules(array<Int32, 2>^ Rules)
{
	// TODO: Need to change this so that it choose the generations based on the belief system
	//for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= Rules->GetUpperBound(1); iSymbol++)
			{
				RPTChange^ C = gcnew RPTChange(iGen, iRule, iSymbol, this->rulesParikhTemplate[iGen, iRule, iSymbol]);
				this->changes->Add(C);

				this->rulesParikhTemplate[iGen, iRule, iSymbol] = Rules[iRule, iSymbol];
			}
		}
	}
}

void PMIProblem::CapturePartialSolution_Rules(List<ProductionRule^>^ Rules)
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
			this->knownRuleLengths[index] = length;
			this->solution->rules->Add(Rules[iRule]);
		//}
	}
}

void PMIProblem::CaptureParikhVector(Int32 Index, array<Int32>^ P)
{
	// TODO: Need to change this so that it can choose to only update the current problem's generation for change over time problems
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol <= P->GetUpperBound(0); iSymbol++)
		{
			RPTChange^ C = gcnew RPTChange(iGen, Index, iSymbol, this->rulesParikhTemplate[iGen, Index, iSymbol]);
			this->changes->Add(C);
			this->UpdateRPT(iGen, Index, iSymbol, P[iSymbol]);
			//this->rulesParikhTemplate[iGen, Index, iSymbol] = P[iSymbol];
		}
	}
}

array<Int32>^ PMIProblem::Cheat(array<Int32>^ Solution)
{
	bool invalid = false;
	this->CreateGenome();

	for (size_t iValue = 0; iValue <= Solution->GetUpperBound(0); iValue++)
	{
		Int32 iRule = this->solution2rule[iValue];

		if (this->knownRuleLengths[iRule] == PMIProblem::cUnknownRuleLength)
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

void PMIProblem::CheckForSolvedGenerations(Int32 Start, Int32 End)
{
	// Check to see if any generations are now completely solved
	for (size_t iGen = Start; iGen < End; iGen++)
	{
		bool solved = true;
		bool solvedModulesOnly = true;

		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				if (iSymbol < this->vocabulary->numModules)
				{
					solvedModulesOnly = solvedModulesOnly && (this->rulesParikhTemplate[iGen, iRule, iSymbol] != PMIProblem::cUnknownParikhValue);
				}
				solved = solved && (this->rulesParikhTemplate[iGen, iRule, iSymbol] != PMIProblem::cUnknownParikhValue);
			}
		}

		this->generationsSolved[iGen] = solved;
		this->generationsSolvedModulesOnly[iGen] = solvedModulesOnly;
	}
}

bool PMIProblem::CheckValidity(array<Int32>^ Solution)
{
	bool result = true;

	return result;
}

Int32 PMIProblem::ComputeActualLength(Int32 iRule)
{
	Int32 result = 0;
	for (size_t iModelRule = 0; iModelRule < this->model->rules->Count; iModelRule++)
	{
		if (this->model->rules[iModelRule]->predecessor == this->vocabularyMaster->Symbols[iRule])
		{
			for (size_t iSymbol = 0; iSymbol < this->model->rules[iModelRule]->successor->Count; iSymbol++)
			{
				for (size_t iVocabularySymbol = 0; iVocabularySymbol < this->vocabulary->NumSymbols(); iVocabularySymbol++)
				{
					if (this->vocabulary->Symbols[iVocabularySymbol]->Value() == this->model->rules[iModelRule]->successor[iSymbol]->Value())
					{
						result++;
					}
				}
			}
		}
	}

	return result;
}

void PMIProblem::ComputeFinalGeneration()
{
	this->finalGeneration = 0;

	for (size_t iModule = 0; iModule < this->vocabulary->numModules; iModule++)
	{
		if (this->lastGenerationSymbol[iModule] > this->finalGeneration)
		{
			this->finalGeneration = this->lastGenerationSymbol[iModule];
		}
	}
}

void PMIProblem::ComputeGrowthPattern()
{
	this->PoG = gcnew array<Int32, 3>(this->generations-1, this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->minPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->maxPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	for (size_t iGen = 0; iGen <= this->PoG->GetUpperBound(0); iGen++)
	{
		for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
			{
				this->PoG[iGen, iRule, iSymbol] = 0;
				if (iGen == 0)
				{
					// TODO: Non-overlapping head, tail and fragments can be included
					Word^ W = this->FindLongestFragment(iRule);
					this->vocabulary->CompressSymbols(W);
					this->minPoG[iRule, iSymbol] = W->parikhVector[iSymbol];
					this->maxPoG[iRule, iSymbol] = 99999;
				}
			}
		}
	}

	// Compute the unaccounted for growth
	List<Word^>^ unaccounted = gcnew List<Word^>;
	unaccounted->Add(this->evidence[0]);
	for (size_t iGen = 1; iGen < this->evidence->Count - 1; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		this->vocabulary->CompressSymbols(a);
		for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
			{
				a->parikhVector[iSymbol] -= this->minPoG[iRule, iSymbol];
			}
		}
		unaccounted->Add(a);
	}

	Int32 idx = 0;
	this->fvMaxOccurance = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols()*this->vocabulary->NumSymbols());
	this->fvDeltaMaxOccurance = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols()*this->vocabulary->NumSymbols());

	for (size_t iGen = 0; iGen < this->evidence->Count - 1; iGen++)
	{
		// Get the two generations of evidence
		Word^ x = this->evidence[iGen + 1];
		Word^ y = this->evidence[iGen];

		idx = 0;
		// For each symbol figure out the maximum growth by symbol
		for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
		{
			if ((y->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
			{
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
				{
					if (iSymbol < this->vocabulary->numModules)
					{
						this->PoG[iGen, iRule, iSymbol] = (int)Math::Floor((x->parikhVector[iSymbol]) / y->parikhVector[iRule]);
						this->maxPoG[iRule, iSymbol] = Math::Min(this->maxPoG[iRule, iSymbol], (int)Math::Floor((x->parikhVector[iSymbol]) / y->parikhVector[iRule]));
					}
					else
					{
						this->PoG[iGen, iRule, iSymbol] = (int)Math::Floor((x->parikhVector[iSymbol] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]);
						this->maxPoG[iRule, iSymbol - this->vocabulary->IndexModulesStart] = Math::Min(this->maxPoG[iRule, iSymbol - this->vocabulary->IndexModulesStart], (int)Math::Floor((x->parikhVector[iSymbol] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]));
					}

					// Compute feature vector
					if ((y->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
					{
						if ((y->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
						{
							this->fvMaxOccurance[iGen, idx] = Math::Floor((x->parikhVector[iSymbol]) / y->parikhVector[iRule]);
						}
						else
						{
							this->fvMaxOccurance[iGen, idx] = Math::Floor((x->parikhVector[iSymbol] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]);
						}
					}

					if (iGen >= 1)
					{
						this->fvDeltaMaxOccurance[iGen - 1, idx] = this->fvMaxOccurance[iGen, idx] - this->fvMaxOccurance[iGen - 1, idx];
					}
					idx++;
				}
			}
		}
	}

	// Get the growth for the last generation
	this->growthSymbol = gcnew array<Int32>(this->vocabulary->NumSymbols());
	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	{
		//this->growthSymbol[iSymbol - this->masterVocabulary->IndexModulesStart] = this->evidence[this->evidence->Count - 1]->parikhVector[iSymbol];
		this->growthSymbol[iSymbol - this->vocabulary->IndexModulesStart] = this->evidence[this->lastGenerationSymbol[iSymbol]]->parikhVector[iSymbol];
	}

	// Compute a feature vector based on growth
	// Compute a feature vector based on changes in growth
	this->fvGrowth = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols());
	this->fvDeltaGrowth = gcnew array<float, 2>(this->generations-1, this->vocabulary->NumSymbols());

	for (size_t iGen = 0; iGen < this->evidence->Count - 1; iGen++)
	{
		// Get the two generations of evidence
		Word^ x = this->evidence[iGen + 1];
		Word^ y = this->evidence[iGen];

		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
		{
			this->fvGrowth[iGen, iSymbol] = x->parikhVector[iSymbol] - y->parikhVector[iSymbol];
			if (iGen >= 1)
			{
				this->fvDeltaGrowth[iGen - 1, iSymbol] = this->fvGrowth[iGen, iSymbol] - this->fvGrowth[iGen - 1, iSymbol];
			}
		}
	}

	// For any symbols with a known value, set the template to this value
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
		{
			for (size_t iSymbol = this->vocabularyMaster->IndexModulesStart; iSymbol <= this->vocabularyMaster->IndexConstantsEnd; iSymbol++)
			{
				if (this->minPoG[iRule, iSymbol] == this->maxPoG[iRule, iSymbol])
				{
					//Console::WriteLine(iGen + ", " + iRule + ", " + iSymbol + " = " + this->rulesParikhTemplate[iGen, iRule, iSymbol] + ", " + this->maxOccuranceSymbolRule[iRule, iSymbol]);
					this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->maxPoG[iRule, iSymbol];
				}
			}
		}
	}

#if _PMI_PROBLEM_VERBOSE_ >= 2
	for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule]->Value());
		for (size_t iSymbol = this->masterVocabulary->IndexModulesStart; iSymbol <= this->masterVocabulary->IndexConstantsEnd; iSymbol++)
		{
			Console::Write(" " + this->vocabulary->Symbols[iSymbol]->Value() + ":" + this->maxOccuranceSymbolRule[iRule, iSymbol]);
		}
		Console::WriteLine("");
	}
	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}

void PMIProblem::ComputeGrowthPattern_Unaccounted()
{
	// Compute the unaccounted for growth
	List<Word^>^ unaccounted = gcnew List<Word^>;
	unaccounted->Add(this->evidence[0]);

	for (size_t iGen = this->currentSet->startGeneration+1; iGen <= this->currentSet->endGeneration; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		this->vocabulary->CompressSymbols(a);
		for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
		{
			if (iSymbol >= this->vocabulary->numModules)
			{
				a->parikhVector[iSymbol] -= this->evidence[iGen - 1]->parikhVector[iSymbol];
			}
			for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
			{
				a->parikhVector[iSymbol] -= this->evidence[iGen - 1]->parikhVector[iRule] * this->minPoG[iRule, iSymbol];
			}
			if (a->parikhVector[iSymbol] < 0)
			{
				this->flagAnalysisFailed = true;
			}
		}
		unaccounted->Add(a);
	}

	Int32 idx = 0;
	this->fvMaxOccurance = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols()*this->vocabulary->NumSymbols());
	this->fvDeltaMaxOccurance = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols()*this->vocabulary->NumSymbols());

	Int32 iUAG = 0;
	for (size_t iGen = this->currentSet->startGeneration; iGen < this->currentSet->endGeneration; iGen++)
	{
		// Get the two generations of evidence
		Word^ ua1 = unaccounted[iUAG + 1];
		Word^ ua0 = unaccounted[iUAG];
		Word^ e1 = this->evidence[iGen + 1];
		Word^ e0 = this->evidence[iGen];

		idx = 0;
		// For each symbol figure out the maximum growth by symbol
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
		{
			Int32 count = 0;
			Int32 unsolvedRuleIndex = 0;
			for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
			{
				if ((e0->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
				{
					if (this->minPoG[iRule, iSymbol] == this->maxPoG[iRule, iSymbol])
					{
						this->PoG[iGen, iRule, iSymbol] = this->minPoG[iRule, iSymbol];
						count++;
					}
					else if (iSymbol < this->vocabulary->numModules)
					{
						this->PoG[iGen, iRule, iSymbol] = this->minPoG[iRule, iSymbol] + (int)Math::Floor((ua1->parikhVector[iSymbol]) / e0->parikhVector[iRule]);
						unsolvedRuleIndex = iRule;
					}
					else
					{
						this->PoG[iGen, iRule, iSymbol] = this->minPoG[iRule, iSymbol] + (int)Math::Floor((ua1->parikhVector[iSymbol]) / e0->parikhVector[iRule]);
						unsolvedRuleIndex = iRule;
					}

					if (this->PoG[iGen, iRule, iSymbol] < 0)
					{
						this->flagAnalysisFailed = true;
					}

					// Compute feature vector
					if ((e0->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
					{
						if ((e0->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
						{
							this->fvMaxOccurance[iGen, idx] = Math::Floor((e1->parikhVector[iSymbol]) / e0->parikhVector[iRule]);
						}
						else
						{
							this->fvMaxOccurance[iGen, idx] = Math::Floor((e1->parikhVector[iSymbol] - e0->parikhVector[iSymbol]) / e0->parikhVector[iRule]);
						}
					}

					if (iGen >= 1)
					{
						this->fvDeltaMaxOccurance[iGen - 1, idx] = this->fvMaxOccurance[iGen, idx] - this->fvMaxOccurance[iGen - 1, idx];
					}
					idx++;
				}
				else
				{
					count++;
				}
			}
			if (count == this->vocabulary->numModules - 1)
			{
				Int32 tmp = this->minPoG[unsolvedRuleIndex, iSymbol];
				this->minPoG[unsolvedRuleIndex, iSymbol] = this->PoG[iGen, unsolvedRuleIndex, iSymbol];
				// if the min PoG has changed, update the ua values for this symbol
				if (this->minPoG[unsolvedRuleIndex, iSymbol] != tmp)
				{
					for (size_t iGen2 = iUAG+1; iGen2 < unaccounted->Count; iGen2++)
					{
						unaccounted[iGen2]->parikhVector[iSymbol] -= this->evidence[this->currentSet->startGeneration + iGen2 - 1]->parikhVector[unsolvedRuleIndex] * (this->minPoG[unsolvedRuleIndex, iSymbol] - tmp);
					}
				}
			}
		}

		iUAG++;
	}

	for (size_t iGen = this->currentSet->startGeneration; iGen < this->currentSet->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->PoG->GetUpperBound(1); iRule++)
		{
			if ((this->evidence[iGen]->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
			{
				for (size_t iSymbol = 0; iSymbol <= this->PoG->GetUpperBound(2); iSymbol++)
				{
					if (this->PoG[iGen, iRule, iSymbol] < this->maxPoG[iRule, iSymbol])
					{
						this->UpdateMaxPoG(iRule, iSymbol, this->PoG[iGen, iRule, iSymbol]);
					}
				}
			}
		}
	}

	//// check to see if for any symbol all but one rule is solved
	//for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
	//{
	//	Int32 count = 0;
	//	Int32 idx;
	//	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//	{
	//		if (this->minPoG[iRule, iSymbol] == this->maxPoG[iRule, iSymbol])
	//		{
	//			count++;
	//		}
	//		else
	//		{
	//			idx = iRule; // record which one doesn't match
	//		}
	//	}

	//	if (count == this->vocabulary->numModules - 1)
	//	{
	//		this->minPoG[idx, iSymbol] = this->maxPoG[idx, iSymbol];
	//	}
	//}

	// Compute the total PoG
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			this->totalPoG[iSymbol] += this->maxPoG[iRule, iSymbol];
		}
	}
}

void PMIProblem::ComputeLexicon()
{
	Int32 longest = 0;
	Int32 longestMO = 0;
	Int32 shortest = 99999999;
	Int32 shortestMO = 99999999;

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

		if (this->minRuleLengthsModulesOnly[iRule] < shortestMO)
		{
			shortestMO = this->minRuleLengthsModulesOnly[iRule];
		}

		if (this->maxRuleLengthsModulesOnly[iRule] > longestMO)
		{
			longestMO = this->maxRuleLengthsModulesOnly[iRule];
		}
	}

	this->processMin = shortest;
	this->processMax = longest;

	this->lexicon = gcnew Lexicon(this->generations, shortest, longest, this->vocabularyMaster, this->minRuleLengths, this->maxRuleLengths, this->ruleHead, this->ruleTail, this->ruleFragments, this->totalPoG, this->minPoG, this->maxPoG);
	this->lexiconModuleOnly = gcnew Lexicon(this->generations, shortestMO, longestMO, this->vocabularyModuleOnly, this->minRuleLengthsModulesOnly, this->maxRuleLengthsModulesOnly, this->ruleHead, this->ruleTail, this->ruleFragments, this->totalPoG, this->minPoG, this->maxPoG);

	if (!this->lexicon->Load(this->lexiconFileName))
	{
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
		Console::WriteLine("Building Lexicon");
		this->ComputeLexiconProcess(this->evidence, this->lexicon, this->maxRuleLengths);
		Console::WriteLine("Compiling Lexicon");
		this->lexicon->CompileLexiconT(this->evidence);
		Console::WriteLine("Compressing Lexicon");
		this->lexicon->Compress();
#if _PMI_PROBLEM_LEXICON_SAVE_ > 0
		this->lexicon->Save(this->lexiconFileName);
#endif
#endif
	}

	if (!this->lexiconModuleOnly->Load(this->lexiconModuleOnlyFileName))
	{
		Console::WriteLine("Compiling Module Only Lexicon");
		this->lexiconModuleOnly->Filter(this->lexicon);
#if _PMI_PROBLEM_LEXICON_MO_SAVE_ > 0
		this->lexiconModuleOnly->Save(this->lexiconModuleOnlyFileName);
#endif
	}
}

void PMIProblem::ComputeLexiconProcessT(Object^ PO)
{
	LexiconThreadParams^ P = (LexiconThreadParams^)PO;
	this->ComputeLexiconProcess(P->evidence, P->lexicon, P->ruleLengths);
}

void PMIProblem::ComputeLexiconInnerProcess(Int32 iGen)
{
	Int32 iLength = this->processMin;
	bool stop = false;
	Int32 count = 0;
	Int32 highestCount = 0;
	while ((!stop) && (iLength <= this->processL->subwordCounts->GetUpperBound(1)) && (iLength <= this->processE[iGen]->symbols->Count))
	{

		Int32 start = 0;
		Int32 end = start + iLength;
		highestCount = 0;
		Word^ sw = gcnew Word();

		// produce the first word
		if ((end - 1) < this->processE[iGen]->symbols->Count)
		{
			for (size_t iSymbol = start; iSymbol < end; iSymbol++)
			{
				sw->Add(this->processE[iGen]->symbols[iSymbol]);
			}
			this->processL->FoundSubword(sw, iGen);
		}

		start++;
		end = start + iLength;

		// for every other word, remove the first symbol and add the next symbol over
		while ((end - 1) < this->processE[iGen]->symbols->Count)
		{
			sw->symbols->RemoveAt(0);
			sw->symbols->Add(this->processE[iGen]->symbols[end - 1]);
			count = this->processL->FoundSubword(sw, iGen);

			if (count > highestCount)
			{
				highestCount = count;
			}
			start++;
			end = start + iLength;
		}

		iLength++;

		// Check for the stopping condition. For every rule
		// 1 - the highest count is greater than the number of modules in the previous generation
		// 2 - the length is greater than the max length of the rule
		stop = true;
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			stop = stop && ((this->evidence[iGen - 1]->parikhVector[iRule] == 0) || (highestCount < this->evidence[iGen-1]->parikhVector[iRule]) || (iLength > this->maxRuleLengths[iRule]));
		}

		//if (stop)
		//{
		//	Console::WriteLine("Early stopping: " + iGen + " : " + iLength + " : " + highestCount);
		//}
	}

	iGen++;
}

void PMIProblem::ComputeLexiconProcess(List<Word^>^ E, Lexicon^ L, array<Int32>^ RuleLengths)
{
	this->processL = L;
	this->processE = E;
	this->processRuleLengths = RuleLengths;

	this->processLowestModuleCount = gcnew array<Int32>(this->generations);

	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		this->processLowestModuleCount[iGen] = 99999999;
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			if (E[iGen]->parikhVector[iRule] < this->processLowestModuleCount[iGen])
			{
				this->processLowestModuleCount[iGen] = E[iGen]->parikhVector[iRule];
			}
		}
	}

	Int32 iLength;
	Int32 iGen = 1;
	bool verified = true;
	Int32 count = 0;
	Int32 highestCount;
	System::Threading::Tasks::ParallelOptions^ options = gcnew System::Threading::Tasks::ParallelOptions();
	options->MaxDegreeOfParallelism = 5;

	System::Threading::Tasks::Parallel::For(1, this->finalGeneration + 1, options, gcnew Action<int>(this, &PMIProblem::ComputeLexiconInnerProcess));

	//do
	//{
	//	highestCount = 0;
	//	iLength = 1;
	//	while ((iLength <= L->subwordCounts->GetUpperBound(1)) && (iLength <= E[iGen]->symbols->Count))
	//	{
	//		Console::WriteLine(iGen + ":" + iLength);
	//		Int32 start = 0;
	//		Int32 end = start + iLength;
	//		Word^ sw = gcnew Word();

	//		// produce the first word
	//		if ((end - 1) < E[iGen]->symbols->Count)
	//		{
	//			for (size_t iSymbol = start; iSymbol < end; iSymbol++)
	//			{
	//				sw->Add(E[iGen]->symbols[iSymbol]);
	//			}
	//			count = L->FoundSubword(sw, iGen);
	//			if (count > highestCount)
	//			{
	//				highestCount = count;
	//			}
	//		}

	//		start++;
	//		end = start + iLength;

	//		// for every other word, remove the first symbol and add the next symbol over
	//		while ((end - 1) < E[iGen]->symbols->Count)
	//		{
	//			sw->symbols->RemoveAt(0);
	//			sw->symbols->Add(E[iGen]->symbols[end-1]);
	//			count = L->FoundSubword(sw, iGen);

	//			if (count > highestCount)
	//			{
	//				highestCount = count;
	//			}
	//			start++;
	//			end = start + iLength;
	//		}

	//		iLength++;
	//	}

	//	if (highestCount < lowestModuleCount[iGen])
	//	{
	//		verified = false;
	//	}

	//	iGen++;
	//	// since final generation is the last generation in which anything is produced  need to look at the next generation (+1) to see what was produced
	//} while ((iGen < this->finalGeneration+1) && (verified));
}

void PMIProblem::ComputeLexiconSymbol()
{
	this->symbolLexicon = gcnew array<Int32, 2>(this->vocabulary->NumSymbols(), this->vocabulary->NumSymbols());
	this->symbolLexiconInverted = gcnew array<Int32, 2>(this->vocabulary->NumSymbols(), this->vocabulary->NumSymbols());

	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		if (this->evidence[iGen]->symbols->Count > 1)
		{
			Int32 prevSymbolIndex1 = this->vocabulary->FindIndex(this->evidence[iGen]->symbols[0]);
			Int32 prevSymbolIndex2 = this->vocabulary->FindIndex(this->evidence[iGen]->symbols[0]);
			for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
			{
				if (iSymbol > 0)
				{
					Int32 symbolIndex = this->vocabulary->FindIndex(this->evidence[iGen]->symbols[iSymbol]);
					//Console::WriteLine(this->evidence[iGen]->symbols[iSymbol-1]->Value() + " (" + prevSymbolIndex1 + ") > " + this->evidence[iGen]->symbols[iSymbol]->Value() + " (" + symbolIndex + ")");
					symbolLexicon[prevSymbolIndex1, symbolIndex]++;
					prevSymbolIndex1 = symbolIndex;
				}

				if (iSymbol < this->evidence[iGen]->symbols->Count - 1)
				{
					Int32 symbolIndex = this->vocabulary->FindIndex(this->evidence[iGen]->symbols[iSymbol+1]);
					//Console::WriteLine(this->evidence[iGen]->symbols[iSymbol]->Value() + " < " + this->evidence[iGen]->symbols[iSymbol+1]->Value());
					symbolLexiconInverted[symbolIndex, prevSymbolIndex2]++;
					prevSymbolIndex2 = symbolIndex;
				}
			}
		}
	}
}

void PMIProblem::ComputeModulesToSolve(PMIProblemDescriptor^ P)
{
	for (size_t iGen = P->startGeneration; iGen < P->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->symbolSubsets->GetUpperBound(1); iRule++)
		{
			P->modulesToSolve[iRule] = P->modulesToSolve[iRule] || this->symbolSubsets[iGen, iRule];
		}
	}
}


void PMIProblem::ComputeNeighbouringSymbols()
{
	this->ComputeNeighbouringSymbols_Cooccurance();
	this->ComputeNeighbouringSymbols_HoNS();
}

void PMIProblem::ComputeNeighbouringSymbols_Cooccurance()
{
	List<Symbol^>^ constants = this->vocabulary->AllConstants();
	List<Word^>^ filteredEvidence = gcnew List<Word^>;

	// Make a copy of all the evidence while removing the constants
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		a->FilterOutSymbol(constants);
		filteredEvidence->Add(a);
	}

#if _PMI_PROBLEM_VERBOSE_ >= 3
	Console::WriteLine("Filtered Evidence");

	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
	Console::WriteLine();
#endif

	List<List<Symbol^>^>^ combinations = gcnew List<List<Symbol^>^>;

	for (size_t i = this->vocabulary->IndexModulesStart; i <= this->vocabulary->IndexModulesEnd; i++)
	{
		for (size_t j = this->vocabulary->IndexModulesStart; j <= this->vocabulary->IndexModulesEnd; j++)
		{
			List<Symbol^>^ x = gcnew List<Symbol^>;
			x->Add(this->vocabulary->Symbols[i]);
			x->Add(this->vocabulary->Symbols[j]);
			combinations->Add(x);
		}
	}

	Int32 firstGen = 2;

	//// Left Neighbour
	//array<float, 2>^ h1 = gcnew array<float, 2>(this->generations - 1, combinations->Count);
	//for (size_t iGen = firstGen; iGen < this->generations; iGen++)
	//{
	//	Int32 count = 0;
	//	for (size_t iSymbol = 1; iSymbol < filteredEvidence[iGen]->symbols->Count; iSymbol++)
	//	{
	//		List<Symbol^>^ pair = gcnew List<Symbol^>;
	//		pair->Add(filteredEvidence[iGen]->symbols[iSymbol]);
	//		pair->Add(filteredEvidence[iGen]->symbols[iSymbol - 1]);

	//		for (size_t i = 0; i < combinations->Count; i++)
	//		{
	//			List<Symbol^>^ candidate = combinations[i];
	//			if ((candidate[0]->Value() == pair[0]->Value()) && (candidate[1]->Value() == pair[1]->Value()))
	//			{
	//				//Console::WriteLine(candidate[0]->Value() + " " + candidate[1]->Value() + " " + pair[0]->Value() + " " + pair[1]->Value());
	//				h1[iGen-firstGen, i]++;
	//				count++;
	//			}	
	//		}
	//	}

	//	for (size_t i = 0; i < h1->GetUpperBound(1); i++)
	//	{
	//		h1[iGen-firstGen, i] /= count;
	//	}
	//}

	// Right Neighbour
	this->fvCooccurance = gcnew array<float, 2>(this->generations - 1, combinations->Count);
	this->fvDeltaCooccurance = gcnew array<float, 2>(this->generations - 2, combinations->Count);

	for (size_t iGen = firstGen; iGen < this->generations; iGen++)
	{
		Int32 count = 0;
		for (size_t iSymbol = 0; iSymbol < filteredEvidence[iGen]->symbols->Count - 1; iSymbol++)
		{
			List<Symbol^>^ pair = gcnew List<Symbol^>;
			pair->Add(filteredEvidence[iGen]->symbols[iSymbol]);
			pair->Add(filteredEvidence[iGen]->symbols[iSymbol + 1]);

			for (size_t i = 0; i < combinations->Count; i++)
			{
				List<Symbol^>^ candidate = combinations[i];
				if ((candidate[0]->Value() == pair[0]->Value()) && (candidate[1]->Value() == pair[1]->Value()))
				{
					//Console::WriteLine(candidate[0]->Value() + " " + candidate[1]->Value() + " " + pair[0]->Value() + " " + pair[1]->Value());
					this->fvCooccurance[iGen - firstGen, i]++;
					count++;
				}
			}
		}

		for (size_t i = 0; i < this->fvCooccurance->GetUpperBound(1); i++)
		{
			this->fvCooccurance[iGen - firstGen, i] /= count;
			
			if (iGen - firstGen >= 1)
			{
				this->fvDeltaCooccurance[iGen - firstGen, i] = this->fvCooccurance[iGen - firstGen, i] - this->fvCooccurance[iGen - firstGen - 1, i];
			}
		}
	}
}

void PMIProblem::ComputeNeighbouringSymbols_HoNS()
{
	List<Symbol^>^ constants = this->vocabulary->AllConstants();
	List<Word^>^ filteredEvidence = gcnew List<Word^>;

	// Make a copy of all the evidence while removing the constants
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		a->FilterOutSymbol(constants);
		filteredEvidence->Add(a);
	}

#if _PMI_PROBLEM_VERBOSE_ >= 3
	Console::WriteLine("Filtered Evidence");

	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
	Console::WriteLine();
#endif

	List<List<Symbol^>^>^ combinations = gcnew List<List<Symbol^>^>;
	array<Int32, 2>^ indexCombinations = MathUtility::Combinations(this->vocabulary->IndexModulesStart, this->vocabulary->IndexModulesEnd, 3);

	for (size_t iCombination = 0; iCombination <= indexCombinations->GetUpperBound(0); iCombination++)
	{
		List<Symbol^>^ x = gcnew List<Symbol^>;
		for (size_t iSymbol = 0; iSymbol <= indexCombinations->GetUpperBound(1); iSymbol++)
		{
			x->Add(this->vocabulary->Symbols[indexCombinations[iCombination, iSymbol]]);
		}
		combinations->Add(x);
	}

	Int32 firstGen = 2;

	// Right Neighbour
	this->fvCooccurance = gcnew array<float, 2>(this->generations - 1, combinations->Count);
	this->fvDeltaCooccurance = gcnew array<float, 2>(this->generations - 2, combinations->Count);

	for (size_t iGen = firstGen; iGen < this->generations; iGen++)
	{
		Int32 count = 0;
		for (size_t iSymbol = 0; iSymbol < filteredEvidence[iGen]->symbols->Count - 1; iSymbol++)
		{
			List<Symbol^>^ pair = gcnew List<Symbol^>;
			pair->Add(filteredEvidence[iGen]->symbols[iSymbol]);
			pair->Add(filteredEvidence[iGen]->symbols[iSymbol + 1]);

			for (size_t i = 0; i < combinations->Count; i++)
			{
				List<Symbol^>^ candidate = combinations[i];
				if ((candidate[0]->Value() == pair[0]->Value()) && (candidate[1]->Value() == pair[1]->Value()))
				{
					//Console::WriteLine(candidate[0]->Value() + " " + candidate[1]->Value() + " " + pair[0]->Value() + " " + pair[1]->Value());
					this->fvCooccurance[iGen - firstGen, i]++;
					count++;
				}
			}
		}

		for (size_t i = 0; i < this->fvCooccurance->GetUpperBound(1); i++)
		{
			this->fvCooccurance[iGen - firstGen, i] /= count;

			if (iGen - firstGen >= 1)
			{
				this->fvDeltaCooccurance[iGen - firstGen, i] = this->fvCooccurance[iGen - firstGen, i] - this->fvCooccurance[iGen - firstGen - 1, i];
			}
		}
	}
}

// TODO: This needs to be more sophisticated
// Currently it is assumed that all rules with the same predecessaro are identical
// Needs to take check whether the successor is the same
// When there are differences needs to combines those rules with the same successor
// and subsequent start/end generations
void PMIProblem::CompressSolution()
{
	List<Symbol^>^ completed = gcnew List<Symbol^>;
	List<ProductionRule^>^ rulesFinal = gcnew List<ProductionRule^>;

	for (size_t iRule = 0; iRule < this->model->rules->Count; iRule++)
	{
		if (!completed->Contains(this->model->rules[iRule]->predecessor))
		{
			completed->Add(this->model->rules[iRule]->predecessor);
			this->model->rules[iRule]->constraints->RemoveRange(0, this->model->rules[iRule]->constraints->Count);
			rulesFinal->Add(this->model->rules[iRule]);
		}
	}

}

void PMIProblem::ComputeParikhVectors()
{
	this->rulesParikhTemplate = gcnew array<Int32, 3>(this->generations, this->vocabularyMaster->numModules, this->vocabularyMaster->NumSymbols());

	for (size_t iGen = 0; iGen <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Gen); iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				if (this->symbolSubsets[iGen, iRule])
				{
					this->rulesParikhTemplate[iGen, iRule, iSymbol] = PMIProblem::cUnknownParikhValue;
				}
				else
				{
					this->rulesParikhTemplate[iGen, iRule, iSymbol] = 0;
				}
			}
		}
	}

	this->knownRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	for (size_t i = 0; i <= this->knownRuleLengths->GetUpperBound(0); i++)
	{
		this->knownRuleLengths[i] = PMIProblem::cUnknownRuleLength;
	}

#if _PMI_PROBLEM_VERBOSE_ >= 3
	this->OutputRPT();
#endif
}

// Name: ComputeProduction
// Purpose: Takes two words in sentential form and computes the set of symbols produced. I.e. it removes the constants from the 2nd word
// Inputs: 
//	A - the predecessor word
//  B - the produced word
// Outputs:
// result - a Parikh vector representing the symbol set

array<Int32>^ PMIProblem::ComputeProduction(Word^ A, Word^ B)
{
	array<Int32>^ result = gcnew array<Int32>(this->vocabulary->NumSymbols());

	for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
	{
		if (iSymbol < this->vocabulary->numModules)
		{
			result[iSymbol] = B->parikhVector[iSymbol];
		}
		else
		{
			result[iSymbol] = B->parikhVector[iSymbol] - A->parikhVector[iSymbol];
		}
	}

	return result;
}

// Name:ComputeRuleHeadsTails
// Purpose: The minimum production for the 1st module in each generation and last module in each generation can be determined by using constants as markers
// Inputs: None
// Outputs: None
// TODO: 
// - can be extended to handle the following situation
// A++B
// A++B...
// Currently it will find A+, but it can actually find A++ because in the 2nd generation there are only 2 ++ symbols and not 3 (or more)
// Similar logic works for tail

void PMIProblem::ComputeRuleHeadsTails()
{
	for (size_t iGen = 0; iGen < this->generations-1; iGen++)
	{
		// Find first module in generation
		Int32 iFirst = 0;
		Int32 iLast = this->evidence[iGen]->symbols->Count - 1;
		Int32 tailSkips = 0;
		 
		while ((iFirst < this->evidence[iGen]->symbols->Count) && (!this->vocabulary->IsModule(this->evidence[iGen]->symbols[iFirst])))
		{
			iFirst++;
		}

		while ((!this->vocabulary->IsModule(this->evidence[iGen]->symbols[iLast])) && (iLast > 0))
		{
			iLast--;
			tailSkips++;
		}

		if (iFirst < this->evidence[iGen]->symbols->Count)
		{
			Int32 ruleIndex1 = this->vocabulary->RuleIndex(this->evidence[iGen]->symbols[iFirst]);
			Int32 ruleIndex2 = this->vocabulary->RuleIndex(this->evidence[iGen]->symbols[iLast]);

			// Find the right neighbour to the 1st symbol and left neighbour of the last symbol

			Word^ head = gcnew Word();
			head->Add(this->evidence[iGen + 1]->symbols[iFirst]);
			Symbol^ right = this->evidence[iGen]->symbols[iFirst + 1];

			// A module is not allowed to produce only itself, so add the next symbol
			if (head[0] == this->vocabulary->Symbols[ruleIndex1])
			{
				iFirst++;
				head->Add(this->evidence[iGen + 1]->symbols[iFirst]);
			}

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

			Word^ tail = gcnew Word();
			// the index is the length of the evidence - the number of symbols skipped from the back
			Int32 idx = this->evidence[iGen + 1]->symbols->Count - (tailSkips)-1;
			tail->Add(this->evidence[iGen + 1]->symbols[idx]);
			Symbol^ left = this->evidence[iGen]->symbols[iLast - 1];

			// A rule is not allowed to produce only itself
			if (tail[0] == this->vocabulary->Symbols[ruleIndex2])
			{
				idx--;
				tail->Add(this->evidence[iGen + 1]->symbols[idx]);
			}

			if (this->vocabulary->IsConstant(left))
			{
				idx--;
				while (this->evidence[iGen + 1]->symbols[idx] != left)
				{
					tail->Add(this->evidence[iGen + 1]->symbols[idx]);
					idx--;
				}
			}

			if (head->Count > this->ruleHead[ruleIndex1]->Count)
			{
				this->ruleHead[ruleIndex1] = head;
			}

			if (tail->Count > this->ruleTail[ruleIndex2]->Count)
			{
				this->ruleTail[ruleIndex2] = tail;
			}
		}
	}

	for (size_t iRule = 0; iRule <= this->ruleTail->GetUpperBound(0); iRule++)
	{
		this->ruleTail[iRule]->symbols->Reverse();
	}
}

UInt64 PMIProblem::ComputeSolutionSpaceSize()
{
	return 0;
}

void PMIProblem::ComputeSymbiology()
{
	this->symbolSubsets = gcnew array<bool, 2>(this->generations, this->vocabulary->IndexModulesEnd - this->vocabulary->IndexModulesStart + 1);
	this->lastGenerationSymbol = gcnew array<Int32>(this->vocabulary->IndexModulesEnd - this->vocabulary->IndexModulesStart + 1);

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
					this->lastGenerationSymbol[iSymbol] = iGen-1;
				}
			}
		}
		else
		{
			this->lastGenerationSymbol[iSymbol] = this->generations-1;
		}
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

float PMIProblem::CompareEvidence_Consistency(List<ProductionRule^>^ Rules, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	int errors = 0;
	float errorValue = 0.0;
	int total = 0;
	float result = 0.0;
	int generationsSolved = 0;

	ConsistencyTable^ C = this->CreateConsistencyTable(Rules);

	return result;
}

float PMIProblem::CompareEvidence_Positional(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	int errors = 0;
	float errorValue = 0.0;
	int total = 0;
	float result = 0.0;
	int generationsSolved = 0;

	for (size_t iGen = 0; iGen < E1->Count; iGen++)
	{
		total += std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count);

		// Check results versus evidence
		Int32 iSymbol = 0;
		while ((iSymbol < std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count)) && (iSymbol < this->solutionLimiter))
		{
			// TODO: make it so that it prefers to find modules
			if ((iSymbol < E1[E1_Start + iGen]->symbols->Count) && (iSymbol < E2[E2_Start + iGen]->symbols->Count))
			{
				//Console::WriteLine(E1[E1_Start + iGen]->symbols[iSymbol] + " " + E2[E2_Start + iGen]->symbols[iSymbol]);
				if (E1[E1_Start + iGen]->symbols[iSymbol] != E2[E2_Start + iGen]->symbols[iSymbol])
				{
					errorValue += (float)1 / (1 + iGen);
					errors++; // symbols do not match
					//if (this->vocabulary->IsModule(E2[E2_Start + iGen]->symbols[iSymbol]))
					//{
					//	errorValue += (float)2 / (1 + iGen);
					//	errors++; // symbols do not match
					//}
					//else
					//{
					//	errorValue += (float)1 / (1 + iGen);
					//	errors++; // symbols do not match
					//}
				}
			}
			else
			{
				errorValue += (float)1 / (1 + iGen);
				errors++; // result is too long or not long enough
			}
			iSymbol++;
		}

		if ((errors == 0) && (iSymbol >= this->solutionLimiter))
		{
			for (size_t iSymbol = this->solutionLimiter; iSymbol < std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count); iSymbol++)
			{
				if ((iSymbol < E1[E1_Start + iGen]->symbols->Count) && (iSymbol < E2[E2_Start + iGen]->symbols->Count))
				{
					if (E1[E1_Start + iGen]->symbols[iSymbol] != E2[E2_Start + iGen]->symbols[iSymbol])
					{
						errorValue += (float)1 / (1 + iGen);
						errors++; // symbols do not match
						//if (this->vocabulary->IsModule(E2[E2_Start + iGen]->symbols[iSymbol]))
						//{
						//	errorValue += (float)2 / (1 + iGen);
						//	errors++; // symbols do not match
						//}
						//else
						//{
						//	errorValue += (float)1 / (1 + iGen);
						//	errors++; // symbols do not match
						//}
					}
				}
				else
				{
					errorValue += (float)1 / (1 + iGen);
					errors++; // result is too long or not long enough
				}
			}
			//if (errors == 0)
			//{
			//	generationsSolved++;
			//}
		}
		//else if (errors == 0)
		//{
		//	generationsSolved++;
		//}
	}

	//result = (this->generations - generationsSolved) + (float)errorValue / total;
	result = (float)errorValue / total;

	return result;
}

float PMIProblem::CompareEvidence_PositionalAbort(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	int errors = 0;
	float errorValue = 0.0;
	int total = 0;
	float result = 0.0;
	int generationsSolved = 0;
	bool abort = false;
	Int32 iGen = 0;

	while (!(abort) && (iGen < E1->Count))
	{
		if (E1[iGen]->symbols->Count != E2[iGen]->symbols->Count)
		{
			abort = true;
		}

		iGen++;
	}

	iGen = 0;

	while (!(abort) && (iGen < E1->Count))
	{
		total += std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count);

		// Check results versus evidence
		Int32 iSymbol = 0;
		while ((!abort) && (iSymbol < std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count)) && (iSymbol < this->solutionLimiter))
		{
			// TODO: make it so that it prefers to find modules
			if ((iSymbol < E1[E1_Start + iGen]->symbols->Count) && (iSymbol < E2[E2_Start + iGen]->symbols->Count))
			{
				//Console::WriteLine(E1[E1_Start + iGen]->symbols[iSymbol] + " " + E2[E2_Start + iGen]->symbols[iSymbol]);
				if (E1[E1_Start + iGen]->symbols[iSymbol] != E2[E2_Start + iGen]->symbols[iSymbol])
				{
					abort = true;
					errors++;
				}
			}
			else
			{
				abort = true;
				errors++; // result is too long or not long enough
			}
			iSymbol++;
		}

		if ((errors == 0) && (iSymbol >= this->solutionLimiter))
		{
			for (size_t iSymbol = this->solutionLimiter; iSymbol < std::max(E1[E1_Start + iGen]->symbols->Count, E2[E2_Start + iGen]->symbols->Count); iSymbol++)
			{
				if ((iSymbol < E1[E1_Start + iGen]->symbols->Count) && (iSymbol < E2[E2_Start + iGen]->symbols->Count))
				{
					//Console::WriteLine(E1[E1_Start + iGen]->symbols[iSymbol] + " " + E2[E2_Start + iGen]->symbols[iSymbol]);
					if (E1[E1_Start + iGen]->symbols[iSymbol] != E2[E2_Start + iGen]->symbols[iSymbol])
					{
						abort = true;
						errors++;
					}
				}
				else
				{
					abort = true;
					errors++;
				}
			}
		}
		iGen++;
	}

	//result = (this->generations - generationsSolved) + (float)errorValue / total;
	if (!abort)
	{
		result = 0.0f;
	}
	else
	{
		result = 1.0f;
	}

	return result;
}

float PMIProblem::CompareEvidence_Indexed(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	return 0.0;
}

ConsistencyTable^ PMIProblem::CreateConsistencyTable(array<Int32>^ RuleLengths)
{
	ConsistencyTable^ result = gcnew ConsistencyTable(this->vocabulary->numModules);
	array<Int32>^ ruleLengths = RuleLengths;

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
	Console::WriteLine("Evidence");

	Console::WriteLine("E1: " + E1->ToString());
	Console::WriteLine("E2: " + E2->ToString());
	Console::WriteLine();
#endif

	Int32 idx;
	Int32 iGen = 0;

	do
	{
		Word^ E1 = this->evidence[this->currentProblem->startGeneration + iGen];
		Word^ E2 = this->evidence[this->currentProblem->startGeneration + 1 + iGen];
		idx = 0;

		for each (Symbol^ s in E1->symbols)
		{
			// If this is a modules which has not been process, then process it
			if (this->vocabulary->IsModule(s))
			{
				Symbol^ predecessor;
				List<Symbol^>^ successor = gcnew List<Symbol^>;
				predecessor = s;

				Int32 iRule = this->vocabulary->RuleIndex(s);

				//Console::WriteLine(s->ToString() + " " + ruleLength + " " + idx);
				for (size_t iSymbol = 0; iSymbol < ruleLengths[iRule]; iSymbol++)
				{
					if (idx < E2->symbols->Count)
					{
						successor->Add(E2->symbols[idx]);
						idx++;
					}
				}

				// Build constraint(s) based on the sub-problem
				List<IProductionRuleConstraint^>^ C = gcnew List<IProductionRuleConstraint^>;
				ProductionRuleConstraint_Time^ C1 = gcnew ProductionRuleConstraint_Time(this->startGeneration, this->endGeneration);
				C->Add(C1);

				result->Add(iRule, gcnew ConsistencyItem(gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor, C)));
			}
			// If it is a constant, then skip one symbol
			else
			{
				idx++;
			}
		}

		iGen++;
	} while (this->currentProblem->startGeneration + 1 + iGen < this->evidence->Count);

#if _PMI_PROBLEM_SHOW_CONSISTENCY_TABLE_ >= 1
	result->Display();
#endif

	return result;
}

ConsistencyTable^ PMIProblem::CreateConsistencyTable(array<Int32, 2>^ Rules)
{
	ConsistencyTable^ result = gcnew ConsistencyTable(this->vocabulary->numModules);
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= Rules->GetUpperBound(1); iSymbol++)
		{
			ruleLengths[iRule] += Rules[iRule, iSymbol];
		}
	}

	result = this->CreateConsistencyTable(ruleLengths);

	return result;
}

ConsistencyTable^ PMIProblem::CreateConsistencyTable(List<ProductionRule^>^ Rules)
{
	ConsistencyTable^ result = gcnew ConsistencyTable(this->vocabulary->numModules);
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		ruleLengths[iRule] = Rules[iRule]->successor->Count;
	}

	result = this->CreateConsistencyTable(ruleLengths);

	return result;
}

void PMIProblem::CreateRuleTemplate()
{
	this->rulesTemplate = gcnew List<ProductionRule^>;

	// TODO:
	// In this function, I need to walk the Parikh template and create a rule
	// where the values in template are unchanged
	// with a time constraint
	// If a rule goes until the last generation it gets no "end time" constraint

	// Create a template for each rule which contains all of the symbols expressed in the corresponding Parikh vector but not ordered properly
	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ predecessor;
		List<Symbol^>^ successor = gcnew List<Symbol^>;

		predecessor = this->vocabulary->Symbols[iRule];

		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
		{
			for (size_t i = 0; i < this->rulesParikhTemplate[0, iRule,iSymbol]; i++)
			{
				successor->Add(this->vocabulary->Symbols[iSymbol]);
			}
		}

		ProductionRule^ R = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor);
		this->rulesTemplate->Add(R);
	}
}

List<ProductionRule^>^ PMIProblem::CreateRulesByScanning()
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	List<Symbol^>^ completed = gcnew List<Symbol^>;

	Word^ E1 = this->evidence[this->currentProblem->startGeneration];
	Word^ E2 = this->evidence[this->currentProblem->startGeneration+1];

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
	Console::WriteLine("Evidence");

	Console::WriteLine("E1: " + E1->ToString());
	Console::WriteLine("E2: " + E2->ToString());
	Console::WriteLine();
#endif


	Int32 idx = 0;
	for each (Symbol^ s in E1->symbols)
	{
		// If this is a modules which has not been process, then process it
		if ((this->vocabulary->IsModule(s)) && (!completed->Contains(s)))
		{
			Symbol^ predecessor;
			List<Symbol^>^ successor = gcnew List<Symbol^>;
			predecessor = s;

			Int32 iRule = this->vocabulary->RuleIndex(s);
			Int32 ruleLength = 0;

			// TODO: This should be walking the generations and order all rules which apply for that generation
			// TODO: should pre-check if existing order works
			// TODO: probably should be using the rulesTemplate since that's where the believed rules are stored
			
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				ruleLength += this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol];
			}

			for (size_t iSymbol = 0; iSymbol < ruleLength; iSymbol++)
			{
				successor->Add(E2->symbols[idx]);
				idx++;
			}

			completed->Add(s);

			// Build constraint(s) based on the sub-problem
			List<IProductionRuleConstraint^>^ C = gcnew List<IProductionRuleConstraint^>;
			ProductionRuleConstraint_Time^ C1 = gcnew ProductionRuleConstraint_Time(this->startGeneration, this->endGeneration);
			C->Add(C1);
			ProductionRule^ r = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor, C);

			R->Add(r);
		}
		// If is a module that has been processed then skip the length of its rules
		else if ((this->vocabulary->IsModule(s)) && (completed->Contains(s)))
		{
			Int32 iRule = this->vocabulary->RuleIndex(s);
			Int32 ruleLength = 0;
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				ruleLength += this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol];
			}
			
			idx += ruleLength;
		}
		// If it is a constant, then skip one symbol
		else
		{
			idx++;
		}
	}

	return R;
}

List<ProductionRule^>^ PMIProblem::CreateRulesByScanning(array<Int32, 3>^ ARPT)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	List<Symbol^>^ completed = gcnew List<Symbol^>;

	Word^ E1 = this->evidence[this->currentProblem->startGeneration];
	Word^ E2 = this->evidence[this->currentProblem->startGeneration + 1];

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
	Console::WriteLine("Evidence");

	Console::WriteLine("E1: " + E1->ToString());
	Console::WriteLine("E2: " + E2->ToString());
	Console::WriteLine();
#endif


	Int32 idx = 0;
	for each (Symbol^ s in E1->symbols)
	{
		// If this is a modules which has not been process, then process it
		if ((this->vocabulary->IsModule(s)) && (!completed->Contains(s)))
		{
			Symbol^ predecessor;
			List<Symbol^>^ successor = gcnew List<Symbol^>;
			predecessor = s;

			Int32 iRule = this->vocabulary->RuleIndex(s);
			Int32 ruleLength = 0;

			// TODO: This should be walking the generations and order all rules which apply for that generation
			// TODO: should pre-check if existing order works
			// TODO: probably should be using the rulesTemplate since that's where the believed rules are stored

			for (size_t iSymbol = 0; iSymbol <= ARPT->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				ruleLength += ARPT[this->currentProblem->startGeneration, iRule, iSymbol];
			}

			for (size_t iSymbol = 0; iSymbol < ruleLength; iSymbol++)
			{
				successor->Add(E2->symbols[idx]);
				idx++;
			}

			completed->Add(s);

			// Build constraint(s) based on the sub-problem
			List<IProductionRuleConstraint^>^ C = gcnew List<IProductionRuleConstraint^>;
			ProductionRuleConstraint_Time^ C1 = gcnew ProductionRuleConstraint_Time(this->startGeneration, this->endGeneration);
			C->Add(C1);
			ProductionRule^ r = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor, C);

			R->Add(r);
		}
		// If is a module that has been processed then skip the length of its rules
		else if ((this->vocabulary->IsModule(s)) && (completed->Contains(s)))
		{
			Int32 iRule = this->vocabulary->RuleIndex(s);
			Int32 ruleLength = 0;
			for (size_t iSymbol = 0; iSymbol <= ARPT->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				ruleLength += ARPT[this->currentProblem->startGeneration, iRule, iSymbol];
			}

			idx += ruleLength;
		}
		// If it is a constant, then skip one symbol
		else
		{
			idx++;
		}
	}

	return R;
}

List<ProductionRule^>^ PMIProblem::CreateRulesByScanning(array<Int32>^ RuleLengths)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	R = this->CreateRulesByScanning(RuleLengths, this->currentProblem->startGeneration, this->currentProblem->endGeneration);
	return R;
}

List<ProductionRule^>^ PMIProblem::CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;
	List<Symbol^>^ completed = gcnew List<Symbol^>;


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
		Word^ E1 = this->evidence[StartGeneration + iGen];
		Word^ E2 = this->evidence[StartGeneration + 1 + iGen];
		idx = 0;

		for each (Symbol^ s in E1->symbols)
		{
			// If this is a modules which has not been process, then process it
			if ((this->vocabulary->IsModule(s)) && (!completed->Contains(s)))
			{
				Symbol^ predecessor;
				List<Symbol^>^ successor = gcnew List<Symbol^>;
				predecessor = s;

				Int32 iRule = this->vocabulary->RuleIndex(s);
				Int32 ruleLength;

				if (this->knownRuleLengths[iRule] != PMIProblem::cUnknownRuleLength)
				{
					ruleLength = this->knownRuleLengths[iRule];
				}
				else
				{
					ruleLength = RuleLengths[this->rule2solution[iRule]];
				}

				//Console::WriteLine(s->ToString() + " " + ruleLength + " " + idx);
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

				completed->Add(s);

				if (!failed)
				{

					// Build constraint(s) based on the sub-problem
					List<IProductionRuleConstraint^>^ C = gcnew List<IProductionRuleConstraint^>;
					ProductionRuleConstraint_Time^ C1 = gcnew ProductionRuleConstraint_Time(StartGeneration, EndGeneration);
					C->Add(C1);
					ProductionRule^ r = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor, C);

					R->Add(r);
				}
			}
			// If is a module that has been processed then skip the length of its rules
			else if ((this->vocabulary->IsModule(s)) && (completed->Contains(s)))
			{
				Int32 iRule = this->vocabulary->RuleIndex(s);
				Int32 ruleLength;
				if (this->knownRuleLengths[iRule] != PMIProblem::cUnknownRuleLength)
				{
					ruleLength = this->knownRuleLengths[iRule];
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
	} while ((!failed) && (R->Count < this->vocabulary->numModules) && (StartGeneration + 1 + iGen < this->evidence->Count));

	if (failed)
	{
		return nullptr;
	}
	else
	{
		return R;
	}
}

GenomeConfiguration<Int32>^ PMIProblem::CreateGenome()
{
	//Int32 numGenes = 0;
	//// For module sub-problems, need a number of genes equal to 
	//// the number of unsolved module -> module pairs.

	//// For constant sub-problems, need a number of genes equal to 
	//// number of unsolved module -> this constant pairs

	//// For ordering sub-problems, need a number of genes equal to all symbols

	//if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Modules)
	//{
	//	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	//	{
	//		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	//		{
	//			// Only consider symbols which:
	//			// 1 - have not already been solved
	//			// 2 - Only those symbols which exist within the sub-problem
	//			// Assumptions:
	//			// 1 - This code assumes that the first generation of the symbol subsets describes all of the symbols which must be solved.
	//			//     Any symbols which appear afterwards must have been already solved.
	//			if ((this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol] == PMIProblem::cUnknownParikhValue) && (this->symbolSubsets[this->currentProblem->startGeneration, iRule]))
	//			{
	//				numGenes += 1;
	//			}
	//		}
	//	}
	//}
	//else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Constants)
	//{
	//	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	//	{
	//		// Only consider symbols which:
	//		// 1 - have not already been solved
	//		// 2 - Only those symbols which exist within the sub-problem
	//		// Assumptions:
	//		// 1 - This code assumes that the first generation of the symbol subsets describes all of the symbols which must be solved.
	//		//     Any symbols which appear afterwards must have been already solved.

	//		// TODO: Need to extend symbol subsets to include constants or add a 'constants subsets'
	//		if (this->symbolSubsets[this->currentProblem->startGeneration, iRule])
	//		{
	//			if (this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex] == PMIProblem::cUnknownParikhValue)
	//			{
	//				numGenes += 1;
	//			}
	//		}
	//	}
	//}
	//else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Order)
	//{
	//	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(0); iRule++)
	//	{
	//		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
	//		{
	//			numGenes += 1;
	//		}
	//	}
	//}

	//array<Int32>^ min = gcnew array<Int32>(numGenes);
	//array<Int32>^ max = gcnew array<Int32>(numGenes);

	//for (size_t i = 0; i < min->Length; i++)
	//{
	//	min[i] = PMIProblem::cGene_Min;
	//	max[i] = PMIProblem::cGene_Max;
	//}

	//GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes,min,max);
	//
	//return result;
	return nullptr;
}

// TODO: Need to extend to take into account that the RPT values may change from generation to generation
void PMIProblem::CreateModelFromRPT()
{
	Int32 iGen = 0;
	array<Int32>^ ruleLengths = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule <= ruleLengths->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
		{
			ruleLengths[iRule] += this->rulesParikhTemplate[iGen, iRule, iSymbol];
		}

		this->knownRuleLengths[iRule] = ruleLengths[iRule];
	}


	this->solution->rules = this->CreateRulesByScanning(ruleLengths, 0, this->generations);
}

List<ProductionRule^>^ PMIProblem::CreateRulesFromSolution(array<Int32>^ Solution)
{
	List<ProductionRule^>^ R = gcnew List<ProductionRule^>;

	array<int>^ order;
	array<int>^ priority;
	Int32 indexGene = 0;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		Symbol^ predecessor;
		List<Symbol^>^ successor = gcnew List<Symbol^>;
		predecessor = this->vocabulary->Symbols[iRule];

		order = gcnew array<int>(this->rulesTemplate[iRule]->successor->Count);
		priority = gcnew array<int>(this->rulesTemplate[iRule]->successor->Count);

		for (size_t i = 0; i < order->Length; i++)
		{
			order[i] = i;
			priority[i] = Solution[indexGene];
			indexGene++;
		}

		Array::Sort(priority, order);

		for (size_t i = 0; i < order->Length; i++)
		{
			successor->Add(this->rulesTemplate[iRule]->successor[order[i]]);
		}

		ProductionRule^ r = gcnew ProductionRule(predecessor, this->vocabulary->YieldsSymbol, successor);
		R->Add(r);
	}

	return R;
}

void PMIProblem::CreateSubProblems_Generations(List<Int32>^ Cutoffs)
{
	// Create sub-problems based on the generation cutoffs
	Int32 start = 0;
	Int32 end = 0;

	for (size_t iProblem = 1; iProblem < Cutoffs->Count; iProblem++)
	{
		start = Cutoffs[iProblem - 1];
		end = Cutoffs[iProblem];

		PMIProblemSet^ S1 = gcnew PMIProblemSet(start, end);

		// Check to see if the modules at this generation produce only constants, if so don't make a module problem
		if (this->numModules[end] > 0)
		{
			PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Modules, PMIProblemDescriptor::PriorityModuleProblem);
			this->ComputeModulesToSolve(P1);
			P1->unset = false;
			S1->moduleProblem = P1;

		}

		// Check to see if the modules at this generation produce no constants, if so don't make a constant problem
		if (this->numModules[start] > 0)
		{
			PMIProblemDescriptor^ P2 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Constants, PMIProblemDescriptor::PriorityConstantProblem);
			PMIProblemDescriptor^ P3 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Order, PMIProblemDescriptor::PriorityOrderProblem);
			this->ComputeModulesToSolve(P2);
			this->ComputeModulesToSolve(P3);
			P2->unset = false;
			P3->unset = false;
			S1->constantProblem = P2;
			S1->orderProblem = P3;
		}

		this->problemSets->Add(S1);
	}
}

void PMIProblem::ErrorAnalysis(array<Int32, 2>^ Rules)
{
	//ConsistencyTable^ C = this->CreateConsistencyTable(Rules);
}

void PMIProblem::ErrorAnalysis(List<ProductionRule^>^ Rules)
{
	//ConsistencyTable^ C = this->CreateConsistencyTable(Rules);
}

void PMIProblem::EstimateGrowthPattern()
{
	this->PoG = gcnew array<Int32, 3>(this->generations - 1, this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->totalPoG = gcnew array<Int32>(this->vocabulary->NumSymbols());
	this->minPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());
	this->maxPoG = gcnew array<Int32, 2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	//for (size_t iGen = 0; iGen <= this->PoG->GetUpperBound(0); iGen++)
	//{
	//	for (size_t iRule = 0; iRule <= this->PoG->GetUpperBound(1); iRule++)
	//	{
	//		for (size_t iSymbol = 0; iSymbol <= this->PoG->GetUpperBound(2); iSymbol++)
	//		{
	//			this->PoG[iGen, iRule, iSymbol] = 0;
	//		}
	//	}
	//}

	for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		array<Int32>^ metaFragmentPV = this->vocabulary->CompressSymbols(metaFragment);
		for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
		{
			this->minPoG[iRule, iSymbol] = metaFragmentPV[iSymbol];
			this->maxPoG[iRule, iSymbol] = 999999;
		}
	}

	this->ComputeGrowthPattern_Unaccounted();
}

bool PMIProblem::EstimateRuleLengths(bool WithLexicon, bool WithInference)
{
	return true;
}

bool PMIProblem::EstimateRuleLengths_Fragments()
{
	return true;
}

bool PMIProblem::EstimateRuleLengths_Growth()
{
	return true;
}

bool PMIProblem::EstimateRuleLengths_Inference(Int32 Start, Int32 End)
{
	this->maxRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengths = gcnew array<Int32>(this->vocabulary->numModules);
	this->maxRuleLengthsModulesOnly = gcnew array<Int32>(this->vocabulary->numModules);
	this->minRuleLengthsModulesOnly = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		this->minRuleLengths[iRule] = Math::Max(this->ruleHead[iRule]->Count, this->ruleTail[iRule]->Count);
	}

	// Find the total minimum and maximum length of all rules
	this->totalMinRuleLength = 0;
	this->totalMaxRuleLength = 99999999;
	this->totalMinRuleLengthModulesOnly = 0;
	this->totalMaxRuleLengthModulesOnly = 99999999;

	for (size_t iGen = Start; iGen <= End - 1; iGen++)
	{
		//if (!this->IsSymbolSubsetChange(iGen,iGen+1))
		//{
		// Get the two generations of evidence
		Word^ e2 = this->evidence[iGen + 1];
		Word^ e1 = this->evidence[iGen];

		// Get the two generations of evidence, modules only
		Word^ eMO2 = this->evidenceModuleOnly[iGen + 1];
		Word^ eMO1 = this->evidenceModuleOnly[iGen];

		// The number of symbols in the generation minus the number of constants in the previous generation
		Int32 min = 0;
		Int32 countMin = e2->symbols->Count - this->vocabulary->CountConstants(e1);
		Int32 idxMin = 0;

		Int32 max = 0;
		Int32 countMax = countMin;
		Int32 idxMax = 0;

		// The number of symbols in the generation minus the number of constants in the previous generation
		Int32 minMO = 0;
		Int32 countMinMO = eMO2->symbols->Count - this->vocabulary->CountConstants(eMO1);
		Int32 idxMinMO = 0;

		Int32 maxMO = 0;
		Int32 countMaxMO = countMinMO;
		Int32 idxMaxMO = 0;

		// Find the most occuring symbol in y
		Int32 highest = 0;
		Int32 lowest = 99999999;
		Int32 highestMO = 0;
		Int32 lowestMO = 99999999;

		if (this->vocabulary->numModules > 1)
		{
			for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
			{
				if (this->symbolSubsets[iGen, iSymbol])
				{
					if (e1->parikhVector[iSymbol] > highest)
					{
						idxMin = iSymbol;
						highest = e1->parikhVector[iSymbol];
					}

					if (e1->parikhVector[iSymbol] < lowest)
					{
						idxMax = iSymbol;
						lowest = e1->parikhVector[iSymbol];
					}

					if (eMO1->parikhVector[iSymbol] > highestMO)
					{
						idxMinMO = iSymbol;
						highestMO = eMO1->parikhVector[iSymbol];
					}

					if (eMO1->parikhVector[iSymbol] < lowestMO)
					{
						idxMaxMO = iSymbol;
						lowestMO = eMO1->parikhVector[iSymbol];
					}
				}
			}

			// Assume every other symbol only produces a single symbol
			// Assume every other symbol produces zero modules (for modules only computation)
			for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
			{
				if (this->symbolSubsets[iGen, iSymbol])
				{
					if (iSymbol != idxMin)
					{
						countMin -= e1->parikhVector[iSymbol];
					}
					else
					{
						min++;
					}

					if (iSymbol != idxMax)
					{
						countMax -= e1->parikhVector[iSymbol];
					}
					else
					{
						max++;
					}

					if (iSymbol != idxMinMO)
					{
						countMinMO -= eMO1->parikhVector[iSymbol];
					}
					else
					{
						minMO++;
					}

					if (iSymbol != idxMaxMO)
					{
						countMaxMO -= eMO1->parikhVector[iSymbol];
					}
					else
					{
						maxMO++;
					}
				}
			}
			//}

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

			if (eMO1->parikhVector[idxMinMO] != 0)
			{
				countMinMO /= eMO1->parikhVector[idxMinMO];
			}

			if (eMO1->parikhVector[idxMaxMO])
			{
				countMaxMO /= eMO1->parikhVector[idxMaxMO];
			}

			minMO += countMinMO;
			maxMO += countMaxMO;

			if (max < this->totalMaxRuleLength)
			{
				this->totalMaxRuleLength = max;
			}

			if ((min > this->totalMinRuleLength) && (min <= this->totalMaxRuleLength))
			{
				this->totalMinRuleLength = min;
			}

			if (maxMO < this->totalMaxRuleLengthModulesOnly)
			{
				this->totalMaxRuleLengthModulesOnly = maxMO;
			}

			if ((minMO > this->totalMinRuleLengthModulesOnly) && (minMO <= this->totalMaxRuleLengthModulesOnly))
			{
				this->totalMinRuleLengthModulesOnly = minMO;
			}
		}
	}

	// Find the maximum rule lengths for each individual rule by summing the proportion of growth values
	if (this->vocabulary->numModules > 1)
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			this->minRuleLengths[iRule] = Math::Max(this->minRuleLengths[iRule], 1);
			this->minRuleLengthsModulesOnly[iRule] = 1;

			Int32 max = 0; // maximum length of the complete rule
			Int32 maxModulesOnly = 0; // maximum length for modules only

			for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
			{
				if (iSymbol < this->vocabulary->numModules)
				{
					max += this->maxPoG[iRule, iSymbol];
					maxModulesOnly += this->maxPoG[iRule, iSymbol];
				}
				else
				{
					max += this->maxPoG[iRule, iSymbol];
				}
			}

			this->maxRuleLengths[iRule] = Math::Min(max, this->totalMaxRuleLength - 1);
			this->maxRuleLengthsModulesOnly[iRule] = Math::Min(maxModulesOnly, this->totalMaxRuleLengthModulesOnly - 1);
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

		this->minRuleLengths[0] = sum;
		this->maxRuleLengths[0] = sum;

		this->minRuleLengthsModulesOnly[0] = this->maxPoG[0, 0];
		this->maxRuleLengthsModulesOnly[0] = this->maxPoG[0, 0];

		this->totalMinRuleLength = this->minRuleLengths[0];
		this->totalMinRuleLengthModulesOnly = this->minRuleLengthsModulesOnly[0];

		this->totalMaxRuleLength = this->maxRuleLengths[0];
		this->totalMaxRuleLengthModulesOnly = this->maxRuleLengthsModulesOnly[0];
	}

	this->OutputRuleLengths();

	// Update Min/Max Lengths based on Lexicon
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if (this->lexicon->symbolSubwords[iRule][0]->symbols->Count > this->minRuleLengths[iRule])
		{
			this->minRuleLengths[iRule] = this->lexicon->symbolSubwords[iRule][0]->symbols->Count;
		}

		if (this->lexicon->symbolSubwords[iRule][this->lexicon->symbolSubwords[iRule]->Count - 1]->symbols->Count < this->maxRuleLengths[iRule])
		{
			this->minRuleLengths[iRule] = this->lexicon->symbolSubwords[iRule][0]->symbols->Count;
		}
	}

	this->OutputRuleLengths();


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
			this->minRuleLengths[iRule] = Math::Max(this->minRuleLengths[iRule], this->totalMinRuleLength - sum);

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
			this->maxRuleLengths[iRule] = Math::Min(this->maxRuleLengths[iRule], this->totalMaxRuleLength - sum);

			if (tmp != this->maxRuleLengths[iRule])
			{
				valueChanged = true;
			}
		}
	} while (valueChanged);

	// Set known rule lengths for any rule where the min equals the max
	for (size_t iRule = 0; iRule <= this->knownRuleLengths->GetUpperBound(0); iRule++)
	{
		if (this->minRuleLengths[iRule] == this->maxRuleLengths[iRule])
		{
			this->knownRuleLengths[iRule] = this->minRuleLengths[iRule];
		}
	}

	this->OutputRuleLengths();

	return true;
}

bool PMIProblem::EstimateRuleLengths_Lexicon()
{
	return true;
}

bool PMIProblem::EstimateRuleLengths_PoG()
{
	return true;
}


void PMIProblem::ExtractVocabulary()
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
	this->vocabularyModuleOnly = gcnew Vocabulary();

	// Add all of the modules
	for each (Symbol^ s in modules)
	{
		this->vocabularyMaster->AddSymbol(s->Value(), true);
		this->vocabularyModuleOnly->AddSymbol(s->Value(), true);
	}
	
	// Add all of the constants
	for each (Symbol^ s in constants)
	{
		this->vocabularyMaster->AddSymbol(s->Value(), false);
	}

	this->vocabularyMaster->IndexModulesStart = 0;
	this->vocabularyMaster->IndexModulesEnd = modules->Count - 1;
	this->vocabularyMaster->IndexConstantsStart = this->vocabularyMaster->IndexModulesEnd + 1;
	this->vocabularyMaster->IndexConstantsEnd = this->vocabularyMaster->IndexConstantsStart + (constants->Count - 1);
	this->vocabularyMaster->IndexSymbolsStart = this->vocabularyMaster->IndexModulesStart;
	this->vocabularyMaster->IndexSymbolsEnd = this->vocabularyMaster->IndexConstantsEnd;
	this->vocabularyMaster->IndexAlpha = this->vocabularyMaster->IndexConstantsEnd + 1;

	this->vocabularyModuleOnly->IndexModulesStart = 0;
	this->vocabularyModuleOnly->IndexModulesEnd = modules->Count - 1;
	this->vocabularyModuleOnly->IndexConstantsStart = this->vocabularyModuleOnly->IndexModulesEnd + 1;
	this->vocabularyModuleOnly->IndexConstantsEnd = this->vocabularyModuleOnly->IndexConstantsStart - 1;
	this->vocabularyModuleOnly->IndexSymbolsStart = this->vocabularyModuleOnly->IndexModulesStart;
	this->vocabularyModuleOnly->IndexSymbolsEnd = this->vocabularyModuleOnly->IndexConstantsEnd;
	this->vocabularyModuleOnly->IndexAlpha = this->vocabularyModuleOnly->IndexConstantsEnd + 1;

	// Re-compute the Parikh vectors based on the problem's vocabulary
	for each (Word^ a in this->evidence)
	{
		this->vocabularyMaster->CompressSymbols(a);
	}

	for each (Word^ a in this->evidenceModuleOnly)
	{
		this->vocabularyModuleOnly->CompressSymbols(a);
	}

	this->vocabulary = this->vocabularyMaster;
}

void PMIProblem::LoadEvidence()
{
	this->evidence = gcnew List<Word^>;
	this->evidenceMaster = gcnew List<Word^>;
	this->evidenceModuleOnly = gcnew List<Word^>;

	for (size_t iGen = this->startGeneration; iGen <= this->endGeneration; iGen++)
	{
		this->evidence->Add(this->model->evidence[iGen]);
		this->evidenceMaster->Add(this->model->evidence[iGen]);
		this->evidenceModuleOnly->Add(this->model->evidenceModuleOnly[iGen]);
	}

	this->generations = this->evidence->Count;
}

HypershapeInt^ PMIProblem::CreateHypershape()
{
	HypershapeInt^ result = gcnew HypershapeInt(0);

	return result;
}

LayeredAntColonyGraph^ PMIProblem::CreateGraph()
{
	LayeredAntColonyGraph^ result = gcnew LayeredAntColonyGraph();

	return result;
}

BruteForceSearchConfiguration^ PMIProblem::CreateSearchSpace()
{
	return nullptr;
}

BruteForceSearchConfiguration^ PMIProblem::CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig)
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



Word^ PMIProblem::FindLongestFragment(Int32 RuleIndex)
{
	Word^ result;

	if (ruleHead[RuleIndex]->Count > ruleTail[RuleIndex]->Count)
	{
		result = ruleHead[RuleIndex];
	}
	else
	{
		result = ruleTail[RuleIndex];
	}

	if (ruleFragments[RuleIndex]->Count > result->Count)
	{
		result = ruleFragments[RuleIndex];
	}

	return result;
}

void PMIProblem::FindSubProblems()
{
	this->problemSets = gcnew List<PMIProblemSet^>;
	List<Int32>^ genCutoffs = gcnew List<Int32>;
	List<Int32>^ tmp;
	tmp = this->FindSubProblems_Symbiology();

	for (size_t i = 0; i < tmp->Count; i++)
	{
		if (!genCutoffs->Contains(tmp[i]))
		{
			genCutoffs->Add(tmp[i]);
		}
	}

	//tmp = this->FindSubProblems_Signature();

	//for (size_t i = 0; i < tmp->Count; i++)
	//{
	//	if (!genCutoffs->Contains(tmp[i]))
	//	{
	//		genCutoffs->Add(tmp[i]);
	//	}
	//}

	this->FindSubProblems_Basic();
	this->CreateSubProblems_Generations(genCutoffs);
}

// This adds a subproblem to find all modules across all generations
// and a subproblem to find all constants across all generations
// This ensures that the problem is fully solved by the time it gets to ordering the symbols
// In some cases, these problems will be 'solved' after all the modules and constants
// are already set in the master rule list, so they won't actually do any processing
void PMIProblem::FindSubProblems_Basic()
{
	//PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(0, this->model->generations, this->model->vocabulary, PMIProblemDescriptor::ProblemType::Modules);
	//this->subProblems->Add(P1);
	//PMIProblemDescriptor^ P2 = gcnew PMIProblemDescriptor(0, this->model->generations, this->model->vocabulary, PMIProblemDescriptor::ProblemType::Constants);
	//this->subProblems->Add(P2);
	//PMIProblemDescriptor^ P3 = gcnew PMIProblemDescriptor(0, this->model->generations-1, this->model->vocabulary, PMIProblemDescriptor::ProblemType::Order, PMIProblemDescriptor::PriorityOrderProblem);
	//this->subProblems->Add(P3);
}


// TODO: Add feature vectors for
// - histogram of neighbouring symbols @ 1, 2, N?
// - growth
// - change in growth
// - change in probability

List<Int32>^ PMIProblem::FindSubProblems_Signature()
{
	List<Int32>^ result = gcnew List<Int32>;

	MatrixMath^ calc = gcnew MatrixMath();
	array<float, 2>^ fv = this->fvCooccurance;
	//array<float, 2>^ fv = calc->Concatenate(h1, h2);
	array<float, 2>^ bounds = gcnew array<float, 2>(fv->GetUpperBound(1)+1, 2);

	for (size_t i = 0; i <= bounds->GetUpperBound(0); i++)
	{
		bounds[i, kMeans::iMin] = 0;
		bounds[i, kMeans::iMax] = 1.0;
	}

	kMeans^ ai1 = gcnew kMeans(2, fv, bounds);
	//ai1->ForgyMethod();
	List<Int32>^ exemplars = gcnew List<Int32>;
	exemplars->Add(0);
	exemplars->Add(4);
	ai1->SetExemplars(exemplars);
	ai1->Solve();

	//fuzzycMeans^ ai2 = gcnew fuzzycMeans(2, fv, bounds);
	//ai2->ForgyMethod();
	//ai2->SetExemplars(exemplars);
	//ai2->Solve();

	return result;
}

//List<Int32>^ PMIProblem::FindSubProblems_Signature()
//{
//	List<Int32>^ result = gcnew List<Int32>;
//
//	for (size_t iGen = 1; iGen < this->evidence->Count; iGen++)
//	{
//		Int32 iSymbol = 0;
//		bool match = true;
//
//		do
//		{
//			match = match && (this->evidence[iGen]->symbols[iSymbol] == this->evidence[iGen - 1]->symbols[iSymbol]);
//			iSymbol++;
//		} while ((match) && (iSymbol < this->evidence[iGen-1]->symbols->Count));
//
//		if (!match)
//		{
//			result->Add(iGen);
//		}
//	}
//
//#if _PMI_PROBLEM_FIND_SUB_PROBLEM_VANISHING_VERBOSE_ >= 1
//	Console::Write("Generation Cutoffs (Signature):");
//
//	for each (Int32 g in result)
//	{
//		Console::Write(" " + g);
//	}
//
//	Console::WriteLine("");
//	Console::WriteLine("Paused");
//	Console::ReadLine();
//#endif
//
//	return result;
//}

List<Int32>^ PMIProblem::FindSubProblems_Symbiology()
{
	// This function creates sub-problems based on the symbiology.

	List<Int32>^ genCutoffs = gcnew List<Int32>;
	genCutoffs->Add(0);
	genCutoffs->Add(this->generations-1);

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
				if ((this->symbolSubsets[iGen, iSymbol] == false) && (this->symbolSubsets[iGen - 1, iSymbol] == true) && (!genCutoffs->Contains(iGen)))
				{
					genCutoffs->Add(iGen);
				}
			}
		}
	}

	genCutoffs->Sort();


#if _PMI_PROBLEM_FIND_SUB_PROBLEM_VANISHING_VERBOSE_ >= 1
	Console::Write("Generation Cutoffs (Symbiology):");

	for each (Int32 g in genCutoffs)
	{
		Console::Write(" " + g);
	}

	//Console::WriteLine("");
	//Console::WriteLine("Paused");
	//Console::ReadLine();
#endif

	return genCutoffs;

}

void PMIProblem::GenerateError()
{
	if (this->errorRate > 0)
	{
		// Assumption: 1st generation never contains an error
		for (size_t iGen = 1; iGen < this->evidence->Count; iGen++)
		{
			for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
			{
				Int32 dieRoll = CommonGlobals::PoolFloat(0, 100);

				if (dieRoll < this->errorRate)
				{
					Int32 idx = CommonGlobals::PoolInt(this->vocabulary->IndexSymbolsStart, this->vocabulary->IndexSymbolsEnd);
					this->evidence[iGen]->symbols[iSymbol] = this->vocabulary->Symbols[idx];
				}
			}
		}

		// Recreate module only evidence if there was some error since some symbols may have changed
		List<Symbol^>^ constants = this->vocabulary->AllConstants();

		// Make a copy of all the evidence while removing the constants
		this->evidenceModuleOnly = gcnew List<Word^>;
		for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
		{
			Word^ a = gcnew Word(this->evidence[iGen]);
			a->FilterOutSymbol(constants);
			this->vocabulary->CompressSymbols(a);
			this->evidenceModuleOnly->Add(a);
		}
	}
}

bool PMIProblem::IsProblemComplete(PMIProblemDescriptor^ P)
{
	bool result = true;

	if (!P->unset)
	{
		Int32 neededGen = 1;
		Int32 missingGen = 0;

		for (size_t iSymbol = 0; iSymbol < this->vocabulary->numModules; iSymbol++)
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
	}

	return result;
}

//bool PMIProblem::IsSetComplete()
//{
//	bool result = true;
//	Int32 index = 0;
//
//	while ((result) && (index < this->currentSet->problems->Count))
//	{
//		result = result && this->IsProblemComplete(this->currentSet->problems[index]);
//		index++;
//	};
//
//	return result;
//}

bool PMIProblem::IsSolved()
{
	bool result = true;
	for (size_t iGen = 0; iGen < this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Gen); iGen++)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				result = result & (this->rulesParikhTemplate[iGen, iRule, iSymbol] != PMIProblem::cUnknownParikhValue);
			}
		}
	}
	return result;
}

bool PMIProblem::IsModuleSolved(Int32 RuleIndex)
{
	bool result = true;

	for (size_t iGen = this->currentProblem->startGeneration; iGen < this->currentProblem->endGeneration; iGen++)
	{
		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol < this->vocabulary->IndexModulesEnd; iSymbol++)
		{
			result = result && (this->rulesParikhTemplate[iGen, RuleIndex, iSymbol] != PMIProblem::cUnknownParikhValue);
		}
	}

	return result;
}

bool PMIProblem::IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex)
{
	bool result = true;

	for (size_t iGen = this->currentProblem->startGeneration; iGen < this->currentProblem->endGeneration; iGen++)
	{
		result = result && (this->rulesParikhTemplate[iGen, RuleIndex, SymbolIndex] != PMIProblem::cUnknownParikhValue);
	}

	return result;
}


bool PMIProblem::IsSolveable()
{
	return true;
}

// Determines if there has been a change in the subset of symbols
bool PMIProblem::IsSymbolAllConstant(Int32 iGen)
{
	Int32 iRule = 0;
	bool noModules = true;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		noModules = noModules && (this->symbolSubsets[iGen, iRule] == false);
	}

	return noModules;
}

// Determines if there has been a change in the subset of symbols
bool PMIProblem::IsSymbolSubsetChange(Int32 Gen1, Int32 Gen2)
{
	Int32 iRule = 0;
	bool match = true;
	bool noModules = true;

	do
	{
		if (this->symbolSubsets[Gen1, iRule] != this->symbolSubsets[Gen2, iRule])
		{
			match = false;
		}

		noModules = noModules && (this->symbolSubsets[Gen2, iRule] == false);

		iRule++;
	} while ((iRule < this->vocabulary->numModules) && (match));

	return !match;
}

void PMIProblem::OrderIncompleteSubProblems()
{

}

bool PMIProblem::LoadFragments()
{
	bool result = false;

	if (System::IO::File::Exists("./" + this->fragmentFileName))
	{
		result = true;
		// Load Configuration File
		System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + this->fragmentFileName);

		array<Char>^ seperator = gcnew array<Char>{','};
		String^ line;

		// Write out the head fragments
		for (size_t iFragment = 0; iFragment <= this->ruleHead->GetUpperBound(0); iFragment++)
		{
			line = sr->ReadLine();
			this->ruleHead[iFragment]->symbols->Clear();
			for (size_t i = 0; i < line->Length; i++)
			{
				String^ symbolValue = line->Substring(i, 1);
				Int32 symbolIndex = this->vocabulary->FindIndex(symbolValue);
				this->ruleHead[iFragment]->Add(this->vocabulary->Symbols[symbolIndex]);
			}
		}

		// Write out marker to indicate tail fragments starting
		line = sr->ReadLine();

		// Write out the tail fragments
		for (size_t iFragment = 0; iFragment <= this->ruleTail->GetUpperBound(0); iFragment++)
		{
			line = sr->ReadLine();
			this->ruleTail[iFragment]->symbols->Clear();
			for (size_t i = 0; i < line->Length; i++)
			{
				String^ symbolValue = line->Substring(i, 1);
				Int32 symbolIndex = this->vocabulary->FindIndex(symbolValue);
				this->ruleTail[iFragment]->Add(this->vocabulary->Symbols[symbolIndex]);
			}
		}

		// Write out marker to indicate general fragments starting
		line = sr->ReadLine();

		// Write out the generation fragments
		for (size_t iFragment = 0; iFragment <= this->ruleFragments->GetUpperBound(0); iFragment++)
		{
			String^ line = sr->ReadLine();
			this->ruleFragments[iFragment]->symbols->Clear();
			for (size_t i = 0; i < line->Length; i++)
			{
				String^ symbolValue = line->Substring(i, 1);
				Int32 symbolIndex = this->vocabulary->FindIndex(symbolValue);
				this->ruleFragments[iFragment]->Add(this->vocabulary->Symbols[symbolIndex]);
			}
		}

		sr->Close();
	}

	return result;
}

void PMIProblem::OutputAnalysis()
{
#if _PMI_PROBLEM_DISPLAY_LEXICON_ > 0
	Console::WriteLine("Lexicon");
	this->lexicon->Display();
	Console::WriteLine();
	Console::WriteLine("Lexicon (Modules Only)");
	this->lexiconModuleOnly->Display();
#endif

#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
	Console::WriteLine("Min/Max PoG");
	Console::Write("  ");
	for (size_t iSymbol = 0; iSymbol < this->vocabulary->Symbols->Count; iSymbol++)
	{
		Console::Write("   " + this->vocabulary->Symbols[iSymbol] + "    ");
	}
	Console::WriteLine();
	for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule]);
		for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
		{
			Console::Write(" " + this->minPoG[iRule, iSymbol].ToString(L"D3") + "/" + this->maxPoG[iRule, iSymbol].ToString(L"D3"));
		}
		Console::Write("  " + this->vocabulary->Symbols[iRule]);
		Console::WriteLine();
	}
	Console::WriteLine();

#if _PMI_PROBLEM_DISPLAY_FRAGMENTS_ >= 1
	// Write out rule fragments
	Console::WriteLine("Rule Heads");
	for (size_t iRule = 0; iRule <= this->ruleHead->GetUpperBound(0); iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule] + ": ");
		for (size_t iSymbol = 0; iSymbol < this->ruleHead[iRule]->Count; iSymbol++)
		{
			Console::Write(this->ruleHead[iRule][iSymbol]->ToString());
		}
		Console::WriteLine();
	}
	Console::WriteLine();

	Console::WriteLine("Rule Tails");
	for (size_t iRule = 0; iRule <= this->ruleTail->GetUpperBound(0); iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule] + ": ");
		for (size_t iSymbol = 0; iSymbol < this->ruleTail[iRule]->Count; iSymbol++)
		{
			Console::Write(this->ruleTail[iRule][iSymbol]->ToString());
		}
		Console::WriteLine();
	}
	Console::WriteLine();
#endif

#if _PMI_PROBLEM_BREAKS_ >= 1
	Console::WriteLine("Press enter to continue");
	Console::ReadLine();
#endif
#endif
}

void PMIProblem::OutputModel()
{
#if _PMI_PROBLEM_CHEAT_ > 0
	Console::WriteLine(" *** CHEAT ENABLED *** ");
	Console::WriteLine(" *** CHEAT ENABLED *** ");
	Console::WriteLine(" *** CHEAT ENABLED *** ");
#endif
		Console::WriteLine("Actual");
		//Console::WriteLine(" Axiom = " + Problem->axiom->ToString());

		Console::WriteLine(" Rules: ");
		for each (ProductionRule^ r in this->model->rules)
		{
			Console::WriteLine("   " + r->ToString());
		}
		Console::WriteLine();

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
		Console::WriteLine("Evidence");

		for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
		{
			Console::WriteLine(this->model->evidence[iGen]->ToString());
		}
		Console::WriteLine();
#endif

		Console::WriteLine("Solution");
		//Console::WriteLine(" Axiom = " + a->ToString());
		Console::WriteLine(" Rules: ");

		for each (ProductionRule^ r in this->solution->rules)
		{
			Console::WriteLine("   " + r->ToString());
		}

		Console::WriteLine();

#if _PMI_PROBLEM_SOLUTION_EVIDENCE_ >= 1
		Console::WriteLine("Evidence");

		Axiom^ a = gcnew Axiom(this->evidence[0]->symbols);

		this->solution->evidence = EvidenceFactory::Process(a, this->solution->rules, 1, this->generations, true);

		for (size_t iGen = 0; iGen < this->solution->evidence->Count; iGen++)
		{
			Console::WriteLine(this->solution->evidence[iGen]->ToString());
		}
		Console::WriteLine();
#endif

#if _PMI_PROBLEM_CHEAT_ > 0
		Console::WriteLine(" *** CHEAT ENABLED *** ");
		Console::WriteLine(" *** CHEAT ENABLED *** ");
		Console::WriteLine(" *** CHEAT ENABLED *** ");
#endif

#if _PMI_PROBLEM_BREAKS_ >= 1
		Console::WriteLine("Press enter to continue");
		Console::ReadLine();
#endif
}

void PMIProblem::OutputRPT()
{
	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule]->ToString());
		for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
		{
			Console::Write("  " + this->vocabularyMaster->Symbols[iSymbol]->ToString());
		}
		Console::WriteLine();

		for (size_t iGen = 0; iGen <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Gen); iGen++)
		{
			Console::Write(iGen + " ");
			for (size_t iSymbol = 0; iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym); iSymbol++)
			{
				if (this->rulesParikhTemplate[iGen, iRule, iSymbol] >= 0)
				{
					Console::Write("+" + this->rulesParikhTemplate[iGen, iRule, iSymbol].ToString(L"D1") + " ");
				}
				else
				{
					Console::Write(this->rulesParikhTemplate[iGen, iRule, iSymbol].ToString(L"D1") + " ");
				}
			}
			Console::WriteLine();
		}
		Console::WriteLine();
	}

#if _PMI_PROBLEM_BREAKS_ >= 1
	Console::WriteLine("Press enter to continue");
	Console::ReadLine();
#endif
}

void PMIProblem::OutputRuleLengths()
{
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ > 0
	Console::WriteLine("Total Min/Max Rule Lengths");
	Console::WriteLine(this->totalMinRuleLength + "/" + this->totalMaxRuleLength);
	Console::WriteLine();

	Console::WriteLine("Min/Max Rule Lengths");
	for (size_t iRule = 0; iRule <= this->minRuleLengths->GetUpperBound(0); iRule++)
	{
		Int32 actualLength = this->ComputeActualLength(iRule);
		Console::WriteLine(this->vocabulary->Symbols[iRule] + " " + this->minRuleLengths[iRule] + "/" + this->maxRuleLengths[iRule] + " : " + actualLength);
	}
	Console::WriteLine();
#if _PMI_PROBLEM_BREAKS_ >= 1
	Console::WriteLine("Press enter to continue");
	Console::ReadLine();
#endif
#endif
}

void PMIProblem::RollbackPartialSolution()
{
	for (size_t iChange = this->changes->Count - 1; iChange < 0; iChange--)
	{
		this->rulesParikhTemplate[this->changes[iChange]->generationIndex, this->changes[iChange]->ruleIndex, this->changes[iChange]->symbolIndex] = this->changes[iChange]->originalValue;
	}

	this->CheckForSolvedGenerations(this->currentProblem->startGeneration, this->currentProblem->endGeneration);
}

void PMIProblem::RollbackPartialSolution_ParikhConstants(array<Int32, 2>^ Rules)
{
	for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
		{
			// Pretty sure this needs to be a subset of the constants
			for (size_t iSymbol = this->vocabulary->IndexConstantsStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
			{
				this->rulesParikhTemplate[iGen, iRule, iSymbol] = PMIProblem::cUnknownParikhValue;
			}
		}
	}
}

void PMIProblem::RollbackPartialSolution_ParikhModules(array<Int32, 2>^ Rules)
{

	for (size_t iGen = this->currentProblem->startGeneration; iGen <= this->currentProblem->endGeneration; iGen++)
	{
		for (size_t iRule = 0; iRule <= Rules->GetUpperBound(0); iRule++)
		{
			for (size_t iSymbol = 0; iSymbol <= Rules->GetUpperBound(1); iSymbol++)
			{
				this->rulesParikhTemplate[iGen, iRule, iSymbol] = PMIProblem::cUnknownParikhValue;
			}
		}
	}
}

bool PMIProblem::RPT_IsConstantSolved(Int32 StartGeneration, Int32 EndGeneration, Int32 ConstantIndex)
{
	bool result = true;
	Int32 iRule = 0;
	Int32 iGen = StartGeneration;
	do
	{
		iRule = 0;
		do
		{
			result = (this->rulesParikhTemplate[iGen, iRule, ConstantIndex] != PMIProblem::cUnknownParikhValue);
			iRule++;
		} while ((result) && (iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule)));
		iGen++;
	} while ((result) && (iGen < EndGeneration));

	return result;
}

void PMIProblem::SaveFragments()
{
	// Load Configuration File
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("./" + this->fragmentFileName, false);

	// Write out the head fragments
	for (size_t iFragment = 0; iFragment <= this->ruleHead->GetUpperBound(0); iFragment++)
	{
		sw->WriteLine(this->ruleHead[iFragment]->ToString());
	}

	// Write out marker to indicate tail fragments starting
	sw->WriteLine("TAIL");

	// Write out the tail fragments
	for (size_t iFragment = 0; iFragment <= this->ruleTail->GetUpperBound(0); iFragment++)
	{
		sw->WriteLine(this->ruleTail[iFragment]->ToString());
	}

	// Write out marker to indicate general fragments starting
	sw->WriteLine("GENERAL");

	// Write out the generation fragments
	for (size_t iFragment = 0; iFragment <= this->ruleFragments->GetUpperBound(0); iFragment++)
	{
		sw->WriteLine(this->ruleFragments[iFragment]->ToString());
	}

	sw->Close();
}


void PMIProblem::Solve()
{
	bool done;
	bool change;
	List<Int32>^ toRemove = gcnew List<Int32>;
	Int32 attempts = 1; // TODO: would like to make this reset whenever a new unique solution is found

	auto startTime = Time::now();

	do
	{
		Int32 priority = 0;
		bool found = false;
		bool anySetSolved = false;

		this->UpdateSolvedProblems();
		this->UpdateCompleteProblems();

		// Loop through every problem set
		for (size_t iSet = 0; iSet < this->problemSets->Count; iSet++)
		{
			this->currentSet = this->problemSets[iSet];

			if (this->currentSet->IsSolved())
			{
				Console::WriteLine("Problem set #" + iSet + " is solved.");
			}
			else if (!this->currentSet->IsComplete())
			{
				Console::WriteLine("Problem set #" + iSet + " is incomplete");
			}
			else
			{
				Console::WriteLine("Problem set #" + iSet + " is complete and unsolved.");
				anySetSolved = anySetSolved || this->SolveProblemSet();
			}
		}
		attempts--;
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

// This function must be overriden by a subclass
bool PMIProblem::SolveProblemSet()
{
	return false;
}

SolutionType^ PMIProblem::SolveProblemBF()
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
	S->fitnessValues[0] = this->Assess(S->solutions[0]);

	for (size_t i = 0; i <= S->solutions[0]->GetUpperBound(0); i++)
	{
		Console::Write(S->solutions[0][i] + ",");
	}
	Console::WriteLine();
	//Console::ReadLine();

	return S;
}

SolutionType^ PMIProblem::SolveProblemGA()
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

	Int32 tmp1 = Math::Max((UInt64)250, Math::Min(this->solutionSpaceSizes[this->constantIndex] * 10, (UInt64)30000));
	Int32 tmp2 = Math::Max((UInt64)250, Math::Min(this->solutionSpaceSizes[this->constantIndex] * 10, (UInt64)250000));

	tmp1 *= this->modifierGenerations;
	tmp2 *= this->modifierGenerations;

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

//bool PMIProblem::SolveLinearAlgebra()
//{
//	return false;
//}

bool PMIProblem::SolveConstantsSearch()
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

bool PMIProblem::SolveFragmentSearch()
{
	return false;
}

bool PMIProblem::SolveModulesSearch()
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

			//Console::WriteLine("(Modules Template) F = " + fitnessValues[iSolution].ToString(L"F5"));
			//for (size_t iRule = 0; iRule <= rules->GetUpperBound(0); iRule++)
			//{
			//	for (size_t iSymbol = 0; iSymbol <= rules->GetUpperBound(1); iSymbol++)
			//	{
			//		Console::Write(rules[iRule, iSymbol] + ",");
			//	}
			//	Console::WriteLine();
			//}

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

// TODO: No longer works 
bool PMIProblem::SolveOrderSearch()
{
//	array<Int32>^ solution;
//	GeneticAlgorithmPMIT^ ai;
//	GeneticAlgorithmConfiguration<Int32>^ configuration;
//	GenomeConfiguration<Int32>^ genomeConfig;
//	GenomeFactoryInt^ factory;
//	float fitness = 0.0;
//
//	// Step 5.2 - Order the model by searching
//
//	genomeConfig = this->CreateGenome();
//	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
//	configuration->populationSizeInit = 20;
//	configuration->populationSizeMax = 20;
//	configuration->crossoverWeight = 0.80;
//	configuration->mutationWeight = 0.05;
//	configuration->preferHighFitness = false;
//
//	ai = gcnew GeneticAlgorithmPMIT(configuration);
//	//ai = gcnew GeneticAlgorithmP<Int32>(configuration);
//	ai->problem = this;
//	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(100000, 2));
//	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0.0));
//	factory = gcnew GenomeFactoryInt(genomeConfig);
//	ai->factory = factory;
//
//	array<array<Int32>^>^ solutions = ai->SolveAllSolutions();
//	array<float>^ fitnessValues = ai->FitnessValues();
//
//	//this->currentProblem->solution = solutions[0];
//	//fitness = fitnessValues[0];
//
//	List<ProductionRule^>^ modelRules = this->CreateRulesByScanning();
//
//#if _PMI_PROBLEM_VERBOSE_ >= 1
//	Console::WriteLine(" Rules: ");
//	for each (ProductionRule^ r in modelRules)
//	{
//		Console::WriteLine("   " + r->ToString());
//	}
//	Console::WriteLine();
//#endif
//
//	fitness = this->AssessProductionRules(modelRules);
//	Console::WriteLine("(model by search) F = " + fitness.ToString(L"F5"));
//
//	if (fitness == 0.0)
//	{
//
//	}
//
//	return fitness  == 0.0;
	return false;
}

bool PMIProblem::SolveOrderScan()
{
	float fitness = 0.0;

	// Step 5 - Prepare the problem to solve symbol ordering
	// Step 5.1 - Order the model by scanning
	List<ProductionRule^>^ modelRules = this->CreateRulesByScanning();

#if _PMI_PROBLEM_VERBOSE_ >= 1
	Console::WriteLine(" Rules: ");
	for each (ProductionRule^ r in modelRules)
	{
		Console::WriteLine("   " + r->ToString());
	}
	Console::WriteLine();
#endif

	fitness = this->AssessProductionRules(modelRules);
	Console::WriteLine("(model by scan) F = " + fitness.ToString(L"F5"));

	if (fitness == 0.0)
	{
		this->CapturePartialSolution_Rules(modelRules);
	}
	else
	{
		this->ErrorAnalysis(modelRules);
	}


	return fitness == 0.0;
}

bool PMIProblem::SolveOrderScanWithVariables()
{
	float fitness = 1.0;

	array<Int32, 3>^ ARPT = gcnew array<Int32, 3>(this->rulesParikhTemplate->GetUpperBound(0) + 1, this->rulesParikhTemplate->GetUpperBound(1) + 1, this->rulesParikhTemplate->GetUpperBound(2) + 1);
	System::Array::Copy(this->rulesParikhTemplate, ARPT, this->rulesParikhTemplate->Length);
	Int32 equationIndex = 0;
	bool newSolution = true;
	List<ProductionRule^>^ modelRules;
	List<Int32>^ solutionIndices = gcnew List<Int32>;

	// get initial solutions for the equations
	for (size_t iEquation = 0; iEquation < this->equations->Count; iEquation++)
	{
		this->equations[iEquation]->FindAllSolutions();
		solutionIndices->Add(0);
	}

	bool success = false;
	Int32 index = 0;
	Console::WriteLine();
	bool increment = false;
	do
	{
		// TODO: This is a really bad solution but I don't care for the moment
		if (increment)
		{
			solutionIndices[index]++;
		}
		else
		{
			increment = true;
		}

		if (solutionIndices[index] >= this->equations[index]->solutions->Count)
		{
			solutionIndices[index] = 0;
			index++;
		}
		else
		{
			index = 0;
			ARPT = this->AdjustRPT(ARPT, solutionIndices);

			// Step 5 - Prepare the problem to solve symbol ordering
			// Step 5.1 - Order the model by scanning
			modelRules = this->CreateRulesByScanning(ARPT);

#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine(" Rules: ");
			for each (ProductionRule^ r in modelRules)
			{
				Console::WriteLine("   " + r->ToString());
			}
			Console::WriteLine();
#endif

			fitness = this->AssessProductionRules(modelRules);
			Console::WriteLine("(model by scan) F = " + fitness.ToString(L"F5"));
		}
	} while (fitness != 0.0);

	if (fitness == 0.0)
	{
		this->CapturePartialSolution_Rules(modelRules);
	}

	return fitness == 0.0;
}

List<Word^>^ PMIProblem::SymbolSubstitution(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End)
{
	array<SymbolPair^>^ swaps = gcnew array<SymbolPair^>(0);
	array<Int32>^ score = gcnew array<Int32>(0);

	for (size_t i = 0; i < this->vocabulary->Symbols->Count; i++)
	{
		for (size_t j = 0; j < this->vocabulary->Symbols->Count; j++)
		{
			Array::Resize(swaps, swaps->Length + 1);
			Array::Resize(score, score->Length + 1);

			swaps[swaps->Length - 1] = gcnew SymbolPair(this->vocabulary->Symbols[i], this->vocabulary->Symbols[j]);
			score[score->Length - 1] = 0;
		}
	}

	// First pass, for each symbol find the symbol it most matches in the evidence
	for (size_t iGen = 0; iGen < E1->Count; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < std::max(E2[E2_Start + iGen]->symbols->Count, E1[E1_Start + iGen]->symbols->Count); iSymbol++)
		{
			if ((iSymbol < E2[E2_Start + iGen]->symbols->Count) && (iSymbol < E1[E1_Start + iGen]->symbols->Count))
			{
				bool found = false;
				int i = 0;
				do
				{
					if ((swaps[i]->a == E2[E2_Start + iGen]->symbols[iSymbol]) && (swaps[i]->b == E1[E1_Start + iGen]->symbols[iSymbol]))
					{
						score[i]++;
						found = true;
					}
					i++;
				} while (!found);
			}
		}
	}

	// Starting with the symbol with the most matches, each symbol gets the abstraction of the symbol it most matches
	List<Symbol^>^ tabu = gcnew List<Symbol^>;
	List<SymbolPair^>^ substitutions = gcnew List<SymbolPair^>;

	Array::Sort(score, swaps, gcnew ReverseComparer);

	// Choose a set of subtitutions
	for each (Symbol^ s in this->vocabulary->Symbols)
	{
		int i = 0;
		bool found = false;

		do
		{
			// Is this the swap with the highest value for the symbol s?
			if ((swaps[i]->a == s) && (!tabu->Contains(swaps[i]->b)))
			{
				substitutions->Add(swaps[i]); // record that the substitution was made
				tabu->Add(swaps[i]->b);
				found = true;
			}
			i++;
		} while (!found);
	}

	for (size_t i = 0; i < substitutions->Count; i++)
	{
		if (substitutions[i]->a != substitutions[i]->b)
		{
			for (size_t iGen = 0; iGen < this->generations; iGen++)
			{
				for (size_t iSymbol = 0; iSymbol < E1[E1_Start + iGen]->symbols->Count; iSymbol++)
				{
					if (E1[E1_Start + iGen]->symbols[iSymbol] == substitutions[i]->b)
					{
						E1[E1_Start + iGen]->symbols[iSymbol] = substitutions[i]->a;
					}
				}
			}
		}
	}

	return E1;
}

void PMIProblem::UpdateCompleteProblems()
{
	// check all but the last problem set
	for (size_t iSet = 0; iSet < this->problemSets->Count - 1; iSet++)
	{
		this->problemSets[iSet]->moduleProblem->complete = this->IsProblemComplete(this->problemSets[iSet]->moduleProblem);
		this->problemSets[iSet]->constantProblem->complete = this->IsProblemComplete(this->problemSets[iSet]->constantProblem);
		this->problemSets[iSet]->orderProblem->complete = this->IsProblemComplete(this->problemSets[iSet]->orderProblem);

		for (size_t iProblem = 0; iProblem < this->problemSets[iSet]->otherProblems->Count; iProblem++)
		{
			this->problemSets[iSet]->otherProblems[iProblem]->complete = this->IsProblemComplete(this->problemSets[iSet]->otherProblems[iProblem]);
		}
	}

	// Last problem set is always complete
	this->problemSets[this->problemSets->Count - 1]->moduleProblem->complete = true;
	this->problemSets[this->problemSets->Count - 1]->constantProblem->complete = true;
	this->problemSets[this->problemSets->Count - 1]->orderProblem->complete = true;

	for (size_t iProblem = 0; iProblem < this->problemSets[this->problemSets->Count - 1]->otherProblems->Count; iProblem++)
	{
		this->problemSets[this->problemSets->Count - 1]->otherProblems[iProblem]->complete = true;
	}
}

void PMIProblem::UpdateSolvedProblems()
{
	for (size_t iSet = 0; iSet < this->problemSets->Count; iSet++)
	{

		// Check to see if the generations are fully solved
		bool isSolved = true;
		bool isSolvedModulesOnly = true;
		for (size_t iGen = this->problemSets[iSet]->startGeneration; iGen < this->problemSets[iSet]->endGeneration; iGen++)
		{
			isSolved = isSolved && this->generationsSolved[iGen];
			isSolvedModulesOnly = isSolvedModulesOnly && this->generationsSolvedModulesOnly[iGen];
		}

		this->problemSets[iSet]->moduleProblem->solved = isSolvedModulesOnly;
		this->problemSets[iSet]->constantProblem->solved = isSolved;
		this->problemSets[iSet]->orderProblem->solved = isSolved;
		for (size_t iProblem = 0; iProblem < this->problemSets[iSet]->otherProblems->Count; iProblem++)
		{
			this->problemSets[iSet]->otherProblems[iProblem]->solved = isSolved;
		}
	}
}

void PMIProblem::UpdateKnownRuleLengths()
{
	// Set known rule lengths for any rule where the min equals the max
	for (size_t iRule = 0; iRule <= this->knownRuleLengths->GetUpperBound(0); iRule++)
	{
		if (this->minRuleLengths[iRule] == this->maxRuleLengths[iRule])
		{
			this->knownRuleLengths[iRule] = this->minRuleLengths[iRule];
		}
	}
}

bool PMIProblem::UpdateMaxPoG(Int32 iRule, Int32 iSymbol, Int32 V)
{
	bool result = false;

	//if (iRule == 19)
	//{
	//	Console::WriteLine(iRule + "," + iSymbol + " (max pog) = " + this->maxPoG[iRule, iSymbol] + " " + V);
	//}

	if (V < 0)
	{
		Console::WriteLine("PMIProblem::UpdateMaxPoG: V < 0");
		this->maxPoG[iRule, iSymbol] = this->minPoG[iRule, iSymbol];
	}
	else if (V < this->maxPoG[iRule, iSymbol])
	{
		result = true;
		this->maxPoG[iRule, iSymbol] = V;
	}

	return result;
}

bool PMIProblem::UpdateMinPoG(Int32 iRule, Int32 iSymbol, Int32 V)
{
	bool result = false;

	//if (iRule == 19)
	//{
	//	Console::WriteLine(iRule + "," + iSymbol + " (min pog) = " + this->minPoG[iRule, iSymbol] + " " + V);
	//}

	if (V > this->maxPoGMaster[iRule, iSymbol])
	{
		Console::WriteLine(iRule + "," + iSymbol + " changing min PoG from " + this->minPoG[iRule, iSymbol] + " to " + V + " Master max PoG = " + this->maxPoGMaster[iRule, iSymbol]);
		this->flagAnalysisFailed = true;
	}
	else if (V > this->minPoG[iRule, iSymbol])
	{
		result = true;
		this->minPoG[iRule, iSymbol] = V;
	}

	return result;
}

bool PMIProblem::UpdateMinRuleLengths(Int32 iRule, Int32 V)
{
	bool result = false;

	//if (iRule == 19)
	//{
	//	Console::WriteLine(iRule + " (min length) = " + this->minRuleLengths[iRule] + " " + V);
	//}

	if ((V > this->minRuleLengths[iRule]) && (V >= this->absoluteMinRuleLength))
	{
		result = true;
		this->minRuleLengths[iRule] = V;

		if (V > this->maxRuleLengths[iRule])
		{
			this->maxRuleLengths[iRule] = V;
		}
	}

	return result;
}

bool PMIProblem::UpdateMaxRuleLengths(Int32 iRule, Int32 V)
{
	bool result = false;

	//if (iRule == 19)
	//{
	//	Console::WriteLine(iRule + " (max length) = " + this->maxRuleLengths[iRule] + " " + V);
	//}

	if ((V < this->maxRuleLengths[iRule]) && (V >= this->absoluteMinRuleLength) && (V >= this->minRuleLengths[iRule]))
	{
		result = true;
		this->maxRuleLengths[iRule] = V;
	}

	return result;
}

void PMIProblem::UpdatePoGForCompleteFragment(Int32 iRule, Word^ W)
{
	if (!W->compressed)
	{
		this->vocabulary->CompressSymbols(W);
	}

	for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
	{
		this->minPoG[iRule, iSymbol] = W->parikhVector[iSymbol];
		this->maxPoG[iRule, iSymbol] = W->parikhVector[iSymbol];
	}
}

bool PMIProblem::UpdateTotalMinRuleLengths(Int32 V)
{
	bool result = false;

	if (V > this->totalMinRuleLength)
	{
		result = true;
		this->totalMinRuleLength = V;
	}

	return result;
}

bool PMIProblem::UpdateTotalMaxRuleLengths(Int32 V)
{
	bool result = false;

	if (V < this->totalMaxRuleLength)
	{
		result = true;
		this->totalMaxRuleLength = V;
	}

	return result;
}

void PMIProblem::UpdateRPT(Int32 iGen, Int32 iRule, Int32 iSymbol, Int32 V)
{
	if (V < 0)
	{
		this->flagAnalysisFailed = true;
	}
	else
	{
		this->rulesParikhTemplate[iGen, iRule, iSymbol] = V;
	}

}

void PMIProblem::UpdateRPT(Int32 iRule, array<Int32>^ PV)
{
	this->CaptureParikhVector(iRule, PV);
}

void PMIProblem::UpdateRPT(List<ProductionRule^>^ Rules)
{
	// If the fitness is zero, it is possible that the model rules are correct by the RPT is wrong
	// E.g. if the RPT says 5 x A and 1 x B and the real answer is 3 x A and 3 x B
	// It will have found the right model because the lengths are the same
	// So convert the rules into Parkih vectors and save back to the RPT
	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		Int32 index = this->vocabulary->RuleIndex(Rules[iRule]->predecessor);
		array<Int32>^ pv = this->vocabulary->CompressSymbols(Rules[iRule]->successor);
		Int32 length = this->vocabulary->ParikhVectorLength(pv);
		this->CaptureParikhVector(index, pv);
		this->knownRuleLengths[index] = length;
	}
}

bool PMIProblem::VerifyOrderScan_Constants(array<Int32, 2>^ Rules)
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

	Int32 iGen = this->currentProblem->startGeneration;
	List<Symbol^>^ constants = this->vocabulary->AllConstants();
	constants->Remove(this->vocabulary->Symbols[this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex]);
	List<Word^>^ filteredEvidence = gcnew List<Word^>;

	// Make a copy of all the evidence while removing the constants
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		a->FilterOutSymbol(constants);
		filteredEvidence->Add(a);
	}

	do
	{
		idxScan = 0;
		iSymbol = 0;

		do
		{
			// Step 1 - Find a module
			//if (this->model->evidence[iGen]->symbols[iSymbol]->isModule)
			if (filteredEvidence[iGen]->symbols[iSymbol]->isModule)
			{
				//Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidence[iGen]->symbols[iSymbol]);
				Int32 ruleIndex = this->vocabulary->RuleIndex(filteredEvidence[iGen]->symbols[iSymbol]);

				array<Int32>^ pv = gcnew array<Int32>(Rules->GetUpperBound(1)+1);
				Int32 ruleLength = 0;
				for (size_t iValue = 0; iValue <= Rules->GetUpperBound(1); iValue++)
				{
					pv[iValue] = Rules[ruleIndex, iValue];
					ruleLength += Rules[ruleIndex, iValue];
				}

				// Step 2 - Scan the evidence and ensure that any order is possible
				while ((ruleLength > 0) && (result))
				{
					//if (idxScan < this->model->evidence[iGen + 1]->symbols->Count)
					if (idxScan < filteredEvidence[iGen + 1]->symbols->Count)
					{
						//Int32 symbolIndex = this->vocabulary->FindIndex(this->model->evidence[iGen + 1]->symbols[idxScan]);
						Int32 symbolIndex = this->vocabulary->FindIndex(filteredEvidence[iGen + 1]->symbols[idxScan]);

						//Console::WriteLine(idxScan + ": " + iSymbol + " " + filteredEvidence[iGen + 1]->symbols[idxScan]);

						//if (this->model->evidence[iGen + 1]->symbols[idxScan]->isModule)
						if (filteredEvidence[iGen + 1]->symbols[idxScan]->isModule)
						{
							pv[symbolIndex]--;
							ruleLength--;
							result = result && (pv[symbolIndex] >= 0);
						}
						else if (symbolIndex == this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex)
						{
							pv[pv->GetUpperBound(0)]--;
							ruleLength--;
							result = result && (pv[pv->GetUpperBound(0)] >= 0);
						}

						idxScan++;
					}
					else
					{
						result = false;
					}
				}

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
		
	} while ((result) && (iGen < this->currentProblem->endGeneration));

	//Console::WriteLine("Verification (Constants): " + result);
	return result;
}

bool PMIProblem::VerifyOrderScan_Modules(array<Int32, 2>^ Rules)
{
	bool result = true;

#ifdef _PMI_PROBLEM_VERBOSE >= 3
	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
#endif

	Int32 idxScan;

	Int32 iGen = this->currentProblem->startGeneration;
	//List<Symbol^>^ constants = this->vocabulary->AllConstants();
	//List<Axiom^>^ filteredEvidence = gcnew List<Axiom^>;

	//// Make a copy of all the evidence while removing the constants
	//for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	//{
	//	Axiom^ a = gcnew Axiom(this->evidence[iGen]);
	//	a->FilterOutSymbol(constants);
	//	filteredEvidence->Add(a);
	//}

	do
	{
		idxScan = 0;
		Int32 iSymbol = 0;

		do
		{
			// Step 1 - Find a module
			//if (this->model->evidence[iGen]->symbols[iSymbol]->isModule)
			if (this->model->evidenceModuleOnly[iGen]->symbols[iSymbol]->isModule)
			{
				//Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidence[iGen]->symbols[iSymbol]);
				Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidenceModuleOnly[iGen]->symbols[iSymbol]);

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
					//if (idxScan < this->model->evidence[iGen + 1]->symbols->Count)
					if (idxScan < this->model->evidenceModuleOnly[iGen + 1]->symbols->Count)
					{
						//Console::WriteLine(idxScan + " " + filteredEvidence[iGen + 1]->symbols[idxScan] + " " + pv[0]);

						//if (this->model->evidence[iGen + 1]->symbols[idxScan]->isModule)
						if (this->model->evidenceModuleOnly[iGen + 1]->symbols[idxScan]->isModule)
						{
							//Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidence[iGen + 1]->symbols[idxScan]);
							Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidenceModuleOnly[iGen + 1]->symbols[idxScan]);
							pv[ruleIndex]--;
							ruleLength--;
							result = result && (pv[ruleIndex] >= 0);
						}

						idxScan++;
					}
					else
					{
						result = false;
					}
				}
			}
			else
			{
				idxScan++;
			}

			iSymbol++;
			//} while ((result) && (iSymbol < this->model->evidence[iGen]->symbols->Count));
		} while ((result) && (iSymbol < this->model->evidenceModuleOnly[iGen]->symbols->Count));

		iGen++;
	} while ((result) && (iGen < this->currentProblem->endGeneration));

	//Console::WriteLine("Verification (Modules): " + result);
	return result;
}

void PMIProblem::WriteAnalysis()
{
	// Output Evidence
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(this->pathResult + "\\evidence.csv");

	for (size_t iGen = 0; iGen < this->evidence->Count - 1; iGen++)
	{
		sw->Write(iGen + ",");
		for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
		{
			sw->Write(this->evidence[iGen]->symbols[iSymbol]->ToString() + ",");
		}
		sw->WriteLine();
	}
	sw->Close();

	// Output Growth Model
	sw = gcnew System::IO::StreamWriter(this->pathResult + "\\maxoccurance.csv");

	for (size_t iGen = 0; iGen < this->evidence->Count - 1; iGen++)
	{
		// Get the two generations of evidence
		Word^ x = this->evidence[iGen + 1];
		Word^ y = this->evidence[iGen];

		// For each symbol figure out the maximum growth by symbol
		for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
		{
			sw->Write(this->vocabulary->Symbols[iRule]->ToString() + ",");
			for (size_t iSymbol = this->vocabularyMaster->IndexModulesStart; iSymbol <= this->vocabularyMaster->IndexConstantsEnd; iSymbol++)
			{
				if ((y->parikhVector[iRule] > 0) && (this->symbolSubsets[iGen, iRule]))
				{
					if (iSymbol < this->vocabulary->numModules)
					{
						sw->Write((int)Math::Floor((x->parikhVector[iSymbol]) / y->parikhVector[iRule]) + ",");
					}
					else
					{
						sw->Write((int)Math::Floor((x->parikhVector[iSymbol] - y->parikhVector[iSymbol]) / y->parikhVector[iRule]) + ",");
					}
				}
			}
		}
		sw->WriteLine();
	}
	sw->Close();

	// Output Histograms
	//sw = gcnew System::IO::StreamWriter(this->pathResult + "\\histogram.csv");

	array<Int32>^ h = gcnew array<Int32>(this->vocabulary->numModules * this->vocabulary->numModules);

	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
		{

		}
	}
	// Output RPT
}

//float PMIProblem::AssessOLD(array<Int32>^ Solution)
//{
//	float result = 0.0;
//	//result->fitness = 0.0;
//	//result->sample = Solution;
//	Int32 index = 0;
//	Int32 local = 0;
//
//	// Generate an axiom
//	Axiom^ a = gcnew Axiom();
//	List<Axiom^>^ solutionAxioms = gcnew List<Axiom^>(0);
//
//	if (!this->axiomKnown)
//	{
//		Int32 axiomConstantsCount = 0;
//		Int32 axiomModulesCount = 0;
//
//		while (local < this->MaxAxiomLength)
//		{
//			if ((axiomConstantsCount < this->axiomConstants) && (axiomModulesCount < this->axiomModules))
//			{
//				// Decode as any symbol
//				a->Add(this->vocabulary->Abstracts[Solution[index]]);
//
//				if ((Solution[index] >= this->vocabulary->IndexModulesStart) && (Solution[index] <= this->vocabulary->IndexModulesEnd))
//				{
//					axiomModulesCount++;
//				}
//				else
//				{
//					axiomConstantsCount++;
//				}
//			}
//			else if (axiomConstantsCount == this->axiomConstants)
//			{
//				// Decode as Module, no more constants permitted!
//				if ((Solution[index] >= this->vocabulary->IndexModulesStart) && (Solution[index] <= this->vocabulary->IndexModulesEnd))
//				{
//					a->Add(this->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp -= this->vocabulary->IndexConstantsStart;
//					} while (tmp > this->vocabulary->IndexModulesEnd);
//
//					a->Add(this->vocabulary->Abstracts[tmp]);
//				}
//
//				axiomModulesCount++;
//			}
//			else
//			{
//				if ((Solution[index] >= this->vocabulary->IndexConstantsStart) && (Solution[index] <= this->vocabulary->IndexConstantsEnd))
//				{
//					a->Add(this->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp += this->vocabulary->IndexConstantsStart;
//					} while (tmp < this->vocabulary->IndexModulesStart);
//
//					if (tmp > this->vocabulary->IndexConstantsEnd)
//					{
//						tmp -= this->vocabulary->IndexModulesEnd;
//					}
//
//					a->Add(this->vocabulary->Abstracts[tmp]);
//				}
//				axiomConstantsCount++;
//			}
//
//			index++;
//			local++;
//		}
//		index = this->MaxAxiomLength;
//	}
//	else
//	{
//		while (local < this->MaxAxiomLength)
//		{
//			a->Add(this->axiom->symbols[local]);
//			local++;
//		}
//	}
//
//	// Generate production rules
//	List<ProductionRule^>^ R = gcnew List<ProductionRule^>(0);
//
//	for (size_t iRule = 0; iRule < this->numRules; iRule++)
//	{
//		Symbol^ operation = this->vocabulary->YieldsSymbol;
//
//		List<Symbol^>^ predecessor = gcnew List<Symbol^>(0);
//		//predecessor->Add(this->vocabulary->Symbols[iRule]);
//		predecessor->Add(this->vocabulary->Abstracts[iRule]);
//
//		local = 0;
//		List<Symbol^>^ successor = gcnew List<Symbol^>(0);
//		bool inBranch = false;
//		while (local < this->sizeSuccessor[iRule])
//		{
//			Int32 successorConstantsCount = 0;
//			Int32 successorModulesCount = 0;
//
//			if ((successorConstantsCount < this->successorConstants[iRule]) && (successorModulesCount < this->successorModules[iRule]))
//			{
//				// Decode as any symbol
//				successor->Add(this->vocabulary->Abstracts[Solution[index]]);
//
//				if ((Solution[index] >= this->vocabulary->IndexModulesStart) && (Solution[index] <= this->vocabulary->IndexModulesEnd))
//				{
//					successorModulesCount++;
//				}
//				else
//				{
//					successorConstantsCount++;
//				}
//			}
//			else if (successorConstantsCount == this->successorConstants[iRule])
//			{
//				// Decode as Module, no more constants permitted!
//				if ((Solution[index] >= this->vocabulary->IndexModulesStart) && (Solution[index] <= this->vocabulary->IndexModulesEnd))
//				{
//					successor->Add(this->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp -= this->vocabulary->IndexConstantsStart;
//					} while (tmp > this->vocabulary->IndexModulesEnd);
//
//					successor->Add(this->vocabulary->Abstracts[tmp]);
//				}
//
//				successorModulesCount++;
//			}
//			else
//			{
//				if ((Solution[index] >= this->vocabulary->IndexConstantsStart) && (Solution[index] <= this->vocabulary->IndexConstantsEnd))
//				{
//					successor->Add(this->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp += this->vocabulary->IndexConstantsStart;
//					} while (tmp < this->vocabulary->IndexModulesStart);
//
//					if (tmp > this->vocabulary->IndexConstantsEnd)
//					{
//						tmp -= this->vocabulary->IndexModulesEnd;
//					}
//
//					successor->Add(this->vocabulary->Abstracts[tmp]);
//				}
//
//				successorConstantsCount++;
//			}
//
//			index++;
//			local++;
//
//			//successor->Add(this->vocabulary->Abstracts[Solution[index]]);
//			//index++;
//			//local++;
//		}
//
//		index += (this->sizeSuccessor[iRule] - local);
//
//		ProductionRule^ R1 = gcnew ProductionRule(predecessor, operation, successor);
//
//		R->Add(R1);
//	}
//
//#if _PMI_PROBLEM_VERBOSE_ >= 1
//	Console::WriteLine("Candidate");
//	Console::WriteLine(" Axiom = " + a->ToString());
//	Console::WriteLine(" Rules: ");
//	for each (ProductionRule^ r in R)
//	{
//		Console::WriteLine("   " + r->ToString());
//		//Console::WriteLine("   " + r->ToStringAbstract());
//	}
//
//	Console::WriteLine();
//#endif
//
//	// Doing production for N generations
//	int errors = 0;
//	float errorValue = 0.0;
//	int total = 0;
//	bool productionFailed = false;
//
//	for (size_t iGen = 0; iGen < this->generations; iGen++)
//	{
//		List<Symbol^>^ x = gcnew List<Symbol^>;
//
//		for each (Symbol^ s in a->symbols)
//		{
//			int index = 0;
//			bool found = false;
//
//			do
//			{
//				found = R[index]->IsMatch(s);
//				if (!found)
//				{
//					index++;
//				}
//			} while ((index < R->Count) && (!found));
//
//			if (found)
//			{
//				x->AddRange(R[index]->Produce(s));
//			}
//			else
//			{
//				x->Add(s);
//			}
//		}
//
//		a->symbols = x;
//
//		if (this->fitnessOption == FitnessOption::Positional)
//		{
//			total += std::max(this->evidence[iGen]->symbols->Count, a->symbols->Count);
//
//			// Check results versus evidence
//			for (size_t iSymbol = 0; iSymbol < std::max(this->evidence[iGen]->symbols->Count, a->symbols->Count); iSymbol++)
//			{
//				if ((iSymbol < this->evidence[iGen]->symbols->Count) && (iSymbol < a->symbols->Count))
//				{
//					if (a->symbols[iSymbol] != this->evidence[iGen]->symbols[iSymbol])
//					{
//						errorValue += 1 / (1 + iGen);
//						errors++; // symbols do not match
//					}
//				}
//				else
//				{
//					errorValue += 1 / (1 + iGen);
//					errors++; // result is too long or not long enough
//				}
//			}
//		}
//		else
//		{
//			solutionAxioms->Add(gcnew Axiom(a->symbols));
//		}
//	}
//
//	if (this->fitnessOption == FitnessOption::Pattern)
//	{
//		array<SymbolPair^>^ swaps = gcnew array<SymbolPair^>(0);
//		array<Int32>^ score = gcnew array<Int32>(0);
//
//		for (size_t i = 0; i < this->vocabulary->Symbols->Count; i++)
//		{
//			for (size_t j = 0; j < this->vocabulary->Abstracts->Count; j++)
//			{
//				Array::Resize(swaps, swaps->Length + 1);
//				Array::Resize(score, score->Length + 1);
//
//				swaps[swaps->Length - 1] = gcnew SymbolPair(this->vocabulary->Symbols[i], this->vocabulary->Abstracts[j]);
//				score[score->Length - 1] = 0;
//			}
//		}
//
//		// First pass, for each symbol find the symbol it most matches in the evidence
//		for (size_t iGen = 0; iGen < this->generations; iGen++)
//		{
//			for (size_t iSymbol = 0; iSymbol < std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count); iSymbol++)
//			{
//				if ((iSymbol < this->evidence[iGen]->symbols->Count) && (iSymbol < solutionAxioms[iGen]->symbols->Count))
//				{
//					bool found = false;
//					int i = 0;
//					do
//					{
//						if ((swaps[i]->a == this->evidence[iGen]->symbols[iSymbol]) && (swaps[i]->b == solutionAxioms[iGen]->symbols[iSymbol]))
//						{
//							score[i]++;
//							found = true;
//						}
//						i++;
//					} while (!found);
//				}
//			}
//		}
//		
//		// Starting with the symbol with the most matches, each symbol gets the abstraction of the symbol it most matches
//		List<Symbol^>^ tabu = gcnew List<Symbol^>;
//		List<SymbolPair^>^ substitutions = gcnew List<SymbolPair^>;
//
//		Array::Sort(score, swaps, gcnew ReverseComparer);
//
//		// Choose a set of subtitutions
//		for each (Symbol^ s in this->vocabulary->Symbols)
//		{
//			int i = 0;
//			bool found = false;
//
//			do
//			{
//				// Is this the swap with the highest value for the symbol s?
//				if ((swaps[i]->a == s) && (!tabu->Contains(swaps[i]->b)))
//				{
//					substitutions->Add(swaps[i]); // record that the substitution was made
//					tabu->Add(swaps[i]->b);
//					found = true;
//				}
//				i++;
//			} while (!found);
//		}
//
//		for (size_t i = 0; i < substitutions->Count; i++)
//		{
//			for (size_t iGen = 0; iGen < this->generations; iGen++)
//			{
//				for (size_t iSymbol = 0; iSymbol < solutionAxioms[iGen]->symbols->Count; iSymbol++)
//				{
//					if (solutionAxioms[iGen]->symbols[iSymbol] == substitutions[i]->b)
//					{
//						solutionAxioms[iGen]->symbols[iSymbol] = substitutions[i]->a;
//					}
//				}
//			}
//		}
//
//
//		// Second pass, count errors
//		for (size_t iGen = 0; iGen < this->generations; iGen++)
//		{
//			total += std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count);
//
//			// Check results versus evidence
//			for (size_t iSymbol = 0; iSymbol < std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count); iSymbol++)
//			{
//				if ((iSymbol < this->evidence[iGen]->symbols->Count) && (iSymbol < solutionAxioms[iGen]->symbols->Count))
//				{
//					if (solutionAxioms[iGen]->symbols[iSymbol] != this->evidence[iGen]->symbols[iSymbol])
//					{
//						errorValue += (float) 1 / (1 + iGen);
//						errors++; // symbols do not match
//					}
//				}
//				else
//				{
//					errorValue += (float) 1 / (1 + iGen);
//					errors++; // result is too long or not long enough
//				}
//			}
//		}
//	}
//
//	result = (float)errorValue / total;
//
//#if _PMI_PROBLEM_VERBOSE_ >= 2
//
//	for (size_t i = 0; i < this->generations; i++)
//	{
//		Console::WriteLine("Evidence #" + i.ToString(L"G") + " : " + this->evidence[i]->ToString());
//		Console::WriteLine("Solution #" + i.ToString(L"G") + " : " + solutionAxioms[i]->ToString());
//		
//	}
//
//	Console::WriteLine("F = " + result.ToString(L"F5"));
//	Console::ReadLine();
//#endif
//
//	return result;
//}
//
//if (this->fitnessOption == FitnessOption::Pattern)
//{
//	array<SymbolPair^>^ swaps = gcnew array<SymbolPair^>(0);
//	array<Int32>^ score = gcnew array<Int32>(0);
//
//	for (size_t i = 0; i < this->vocabulary->Symbols->Count; i++)
//	{
//		for (size_t j = 0; j < this->vocabulary->Abstracts->Count; j++)
//		{
//			Array::Resize(swaps, swaps->Length + 1);
//			Array::Resize(score, score->Length + 1);
//
//			swaps[swaps->Length - 1] = gcnew SymbolPair(this->vocabulary->Symbols[i], this->vocabulary->Abstracts[j]);
//			score[score->Length - 1] = 0;
//		}
//	}
//
//	 First pass, for each symbol find the symbol it most matches in the evidence
//	for (size_t iGen = 1; iGen < this->generations; iGen++)
//	{
//		for (size_t iSymbol = 0; iSymbol < std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count); iSymbol++)
//		for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
//		{
//			if ((iSymbol < this->evidence[iGen]->symbols->Count) && (iSymbol < solutionAxioms[iGen]->symbols->Count))
//			{
//				bool found = false;
//				int i = 0;
//				do
//				{
//					if ((swaps[i]->a == this->evidence[iGen]->symbols[iSymbol]) && (swaps[i]->b == solutionAxioms[iGen]->symbols[iSymbol]))
//					{
//						score[i]++;
//						found = true;
//					}
//					i++;
//				} while (!found);
//			}
//		}
//	}
//
//	 Starting with the symbol with the most matches, each symbol gets the abstraction of the symbol it most matches
//	List<Symbol^>^ tabu = gcnew List<Symbol^>;
//	List<SymbolPair^>^ substitutions = gcnew List<SymbolPair^>;
//
//	Array::Sort(score, swaps, gcnew ReverseComparer);
//
//	 Choose a set of subtitutions
//	for each (Symbol^ s in this->vocabulary->Symbols)
//	{
//		int i = 0;
//		bool found = false;
//
//		do
//		{
//			 Is this the swap with the highest value for the symbol s?
//			if ((swaps[i]->a == s) && (!tabu->Contains(swaps[i]->b)))
//			{
//				substitutions->Add(swaps[i]); // record that the substitution was made
//				tabu->Add(swaps[i]->b);
//				found = true;
//			}
//			i++;
//		} while (!found);
//	}
//
//	for (size_t i = 0; i < substitutions->Count; i++)
//	{
//		for (size_t iGen = 0; iGen < this->generations; iGen++)
//		{
//			for (size_t iSymbol = 0; iSymbol < solutionAxioms[iGen]->symbols->Count; iSymbol++)
//			{
//				if (solutionAxioms[iGen]->symbols[iSymbol] == substitutions[i]->b)
//				{
//					solutionAxioms[iGen]->symbols[iSymbol] = substitutions[i]->a;
//				}
//			}
//		}
//	}
//
//
//	 Second pass, count errors
//	for (size_t iGen = 0; iGen < this->generations; iGen++)
//	{
//		total += std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count);
//
//		 Check results versus evidence
//		for (size_t iSymbol = 0; iSymbol < std::max(this->evidence[iGen]->symbols->Count, solutionAxioms[iGen]->symbols->Count); iSymbol++)
//		for (size_t iSymbol = 0; iSymbol < this->evidence[iGen]->symbols->Count; iSymbol++)
//		{
//			if ((iSymbol < this->evidence[iGen]->symbols->Count) && (iSymbol < solutionAxioms[iGen]->symbols->Count))
//			{
//				if (solutionAxioms[iGen]->symbols[iSymbol] != this->evidence[iGen]->symbols[iSymbol])
//				{
//					errorValue += (float)1 / (1 + iGen);
//					errors++; // symbols do not match
//				}
//			}
//			else
//			{
//				errorValue += (float)1 / (1 + iGen);
//				errors++; // result is too long or not long enough
//			}
//		}
//	}
//}
