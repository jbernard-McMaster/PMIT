#pragma once
#include "AlphabetV3.h"
#include "ProductionTableV3.h"
#include "EvidenceFactoryV3.h"
#include "WordV3.h"


public ref class ModelV3
{
public:
	ModelV3();
	ModelV3(String^ FileName);
	ModelV3(String^ FileName, bool WithOverlays);
	virtual ~ModelV3();

	void Initialize();

	AlphabetV3^ alphabet;
	AlphabetV3^ alphabetShadow;
	WordXV3^ axiom;
	List<ProductionTableV3^>^ productions;
	array<WordXV3^>^ words;
	array<WordXV3^>^ evidence;
	List<String^>^ parameters;
	bool contextFree;
	double oddsProduced;

	Int32 generations;
	Int32 numRules;
	Int32 numSymbols;
	Int32 numSpecials;
	Int32 successorShortest;
	Int32 successorLongest;

	virtual ModelV3^ Adjust(Int32 NumSymbol);

	virtual void ComputeGenerations();
	virtual List<SAC^>^ ComputeSACsInWord(WordXV3^ w);
	virtual WordXV3^ ComputeWordX(WordV3^ W);
	virtual void ComputeOddsProduced();

	virtual void CreateError(float Pnop, float Pr, float Pi, float Pd, bool ReplacementAllowed, bool InsertionAllowed, bool DeletionAllowed);
	virtual void CreateEvidence();
	virtual void CreateEvidence(bool WithOverlays);
	virtual void CreateEvidenceS0L();
	virtual void CreateEvidence_Dynamic();
	virtual void CreateOverlays();
	virtual void Filter();

	virtual void Display();
	
	virtual void GenerateD2L();
	virtual void GenerateS0L();
	virtual void GenerateS0LPrefix();

	virtual bool IsMatch(ModelV3^ Model);
	virtual bool IsSACInList(List<SACX^>^ L, SACX^ S);
	virtual bool IsSACInList(List<SAC^>^ L, SAC^ S);

	virtual void Load(String^ FileName);
	virtual void Load(String^ FileName, List<String^>^ Filters);
	virtual void LoadRule(array<String^, 1>^ Words, List<String^>^ Parameters, List<String^>^ Filters);
	virtual void Save();

	virtual List<ProductionRuleV3^>^ SortProductionRules(List<ProductionRuleV3^>^ Rules);
	
	enum class ConfigFileLine
	{
		VocabularyFile,
		Generations,
		Axiom,
		Table,
		Parameters,
		Rules
	};

	static const Int32 cRuleName = 0;
	static const Int32 cTableIndex = 1;
	static const Int32 cOdds = 2;
	//static const Int32 cTimeConditionStart = 3;
	//static const Int32 cTimeConditionEnd = 4;
	static const Int32 cPredecessor= 3;
	static const Int32 cLeftContext = 4;
	static const Int32 cRightContext = 5;
	static const Int32 cCondition= 6;
	static const Int32 cSuccessor = 7;

	array<float>^ lengthDistribution;
	array<float>^ symbolDistribution;

};

