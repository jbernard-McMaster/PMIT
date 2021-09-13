#pragma once
#include "PMIProblem.h"

ref class PMIProblem_Order :
	public PMIProblem
{
public:

	PMIProblem_Order(PlantModel^ Model);
	virtual ~PMIProblem_Order();

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

};

