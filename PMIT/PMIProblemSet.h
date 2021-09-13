#pragma once
#include "PMIProblemDescriptor.h"

using namespace System;
using namespace System::Collections::Generic;

public interface class IPMIProblemSet
{
	property bool disabled;

	bool IsComplete();
	bool IsSolved();
};

public ref class PMIProblemSet :
	public IPMIProblemSet
{
public:
	PMIProblemSet(Int32 StartGeneration, Int32 EndGeneration);
	virtual ~PMIProblemSet();

	Int32 startGeneration;
	Int32 endGeneration;

	PMIProblemDescriptor^ moduleProblem;
	PMIProblemDescriptor^ constantProblem;
	PMIProblemDescriptor^ orderProblem;
	List<PMIProblemDescriptor^>^ otherProblems; // TODO: currently used to store length problems. Probably need to subclass this off.
	
	virtual property bool disabled;

	virtual bool IsComplete();
	virtual bool IsSolved();
};