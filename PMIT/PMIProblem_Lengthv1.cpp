#include "stdafx.h"
#include "PMIProblem_Lengthv1.h"


PMIProblem_LengthV1::PMIProblem_LengthV1(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2(Model, ErrorRate, FileName)
{
}


PMIProblem_LengthV1::~PMIProblem_LengthV1()
{
}

float PMIProblem_LengthV1::Assess_Length(array<Int32>^ Solution)
{
	float fitness = 0.0;

#if _PMI_PROBLEM_CHEAT_ > 0
	Solution = this->Cheat(Solution);
#endif

	List<ProductionRule^>^ modelRules = this->CreateRulesByScanning(Solution);

#if _PMI_PROBLEM_VERBOSE_ >= 5
	Console::WriteLine(" Rules: ");
	for each (ProductionRule^ r in modelRules)
	{
		Console::WriteLine("   " + r->ToString());
	}
	Console::WriteLine();
#endif

	PlantModel^ P = gcnew PlantModel();
	P->rules = modelRules;

	bool isMatch = this->model->IsMatch(P);

	if (modelRules != nullptr)
	{
		fitness = this->AssessProductionRules(modelRules);
	}
	else
	{// invalid solution
		fitness = this->generations+1;
	}

	return fitness;
}

GenomeConfiguration<Int32>^ PMIProblem_LengthV1::CreateGenome()
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
			if (this->symbolSubsets[this->currentProblem->endGeneration-1, iRule])
			{
				Int32 iSymbol = 0;
				bool found = false;

				do
				{
					if (this->rulesParikhTemplate[this->currentProblem->endGeneration-1, iRule, iSymbol] == PMIProblem::cUnknownParikhValue)
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

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t iGene = 0; iGene < min->Length; iGene++)
	{
		Int32 iRule = this->solution2rule[iGene];
		min[iGene] = this->minRuleLengths[iRule];
		max[iGene] = this->maxRuleLengths[iRule];
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

bool PMIProblem_LengthV1::SolveProblemSet()
{
	this->currentProblem = this->currentSet->otherProblems[0];

	if (!this->currentProblem->solved)
	{
		this->currentProblem->solved = this->SolveLengthSearch();
		//this->RollbackPartialSolution();
		//this->changes->RemoveRange(0, this->changes->Count);
		if (!this->currentProblem->solved)
		{
			for (size_t iProblem = 0; iProblem < this->currentSet->otherProblems->Count; iProblem++)
			{
				this->currentSet->otherProblems[iProblem]->Reset();
			}
		}
	}
	
	return this->currentProblem->solved;
}

void PMIProblem_LengthV1::CreateSubProblems_Generations(List<Int32>^ Cutoffs)
{
	Int32 start = 0;
	Int32 end = 0;
	for (size_t iProblem = 1; iProblem < Cutoffs->Count; iProblem++)
	{
		start = Cutoffs[iProblem - 1];
		end = Cutoffs[iProblem];

		// If there are modules at the start of the evidence range then create a length and order problem, otherwise there is nothing to solve
		if (this->numModules[start] > 0)
		{
			PMIProblemSet^ S1 = gcnew PMIProblemSet(start, end);
			PMIProblemDescriptor^ P1 = gcnew PMIProblemDescriptor(start, end, this->vocabularyMaster, PMIProblemDescriptor::ProblemType::Length, PMIProblemDescriptor::PriorityLengthProblem);
			//PMIProblemDescriptor^ P2 = gcnew PMIProblemDescriptor(start, end, this->masterVocabulary, PMIProblemDescriptor::ProblemType::Order, PMIProblemDescriptor::PriorityOrderProblem);
			P1->unset = false;
			S1->otherProblems->Add(P1);
			//S1->problems->Add(P2);
			this->problemSets->Add(S1);
		}
	}
}

bool PMIProblem_LengthV1::SolveLengthSearch()
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
	List<ProductionRule^>^ rules = this->CreateRulesByScanning(solutions[0]);
	fitness = this->AssessProductionRules(rules);
	Console::WriteLine("(Length) F = " + fitness.ToString(L"F5"));

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
