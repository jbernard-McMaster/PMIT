#pragma once
#include "PMIProblemV2A_Rule.h"

public ref class PMIProblemV2A_D_v4 :
	public PMIProblemV2A_Rule
{
public:
	PMIProblemV2A_D_v4(PlantModel^ Model, float ErrorRate, String^ FileName);
	virtual ~PMIProblemV2A_D_v4();

	array<Int32>^ totals;
	array<Int32>^ totalsInverted;
	Symbol^ blank;
	List<Int32>^ ruleOptions;

	virtual bool AnalyzeSolution(PMISolution^ s) override;

	virtual float AssessRuleLengths(array<Int32>^ Solution, bool ForceRuleLength) override;
	virtual List<ProductionRule^>^ Assess_CreateRulesStep(array<Int32>^ Solution) override;

	virtual void ComputeFragmentsFromPartialSolution() override;
	virtual UInt64 ComputeSolutionSpaceSize() override;

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;

	virtual Int32 DecodeRule(Int32 Ticket);

	virtual Symbol^ PickConstant(Int32 Index, Int32 Ticket, bool BlankAllowed);
	virtual Symbol^ PickSymbol(Int32 Index, Int32 Ticket, bool BlankAllowed);
	virtual Symbol^ PickAnySymbol(Int32 Ticket, bool BlankAllowed);

	virtual Symbol^ PickConstantInverted(Int32 Index, Int32 Ticket, bool BlankAllowed);
	virtual Symbol^ PickSymbolInverted(Int32 Index, Int32 Ticket, bool BlankAllowed);
	virtual Symbol^ PickAnySymbolInverted(Int32 Ticket, bool BlankAllowed);

	virtual void RollbackSpecial() override;

};

