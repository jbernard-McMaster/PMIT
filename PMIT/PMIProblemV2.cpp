#include "stdafx.h"
#include "PMIProblemV2.h"


PMIProblemV2::PMIProblemV2(PlantModel^ Model, float ErrorRate, String^ FileName) : PMIProblem(Model, ErrorRate, FileName)
{
}

PMIProblemV2::~PMIProblemV2()
{
}

array<Int32>^ PMIProblemV2::ComputeRuleLengths(List<ProductionRule^>^ R)
{
	array<Int32>^ result = gcnew array<Int32>(this->vocabulary->numModules);

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t jRule = 0; jRule < R->Count; jRule++)
		{
			if (R[jRule]->predecessor->Value() == this->vocabulary->Symbols[iRule]->Value())
			{
				result[iRule] = R[jRule]->successor->Count;
			}
		}
	}

	return result;
}

void PMIProblemV2::ComputeGrowthPattern()
{
	for (size_t iRule = 0; iRule <= this->minPoG->GetUpperBound(0); iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		array<Int32>^ metaFragmentPV = this->vocabulary->CompressSymbols(metaFragment);
		for (size_t iSymbol = 0; iSymbol <= this->minPoG->GetUpperBound(1); iSymbol++)
		{
			this->UpdateMinPoG(iRule, iSymbol, metaFragmentPV[iSymbol]);
		}
	}

#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
	// Adjust the PoG based on the Lexicon... i.e. if the Lexicon has better min/max values use those
	// TODO: Move this into Lexicon::CompileLexicon
	array<Int32,2>^ lowestPoGLexicon = gcnew array<Int32,2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());
	array<Int32,2>^ largestPoGLexicon = gcnew array<Int32,2>(this->vocabulary->numModules, this->vocabulary->NumSymbols());

	for (size_t iRule = 0; iRule <= lowestPoGLexicon->GetUpperBound(0); iRule++)
	{
		for (size_t iSymbol = 0; iSymbol <= lowestPoGLexicon->GetUpperBound(1); iSymbol++)
		{
			lowestPoGLexicon[iRule, iSymbol] = 9999999;
			largestPoGLexicon[iRule, iSymbol] = 0;
		}
	}

	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if (this->lexicon->symbolSubwords[iRule]->Count > 0)
		{
			for (size_t iWord = 0; iWord < this->lexicon->symbolSubwords[iRule]->Count; iWord++)
			{
				for (size_t iSymbol = 0; iSymbol <= this->lexicon->symbolSubwords[iRule][iWord]->parikhVector->GetUpperBound(0); iSymbol++)
				{
					if (this->lexicon->symbolSubwords[iRule][iWord]->parikhVector[iSymbol] < lowestPoGLexicon[iRule, iSymbol])
					{
						lowestPoGLexicon[iRule, iSymbol] = this->lexicon->symbolSubwords[iRule][iWord]->parikhVector[iSymbol];
					}

					if (this->lexicon->symbolSubwords[iRule][iWord]->parikhVector[iSymbol] > largestPoGLexicon[iRule, iSymbol])
					{
						largestPoGLexicon[iRule, iSymbol] = this->lexicon->symbolSubwords[iRule][iWord]->parikhVector[iSymbol];
					}
				}
			}
		}
		else
		{
			throw gcnew Exception("Rule #" + iRule + " has no words in the lexicon");
		}
	}

	// Update the Min PoG from the Lexicon
	// TODO: I don't think can ever actually result in an update as the Lexicon throws out any words which violate the estimate min/max PoG
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			//this->minPoG[iRule, iSymbol] = this->UpdateMinPoG(iRule, iSymbol, Math::Max(this->minPoG[iRule, iSymbol], lowestPoGLexicon[iRule, iSymbol]));
			this->UpdateMinPoG(iRule, iSymbol, Math::Max(this->minPoG[iRule, iSymbol], lowestPoGLexicon[iRule, iSymbol]));
		}
	}
