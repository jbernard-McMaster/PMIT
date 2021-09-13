#include "stdafx.h"
#include "LSIProblemV3_Wolfram.h"


LSIProblemV3_Wolfram::LSIProblemV3_Wolfram(ModelV3^ M, ProblemType T) : LSIProblemV3(M,T)
{
}


LSIProblemV3_Wolfram::~LSIProblemV3_Wolfram()
{
}

GenomeConfiguration<Int32>^ LSIProblemV3_Wolfram::CreateGenome()
{
	array<Int32>^ min = gcnew array<Int32>(this->MAO->factsList->Count);
	array<Int32>^ max = gcnew array<Int32>(this->MAO->factsList->Count);
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;

	for (size_t iGene = 0; iGene < min->Length; iGene++)
	{
		min[iGene] = this->MAO->factsList[iGene]->length->min;
		max[iGene] = this->MAO->factsList[iGene]->length->max;
		this->rule2solution->Add(iGene);
		this->solution2rule->Add(iGene);
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(this->MAO->factsList->Count, min, max);
	return result;
}

BruteForceSearchConfiguration^ LSIProblemV3_Wolfram::CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig)
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

void LSIProblemV3_Wolfram::Solve()
{
	auto startTime = Time::now();

	SolutionType^ solution = this->SolveProblemBF();

	auto endTime = Time::now();

	std::chrono::duration<double> duration = endTime - startTime;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

	this->solveTime = ms.count();

	List<List<ProductionRuleV3^>^>^ ruleSets = this->CreateModelFromSolution(solution);

	for (size_t iSet = 0; iSet < ruleSets->Count; iSet++)
	{
		Console::WriteLine("Solution #" + iSet+1);
		List<ProductionRuleV3^>^ rules = ruleSets[iSet];
		for (size_t iRule = 0; iRule < rules->Count; iRule++)
		{
			rules[iRule]->Display();
		}
	}

	Console::WriteLine("MTTS: " + this->analysisTime + this->solveTime + " Analzye Time: " + this->analysisTime + " Search Time: " + this->solveTime);
	System::IO::StreamWriter^ sw;

	if (this->tracker == 1)
	{
		sw = gcnew System::IO::StreamWriter("./wolfram_1symbol.csv", false);
	}
	else
	{
		sw = gcnew System::IO::StreamWriter("./wolfram_1symbol.csv", true);
	}

	sw->WriteLine("Rule," + this->tracker + "," + (this->analysisTime + this->solveTime));

	sw->Close();

	//Console::WriteLine("Paused");
	//Console::ReadLine();
}

SolutionType^ LSIProblemV3_Wolfram::SolveProblemBF()
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
	//S->solutions[0] = ai->SolveAllSolutions();
	S->fitnessValues[0] = this->Assess(S->solutions[0]);

	//for (size_t i = 0; i <= S->solutions[0]->GetUpperBound(0); i++)
	//{
	//	Console::Write(S->solutions[0][i] + ",");
	//}
	//Console::WriteLine();
	//Console::ReadLine();

	return S;
}