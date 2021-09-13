#pragma once
#include "PMIProblemV2.h"

public ref class PMIProblem_Search :
	public PMIProblemV2
{
public:
	PMIProblem_Search(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_Search();

	virtual bool SolveProblemSet() override;
};

