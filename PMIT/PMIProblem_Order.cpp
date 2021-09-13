#include "stdafx.h"
#include "PMIProblem_Order.h"

PMIProblem_Order::PMIProblem_Order(PlantModel^ Model) : PMIProblem(Model)
{
}

PMIProblem_Order::~PMIProblem_Order()
{
}

GenomeConfiguration<Int32>^ PMIProblem_Order::CreateGenome()
{
	// the number of genes needed is equal to the number of rules (i.e. # of modules) times the number of symbols being solved for, e.g the number of modules or the number of constants
	// However, this is equivalent to the size of the min/max # of occurances per rule matrix

	Int32 numGenes = (this->maxPoG->GetUpperBound(0) + 1) * (this->maxPoG->GetUpperBound(1) + 1);

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	for (size_t i = 0; i < min->Length; i++)
	{
		min[i] = 0;
		max[i] = 99;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}
