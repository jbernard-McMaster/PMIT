#include "stdafx.h"
#include "GeneticAlgorithmPMIT.h"


GeneticAlgorithmPMIT::GeneticAlgorithmPMIT(GeneticAlgorithmConfiguration<int>^ Configuration) : GeneticAlgorithmP<int>(Configuration)
{
}

GeneticAlgorithmPMIT::~GeneticAlgorithmPMIT()
{
}

//void GeneticAlgorithmPMIT::Survival()
//{
//	for (size_t i = 0; i < this->samples->Length; i++)
//	{
//		this->fitnessSamples[i] = this->_problem->Assess(this->samples[i]);
//	}
//
//	this->survivalOperator->Process(this->population, this->samples, this->fitnessPop, this->fitnessSamples);
//	this->state->fitnessBest = this->fitnessPop[0];
//
//#if _GAPMIT_TRACK_FITNESS_CHANGE_ > 0
//	float fitnessSum = 0.0;
//	for (size_t i = 0; i < this->fitnessPop->Length; i++)
//	{
//		fitnessSum += this->fitnessPop[i];
//	}
//
//	if (fitnessSum != this->fitnessSumLast)
//	{
//		this->state->fitnessChanged = true;
//		this->fitnessSumLast = fitnessSum;
//	}
//	else
//	{
//		this->state->fitnessChanged = false;
//	}
//#endif
//}
//
//void GeneticAlgorithmPMIT::InitializePopulation()
//{
//	this->population = gcnew array<array<int>^>(this->configuration->populationSizeInit);
//	this->fitnessPop = gcnew array<float>(this->configuration->populationSizeInit);
//
//	for (int i = 0; i < this->configuration->populationSizeInit; i++)
//	{
//		this->population[i] = this->factory->BuildP();
//		this->fitnessPop[i] = this->_problem->Assess(this->population[i]);
//	}
//
//	Array::Sort(this->fitnessPop, this->population, this->comparer);
//	this->state->fitnessBest = this->fitnessPop[0];
//}

