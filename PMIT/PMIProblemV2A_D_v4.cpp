#include "stdafx.h"
#include "PMIProblemV2A_D_v4.h"


PMIProblemV2A_D_v4::PMIProblemV2A_D_v4(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblemV2A_Rule(Model, ErrorRate, FileName)
{
	this->modifierGenerations = 4;
	this->produceFragments = false;
}


PMIProblemV2A_D_v4::~PMIProblemV2A_D_v4()
{
}

//float PMIProblemV2A_D_v4::Assess(array<Int32>^ Solution)
//{
//	if (Solution != nullptr)
//	{
//		return this->AssessRuleLengths(this->Assess_CreateRuleLengths(Solution));
//	}
//	else // this gets called when there is no solution to assess but rather when all the rule lengths are known
//	{
//		List<ProductionRule^>^ rules = this->Assess_CreateRulesStep(Solution);
//		return this->AssessProductionRules(rules);
//	}
//}

bool PMIProblemV2A_D_v4::AnalyzeSolution(PMISolution^ s)
{
	bool result = false;
	List<ProductionRule^>^ rules = this->Assess_CreateRulesStep(s->solution);
	if (s->fitness == 0)
	{
		//this->OutputRules(rules);
		//if (this->VerifyOrderScan(rules))
		bool verified = true;

		if (verified)
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
			Console::WriteLine("Verification: Success");
#endif
			this->currentTabu = gcnew TabuItem(this->constantIndex, rules);
			this->CapturePartialSolution_Rules(rules);
			result = true;
		}
		else
		{
#if _PMI_PROBLEM_DISPLAY_ANALYSIS_ >= 1
			Console::WriteLine("Verification: Failure");
#endif
		}
	}
	else
	{// if there is no solution than the previous solution is no good and should be marked tabu
	 //this->AddTabuItem(this->currentTabu);
		if (this->currentTabu->index >= 0)
		{
			this->RollbackAnalysisForTabu(this->currentTabu);
		}
	}

	if (!result)
	{
		TabuItem^ t = gcnew TabuItem(this->constantIndex, rules);
		t->ruleLengths = s->solution;
		this->AddTabuItem(t);
	}

#if _PMI_PROBLEM_DISPLAY_RULES_ > 0
	this->OutputRules(rules);
#endif // _PMI_PROBLEM_DISPLAY_RULES_ > 0

	return result;
}

float PMIProblemV2A_D_v4::AssessRuleLengths(array<Int32>^ RuleLengths, bool ForceRuleLength)
{
	float fitness = 0.0;
	bool tabu = false;

#if _PMI_PROBLEM_CHEAT_ > 0
	RuleLengths = this->Cheat(RuleLengths);
#endif

	List<ProductionRule^>^ modelRules;

	if ((ForceRuleLength && !this->IsTabu(this->constantIndex, RuleLengths)) || (!ForceRuleLength))
	{
		// Sometimes need to force this to treat the incoming values as absolute rule lengths
		// For example, when assess branching symbols
		// Otherwise need to create the step using whatever custom method exists for creating them
		// The function "Assess_CreateRulesStep" originally only accepted ruleLengths but really this should be a solution
		if (ForceRuleLength)
		{
			modelRules = this->Assess_CreateRulesStep_RuleLengths(RuleLengths);
		}
		else
		{
			modelRules = this->Assess_CreateRulesStep(RuleLengths);
			tabu = this->IsTabu(this->constantIndex, modelRules);
		}


		if ((modelRules != nullptr) && (!tabu))
		{
#if _PMI_PROBLEM_VERBOSE_ >= 1
			Console::WriteLine(" Rules: ");
			for each (ProductionRule^ r in modelRules)
			{
				Console::WriteLine("   " + r->ToString());
			}
			Console::WriteLine();
#endif 
			fitness = this->AssessProductionRules(modelRules);
		}
		else
		{
			fitness = this->currentSet->endGeneration - this->startGeneration + 1;
		}
	}
	else
	{
		fitness = this->currentSet->endGeneration - this->startGeneration + 1;
	}

	return fitness;
}

