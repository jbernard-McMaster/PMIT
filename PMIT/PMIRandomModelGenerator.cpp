#include "stdafx.h"
#include "PMIRandomModelGenerator.h"


PMIRandomModelGenerator::PMIRandomModelGenerator()
{
	this->moduleValues = gcnew List<String^>;
	this->constantValues = gcnew List<String^>;

	this->moduleValues->Add("A");
	this->moduleValues->Add("B");
	this->moduleValues->Add("C");
	this->moduleValues->Add("D");
	this->moduleValues->Add("E");
	this->moduleValues->Add("G");
	this->moduleValues->Add("H");
	this->moduleValues->Add("I");
	this->moduleValues->Add("J");
	this->moduleValues->Add("K");
	this->moduleValues->Add("L");
	this->moduleValues->Add("M");
	this->moduleValues->Add("N");
	this->moduleValues->Add("O");
	this->moduleValues->Add("P");
	this->moduleValues->Add("Q");
	this->moduleValues->Add("R");
	this->moduleValues->Add("S");
	this->moduleValues->Add("T");
	this->moduleValues->Add("U");
	this->moduleValues->Add("V");
	this->moduleValues->Add("W");
	this->moduleValues->Add("X");
	this->moduleValues->Add("Y");
	this->moduleValues->Add("Z");

	this->moduleValues->Add("a");
	this->moduleValues->Add("b");
	this->moduleValues->Add("c");
	this->moduleValues->Add("d");
	this->moduleValues->Add("e");
	this->moduleValues->Add("g");
	this->moduleValues->Add("h");
	this->moduleValues->Add("i");
	this->moduleValues->Add("j");
	this->moduleValues->Add("k");
	this->moduleValues->Add("l");
	this->moduleValues->Add("m");
	this->moduleValues->Add("n");
	this->moduleValues->Add("o");
	this->moduleValues->Add("p");
	this->moduleValues->Add("q");
	this->moduleValues->Add("r");
	this->moduleValues->Add("s");
	this->moduleValues->Add("t");
	this->moduleValues->Add("u");
	this->moduleValues->Add("v");
	this->moduleValues->Add("w");
	this->moduleValues->Add("x");
	this->moduleValues->Add("y");
	this->moduleValues->Add("z");

	this->moduleValues->Add("1");
	this->moduleValues->Add("2");
	this->moduleValues->Add("3");
	this->moduleValues->Add("4");
	this->moduleValues->Add("5");
	this->moduleValues->Add("6");
	this->moduleValues->Add("7");
	this->moduleValues->Add("8");
	this->moduleValues->Add("9");
	this->moduleValues->Add("0");

	this->moduleValues->Add("#");
	this->moduleValues->Add("@");
	this->moduleValues->Add("%");
	this->moduleValues->Add("(");
	this->moduleValues->Add(")");
	this->moduleValues->Add("-");
	this->moduleValues->Add("_");
	this->moduleValues->Add("=");
	this->moduleValues->Add("{");
	this->moduleValues->Add("}");
	this->moduleValues->Add("'");
	this->moduleValues->Add(":");
	this->moduleValues->Add(";");
	this->moduleValues->Add("<");
	this->moduleValues->Add(">");
	this->moduleValues->Add("~");
	this->moduleValues->Add("`");

	this->moduleValues->Add("♦");
	this->moduleValues->Add("♣");
	this->moduleValues->Add("♣");
	this->moduleValues->Add("¡");
	this->moduleValues->Add("¢");
	this->moduleValues->Add("£");
	this->moduleValues->Add("¤");
	this->moduleValues->Add("¥");
	this->moduleValues->Add("¦");
	this->moduleValues->Add("§");
	this->moduleValues->Add("©");
	this->moduleValues->Add("«");
	this->moduleValues->Add("¬");
	this->moduleValues->Add("®");
	this->moduleValues->Add("¯");
	this->moduleValues->Add("°");
	this->moduleValues->Add("±");
	this->moduleValues->Add("µ");
	this->moduleValues->Add("¶");
	this->moduleValues->Add("·");
	this->moduleValues->Add("¸");
	this->moduleValues->Add("»");
	this->moduleValues->Add("¼");
	this->moduleValues->Add("½");
	this->moduleValues->Add("¾");
	this->moduleValues->Add("¿");
	this->moduleValues->Add("À");

	this->constantValues->Add("F");
	this->constantValues->Add("f");
	this->constantValues->Add("/");
	this->constantValues->Add("\\");
	this->constantValues->Add("^");
	this->constantValues->Add("+");
	this->constantValues->Add("-");
	this->constantValues->Add("[");
	this->constantValues->Add("]");
}

