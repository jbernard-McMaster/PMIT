#pragma once

using namespace System;
using namespace System::Collections::Generic;

public ref class RPTCoordinate 
{
public:
	RPTCoordinate(Int32 Start, Int32 End, Int32 Rule, Int32 Symbol, Int32 Multiplier);
	virtual ~RPTCoordinate();

	Int32 startGeneration;
	Int32 endGeneration;
	Int32 rule;
	Int32 symbol;
	Int32 multiplier;
	Int32 value;

	void Display();
};

public ref class ParikhEquation
{
public:
	ParikhEquation();
	virtual ~ParikhEquation();

	float value;
	List<RPTCoordinate^>^ variables;

	Int32 metaIndex;
	List<Int32>^ indices;
	List<List<Int32>^>^ solutions;

	void Display();
	void FindAllSolutions();
	bool FindSolution();
	void Reset();
};