List<ProductionRule^>^ PMIProblemV2A_D_v4::Assess_CreateRulesStep(array<Int32>^ Solution)
{
	List<ProductionRule^>^ rules = gcnew List<ProductionRule^>;
	List<Int32>^ insertPoint = gcnew List<Int32>;
	List<bool>^ buildInverted = gcnew List<bool>;
	List<bool>^ buildFragment = gcnew List<bool>;

	Int32 numRules = 0;

	// Make a set of rules with blank successors
	for (size_t iRule = this->vocabulary->IndexModulesStart; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Symbol^ p = this->vocabulary->Symbols[iRule];
		List<Symbol^>^ s = gcnew List<Symbol^>;
		ProductionRule^ r = gcnew ProductionRule(p, this->vocabulary->YieldsSymbol, s);

		buildInverted->Add(false);
		buildFragment->Add(false);

		if (this->IsModuleInSet(iRule))
		{
			bool addHead = true;
			bool addTail = true;

			if ((this->ruleFragments[iRule]->symbols->Count > 0) &&
				((this->ruleFragments[iRule]->symbols->Count >= (this->ruleHead[iRule]->symbols->Count + this->ruleTail[iRule]->symbols->Count)) ||
				(Math::Abs(this->maxRuleLengths[iRule] - this->ruleFragments[iRule]->symbols->Count) <= Math::Abs(this->maxRuleLengths[iRule] - this->ruleHead[iRule]->symbols->Count - this->ruleTail[iRule]->symbols->Count)) ||
					(this->maxRuleLengths[iRule] == this->ruleFragments[iRule]->symbols->Count)
					)
				)
			{
				addHead = false;
				addTail = false;
				//Console::WriteLine("Has Fragment");

				r->successor->AddRange(this->ruleFragments[iRule]->symbols);
				buildFragment[iRule] = true;
			}

			if (addHead)
			{// add the head
				r->successor->AddRange(this->ruleHead[iRule]->symbols);
			}

			insertPoint->Add(r->successor->Count);

			// TODO: if the second condition is false then it must be an overlapping head/tail and so the rule is complete
			if ((addTail) && ((r->successor->Count + this->ruleTail[iRule]->symbols->Count < this->maxRuleLengths[iRule])))
			{// add the tail
				r->successor->AddRange(this->ruleTail[iRule]->symbols);
			}

			if ((this->ruleHead[iRule]->Count == 0) && (this->ruleTail[iRule]->Count > 0))
			{
				buildInverted[iRule] = true;
			}

			//TODO: I think this is flawed, sometimes it is helpful like in problem 16
			// Other times it is NOT helpful like problem 23
			// 
			if ((this->constantIndex > 0) && (this->knownRuleLengthsIndexed[this->constantIndex, iRule] > 0))
			{
				for (size_t iSymbol = r->successor->Count; iSymbol < this->knownRuleLengthsIndexed[this->constantIndex, iRule]; iSymbol++)
				{
					r->successor->Add(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]);
				}
			}

			// if the successor is empty but the minimum is known, then it must consist of these symbols
			if ((this->constantIndex > 0) && (this->minRuleLengths[iRule] > 0) && (r->successor->Count == 0))
			{
				for (size_t iSymbol = 0; iSymbol < this->minRuleLengths[iRule]; iSymbol++)
				{
					r->successor->Add(this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1]);
				}
			}
		}

		rules->Add(r);
	}

#if _PMIT_D_V4_VERBOSE_ > 0
	Console::WriteLine("Before");
	for (size_t iRule = 0; iRule < rules->Count; iRule++)
	{
		if (this->IsModuleInSet(iRule))
		{
			rules[iRule]->Display();

			if ((rules[iRule]->successor->Count < this->minRuleLengths[iRule]) || (rules[iRule]->successor->Count > this->maxRuleLengths[iRule]))
			{
				Console::WriteLine("Rule size out of range.");
			}
		}
	}