PMIRandomModelGenerator::~PMIRandomModelGenerator()
{
}

Word^ PMIRandomModelGenerator::BuildBranch(bool IsSubBranch)
{
	Word^ result = gcnew Word();
	Int32 length = CommonGlobals::PoolInt(this->lengthBranch->min, this->lengthBranch->max);
	bool hasBranch;
	
	if (IsSubBranch)
	{
		hasBranch = false;
	}
	else
	{
		hasBranch = (CommonGlobals::PoolFloat() <= this->chanceBranchInBranch);
	}

	Symbol^ s = this->PickSymbol(nullptr, true, false);
	result->symbols->Add(s);

	List<Int32>^ branchPositions = gcnew List<Int32>;

	if (hasBranch)
	{
		Int32 numBranch = CommonGlobals::PoolInt(this->numBranchOnBranch->min, this->numBranchOnBranch->max);

		for (size_t iBranch = 0; iBranch < numBranch; iBranch++)
		{
			branchPositions->Add(CommonGlobals::PoolInt(0, length));
		}
	}

	Symbol^ previous = nullptr;
	Int32 countModules = 0;
	Int32 countConstants = 0;
	float currentRatio = 0.0;
	float ratioModule = CommonGlobals::PoolFloat(this->ratioModuleBranch->min, this->ratioModuleBranch->max);

	for (size_t iSymbol = 0; iSymbol < length; iSymbol++)
	{
		for (size_t iBranch = 0; iBranch < branchPositions->Count; iBranch++)
		{
			if (iSymbol == branchPositions[iBranch])
			{
				// go create the branch
				result->symbols->Add(this->model->vocabulary->Symbols[this->model->vocabulary->IndexConstantsStart]);

				result->symbols->AddRange(this->BuildBranch(true)->symbols);

				result->symbols->Add(this->model->vocabulary->Symbols[this->model->vocabulary->IndexConstantsStart + 1]);
			}
		}

		Symbol^ s;
		if (currentRatio > ratioModule)
		{
			s = this->PickSymbol(previous, false, true);
		}
		else
		{
			s = this->PickSymbol(previous, (iSymbol == 0), false);
		}

		if (this->model->vocabulary->IsModule(s))
		{
			countModules++;
		}
		else
		{
			countConstants++;
		}

		currentRatio = (float)countModules / (iSymbol + 1);

		result->symbols->Add(s);
		previous = s;
	}

	return result;
}

