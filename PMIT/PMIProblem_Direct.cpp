#include "stdafx.h"
#include "PMIProblem_Direct.h"


PMIProblem_Direct::PMIProblem_Direct(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblem_Search(Model, ErrorRate, FileName)
{
}

PMIProblem_Direct::~PMIProblem_Direct()
{
}

List<ProductionRule^>^ PMIProblem_Direct::Assess_CreateRulesStep_Direct(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;

	Int32 iGene = 0;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;

		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			for (size_t i = 0; i < this->maxPoG[iRule, iSymbol]; i++)
			{
				if (Solution[iGene] < this->vocabulary->NumSymbols())
				{// Non-blank
					s->Add(this->vocabulary->Symbols[Solution[iGene]]);
				}
				iGene++;
			}
		}

		rules->Add(gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s));
	}

	return rules;
}

GenomeConfiguration<Int32>^ PMIProblem_Direct::CreateGenome()
{
	Int32 numGenes = 0;

	for (size_t iRule = 0; iRule <= this->maxPoG->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
		{
			numGenes += this->maxPoG[iRule, iSymbol];
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	// Gene 0..R ... represent the 1st gene in each rule, since every rule must have 1 gene
	// Values: 0..S, where S is the number of possible symbols
	// Gene R..N ... represent the symbols that cannot be blank due to minimum/maximums
	// Values: 0..S, where S is the number of possible symbols
	// Gene N..M represent the symbols that may be blank
	// Values: 0..S+1 where S is the number of possible symbols, value S+1 indicates blank

	// Example: if there is a maximum of 6 symbols (total) and a minimum of 5 symbols and two rules.
	// Genes 1..2 represent the first symbol in each rule
	// Genes 3..5 represent the symbols that cannot be blank
	// Gene 6 represent a possible blank symbol

	Int32 iGene = 0;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		Int32 idx = 0;
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			for (size_t i = 0; i < this->maxPoG[iRule,iSymbol]; i++)
			{
				if (idx == 0)
				{
					min[iGene] = 0;
					max[iGene] = this->vocabulary->NumSymbols()-1; // the first symbol of each rule cannot be blank
				}
				else
				{
					min[iGene] = 0;
					max[iGene] = this->vocabulary->NumSymbols(); // don't need the plus 1 for the blank as this is zero-indexed
				}
				iGene++;
				idx++;
			}
		}
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

void PMIProblem_Direct::CreateSubProblems_Generations(List<Int32>^ Cutoffs)
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
			PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Direct, PMIProblemDescriptor::PriorityWholeRuleProblem);
			S1->moduleProblem = P1;
		}

		this->problemSets->Add(S1);
	}
}

bool PMIProblem_Direct::SolveProblemSet()
{
	do
	{
		// TODO: Add a check here that prevents a search if it has been searched many times already
		// If there are no full solutions go try to find one
		this->currentProblem = this->currentSet->moduleProblem;
		if (this->currentSet->moduleProblem->fullSolutions->Count == 0)
		{
			Int32 tabuCount = this->currentSet->moduleProblem->tabu->Count;
			do
			{
				this->currentProblem->solved = this->SolveDirectSearch();
			} while ((!this->currentProblem->solved) && (this->currentProblem->tabu->Count > tabuCount));
		}

		// There is a full solution to try
		if (this->currentSet->moduleProblem->fullSolutions->Count > 0)
		{
			// Pop the top solution from the list
			PMISolution^ s = this->currentProblem->fullSolutions[0];
			this->currentSet->moduleProblem->fullSolutions->RemoveAt(0);

			// Create a rule set based on the solution
			array<Int32, 2>^ rules = this->Assess_CreateParikhRulesStep_Modules(s->solution);

			// Update the RPT
			this->CapturePartialSolution(rules);
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
	} while ((!this->currentSet->IsSolved()) && (!this->currentSet->disabled));

	return this->currentSet->IsSolved();
}

bool PMIProblem_Direct::SolveDirectSearch()
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
	configuration->survival = SurvivalType::UniqueElite;
	
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
		Console::WriteLine("(Direct) F = " + fitnessValues[iSolution].ToString(L"F5"));
		PMISolution^ s = gcnew PMISolution(solutions[iSolution], fitnessValues[iSolution]);

		if (fitnessValues[iSolution] == 0)
		{
			List<ProductionRule^>^ rules = this->Assess_CreateRulesStep_Direct(s->solution);
			this->CapturePartialSolution_Rules(rules);
			// No verification is needed as the solution is complete and ordered
			this->currentProblem->fullSolutions->Add(s);
			result = true;
			done = true;
		}
		else if (fitnessValues[iSolution] < this->currentProblem->NumGenerations())
		{
			this->currentProblem->partialSolutions->Add(s);
			done = true;
		}

		iSolution++;
	} while ((!done) && (iSolution < solutions->GetUpperBound(0)));

	return result;
}