#endif

	// Get the growth for the last generation
	this->growthSymbol = gcnew array<Int32>(this->vocabulary->NumSymbols());
	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
	{
		if (iSymbol < this->vocabulary->numModules)
		{
			this->growthSymbol[iSymbol] = this->evidence[this->lastGenerationSymbol[iSymbol]]->parikhVector[iSymbol];
		}
		else
		{
			this->growthSymbol[iSymbol] = this->evidence[this->lastGenerationSymbol[iSymbol]]->parikhVector[iSymbol] - this->evidence[this->lastGenerationSymbol[iSymbol] - 1]->parikhVector[iSymbol];
		}

		// In the special case where growth is zero, it means that the modules which produce the symbol are gone
		// Therefore find the last generation in which the symbol was produced and use that
		// Also update lastGenerationSymbol
		if (this->growthSymbol[iSymbol] == 0)
		{
			Int32 iGen = this->generations - 1;

			do
			{
				if (iSymbol < this->vocabulary->numModules)
				{
					this->growthSymbol[iSymbol] = this->evidenceMaster[iGen]->parikhVector[iSymbol];
				}
				else
				{
					this->growthSymbol[iSymbol] = this->evidenceMaster[iGen]->parikhVector[iSymbol] - this->evidenceMaster[iGen - 1]->parikhVector[iSymbol];
				}
				this->lastGenerationSymbol[iSymbol] = iGen;
				iGen--;
			} while ((this->growthSymbol[iSymbol] == 0) && (iGen >= 1));
		}
	}

	// Get the growth for each generation
	this->growthSymbolByGen = gcnew array<Int32, 2>(this->generations, this->vocabularyMaster->NumSymbols());
	for (size_t iGen = 1; iGen < this->generations; iGen++)
	{
		for (size_t iSymbol = this->vocabularyMaster->IndexSymbolsStart; iSymbol <= this->vocabularyMaster->IndexSymbolsEnd; iSymbol++)
		{
			if (iSymbol < this->vocabularyMaster->numModules)
			{
				this->growthSymbolByGen[iGen, iSymbol] = this->evidenceMaster[iGen]->parikhVector[iSymbol];
			}
			else
			{
				this->growthSymbolByGen[iGen, iSymbol] = this->evidenceMaster[iGen]->parikhVector[iSymbol] - this->evidenceMaster[iGen - 1]->parikhVector[iSymbol];
			}
		}
	}

	this->ComputeGrowthPattern_Unaccounted();

	// Analyze growth pattern
	// if growth is constant for a symbol S1, than any symbols that appeared in the previous generation did not produce the symbol S1 
	// unless a symbol has vanished which may have produced S1
	for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
	{
		for (size_t iGen = 1; iGen < this->generations - 1; iGen++)
		{
			//if ((this->growthSymbolByGen[iGen, iSymbol] == this->growthSymbolByGen[iGen + 1, iSymbol]) || (this->growthSymbolByGen[iGen, iSymbol] / this->growthSymbolByGen[iGen-1, iSymbol] == this->growthSymbolByGen[iGen + 1, iSymbol] / this->growthSymbolByGen[iGen, iSymbol]))
			if ((this->growthSymbolByGen[iGen, iSymbol] != 0) && (((this->growthSymbolByGen[iGen, iSymbol] == this->growthSymbolByGen[iGen + 1, iSymbol])) || (((float)this->growthSymbolByGen[iGen, iSymbol] / this->growthSymbolByGen[iGen - 1, iSymbol]) == ((float)this->growthSymbolByGen[iGen + 1, iSymbol] / this->growthSymbolByGen[iGen, iSymbol]))))
			{
				bool allowed = true;
				// check for any modules that vanished which might have produced the symbol
				for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
				{
					if ((((!this->symbolSubsets[iGen, iRule]) && (this->symbolSubsets[iGen - 1, iRule])) || ((this->symbolSubsets[iGen, iRule]) && (!this->symbolSubsets[iGen + 1, iRule]))) && (this->maxPoG[iRule,iSymbol]>0))
					{
						allowed = false;
					}
				}

				if (allowed)
				{
					for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
					{
						if ((this->symbolSubsets[iGen, iRule]) && (!this->symbolSubsets[iGen - 1, iRule]))
						{
							//Console::WriteLine("Module " + this->vocabulary->Symbols[iRule]->ToString() + " does not produce " + this->vocabulary->Symbols[iSymbol]->ToString());
#if _PMI_PROBLEM_TESTING_ >= 1
							for (size_t iModelRule = 0; iModelRule < this->model->rules->Count; iModelRule++)
							{
								if (this->model->rules[iRule]->predecessor == this->vocabulary->Symbols[iRule])
								{
									for (size_t iRuleSymbol = 0; iRuleSymbol < this->model->rules[iRule]->successor->Count; iRuleSymbol++)
									{
										//Console::WriteLine(this->model->rules[iRule]->successor[iRuleSymbol]->ToString());
										if (this->model->rules[iRule]->successor[iRuleSymbol] == this->vocabulary->Symbols[iSymbol])
										{
											throw gcnew Exception("Module " + this->vocabulary->Symbols[iRule]->ToString() + " **does** produce " + this->vocabulary->Symbols[iSymbol]);
										}
									}
								}
							}
#endif
							this->minPoG[iRule, iSymbol] = 0;
							this->maxPoG[iRule, iSymbol] = 0;
						}
					}
				}
			}
		}
	}