PlantModel^ PMIRandomModelGenerator::BuildD0L()
{
	do
	{
		this->model = gcnew PlantModel();

		Int32 numRules = CommonGlobals::PoolInt(this->numModules->min, this->numModules->max);
		Int32 numConstants = CommonGlobals::PoolInt(this->numConstants->min, this->numConstants->max);
		Int32 numConstantOnlyRules = CommonGlobals::PoolInt(this->numConstantOnlyRules->min, this->numConstantOnlyRules->max);
		Int32 countConstantOnlyRules = 0;
		this->model->generations = numRules + 1;

		// Add modules to the vocabulary
		for (size_t iRule = 0; iRule < numRules; iRule++)
		{
			this->model->vocabulary->AddSymbol(this->moduleValues[iRule], true);
		}

		// If constants are unknown add constants to the vocabulary
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ == 0
		for (size_t iSymbol = 0; iSymbol < numConstants; iSymbol++)
		{
			result->vocabulary->AddSymbol(this->constantValues[iSymbol], true);
		}
#endif

		// If needed add branching symbols to the vocabulary
		if (this->branchingAllowed)
		{
			this->model->vocabulary->AddSymbol("[", false);
			this->model->vocabulary->AddSymbol("]", false);
		}

		// If needed add left/right symbols to the vocabulary
		if (this->leftRightAllowed)
		{
			this->model->vocabulary->AddSymbol("+", false);
			this->model->vocabulary->AddSymbol("-", false);
		}

		// If constants are known, add them to the vocabulary

#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0
		for (size_t iSymbol = 0; iSymbol < numConstants; iSymbol++)
		{
			this->model->vocabulary->AddSymbol(this->constantValues[iSymbol], false);
		}
#endif

		this->model->vocabulary->ComputeIndices();

		// Create the successor's for each module
		float dieRoll = 0.0f;

		for (size_t iRule = 0; iRule < this->model->vocabulary->numModules; iRule++)
		{
			ProductionRule^ r = gcnew ProductionRule();
			r->predecessor = this->model->vocabulary->Symbols[iRule];
			r->operation = this->model->vocabulary->YieldsSymbol;

			if (iRule < numRules)
			{
				// determine if this rule has a branch
				bool hasBranch = (CommonGlobals::PoolFloat() <= this->chanceRuleHasBranch);
				bool hasBranchInBranch = (CommonGlobals::PoolFloat() <= this->chanceBranchInBranch);

				// determine rule length
				dieRoll = CommonGlobals::PoolFloat();
				Int32 ruleLengthType;
				if (dieRoll <= this->chanceRuleIsShort)
				{
					ruleLengthType = 0;
				}
				else if (dieRoll <= this->chanceRuleIsShort + this->chanceRuleIsMedium)
				{
					ruleLengthType = 1;
				}
				else if (dieRoll <= this->chanceRuleIsShort + this->chanceRuleIsMedium + this->chanceRuleIsLong)
				{
					ruleLengthType = 2;
				}
				else
				{
					ruleLengthType = 3;
				}

				// Determine if the rule is constant only
				bool constantOnly = false;
				Int32 ruleLength = 0;
				Int32 numBranch = 0;
				float ratioModule = 0.0f;

				switch (ruleLengthType)
				{
				case 0:
					dieRoll = CommonGlobals::PoolFloat();

					if ((countConstantOnlyRules < numConstantOnlyRules) && (dieRoll < this->chanceRuleConstantOnlyShort))
					{
						constantOnly = true;
						countConstantOnlyRules++;
					}

					if (hasBranch)
					{
						numBranch = CommonGlobals::PoolInt(this->numBranchShortRule->min, this->numBranchShortRule->max);
					}

					ruleLength = CommonGlobals::PoolInt(this->lengthShort->min, this->lengthShort->max);
					ratioModule = CommonGlobals::PoolFloat(this->ratioModuleShort->min, this->ratioModuleShort->max);
					break;
				case 1:
					dieRoll = CommonGlobals::PoolFloat();

					if ((countConstantOnlyRules < numConstantOnlyRules) && (dieRoll < this->chanceRuleConstantOnlyMedium))
					{
						constantOnly = true;
						countConstantOnlyRules++;
					}

					if (hasBranch)
					{
						numBranch = CommonGlobals::PoolInt(this->numBranchMediumRule->min, this->numBranchMediumRule->max);
					}

					ruleLength = CommonGlobals::PoolInt(this->lengthMedium->min, this->lengthMedium->max);
					ratioModule = CommonGlobals::PoolFloat(this->ratioModuleMedium->min, this->ratioModuleMedium->max);
					break;
				case 2:
					dieRoll = CommonGlobals::PoolFloat();

					if ((countConstantOnlyRules < numConstantOnlyRules) && (dieRoll < this->chanceRuleConstantOnlyLong))
					{
						constantOnly = true;
						countConstantOnlyRules++;
					}

					if (hasBranch)
					{
						numBranch = CommonGlobals::PoolInt(this->numBranchLongRule->min, this->numBranchLongRule->max);
					}

					ruleLength = CommonGlobals::PoolInt(this->lengthLong->min, this->lengthLong->max);
					ratioModule = CommonGlobals::PoolFloat(this->ratioModuleLong->min, this->ratioModuleLong->max);
					break;
				case 3:
					dieRoll = CommonGlobals::PoolFloat();

					if (dieRoll < this->chanceRuleConstantOnlyVeryLong)
					{
						constantOnly = true;
						countConstantOnlyRules++;
					}

					if (hasBranch)
					{
						numBranch = CommonGlobals::PoolInt(this->numBranchVeryLongRule->min, this->numBranchVeryLongRule->max);
					}

					ruleLength = CommonGlobals::PoolInt(this->lengthVeryLong->min, this->lengthVeryLong->max);
					ratioModule = CommonGlobals::PoolFloat(this->ratioModuleVeryLong->min, this->ratioModuleVeryLong->max);
					break;
				default:
					break;
				}

				List<Int32>^ branchPositions = gcnew List<Int32>;

				if (hasBranch)
				{
					for (size_t iBranch = 0; iBranch < numBranch; iBranch++)
					{
						branchPositions->Add(CommonGlobals::PoolInt(0, ruleLength));
					}
				}

				Symbol^ previous = nullptr;
				Int32 countModules = 0;
				Int32 countConstants = 0;
				float currentRatio = 0.0f;

				for (size_t iSymbol = 0; iSymbol < ruleLength; iSymbol++)
				{
					for (size_t iBranch = 0; iBranch < branchPositions->Count; iBranch++)
					{
						if (iSymbol == branchPositions[iBranch])
						{
							// go create the branch
							r->successor->Add(this->model->vocabulary->Symbols[this->model->vocabulary->IndexConstantsStart]);

							r->successor->AddRange(this->BuildBranch(false)->symbols);

							r->successor->Add(this->model->vocabulary->Symbols[this->model->vocabulary->IndexConstantsStart + 1]);
						}
					}

					Symbol^ s;
					if (currentRatio > ratioModule)
					{
						s = this->PickSymbol(previous, false, true);
					}
					else
					{
						s = this->PickSymbol(previous, (iSymbol == 0), constantOnly);
					}

					if (this->model->vocabulary->IsModule(s))
					{
						countModules++;
					}
					else
					{
						countConstants++;
					}

					currentRatio = (float)countModules / (iSymbol + 1);

					r->successor->Add(s);
					previous = s;
				}
			}
			else
			{// must be an unknown constant
				r->successor->Add(this->model->vocabulary->Symbols[iRule]);
			}

			this->model->rules->Add(r);
		}

		this->model->axiom->Add(this->model->vocabulary->Symbols[0]);

		this->model->Display();
	} while (!this->Verify());

	this->model->Save();

	return this->model;
}

