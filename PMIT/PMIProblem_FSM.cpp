#include "stdafx.h"
#include "PMIProblem_FSM.h"


PMIProblem_FSM::PMIProblem_FSM(PlantModel^ Model) : PMIProblem(Model)
{
}


PMIProblem_FSM::~PMIProblem_FSM()
{
}


void PMIProblem_FSM::Solve()
{
	// Process flow for solving a problem subset
	// Phase 1
	// Step 1 - Find solutions to the module sub-problem
	// Step 2 - For each solution with zero fitness, find solutions for constants
	// Step 3 - For a set of module and constants solutions, try to order them
	// Step 4 - If the solution won't order properly, then add it to the tabu list
	// Step 5 - If no zero fitness solution works, repeat steps 2-4 until no further zero fitness solutions are found
	// Step 6 - Take top non-zero fitness solution, and create sub-problems


	while (!this->ShouldTerminate())
	{
		this->current = this->current->Process();
	}
}

bool PMIProblem_FSM::ShouldTerminate()
{
	return this->current->IsTerminal;
}