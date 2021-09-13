#include "stdafx.h"
#include "PMIModelGeneratorV3.h"

GeneratorStatistics::GeneratorStatistics()
{
	this->distributionRuleLengths = gcnew List<float>;
	this->distributionRuleLengthsProducing = gcnew List<float>;
	this->distributionRuleLengthsGraphical = gcnew List<float>;
	this->patterns = gcnew List<String^>;
	this->patternsCompressed = gcnew List<String^>;

	for (size_t i = 1; i < 100; i++)
	{
		this->distributionRuleLengths->Add(0);
		this->distributionRuleLengthsProducing->Add(0);
		this->distributionRuleLengthsGraphical->Add(0);
	}

	this->avgNumRulesProducing = 0.0;
	this->avgNumRulesDrawing = 0.0;
	this->avgNumRulesTerminal = 0.0;
	this->maxNumRules = 0.0;

	this->ratioProducingRules = 0.0;

	this->avgRepeatedSymbols = 0.0;
	this->minRepeatedSymbols = 99999.0;
	this->maxRepeatedSymbols = 0.0;

	this->avgG2PTransitions = 0.0;
	this->minG2PTransitions = 99999.0;
	this->maxG2PTransitions = 0.0;

	this->avgP2GTransitions = 0.0;
	this->minP2GTransitions = 99999.0;
	this->maxP2GTransitions = 0.0;

	patternsIsProducing = gcnew List<bool>;
	patternsIsGraphical = gcnew List<bool>;
	patternsIsTerminal = gcnew List<bool>;
	patternsLength = gcnew List<Int32>;
	patternsLengthCompressed = gcnew List<Int32>;
	patternsNumProducingSymbols = gcnew List<Int32>;
	patternsNumGraphicalSymbols = gcnew List<Int32>;
	patternsNumTerminalSymbols = gcnew List<Int32>;
	patternsP2GRatio = gcnew List<float>;
	patterns = gcnew List<String^>;

	patternsCompressedIsProducing = gcnew List<bool>;
	patternsCompressedIsGraphical = gcnew List<bool>;
	patternsCompressedIsTerminal = gcnew List<bool>;
	patternsCompressedLength = gcnew List<Int32>;
	patternsCompressedLengthCompressed = gcnew List<Int32>;
	patternsCompressedNumProducingSymbols = gcnew List<Int32>;
	patternsCompressedNumGraphicalSymbols = gcnew List<Int32>;
	patternsCompressedNumTerminalSymbols = gcnew List<Int32>;
	patternsCompressedP2GRatio = gcnew List<float>;
	patternsCompressed = gcnew List<String^>;
}

GeneratorStatistics::~GeneratorStatistics()
{

}

