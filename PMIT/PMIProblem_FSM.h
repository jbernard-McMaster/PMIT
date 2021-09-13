#pragma once
#include "PMIProblem.h"
#include "ProblemState.h"

public ref class PMIProblem_FSM :
	public PMIProblem
{
public:
	PMIProblem_FSM(PlantModel^ Model);
	virtual ~PMIProblem_FSM();

	IProblemState^ current;

	virtual bool ShouldTerminate();
	virtual void Solve() override;

	enum class Actions
	{
		GetModuleSolutions,
		GetConstantSolutions,
		OrderCandidateSolution,
		

		Solved
	};
};

