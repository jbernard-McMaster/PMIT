#include "stdafx.h"
#include "EDCProblemV3_SuccessorSearch.h"


EDCProblemV3_SuccessorSearch::EDCProblemV3_SuccessorSearch(ModelV3^ M, ProblemType T, array<List<SuccessorCount^>^>^ Successors) : EDCProblemV3(M,T)
{
	this->successors = Successors;
}

EDCProblemV3_SuccessorSearch::~EDCProblemV3_SuccessorSearch()
{
}

void EDCProblemV3_SuccessorSearch::Initialize()
{

}

float EDCProblemV3_SuccessorSearch::Assess(array<Int32>^ Scenario)
{
	String^ corrected = this->model->evidence[0]->GetSymbols();
	String^ replacement = "";
	Int32 errors = 0;
	Int32 total = 0;
	float fitness = 0.0;

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		Int32 iScan = 0;
		total += this->model->evidence[iGen + 1]->Length();
		for (size_t iPos = 0; iPos < corrected->Length; iPos++)
		{
			String^ symbol = corrected->Substring(iPos, 1);
			Int32 iSymbol = this->model->alphabet->FindIndex(corrected->Substring(iPos, 1));
			String^ successor = this->successors[iSymbol][Scenario[iSymbol]]->successor;
			String^ tmp = this->model->evidence[iGen + 1]->GetSymbols(iScan, successor->Length);
			replacement += successor;
			iScan += successor->Length;
			Int32 e = MasterAnalysisObject::ComputeError(successor, tmp);
			errors += e;
			Console::WriteLine("For symbol " + symbol + " successor " + successor + " to production " + tmp + " has " + e + " errors (" + errors + ")");
		}
		Console::WriteLine((iGen + 1) + ": Corrected evidence " + replacement + " (" + replacement->Length + ")");
		corrected = replacement;
		replacement = "";
	}

	fitness = (float)errors / total;

	return fitness;
}

GenomeConfiguration<Int32>^ EDCProblemV3_SuccessorSearch::CreateGenome()
{
	GenomeConfiguration<Int32>^ result;

	Int32 numGenes = this->successors->GetUpperBound(0)+1;

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t iRule = 0; iRule <= this->successors->GetUpperBound(0); iRule++)
	{
		min[iRule] = 0;
		max[iRule] = this->successors[iRule]->Count - 1;
	}

	result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

BruteForceSearchConfiguration^ EDCProblemV3_SuccessorSearch::CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig)
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

List<ProductionRuleV3^>^ EDCProblemV3_SuccessorSearch::SolveForRules()
{
	SolutionType^ s = this->SolveProblemBF();

	List<ProductionRuleV3^>^ result = gcnew List<ProductionRuleV3^>;

	for (size_t iRule = 0; iRule <= this->successors->GetUpperBound(0); iRule++)
	{
		ProductionRuleV3^ r = gcnew ProductionRuleV3();
		r->predecessor = this->model->alphabet->GetSymbol(iRule);
		r->contextLeft->Add(this->model->alphabet->GetSymbol(-1));
		r->contextRight->Add(this->model->alphabet->GetSymbol(-1));
		r->condition->Add("*");
		r->odds->Add(100);
		r->parameters = this->model->parameters;
		r->successor->Add(this->successors[iRule][s->solutions[0][iRule]]->successor);
		result->Add(r);
	}

	result = this->SortProductionRules(result);

	return result;
}

SolutionType^ EDCProblemV3_SuccessorSearch::SolveProblemBF()
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
	//S->solutions = ai->SolveAllSolutions();
	S->fitnessValues[0] = this->Assess(S->solutions[0]);

	//for (size_t i = 0; i <= S->solutions[0]->GetUpperBound(0); i++)
	//{
	//	Console::Write(S->solutions[0][i] + ",");
	//}
	//Console::WriteLine();
	//Console::ReadLine();

	return S;
}
