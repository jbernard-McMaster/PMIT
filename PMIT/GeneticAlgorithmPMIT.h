#pragma once
#include "PMIProblem.h"

#define _GAPMIT_TRACK_FITNESS_CHANGE_ 1

using namespace System;

public ref class GeneticAlgorithmPMIT :
public GeneticAlgorithmP<Int32>
{
public:
	GeneticAlgorithmPMIT(GeneticAlgorithmConfiguration<int>^ Configuration);
	virtual ~GeneticAlgorithmPMIT();

private:
};

