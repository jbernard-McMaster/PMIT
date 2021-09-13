#include "stdafx.h"
#include "PlantModel.h"

RuleStatistics::RuleStatistics()
{
}

RuleStatistics::~RuleStatistics()
{
}

ModelStatistics::ModelStatistics()
{
	this->ruleStats = gcnew List<RuleStatistics^>;
}

ModelStatistics::~ModelStatistics()
{
}

void ModelStatistics::Display()
{
	Console::WriteLine("Avg Rule True Length = " + this->avgRuleLength + " Avg Rule Compressed Length " + this->avgRuleLengthCompressed);
	Console::WriteLine("# Drawing Rules   = " + this->countDrawingRules);
	Console::WriteLine("# Producing Rules = " + this->countProducingRules);
	Console::WriteLine("# Terminal Rules  = " + this->countTerminalRules);

	Console::WriteLine("Symbol Pattern");
	Console::WriteLine("==============");
	for (size_t i = 0; i <= this->symbolPattern->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->symbolPattern->GetUpperBound(1); j++)
		{
			Console::Write(this->symbolPattern[i, j].ToString(L"D3") + " ");
		}
		Console::WriteLine();
	}

	Console::WriteLine("");
	Console::WriteLine("Compressed Symbol Pattern");
	Console::WriteLine("=========================");
	for (size_t i = 0; i <= this->symbolPatternCompressed->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->symbolPatternCompressed->GetUpperBound(1); j++)
		{
			Console::Write(this->symbolPatternCompressed[i, j].ToString(L"D3") + " ");
		}
		Console::WriteLine();
	}

	for (size_t iRule = 0; iRule < this->ruleStats->Count; iRule++)
	{
		Console::WriteLine();
		Console::WriteLine("Rule #" + iRule.ToString(L"D2") + " Statistics");
		Console::WriteLine("===================");
		Console::WriteLine("Rule Pattern: " + this->ruleStats[iRule]->pattern);
		Console::WriteLine("Compress Pattern: " + this->ruleStats[iRule]->patternCompressed);
		Console::WriteLine("True Length: " + this->ruleStats[iRule]->length + " Compressed Length: " + this->ruleStats[iRule]->lengthCompressed);
		Console::WriteLine("Ratio Producing to Graphical = " + this->ruleStats[iRule]->ratioProducing);
		Console::WriteLine("# Repeated Symbols = " + this->ruleStats[iRule]->countRepeatedSymbols);
		Console::WriteLine("# G2P transitions = " + this->ruleStats[iRule]->countGraphical2ProducingSymbols);
		Console::WriteLine("# P2G transitions = " + this->ruleStats[iRule]->countProducing2GraphicalSymbols);
		Console::Write("Unique Symbols: ");
		bool producesProducers = false;
		for (size_t i = 0; i < this->ruleStats[iRule]->uniqueSymbols->Count; i++)
		{
			Console::Write(this->ruleStats[iRule]->uniqueSymbols[i] + " ");
		}
		Console::WriteLine();
		Console::Write("Unique Graphics: ");
		for (size_t i = 0; i < this->ruleStats[iRule]->uniqueGraphics->Count; i++)
		{
			Console::Write(this->ruleStats[iRule]->uniqueGraphics[i] + " ");
		}
		Console::WriteLine();

		Console::WriteLine("Is producer? " + this->ruleStats[iRule]->ruleIsProducer);
		Console::WriteLine("Produces producer? " + this->ruleStats[iRule]->ruleProducesProducer);
	}
}

