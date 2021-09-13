#pragma once
#include "PlantModel.h"
#include "ProductionRuleStochastic.h"

public ref class PlantModelStochastic :
	public PlantModel
{
public:
	PlantModelStochastic();
	virtual ~PlantModelStochastic();

	List<ProductionRuleStochastic^>^ rules;

	virtual void CreateEvidence() override;

	bool IsMatch(PlantModel^ Model) override;

	virtual void LoadRule(array<String^, 1>^ Words) override;

};

