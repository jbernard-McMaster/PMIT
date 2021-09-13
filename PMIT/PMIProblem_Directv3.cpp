#include "stdafx.h"
#include "PMIProblem_Directv3.h"


PMIProblem_Directv3::PMIProblem_Directv3(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblem_Directv2(Model, ErrorRate, FileName)
{
}


PMIProblem_Directv3::~PMIProblem_Directv3()
{
}

void PMIProblem_Directv3::AnalyzeRuleFragments()
{
	Int32 totalMin = 0;
	for (size_t iRule = 0; iRule <= this->ruleFragments->GetUpperBound(0); iRule++)
	{
		this->minRuleLengths[iRule] = this->ruleFragments[iRule]->Count;
		totalMin += this->ruleFragments[iRule]->Count;
	}

	if (totalMin > this->totalMinRuleLength)
	{
		this->totalMinRuleLength = totalMin;
	}

	for (size_t iOuter = 0; iOuter <= this->maxRuleLengths->GetUpperBound(0); iOuter++)
	{
		// Compute the sum of the minimum except the rule itself
		Int32 min = 0;
		for (size_t iInner = 0; iInner <= this->maxRuleLengths->GetUpperBound(0); iInner++)
		{
			if (iOuter != iInner)
			{
				min = min + this->minRuleLengths[iInner];
			}
		}

		this->maxRuleLengths[iOuter] = this->totalMaxRuleLength - min;
	}

}

List<ProductionRule^>^ PMIProblem_Directv3::Assess_CreateRulesStep_Fragment(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;

	Int32 iGene = this->vocabulary->numModules;

	// Make a set of rules with blank successors
	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;
		rules->Add(gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s));
	}

	for (size_t iGene = this->vocabulary->numModules; iGene <= Solution->GetUpperBound(0); iGene++)
	{
		Int32 iRule = Solution[iGene] / this->vocabulary->numConstants;
		Int32 iSymbol = Solution[iGene] % this->vocabulary->numConstants;

		for (size_t j = this->vocabulary->IndexModulesStart; j <= this->vocabulary->IndexModulesEnd; j++)
		{
			if (Solution[j] == iGene)
			{
				rules[j]->successor->AddRange(this->ruleFragments[j]->symbols);
			}
		}

		if (iRule < this->vocabulary->numModules)
		{
			rules[iRule]->successor->Add(this->vocabulary->Symbols[this->vocabulary->IndexConstantsStart + iSymbol]);
		}
	}

	// Check to see if the fragment should be added at the end
	for (size_t j = this->vocabulary->IndexModulesStart; j <= this->vocabulary->IndexModulesEnd; j++)
	{
		if (Solution[j] == Solution->Length)
		{
			rules[j]->successor->AddRange(this->ruleFragments[j]->symbols);
		}
	}

	return rules;
}

List<ProductionRule^>^ PMIProblem_Directv3::Assess_CreateRulesStep_Modules(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;

	Int32 index = 0;

	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;
		s->Add(this->vocabulary->Symbols[Solution[iRule]]);
		rules->Add(gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s));
		index++;
	}

	for (size_t iValue = index; iValue <= Solution->GetUpperBound(0); iValue++)
	{
		Int32 iRule = Solution[iValue] / this->vocabulary->numModules;
		Int32 iSymbol = Solution[iValue] % this->vocabulary->numModules;

		// if the division causes a value greater than the number of rules treat as a blank or unassigned symbol
		if (iRule < this->vocabulary->numModules)
		{
			rules[iRule]->successor->Add(this->vocabulary->Symbols[iSymbol]);
		}
	}

	return rules;
}