/*
PlantModel^ PMIRandomModelGenerator::BuildD0L_Special(Int32 NumSymbols, Int32 MaxLength)
{
	this->model = gcnew PlantModel();
	Int32 numRules = NumSymbols;
	float tmp = (float)numRules / 2;
	Int32 maxEquations = Math::Ceiling(tmp);
	Int32 numEquations = CommonGlobals::PoolInt(2, maxEquations);
	
	this->model->generations = Math::Min(4, numRules + 1);
	//this->model->generations = numRules + 1;

		// Add modules to the vocabulary
	for (size_t iRule = 0; iRule < numRules; iRule++)
	{
		this->model->vocabulary->AddSymbol(this->moduleValues[iRule], true);
	}

	this->model->vocabulary->ComputeIndices();

	Int32 iEquation = 0;

	for (size_t iRule = 0; iRule < this->model->vocabulary->numModules; iRule++)
	{
		ProductionRule^ r = gcnew ProductionRule();
		r->predecessor = this->model->vocabulary->Symbols[iRule];
		r->operation = this->model->vocabulary->YieldsSymbol;

		for (size_t iSymbol = 0; iSymbol < MaxLength - iEquation; iSymbol++)
		{
			r->successor->Add(this->model->vocabulary->Symbols[iRule]);
		}

		iEquation++;

		if ((iEquation >= numEquations) || (MaxLength - iEquation < 2))
		{
			iEquation = 0;
		}

		this->model->rules->Add(r);
		this->model->axiom->Add(this->model->vocabulary->Symbols[iRule]);
	}


	this->model->Display();

	return this->model;
}
*/

