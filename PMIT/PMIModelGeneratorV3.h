#pragma once
#include "PMIRandomModelGenerator.h"
#include "ModelV3.h"

#include "AlphabetV3.h"
#include "ProductionTableV3.h"
#include "ProductionRuleV3.h"

public ref class GeneratorStatistics
{
public:
	GeneratorStatistics();
	~GeneratorStatistics();

	virtual void Load(String^ FileName);

	List<float>^ distributionRuleLengths;
	List<float>^ distributionRuleLengthsProducing;
	List<float>^ distributionRuleLengthsGraphical;

	List<bool>^ patternsIsProducing;
	List<bool>^ patternsIsGraphical;
	List<bool>^ patternsIsTerminal;
	List<Int32>^ patternsLength;
	List<Int32>^ patternsLengthCompressed;
	List<Int32>^ patternsNumProducingSymbols;
	List<Int32>^ patternsNumGraphicalSymbols;
	List<Int32>^ patternsNumTerminalSymbols;
	List<float>^ patternsP2GRatio;
	List<String^>^ patterns;

	List<bool>^ patternsCompressedIsProducing;
	List<bool>^ patternsCompressedIsGraphical;
	List<bool>^ patternsCompressedIsTerminal;
	List<Int32>^ patternsCompressedLength;
	List<Int32>^ patternsCompressedLengthCompressed;
	List<Int32>^ patternsCompressedNumProducingSymbols;
	List<Int32>^ patternsCompressedNumGraphicalSymbols;
	List<Int32>^ patternsCompressedNumTerminalSymbols;
	List<float>^ patternsCompressedP2GRatio;
	List<String^>^ patternsCompressed;


	float avgNumRulesProducing;
	float avgNumRulesDrawing;
	float avgNumRulesTerminal;
	float maxNumRules;

	float avgRepeatedSymbols;
	float minRepeatedSymbols;
	float maxRepeatedSymbols;

	float avgG2PTransitions;
	float minG2PTransitions;
	float maxG2PTransitions;

	float avgP2GTransitions;
	float minP2GTransitions;
	float maxP2GTransitions;

	float ratioProducingRules;

	enum class ConfigFileLine
	{
		NumRules,
		P2GRuleRatio,
		G2PTransitions,
		P2GTransitions,
		RepeatedSymbols,
		LengthDistribution,
		Patterns,
		CompressedPatterns,
	};
};

public ref class PMIModelGeneratorV3
{
public:
	PMIModelGeneratorV3(GeneratorStatistics^ Stats);
	virtual ~PMIModelGeneratorV3();

	virtual ModelV3^ BuildD0L();

	virtual ProductionRuleV3^BuildDrawingRule(String^ Predecessor);
	virtual ProductionRuleV3^ BuildProducingRule(String^ Predecessor);


	ModelV3^ model;
	GeneratorStatistics^ stats;

	List<String^>^ symbolPool;
	List<String^>^ specialPool;

	MinMaxI^ alphabetSize;
	MinMaxI^ producingSymbols;

	MinMaxI^ lengthShort;
	MinMaxI^ lengthMedium;
	MinMaxI^ lengthLong;
	MinMaxI^ lengthVeryLong;

	MinMaxI^ lengthBranch;
	MinMaxI^ lengthBranchOnBranch;

	float chanceRuleIsShort;
	float chanceRuleIsMedium;
	float chanceRuleIsLong;
	float chanceRuleIsVeryLong;

	MinMaxF^ ratioModuleShort;
	MinMaxF^ ratioModuleMedium;
	MinMaxF^ ratioModuleLong;
	MinMaxF^ ratioModuleVeryLong;
	MinMaxF^ ratioModuleBranch;

	MinMaxI^ numConstantOnlyRules;

	float chanceRuleConstantOnlyShort;
	float chanceRuleConstantOnlyMedium;
	float chanceRuleConstantOnlyLong;
	float chanceRuleConstantOnlyVeryLong;

	bool branchingAllowed;
	bool leftRightAllowed;

	float chanceRuleHasBranch;
	float chanceBranchInBranch;
	MinMaxI^ numBranchShortRule;
	MinMaxI^ numBranchMediumRule;
	MinMaxI^ numBranchLongRule;
	MinMaxI^ numBranchVeryLongRule;
	MinMaxI^ numBranchOnBranch;

	float chanceModuleNeighbourIsConstant;
	float chanceConstantNeighbourIsConstant;
	float chanceConstantNeighbourIsSameConstant;
};