bool PMIProblem_Directv3::ComputeSymbolPools(List<ProductionRule^>^ Rules)
{
	bool failed = false;

#ifdef _PMI_PROBLEM_VERBOSE >= 3
	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
#endif

	Int32 idxScan;
	Int32 iGen = this->currentProblem->startGeneration;
	List<Symbol^>^ completed = gcnew List<Symbol^>;

	do
	{
		idxScan = 0;
		Int32 iSymbol = 0;

		do
		{
			// Step 1 - Find a module
			// Check to see if it is a module, is then begin scanning for symbols which appear in the rule
			if ((this->model->evidence[iGen]->symbols[iSymbol]->isModule) && (!completed->Contains(this->model->evidence[iGen]->symbols[iSymbol])))
			{
				// Step 1 - Scan a build a rule fragment until the last module of the successor is found

				completed->Add(this->model->evidence[iGen]->symbols[iSymbol]);
				Int32 ruleIndex = this->vocabulary->RuleIndex(this->model->evidence[iGen]->symbols[iSymbol]);

				// First find the rule for the module in question
				ProductionRule^ r;
				Int32 iRule = 0;

				do
				{
					r = Rules[iRule];
					iRule++;
				} while (r->predecessor != this->model->evidence[iGen]->symbols[iSymbol]);

				List<Symbol^>^ found = gcnew List<Symbol^>;

				// Advance idxScan to the point of matching the first symbol in the successor
				while ((this->model->evidence[iGen + 1]->symbols[idxScan] != r->successor[0]) && (!failed))
				{
					idxScan++;
					if (idxScan == this->model->evidence[iGen + 1]->symbols->Count)
					{
						failed = true;
					}
				}

				if (!failed)
				{
					found->Add(this->model->evidence[iGen + 1]->symbols[idxScan]);

					Int32 iSuccessorSymbol = 1;
					while ((iSuccessorSymbol < r->successor->Count) && (!failed))
					{
						// Scan until the next module in the successor is found
						// Add all the symbol inbetween to the list of found symbols
						do
						{
							idxScan++;
							if (idxScan < this->model->evidence[iGen + 1]->symbols->Count)
							{
								found->Add(this->model->evidence[iGen + 1]->symbols[idxScan]);
							}
							else
							{
								failed = true;
							}
						} while ((!failed) && (this->model->evidence[iGen + 1]->symbols[idxScan] != r->successor[iSuccessorSymbol]));
						iSuccessorSymbol++;
					}

					// Step 3 - Scan and add to the fragment until one of the following is true
					// if the next symbol is a constant until that constant is found
					// if the next symbol is a module until the first symbol of its successor is found
					if ((!failed) && (iSymbol < this->model->evidence[iGen]->symbols->Count))
					{
						idxScan++;
						if (idxScan < this->model->evidence[iGen + 1]->symbols->Count)
						{
							Symbol^ right = this->model->evidence[iGen]->symbols[iSymbol + 1];

							if (!this->vocabulary->IsModule(right))
							{
								while ((this->model->evidence[iGen + 1]->symbols[idxScan] != right) && (!failed))
								{
									found->Add(this->model->evidence[iGen + 1]->symbols[idxScan]);
									idxScan++;
									if ((idxScan == this->model->evidence[iGen + 1]->symbols->Count) || (this->vocabulary->IsModule(this->model->evidence[iGen + 1]->symbols[idxScan])))
									{
										failed = true;
									}
								}
							}
						}
					}

					// Step 4 - Record the rule fragment
					if (!failed)
					{
						for (size_t iSymbol = 0; iSymbol < found->Count; iSymbol++)
						{
							if (found[iSymbol]->isModule)
							{
								ruleModulesPool[ruleIndex]->Add(found[iSymbol]);
							}
							else {
								ruleConstantsPool[ruleIndex]->Add(found[iSymbol]);
							}
						}
						this->ruleFragments[ruleIndex]->symbols = found;
					}
				}
			}

			iSymbol++;
		} while ((iSymbol < this->model->evidence[iGen]->symbols->Count) && (completed->Count < this->vocabulary->numModules) && (!failed));

		iGen++;
	} while ((iGen < this->currentProblem->endGeneration) && (completed->Count < this->vocabulary->numModules) && (!failed));

	return (!failed);
}

