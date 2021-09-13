#pragma once
#include "ParikhEquation.h"
#include "PMIProblem.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class PMIProblem_Math :
	public PMIProblem
{
public:
	PMIProblem_Math(PlantModel^ Model);
	virtual ~PMIProblem_Math();

	virtual bool SolveProblemSet() override;

	virtual bool SolveModulesLinearAlgebra();
	virtual bool SolveConstantsLinearAlgebra();
};

public ref class PMIProblem_Mathv2 :
	public PMIProblem_Math
{
public:
	PMIProblem_Mathv2(PlantModel^ Model);
	virtual ~PMIProblem_Mathv2();

	virtual bool SolveProblemSet() override;

	virtual bool SolveLengthLinearAlgebra() override;
};