void ModelStatistics::Write(String^ FileName)
{
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(FileName, false);

	sw->WriteLine("Avg Rule True Length = ," + this->avgRuleLength + ", Avg Rule Compressed Length ," + this->avgRuleLengthCompressed);
	sw->WriteLine("# Drawing Rules   = ," + this->countDrawingRules + ",# Producing Rules = ," + this->countProducingRules + ",# Terminal Rules  = ," + this->countTerminalRules);

	sw->WriteLine("Symbol Pattern");
	sw->WriteLine("==============");
	for (size_t i = 0; i <= this->symbolPattern->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->symbolPattern->GetUpperBound(1); j++)
		{
			sw->Write(this->symbolPattern[i, j].ToString(L"D3") + ",");
		}
		sw->WriteLine();
	}

	sw->WriteLine("");
	sw->WriteLine("Compressed Symbol Pattern");
	sw->WriteLine("=========================");
	for (size_t i = 0; i <= this->symbolPatternCompressed->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->symbolPatternCompressed->GetUpperBound(1); j++)
		{
			sw->Write(this->symbolPatternCompressed[i, j].ToString(L"D3") + ",");
		}
		sw->WriteLine();
	}

	for (size_t iRule = 0; iRule < this->ruleStats->Count; iRule++)
	{
		sw->WriteLine();
		sw->WriteLine("Rule #" + iRule.ToString(L"D2") + " Statistics");
		sw->WriteLine("===================");
		sw->WriteLine("Rule Pattern: ," + this->ruleStats[iRule]->pattern);
		sw->WriteLine("Compressed Pattern: ," + this->ruleStats[iRule]->patternCompressed);
		sw->WriteLine("True Length: ," + this->ruleStats[iRule]->length + " Compressed Length: " + this->ruleStats[iRule]->lengthCompressed);
		sw->WriteLine("Ratio Producing to Graphical = ," + this->ruleStats[iRule]->ratioProducing);
		sw->WriteLine("# Repeated Symbols = ," + this->ruleStats[iRule]->countRepeatedSymbols);
		sw->WriteLine("# G2P transitions = ," + this->ruleStats[iRule]->countGraphical2ProducingSymbols);
		sw->WriteLine("# P2G transitions = ," + this->ruleStats[iRule]->countProducing2GraphicalSymbols);
		sw->Write("Unique Symbols: ");
		bool producesProducers = false;
		for (size_t i = 0; i < this->ruleStats[iRule]->uniqueSymbols->Count; i++)
		{
			sw->Write(this->ruleStats[iRule]->uniqueSymbols[i] + ",");
		}
		sw->WriteLine();
		sw->Write("Unique Graphics: ");
		for (size_t i = 0; i < this->ruleStats[iRule]->uniqueGraphics->Count; i++)
		{
			sw->Write(this->ruleStats[iRule]->uniqueGraphics[i] + ",");
		}
		sw->WriteLine();

		sw->WriteLine("Is producer? ," + this->ruleStats[iRule]->ruleIsProducer);
		sw->WriteLine("Produces producer? ," + this->ruleStats[iRule]->ruleProducesProducer);
	}

	sw->Close();
}


PlantModel::PlantModel()
{
	this->axiom = gcnew Word();
	this->rules = gcnew List<ProductionRule^>;
	this->vocabulary = gcnew Vocabulary(false);
}

PlantModel::~PlantModel()
{
}