#endif

	for (size_t iGene = 0; iGene <= Solution->GetUpperBound(0); iGene++)
	{
		Int32 iOption = this->DecodeRule(Solution[iGene]);
		Int32 iRule = this->ruleOptions[iOption];
		Int32 v = Solution[iGene];
		Int32 jOption = 0;
		while ((jOption < this->ruleOptions->Count) && (this->ruleOptions[jOption] != iRule))
		{
			v = v - PMIProblem::cGene_Max_Large;
			jOption++;
		}

		Int32 attempts = 0;
		while ((rules[this->ruleOptions[iOption]]->successor->Count == this->maxRuleLengths[this->ruleOptions[iOption]]) && (attempts <= this->ruleOptions->Count))
		{
			iOption++;
			attempts++;
			if (iOption >= this->ruleOptions->Count)
			{
				iOption = 0;
			}
		}

		if (rules[this->ruleOptions[iOption]]->successor->Count < this->maxRuleLengths[this->ruleOptions[iOption]])
		{
			if (this->constantIndex == 0)
			{
				Symbol^ s;
				if (!buildFragment[iRule])
				{// build assuming there exists either a head, tail or both

				 // Check to see if we're picking the 1st symbol
					if ((rules[iRule]->successor->Count == 0) && (this->maxRuleLengths[iRule] > 0))
					{
						if (this->minRuleLengths[iRule] == 0)
						{// Add a symbol which may be blank
							s = this->PickAnySymbol(v, true);
						}
						else
						{// Add a non-blank symbol
							s = this->PickAnySymbol(v, false);
						}
					}
					else
					{
						if (!buildInverted[iRule])
						{
							// Get the last symbol of the successor
							Symbol^ last = rules[iRule]->successor[rules[iRule]->successor->Count - 1];
							Int32 index = this->vocabulary->FindIndex(last);

							if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
							{// add a non-blank symbol
								s = this->PickSymbol(index, v, false);
							}
							else
							{// add a symbol which may be blank
								s = this->PickSymbol(index, v, true);
							}
						}
						else
						{
							// Get the first symbol of the successor
							Symbol^ first = rules[iRule]->successor[0];
							Int32 index = this->vocabulary->FindIndex(first);

							if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
							{// add a non-blank symbol
								s = this->PickSymbolInverted(index, v, false);
							}
							else
							{// add a symbol which may be blank
								s = this->PickSymbolInverted(index, v, true);
							}
						}
					}

					if (s->Value() != "")
					{
						rules[iRule]->successor->Insert(insertPoint[iRule], s);
						if (!buildInverted[iRule])
						{
							insertPoint[iRule]++;
						}
					}
				}
				else
				{
					bool invert = false;

					if (v >= PMIProblem::cGene_Half_Large)
					{
						invert = true;
						v = (v - PMIProblem::cGene_Half_Large - 1) * ((float)PMIProblem::cGene_Max_Large) / (PMIProblem::cGene_Half_Large);
					}
					else
					{
						v = v * ((float)PMIProblem::cGene_Max_Large / (PMIProblem::cGene_Half_Large - 1));
					}

					// Need to decide whether to build regular or inverted
					if (!invert)
					{
						// Get the last symbol of the successor
						Symbol^ last = rules[iRule]->successor[rules[iRule]->successor->Count - 1];
						Int32 index = this->vocabulary->FindIndex(last);

						if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
						{// add a non-blank symbol
							s = this->PickSymbol(index, v, false);
						}
						else
						{// add a symbol which may be blank
							s = this->PickSymbol(index, v, true);
						}
					}
					else
					{
						// Get the first symbol of the successor
						Symbol^ first = rules[iRule]->successor[0];
						Int32 index = this->vocabulary->FindIndex(first);

						if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
						{// add a non-blank symbol
							s = this->PickSymbolInverted(index, v, false);
						}
						else
						{// add a symbol which may be blank
							s = this->PickSymbolInverted(index, v, true);
						}
					}

					if (s->Value() != "")
					{
						if (invert)
						{
							rules[iRule]->successor->Insert(0, s);
						}
						else
						{
							rules[iRule]->successor->Add(s);
						}
					}
				}
			}
			else
			{
				Symbol^ s;
				if (!buildFragment[iRule])
				{// build assuming there exists either a head, tail or both

				 // Check to see if we're picking the 1st symbol
					if ((rules[iRule]->successor->Count == 0) && (this->maxRuleLengths[iRule] > 0))
					{
						if (this->minRuleLengths[iRule] == 0)
						{// Add a symbol which may be blank
							s = this->PickConstant(-1, v, true);
						}
						else
						{// Add a non-blank symbol
							s = this->PickConstant(-1, v, false);
						}
					}
					else
					{
						if (!buildInverted[iRule])
						{
							// Get the last symbol of the successor
							Symbol^ last = rules[iRule]->successor[rules[iRule]->successor->Count - 1];
							Int32 index = this->vocabulary->FindIndex(last);

							if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
							{// add a non-blank symbol
								s = this->PickConstant(index, v, false);
							}
							else
							{// add a symbol which may be blank
								s = this->PickConstant(index, v, true);
							}
						}
						else
						{
							// Get the first symbol of the successor
							Symbol^ first = rules[iRule]->successor[0];
							Int32 index = this->vocabulary->FindIndex(first);

							if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
							{// add a non-blank symbol
								s = this->PickConstantInverted(index, v, false);
							}
							else
							{// add a symbol which may be blank
								s = this->PickConstantInverted(index, v, true);
							}
						}
					}

					if (s->Value() != "")
					{
						rules[iRule]->successor->Insert(insertPoint[iRule], s);
						if (!buildInverted[iRule])
						{
							insertPoint[iRule]++;
						}
					}
				}
				else
				{
					bool invert = false;

					if (v >= PMIProblem::cGene_Half_Large)
					{
						invert = true;
						v = (v - PMIProblem::cGene_Half_Large - 1) * ((float)PMIProblem::cGene_Max_Large) / (PMIProblem::cGene_Half_Large);
					}
					else
					{
						v = v * ((float)PMIProblem::cGene_Max_Large / (PMIProblem::cGene_Half_Large - 1));
					}

					// Need to decide whether to build regular or inverted
					if (!invert)
					{
						// Get the last symbol of the successor
						Symbol^ last = rules[iRule]->successor[rules[iRule]->successor->Count - 1];
						Int32 index = this->vocabulary->FindIndex(last);

						if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
						{// add a non-blank symbol
							s = this->PickConstant(index, v, false);
						}
						else
						{// add a symbol which may be blank
							s = this->PickConstant(index, v, true);
						}
					}
					else
					{
						// Get the first symbol of the successor
						Symbol^ first = rules[iRule]->successor[0];
						Int32 index = this->vocabulary->FindIndex(first);

						if (rules[iRule]->successor->Count < this->minRuleLengths[iRule])
						{// add a non-blank symbol
							s = this->PickConstantInverted(index, v, false);
						}
						else
						{// add a symbol which may be blank
							s = this->PickConstantInverted(index, v, true);
						}
					}

					if (s->Value() != "")
					{
						if (invert)
						{
							rules[iRule]->successor->Insert(0, s);
						}
						else
						{
							rules[iRule]->successor->Add(s);
						}
					}
				}
			}
		}
	}

