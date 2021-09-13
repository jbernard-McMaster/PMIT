#pragma once

#include "defines.h"
#include "Word.h"
#include "ConsistencyItem.h"
#include "ConsistencyTable.h"
#include "Lexicon.h"
#include "ParikhEquation.h"
#include "PlantModel.h"
#include "PMIProblemDescriptor.h"
#include "PMIProblemSet.h"
#include "PMISolution.h"
#include "ProductionRule.h"
#include "Vocabulary.h"
#include "PMIException.h"
#include "RPTChange.h"
#include "TabuItem.h"
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstdlib>

typedef std::chrono::high_resolution_clock Time;

#include "GeneticAlgorithmPMIT.h"


using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Threading::Tasks;

enum class FitnessOption
{
	Positional,
	Indexed, /* Make the indexed fitness option where rather than scanning the entire tree it examines the location in the evidnece where symbol production 'should' match
	i.e.given evidence ABBA and ABAAAB from A = >AB and B = >A.Thus if it is believed that A = >AB to confirm check the first two symbols of the evidence rather than the entire string.
	This is only a speed boost.*/
	Pattern, // Pattern based fitness only work if you search for the axiom in addition to the rules
	Consistency
};

public ref class LexiconThreadParams
{
public:
	LexiconThreadParams(List<Word^>^ Evidence, Lexicon^ Lexicon, array<Int32>^ RuleLengths)
	{
		this->evidence = Evidence;
		this->lexicon = Lexicon;
		this->ruleLengths = RuleLengths;
	}

	List<Word^>^ evidence;
	Lexicon^ lexicon;
	array<Int32>^ ruleLengths;
};

/*
TODO:

1 - Extend the evidence generator to produce an evidence file
2 -	Can direct encoding be improved by using filtered evidence?

	To Solve contextual L-systems, find the best fitting model. Then do an error analysis. By examining if the errors always occur in a regualr pattern, i.e. it is always an A next to a B when it should be a C next to a B then the context can be found.

	To Solve stochastic models, use the lexicon. Pick a lexicon word for a module and have a series of "backups" to use if they don't match. This could be length based as well.
*/

public ref struct SolutionType
{
	array<array<Int32>^>^ solutions;
	array<float>^ fitnessValues;
};