void PlantModel::Analyze()
{
	this->turtleAlphabet = gcnew List<String^>;

	Int32 tmp = this->vocabulary->FindIndex("F");
	bool isFProducer = false;

	if (tmp < this->rules->Count)
	{
		if (this->rules[tmp]->successor->Count > 1)
		{
			isFProducer = true;
		}
	}

	if (!isFProducer)
	{
		this->turtleAlphabet->Add("F");
	}

	this->turtleAlphabet->Add("f");
	this->turtleAlphabet->Add("+");
	this->turtleAlphabet->Add("-");
	this->turtleAlphabet->Add("[");
	this->turtleAlphabet->Add("]");
	this->turtleAlphabet->Add("&");
	this->turtleAlphabet->Add("^");
	this->turtleAlphabet->Add("/");
	this->turtleAlphabet->Add("\\");
	this->turtleAlphabet->Add("!");

	this->modelStats = gcnew ModelStatistics();
	
	this->modelStats->avgRuleLength = 0.0;
	this->modelStats->avgRuleLengthCompressed = 0.0;
	this->modelStats->numRules = 0.0;

	this->modelStats->countDrawingRules = 0.0;
	this->modelStats->countProducingRules = 0.0; 
	this->modelStats->countTerminalRules = 0.0;

	this->modelStats->symbolPattern = gcnew array<Int32,2>(this->vocabulary->NumSymbols(), this->vocabulary->NumSymbols());
	this->modelStats->symbolPatternCompressed = gcnew array<Int32, 2>(2,2);

	for (size_t iRule = 0; iRule < this->rules->Count; iRule++)
	{
		this->modelStats->ruleStats->Add(gcnew RuleStatistics());
		this->modelStats->ruleStats[iRule]->ruleIsProducer = false;
		this->modelStats->ruleStats[iRule]->ruleIsGraphical = false;
		this->modelStats->ruleStats[iRule]->ruleIsTerminal = false;
		this->modelStats->ruleStats[iRule]->ruleProducesProducer= false;

		this->modelStats->ruleStats[iRule]->pattern = "";
		this->modelStats->ruleStats[iRule]->patternCompressed = "";
		this->modelStats->ruleStats[iRule]->uniqueSymbols = gcnew List<String^>;
		this->modelStats->ruleStats[iRule]->uniqueGraphics = gcnew List<String^>;

		this->modelStats->ruleStats[iRule]->length = 0.0;
		this->modelStats->ruleStats[iRule]->lengthCompressed = 0.0;
		this->modelStats->ruleStats[iRule]->countGraphicalSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->countProducingSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->countTerminalSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->countRepeatedSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->countGraphical2ProducingSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->countProducing2GraphicalSymbols = 0.0;
		this->modelStats->ruleStats[iRule]->ratioProducing = 0.0;

		this->modelStats->ruleStats[iRule]->length = this->rules[iRule]->successor->Count;

		bool prevSymbolGraphical = false;
		bool prevSymbolTerminal = false;
		bool prevSymbolBranch = false;

		String^ currentPattern = "";
		String^ prevPattern = "";
		float countConsecutive = 1.0;
		bool patternChange = false;

		String^ prevSymbol = "";
		Int32  prevIdx = 0;
		Int32  currentIdx = 0;

		for (size_t iSymbol = 0; iSymbol < this->rules[iRule]->successor->Count; iSymbol++)
		{
			// Fill in the symbol pattern
			if (prevSymbol != "")
			{
				currentIdx = this->vocabulary->FindIndex(this->rules[iRule]->successor[iSymbol]->Value());
				this->modelStats->symbolPattern[currentIdx, prevIdx]++;

				Int32 i1 = 0;
				Int32 i2 = 0;

				if ((this->vocabulary->Symbols[currentIdx]->isTerminal) || (turtleAlphabet->Contains(this->rules[iRule]->successor[iSymbol]->Value())))
				{
					i1 = 1;
				}

				if (prevSymbolGraphical || prevSymbolTerminal)
				{
					i2 = 1;
				}

				this->modelStats->symbolPatternCompressed[i1, i2]++;
			}

			// Process the symbol
			if (this->rules[iRule]->successor[iSymbol]->isTerminal)
			{
				if (!this->modelStats->ruleStats[iRule]->uniqueGraphics->Contains(this->rules[iRule]->successor[iSymbol]->Value()))
				{
					this->modelStats->ruleStats[iRule]->uniqueGraphics->Add(this->rules[iRule]->successor[iSymbol]->Value());
				}

				if (this->rules[iRule]->successor[iSymbol]->Value() != prevSymbol)
				{
					this->modelStats->ruleStats[iRule]->countTerminalSymbols++;
					this->modelStats->ruleStats[iRule]->lengthCompressed++;
				}

				if (((prevSymbolGraphical) || (prevSymbolTerminal)) && (!prevSymbolBranch))
				{
					countConsecutive++;
				}
				else
				{
					prevPattern = currentPattern;
					currentPattern = "G";
					patternChange = true;
				}

				prevSymbolGraphical = false;
				prevSymbolTerminal = true;
			}
			else if (this->turtleAlphabet->Contains(this->rules[iRule]->successor[iSymbol]->Value()))
			{
				if (!this->modelStats->ruleStats[iRule]->uniqueGraphics->Contains(this->rules[iRule]->successor[iSymbol]->Value()))
				{
					this->modelStats->ruleStats[iRule]->uniqueGraphics->Add(this->rules[iRule]->successor[iSymbol]->Value());
				}

				if ((!prevSymbolGraphical) && (iSymbol != 0))
				{
					this->modelStats->ruleStats[iRule]->countProducing2GraphicalSymbols++;
				}

				if (this->rules[iRule]->successor[iSymbol]->Value() != prevSymbol)
				{
					if (this->rules[iRule]->successor[iSymbol]->Value() == "[")
					{
						prevPattern = currentPattern;
						currentPattern = "SB";
						patternChange = true;
						prevSymbolBranch = true;
					}
					else if (this->rules[iRule]->successor[iSymbol]->Value() == "]")
					{
						prevPattern = currentPattern;
						currentPattern = "EB";
						patternChange = true;
						prevSymbolBranch = true;
					}
					else if (((prevSymbolGraphical) || (prevSymbolTerminal)) && (!prevSymbolBranch))
					{
						countConsecutive++;
					}
					else
					{
						prevPattern = currentPattern;
						currentPattern = "G";
						patternChange = true;
					}

					this->modelStats->ruleStats[iRule]->countGraphicalSymbols++;
					this->modelStats->ruleStats[iRule]->lengthCompressed++;
				}

				prevSymbolGraphical = true;
				prevSymbolTerminal = false;
			}
			else
			{
				if (!this->modelStats->ruleStats[iRule]->uniqueSymbols->Contains(this->rules[iRule]->successor[iSymbol]->Value()))
				{
					this->modelStats->ruleStats[iRule]->uniqueSymbols->Add(this->rules[iRule]->successor[iSymbol]->Value());
				}

				if ((prevSymbolGraphical) && (iSymbol != 0))
				{
					this->modelStats->ruleStats[iRule]->countGraphical2ProducingSymbols++;
				}

				if (this->rules[iRule]->successor[iSymbol]->Value() == prevSymbol)
				{
					this->modelStats->ruleStats[iRule]->countRepeatedSymbols++;
				}

				if ((!prevSymbolGraphical) && (!prevSymbolTerminal) && (iSymbol != 0))
				{
					countConsecutive++;
				}
				else
				{
					prevPattern = currentPattern;
					currentPattern = "S";
					patternChange = true;
				}

				prevSymbolGraphical = false;
				prevSymbolTerminal = false;
				this->modelStats->ruleStats[iRule]->countProducingSymbols++;
				this->modelStats->ruleStats[iRule]->lengthCompressed++;
			}

			if ((patternChange) && (iSymbol != 0))
			{
				if ((prevPattern != "SB") && (prevPattern != "EB"))
				{
					prevPattern = countConsecutive + prevPattern + " ";
				}
				else
				{
					prevPattern = prevPattern + " ";
				}

				this->modelStats->ruleStats[iRule]->pattern = this->modelStats->ruleStats[iRule]->pattern + prevPattern;
				countConsecutive = 1;
				patternChange = false;
			}
			else if (iSymbol == 0)
			{
				patternChange = false;
			}

			if (iSymbol == this->rules[iRule]->successor->Count - 1)
			{
				if ((currentPattern != "SB") && (currentPattern != "EB"))
				{
					currentPattern = countConsecutive + currentPattern + " ";
				}
				else
				{
					currentPattern = currentPattern + " ";
				}

				this->modelStats->ruleStats[iRule]->pattern = this->modelStats->ruleStats[iRule]->pattern + currentPattern;
				countConsecutive = 1;
				patternChange = false;
			}

			prevSymbol = this->rules[iRule]->successor[iSymbol]->Value();
			prevIdx = currentIdx;
		}

		this->modelStats->ruleStats[iRule]->patternCompressed = this->CompressPattern(this->modelStats->ruleStats[iRule]->pattern);

		this->modelStats->avgRuleLength += this->modelStats->ruleStats[iRule]->length;
		this->modelStats->avgRuleLengthCompressed += this->modelStats->ruleStats[iRule]->lengthCompressed;

		this->modelStats->ruleStats[iRule]->ratioProducing = this->modelStats->ruleStats[iRule]->countProducingSymbols / (this->modelStats->ruleStats[iRule]->countGraphicalSymbols + this->modelStats->ruleStats[iRule]->countProducingSymbols + this->modelStats->ruleStats[iRule]->countTerminalSymbols);

		if (this->modelStats->ruleStats[iRule]->length == 1 && this->modelStats->ruleStats[iRule]->ratioProducing == 0)
		{
			this->modelStats->ruleStats[iRule]->ruleIsTerminal = true;
			this->modelStats->countTerminalRules++;
		}
		else if (this->modelStats->ruleStats[iRule]->ratioProducing == 0)
		{
			this->modelStats->ruleStats[iRule]->ruleIsGraphical = true;
			this->modelStats->countDrawingRules++;
			this->modelStats->numRules++;
		}
		else
		{
			this->modelStats->ruleStats[iRule]->ruleIsProducer = true;
			this->modelStats->countProducingRules++;
			this->modelStats->numRules++;
		}
	}

	this->modelStats->avgRuleLength = this->modelStats->avgRuleLength / this->modelStats->numRules;
	this->modelStats->avgRuleLengthCompressed = this->modelStats->avgRuleLengthCompressed / this->modelStats->numRules;

	for (size_t iRule = 0; iRule < this->rules->Count; iRule++)
	{
		for (size_t i = 0; i < this->modelStats->ruleStats[iRule]->uniqueSymbols->Count; i++)
		{
			Int32 idx = this->vocabulary->FindIndex(this->modelStats->ruleStats[iRule]->uniqueSymbols[i]);
			this->modelStats->ruleStats[iRule]->ruleProducesProducer = this->modelStats->ruleStats[iRule]->ruleProducesProducer || this->modelStats->ruleStats[idx]->ruleIsProducer;
		}
	}
}