#if _PMIT_D_V4_VERBOSE_ > 0
	Console::WriteLine("After");
	for (size_t iRule = 0; iRule < rules->Count; iRule++)
	{
		if (this->IsModuleInSet(iRule))
		{
			rules[iRule]->Display();

			if ((rules[iRule]->successor->Count < this->minRuleLengths[iRule]) || (rules[iRule]->successor->Count > this->maxRuleLengths[iRule]))
			{
				Console::WriteLine("Rule size out of range.");
			}
		}
	}

	Console::WriteLine();
#endif

	return rules;
}

void PMIProblemV2A_D_v4::ComputeFragmentsFromPartialSolution()
{
	//for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//{
	//	List<Symbol^>^ toAdd = gcnew List<Symbol^>;

	//	Int32 iSymbol = 0;
	//	bool done = false;

	//	// find all the symbols before the first module
	//	while ((!done) && (iSymbol < this->rulesSolution[iRule]->successor->Count))
	//	{
	//		if (this->ruleFragmentsMaster[iRule]->symbols->Count > 0)
	//		{
	//			if (this->rulesSolution[iRule]->successor[iSymbol] != this->ruleFragmentsMaster[iRule][0])
	//			{
	//				//Console::Write(this->rulesSolution[iRule]->successor[iSymbol]);
	//				toAdd->Add(this->rulesSolution[iRule]->successor[iSymbol]);
	//				iSymbol++;
	//			}
	//			else
	//			{
	//				done = true;
	//			}
	//		}
	//		else
	//		{
	//			toAdd->AddRange(this->rulesSolution[iRule]->successor);
	//			done = true;
	//		}
	//	}

	//	for (size_t i = 0; i < toAdd->Count; i++)
	//	{
	//		this->ruleFragmentsMaster[iRule]->symbols->Insert(i, toAdd[i]);
	//	}

	//	//Console::WriteLine();

	//	toAdd->Clear();
	//	iSymbol = this->rulesSolution[iRule]->successor->Count-1;
	//	done = false;

	//	// find all the symbols before the first module
	//	while ((!done) && (iSymbol > 0))
	//	{
	//		if (this->rulesSolution[iRule]->successor[iSymbol] != this->ruleFragmentsMaster[iRule][this->ruleFragmentsMaster[iRule]->symbols->Count-1])
	//		{
	//			//Console::Write(this->rulesSolution[iRule]->successor[iSymbol]);
	//			toAdd->Add(this->rulesSolution[iRule]->successor[iSymbol]);
	//			iSymbol--;
	//		}
	//		else
	//		{
	//			done = true;
	//		}
	//	}

	//	toAdd->Reverse();
	//	//Console::WriteLine();

	//	for (size_t i = 0; i < toAdd->Count; i++)
	//	{
	//		this->ruleFragmentsMaster[iRule]->symbols->Add(toAdd[i]);
	//	}
	//}
}

