#pragma once

using namespace System;

public ref class BruteForceSearchR_Dbl_PMIT :
	public BruteForceSearchR_Dbl
{
public:
	BruteForceSearchR_Dbl_PMIT(BruteForceSearchConfiguration^ Configuration);
	virtual ~BruteForceSearchR_Dbl_PMIT();

	array<Int32>^ skipValues;

	bool CheckDelayedSkip(Int32 Index);
	void SetDelayedSkip(Int32 Index, Int32 Value);

	void Override(Int32 Index, Int32 Value);

	virtual array<Int32>^ Solve() override;

	float fitnessTarget;
	bool flagOverride;
	Int32 valueSkip;
};

