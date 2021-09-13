#pragma once
#include "LSIProblemV3.h"

public ref class EDCProblemV3 :
	public LSIProblemV3
{
public:
	EDCProblemV3(ModelV3^ M, ProblemType T);
	virtual ~EDCProblemV3();

	void Initialize() override;

	virtual void Analyze() override;

	virtual float Assess(array<Int32>^ RuleLengths) override;
	virtual float AssessProductionRules(List<ProductionRuleV3^>^ Rules) override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

	virtual List<ProductionRuleV3^>^ CreateRulesByScanning(array<Int32>^ RuleLengths) override;
	virtual List<ProductionRuleV3^>^ CreateRulesByScenario(array<List<SuccessorCount^>^>^ Successors, array<Int32>^ Scenario);
	virtual array<List<SuccessorCount^>^>^ CreateSuccessorsByScanning(array<Int32>^ RuleLengths);

	virtual void Solve() override;

	float Pr;
	float Pi;
	float Pd;
	bool replacementAllowed;
	bool deletionAllowed;
	bool insertionAllowed;

	array<List<SuccessorCount^>^>^ successors;
};