UInt64 PMIProblemV2A_D_v4::ComputeSolutionSpaceSize()
{
	UInt64 result = 1;
	for (size_t iRule = 0; iRule < this->vocabulary->numModules - 1; iRule++)
	{
		if ((this->IsModuleInSet(iRule)) && (this->lexicon->symbolSubwords[iRule]->Count > 1))
		{
			result *= this->lexicon->symbolSubwords[iRule]->Count;
		}
	}

	return result;
}

GenomeConfiguration<Int32>^ PMIProblemV2A_D_v4::CreateGenome()
{
	this->blank = gcnew Symbol("");
	
	// Create the symbol Lexicon
	this->ComputeLexiconSymbol();

	// Precompute the total possible symbols
	this->totals = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);
	for (size_t iRow = 0; iRow < this->vocabulary->numModules + this->constantIndex; iRow++)
	{
		for (size_t iCol = 0; iCol < this->vocabulary->numModules + this->constantIndex; iCol++)
		{
			if (this->symbolLexicon[iRow, iCol] > 0)
			{
				this->totals[iRow]++;
			}
		}
	}

	this->totalsInverted = gcnew array<Int32>(this->vocabulary->numModules + this->constantIndex);
	for (size_t iRow = 0; iRow < this->vocabulary->numModules + this->constantIndex; iRow++)
	{
		for (size_t iCol = 0; iCol < this->vocabulary->numModules + this->constantIndex; iCol++)
		{
			if (this->symbolLexiconInverted[iRow, iCol] > 0)
			{
				this->totalsInverted[iRow]++;
			}
		}
	}

	Int32 numGenes1 = this->totalMaxRuleLength;
	Int32 numGenes2 = 0;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if (this->IsModuleInSet(iRule))
		{
			numGenes2 += this->maxRuleLengths[iRule];

			if (this->ruleFragments[iRule]->symbols->Count >= (this->ruleHead[iRule]->symbols->Count + this->ruleHead[iRule]->symbols->Count))
			{
				numGenes1 -= this->ruleFragments[iRule]->symbols->Count;
				numGenes2 -= this->ruleFragments[iRule]->symbols->Count;
			}
			else if ((this->constantIndex > 0) && (this->knownRuleLengthsIndexed[this->constantIndex, iRule] > 0))
			{
				numGenes1 -= this->knownRuleLengthsIndexed[this->constantIndex, iRule];
				numGenes2 -= this->knownRuleLengthsIndexed[this->constantIndex, iRule];
			}
			else
			{
				if ((this->ruleHead[iRule]->symbols->Count + this->ruleTail[iRule]->symbols->Count < this->maxRuleLengths[iRule]))
				{
					numGenes1 -= this->ruleHead[iRule]->symbols->Count;
					numGenes1 -= this->ruleTail[iRule]->symbols->Count;
					numGenes2 -= this->ruleHead[iRule]->symbols->Count;
					numGenes2 -= this->ruleTail[iRule]->symbols->Count;
				}
				else
				{
					numGenes1 -= this->ruleHead[iRule]->symbols->Count;
					numGenes2 -= this->ruleHead[iRule]->symbols->Count;
				}
			}

			if ((this->constantIndex > 0)
				&& (this->minRuleLengths[iRule] > 0)
				&& (this->ruleHead[iRule]->symbols->Count == 0)
				&& (this->ruleTail[iRule]->symbols->Count == 0)
				&& (this->ruleFragments[iRule]->symbols->Count == 0)
				)
			{
				numGenes1 -= this->minRuleLengths[iRule];
				numGenes2 -= this->minRuleLengths[iRule];
			}

		}
	}

	Int32 numGenes = Math::Min(numGenes1, numGenes2);

	array<Int32>^ min = gcnew array<Int32>(numGenes);
	array<Int32>^ max = gcnew array<Int32>(numGenes);

	Int32 numRuleOptions = 0;
	this->ruleOptions = gcnew List<Int32>;

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if ((this->minRuleLengths[iRule] != this->maxRuleLengths[iRule]) && (this->IsModuleInSet(iRule)))
		{
			numRuleOptions++;
			this->ruleOptions->Add(iRule);
		}
	}

	for (size_t iGene = 0; iGene < numGenes; iGene++)
	{
		min[iGene] = PMIProblem::cGene_Min_Large;
		max[iGene] = (numRuleOptions * PMIProblem::cGene_Max_Large) - 1;
	}

	GenomeConfiguration<Int32>^ result = gcnew GenomeConfiguration<Int32>(numGenes, min, max);

	return result;
}

