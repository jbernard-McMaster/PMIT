#include "stdafx.h"
#include "PMIProblemSet.h"

PMIProblemSet::PMIProblemSet(Int32 StartGeneration, Int32 EndGeneration)
{
	this->moduleProblem = gcnew PMIProblemDescriptor;
	this->constantProblem = gcnew PMIProblemDescriptor;
	this->orderProblem = gcnew PMIProblemDescriptor;
	this->otherProblems = gcnew List<PMIProblemDescriptor^>;
	this->startGeneration = StartGeneration;
	this->endGeneration = EndGeneration;
}

PMIProblemSet::~PMIProblemSet()
{
}

bool PMIProblemSet::IsComplete()
{
	bool result = true;

	if (this->moduleProblem != nullptr)
	{
		result = result && (this->moduleProblem->complete || this->moduleProblem->unset);
	}

	if (this->constantProblem != nullptr)
	{
		result = result && (this->constantProblem->complete || this->constantProblem->unset);
	}

	if (this->orderProblem != nullptr)
	{
		result = result && (this->orderProblem->complete || this->orderProblem->unset);
	}

	for (size_t iProblem = 0; iProblem < this->otherProblems->Count; iProblem++)
	{
		result = result && (this->otherProblems[iProblem]->complete || this->otherProblems[iProblem]->unset);
	}

	result = result && (~this->disabled);

	return result;
}

bool PMIProblemSet::IsSolved()
{
	bool result = true;

	if (this->moduleProblem != nullptr)
	{
		result = result && (this->moduleProblem->solved || this->moduleProblem->unset);
	}

	if (this->constantProblem != nullptr)
	{
		result = result && (this->constantProblem->solved || this->constantProblem->unset);
	}

	if (this->orderProblem != nullptr)
	{
		result = result && (this->orderProblem->solved || this->orderProblem->unset);
	}

	for (size_t iProblem = 0; iProblem < this->otherProblems->Count; iProblem++)
	{
		result = result && (this->otherProblems[iProblem]->solved || this->otherProblems[iProblem]->unset);
	}

	return result;
}