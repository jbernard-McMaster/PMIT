#pragma once
#include <chrono>

#include "defines.h"
#include "ModelV3.h"
#include "MasterAnalysisObject.h"
#include "FactCounter.h"
#include "GeneticAlgorithmPMIT.h"

using namespace System;
using namespace System::Collections::Generic;

typedef std::chrono::high_resolution_clock Time;

// Master Problem Solve Steps
// 1. Analyze
// 2. Determine possible L-system types (Phase IV only). Pick the most preferred L-system possible (maybe most likely?).
// 3. Create sub-problems by generation. These problems are guaranteed to be self-contained.
// -- when creating the problem adjust it so that it is agnostic to the source problem, i.e. 'eliminate' unused symbols, treat last generation symbols as terminal, etc.
// 4. Solve each sub-problem
// 5a. If no solution returned by sub-problem, Loop to step 2 until all L-system types have been tried.
// 5b. If solution return, modify the MAO if the sub-problem returns a solution. Re-analyze. Adjust other sub-problems as needed.
// 6. Return best possible L-system (a SL-system will always be found if nothing else)

// Generation Sub-Problem Solve Steps
// 1. Analyze
// 2. Create sub-problem by symbol
// 2a. Create a sub-problem for all producing symbols. Treat drawing symbols as terminals. Does this need filtered evidence?
// 2b. Create a sub-problem for each graphical and terminal symbol in the following order:
// --- [ 
// --- ] no search required, can always be solved by rule, see PMIProblemV2A::Assess_BranchingSymbol
// --- + maybe can be solved by rule now? See new Lexicon
// --- - maybe can be solved by rule now? See new Lexicon
// --- Other terminals
// --- F
// --- f maybe can be solved by rule now? See new Lexicon
// 3. Solve each sub-problem.
// -- As before, adjust each sub-problem such that it is agnostic to the source.
// 4a. If no solution is return for 1st sub-problem, return "no solution" to master problem.
// 4b. If no solution is returned, rollback the MAO.
// 4c. If a solution is returned, re-analyze. Create a commit log of any changes to the MAO.

// Symbol Sub-Problem Solve Steps
// 1. Analyze
// 2. Search for a solution if necessary, i.e. Analyze did not completely solve the problem
// 3. Return a solution if one is found, otherwise return nullptr

// PMIT Types
// PMIT-M
// PMIT-L & PoL
// PMIT-PoG
// PMIT-D

// PMIT-M include only when solving D0L, D1L, D2L, DT0L, DT1L, DT2L and for PMIT-L, PMIT-DPoG.
// PMIT-D v5 (better fragments, improved lexicon)
// PMIT-D and PoG can be combined into hybrid. PMIT-DPoG.
// PMIT-L

public ref struct GenerationRange
{
	Int32 StartGen;
	Int32 EndGen;
};

//public ref struct SolutionType
//{
//	array<array<Int32>^>^ solutions;
//	array<float>^ fitnessValues;
//};

public ref struct MaxFragmentCandidates
{
	List<Int32>^ candidates;
	List<Int32>^ fragmentIndex;
};

