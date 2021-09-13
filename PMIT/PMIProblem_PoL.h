#pragma once
#include "PMIProblem.h"
#include "PMIProblem_Lengthv1.h"

public ref class PMIProblem_PoL :
	public PMIProblem_LengthV1
{
public:
	PMIProblem_PoL(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblem_PoL();

	virtual array<Int32>^ Assess_CreateRuleLengths(array<Int32>^ Solution);

	virtual float Assess_Length(array<Int32>^ Solution) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

	virtual bool SolveLengthSearch() override;


};

