#pragma once
#include "PMIProblemV2.h"
#include "PMIFitnessFunction.h"

public ref struct SymbolFragmentType
{
	Symbol^ s;
	Word^ fragment;
	bool isComplete;
	bool isHead;
};

public ref struct MarkerMapType
{
	Word^ marker;
	Int32 start;
	Int32 end;
};

public ref class PMIProblemV2A :
	public PMIProblemV2
{
public:
	PMIProblemV2A(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A();

	array<Int32, 2>^ maxPoGWIP;
	array<Int32, 2>^ minPoGWIP;
	array<Int32>^ maxRuleLengthsWIP;
	array<Int32>^ minRuleLengthsWIP;
	array<Int32,2>^ knownRuleLengthsIndexed;
	bool produceFragments;
	
	List<TabuItem^>^ tabu;
	List<ProductionRule^>^ rulesSolution;

	virtual void Initialize(PlantModel^ Model, Int32 StartGeneration, Int32 EndGeneration) override;
	virtual void InitializeAnalysisTables();
	virtual void InitializeGrowthPattern() override;

	virtual void AddTabuItem(TabuItem^ T);

	virtual void Analyze();
	virtual void AnalyzeMaxFragments(List<SymbolFragmentType^>^ Fragments);
	virtual void AnalyzeMinFragments(List<SymbolFragmentType^>^ Fragments);
	virtual bool AnalyzeSolution(PMISolution^ s);

	virtual float Assess(array<Int32>^ Solution) override;
	virtual bool Assess_BranchingSymbol();
	virtual array<Int32>^ Assess_CreateAxiomStep(Int32 Size, array<Int32>^ Solution);
	virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution);
	virtual List<ProductionRule^>^ Assess_CreateRulesStep_RuleLengths(array<Int32>^ Solution);
	virtual float Assess_Modules(array<Int32>^ Solution) override;
	virtual float Assess_Constants(array<Int32>^ Solution) override;
	virtual float AssessRuleLengths(array<Int32>^ Solution);
	virtual float AssessRuleLengths(array<Int32>^ Solution, bool ForceRuleLength);
	virtual float AssessProductionRules(List<ProductionRule^>^ Rules) override;

	virtual void CaptureFragment(Int32 iRule, Word^ Rules);
	virtual void CapturePartialFragment(Int32 iRule, Word^ Rules);
	virtual void CapturePartialSolution(array<Int32, 2>^ Rules) override;
	virtual void CapturePartialSolution_Rules(List<ProductionRule^>^ Rules) override;

	virtual array<Int32>^ Cheat(array<Int32>^ Solution) override;
	virtual array<Int32, 2>^ CheatParikh();

	virtual void CompressEvidence();
	virtual Lexicon^ CompressMarkerList(Lexicon^ Source, Lexicon^ Production, Int32 CountLowerBound, Int32 CountUpperBound);
	virtual List<Word^>^ CompressWordList(List<Word^>^ W);

	virtual bool ComputeConstants();
	virtual List<SymbolFragmentType^>^ ComputeConstantsFromEqualWords(Word^ W);
	virtual List<Word^>^ ComputeEvidenceFromMarkers(List<MarkerMapType^>^ MarkerMap, Word^ E);
	virtual bool ComputeFragments(array<Int32, 2>^ Rules);
	virtual bool ComputeFragments_v2(array<Int32, 2>^ Rules);
	virtual void ComputeFragmentsFromEvidence();
	virtual void ComputeFragmentsFromPartialSolution();
	virtual List<SymbolFragmentType^>^ ComputeMaxFragmentsFromEvidence(Word^ E1, Word^ E2);
	virtual List<SymbolFragmentType^>^ ComputeMinFragmentsFromEvidence(Word^ E1, Word^ E2);
	//virtual void ComputeFragmentsByScanWithMarkers(Word^ W0, Word^ W1, Lexicon^ Markers);
	virtual void ComputeFragmentsUsingMarkerMap(Word^ W0, Word^ W1);
	virtual void ComputeFragmentsUsingSymbolMarkers(Word^ W0, Word^ W1, Symbol^ Left, Symbol^ Right);
	virtual bool ComputeFragmentsFromRPT();
	virtual void ComputeLexicon() override;
	virtual Lexicon^ ComputeLexiconForWord(Word^ W, Int32 SizeLimit);
	virtual List<MarkerMapType^>^ ComputeMarkerMap(Lexicon^ Markers, Word^ E);
	virtual List<MarkerMapType^>^ ComputeMarkerMap(List<MarkerMapType^>^ Markers, Word^ E);
	virtual List<SymbolFragmentType^>^ ComputeMaxFragment(Word^ Source, Word^ Production);
	virtual SymbolFragmentType^ ComputeMaxHead(Word^ Source, Word^ Production);
	virtual SymbolFragmentType^ ComputeMinHead(Word^ Source, Word^ Production);
	virtual SymbolFragmentType^ ComputeMaxTail(Word^ Source, Word^ Production);
	virtual SymbolFragmentType^ ComputeMinTail(Word^ Source, Word^ Production);
	virtual void ComputeMinHeadsTails() override;
	virtual array<Int32>^ ComputeRuleLengthFromRPT(Int32 iGen, Int32 SymbolIndex);
	virtual void ComputeSymbiology() override;

	virtual List<MarkerMapType^>^ ConvertMarkerMap(List<MarkerMapType^>^ Markers);
	virtual Word^ ConvertWord(Word^ W);

	virtual void CreateModelFromRPT() override;
	virtual List<ProductionRule^>^ CreateRulesFromRPT();
	virtual array<Int32, 2>^ CreateParikhRuleFromRPT();
	virtual List<ProductionRule^>^ CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration) override;
	virtual List<ProductionRule^>^ CreateRulesByScanning(array<Int32>^ RuleLengths, Int32 StartGeneration, Int32 EndGeneration, bool Finalize);
	virtual void CreateSubProblems_Generations(List<Int32>^ Cutoffs) override;

	virtual void EstimateGrowthPattern() override;
	virtual bool EstimateRuleLengths_Fragments() override;
	virtual bool EstimateRuleLengths_Growth() override;
	virtual bool EstimateRuleLengths_Inference(Int32 Start, Int32 End) override;
	virtual bool EstimateRuleLengths_Inference(Word^ Source, Word^ Production); // NOT WORKING
	virtual bool EstimateRuleLengths_Lexicon() override;
	virtual bool EstimateRuleLengths_PoG() override;

	virtual void ExtractVocabulary() override;

	virtual bool IsGenerationAllConstants(Int32 Generation);
	virtual bool IsModuleInGeneration(Int32 Generation, Int32 iRule);
	virtual bool IsModuleSolved(Int32 iRule) override;
	virtual bool IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex) override;
	virtual bool IsModuleSolvedForSymbol(Int32 RuleIndex, Int32 SymbolIndex, Int32 Start, Int32 End);
	virtual bool IsParikhAllConstants(array<Int32>^ PV);
	virtual bool IsProblemSetComplete(PMIProblemSet^ P);
	virtual bool IsProblemSolved(Int32 StartGen, Int32 EndGen, Int32 StartSymbolIndex, Int32 EndSymbolIndex);
	virtual bool IsTabu(array<Int32, 2>^ Rules);
	virtual bool IsTabu(Int32 Index, array<Int32, 2>^ Rules);
	virtual bool IsTabu(Int32 Index, array<Int32>^ Lengths);
	virtual bool IsTabu(Int32 Index, List<ProductionRule^>^ Rules);
	//virtual bool IsTabu(array<Int32>^ Solution);
	virtual bool IsWordAllConstants(Word^ W);

	virtual void ModifyProblem(Int32 SymbolIndex);
	virtual void ModifyVocabulary(Symbol^ S);
	
	virtual void OutputRules(array<Int32, 2>^ R);
	virtual void OutputRules(List<ProductionRule^>^ R);

	virtual void RollbackAnalysisForTabu(TabuItem^ T);
	virtual void RollbackKnownRuleLengths(Int32 Index);
	virtual void RollbackPartialSolution() override;
	virtual void RollbackProblemSet();
	virtual void RollbackSpecial();

	virtual void Solve() override;
	virtual void Solve_Special() override;
	virtual bool SolveProblemSet() override;
	virtual bool SolveProblemSet_Special() override;
	virtual bool SolveProblem();
	virtual bool SolveForModules();
	virtual bool SolveForConstants();
	virtual bool SolveForOrder();

	virtual void StripConstants(Word^ W);
	virtual void StripConstants(Word^ E1, Word^ E2);

	virtual void SynchronizeAnalysis();

	virtual void UpdateKnownRuleLengths() override;
	virtual void UpdateKnownRuleLengths(Word^ W);
	virtual bool UpdateMaxPoG(Int32 iRule, Word^ Fragment);
	virtual void UpdateSolvedModules();
	virtual void UpdateTotalRuleLengths() override;
	virtual void UpdateWIP(array<Int32, 2>^ Rules);
	virtual void UpdateWIP(List<ProductionRule^>^ Rules);

	bool VerifyOrderScan(array<Int32, 2>^ Rules);

	IPMIFitnessFunction^ fitnessFunction;

protected:
	static const Int32 cMaxAttempts = 10000;
	static const Int32 cMaxMarkerLength = 35;
	static const Int32 cMinMarkerFrequency = 1;
	static const Int32 cMaxMarkerFrequency = 100;
	static const Int32 cMaxMarkerFrequencyDifference = 0;
};