#if _PMI_PROBLEM_ANALYZE_LEXICON > 0
	// Update the Max PoG from the Lexicon
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
		{
			//this->maxPoG[iRule, iSymbol] = this->UpdateMaxPoG(iRule, iSymbol, Math::Min(this->maxPoG[iRule, iSymbol], largestPoGLexicon[iRule, iSymbol]));
			this->UpdateMaxPoG(iRule, iSymbol, Math::Min(this->maxPoG[iRule, iSymbol], largestPoGLexicon[iRule, iSymbol]));
		}
	}
#endif


	// Compute a feature vector based on growth
	// Compute a feature vector based on changes in growth
	this->fvGrowth = gcnew array<float, 2>(this->generations, this->vocabulary->NumSymbols());
	this->fvDeltaGrowth = gcnew array<float, 2>(this->generations - 1, this->vocabulary->NumSymbols());

	for (size_t iGen = 0; iGen < this->evidence->Count - 1; iGen++)
	{
		// Get the two generations of evidence
		Word^ x = this->evidence[iGen + 1];
		Word^ y = this->evidence[iGen];

		for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
		{
			this->fvGrowth[iGen, iSymbol] = x->parikhVector[iSymbol] - y->parikhVector[iSymbol];
			if (iGen >= 1)
			{
				this->fvDeltaGrowth[iGen - 1, iSymbol] = this->fvGrowth[iGen, iSymbol] - this->fvGrowth[iGen - 1, iSymbol];
			}
		}
	}

	// For any symbols with a known value, set the template to this value
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
		{
			if (this->IsModuleInSet(iRule))
			{
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
				{
					if (this->minPoG[iRule, iSymbol] == this->maxPoG[iRule, iSymbol])
					{
						//Console::WriteLine(iGen + ", " + iRule + ", " + iSymbol + " = " + this->rulesParikhTemplate[iGen, iRule, iSymbol] + ", " + this->maxOccuranceSymbolRule[iRule, iSymbol]);
						this->rulesParikhTemplate[iGen, iRule, iSymbol] = this->maxPoG[iRule, iSymbol];
					}
				}
			}
		}
	}

	this->OutputAnalysis();


#if _PMI_PROBLEM_VERBOSE_ >= 2
	for (size_t iRule = 0; iRule <= this->vocabulary->IndexModulesEnd; iRule++)
	{
		Console::Write(this->vocabulary->Symbols[iRule]->Value());
		for (size_t iSymbol = this->masterVocabulary->IndexModulesStart; iSymbol <= this->masterVocabulary->IndexConstantsEnd; iSymbol++)
		{
			Console::Write(" " + this->vocabulary->Symbols[iSymbol]->Value() + ":" + this->maxOccuranceSymbolRule[iRule, iSymbol]);
		}
		Console::WriteLine("");
	}
	Console::WriteLine("Paused");
	Console::ReadLine();
#endif
}