PlantModel^ PMIRandomModelGenerator::BuildD0L_Special(Int32 NumSymbols, Int32 MaxLength)
{
	this->model = gcnew PlantModel();
	Int32 numRules = NumSymbols;

	this->model->generations = Math::Min(6, numRules + 1);

	// Add modules to the vocabulary
	for (size_t iRule = 0; iRule < numRules; iRule++)
	{
		this->model->vocabulary->AddSymbol(this->moduleValues[iRule], true);
	}

	this->model->vocabulary->AddSymbol("F", false);
	this->model->vocabulary->AddSymbol("f", false);
	this->model->vocabulary->AddSymbol("+", false);
	this->model->vocabulary->AddSymbol("-", false);
	this->model->vocabulary->AddSymbol("[", false);
	this->model->vocabulary->AddSymbol("]", false);

	this->model->vocabulary->ComputeIndices();

	Int32 axiomLength = 1;

	for (size_t iPos = 0; iPos < axiomLength; iPos++)
	{
		this->model->axiom->Add(this->model->vocabulary->Symbols[CommonGlobals::PoolInt(1, NumSymbols - 1)]);
	}

	for (size_t iRule = 0; iRule < numRules; iRule++)
	{
		ProductionRule^ r = gcnew ProductionRule();
		r->predecessor = this->model->vocabulary->Symbols[iRule];
		r->operation = this->model->vocabulary->YieldsSymbol;

		Int32 successorLength = CommonGlobals::PoolInt(1, MaxLength);

		bool branchStarted = false;
		bool firstBranchSymbol = false;
		bool onlyConstants = true;
		Int32 consecutiveModules = 0;

		for (size_t iSymbol = 0; iSymbol < successorLength; iSymbol++)
		{
			Int32 idx = 0;
			float dieRoll = CommonGlobals::PoolFloat(0, 1);

			if (dieRoll < 0.70-(0.20*consecutiveModules))
			{// add a module
				onlyConstants = false;
				consecutiveModules++;
				if (iSymbol == 0 && iRule < numRules - 1)
				{
					idx = CommonGlobals::PoolInt(iRule + 1, NumSymbols - 1);
				}
				if (iSymbol == 0 && iRule == numRules - 1)
				{
					idx = 0;
				}
				else
				{
					idx = CommonGlobals::PoolInt(0, NumSymbols - 1);
				}
				r->successor->Add(this->model->vocabulary->Symbols[idx]);
			}
			else
			{// add a constant
				consecutiveModules = 0;
					
				if (branchStarted)
				{
					branchStarted = false;
					r->successor->Add(this->model->vocabulary->Symbols[NumSymbols+5]); // add end branch
				}
				else
				{
					idx = CommonGlobals::PoolInt(0, 4); // pick a constant
					r->successor->Add(this->model->vocabulary->Symbols[NumSymbols + idx]);

					if (this->model->vocabulary->Symbols[NumSymbols + idx]->Value() == "[")
					{// add a direction symbol
						branchStarted = true;
						Int32 idx2 = CommonGlobals::PoolInt(2, 3); // pick a direction
						r->successor->Add(this->model->vocabulary->Symbols[NumSymbols + idx2]);
						Int32 idx3 = CommonGlobals::PoolInt(0, NumSymbols);
						r->successor->Add(this->model->vocabulary->Symbols[idx3]);
					}
				}
			}
		}

		if (branchStarted)
		{// if mid-branch add the end branch symbol
			r->successor->Add(this->model->vocabulary->Symbols[NumSymbols + 5]); // add end branch
		}

		if ((r->successor->Count == 1 && r->successor[0] == this->model->vocabulary->Symbols[iRule]) || (onlyConstants))
		{// a symbol may not produce only itself
			Int32 idx = CommonGlobals::PoolInt(0, NumSymbols - 1);
			r->successor->Add(this->model->vocabulary->Symbols[idx]);
		}

		this->model->rules->Add(r);
	}

	this->model->Display();

	return this->model;
}