GenomeConfiguration<Int32>^ PMIProblem_Directv3::CreateGenome()
{
	Int32 numGenes = 0;
	Int32 totalFragmentLength = 0;
	Int32 numBlanks = this->totalMaxRuleLength - this->totalMinRuleLength;

	if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::DirectModulesOnly)
	{
		numGenes = this->totalMaxRuleLengthModulesOnly;
	}
	else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Fragment)
	{
		numGenes = this->vocabulary->numModules; // one gene for each modules to show where the fragment will go
		for (size_t iRule = 0; iRule <= this->ruleFragments->GetUpperBound(0); iRule++)
		{
			totalFragmentLength += this->ruleFragments[iRule]->Count;
		}
		numGenes += this->totalMaxRuleLength - totalFragmentLength;
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Gene 0..R ... represent the 1st gene in each rule, since every rule must have 1 gene
	// Values: 0..S, where S is the number of possible symbols
	// TODO: current assumes every symbol is possible, this should only be the symbol under consideration
	// Gene R..N ... represent the symbols that cannot be blank due to minimum/maximums
	// Values: 0..S, where S is the number of possible symbols
	// Gene N..M represent the symbols that may be blank
	// Values: 0..S+1 where S is the number of possible symbols, value S+1 indicates blank

	// Example: if there is a maximum of 6 symbols (total) and a minimum of 5 symbols and two rules.
	// Genes 1..2 represent the first symbol in each rule
	// Genes 3..5 represent the symbols that cannot be blank
	// Gene 6 represent a possible blank symbol

	if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::DirectModulesOnly)
	{
		for (size_t iGene = 0; iGene < min->Length; iGene++)
		{
			// First S genes are for the 1st symbol in each rule
			if (iGene < this->vocabulary->numModules)
			{
				min[iGene] = 0;
				max[iGene] = this->vocabulary->numModules - 1;
			}
			// Up to the minimum represent non-blank symbols
			else if (iGene < this->totalMinRuleLengthModulesOnly)
			{
				min[iGene] = 0;
				max[iGene] = (this->vocabulary->numModules * this->vocabulary->numModules) - 1; // every possibilty of each symbol being in each rule, remember zero indexed
			}
			else
			{
				min[iGene] = 0;
				max[iGene] = (this->vocabulary->numModules * this->vocabulary->numModules); // every possibilty of each symbol being in each rule plus a blank, remember zero indexed
			}
		}
	}
	else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Fragment)
	{
		for (size_t iGene = 0; iGene < min->Length; iGene++)
		{
			if (iGene < this->vocabulary->numModules)
			{
				min[iGene] = this->vocabulary->numModules;
				max[iGene] = min->Length; // represent at what point to insert the fragment based on how many genes processed so far
			}
			else if (iGene - this->vocabulary->numModules >= numBlanks)
			{
				min[iGene] = 0;
				max[iGene] = (this->vocabulary->numModules * this->vocabulary->numConstants) - 1;
			}
			else
			{
				min[iGene] = 0;
				max[iGene] = (this->vocabulary->numModules * this->vocabulary->numConstants);
			}
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

void PMIProblem_Directv3::CreateSubProblems_Generations(List<Int32>^ Cutoffs)
{
	// Create sub-problems based on the generation cutoffs
	Int32 start = 0;
	Int32 end = 0;

	for (size_t iProblem = 1; iProblem < Cutoffs->Count; iProblem++)
	{
		start = Cutoffs[iProblem - 1];
		end = Cutoffs[iProblem];

		PMIProblemSet^ S1 = gcnew PMIProblemSet(start, end);

		if (this->numModules[start] > 0)
		{
			PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::DirectModulesOnly, PMIProblemDescriptor::PriorityModuleProblem);
			PMIProblemDescriptor^ P2 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Fragment, PMIProblemDescriptor::PriorityWholeRuleProblem);
			S1->otherProblems->Add(P1);
			S1->otherProblems->Add(P2);
		}

		this->problemSets->Add(S1);
	}
}