bool PMIProblemV2::EstimateRuleLengths(bool WithLexicon, bool WithInference)
{
	bool result = false;

	// Find the maximum rule lengths for each individual rule by summing the proportion of growth values
	if (!this->flagAnalysisFailed)
	{
		result = this->EstimateRuleLengths_PoG() || result;
		//Console::WriteLine("EstimateRuleLengths::PoG       :: Solution Space Size = " + this->ComputeSolutionSpaceSize());
	}

#if _PMI_PROBLEM_ANALYZE_FRAGMENTS_ > 0
	// Update min values based on available fragments
	if (!this->flagAnalysisFailed)
	{
		result = this->EstimateRuleLengths_Fragments() || result;
		//Console::WriteLine("EstimateRuleLengths::Frag      :: Solution Space Size = " + this->ComputeSolutionSpaceSize());
	}
#endif

	if (!this->flagAnalysisFailed)
	{
		this->UpdateTotalRuleLengths();
	}
	// Update based on Actual production to maximum production
	//result = this->EstimateRuleLengths_Growth() || result;

#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
	// Update based on lexicon
	if ((WithLexicon) && (!this->flagAnalysisFailed))
	{
		result = this->EstimateRuleLengths_Lexicon() || result;
		//Console::WriteLine("EstimateRuleLengths::Lex       :: Solution Space Size = " + this->ComputeSolutionSpaceSize());
	}
#endif

#if _PMI_PROBLEM_ANALYZE_INFERENCE_ > 0
	// Update based on inference
	if ((WithInference) && (!this->flagAnalysisFailed))
	{
		for (size_t iSet = 0; iSet < this->problemSets->Count; iSet++)
		{
			result = this->EstimateRuleLengths_Inference(this->problemSets[iSet]->startGeneration, this->problemSets[iSet]->endGeneration) || result;
			//Console::WriteLine("EstimateRuleLengths::Infer (" + iSet.ToString(L"D2") + "):: Solution Space Size = " + this->ComputeSolutionSpaceSize());
		}
	}
#endif

	if (!this->flagAnalysisFailed)
	{
		this->UpdateKnownRuleLengths();
		this->UpdateTotalRuleLengths();
		this->OutputRuleLengths();
	}

#if _PMI_PROBLEM_ANALYZE_LEXICON_ > 0
	if ((result) && (WithLexicon) && (!this->flagAnalysisFailed))
	{
		for (size_t iRule = 0; iRule <= this->minRuleLengths->GetUpperBound(0); iRule++)
		{
			if (this->minRuleLengths[iRule] > this->maxRuleLengths[iRule])
			{
				this->flagAnalysisFailed = true;
				//throw gcnew Exception("Rule #" + iRule + "minimum " + this->minRuleLengths[iRule] + " > " + this->maxRuleLengths[iRule]);
			}
			else if (this->minRuleLengths[iRule] > this->lexicon->symbolSubwords[iRule][this->lexicon->symbolSubwords[iRule]->Count-1]->symbols->Count)
			{
				this->flagAnalysisFailed = true;
				//throw gcnew Exception("Rule #" + iRule + "minimum " + this->minRuleLengths[iRule] + " > " + this->lexicon->symbolSubwords[iRule][this->lexicon->symbolSubwords[iRule]->Count - 1]->symbols->Count);
			}
			else
			{
				this->lexicon->UpdateLexicon_Length(iRule, this->minRuleLengths[iRule], this->maxRuleLengths[iRule]);
			}
		}
	}
#endif

	//Console::WriteLine("EstimateRuleLengths::Finished  :: Solution Space Size = " + this->ComputeSolutionSpaceSize());
	return result;
}

