#pragma once

using namespace System;

public ref class PMISolution
{
public:
	PMISolution(array<Int32>^ Solution, float Fitness);
	virtual ~PMISolution();

	float fitness;
	array<Int32>^ solution;
};