public ref class LSIProblemV3 :
	public OptimizationProblem<Int32>
{
public:
	enum class ProblemType {
		Master,
		Primary,
		Secondary
	};

	LSIProblemV3(ModelV3^ M, ProblemType T);
	virtual ~LSIProblemV3();
	virtual void Initialize();

	bool isAmbiguous;
	ModelV3^ model;
	ModelV3^ solution;
	MasterAnalysisObject^ MAO;
	List<GenerationRange^>^ generationSubProblems;
	
	Int32 absoluteMinLength;
	ProblemType problemType;
	bool contextFree;

	// Performance Metrics
	std::size_t analysisTime;
	std::size_t solveTime;
	bool solved;
	bool matched;
	bool firstPass;
	bool firstPassBTF;
	//List<UInt64>^ solutionSpaceSizes;

	virtual void Analyze();

	virtual float Assess(array<Int32>^ RuleLengths) override;
	virtual List<ProductionRuleV3^>^ Assess_CreateRulesStep_RuleLengths(array<Int32>^ Solution) override;
	virtual float AssessProductionRules(List<ProductionRuleV3^>^ Rules) override;

	virtual float CompareEvidence(List<WordXV3^>^ SolutionWords);
	virtual List<ProductionRuleV3^>^ CreateRulesByScanning(array<Int32>^ RuleLengths) override;
	virtual List<List<ProductionRuleV3^>^>^ CreateModelFromSolution(SolutionType^ Solutions);

	virtual List<ProductionRuleV3^>^ SortProductionRules(List<ProductionRuleV3^>^ Rules);


	virtual Fact^ CreateBaseFact(Int32 iSymbol, Int32 iLeft, Int32 iRight);

	// ASSUMPTION MODEL
	List<List<SAC^>^>^ equivalence;

	array<Int32>^ CalculateGrowth(WordV3^ From, WordV3^ To);
	FactCounter^ ComputeFactCount(Int32 iGen, WordXV3^ W);

	// these function compute new fragments
	List<String^>^ ComputeFragments_FragmentAndList(Fragment^ Fr, List<FragmentSigned^>^ L);
	void ComputeFragments_FactBTF();
	void ComputeFragments_Localization(Int32 iGen);
	void ComputeFragments_Markers(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2);
	void ComputeFragments_Overlap();
	void ComputeFragments_PartialSolution(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2);
	void ComputeFragment_Position_HeadOnly(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 iPos2);
	void ComputeFragment_Position_TailOnly(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 iPos2);
	Fragment^ ComputeFragment_Position_BTFOnly_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);
	Fragment^ ComputeFragment_Position_BTFOnly_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);
	void ComputeFragments_Position(Int32 iGen1, WordXV3^ W1, Int32 iGen2, WordXV3^ W2);
	void ComputeFragments_Revise();
	Range^ ComputeFragment_Wake_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);
	Range^ ComputeFragment_Wake_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);

	void ComputeBTF_Cache();
	void ComputeBTF_Cache_Step1();
	void ComputeBTF_Cache_Step2();
	void ComputeBTF_Cache_Step3();
	void ComputeBTF_Cache_Step4();
	void ComputeBTF_Localization(Int32 iGen);
	String^ ComputeBTF_LeftSignature(Int32 iGen, Int32 iPos, WordXV3^ W, String^ Frag);
	String^ ComputeBTF_RightSignature(Int32 iGen, Int32 iPos, WordXV3^ W, String^ Frag);
	Fragment^ ComputeBTF_LocalizationCompatability_LeftToRight(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake);
	Fragment^ ComputeBTF_LocalizationCompatability_RightToLeft(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake);
	Range^ ComputeBTF_Wake_LeftToRight(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);
	Range^ ComputeBTF_Wake_RightToLeft(Int32 iGen, WordXV3^ W1, Int32 iPos1, WordXV3^ W2, Int32 StartWake, Int32 EndWake);

	void ComputeLength_AbsoluteMinMax();
	void ComputeLength_Fragment();
	void ComputeLength_Growth(Int32 iGen); // exists in Phase 2
	void ComputeLength_Lexicon(); // exists in Phase 2
	void ComputeLength_TotalSymbolProduction(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // exists in Phase 2
	void ComputeLength_TotalSymbolProductionOLD(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // exists in Phase 2
	void ComputeLength_TotalLength(Int32 iGen); // exists in Phase 2
	void ComputeLength_Symbiology(); // exists in Phase 2

	void ComputeTotalLength_Length(Int32 iGen); // exists in Phase 2 in one function
	void ComputeTotalLength_TotalSymbolProduction(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // exists in Phase 2 in same function as rule above
	void ComputeTotalLength_Symbiology(Int32 iGen); // exists in Phase 2 in one function

	void ComputeGrowth_AbsoluteMinMax();
	void ComputeGrowth_Fragment();
	void ComputeGrowth_Lexicon(); // exists in Phase 2
	void ComputeGrowth_Symbiology(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // exists in Phase 2 but separated in the code
	void ComputeGrowth_TotalGrowth(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // does not exist in Phase 2
	void ComputeGrowth_UnaccountedGrowth(Int32 iGen, WordXV3^ W1, WordXV3^ W2); // exists in Phase 2

	void ComputeLexicon();

	// TODO:
	// - Oh yeah don't forget to update the signature when you update the BTF
	// - need to code a BTF compression (eliminate redunant BTFs) after they've been all updated
	virtual void ComputeSymbolLexicons();
	void ComputeSymbolLocalization();
	void ComputeSymbolLocalization_Analyze(Int32 iGen);
	void ComputeSymbolLocalization_Analyze_BTFOnly(Int32 iGen);
	MaxFragmentCandidates^ ComputeSymbolLocalization_FindBTFCandidates_LeftToRight(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F);
	MaxFragmentCandidates^ ComputeSymbolLocalization_FindBTFCandidates_RightToLeft(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F);
	MaxFragmentCandidates^ ComputeSymbolLocalization_FindMaxFragmentCandidates_LeftToRight(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F);
	MaxFragmentCandidates^ ComputeSymbolLocalization_FindMaxFragmentCandidates_RightToLeft(Int32 iGen, Int32 StartWake, Int32 EndWake, Fact^ F);
	void ComputeSymbolLocalization_LeftToRightLocalization(Int32 iGen);
	void ComputeSymbolLocalization_LeftToRightLocalization_BTFOnly(Int32 iGen);
	void ComputeSymbolLocalization_RightToLeftLocalization(Int32 iGen);
	void ComputeSymbolLocalization_RightToLeftLocalization_BTFOnly(Int32 iGen);
	List<Int32>^ ComputeSymbolLocalization_MaxFragment_LeftToRight(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake);
	List<Int32>^ ComputeSymbolLocalization_MaxFragment_RightToLeft(Fragment^ Fr, WordXV3^ W, Int32 StartWake, Int32 EndWake);

	float CompareEvidence_Positional(List<WordV3^>^ E1, Int32 E1_Start, Int32 E1_End, List<WordV3^>^ E2, Int32 E2_Start, Int32 E2_End);

	void DisplayParikhVector(array<Int32>^ PV, String^ msg);

	void FindSubProblems();

	void IsAmbiguous();

	String^ Reverse(String^ S){
		array<wchar_t>^ tmp = S->ToCharArray();
		Array::Reverse(tmp);
		return gcnew String(tmp);
	}

	//ValidatedFragmentX^ ReviseFragment_HeadTailFragmentOnly(String^ Frag, Fact^ F);

	void SimplifyModel();

	virtual void Solve();
	void SolveGenerationSubProblem();
	void SolveSymbolSubProblem();

	bool SymbolCountsMatch(array<Int32>^ A, array<Int32>^ B);

	virtual GenomeConfiguration<Int32>^ CreateGenome();
	virtual SolutionType^ SolveProblemGA();

	bool ValidateLeftNeighbourTurtle(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2);
	bool ValidateRightNeighbour(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2, Fragment^ Fr);
	bool ValidateRightNeighbourTurtle(WordXV3^ W1, WordXV3^ W2, Int32 Pos1, Int32 Pos2);

	Int32 tracker;

	static const Int32 cPass_Minimum = 1;
	static const Int32 cPass_ComputeLength_TotalSymbolProduction = 0;
	static const Int32 cPass_ComputeTotalLength_TotalSymbolProduction = 0;
	static const Int32 cPass_ComputeTotalLength_Symbiology = 0;
	static const Int32 cPass_ComputeTotalLength_Length = 0;
	static const Int32 cPass_ComputeLength_TotalLength = 0;
	static const Int32 cPass_ComputeLength_Symbiology = 0;
	static const Int32 cPass_ComputeGrowth_Symbiology = 0;
	static const Int32 cPass_ComputeGrowth_UnaccountedGrowth = 0;
	static const Int32 cPass_ComputeGrowth_TotalGrowth = 0;
	static const Int32 cPass_ComputeLength_Growth = 0;
	static const Int32 cPass_ComputeFragments_Position = 0;
	static const Int32 cPass_ComputeFragments_Cache = 1;
	static const Int32 cPass_ComputeSymbolLocalization = 1;
	static const Int32 cPass_ComputeFragments_Revise = 0;

protected:
	Int32 currentIndex;

	static const Int32 cSmallSolutionSpace = 500;
	bool hasSubProblems;

	List<Int32>^ rule2solution; // used to translate a rule index to a place in the solution
	List<Int32>^ solution2rule; // used to translate a rule index to a place in the solution

};