bool PMIProblemV2::EstimateRuleLengths_Fragments()
{
	bool result = false;
	Console::WriteLine("Rule lengths based on Fragments");
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		List<Symbol^>^ metaFragment = this->AssembleFragments(iRule); // NOTE: This fragment has no resemblance to what might be produced, it is simply the sum of all symbols which are produced
		result = this->UpdateMinRuleLengths(iRule, metaFragment->Count) || result;
	}

	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2::EstimateRuleLengths_Growth()
{
	bool result = false;

	Console::WriteLine("Update Lengths Based on Actual Production to Estimated Production");
	for (size_t iGen = 0; iGen < this->generations - 1; iGen++)
	{
		// Total production is the number of symbols in the next generation minus the number of constants in this generation
		// # of constants is equal to the total number of symbols minus the number of modules
		Int32 totalProduction = this->evidence[iGen + 1]->symbols->Count - (this->evidence[iGen]->symbols->Count - this->evidenceModuleOnly[iGen]->symbols->Count);
		Int32 maxProduction = 0;
		Int32 minProduction = 0;
		Int32 avgProduction = 0;
		for (size_t iRule = 0; iRule <= this->maxRuleLengths->GetUpperBound(0); iRule++)
		{
			maxProduction += this->maxRuleLengths[iRule] * this->evidence[iGen]->parikhVector[iRule];
			minProduction += this->minRuleLengths[iRule] * this->evidence[iGen]->parikhVector[iRule];
			//avgProduction += ((this->minRuleLengths[iRule] + this->maxRuleLengths[iRule]) / 2) * this->evidence[iGen]->parikhVector[iRule];
		}

		// Compute the error
		Int32 errorMax = maxProduction - totalProduction;
		Int32 errorMin = totalProduction - minProduction;
		Int32 errorAvg = Math::Abs(totalProduction - avgProduction);
		//Console::WriteLine(totalProduction + " : " + minProduction + " : " +  avgProduction + " : " + maxProduction);
		//Console::WriteLine(totalProduction + " : " + minProduction + " : " + maxProduction);

		// For any symbols in this generation, the difference between the min/max cannot be greater than the error
		for (size_t iRule = 0; iRule <= this->maxRuleLengths->GetUpperBound(0); iRule++)
		{
			if (this->symbolSubsets[iGen, iRule])
			{
				result = this->UpdateMinRuleLengths(iRule, this->maxRuleLengths[iRule] - errorMax) || result;
				result = this->UpdateMaxRuleLengths(iRule, this->minRuleLengths[iRule] + errorMin) || result;
				//result = this->UpdateMinRuleLengths(iRule, ((this->maxRuleLengths[iRule] + this->minRuleLengths[iRule]) / 2) - errorAvg) || result;
				//result = this->UpdateMaxRuleLengths(iRule, ((this->maxRuleLengths[iRule] + this->minRuleLengths[iRule]) / 2) + errorAvg) || result;
			}
		}
	}
	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2::EstimateRuleLengths_Lexicon()
{
	bool result = false;

	// Update Min/Max Lengths based on Lexicon
	Console::WriteLine("Rule lengths based on Lexicon");
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		if (this->lexicon->symbolSubwords[iRule]->Count > 1)
		{
			result = this->UpdateMinRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			result = this->UpdateMaxRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][this->lexicon->symbolSubwords[iRule]->Count - 1]->symbols->Count) || result;
		}
		else
		{// if the lexicon has a single word then this is the solution
			this->solvedModules[iRule] = true;
			result = this->UpdateMinRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			result = this->UpdateMaxRuleLengths(iRule, this->lexicon->symbolSubwords[iRule][0]->symbols->Count) || result;
			this->UpdatePoGForCompleteFragment(iRule, this->lexicon->symbolSubwords[iRule][0]);
		}
	}
	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2::EstimateRuleLengths_Inference(Int32 Start, Int32 End)
{
	bool result = false;

	for (size_t iGen = Start; iGen <= End - 1; iGen++)
	{
		// Ensure that the modules have reached a steady state
		if (!this->IsSymbolSubsetChange(iGen, iGen + 1))
		{
			// Get the two generations of evidence
			Word^ e2 = this->evidence[iGen + 1];
			Word^ e1 = this->evidence[iGen];

			// Get the two generations of evidence, modules only
			Word^ eMO2 = this->evidenceModuleOnly[iGen + 1];
			Word^ eMO1 = this->evidenceModuleOnly[iGen];

			// The number of symbols in the generation minus the number of constants in the previous generation
			Int32 min = 0;
			Int32 countMin = e2->symbols->Count - this->vocabulary->CountConstants(e1);
			Int32 idxMin = 0;

			Int32 max = 0;
			Int32 countMax = countMin;
			Int32 idxMax = 0;

			// The number of symbols in the generation minus the number of constants in the previous generation
			Int32 minMO = 0;
			Int32 countMinMO = eMO2->symbols->Count - this->vocabulary->CountConstants(eMO1);
			Int32 idxMinMO = 0;

			Int32 maxMO = 0;
			Int32 countMaxMO = countMinMO;
			Int32 idxMaxMO = 0;

			// Find the most occuring symbol in y
			Int32 highest = 0;
			Int32 lowest = 99999999;
			Int32 highestMO = 0;
			Int32 lowestMO = 99999999;

			if (this->vocabulary->numModules > 1)
			{
				// Step 1 - Find the modules that exist the least and most
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
				{
					if (this->symbolSubsets[iGen, iSymbol])
					{
						Int32 tmp = idxMin;
						if ((e1->parikhVector[iSymbol] > highest) || ((e1->parikhVector[iSymbol] == highest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp])))
						{
							idxMin = iSymbol;
							highest = e1->parikhVector[iSymbol];
						}

						if ((e1->parikhVector[iSymbol] < lowest) || ((e1->parikhVector[iSymbol] == lowest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp])))
						{
							idxMax = iSymbol;
							lowest = e1->parikhVector[iSymbol];
						}

						if ((eMO1->parikhVector[iSymbol] > highestMO) || ((e1->parikhVector[iSymbol] == highestMO) && (this->minRuleLengthsModulesOnly[iSymbol] > this->minRuleLengthsModulesOnly[tmp])))
						{
							idxMinMO = iSymbol;
							highestMO = eMO1->parikhVector[iSymbol];
						}

						if ((eMO1->parikhVector[iSymbol] < lowestMO) || ((e1->parikhVector[iSymbol] == lowestMO) && (this->minRuleLengthsModulesOnly[iSymbol] > this->minRuleLengthsModulesOnly[tmp])))
						{
							idxMaxMO = iSymbol;
							lowestMO = eMO1->parikhVector[iSymbol];
						}
					}
				}

				// Assume every other symbol only produces their minimum
				// if the minimum is unknown assume it is 1
				// if the minimum is unknown or modules only assume it is 0
				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
				{
					if (this->symbolSubsets[iGen, iSymbol])
					{
						if (iSymbol != idxMin)
						{
							countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
							//countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
						}
						else
						{
							min++;
						}

						if (iSymbol != idxMax)
						{
							//countMax -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
							countMax -= e1->parikhVector[iSymbol];
						}
						else
						{
							max++;
						}

						if (iSymbol != idxMinMO)
						{
							//countMinMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
							countMinMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
						}
						else
						{
							minMO++;
						}

						if (iSymbol != idxMaxMO)
						{
							//countMaxMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
							countMaxMO -= eMO1->parikhVector[iSymbol];
						}
						else
						{
							maxMO++;
						}
					}
				}

				if (e1->parikhVector[idxMin] != 0)
				{
					countMin /= e1->parikhVector[idxMin];
				}

				if (e1->parikhVector[idxMax] != 0)
				{
					countMax /= e1->parikhVector[idxMax];
				}

				min += countMin;
				max += countMax;

				if (eMO1->parikhVector[idxMinMO] != 0)
				{
					countMinMO /= eMO1->parikhVector[idxMinMO];
				}

				if (eMO1->parikhVector[idxMaxMO])
				{
					countMaxMO /= eMO1->parikhVector[idxMaxMO];
				}

				minMO += countMinMO;
				maxMO += countMaxMO;

				result = this->UpdateTotalMaxRuleLengths(max) || result;
				result = this->UpdateTotalMinRuleLengths(min) || result;

				if (maxMO < this->totalMaxRuleLengthModulesOnly)
				{
					this->totalMaxRuleLengthModulesOnly = Math::Min(maxMO, this->totalMaxRuleLengthModulesOnly);
				}

				if ((minMO > this->totalMinRuleLengthModulesOnly) && (minMO <= this->totalMaxRuleLengthModulesOnly))
				{
					this->totalMinRuleLengthModulesOnly = Math::Max(minMO, this->totalMinRuleLengthModulesOnly);
				}
			}
		}
	}

	// Given the total maximum rule length, the minimum individual rule length for each rule can be computed by
	// TotalMin - Sum(Maximums except iRule)
	// And
	// TotalMax - Sum(Minimum except iRule)
	// Repeat this process until convergence
	bool valueChanged;
	do
	{
		valueChanged = false;
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			Int32 sum = 0;

			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
			{
				if (iValue != iRule)
				{
					sum += this->maxRuleLengths[iValue];
				}
			}

			Int32 tmp = this->minRuleLengths[iRule]; // capture the current value

			result = this->UpdateMinRuleLengths(iRule, this->totalMinRuleLength - sum) || result;

			if (tmp != this->minRuleLengths[iRule])
			{
				valueChanged = true;
			}
		}

		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			Int32 sum = 0;

			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
			{
				if (iValue != iRule)
				{
					sum += this->minRuleLengths[iValue];
				}
			}

			Int32 tmp = this->maxRuleLengths[iRule]; // capture the current value
			result = this->UpdateMaxRuleLengths(iRule, this->totalMaxRuleLength - sum) || result;

			if (tmp != this->maxRuleLengths[iRule])
			{
				valueChanged = true;
			}
		}
	} while (valueChanged);

	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2::EstimateRuleLengths_PoG()
{
	bool result = false;
	// Find the maximum rule lengths for each individual rule by summing the proportion of growth values
#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Rule lengths based on PoG");
#endif
	if (this->vocabulary->numModules > 1)
	{
		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
		{
			result = this->UpdateMinRuleLengths(iRule, Math::Max(this->minRuleLengths[iRule], 1)) || result;
			this->minRuleLengthsModulesOnly[iRule] = 0;

			Int32 max = 0; // maximum length of the complete rule
			Int32 maxModulesOnly = 0; // maximum length for modules only

			for (size_t iSymbol = 0; iSymbol < this->vocabulary->NumSymbols(); iSymbol++)
			{
				if (iSymbol < this->vocabulary->numModules)
				{
					max += this->maxPoG[iRule, iSymbol];
					maxModulesOnly += this->maxPoG[iRule, iSymbol];
				}
				else
				{
					max += this->maxPoG[iRule, iSymbol];
				}
			}

			result = this->UpdateMaxRuleLengths(iRule, max) || result;
			this->maxRuleLengthsModulesOnly[iRule] = maxModulesOnly;
		}
	}
	else
	{
		// Will be equal to the sum of PoG for the symbol 0 since there is only one module
		Int32 sum = 0;
		for (size_t iSymbol = 0; iSymbol <= this->maxPoG->GetUpperBound(1); iSymbol++)
		{
			sum += this->maxPoG[0, iSymbol];
		}

		result = this->UpdateMinRuleLengths(0, sum) || result;
		result = this->UpdateMaxRuleLengths(0, sum) || result;

		this->minRuleLengthsModulesOnly[0] = this->maxPoG[0, 0];
		this->maxRuleLengthsModulesOnly[0] = this->maxPoG[0, 0];
	}
	this->OutputRuleLengths();

	return result;
}

