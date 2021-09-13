#pragma once

#include "mathparser.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class ProductionRuleV3
{
public:
	ProductionRuleV3();
	virtual ~ProductionRuleV3();

	String^ predecessor;

	List<String^>^ successor; // Should this be a WordV3?
	List<Int32>^ odds;
	List<String^>^ contextLeft;
	List<String^>^ contextRight;
	List<String^>^ condition;
	List<String^>^ parameters;
	List<Int32>^ count;

	void Display();

	virtual bool IsMatchPredecessor(String^ P);

	virtual Int32 GetSuccessorID(String^ S);
	virtual String^ Produce(String^ L, String^ R, Int32 O, Int32 T, List<float>^ V);
	virtual bool ShouldFire(Int32 RuleIndex, String^ L, String^ R, Int32 O, Int32 T, List<float>^ V);

	static String^ WrongRule = "***";
};