void PMIRandomModelGenerator::Load()
{
	// controls the number of modules and constants in the problem
	this->numModules = gcnew MinMaxI();
	this->numConstants = gcnew MinMaxI();
	this->numConstantOnlyRules = gcnew MinMaxI();
	this->numModules->min = 10;
	this->numModules->max = 20;
	this->numConstants->min = 1;
	this->numConstants->max = 8;
	// controls the number of rules that are composed only of constants
	this->numConstantOnlyRules->min = 1;
	this->numConstantOnlyRules->max = 6;

	// controls the chance for each type of length
	this->chanceRuleIsShort = 0.70f;
	this->chanceRuleIsMedium = 0.25f;
	this->chanceRuleIsLong = 0.4f;
	this->chanceRuleIsVeryLong = 0.1f;

	// controls the length of each type of rule
	this->lengthShort = gcnew MinMaxI();
	this->lengthMedium = gcnew MinMaxI();
	this->lengthLong = gcnew MinMaxI();
	this->lengthVeryLong = gcnew MinMaxI();
	this->lengthBranch= gcnew MinMaxI();
	this->lengthBranchOnBranch = gcnew MinMaxI();
	this->lengthShort->min = 2;
	this->lengthShort->max = 5;
	this->lengthMedium->min = 4;
	this->lengthMedium->max = 7;
	this->lengthLong->min = 7;
	this->lengthLong->max = 12;
	this->lengthVeryLong->min = 10;
	this->lengthVeryLong->max = 40;
	// controls the length of branches and branches on branches
	this->lengthBranch->min = 1;
	this->lengthBranch->max = 3;
	this->lengthBranchOnBranch->min = 1;
	this->lengthBranchOnBranch->max = 3;

	// controls the ratio of modules to constants for each rule length type
	this->ratioModuleShort = gcnew MinMaxF();
	this->ratioModuleMedium = gcnew MinMaxF();
	this->ratioModuleLong = gcnew MinMaxF();
	this->ratioModuleVeryLong = gcnew MinMaxF();
	this->ratioModuleBranch= gcnew MinMaxF();
	this->ratioModuleShort->min = 0.50f;
	this->ratioModuleShort->max = 1.00f;
	this->ratioModuleMedium->min = 0.00f;
	this->ratioModuleMedium->max = 0.25f;
	this->ratioModuleLong->min = 0.00f;
	this->ratioModuleLong->max = 0.05f;
	this->ratioModuleVeryLong->min = 0.00f;
	this->ratioModuleVeryLong->max = 0.00f;
	this->ratioModuleBranch->min = 0.00f;
	this->ratioModuleBranch->max = 0.25f;

	// controls the chance that a rule will be made of constants
	// Note, branches can have modules even in constant only rules
	// TODO: add a chance for branches to be constant only?
	this->chanceRuleConstantOnlyShort = 0.25f;
	this->chanceRuleConstantOnlyMedium = 0.35f;
	this->chanceRuleConstantOnlyLong = 0.60f;
	this->chanceRuleConstantOnlyVeryLong = 1.00f;

	// is branching allowed at all?
	this->branchingAllowed = true;
	// are left and right indicators permitted at all?
	this->leftRightAllowed = true;

	// what are the odds of a rule containing branches and whether a branch contains a branch
	// does nothing if branching is off
	this->chanceRuleHasBranch = 0.50f;
	this->chanceBranchInBranch = 0.0f;

	// controls the number of branches that can appear on a rule of different lengths
	this->numBranchShortRule = gcnew MinMaxI();
	this->numBranchMediumRule = gcnew MinMaxI();
	this->numBranchLongRule = gcnew MinMaxI();
	this->numBranchVeryLongRule = gcnew MinMaxI();
	this->numBranchOnBranch = gcnew MinMaxI();
	this->numBranchShortRule->min = 1;
	this->numBranchShortRule->max = 1;
	this->numBranchMediumRule->min = 1;
	this->numBranchMediumRule->max = 2;
	this->numBranchLongRule->min = 1;
	this->numBranchLongRule->max = 4;
	this->numBranchVeryLongRule->min = 3;
	this->numBranchVeryLongRule->max = 9;
	this->numBranchOnBranch->min = 1;
	this->numBranchOnBranch->max = 3;

	// to keep the rules from looking too "random"
	// try to make constants follow modules a bit
	// try to make the same constant follow a constant a bit
	this->chanceModuleNeighbourIsConstant = 0.75f;
	this->chanceConstantNeighbourIsConstant = 0.50f;
	this->chanceConstantNeighbourIsSameConstant = 0.33f;
}

