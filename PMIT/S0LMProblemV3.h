#pragma once
#include "S0LProblemV3.h"
#include "ModelV3.h"

public ref class S0LMProblemV3 :
	public S0LProblemV3
{
public:
	S0LMProblemV3(ModelV3^ M, ProblemType T);
	virtual ~S0LMProblemV3();

	virtual double Assess_Dbl(array<Int32>^ RuleLengths) override;
	virtual double Assess_Successors_Dbl(array<List<SuccessorCount^>^>^ Successors, ModelV3^ M);

	List<ModelV3^>^ secondaryModels;
};