bool PMIProblemV2::IsModuleInSet(Int32 iRule)
{
	Int32 iGen = this->currentSet->startGeneration;
	bool result = false;
	do
	{
		result = this->symbolSubsets[iGen, iRule];
		iGen++;
	} while ((!result) && (iGen < this->currentSet->endGeneration));

	return result;
}

void PMIProblemV2::UpdateTotalRuleLengths()
{
	bool result = false;

	Int32 totalMin = 0;
	Int32 totalMax = 0;
	Int32 totalMinMO = 0;
	Int32 totalMaxMO = 0;

#if _PMI_PROBLEM_DISPLAY_RULE_LENGTHS_ >= 1
	Console::WriteLine("Update Total Min/Max Based on Rule Min/Max Length");
#endif
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		totalMin += this->minRuleLengths[iRule];
		totalMax += this->maxRuleLengths[iRule];
		totalMinMO += this->minRuleLengthsModulesOnly[iRule];
		totalMaxMO += this->maxRuleLengthsModulesOnly[iRule];
	}

	result = this->UpdateTotalMinRuleLengths(totalMin) || result;
	result = this->UpdateTotalMaxRuleLengths(totalMax) || result;
	this->totalMinRuleLengthModulesOnly = totalMinMO;
	this->totalMaxRuleLengthModulesOnly = totalMaxMO;
	
	this->OutputRuleLengths();
}

