#pragma once
#include "PMIProblem.h"
#include "PlantModel.h"

ref class PMIProblem_ECC :
	public PMIProblem
{
public:
	PMIProblem_ECC(PlantModel^ Model);
	virtual ~PMIProblem_ECC();

	virtual void Solve() override;
};