void GeneratorStatistics::Load(String^ FileName)
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + FileName);

	array<Char>^ seperator = gcnew array<Char>{','};
	ConfigFileLine lineNum = GeneratorStatistics::ConfigFileLine::NumRules;

	int valueI;
	float valueF;
	bool valueB;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();

		if (line != "")
		{
			array<String^, 1>^ words;

			words = line->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);

			if (words[0] == "Length")
			{
				lineNum++;
			}
			else if (words[0] == "Patterns")
			{
				lineNum++;
			}
			else if (words[0] == "Compressed Patterns")
			{
				lineNum++;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::NumRules)
			{
				float::TryParse(words[1], valueF);
				this->maxNumRules = valueF;
				float::TryParse(words[3], valueF);
				this->avgNumRulesProducing = valueF;
				float::TryParse(words[5], valueF);
				this->avgNumRulesDrawing = valueF;
				float::TryParse(words[7], valueF);
				this->avgNumRulesTerminal = valueF;

				lineNum++;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::P2GRuleRatio)
			{
				float::TryParse(words[1], valueF);
				this->ratioProducingRules = valueF;

				lineNum++;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::G2PTransitions)
			{
				float::TryParse(words[1], valueF);
				this->avgG2PTransitions = valueF;
				float::TryParse(words[3], valueF);
				this->minG2PTransitions = valueF;
				float::TryParse(words[5], valueF);
				this->maxG2PTransitions = valueF;

				lineNum++;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::P2GTransitions)
			{
				float::TryParse(words[1], valueF);
				this->avgP2GTransitions = valueF;
				float::TryParse(words[3], valueF);
				this->minP2GTransitions = valueF;
				float::TryParse(words[5], valueF);
				this->maxP2GTransitions = valueF;

				lineNum++;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::RepeatedSymbols)
			{
				float::TryParse(words[1], valueF);
				this->avgRepeatedSymbols = valueF;
				float::TryParse(words[3], valueF);
				this->minRepeatedSymbols = valueF;
				float::TryParse(words[5], valueF);
				this->maxRepeatedSymbols = valueF;
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::LengthDistribution)
			{
				int::TryParse(words[1], valueI);
				this->distributionRuleLengths->Add(valueI);
				int::TryParse(words[2], valueI);
				this->distributionRuleLengthsProducing->Add(valueI);
				int::TryParse(words[3], valueI);
				this->distributionRuleLengthsGraphical->Add(valueI);
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::Patterns)
			{
				bool::TryParse(words[1], valueB);
				this->patternsIsProducing->Add(valueB);
				bool::TryParse(words[2], valueB);
				this->patternsIsGraphical->Add(valueB);
				bool::TryParse(words[3], valueB);
				this->patternsIsTerminal->Add(valueB);
				int::TryParse(words[4], valueI);
				this->patternsLength->Add(valueI);
				int::TryParse(words[5], valueI);
				this->patternsLengthCompressed->Add(valueI);
				int::TryParse(words[6], valueI);
				this->patternsNumProducingSymbols->Add(valueI);
				int::TryParse(words[7], valueI);
				this->patternsNumGraphicalSymbols->Add(valueI);
				int::TryParse(words[8], valueI);
				this->patternsNumTerminalSymbols->Add(valueI);
				float::TryParse(words[9], valueF);
				this->patternsP2GRatio->Add(valueI);
				this->patterns->Add(words[10]);
			}
			else if ((int)lineNum == (int)GeneratorStatistics::ConfigFileLine::CompressedPatterns)
			{
				bool::TryParse(words[1], valueB);
				this->patternsCompressedIsProducing->Add(valueB);
				bool::TryParse(words[2], valueB);
				this->patternsCompressedIsGraphical->Add(valueB);
				bool::TryParse(words[3], valueB);
				this->patternsCompressedIsTerminal->Add(valueB);
				int::TryParse(words[4], valueI);
				this->patternsCompressedLength->Add(valueI);
				int::TryParse(words[5], valueI);
				this->patternsCompressedLengthCompressed->Add(valueI);
				int::TryParse(words[6], valueI);
				this->patternsCompressedNumProducingSymbols->Add(valueI);
				int::TryParse(words[7], valueI);
				this->patternsCompressedNumGraphicalSymbols->Add(valueI);
				int::TryParse(words[8], valueI);
				this->patternsCompressedNumTerminalSymbols->Add(valueI);
				float::TryParse(words[9], valueF);
				this->patternsCompressedP2GRatio->Add(valueI);
				this->patternsCompressed->Add(words[10]);
			}
		}
	}
}

PMIModelGeneratorV3::PMIModelGeneratorV3(GeneratorStatistics^ Stats)
{
	this->stats = Stats;

	this->symbolPool = gcnew List<String^>;
	this->specialPool = gcnew List<String^>;

	this->symbolPool->Add("A");
	this->symbolPool->Add("B");
	this->symbolPool->Add("C");
	this->symbolPool->Add("D");
	this->symbolPool->Add("E");
	this->symbolPool->Add("G");
	this->symbolPool->Add("H");
	this->symbolPool->Add("I");
	this->symbolPool->Add("J");
	this->symbolPool->Add("K");
	this->symbolPool->Add("L");
	this->symbolPool->Add("M");
	this->symbolPool->Add("N");
	this->symbolPool->Add("O");
	this->symbolPool->Add("P");
	this->symbolPool->Add("Q");
	this->symbolPool->Add("R");
	this->symbolPool->Add("S");
	this->symbolPool->Add("T");
	this->symbolPool->Add("U");
	this->symbolPool->Add("V");
	this->symbolPool->Add("W");
	this->symbolPool->Add("X");
	this->symbolPool->Add("Y");
	this->symbolPool->Add("Z");
	this->symbolPool->Add("a");
	this->symbolPool->Add("b");
	this->symbolPool->Add("c");
	this->symbolPool->Add("d");
	this->symbolPool->Add("e");
	this->symbolPool->Add("g");
	this->symbolPool->Add("h");
	this->symbolPool->Add("i");
	this->symbolPool->Add("j");
	this->symbolPool->Add("k");
	this->symbolPool->Add("l");
	this->symbolPool->Add("m");
	this->symbolPool->Add("n");
	this->symbolPool->Add("o");
	this->symbolPool->Add("p");
	this->symbolPool->Add("q");
	this->symbolPool->Add("r");
	this->symbolPool->Add("s");
	this->symbolPool->Add("t");
	this->symbolPool->Add("u");
	this->symbolPool->Add("v");
	this->symbolPool->Add("w");
	this->symbolPool->Add("x");
	this->symbolPool->Add("y");
	this->symbolPool->Add("z");

	this->specialPool->Add("?");
	this->specialPool->Add("%");
	this->specialPool->Add("@");
	this->specialPool->Add("~");
	this->specialPool->Add("(");
	this->specialPool->Add(")");
	this->specialPool->Add("{");
	this->specialPool->Add("}");
	this->specialPool->Add(";");
	this->specialPool->Add(":");
}


