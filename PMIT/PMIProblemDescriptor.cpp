#include "stdafx.h"
#include "PMIProblemDescriptor.h"


PMIProblemDescriptor::PMIProblemDescriptor()
{
	this->tabuSolutions = gcnew List<array<Int32>^>;
	this->fullSolutions = gcnew List<PMISolution^>;
	this->partialSolutions = gcnew List<PMISolution^>;
	this->unset = true;
}

PMIProblemDescriptor::PMIProblemDescriptor(Int32 StartGeneration, Int32 EndGeneration, Vocabulary^ Vocabulary, PMIProblemDescriptor::ProblemType SolveFor, Int32 Priority)
{
	this->startGeneration = StartGeneration;
	this->endGeneration = EndGeneration;
	this->vocabulary = Vocabulary;
	this->solveFor = SolveFor;
	this->priority = Priority;
	this->tabu = gcnew List<TabuItem^>;
	this->tabuSolutions = gcnew List<array<Int32>^>;
	this->fullSolutions = gcnew List<PMISolution^>;
	this->partialSolutions = gcnew List<PMISolution^>;
	this->modulesToSolve = gcnew array<bool>(this->vocabulary->numModules);
	this->unset = false;
}

PMIProblemDescriptor::~PMIProblemDescriptor()
{
}

void PMIProblemDescriptor::Reset()
{
	this->solved = false;
	this->constantIndex = 0;
}

String^ PMIProblemDescriptor::ToString()
{
	String^ result;

	result += "S: " + this->startGeneration + ", E: " + this->endGeneration;

	switch (this->solveFor)
	{
	case PMIProblemDescriptor::ProblemType::Modules:
		result = result + ", SF: Modules";
		break;
	case PMIProblemDescriptor::ProblemType::Constants:
		result = result + ", SF: Constants";
		break;
	case PMIProblemDescriptor::ProblemType::Length:
		result = result + ", SF: Length";
		break;
	case PMIProblemDescriptor::ProblemType::Order:
		result = result + ", SF: Order";
		break;
	}

	return result;
}

bool PMIProblemDescriptor::IsTabu(array<Int32, 2>^ Rules)
{
	return this->IsTabu(TabuItem::ModulesOnlyIndex, Rules);
}

bool PMIProblemDescriptor::IsTabu(Int32 Index, array<Int32, 2>^ Rules)
{
	bool result = false;
	Int32 idx = 0;

	while ((!result) && (idx < this->tabu->Count))
	{
		if (Index == this->tabu[idx]->index)
		{
			array<Int32, 2>^ tabuRules = this->tabu[idx]->rulesParikh;
			Int32 iRule = 0;
			bool isMatch = true;
			while ((isMatch) && (iRule <= Rules->GetUpperBound(0)))
			{
				Int32 iSymbol = 0;

				while ((isMatch) && (iSymbol <= Rules->GetUpperBound(1)))
				{
					isMatch = Rules[iRule, iSymbol] == tabuRules[iRule, iSymbol];
					iSymbol++;
				}
				iRule++;
			}

			result = isMatch;
		}
		idx++;
	}
	return result;
}

bool PMIProblemDescriptor::IsTabu(array<Int32>^ Solution)
{
	Int32 iTabu = 0;
	bool result = false;

	while ((iTabu < this->tabuSolutions->Count) && (!result))
	{
		Int32 iValue = 0;
		bool isMatch = true;
		array<Int32>^ tabuSolution = this->tabuSolutions[iTabu];

		do
		{
			if (Solution[iValue] != tabuSolution[iValue])
			{
				isMatch = false;
			}
			iValue++;
		} while ((iValue <= Solution->GetUpperBound(0)) && (isMatch));

		if (isMatch)
		{
			result = true;
		}

		iTabu++;
	}

	return result;
}
