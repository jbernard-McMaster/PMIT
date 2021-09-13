#pragma once
#include "PMISolution.h"
#include "TabuItem.h"
#include "Vocabulary.h"

using namespace System;

public ref class PMIProblemDescriptor
{
public:
	enum class ProblemType
	{
		Modules,
		Constants,
		Length,
		LengthModulesOnly,
		Order,
		Fragment,
		Direct,
		DirectModulesOnly
	};

	PMIProblemDescriptor();
	PMIProblemDescriptor(Int32 StartGeneration, Int32 EndGeneration, Vocabulary^ Vocabulary, ProblemType SolveFor, Int32 Priority);
	virtual ~PMIProblemDescriptor();

	Int32 startGeneration;
	Int32 endGeneration;
	array<bool>^ modulesToSolve;
	Vocabulary^ vocabulary;
	Int32 priority = 0;

	PMISolution^ solution;

	List<PMISolution^>^ fullSolutions;
	List<PMISolution^>^ partialSolutions;

	Int32 constantIndex = 0;
	bool solved = false;
	bool complete = true;
	bool unset = true;

	ProblemType solveFor;

	static const Int32 PriorityModuleProblem = 0;
	static const Int32 PriorityLengthProblem = 0;
	static const Int32 PriorityConstantProblem = 1;
	static const Int32 PriorityOrderProblem = 2;
	static const Int32 PriorityWholeRuleProblem = 3;

	List<TabuItem^>^ tabu;
	List<array<Int32>^>^ tabuSolutions;

	virtual void Reset();

	virtual Int32 NumGenerations() { return this->endGeneration - this->startGeneration + 1; };
	virtual String^ ToString() new;
	virtual bool IsTabu(array<Int32, 2>^ Rules);
	virtual bool IsTabu(Int32 Index, array<Int32, 2>^ Rules);
	virtual bool IsTabu(array<Int32>^ Solution);
};