Symbol^ PMIProblemV2A_D_v4::PickConstantInverted(Int32 Index, Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result = this->blank;

	Int32 options = 1;

	if (BlankAllowed)
	{
		options++;
	}

	if ((!BlankAllowed && options > 0) || (BlankAllowed && options > 1))
	{// if there are no options return a blank even if a blank is not allowed
		Int32 size = PMIProblem::cGene_Max_Large / options;
		Int32 choice = Ticket / size;
		if (choice > options)
		{
			choice = options;
		}

		if ((BlankAllowed) && (choice == options - 1))
		{// a blank has been picked
			result = this->blank;
		}
		else
		{
			result = this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1];
		}
	}

	return result;
}

Symbol^ PMIProblemV2A_D_v4::PickSymbolInverted(Int32 Index, Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result = this->blank;

	Int32 options = this->totalsInverted[Index];

	if (BlankAllowed)
	{
		options++;
	}

	if ((!BlankAllowed && options > 0) || (BlankAllowed && options > 1))
	{// if there are no options return a blank even if a blank is not allowed
		Int32 size = PMIProblem::cGene_Max_Large / options;
		Int32 choice = Ticket / size;
		if (choice > options)
		{
			choice = options;
		}

		if ((BlankAllowed) && (choice == options-1))
		{// a blank has been picked
			result = this->blank;
		}
		else
		{
			bool found = false;
			Int32 iCol = 0;
			Int32 pick = -1;
			do
			{
				if (this->symbolLexiconInverted[Index, iCol] > 0)
				{
					pick++;
					if (pick == choice)
					{
						found = true;
					}
				}
				iCol++;
			} while ((!found) && (iCol <= this->symbolLexiconInverted->GetUpperBound(1)));

			result = this->vocabulary->Symbols[iCol-1];
		}
	}

	return result;
}

