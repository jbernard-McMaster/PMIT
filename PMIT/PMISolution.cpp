#include "stdafx.h"
#include "PMISolution.h"


PMISolution::PMISolution(array<Int32>^ Solution, float Fitness)
{
	this->fitness = Fitness;
	this->solution = Solution;
}


PMISolution::~PMISolution()
{
}
