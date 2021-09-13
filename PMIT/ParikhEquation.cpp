#include "stdafx.h"
#include "ParikhEquation.h"

RPTCoordinate::RPTCoordinate(Int32 Start, Int32 End, Int32 Rule, Int32 Symbol, Int32 Multiplier)
{
	this->startGeneration = Start;
	this->endGeneration = End;
	this->rule = Rule;
	this->symbol = Symbol;
	this->multiplier = Multiplier;
}

RPTCoordinate::~RPTCoordinate()
{
}

void RPTCoordinate::Display()
{
	Console::WriteLine("R=" + this->rule + ", S=" + this->symbol + ", M=" + this->multiplier + ", V=" + this->value);
}

ParikhEquation::ParikhEquation()
{
	this->value = 0;
	this->variables = gcnew List<RPTCoordinate^>;
	this->metaIndex = 0;
}

ParikhEquation::~ParikhEquation()
{
}

void ParikhEquation::Display()
{
	Console::WriteLine("Variables");
	for (size_t iVariable = 0; iVariable < this->variables->Count; iVariable++)
	{
		this->variables[iVariable]->Display();
	}
}

void ParikhEquation::FindAllSolutions()
{
	this->solutions = gcnew List<List<Int32>^>;

	bool success = false;
	bool increment = false;
	Int32 index = 0;
	Console::WriteLine();

	do
	{
		// TODO: this is a really bad solution
		if (increment)
		{
			this->variables[index]->value++;
		}
		else
		{
			increment = true;
		}

		if (this->variables[index]->value > this->value)
		{
			this->variables[index]->value = 0;
			index++;
		}
		else
		{
			success = true;
			index = 0;
			Int32 solutionValue = 0;
			List<Int32>^ s = gcnew List<Int32>;
			for (size_t iVariable = 0; iVariable < this->variables->Count; iVariable++)
			{
				Console::Write(this->variables[iVariable]->value + " ");
				s->Add(this->variables[iVariable]->value);
				solutionValue += this->variables[iVariable]->multiplier * this->variables[iVariable]->value;
			}
			Console::WriteLine();

			if (solutionValue == this->value)
			{
				this->solutions->Add(s);
			}
		}
	} while (index < this->variables->Count);
}

bool ParikhEquation::FindSolution()
{
	bool result = true;
	Int32 solutionValue;
	do
	{
		solutionValue = 0;
		this->variables[metaIndex]->value++;
		if (this->variables[metaIndex]->value > this->value)
		{
			this->variables[metaIndex]->value = 0;
			this->metaIndex++;
			if (this->metaIndex >= this->variables->Count)
			{
				result = false;
			}
		}

		for (size_t iVariable = 0; iVariable < this->variables->Count; iVariable++)
		{
			solutionValue += this->variables[iVariable]->value;
			Console::Write(this->variables[iVariable]->value + " ");
		}

		Console::WriteLine();
	} while ((solutionValue != value) && (result));

	return result;
}

void ParikhEquation::Reset()
{
	this->indices = gcnew List<Int32>;

	this->metaIndex = 0;
	for (size_t iVariable = 0; iVariable < this->variables->Count; iVariable++)
	{
		this->indices->Add(0);
	}
}