String^ PlantModel::CompressPattern(String^ P)
{
	String^ result = "";
	array<Char>^ seperator = gcnew array<Char>{' '};
	array<String^, 1>^ words;
	words = P->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);
	String^ prev = "";

	for (size_t iWord = 0; iWord <= words->GetUpperBound(0); iWord++)
	{
		String^ w = words[iWord];
		String^ last = w->Substring(w->Length - 1, 1);

		if (last == "B") {
			if (w == "SB")
			{
				result += "B";
			}
			else
			{
				result += "E";
			}
		}
		else {
			if (last != prev)
			{
				result += last;
			}
			prev = last;
		}
	}

	return result;
}

void PlantModel::CreateEvidence()
{

#if _PMI_PLANT_MODEL_VERBOSE_ >= 1
	Console::WriteLine("Problem");
	Console::WriteLine(" Axiom = " + this->axiom->ToString());
	Console::WriteLine(" Rules: ");
	for each (ProductionRule^ r in this->rules)
	{
		Console::WriteLine("   " + r->ToString());
	}

	Console::WriteLine();
#endif

	// Create the solution
	this->evidence = gcnew List<Word^>;

	// Create the evidence for this solution
	Word^ a = gcnew Word();
	for each (Symbol^ s in this->axiom->symbols)
	{
		a->Add(s);
	}

	this->evidence = EvidenceFactory::Process(a, this->rules, 0, this->generations, false);

	for each (Word^ a in this->evidence)
	{
		this->vocabulary->CompressSymbols(a);
	}

	List<Symbol^>^ constants = this->vocabulary->AllConstants();

	// Make a copy of all the evidence while removing the constants
	this->evidenceModuleOnly = gcnew List<Word^>;
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		a->FilterOutSymbol(constants);
		this->vocabulary->CompressSymbols(a);
		this->evidenceModuleOnly->Add(a);
	}

