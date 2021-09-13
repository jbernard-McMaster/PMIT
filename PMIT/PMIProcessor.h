#pragma once

#include "PlantModelStochastic.h"

#include "PMIPerformanceMetrics.h"
#include "PMIProblem.h"
#include "PMIProblem_Direct.h"
#include "PMIProblem_Directv2.h"
#include "PMIProblem_Directv3.h"
#include "PMIProblem_ECC.h"
#include "PMIProblem_Lengthv1.h"
#include "PMIProblem_LengthV2.h"
#include "PMIProblem_Math.h"
#include "PMIProblem_PoG.h"
#include "PMIProblem_PoL.h"
#include "PMIRandomModelGenerator.h"

#include "PMITemplateProblem.h"
#include "PMITemplateProblem_Modules.h"
#include "PMITemplateProblem_Constants.h"
#include "PMITemplateProblem_Axiom.h"

#include "PMIException.h"

#include <chrono>
#include <algorithm>
#include <vector>

#include "GeneticAlgorithmPMIT.h"

#define _PMI_PROCESSOR_VERBOSE_ 0

using namespace System;

// TODO: Currently, PMIT does not remember symbols which have already been solved by a earlier sub-problem
// For example, in subproblem #1 of Apple Twig B,C,U,W are solved.
// However during subproblem #2 these are solved again

// TODO: To resolve systems which generate a very large amount of evidence, i.e. Hilbert curve, I can solve by using a faux parameterized evidence with wildcards
// Rather ABC+FBA-+FF-BC this becomes A*****A******* which becomes A5A7

// Solve/Unsolved List
// * Indicates solved
// ? Kind of solve but some issues
// X Unsolved

// * Algae
// ? Apple Twig. Have to cheat a little. Each individual sub-problem solves, but doesn't bring the whole model together.
// X Bush Like. Cannot solve modules for unknown reason.
// * Cantor Dust
// * Dragon Cruve
// * E-Curve
// * Fractal Plant
// * Gosper Curve 1
// * Gosper Curve 2
// X Hilbert - Evidence is too large, runs out of memory
// * Koch
// * Peano
// * Pythagoras Tree
// * Sierpenski Triangle 1
// * Sierpenski Triangle 2

// Step 1 - Preprocess Problem
// 1.1 - Compute evidence Parikh vectors
// 1.2 - Compute growth pattern
// 1.3 - Find sub-problems by analysis
// 1.4 - Find sub-problems by machine learning

// Step 2 - Solve using Linear Algebra
// 2.1 - Solve complete sub-problems for modules
// 2.2 Check incomplete module sub-problems for completeness. Loop to 2.1.
// 2.2.1 If 2.2 solved, capture partial solution
// 2.3 - Solve complete sub-problems for constants (should only be one)
// 2.3.1 If 2.3 solved, capture partial solution
// 2.4 Check incomplete module sub-problems for completeness. Loop to 2.3.

// Step 3 - Solve for the Parikh vector templates.
// 3.1 Solve complete module sub-problems
// 3.1.1 If 3.1 solved, capture partial solution
// 3.2 Check incomplete module sub-problems for completeness. Loop to 3.1.
// 3.3 Solve complete constant sub-problems (should only be one)
// 3.3.1 If 3.3 solved, capture partial solution
// 3.4 Check incomplete constant sub-problems for completeness. Loop to 3.3.

// Step 4 - Add the axiom (constant alpha => Generation 1)

// Step 5 - Solve the symbol order
// 5.1 - Solve for symbol order by scanning
// 5.2 - Solve for symbol order by search

public ref class PMIProcessor
{
public:
	PMIProcessor();
	virtual ~PMIProcessor();

	enum class AlgorithmChoices
	{
		EvidenceGenerator,
		GeneticAlgorithm,
		AntColonySystem,
		Analyzer
	};

	enum class PMITType
	{
		PoG_v1,
		PoG_v1b,
		PoG_v2,
		Length_v1,
		Length_v2,
		PoL_v1,
		PoL_v1b,
		Directv1,
		Directv2,
		Directv3,
		Directv4,
		Mathv1,
		Mathv2,
		Lex_v1,
		Lex_v1b,
		ML,
		PoGL,
		ErrorCorrection,
		Analyzer
	};

	bool output = true;
	AlgorithmChoices algorithmChoice;
	PMITType pmitType;
	String^ problemFile;
	String^ vocabularyFile;
	String^ configFile;
	PlantModel^ model;
	Int32 numExecutions;
	float errorRate;

	PMIProblem^ problem;
	PMITemplateProblem_Modules^ problemModules;
	PMITemplateProblem_Constants^ problemConstants;
	PMITemplateProblem_Axiom^ problemAxiom;

	virtual bool AlgorithmMenu();
	bool AnalysisMenu();
	bool OptionsMenu();

	virtual void Execute(Int32 iExecution);

	bool ProblemMenu();

	bool ProblemMenu_D0L();
	bool ProblemMenu_D0L_LimitedConstants();
	bool ProblemMenu_Stochastic();
	bool ProblemMenu_ChangeOverTime();
	bool ProblemMenu_Contextual();
	bool ProblemMenu_Parameterized();

	void LoadConfiguration();

	enum class ConfigFileLine
	{
		NumExecutions
	};


	//void TestGAO();
	void EvidenceGenerator();
	//Axiom^ ChangeAxiomMenu();
	//List<ProductionRule^>^ ChangeRulesMenu();

	//void Solve();

	//void TestPSOO();
	//void TestPSOP();
	//void TestACOO();
	//void TestACOP();
	//void TestGS();
	//void TestFSSRS();
	//void TestFSSDRS();

	//void OutputModel(PlantModel^ Actual, PlantModel^ Solution);

protected:
	Int32 indexAxiomLength = 1;
	Int32 indexGenerations = 0;

	void PMIProcessor::RecursiveFor(Int32 Level, Int32 Limit, List<Int32>^ Values, Int32 NumSymbols, System::IO::StreamWriter^ sw);

	List<Int32>^ combination;
	List<Int32>^ options;

	Int32 maximumNumSymbols = 4;
	Int32 maximumSymbolOccurance = 4;

	PMIPerformanceMetrics^ metrics;

private:
};