//bool PMIProblemV2::EstimateRuleLengths_Inference(Int32 Start, Int32 End)
//{
//	bool result = false;
//
//	for (size_t iGen = Start; iGen <= End - 1; iGen++)
//	{
//		// Ensure that the modules have reached a steady state
//		if (!this->IsSymbolSubsetChange(iGen, iGen + 1))
//		{
//			// Get the two generations of evidence
//			Word^ e2 = this->evidence[iGen + 1];
//			Word^ e1 = this->evidence[iGen];
//
//			// Get the two generations of evidence, modules only
//			Word^ eMO2 = this->evidenceModuleOnly[iGen + 1];
//			Word^ eMO1 = this->evidenceModuleOnly[iGen];
//
//			// The number of symbols in the generation minus the number of constants in the previous generation
//			Int32 min = 0;
//			Int32 countMin = e2->symbols->Count - this->vocabulary->CountConstants(e1);
//			Int32 idxMin = 0;
//
//			Int32 max = 0;
//			Int32 countMax = countMin;
//			Int32 idxMax = 0;
//
//			// The number of symbols in the generation minus the number of constants in the previous generation
//			Int32 minMO = 0;
//			Int32 countMinMO = eMO2->symbols->Count - this->vocabulary->CountConstants(eMO1);
//			Int32 idxMinMO = 0;
//
//			Int32 maxMO = 0;
//			Int32 countMaxMO = countMinMO;
//			Int32 idxMaxMO = 0;
//
//			// Find the most occuring symbol in y
//			Int32 highest = 0;
//			Int32 lowest = 99999999;
//			Int32 highestMO = 0;
//			Int32 lowestMO = 99999999;
//
//			if (this->vocabulary->numModules > 1)
//			{
//				// Step 1 - Find the modules that exist the least and most
//				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
//				{
//					if (this->symbolSubsets[iGen, iSymbol])
//					{
//						Int32 tmp = idxMin;
//						if ((e1->parikhVector[iSymbol] > highest) || ((e1->parikhVector[iSymbol] == highest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp])))
//						{
//							idxMin = iSymbol;
//							highest = e1->parikhVector[iSymbol];
//						}
//
//						if ((e1->parikhVector[iSymbol] < lowest) || ((e1->parikhVector[iSymbol] == lowest) && (this->minRuleLengths[iSymbol] > this->minRuleLengths[tmp])))
//						{
//							idxMax = iSymbol;
//							lowest = e1->parikhVector[iSymbol];
//						}
//
//						if ((eMO1->parikhVector[iSymbol] > highestMO) || ((e1->parikhVector[iSymbol] == highestMO) && (this->minRuleLengthsModulesOnly[iSymbol] > this->minRuleLengthsModulesOnly[tmp])))
//						{
//							idxMinMO = iSymbol;
//							highestMO = eMO1->parikhVector[iSymbol];
//						}
//
//						if ((eMO1->parikhVector[iSymbol] < lowestMO) || ((e1->parikhVector[iSymbol] == lowestMO) && (this->minRuleLengthsModulesOnly[iSymbol] > this->minRuleLengthsModulesOnly[tmp])))
//						{
//							idxMaxMO = iSymbol;
//							lowestMO = eMO1->parikhVector[iSymbol];
//						}
//					}
//				}
//
//				// Assume every other symbol only produces their minimum
//				// if the minimum is unknown assume it is 1
//				// if the minimum is unknown or modules only assume it is 0
//				for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
//				{
//					if (this->symbolSubsets[iGen, iSymbol])
//					{
//						if (iSymbol != idxMin)
//						{
//							countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
//							//countMin -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
//						}
//						else
//						{
//							min++;
//						}
//
//						if (iSymbol != idxMax)
//						{
//							//countMax -= e1->parikhVector[iSymbol] * this->minRuleLengths[iSymbol];
//							countMax -= e1->parikhVector[iSymbol];
//						}
//						else
//						{
//							max++;
//						}
//
//						if (iSymbol != idxMinMO)
//						{
//							//countMinMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
//							countMinMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
//						}
//						else
//						{
//							minMO++;
//						}
//
//						if (iSymbol != idxMaxMO)
//						{
//							//countMaxMO -= eMO1->parikhVector[iSymbol] * this->minRuleLengthsModulesOnly[iSymbol];
//							countMaxMO -= eMO1->parikhVector[iSymbol];
//						}
//						else
//						{
//							maxMO++;
//						}
//					}
//				}
//
//				if (e1->parikhVector[idxMin] != 0)
//				{
//					countMin /= e1->parikhVector[idxMin];
//				}
//
//				if (e1->parikhVector[idxMax] != 0)
//				{
//					countMax /= e1->parikhVector[idxMax];
//				}
//
//				min += countMin;
//				max += countMax;
//
//				if (eMO1->parikhVector[idxMinMO] != 0)
//				{
//					countMinMO /= eMO1->parikhVector[idxMinMO];
//				}
//
//				if (eMO1->parikhVector[idxMaxMO])
//				{
//					countMaxMO /= eMO1->parikhVector[idxMaxMO];
//				}
//
//				minMO += countMinMO;
//				maxMO += countMaxMO;
//
//				result = this->UpdateTotalMaxRuleLengths(max) || result;
//				result = this->UpdateTotalMinRuleLengths(min) || result;
//
//				if (maxMO < this->totalMaxRuleLengthModulesOnly)
//				{
//					this->totalMaxRuleLengthModulesOnly = Math::Min(maxMO, this->totalMaxRuleLengthModulesOnly);
//				}
//
//				if ((minMO > this->totalMinRuleLengthModulesOnly) && (minMO <= this->totalMaxRuleLengthModulesOnly))
//				{
//					this->totalMinRuleLengthModulesOnly = Math::Max(minMO, this->totalMinRuleLengthModulesOnly);
//				}
//			}
//		}
//	}
//
//	// Given the total maximum rule length, the minimum individual rule length for each rule can be computed by
//	// TotalMin - Sum(Maximums except iRule)
//	// And
//	// TotalMax - Sum(Minimum except iRule)
//	// Repeat this process until convergence
//	bool valueChanged;
//	do
//	{
//		valueChanged = false;
//		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
//		{
//			Int32 sum = 0;
//
//			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
//			{
//				if (iValue != iRule)
//				{
//					sum += this->maxRuleLengths[iValue];
//				}
//			}
//
//			Int32 tmp = this->minRuleLengths[iRule]; // capture the current value
//
//			result = this->UpdateMinRuleLengths(iRule, this->totalMinRuleLength - sum) || result;
//
//			if (tmp != this->minRuleLengths[iRule])
//			{
//				valueChanged = true;
//			}
//		}
//
//		for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
//		{
//			Int32 sum = 0;
//
//			for (size_t iValue = 0; iValue < this->vocabulary->numModules; iValue++)
//			{
//				if (iValue != iRule)
//				{
//					sum += this->minRuleLengths[iValue];
//				}
//			}
//
//			Int32 tmp = this->maxRuleLengths[iRule]; // capture the current value
//			result = this->UpdateMaxRuleLengths(iRule, this->totalMaxRuleLength - sum) || result;
//
//			if (tmp != this->maxRuleLengths[iRule])
//			{
//				valueChanged = true;
//			}
//		}
//	} while (valueChanged);
//
//	this->OutputRuleLengths();
//
//	return result;
//}