#if _PMI_PLANT_MODEL_VERBOSE_ >= 2
	for (size_t i = 0; i < this->generations; i++)
	{
		Console::WriteLine("Evidence #" + i.ToString(L"G") + " : " + this->evidence[i]->ToString());
	}
	Console::ReadLine();
#endif
}

void PlantModel::Display()
{
	Console::WriteLine("Vocabulary");
	Console::Write("Modules:   ");
	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	{
		Console::Write(this->vocabulary->Symbols[iSymbol] + ",");
	}
	Console::WriteLine();

	Console::Write("Constants: ");
	for (size_t iSymbol = this->vocabulary->IndexConstantsStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
	{
		Console::Write(this->vocabulary->Symbols[iSymbol] + ",");
	}
	Console::WriteLine();

	Console::WriteLine("Rules");
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		this->rules[iRule]->Display();
	}
}

void PlantModel::Load(String^ FileName)
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + FileName);

	array<Char>^ seperator = gcnew array<Char>{','};
	ConfigFileLine lineNum = PlantModel::ConfigFileLine::VocabularyFile;

	int valueI;
	float valueF;
	bool valueB;
	Int32 ruleNum = 0;
	//this->successorModules = gcnew List<Int32>;
	//this->successorConstants = gcnew List<Int32>;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();

		if (line != "")
		{
			array<String^, 1>^ words;

			//Console::WriteLine("Processing Config Line # " + ((int)lineNum).ToString() + " : " + line);

			words = line->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);

			if ((int)lineNum == (int)PlantModel::ConfigFileLine::VocabularyFile)
			{
				this->vocabulary = gcnew Vocabulary(false);
				array<String^, 1>^ parts = words[1]->Split(gcnew array<Char>{'|'});
				array<String^, 1>^ modules = parts[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
				array<String^, 1>^ constants = parts[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);

				this->numModules = modules->Length;
				this->numConstants = constants->Length;

				for (size_t i = 0; i < modules->Length; i++)
				{
					this->vocabulary->AddSymbol(modules[i], true);
				}

				for (size_t i = 0; i < constants->Length; i++)
				{
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ == -1
					if ((constants[i] == "[") || (constants[i] == "]") || (constants[i] == "{") || (constants[i] == "}") || (constants[i] == "+") || (constants[i] == "-"))
					{
						this->vocabulary->AddSymbol(constants[i], false);
					}
					else
					{
						this->vocabulary->AddSymbol(constants[i], true);
					}
#endif

#if _PMI_PROBLEM_KNOWN_CONSTANTS_ >= 0
					this->vocabulary->AddSymbol(constants[i], false);
#endif

				}

				this->vocabulary->ComputeIndices();

				// Add the alpha symbol
				//this->vocabulary->AddSymbol(L"\u03B1");
			}
			else if ((int)lineNum == (int)PlantModel::ConfigFileLine::Generations)
			{
				if (words[1] == "*")
				{
					this->generations = (this->numModules) + 1;
				}
				else
				{
					int::TryParse(words[1], valueI);
					this->generations = valueI;
				}
			}
			else if ((int)lineNum == (int)PlantModel::ConfigFileLine::Axiom)
			{
				array<String^, 1>^ symbols;
				if (words->Length == 1)
				{
					symbols = words[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
				}
				else
				{
					symbols = words;
				}

				for (size_t i = 0; i < symbols[1]->Length; i++)
				{
					bool found = false;
					Int32 iSymbol = 0;
					String^ symbol = symbols[1]->Substring(i, 1);

					while ((iSymbol < this->vocabulary->Symbols->Count) && (!found))
					{
						if (symbol == this->vocabulary->Symbols[iSymbol]->Value())
						{
							this->axiom->Add(this->vocabulary->Symbols[iSymbol]);
							found = true;
						}
						iSymbol++;
					}

					if (!found)
					{
						throw gcnew SymbolNotFoundException("Symbol " + symbols[i] + " not found in vocabulary");
					}
				}
			}
			else if ((int)lineNum >= (int)PlantModel::ConfigFileLine::Rules)
			{
				this->LoadRule(words);
				ruleNum++;
			}
		}

		lineNum++;
	}

	sr->Close();

	this->numRules = this->rules->Count;
}

