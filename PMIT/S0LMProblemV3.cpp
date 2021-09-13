#include "stdafx.h"
#include "S0LMProblemV3.h"


S0LMProblemV3::S0LMProblemV3(ModelV3^ M, ProblemType T) : S0LProblemV3(M,T)
{
}


S0LMProblemV3::~S0LMProblemV3()
{
}

double S0LMProblemV3::Assess_Dbl(array<Int32>^ RuleLengths)
{
	this->modelCurrent = this->model;
	double fitness = this->Assess_Dbl_L(RuleLengths);

	//for (size_t iRule = 0; iRule <= this->successorsCurrent->GetUpperBound(0); iRule++)
	//{
	//	for (size_t iSuccessor = 0; iSuccessor < this->successorsCurrent[iRule]->Count; iSuccessor++)
	//	{
	//		Console::WriteLine("Precount: " + this->successorsCurrent[iRule][iSuccessor]->successor + " " + this->successorsCurrent[iRule][iSuccessor]->count);
	//	}
	//}

	if (this->secondaryModels != nullptr)
	{
		for (size_t iModel = 0; iModel < this->secondaryModels->Count; iModel++)
		{
			this->modelCurrent = this->secondaryModels[iModel];
			fitness += this->Assess_Successors_Dbl(this->successorsCurrent, this->secondaryModels[iModel]);
		}
	}

	if (fitness > this->fitnessBest)
	{
		//for (size_t iRule = 0; iRule <= this->successorsCurrent->GetUpperBound(0); iRule++)
		//{
		//	for (size_t iSuccessor = 0; iSuccessor < this->successorsCurrent[iRule]->Count; iSuccessor++)
		//	{
		//		Console::WriteLine("Postcount: " + this->successorsCurrent[iRule][iSuccessor]->successor + " " + this->successorsCurrent[iRule][iSuccessor]->count);
		//	}
		//}

		this->fitnessBest = fitness;
		this->successorsBest = this->successorsCurrent;
	}

	return fitness;
}

double S0LMProblemV3::Assess_Successors_Dbl(array<List<SuccessorCount^>^>^ Successors, ModelV3^ M)
{
	double fitness = 0;
	Int32 iGen = 0;
	Int32 iPos = 0;
	Int32 iScan = 0;
	bool done = false;
	bool failed = false;
	Int32 iSymbol;
	this->longestPath = gcnew array<Int32>(this->modelCurrent->evidence[0]->Length());

	while (!done && !failed && iGen < this->modelCurrent->generations - 1)
	{
		iSymbol = this->modelCurrent->evidence[iGen]->symbolIndices[iPos];
		String^ successor = "";
		Int32 length = 0;
		if (!this->modelCurrent->alphabet->IsTurtle(iSymbol))
		{
			bool found = false;
			Int32 iSuccessor = 0;

			successor = this->Assess_Dbl_PeekAheadStep_L(Successors, iSymbol, iGen, iPos, iScan, 0);

			if (successor == "")
			{
				failed = true;
			}
		}
		else
		{
			successor = this->modelCurrent->alphabet->GetSymbol(iSymbol);
		}

		failed = failed || (successor == "");

		if (!failed)
		{
			if (!this->modelCurrent->alphabet->IsTurtle(iSymbol))
			{
				Successors = this->AddSuccessor(iGen, iPos, Successors, successor);
				fitness++;
			}

			iPos++;
			iScan += successor->Length;

			if (iPos >= this->modelCurrent->evidence[iGen]->Length())
			{
				iGen++;
				iPos = 0;
				iScan = 0;
				this->longestPath = gcnew array<Int32>(this->modelCurrent->evidence[iGen]->Length());
				this->pathEnd = -1;
			}

			if (iGen >= this->modelCurrent->generations - 1)
			{
				done = true;
			}
		}
	}

	if (failed)
	{
		fitness = fitness / 2;
	}

	return fitness;
}