Symbol^ PMIRandomModelGenerator::PickConstant(Symbol^ Previous, bool LeadSymbol)
{
	Symbol^ result;
	Int32 idx;
	float dieRoll;
	Int32 offset = 0;

	if (this->branchingAllowed)
	{// do not allow a branching symbol to be picked
		offset = 2;
	}

	if (LeadSymbol)
	{
		idx = CommonGlobals::PoolInt(this->model->vocabulary->IndexConstantsStart+offset, this->model->vocabulary->IndexConstantsEnd);
		result = this->model->vocabulary->Symbols[idx];
	}
	else
	{
		dieRoll = CommonGlobals::PoolFloat();

		if (dieRoll < this->chanceConstantNeighbourIsSameConstant)
		{
			result = Previous;
		}
		else
		{
			idx = CommonGlobals::PoolInt(this->model->vocabulary->IndexConstantsStart + offset, this->model->vocabulary->IndexConstantsEnd);
			result = this->model->vocabulary->Symbols[idx];
		}
	}

	return result;
}

Symbol^ PMIRandomModelGenerator::PickModule()
{
	Int32 offset = CommonGlobals::PoolInt(this->model->vocabulary->IndexModulesStart, this->model->vocabulary->IndexModulesEnd);
	
	return this->model->vocabulary->Symbols[offset];
}

Symbol^ PMIRandomModelGenerator::PickSymbol(Symbol^ Previous, bool LeadSymbol, bool ConstantOnly)
{
	Symbol^ result;
	float dieRoll;

	if (ConstantOnly)
	{
		result = this->PickConstant(Previous, LeadSymbol);
	}
	else if (LeadSymbol)
	{
		dieRoll = CommonGlobals::PoolFloat();
		if (dieRoll < 0.5)
		{
			result = this->PickModule();
		}
		else
		{
			result = this->PickConstant(Previous, LeadSymbol);
		}
	}
	else
	{
		if (this->model->vocabulary->IsModule(Previous))
		{
			dieRoll = CommonGlobals::PoolFloat();
			if (dieRoll < this->chanceModuleNeighbourIsConstant)
			{
				result = this->PickConstant(Previous, LeadSymbol);
			}
			else
			{
				result = this->PickModule();
			}
		}
		else
		{
			dieRoll = CommonGlobals::PoolFloat();
			if (dieRoll < this->chanceConstantNeighbourIsConstant)
			{
				result = this->PickConstant(Previous, LeadSymbol);
			}
			else
			{
				result = this->PickModule();
			}
		}
	}

	return result;
}

bool PMIRandomModelGenerator::Verify()
{
	bool result = false;

	// Check to make sure every symbol is produced at least once
	List<Symbol^>^ produced = gcnew List<Symbol^>;

	for (size_t iSymbol = 0; iSymbol < this->model->axiom->Count; iSymbol++)
	{
		produced->Add(this->model->axiom[iSymbol]);
	}

	bool unchanged = false;
	while ((!unchanged) && (produced->Count < this->model->vocabulary->NumSymbols()))
	{
		unchanged = true;
		for (size_t iRule = 0; iRule < this->model->rules->Count; iRule++)
		{
			if (produced->Contains(this->model->rules[iRule]->predecessor))
			{
				for (size_t iSymbol = 0; iSymbol < this->model->rules[iRule]->successor->Count; iSymbol++)
				{
					if (!produced->Contains(this->model->rules[iRule]->successor[iSymbol]))
					{
						produced->Add(this->model->rules[iRule]->successor[iSymbol]);
						unchanged = false;
					}
				}
			}
		}
	}

	if (produced->Count == this->model->vocabulary->NumSymbols())
	{
		Console::WriteLine("Production verification: Success");
		result = true;
	}
	else
	{
		Console::WriteLine("Production verification: Failed.");
	}
	

	// Otherwise, if the non-produced symbol is a constant remove it

	// If it is a module the remove the symbol and the rule


	// Check to make sure the production is not degenerate
	this->model->CreateEvidence();

	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		if (this->model->evidence[iGen]->symbols->Count > ProductionRule::cMaxLength)
		{
			Console::WriteLine("Evidence verification: Failed");
			result = false;
		}
	}

	return result;
}