bool PMIProblem_Directv3::SolveProblemSet()
{
	do
	{
		// TODO: Add a check here that prevents a search if it has been searched many times already
		// If there are no full solutions go try to find one
		this->currentProblem = this->currentSet->otherProblems[0];
		this->EstimateRuleLengths_Inference(this->currentProblem->startGeneration, this->currentProblem->endGeneration - 1);
		if (this->currentProblem->fullSolutions->Count == 0)
		{
			Int32 tabuCount = this->currentProblem->tabuSolutions->Count;
			do
			{
				this->currentProblem->solved = this->SolveModulesSearch();
			} while ((!this->currentProblem->solved) && (this->currentProblem->tabuSolutions->Count > tabuCount));
		}

		// There is a full solution to try
		if (this->currentProblem->fullSolutions->Count > 0)
		{
			// Pop the top solution from the list
			PMISolution^ s = this->currentProblem->fullSolutions[0];
			this->currentProblem->fullSolutions->RemoveAt(0);

			// Create a rule set based on the solution
			List<ProductionRule^>^ rules = this->Assess_CreateRulesStep_Modules(s->solution);

			// Calculate the symbol pools per rule
			bool verified = this->ComputeSymbolPools(rules);

			if (verified)
			{
				this->UpdateRPT(rules);

				// Recompute the minimum rule lengths
				this->AnalyzeRuleFragments();

				if (this->vocabulary->numConstants > 0)
				{
					// Then solve the entire rule
					this->currentProblem = this->currentSet->otherProblems[1];
					this->currentProblem->solved = this->SolveFragmentSearch();

					if (!this->currentProblem->solved)
					{
						this->RollbackPartialSolution();
						this->changes->RemoveRange(0, this->changes->Count);

						this->currentSet->otherProblems[0]->tabuSolutions->Add(s->solution);
						this->currentSet->otherProblems[0]->Reset();
						this->currentProblem->Reset();
					}
				}
				else
				{
					this->CapturePartialSolution_Rules(rules);
					this->currentSet->otherProblems[1]->solved = true;
				}
			}
			else
			{
				this->currentProblem->tabuSolutions->Add(s->solution);
				this->currentProblem->Reset();
				// TODO: add solution to tabu list
			}
		}
		else if (this->currentProblem->partialSolutions->Count > 0)
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
	} while ((!this->currentSet->IsSolved()) && (!this->currentSet->disabled));

	return this->currentSet->IsSolved();
}

bool PMIProblem_Directv3::SolveFragmentSearch()
{
	bool result = true;
	float fitness = 0.0;

	array<array<Int32>^>^ solutions;
	GeneticAlgorithmPMIT^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

	genomeConfig = this->CreateGenome();
	configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
	configuration->populationSizeInit = 20;
	configuration->populationSizeMax = 20;
	configuration->crossoverWeight = 0.80;
	configuration->mutationWeight = 0.25;
	configuration->preferHighFitness = false;

	ai = gcnew GeneticAlgorithmPMIT(configuration);
	ai->problem = this;
	ai->terminationConditions->Add(gcnew TerminationCondition_NoImprovementLoose<GeneticAlgorithmState^>(100000, 2));
	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0.0));
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(1000000));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;

	// TODO: change to work with returning multiple solutions
	solutions = ai->SolveAllSolutions();
	List<ProductionRule^>^ rules = this->Assess_CreateRulesStep_Fragment(solutions[0]);
	fitness = this->AssessProductionRules(rules);
	Console::WriteLine("(Fragment) F = " + fitness.ToString(L"F5"));

	if (fitness == 0.0)
	{
		this->CapturePartialSolution_Rules(rules);
		this->CheckForSolvedGenerations(this->currentProblem->startGeneration, this->currentProblem->endGeneration);

		result = true;
	}
	else
	{
		//this->currentProblem->tabu->Add(gcnew TabuItem(0, rules));
		result = false;
	}

	return result;
}

bool PMIProblem_Directv3::SolveModulesSearch()
{
	bool result = false;
	float fitness = 0.0;

	array<Int32>^ solution;
	GeneticAlgorithmPMIT^ ai;
	GeneticAlgorithmConfiguration<Int32>^ configuration;
	GenomeConfiguration<Int32>^ genomeConfig;
	GenomeFactoryInt^ factory;

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
	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(1000000));
	factory = gcnew GenomeFactoryInt(genomeConfig);
	ai->factory = factory;

	array<array<Int32>^>^ solutions = ai->SolveAllSolutions();
	array<float>^ fitnessValues = ai->FitnessValues();

	Int32 iSolution = 0;
	bool done = false;

	do
	{
		// TODO: Rather than storing the raw solution store it as rules
		// TODO: Only store unique rule solutions
		if (fitnessValues[iSolution] == 0)
		{
			Console::WriteLine("(Modules Template) F = " + fitnessValues[iSolution].ToString(L"F5"));
			PMISolution^ s = gcnew PMISolution(solutions[iSolution], fitnessValues[iSolution]);
			this->currentProblem->fullSolutions->Add(s);
		
			this->CheckForSolvedGenerations(this->currentProblem->startGeneration, this->currentProblem->endGeneration);
			result = true;
			done = true;
		}

		iSolution++;
	} while ((!done) && (iSolution < solutions->GetUpperBound(0)));

	return result;
}
