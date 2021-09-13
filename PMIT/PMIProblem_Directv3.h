#pragma once
#include "PMIProblem_Directv2.h"

// 1st solve the modules
// Then using the modules, optionally find rule minimum (this will really help)
// Then solve the complete problem
// Encoding scheme:
// For the 1st search, as either v1 or v2 based on which solution space is smaller
// For the 2nd search:
// # of genes = sum of max rule length (like v2)
// Use priority gene values (0..999), going to need a big value to support the number of options
// Consider the genes as groups representing the rules
// The top M, where M is the number of modules for that rule, give it a module
// The top M+1 to N, where N is the minimum length for that rule, gets a constant.  How to convert priority value to symbol???
// Now consider all the remaining genes, the top X gets a constant where X is the value from the sum of the minimum rule lengths to the total maximum rule length.  Need some ability to have blanks!!!

public ref class PMIProblem_Directv3:
	PMIProblem_Directv2
{
public:
	PMIProblem_Directv3(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_Directv3();

	virtual void AnalyzeRuleFragments();

	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Fragment(array<Int32>^ Solution) override;
	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Modules(array<Int32>^ Solution) override;

	virtual bool ComputeSymbolPools(List<ProductionRule^>^ Rules);

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;

	virtual bool SolveProblemSet() override;

	virtual bool SolveFragmentSearch() override;
	virtual bool SolveModulesSearch() override;

};

