#pragma once
#include "LSIProblemV3.h"
#include "ModelV3.h"
#include "BruteForceSearchR_Dbl_PMIT.h"

public ref class S0LProblemV3 :
	public LSIProblemV3
{
public:
	S0LProblemV3(ModelV3^ M, ProblemType T);

	virtual void Initialize() override;

	virtual void Analyze() override;

	virtual array<List<SuccessorCount^>^>^ AddSuccessor(Int32 iGen, Int32 iPos, array<List<SuccessorCount^>^>^ Successors, String^ S);

	virtual float Assess(array<Int32>^ RuleLengths) override;
	virtual double Assess_Dbl(array<Int32>^ RuleLengths) override;
	virtual double Assess_Dbl_PoL(array<Int32>^ RuleLengths) override;
	virtual double Assess_Dbl_PoL2(array<Int32>^ RuleLengths) override;
	virtual double Assess_Dbl_L(array<Int32>^ RuleLengths) override;
	virtual String^ Assess_Dbl_ConsumeGeneStep_PoL(array<Int32>^ RuleLengths, Int32 iGene, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin);
	virtual String^ Assess_Dbl_ConsumeGeneStep_PoL2(Int32 V, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin);
	virtual String^ Assess_Dbl_ConsumeGeneStep_PoL_Old(array<Int32>^ RuleLengths, Int32 iGene, Int32 iGen, Int32 iPos, Int32 iScan, Int32 ReserveRightMax, Int32 ReserveRightMin);
	virtual String^ Assess_Dbl_ConsumeGeneStep_L(Int32 V, Int32 iGen, Int32 iPos, Int32 iScan);
	virtual String^ Assess_Dbl_PeekAheadStep_L(array<List<SuccessorCount^>^>^ Successors, Int32 iSymbol, Int32 iGen, Int32 iPos, Int32 iScan, Int32 PeekCount);
	virtual Int32 Assess_Dbl_PeekAheadStepWithRecursion_L(array<List<SuccessorCount^>^>^ Successors, Int32 iSymbol, Int32 iGen, Int32 iPos, Int32 iScan, Int32 PeekCount, Int32 RecursionCount);

	virtual array<List<SuccessorCount^>^>^ CombineSuccessorCounts(array<List<SuccessorCount^>^>^ A, array<List<SuccessorCount^>^>^ B);
	
	virtual void ComputeMetrics(array<List<SuccessorCount^>^>^ Successors, double Odds);

	virtual void ComputeLength_Production_S0LOnly();
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_Down_Left2Right(array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 StartGen, Int32 StartPos);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_Down_Left2Right(array<List<SuccessorCount^>^>^ H);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_Up_Left2Right(array<List<SuccessorCount^>^>^ H);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_Down_Right2Left(array<List<SuccessorCount^>^>^ H);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_Up_Right2Left(array<List<SuccessorCount^>^>^ H);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessors_Greedy_SingleGeneration(array<List<SuccessorCount^>^>^ H, Int32 iGen);

	virtual double ComputeGreedySuccessor(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep);
	virtual double ComputeGreedySuccessor_Right2Left(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep);
	virtual double ComputeGreedySuccessor_SharedHistogram(Int32 iGen, Int32 iPos, Int32 iScan, float O, array<List<SuccessorCount^>^>^ H, array<List<SuccessorCount^>^>^ Successors, Int32 ReserveRightMin, Int32 ReserveRightMax, Int32 iStep);
	virtual List<SuccessorCount^>^ ComputeSuccessorList(array<List<SuccessorCount^>^>^ H);
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessorHistogram();
	virtual array<List<SuccessorCount^>^>^ ComputeSuccessorHistogram(array<List<SuccessorCount^>^>^ Successors);
	virtual double ComputeSuccessorOdds(String^ S, array<List<SuccessorCount^>^>^ Counts, Int32 iFact);
	virtual double ComputeSuccessorsOdds(array<List<SuccessorCount^>^>^ Successors);
	virtual double ComputeSuccessorsOdds_SingleGenerationOnly(array<List<SuccessorCount^>^>^ Successors);

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual GenomeConfiguration<Int32>^ CreateGenome_GreedyBF();
	virtual BruteForceSearchConfiguration^ CreateSearchSpace(GenomeConfiguration<Int32>^ GenomeConfig);

	virtual List<ProductionRuleV3^>^ CreateRulesByScanning(array<Int32>^ RuleLengths) override;

	virtual void Decode(array<Int32>^ RuleLengths);
	virtual void Decode2(array<Int32>^ RuleLengths);
	virtual void Decode_L(array<Int32>^ RuleLengths);
	virtual void Decode_PoL(array<Int32>^ RuleLengths);

	virtual void ComputeModelOdds();

	virtual double EstimateTerminationFitness();

	virtual void OverrideSearchSpace(Int32 Index, Int32 Value);

	virtual void Solve() override;
	BruteForceSearchR_Dbl_PMIT^ aiBF;

	virtual SolutionType^ SolveProblemBF();
	virtual SolutionType^ SolveProblemGreedyBF();
	virtual SolutionType^ SolveProblemGreedyGA();
	virtual SolutionType^ SolveProblemGreedy_V1();
	virtual SolutionType^ SolveProblemGreedy_V2();
	virtual SolutionType^ SolveProblemGreedy_V1_1();
	virtual SolutionType^ SolveProblemGreedyNash_V1();
	virtual SolutionType^ SolveProblemGreedyNash_V2();

	array<Int32>^ lastSymbolPos;
	array<List<Fact^>^>^ factOverlay;
	array<array<Int32>^>^ factIndexOverlay;
	List<SuccessorCount^>^ successorList;
	array<List<SuccessorCount^>^>^ histogram;
	array<List<SuccessorCount^>^>^ successorsBest;
	array<List<SuccessorCount^>^>^ successorsCurrent;
	ModelV3^ modelCurrent;
	array<Int32>^ factCount;
	array<Int32>^ lengthValues;
	Int32 lastLength;
	bool flagSkipSet;
	Int32 pathEnd;
	array<Int32>^ longestPath;
	Int32 absolutePeekLimit;
	Int32 absoluteRecursionLimit;
	Int32 iteration;
	UInt64 maxSearchSpaceSize;
	bool peekOverrideFlag;
	bool lengthBasedFlag;
	bool doneFlag;
	bool solvedFlag;
	bool nontrivialFlag;

	bool flagOverride;
	Int32 overrideIndex;
	Int32 overrideValue;

	Int32 absoluteMaxLength;
	Int32 maxGreedySteps;
	float avgLength;
	float preferredLength;
	double modelOdds;
	double fitnessBest;
	double fitnessTarget;
	double successTarget;
	Int32 numGenes;
	float wtp1;
	float wtp2;
	float wfp;
	float wfn;
	float errorAvg;
	float errorTotal;
	Int32 successorCountDiff;
	bool successorCountMM;
	bool success;
	bool cheat = false;

	const double cFitnessTarget = 9999999;
};

