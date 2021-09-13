#pragma once

#include "defines.h"
#include "Word.h"
#include "EvidenceFactory.h"
#include "ProductionRule.h"
#include "ProductionRuleConstraint.h"
#include "PMIException.h"
#include "Vocabulary.h"

#define _PMI_PLANT_MODEL_VERBOSE_ 0

public ref class RuleStatistics
{
public:
	RuleStatistics();
	virtual ~RuleStatistics();

	bool ruleIsProducer;
	bool ruleIsGraphical;
	bool ruleIsTerminal;
	bool ruleProducesProducer;

	String^ pattern = "";
	String^ patternCompressed = "";
	List<String^>^ uniqueSymbols = gcnew List<String^>;
	List<String^>^ uniqueGraphics = gcnew List<String^>;

	float length = 0.0;
	float lengthCompressed = 0.0;
	float countGraphicalSymbols = 0.0;
	float countProducingSymbols = 0.0;
	float countTerminalSymbols = 0.0;
	float countRepeatedSymbols = 0.0;
	float countGraphical2ProducingSymbols = 0.0;
	float countProducing2GraphicalSymbols = 0.0;
	float ratioProducing = 0.0;
};

public ref class ModelStatistics
{
public:
	ModelStatistics();
	virtual ~ModelStatistics();

	List<RuleStatistics^>^ ruleStats;
	array<Int32, 2>^ symbolPattern;
	array<Int32, 2>^ symbolPatternCompressed;

	float avgRuleLength;
	float avgRuleLengthCompressed;
	float numRules;

	float countDrawingRules = 0.0;
	float countProducingRules = 0.0;
	float countTerminalRules = 0.0;

	void Display();
	void Write(String^ FileName);
};

public interface class IPlantModel
{
	property List<IProductionRule^>^ rules;
};

public ref class PlantModel
{
public:
	PlantModel();
	virtual ~PlantModel();

	Vocabulary^ vocabulary;
	Word^ axiom;
	List<ProductionRule^>^ rules;
	List<Word^>^ evidence;
	List<Word^>^ evidenceModuleOnly;
	List<String^>^ turtleAlphabet;
	ModelStatistics^ modelStats;


	Int32 generations;
	Int32 numRules;
	Int32 numModules;
	Int32 numConstants;

	virtual void Analyze();

	virtual String^ CompressPattern(String^ P);
	virtual void CreateEvidence();

	virtual void Display();
	virtual bool IsMatch(PlantModel^ Model);

	virtual void Load(String^ FileName);
	virtual void LoadRule(array<String^, 1>^ Words);
	virtual void Save();

	bool Validate();

	enum class ConfigFileLine
	{
		VocabularyFile,
		Generations,
		Axiom,
		Table,
		Parameters,
		RuleHeader,
		Rules
	};

	static const Int32 cRuleName = 0;
	static const Int32 cRuleGenerationConstraint = 1;
	static const Int32 cRuleSymbols = 2;
};