public ref class PMIProblem :
	public OptimizationProblem<Int32>
{
public:
	PMIProblem(PMIProblem^ Problem);
	PMIProblem(PlantModel^ Model);
	PMIProblem(PlantModel^ Model, float ErrorRate, String^ FileName);
	PMIProblem(PlantModel^ Model, Int32 IndexStart, Int32 IndexEnd);
	virtual ~PMIProblem();
	virtual void Initialize(PlantModel^ Model, Int32 StartGeneration, Int32 EndGeneration);

	PlantModel^ model; // The actual plant model to find. This should NOT be referenced during processing as it would mean peeking across the veil.
	PlantModel^ solution; // The actual plant model to find.
	String^ modelFileName;
	String^ fragmentFileName;
	String^ lexiconFileName;
	String^ lexiconModuleOnlyFileName;

	// A set of smaller problems which will infer the plant model
	List<PMIProblemSet^>^ problemSets;
	PMIProblemSet^ currentSet;
	PMIProblemDescriptor^ currentProblem;
	array<bool, 2>^ symbolSubsets;
	array<bool>^ generationsSolved;
	array<bool>^ generationsSolvedModulesOnly;

	BeliefSystem^ beliefs;
	array<Int32>^ numModules;
	array<Int32>^ numConstants;

	List<RPTChange^>^ changes;
	bool flagAnalysisFailed;


	//Int32 currentPriority;

	Int32 generations;
	Int32 finalGeneration; // special case of when all modules go away and all subsequent generations are constants only

	Int32 mode;

	Vocabulary^ vocabulary; // The local vocabulary for the problem being solved
	Vocabulary^ vocabularyMaster; // The master vocabulary for the overall problem
	Vocabulary^ vocabularyModuleOnly; // The local vocabulary for the problem being solved

	List<Word^>^ evidence;
	List<Word^>^ evidenceMaster;
	List<Word^>^ evidenceModuleOnly;

	Lexicon^ lexicon;
	Lexicon^ lexiconMaster;
	Lexicon^ lexiconModuleOnly;

	array<Word^>^ ruleHead;
	array<Word^>^ ruleTail;
	array<Word^>^ ruleFragments;
	array<Word^>^ ruleFragmentsOriginal;

	float errorRate;

	Int32 numRules;
	Int32 startGeneration;
	Int32 endGeneration;
	Int32 solutionLimiter = 75;
	bool useStandardVocuabularySymbols = false;

	// Properties related to the model template
	List<ProductionRule^>^ rulesTemplate;
	array<Int32, 3>^ rulesParikhTemplate;
	array<Int32, 3>^ rulesParikhTemplateMaster;

	// Templates as Parikh vectors
	array<Int32, 3>^ PoG;
	array<Int32>^ totalPoG;
	array<Int32, 2>^ maxPoG;
	array<Int32, 2>^ minPoG;
	array<Int32>^ growthSymbol;
	array<Int32>^ lastGenerationSymbol; // contains the last generation in produces or is produced
	Int32 totalMinRuleLength;
	Int32 totalMaxRuleLength;
	Int32 totalMinRuleLengthModulesOnly;
	Int32 totalMaxRuleLengthModulesOnly;
	array<Int32>^ maxRuleLengths;
	array<Int32>^ minRuleLengths;
	array<Int32>^ maxRuleLengthsModulesOnly;
	array<Int32>^ minRuleLengthsModulesOnly;
	array<List<Symbol^>^>^ ruleModulesPool;
	array<List<Symbol^>^>^ ruleConstantsPool;

	array<Word^>^ ruleHeadMaster;
	array<Word^>^ ruleTailMaster;
	array<Word^>^ ruleFragmentsMaster;
	array<Int32, 3>^ PoGMaster;
	array<Int32>^ totalPoGMaster;
	array<Int32, 2>^ maxPoGMaster;
	array<Int32, 2>^ minPoGMaster;
	array<Int32>^ maxRuleLengthsMaster;
	array<Int32>^ minRuleLengthsMaster;
	array<Int32, 2>^ symbolLexicon;
	array<Int32, 2>^ symbolLexiconInverted;

	// Feature Vectors
	array<float, 2>^ fvCooccurance;
	array<float, 2>^ fvDeltaCooccurance;
	array<float, 2>^ fvGrowth;
	array<float, 2>^ fvDeltaGrowth;
	array<float, 2>^ fvMaxOccurance;
	array<float, 2>^ fvDeltaMaxOccurance;
	array<float, 2>^ fvHoNS;

	Int32 constantIndex;

	// Performance Metrics
	std::size_t analysisTime;
	std::size_t solveTime;
	bool solved;
	bool matched;
	List<UInt64>^ solutionSpaceSizes;

	List<ParikhEquation^>^ equations;

	FitnessOption fitnessOption;

	List<Int32>^ rule2solution; // used to translate a rule index to a place in the solution
	List<Int32>^ solution2rule; // used to translate a rule index to a place in the solution
	array<Int32>^ knownRuleLengths;

	virtual array<Int32, 3>^ AdjustRPT(array<Int32, 3>^ ARPT, List<Int32>^ SolutionIndex);

	virtual List<Symbol^>^ AssembleFragments(Int32 RuleIndex);
	virtual float Assess(array<Int32>^ Solution) override;
	virtual float Assess_Constants(array<Int32>^ Solution);
	virtual float Assess_Direct(array<Int32>^ Solution);
	virtual float Assess_Direct_ModulesOnly(array<Int32>^ Solution);
	virtual float Assess_Fragment(array<Int32>^ Solution);
	virtual float Assess_Length(array<Int32>^ Solution);
	virtual float Assess_Modules(array<Int32>^ Solution);
	virtual float AssessProductionRules(List<ProductionRule^>^ Rules);
	virtual float AssessProductionRules_ModulesOnly(List<ProductionRule^>^ Rules);
	virtual float AssessProductionRules_Parikh(List<ProductionRule^>^ Rules);

	virtual array<Int32>^ Assess_CreateAxiomStep_Constants(Int32 Size, array<Int32>^ Solution);
	virtual array<Int32>^ Assess_CreateAxiomStep_Modules(Int32 Size, array<Int32>^ Solution);

	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Direct(array<Int32>^ Solution);
	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Fragment(array<Int32>^ Solution);
	virtual List<ProductionRule^>^ Assess_CreateRulesStep_Modules(array<Int32>^ Solution);

	virtual array<Int32, 2>^ Assess_CreateParikhRulesStep_Constants(array<Int32>^ Solution);
	virtual array<Int32, 2>^ Assess_CreateParikhRulesStep_Modules(array<Int32>^ Solution);

	virtual void CapturePartialSolution(array<Int32, 2>^ Rules);
	virtual void CapturePartialSolution_ParikhModules(array<Int32, 2>^ Rules);
	virtual void CapturePartialSolution_ParikhConstants(array<Int32, 2>^ Rules);
	virtual void CapturePartialSolution_Rules(List<ProductionRule^>^ Rules);
	virtual void CaptureParikhVector(Int32 Index, array<Int32>^ P);

	virtual array<Int32>^ Cheat(array<Int32>^ Solution);

	virtual void CheckForSolvedGenerations(Int32 Start, Int32 End);
	virtual bool CheckValidity(array<Int32>^ Solution) override;

	float CompareEvidence_Consistency(List<ProductionRule^>^ Rules, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End);
	float CompareEvidence_Positional(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End);
	float CompareEvidence_PositionalAbort(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End);
	float CompareEvidence_Indexed(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End);

	virtual void CompressSolution();

	virtual Int32 ComputeActualLength(Int32 iRule);
	virtual void ComputeFinalGeneration();
	virtual void ComputeGrowthPattern();
	virtual void ComputeGrowthPattern_Unaccounted();
	virtual void ComputeLexicon();
	virtual void ComputeLexiconInnerProcess(Int32 iGen);
	virtual void ComputeLexiconProcessT(Object^ PO);
	virtual void ComputeLexiconProcess(List<Word^>^ Evidence, Lexicon^ L, array<Int32>^ RuleLengths);
	virtual void ComputeLexiconSymbol();
	virtual void ComputeModulesToSolve(PMIProblemDescriptor^ P);
	virtual void ComputeNeighbouringSymbols();
	virtual void ComputeNeighbouringSymbols_Cooccurance();
	virtual void ComputeNeighbouringSymbols_HoNS();
	virtual void ComputeParikhVectors();
	virtual array<Int32>^ ComputeProduction(Word^ A, Word^ B);
	virtual void ComputeRuleHeadsTails();
	virtual UInt64 ComputeSolutionSpaceSize();
	virtual void ComputeSymbiology();

	virtual ConsistencyTable^ CreateConsistencyTable(array<Int32>^ RuleLengths);
	virtual ConsistencyTable^ CreateConsistencyTable(array<Int32, 2>^ Rules);
	virtual ConsistencyTable^ CreateConsistencyTable(List<ProductionRule^>^ Rules);
	virtual void CreateRuleTemplate();
	virtual void CreateModelFromRPT();
	virtual List<ProductionRule^>^ CreateRulesFromSolution(array<Int32>^ Solution);
	virtual List<ProductionRule^>^ CreateRulesByScanning();
	virtual List<ProductionRule^>^ CreateRulesByScanning(array<Int32, 3>^ ARPT);
	virtual List<ProductionRule^>^ CreateRulesByScanning(array<Int32>^ RuleLengths);
	virtual List<ProductionRule^>^ CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration);
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs);

	virtual GenomeConfiguration<Int32>^ CreateGenome();
	virtual HypershapeInt^ CreateHypershape();
	virtual LayeredAntColonyGraph^ CreateGraph();
	virtual BruteForceSearchConfiguration^ CreateSearchSpace();
	virtual BruteForceSearchConfiguration^ CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig);

	virtual void ErrorAnalysis(array<Int32, 2>^ Rules);
	virtual void ErrorAnalysis(List<ProductionRule^>^ Rules);
	virtual void EstimateGrowthPattern();
	virtual bool EstimateRuleLengths(bool WithLexicon, bool WithInference);
	virtual bool EstimateRuleLengths_Fragments();
	virtual bool EstimateRuleLengths_Growth();
	virtual bool EstimateRuleLengths_Inference(Int32 Start, Int32 End);
	virtual bool EstimateRuleLengths_Lexicon();
	virtual bool EstimateRuleLengths_PoG();
	virtual void ExtractVocabulary();

	virtual Word^ FindLongestFragment(Int32 RuleIndex);
	virtual void FindSubProblems();
	virtual void FindSubProblems_Basic();
	virtual List<Int32>^ FindSubProblems_Signature();
	virtual List<Int32>^ FindSubProblems_Symbiology();

	virtual void GenerateError();

	//virtual bool IsSetComplete();
	virtual bool IsProblemComplete(PMIProblemDescriptor^ P);
	virtual bool IsSolved();
	virtual bool IsModuleSolved(Int32 RuleIndex);
	virtual bool IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex);
	virtual bool IsSolveable();
	virtual bool IsSymbolAllConstant(Int32 iGen);
	virtual bool IsSymbolSubsetChange(Int32 Gen1, Int32 Gen2);

	virtual void OrderIncompleteSubProblems();

	virtual void LoadEvidence();
	virtual bool LoadFragments();

	virtual void OutputAnalysis();
	virtual void OutputModel();
	virtual void OutputRPT();
	virtual void OutputRuleLengths();

	virtual void RollbackPartialSolution();
	virtual void RollbackPartialSolution_ParikhConstants(array<Int32, 2>^ Rules);
	virtual void RollbackPartialSolution_ParikhModules(array<Int32, 2>^ Rules);

	virtual bool RPT_IsConstantSolved(Int32 StartGeneration, Int32 EndGeneration, Int32 ConstantIndex);

	virtual void SaveFragments();

	virtual void Solve();
	virtual bool SolveProblemSet();
	virtual bool SolveConstantsSearch();
	virtual bool SolveFragmentSearch();
	virtual bool SolveModulesSearch();
	virtual bool SolveOrderSearch(); // TODO: No longer works but currently solving by scan anyway so just leave it be

	virtual SolutionType^ SolveProblemBF();
	virtual SolutionType^ SolveProblemGA();

	//virtual bool SolveLinearAlgebra();

	virtual bool SolveOrderScan();
	virtual bool SolveOrderScanWithVariables();

	virtual List<Word^>^ SymbolSubstitution(List<Word^>^ E1, Int32 E1_Start, Int32 E1_End, List<Word^>^ E2, Int32 E2_Start, Int32 E2_End);

	void UpdateCompleteProblems();
	void UpdateSolvedProblems();

	virtual void UpdateKnownRuleLengths();
	bool UpdateMaxPoG(Int32 iRule, Int32 iSymbol, Int32 V);
	bool UpdateMinPoG(Int32 iRule, Int32 iSymbol, Int32 V);
	bool UpdateMaxRuleLengths(Int32 iRule, Int32 V);
	bool UpdateMinRuleLengths(Int32 iRule, Int32 V);
	void UpdatePoGForCompleteFragment(Int32 iRule, Word^ W);
	bool UpdateTotalMinRuleLengths(Int32 V);
	bool UpdateTotalMaxRuleLengths(Int32 V);
	void UpdateRPT(Int32 iGen, Int32 iRule, Int32 iSymbol, Int32 V);
	void UpdateRPT(Int32 iRule, array<Int32>^ PV);
	void UpdateRPT(List<ProductionRule^>^ Rules);

	virtual bool VerifyOrderScan_Constants(array<Int32, 2>^ Rules);
	virtual bool VerifyOrderScan_Modules(array<Int32, 2>^ Rules);

	virtual void WriteAnalysis();

	enum class RandomFileLine
	{
		NumRules,
		AxiomSize,
		AxiomKnown,
		Generations
	};

	static const Int32 cUnknownParikhValue = -1;
	static const Int32 cVariableParikhValue = -2;
	static const Int32 cUnknownRuleLength = -1;
	static const Int32 cDim_RPT_Gen = 0;
	static const Int32 cDim_RPT_Rule = 1;
	static const Int32 cDim_RPT_Sym = 2;
	static const Int32 cDim_SymSub_Gen = 0;
	static const Int32 cDim_SymSub_Sym = 1;
	static const Int32 cGene_Min = 1;
	static const Int32 cGene_Half = 50;
	static const Int32 cGene_Max = 99;
	static const Int32 cGene_Min_Large = 1;
	static const Int32 cGene_Half_Large = 500;
	static const Int32 cGene_Max_Large = 999;
	static const float cGene_Ratio = 999/499;
	static const Int32 cEndOfTime = INT_MAX;
	static const Int32 cMaxFragmentLength = 110;
	static const Int32 cMaxFragmentLengthModuleOnly = 60;

	static String^ PathResult = Environment::CurrentDirectory + "\\Result";

protected:
	Int32 absoluteMinRuleLength  = 1;
	List<String^>^ randomSymbols;
	property String^ pathResult;
	Lexicon^ processL;
	List<Word^>^ processE;
	array<Int32>^ processRuleLengths;
	array<Int32>^ processLowestModuleCount;
	Int32 processMin;
	Int32 processMax;
	Int32 tracker = 0;

	Int32 modifierGenerations = 1;

private:

};

public ref class SymbolPair
{
public:
	SymbolPair(Symbol^ A, Symbol^ B) {
		this->a = A;
		this->b = B;
	}
	virtual ~SymbolPair() {};

	Symbol^ a;
	Symbol^ b;
};

public ref class ReverseComparer : System::Collections::Generic::IComparer<Int32>
{
public:
	virtual int Compare(Int32 a, Int32 b)
	{
		return -1 * a.CompareTo(b);
	}
};
