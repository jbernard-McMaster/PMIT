#include "stdafx.h"
#include "PMIProblem_PoL.h"


PMIProblem_PoL::PMIProblem_PoL(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblem_LengthV1(Model, ErrorRate, FileName)
{
}


PMIProblem_PoL::~PMIProblem_PoL()
{
}

array<Int32>^ PMIProblem_PoL::Assess_CreateRuleLengths(array<Int32>^ Solution)
{
	array<Int32>^ ruleLengths = gcnew array<Int32>(Solution->GetUpperBound(0));

	Int32 t = 0;

	for (size_t i = 0; i < Solution->GetUpperBound(0); i++)
	{
		t += Solution[i];
	}

	for (size_t iRule = 0; iRule < Solution->GetUpperBound(0); iRule++)
	{
		//ruleLengths[iRule] = Math::Min(this->maxRuleLengths[this->solution2rule[iRule]], Math::Max(this->minRuleLengths[this->solution2rule[iRule]], (int)(Solution[Solution->GetUpperBound(0)] * ((float)Solution[iRule] / t))));
		ruleLengths[iRule] = Math::Max(this->minRuleLengths[this->solution2rule[iRule]], (int)(Solution[Solution->GetUpperBound(0)] * ((float)Solution[iRule] / t)));
	}

	return ruleLengths;
}

float PMIProblem_PoL::Assess_Length(array<Int32>^ Solution)
{
	float fitness = 0.0;


	List<ProductionRule^>^ modelRules = this->CreateRulesByScanning(this->Assess_CreateRuleLengths(Solution));

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

		if (fitness == 0)
		{
			fitness = this->AssessProductionRules(modelRules);
		}
	}
	else
	{// invalid solution
		fitness = this->generations + 1;
	}

	return fitness;
}

GenomeConfiguration<Int32>^ PMIProblem_PoL::CreateGenome()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;
	// Need a number of genes equal to the number of rules in the sub-problem

	if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Length)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			this->rule2solution->Add(-1);
			// TODO: should only consider rules which have at least one unknown value
			if (this->symbolSubsets[this->currentProblem->endGeneration - 1, iRule])
			{
				Int32 iSymbol = 0;
				bool found = false;

				do
				{
					if (this->rulesParikhTemplate[this->currentProblem->endGeneration - 1, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
					{
						this->rule2solution[iRule] = numGenes;
						this->solution2rule->Add(iRule);
						numGenes += 1;
						found = true;
					}
					iSymbol++;
				} while ((!found) && (iSymbol <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Sym)));
			}
		}
	}

	numGenes++;

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	min[numGenes-1] = this->totalMinRuleLength;
	max[numGenes-1] = this->totalMaxRuleLength;

	for (size_t iGene = 0; iGene < numGenes-1; iGene++)
	{
		min[iGene] = 1;
		max[iGene] = 99;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

bool PMIProblem_PoL::SolveLengthSearch()
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
	List<ProductionRule^>^ rules = this->CreateRulesByScanning(this->Assess_CreateRuleLengths(solutions[0]));

	fitness = this->AssessProductionRules(rules);
	Console::WriteLine("(PoL) F = " + fitness.ToString(L"F5"));

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