Symbol^ PMIProblemV2A_D_v4::PickAnySymbolInverted(Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result;

	Int32 options = this->vocabulary->numModules + this->constantIndex;

	if (BlankAllowed)
	{
		options++;
	}

	Int32 size = PMIProblem::cGene_Max_Large / options;
	Int32 choice = Ticket / size;

	if ((BlankAllowed) && (choice >= this->vocabulary->numModules + this->constantIndex))
	{// a blank has been picked
		result = this->blank;
	}
	else
	{
		// Due to rounding errors
		if (choice > this->vocabulary->numModules + this->constantIndex)
		{
			choice = this->vocabulary->numModules + this->constantIndex;
		}
		result = this->vocabulary->Symbols[choice];
	}

	return result;
}

Int32 PMIProblemV2A_D_v4::DecodeRule(Int32 Ticket)
{
	return Ticket / PMIProblem::cGene_Max_Large;
	//return this->ruleOptions[idx];
}

Symbol^ PMIProblemV2A_D_v4::PickConstant(Int32 Index, Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result = this->blank;

	Int32 options = 1;

	if (BlankAllowed)
	{
		options++;
	}

	if ((!BlankAllowed && options > 0) || (BlankAllowed && options > 1))
	{// if there are no options return a blank even if a blank is not allowed
		Int32 size = PMIProblem::cGene_Max_Large / options;
		Int32 choice = Ticket / size;
		if (choice > options)
		{
			choice = options;
		}

		if ((BlankAllowed) && (choice == options - 1))
		{// a blank has been picked
			result = this->blank;
		}
		else
		{
			result = this->vocabulary->Symbols[this->vocabulary->numModules + this->constantIndex - 1];
		}
	}

	return result;
}

Symbol^ PMIProblemV2A_D_v4::PickSymbol(Int32 Index, Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result = this->blank;

	Int32 options = this->totals[Index];

	if (BlankAllowed)
	{
		options++;
	}

	if ((!BlankAllowed && options > 0) || (BlankAllowed && options > 1))
	{// if there are no options return a blank even if a blank is not allowed
		Int32 size = PMIProblem::cGene_Max_Large / options;
		Int32 choice = Ticket / size;
		if (choice > options)
		{
			choice = options;
		}

		if ((BlankAllowed) && (choice == options-1))
		{// a blank has been picked
			result = this->blank;
		}
		else
		{
			bool found = false;
			Int32 iCol = 0;
			Int32 pick = -1;
			do
			{
				if (this->symbolLexicon[Index, iCol] > 0)
				{
					pick++;
					if (pick == choice)
					{
						found = true;
					}
				}
				iCol++;
			} while ((!found) && (iCol <= this->symbolLexicon->GetUpperBound(1)));

			result = this->vocabulary->Symbols[iCol-1];
		}
	}

	return result;
}

Symbol^ PMIProblemV2A_D_v4::PickAnySymbol(Int32 Ticket, bool BlankAllowed)
{
	Symbol^ result;

	Int32 options = this->vocabulary->numModules + this->constantIndex;

	if (BlankAllowed)
	{
		options++;
	}

	Int32 size = PMIProblem::cGene_Max_Large / options;
	Int32 choice = Ticket / size;

	if ((BlankAllowed) && (choice >= this->vocabulary->numModules + this->constantIndex))
	{// a blank has been picked
		result = this->blank;
	}
	else
	{
		// Due to rounding errors
		// TODO: should fix rounding errors
		if (choice >= this->vocabulary->numModules + this->constantIndex)
		{
			choice = (this->vocabulary->numModules + this->constantIndex)-1;
		}
		result = this->vocabulary->Symbols[choice];
	}

	return result;
}

void PMIProblemV2A_D_v4::RollbackSpecial()
{
	for (size_t iRule = 0; iRule < this->rulesSolution->Count; iRule++)
	{
		this->rulesSolution[iRule] = gcnew ProductionRule();
	}
}