void PlantModel::LoadRule(array<String^, 1>^ Words)
{
	List<IProductionRuleConstraint^>^ constraints = gcnew List<IProductionRuleConstraint^>;
	// array<String^, 1>^ parts = Words[PlantModel::cRuleSymbols]->Split(gcnew array<Char>{'|'}, System::StringSplitOptions::RemoveEmptyEntries); /// version 2
	array<String^, 1>^ parts = Words[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries); // version 3
 
/*	 Version 2
array<String^, 1>^ symbols = parts[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
	String^ p = symbols[0]; // the activating symbol of the predecessor is always in the 1st spot
	Symbol^ predecessor = this->vocabulary->FindSymbol(p);
	*/

	String^ p = parts[3]; // the activating symbol of the predecessor is always in the 1st spot
	Symbol^ predecessor = this->vocabulary->FindSymbol(p);
	array<String^, 1>^ symbols = parts[7]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);

	// There is a constraint or parameter
	if (symbols->Length > 1)
	{
		// If the second symbol is a ( then it is a parameter list
		// If the second symbol is < or > then it is a context constraint
		if (symbols[1] == "(")
		{

		}
		else if ((symbols[1] == "<") || (symbols[1] == ">"))
		{
			String^ c = symbols[2];
			Symbol^ contextSymbol = this->vocabulary->FindSymbol(c);
			if (symbols[1] == "<")
			{
				ProductionRuleConstraint_ContextLeft^ contextConstraint = gcnew ProductionRuleConstraint_ContextLeft(contextSymbol);
				constraints->Add(contextConstraint);
			}
			else if (symbols[1] == ">")
			{
				ProductionRuleConstraint_ContextRight^ contextConstraint = gcnew ProductionRuleConstraint_ContextRight(contextSymbol);
				constraints->Add(contextConstraint);
			}
		}
	}

	// Create the operation. Currently only Yields operations are used.
	Symbol^ operation = this->vocabulary->YieldsSymbol;

	// Create the successor
	List<Symbol^>^ successor = gcnew List<Symbol^>(0);
	String^ succ = parts[7];

	//symbols = parts[7]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
	for (size_t i = 0; i < succ->Length; i++)
	{
		successor->Add(this->vocabulary->FindSymbol(succ->Substring(i,1)));
	}

	if ((successor->Count == 1) && (successor[0] == predecessor))
	{
		Int32 idx = this->vocabulary->FindIndex(predecessor);
		this->vocabulary->Symbols[idx]->isTerminal = true;
	}

	// Create a time constraint if there is any
	array<String^, 1>^ genConstraint = parts[6]->Split(gcnew array<Char>{'|'}, System::StringSplitOptions::RemoveEmptyEntries);;

	Int32 start = 0;
	Int32 end = ProductionRule::cEndOfTime;

	if ((genConstraint[0] != "*") && (genConstraint[1] == "*"))
	{
		int::TryParse(genConstraint[0], start);
		end = ProductionRule::cEndOfTime;
	}
	else if ((genConstraint[0] != "*") && (genConstraint[1] != "*"))
	{
		int::TryParse(genConstraint[0], start);
		int::TryParse(genConstraint[1], end);
	}

	ProductionRuleConstraint_Time^ constraint = gcnew ProductionRuleConstraint_Time(start, end);
	constraints->Add(constraint);

	// Create the production
	ProductionRule^ r = gcnew ProductionRule(predecessor, operation, successor, constraints);

	// Add it to the rule set
	this->rules->Add(r);
}

