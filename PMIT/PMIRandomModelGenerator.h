#pragma once

#include "PlantModel.h"

using namespace System;

public ref struct MinMaxI
{
	Int32 min;
	Int32 max;
};

public ref struct MinMaxF
{
	float min;
	float max;
};

public ref class PMIRandomModelGenerator
{
public:
	PMIRandomModelGenerator();
	virtual ~PMIRandomModelGenerator();

	virtual PlantModel^ BuildD0L();
	virtual PlantModel^ BuildD0L_Special(Int32 NumSymbols, Int32 MaxLength);

	virtual void Load();


private:
	virtual Word^ BuildBranch(bool IsSubBranch);
	
	virtual Symbol^ PickConstant(Symbol^ Previous, bool LeadSymbol);
	virtual Symbol^ PickModule();
	virtual Symbol^ PickSymbol(Symbol^ Previous, bool LeadSymbol, bool ConstantOnly);
	virtual bool Verify();

	PlantModel^ model;

	List<String^>^ moduleValues;
	List<String^>^ constantValues;

	MinMaxI^ numModules;
	MinMaxI^ numConstants;

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

