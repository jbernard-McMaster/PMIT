#include "stdafx.h"
#include "PMIProblem_PoG.h"


PMIProblem_PoG::PMIProblem_PoG(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblem_Search(Model, ErrorRate, FileName)
{
}


PMIProblem_PoG::~PMIProblem_PoG()
{
}

GenomeConfiguration<Int32>^ PMIProblem_PoG::CreateGenome()
{
	Int32 numGenes = 0;
	// For module sub-problems, need a number of genes equal to 
	// the number of unsolved module -> module pairs.

	// For constant sub-problems, need a number of genes equal to 
	// number of unsolved module -> this constant pairs

	// For ordering sub-problems, need a number of genes equal to all symbols

	if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Modules)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
			{
				// Only consider symbols which:
				// 1 - have not already been solved
				// 2 - Only those symbols which exist within the sub-problem
				//if ((this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, iSymbol] == PMIProblem::cUnknownParikhValue) && (this->symbolSubsets[this->currentProblem->startGeneration, iRule]))
				if ((this->currentProblem->modulesToSolve[iRule]) && (!this->IsModuleSolvedForSymbol(iRule, iSymbol)))
				{
					numGenes += 1;
				}
			}
		}
	}
	else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Constants)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(PMIProblem::cDim_RPT_Rule); iRule++)
		{
			// Only consider symbols which:
			// 1 - have not already been solved
			// 2 - Only those symbols which exist within the sub-problem
			// Assumptions:
			// 1 - This code assumes that the first generation of the symbol subsets describes all of the symbols which must be solved.
			//     Any symbols which appear afterwards must have been already solved.

			//if (this->symbolSubsets[this->currentProblem->startGeneration, iRule])
			//{
				//if (this->rulesParikhTemplate[this->currentProblem->startGeneration, iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex] == PMIProblem::cUnknownParikhValue)
			if ((this->currentProblem->modulesToSolve[iRule]) && (!this->IsModuleSolvedForSymbol(iRule, this->vocabulary->IndexConstantsStart + this->currentProblem->constantIndex)))
			{
				numGenes += 1;
			}
			//}
		}
	}
	else if (currentProblem->solveFor == PMIProblemDescriptor::ProblemType::Order)
	{
		for (size_t iRule = 0; iRule <= this->rulesParikhTemplate->GetUpperBound(0); iRule++)
		{
			for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
			{
				numGenes += 1;
			}
		}
	}

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t i = 0; i < min->Length; i++)
	{
		min[i] = PMIProblem::cGene_Min;
		max[i] = PMIProblem::cGene_Max;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}
