#include "stdafx.h"
#include "BruteForceSearchR_Dbl_PMIT.h"


BruteForceSearchR_Dbl_PMIT::BruteForceSearchR_Dbl_PMIT(BruteForceSearchConfiguration^ Configuration) : BruteForceSearchR_Dbl(Configuration)
{
	this->skipValues = gcnew array<Int32>(Configuration->searchSpace->GetUpperBound(0)+1);
	for (size_t i = 0; i <= this->skipValues->GetUpperBound(0); i++)
	{
		this->skipValues[i] = -1;
	}
}

BruteForceSearchR_Dbl_PMIT::~BruteForceSearchR_Dbl_PMIT()
{
}

bool BruteForceSearchR_Dbl_PMIT::CheckDelayedSkip(Int32 Index)
{
	bool result = false;
	if (this->state->current[Index] >= this->skipValues[Index] && this->skipValues[Index] != -1)
	{
		result = true;
		for (size_t i = Index; i <= this->skipValues->GetUpperBound(0); i++)
		{
			this->skipValues[i] = -1;
		}

		this->state->current[Index] = this->searchSpace[Index]->min;
		this->state->index = Index - 1;
	}

	return result;
}

void BruteForceSearchR_Dbl_PMIT::SetDelayedSkip(Int32 Index, Int32 Value)
{
	if (this->skipValues[Index] == -1)
	{
		this->skipValues[Index] = Value;
	}
}

void BruteForceSearchR_Dbl_PMIT::Override(Int32 Index, Int32 Value)
{
	this->state->current[Index] = Math::Min(Value, this->searchSpace[Index]->max);
	//Console::WriteLine("State[" + Index + "] set to " + Value);
}

array<Int32>^ BruteForceSearchR_Dbl_PMIT::Solve()
{
	float fitness = 0;
	float lowest;
	this->flagSkip = false;
	this->state->index = this->state->current->GetUpperBound(0);

	if (this->preferHighFitness)
	{
		lowest = float::MinValue;
	}
	else {
		lowest = float::MaxValue;
	}

	array<Int32>^ result = gcnew array<Int32>(this->configuration->searchSpace->GetUpperBound(0) + 1);

	fitness = this->problem->Assess_Dbl(this->state->current);

	if ((!this->preferHighFitness && fitness < lowest) || (this->preferHighFitness && fitness > lowest))
	{
		lowest = fitness;
		for (size_t i = 0; i <= result->GetUpperBound(0); i++)
		{
			result[i] = this->state->current[i];
		}
	}

	//while (((fitness > this->fitnessTarget && !this->preferHighFitness) || (fitness < this->fitnessTarget && !this->preferHighFitness)) && (this->state->index ))

	while (((fitness > this->fitnessTarget && !this->preferHighFitness) || (fitness < this->fitnessTarget && this->preferHighFitness)) && (this->state->index >= 0))
	{
		bool stop = true;

		do
		{
			if (this->flagOverride)
			{
				this->flagOverride = false;
				this->state->current[this->indexSkip] = this->valueSkip;

				for (size_t i = this->indexSkip + 1; i <= this->state->current->GetUpperBound(0); i++)
				{
					this->state->current[i] = this->searchSpace[i]->min;
				}

				if (this->indexSkip % 2 == 0 && this->state->current[this->indexSkip] > 1 && this->state->current[this->indexSkip + 1] == -1)
				{
					this->state->current[this->indexSkip + 1] = 0;
				}

				if (!this->CheckDelayedSkip(this->indexSkip))
				{
					if (this->state->current[this->indexSkip] > this->searchSpace[this->indexSkip]->max)
					{
						this->state->current[this->indexSkip] = this->searchSpace[this->indexSkip]->min;
						this->state->index = this->indexSkip - 1;
						stop = false;
					}
					else
					{
						this->state->index = this->state->current->GetUpperBound(0);
						stop = true;
					}
				}
			}
			else if (this->flagSkip)
			{
				this->flagSkip = false;
				this->state->current[this->indexSkip]++;

				if (!this->CheckDelayedSkip(this->indexSkip))
				{
					if (this->state->current[this->indexSkip] > this->searchSpace[this->indexSkip]->max)
					{
						this->state->current[this->indexSkip] = this->searchSpace[this->indexSkip]->min;
						this->state->index = this->indexSkip - 1;
						stop = false;
					}
					else
					{
						this->state->index = this->state->current->GetUpperBound(0);
						stop = true;
					}
				}
			}
			else
			{
				this->state->current[this->state->index]++;

				if (this->indexSkip % 2 == 0 && this->state->current[this->state->index] > 1 && this->state->current[this->indexSkip + 1] == -1)
				{
					this->state->current[this->state->index + 1] = 0;
				}

				if (!this->CheckDelayedSkip(this->state->index))
				{
					if (this->state->current[this->state->index] > this->searchSpace[this->state->index]->max)
					{
						this->state->current[this->state->index] = this->searchSpace[this->state->index]->min;
						this->state->index--;
						stop = false;
					}
					else
					{
						this->state->index = this->state->current->GetUpperBound(0);
						stop = true;
					}
				}
				else
				{
					if (this->state->current[this->state->index] > this->searchSpace[this->state->index]->max)
					{
						this->state->current[this->state->index] = this->searchSpace[this->state->index]->min;
						this->state->index--;
						stop = false;
					}
				}
			}

			if (this->state->index >= 0 && this->state->index + 1 <= this->state->current->GetUpperBound(0) && this->state->current[this->state->index] > 1 && this->state->current[this->state->index + 1] == -1)
			{
				this->state->current[this->state->index + 1] = 0;
			}
		} while ((!stop) && (this->state->index >= 0));

		//for (size_t i = 0; i < this->state->current->GetUpperBound(0); i++)
		//{
		//	Console::Write(this->state->current[i] + ",");
		//}
		//Console::WriteLine();

		fitness = this->problem->Assess_Dbl(this->state->current);
		if ((!this->preferHighFitness && fitness < lowest) || (this->preferHighFitness && fitness > lowest))
		{
			lowest = fitness;
			for (size_t i = 0; i <= result->GetUpperBound(0); i++)
			{
				result[i] = this->state->current[i];
			}
		}
	}

	return result;
}