void PlantModel::Save()
{
	String^ dateString = DateTime::Now.ToString("d");
	String^ timeString = DateTime::Now.Hour.ToString() + DateTime::Now.Minute.ToString();

	String^ path = "F://PMIT//random_models//random_" + this->vocabulary->numModules + "_" + dateString + "_" + timeString + CommonGlobals::PoolInt(0,9999999).ToString() + ".txt";

	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(path, false);

	sw->WriteLine("Vocabulary");
	sw->Write("Modules:   ");
	for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	{
		sw->Write(this->vocabulary->Symbols[iSymbol] + ",");
	}
	sw->WriteLine();

	sw->Write("Constants: ");
	for (size_t iSymbol = this->vocabulary->IndexConstantsStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
	{
		sw->Write(this->vocabulary->Symbols[iSymbol] + ",");
	}
	sw->WriteLine();

	sw->WriteLine("Rules");
	for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	{
		sw->Write(this->rules[iRule]->predecessor->Value() + " => ");
		for (size_t iSymbol = 0; iSymbol < this->rules[iRule]->successor->Count; iSymbol++)
		{
			sw->Write(this->rules[iRule]->successor[iSymbol]);
		}
		sw->WriteLine();
	}

	sw->Close();
}

bool PlantModel::IsMatch(PlantModel^ Model)
{
	bool result = true;

	Int32 iRule1 = 0;

	do
	{
		for (size_t iRule2 = 0; iRule2 < Model->rules->Count; iRule2++)
		{
			// if the predecessor are the same and the lengths match
			if (this->rules[iRule1]->predecessor == Model->rules[iRule2]->predecessor)
			{
				if (this->rules[iRule1]->successor->Count == Model->rules[iRule2]->successor->Count)
				{
					for (size_t iSymbol = 0; iSymbol < this->rules[iRule1]->successor->Count; iSymbol++)
					{
						if (this->rules[iRule1]->successor[iSymbol] != Model->rules[iRule2]->successor[iSymbol])
						{
							result = false;
						}
					}
				}
				else
				{
					result = false;
				}
			}
		}

		iRule1++;
	} while ((iRule1 < this->rules->Count) && (result));

	return result;
}

bool PlantModel::Validate()
{
	bool result = true;
	// check to ensure every symbol appears at least once
	List<bool>^ used = gcnew List<bool>;

	for (size_t i = 0; i < this->vocabulary->numModules; i++)
	{
		used->Add(false);
	}

	for (size_t iWord = 0; iWord < this->evidence->Count; iWord++)
	{
		for (size_t iPos = 0; iPos < this->evidence[iWord]->symbols->Count; iPos++)
		{
			Symbol^ S = this->evidence[iWord]->symbols[iPos];
			if (this->vocabulary->IsModule(S))
			{
				Int32 idx = this->vocabulary->FindIndex(S);
				used[idx] = true;
			}
		}
	}

	for (size_t i = 0; i < this->numModules; i++)
	{
		result = result && used[i];
	}

	return result;
}