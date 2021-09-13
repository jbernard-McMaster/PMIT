#include "stdafx.h"
#include "PMIProblemV2A_PoL_v1.h"


PMIProblemV2A_PoL_v1::PMIProblemV2A_PoL_v1(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Rule(Model, ErrorRate, FileName)
{
}


PMIProblemV2A_PoL_v1::~PMIProblemV2A_PoL_v1()
{
}

//float PMIProblemV2A_PoL::Assess(array<Int32>^ Solution)
//{
//	if (Solution != nullptr)
//	{
//		return this->AssessRuleLengths(this->Assess_CreateRuleLengths(Solution));
//	}
//	else
//	{
//		return this->AssessRuleLengths(Solution);
//	}
//}

array<Int32>^ PMIProblemV2A_PoL_v1::Assess_CreateRuleLengths(array<Int32>^ Solution)
{
	array<Int32>^ ruleLengths = gcnew array<Int32>(Solution->GetUpperBound(0));
	//array<Int32>^ ruleLengths = gcnew array<Int32>(Solution->GetUpperBound(0)+1);

	Int32 t = 0;

	for (size_t i = 0; i < Solution->GetUpperBound(0); i++)
	{
		t += Solution[i];
	}

	//Int32 lengthRemaining = Solution[Solution->GetUpperBound(0)];

	for (size_t iRule = 0; iRule < Solution->GetUpperBound(0); iRule++)
	{
		//ruleLengths[iRule] = Math::Min(this->maxRuleLengths[this->solution2rule[iRule]], Math::Max(this->minRuleLengths[this->solution2rule[iRule]], (int)(Solution[Solution->GetUpperBound(0)] * ((float)Solution[iRule] / t))));
		float proportion = (float)Solution[iRule] / t;
		float length = Solution[Solution->GetUpperBound(0)] * proportion;
		//float proportion = (float)Solution[iRule] / PMIProblem::cGene_Max;
		//float length = lengthRemaining * proportion;
		Int32 tmp = (int)Math::Round(length);
		//lengthRemaining -= tmp;
		ruleLengths[iRule] = Math::Min(this->maxRuleLengths[this->solution2rule[iRule]], Math::Max(this->minRuleLengths[this->solution2rule[iRule]], tmp));
	}

	//if (lengthRemaining >= 0)
	//{
	//	ruleLengths[Solution->GetUpperBound(0)] = lengthRemaining;
	//}
	//else
	//{
	//	Console::WriteLine("Uhhh");
	//}


	return ruleLengths;
}

// TODO: http://boards.straightdope.com/sdmb/showthread.php?t=823631
/*
#include	<stdio.h>
#include	<stdlib.h>

#define	NUM_URN	7
int	Num_Ball;
int	MinB[] = { 2,2,1,1,1,2,1, };
int	MaxB[] = { 11,3,11,11,35,4,57, };

int	count_them(int urn_no, int num_ball)
{
	int	i, numsol = 0;

	if (urn_no == NUM_URN - 1)
		return num_ball >= MinB[urn_no] && num_ball <= MaxB[urn_no];
	for (i = MinB[urn_no]; i <= MaxB[urn_no] && i <= num_ball; i++)
		numsol += count_them(urn_no + 1, num_ball - i);
	return numsol;
}

int main(int argc, char **argv)
{
	for (Num_Ball = 14; Num_Ball <= 74; Num_Ball++)
		printf("%d --> %d\n", Num_Ball, count_them(0, Num_Ball));
	exit(0);
}
*/

UInt64 PMIProblemV2A_PoL_v1::ComputeSolutionSpaceSize()
{
	UInt64 result = 0;
	Int32 numGenes = 0;
	// Need a number of genes equal to the number of rules in the sub-problem
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
		{
			numGenes += 1;
		}
	}

	numGenes++;

	Int32 N = (this->totalMaxRuleLength - this->totalMinRuleLength);

	for (size_t iGene = 0; iGene < numGenes - 1; iGene++)
	{
		result += 1 + (99 - 1);
	}

	return result;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_PoL_v1::CreateGenome()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;
	Int32 minT;
	Int32 maxT;

	minT = this->totalMinRuleLength;
	maxT = this->totalMaxRuleLength;
	// Need a number of genes equal to the number of rules in the sub-problem

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		this->rule2solution->Add(-1);

		if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
		{
			this->rule2solution[iRule] = numGenes;
			this->solution2rule->Add(iRule);
			numGenes += 1;
		}
		else
		{
			minT -= this->minRuleLengths[iRule];
			maxT -= this->minRuleLengths[iRule];
		}
	}

	numGenes++;

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	min[numGenes - 1] = minT;
	max[numGenes - 1] = maxT;

	for (size_t iGene = 0; iGene < numGenes - 1; iGene++)
	{
		min[iGene] = PMIProblem::cGene_Min;
		max[iGene] = PMIProblem::cGene_Max;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

// TODO: This is the code for PMIT-L
BruteForceSearchConfiguration^ PMIProblemV2A_PoL_v1::CreateSearchSpace()
{
	Int32 numGenes = 0;
	this->rule2solution = gcnew List<Int32>;
	this->solution2rule = gcnew List<Int32>;
	// Need a number of genes equal to the number of rules in the sub-problem

	for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
	{
		this->rule2solution->Add(-1);
		this->solution2rule->Add(-1);

		if ((!this->IsModuleSolved(iRule)) && this->IsModuleInSet(iRule))
		{
			this->rule2solution[iRule] = numGenes;
			this->solution2rule[numGenes] = iRule;
			numGenes += 1;
		}
	}

	//array<Int32>^ min = gcnew array<Int32>(numGenes);
	//array<Int32>^ max = gcnew array<Int32>(numGenes);
	array<DimensionSpecType^>^ searchSpace = gcnew array<DimensionSpecType^>(numGenes);

	for (size_t iRule = 0; iRule < searchSpace->Length; iRule++)
	{
		searchSpace[iRule] = gcnew DimensionSpecType();
		searchSpace[iRule]->min = this->minRuleLengths[iRule];
		searchSpace[iRule]->max = this->maxRuleLengths[iRule];
	}

	BruteForceSearchConfiguration^ result = gcnew BruteForceSearchConfiguration(searchSpace);

	return result;
}