PMIModelGeneratorV3::~PMIModelGeneratorV3()
{
}

ModelV3^ PMIModelGeneratorV3::BuildD0L()
{
	this->model = gcnew ModelV3();

	// Build the alphabet
	Int32 numSymbols = CommonGlobals::PoolInt(2, 2 * this->stats->maxNumRules);
	float P2GRuleRatio = CommonGlobals::PoolFloat(this->stats->ratioProducingRules - 0.2, this->stats->ratioProducingRules + 0.2);
	float G2TRuleRatio = CommonGlobals::PoolFloat(0, 0.5);

	if (P2GRuleRatio < 0.33)
	{
		P2GRuleRatio = 0.33;
	}
	else if (P2GRuleRatio > 1)
	{
		P2GRuleRatio = 1.0;
	}

	
	Int32 numProducingSymbols = numSymbols * P2GRuleRatio;
	Int32 numDrawingSymbols = numSymbols - numProducingSymbols;
	Int32 numTerminalSymbols = numDrawingSymbols * G2TRuleRatio;
	numDrawingSymbols -= numTerminalSymbols;
	for (size_t iSymbol = 0; iSymbol < numProducingSymbols; iSymbol++)
	{
		this->model->alphabet->AddSymbol(symbolPool[iSymbol], symbolPool[iSymbol], AlphabetV3::UnknownSuccessor, true);
	}

	for (size_t iSymbol = 0; iSymbol < numTerminalSymbols; iSymbol++)
	{
		this->model->alphabet->AddSymbol(specialPool[iSymbol], specialPool[iSymbol], AlphabetV3::UnknownSuccessor, true);
	}
	this->model->alphabet->AddBranching();
	this->model->alphabet->Add2DTurtleGraphics();
	this->model->alphabet->ComputeIndices();

	this->model->axiom->SetSymbols(this->model->alphabet->symbols[0]);
	this->model->alphabet->CreateParikhVector(this->model->axiom);

	ProductionTableV3^ t = gcnew ProductionTableV3();
	t->rules->Add(this->BuildProducingRule("A"));
	t->rules->Add(this->BuildDrawingRule("B"));

	this->model->productions->Add(t);

	//this->model->generations = this->model->alphabet->numSymbols + 1;
	this->model->generations = this->model->alphabet->numSymbols + 3;

	this->model->CreateEvidence();
	this->model->Display();

	Console::WriteLine("Paused");
	Console::ReadLine();

	return this->model;
}

ProductionRuleV3^ PMIModelGeneratorV3::BuildDrawingRule(String^ Predecessor)
{
	ProductionRuleV3^ r = gcnew ProductionRuleV3();
 
	r->predecessor = Predecessor;
	r->successor->Add("FFA");
	r->condition->Add("");
	r->contextLeft->Add("*");
	r->contextRight->Add("*");
	r->odds->Add(100);
	//r->timeStart->Add(0);
	//r->timeEnd->Add(999);

	return r;
}

ProductionRuleV3^ PMIModelGeneratorV3::BuildProducingRule(String^ Predecessor)
{
	ProductionRuleV3^ r = gcnew ProductionRuleV3();

	r->predecessor = Predecessor;
	r->successor->Add("ABA");
	r->condition->Add("");
	r->contextLeft->Add("F");
	r->contextRight->Add("*");
	r->odds->Add(100);
	//r->timeStart->Add(0);
	//r->timeEnd->Add(999);

	r->predecessor = Predecessor;
	r->successor->Add("AB");
	r->condition->Add("");
	r->contextLeft->Add("*");
	r->contextRight->Add("*");
	r->odds->Add(100);
	//r->timeStart->Add(0);
	//r->timeEnd->Add(999);

	return r;
}