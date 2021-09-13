#pragma once
#include "PMIProcessor.h"
#include "PMIProblemV1E_D_v1.h"
#include "PMIProblemV1E_D_v2.h"
#include "PMIProblemV2A_D_v3.h"
#include "PMIProblemV2A_D_v4.h"
#include "PMIProblemV2A_L_v2.h"
#include "PMIProblemV2A_Lex_v1.h"
#include "PMIProblemV2A_Lex_v1b.h"
#include "PMIProblemV2A_ML.h"
#include "PMIProblemV2A_PoG_v1.h"
#include "PMIProblemV2A_PoG_v1b.h"
#include "PMIProblemV2A_PoG_v2.h"
#include "PMIProblemV2A_PoL_v1.h"
#include "PMIRandomModelGenerator.h"

#include "PMIFitnessFunction_Parikh.h"
#include "PMIFitnessFunction_Positional.h"

public ref class PMIProcessor_V2A :
	public PMIProcessor
{
public:
	PMIProcessor_V2A();
	virtual ~PMIProcessor_V2A();

	enum class FitnessFunctionChoices
	{
		Positional,
		Parikh,
		RuleConsistency,
		RuleSymbolConsistency,
		Assembled
	};

	FitnessFunctionChoices fitnessFunctionChoice;

	IPMIFitnessFunction^ fitnessFunction;

	PMIProblemV2A^ problem;

	virtual bool AlgorithmMenu() override;
	virtual bool FitnessFunctionMenu();

	virtual void Execute(Int32 iExecution) override;
};

