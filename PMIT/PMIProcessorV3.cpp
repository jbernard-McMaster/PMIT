#include "stdafx.h"
#include "PMIProcessorV3.h"

PMIProcessorV3::PMIProcessorV3()
{
	this->problems = gcnew List<Int32>;
	this->metrics = gcnew List<PMIPerformanceMetrics^>;
}

PMIProcessorV3::~PMIProcessorV3()
{
}

GeneratorStatistics^ PMIProcessorV3::Analyze()
{
	GeneratorStatistics^ result;
	String^ problemFile;
	String^ folder = "./";

	array<String^>^ files = Directory::GetFiles(folder, "0*problem*.txt");
	List<ModelStatistics^>^ stats = gcnew List<ModelStatistics^>;


	for (size_t iStyle = 0; iStyle < 3; iStyle++)
	{
		// Don't do the first one because it is the sample V3
		// Don't do the last one because it will be the random one
		List<Int32>^ style = gcnew List<Int32>;
		String^ fileName = "";
		if (iStyle == 0)
		{
			fileName = "fractal_style_stats1.csv";
			// Fractal Style
			style->Add(1);
			style->Add(2);
			style->Add(3);
			style->Add(4);
			style->Add(5);
			style->Add(6);
			style->Add(7);
			style->Add(8);
			style->Add(9);
			style->Add(10);
			style->Add(11);
		}
		else if (iStyle == 1)
		{
			fileName = "PDcombined_style_stats2.csv";
			// Combined Style
			style->Add(12);
			style->Add(13);
			style->Add(15);
			style->Add(22);
			style->Add(23);
			style->Add(24);
		}
		else if (iStyle == 2)
		{
			fileName = "PDseparated_style_stats3.csv";
			// Separated Style
			style->Add(14);
			style->Add(16);
			style->Add(17);
			style->Add(18);
			style->Add(19);
			style->Add(20);
			style->Add(21);
		}

		for (size_t iProblem = 1; iProblem < files->GetUpperBound(0); iProblem++)
		{
			if (style->Contains(iProblem))
			{
				problemFile = files[iProblem];

				PlantModel^ m = gcnew PlantModel();
				m->Load(problemFile);
				m->Analyze();
				stats->Add(m->modelStats);

				Console::WriteLine();
				Console::WriteLine("Problem: " + problemFile);
				m->modelStats->Display();
				m->modelStats->Write(problemFile->Substring(0, problemFile->Length - 4) + "_stats.csv");
			}

			result = this->CompileStats(stats, fileName);
		}
	}

	return result;
}

GeneratorStatistics^ PMIProcessorV3::CompileStats(List<ModelStatistics^>^ Stats, String^ FileName)
{
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("./" + FileName, false);

	GeneratorStatistics^ genStats = gcnew GeneratorStatistics();
	float totalRulesNonTerminal = 0.0;

	List<String^>^ outputPatterns = gcnew List<String^>;
	List<String^>^ outputPatternsCompressed = gcnew List<String^>;
	List<String^>^ completedPatterns = gcnew List<String^>;
	List<String^>^ completedPatternsCompressed = gcnew List<String^>;
	
	for (size_t iStat = 0; iStat < Stats->Count; iStat++)
	{
		float countRules = 0.0;
		for (size_t iRule = 0; iRule < Stats[iStat]->ruleStats->Count; iRule++)
		{
			String^ oPattern = Stats[iStat]->ruleStats[iRule]->pattern;
			oPattern = Stats[iStat]->ruleStats[iRule]->ruleIsProducer + "," + Stats[iStat]->ruleStats[iRule]->ruleIsGraphical + "," + Stats[iStat]->ruleStats[iRule]->ruleIsTerminal +
				"," + Stats[iStat]->ruleStats[iRule]->length + "," + Stats[iStat]->ruleStats[iRule]->lengthCompressed +
				"," + Stats[iStat]->ruleStats[iRule]->countProducingSymbols + "," + Stats[iStat]->ruleStats[iRule]->countGraphicalSymbols + "," + Stats[iStat]->ruleStats[iRule]->countTerminalSymbols +
				"," + Stats[iStat]->ruleStats[iRule]->ratioProducing +
				"," + oPattern;

			if (!completedPatterns->Contains(oPattern))
			{
				genStats->patterns->Add(oPattern);
				completedPatterns->Add(oPattern);
			}

				String^ oPatternCompressed = Stats[iStat]->ruleStats[iRule]->patternCompressed;
				oPatternCompressed = Stats[iStat]->ruleStats[iRule]->ruleIsProducer + "," + Stats[iStat]->ruleStats[iRule]->ruleIsGraphical + "," + Stats[iStat]->ruleStats[iRule]->ruleIsTerminal +
					"," + Stats[iStat]->ruleStats[iRule]->length + "," + Stats[iStat]->ruleStats[iRule]->lengthCompressed +
					"," + Stats[iStat]->ruleStats[iRule]->countProducingSymbols + "," + Stats[iStat]->ruleStats[iRule]->countGraphicalSymbols + "," + Stats[iStat]->ruleStats[iRule]->countTerminalSymbols +
					"," + Stats[iStat]->ruleStats[iRule]->ratioProducing +
					"," + oPatternCompressed;

			if (!completedPatternsCompressed->Contains(oPatternCompressed))
			{
				genStats->patternsCompressed->Add(oPatternCompressed);
				completedPatternsCompressed->Add(oPatternCompressed);
			}

			// compute transition statistics for producing and drawing rules
			if ((Stats[iStat]->ruleStats[iRule]->ruleIsProducer) || (Stats[iStat]->ruleStats[iRule]->ruleIsGraphical))
			{
				genStats->avgG2PTransitions += Stats[iStat]->ruleStats[iRule]->countGraphical2ProducingSymbols;
				genStats->avgP2GTransitions += Stats[iStat]->ruleStats[iRule]->countProducing2GraphicalSymbols;
				genStats->avgRepeatedSymbols += Stats[iStat]->ruleStats[iRule]->countRepeatedSymbols;

				if (Stats[iStat]->ruleStats[iRule]->countGraphical2ProducingSymbols < genStats->minG2PTransitions)
				{
					genStats->minG2PTransitions = Stats[iStat]->ruleStats[iRule]->countGraphical2ProducingSymbols;
				}
				
				if (Stats[iStat]->ruleStats[iRule]->countGraphical2ProducingSymbols > genStats->maxG2PTransitions)
				{
					genStats->maxG2PTransitions = Stats[iStat]->ruleStats[iRule]->countGraphical2ProducingSymbols;
				}

				if (Stats[iStat]->ruleStats[iRule]->countProducing2GraphicalSymbols < genStats->minP2GTransitions)
				{
					genStats->minP2GTransitions = Stats[iStat]->ruleStats[iRule]->countProducing2GraphicalSymbols;
				}

				if (Stats[iStat]->ruleStats[iRule]->countProducing2GraphicalSymbols > genStats->maxP2GTransitions)
				{
					genStats->maxP2GTransitions = Stats[iStat]->ruleStats[iRule]->countProducing2GraphicalSymbols;
				}

				if (Stats[iStat]->ruleStats[iRule]->countRepeatedSymbols < genStats->minRepeatedSymbols)
				{
					genStats->minRepeatedSymbols = Stats[iStat]->ruleStats[iRule]->countRepeatedSymbols;
				}

				if (Stats[iStat]->ruleStats[iRule]->countRepeatedSymbols > genStats->maxRepeatedSymbols)
				{
					genStats->maxRepeatedSymbols = Stats[iStat]->ruleStats[iRule]->countRepeatedSymbols;
				}
			}

			if (Stats[iStat]->ruleStats[iRule]->ruleIsProducer)
			{
				genStats->ratioProducingRules++;
				totalRulesNonTerminal++;
				genStats->avgNumRulesProducing++;
				countRules++;
			}
			else if (Stats[iStat]->ruleStats[iRule]->ruleIsGraphical)
			{
				totalRulesNonTerminal++;
				genStats->avgNumRulesDrawing++;
				countRules++;
			}
			else if (Stats[iStat]->ruleStats[iRule]->ruleIsTerminal)
			{
				genStats->avgNumRulesTerminal++;
			}

			if (!Stats[iStat]->ruleStats[iRule]->ruleIsTerminal)
			{
				genStats->distributionRuleLengths[Stats[iStat]->ruleStats[iRule]->length]++;
			}

			if (Stats[iStat]->ruleStats[iRule]->ruleIsProducer)
			{
				genStats->distributionRuleLengthsProducing[Stats[iStat]->ruleStats[iRule]->length]++;
			}
			else
			{
				genStats->distributionRuleLengthsGraphical[Stats[iStat]->ruleStats[iRule]->length]++;
			}
		}

		if (countRules > genStats->maxNumRules)
		{
			genStats->maxNumRules = countRules;
		}
	}

	genStats->ratioProducingRules /= totalRulesNonTerminal;
	genStats->avgG2PTransitions /= totalRulesNonTerminal;
	genStats->avgP2GTransitions /= totalRulesNonTerminal;
	genStats->avgRepeatedSymbols /= totalRulesNonTerminal;

	genStats->avgNumRulesDrawing /= Stats->Count;
	genStats->avgNumRulesProducing /= Stats->Count;
	genStats->avgNumRulesTerminal /= Stats->Count;

	Console::WriteLine("Max # Rules: " + genStats->maxNumRules + " Avg # Producing Rules: " + genStats->avgNumRulesProducing + " Avg # Drawing Rules: " + +genStats->avgNumRulesDrawing + " Avg # Terminal Rules: " + +genStats->avgNumRulesTerminal);
	sw->WriteLine("Max # Rules: ," + genStats->maxNumRules + ", Avg # Producing Rules: ," + genStats->avgNumRulesProducing + ", Avg # Drawing Rules: ," + +genStats->avgNumRulesDrawing + ", Avg # Terminal Rules: ," + +genStats->avgNumRulesTerminal);

	Console::WriteLine("P2G Rule Ratio: " + genStats->ratioProducingRules);
	sw->WriteLine("P2G Rule Ratio:," + genStats->ratioProducingRules);

	Console::WriteLine("Avg # G2P Transitions: " + genStats->avgG2PTransitions + " Min # G2P Transitions: " + genStats->minG2PTransitions + " Max # G2P Transitions : " + genStats->maxG2PTransitions);
	sw->WriteLine("Avg # G2P Transitions:," + genStats->avgG2PTransitions + ",Min # G2P Transitions:," + genStats->minG2PTransitions + ",Max # G2P Transitions:," + genStats->maxG2PTransitions);

	Console::WriteLine("Avg # P2G Transitions: " + genStats->avgP2GTransitions + " Min # P2G Transitions: " + genStats->minP2GTransitions + " Max # P2G Transitions : " + genStats->maxP2GTransitions);
	sw->WriteLine("Avg # P2G Transitions:," + genStats->avgP2GTransitions + ",Min # P2G Transitions:," + genStats->minP2GTransitions + ",Max # P2G Transitions:, " + genStats->maxP2GTransitions);

	Console::WriteLine("Avg # Repeated: " + genStats->avgRepeatedSymbols + " Min # Repeated: " + genStats->minRepeatedSymbols + " Max # Repeated: " + genStats->maxRepeatedSymbols);
	sw->WriteLine("Avg # Repeated:," + genStats->avgRepeatedSymbols + ",Min # G2P Transitions:," + genStats->minRepeatedSymbols + ",Max # G2P Transitions:," + genStats->maxRepeatedSymbols);

	Console::WriteLine();
	Console::WriteLine("Length,# Overall, # Producing, # Graphical");

	sw->WriteLine();
	sw->WriteLine("Length,# Overall, # Producing, # Graphical");

	for (size_t iLength = 0; iLength < genStats->distributionRuleLengths->Count; iLength++)
	{
		Console::WriteLine((iLength+1).ToString(L"D3") + ": " + Int32(genStats->distributionRuleLengths[iLength]).ToString(L"D3") + " " + Int32(genStats->distributionRuleLengthsProducing[iLength]).ToString(L"D3") + " " + Int32(genStats->distributionRuleLengthsGraphical[iLength]).ToString(L"D3"));
		sw->WriteLine((iLength + 1).ToString(L"D3") + "," + Int32(genStats->distributionRuleLengths[iLength]).ToString(L"D3") + "," + Int32(genStats->distributionRuleLengthsProducing[iLength]).ToString(L"D3") + "," + Int32(genStats->distributionRuleLengthsGraphical[iLength]).ToString(L"D3"));
	}

	Console::WriteLine("Patterns");
	Console::WriteLine("ID,Is Producer?,Is Graphical?, Is Terminal?, Length, Length Compressed, # Producing Symbols, # Graphical Symbols, # Terminal Symbols, Ratio P2G,Pattern");

	sw->WriteLine("Patterns");
	sw->WriteLine("ID,Is Producer?,Is Graphical?, Is Terminal?, Length, Length Compressed, # Producing Symbols, # Graphical Symbols, # Terminal Symbols, Ratio P2G,Pattern");

	for (size_t iPattern = 0; iPattern < genStats->patterns->Count; iPattern++)
	{
		Console::WriteLine((iPattern + 1).ToString(L"D5") + ": " + genStats->patterns[iPattern]);
		sw->WriteLine((iPattern + 1).ToString(L"D5") + ":," + genStats->patterns[iPattern]);
	}

	Console::WriteLine("Compressed Patterns");
	Console::WriteLine("ID,Is Producer?,Is Graphical?, Is Terminal?, Length, Length Compressed, # Producing Symbols, # Graphical Symbols, # Terminal Symbols, Ratio P2G,Pattern");

	sw->WriteLine("Compressed Patterns");
	sw->WriteLine("ID,Is Producer?,Is Graphical?, Is Terminal?, Length, Length Compressed, # Producing Symbols, # Graphical Symbols, # Terminal Symbols, Ratio P2G,Pattern");

	for (size_t iPattern = 0; iPattern < genStats->patternsCompressed->Count; iPattern++)
	{
		Console::WriteLine((iPattern + 1).ToString(L"D5") + ": " + genStats->patternsCompressed[iPattern]);
		sw->WriteLine((iPattern + 1).ToString(L"D5") + ":," + genStats->patternsCompressed[iPattern]);
	}

	sw->Close();

	return genStats;
}

void PMIProcessorV3::Execute(Int32 iExecution)
{
	switch (this->lsitType)
	{
	case PMIProcessorV3::LSITType::Direct:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Length:
		//this->problem = gcnew LSIProblemV3(this->model, LSIProblemV3::ProblemType::Master);
		this->problem->Solve();
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoL_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v2:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Lex:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::ML:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::M:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	default:
		break;
	}

	// Ignore the first one because the timings are off due to code optimization
	if ((this->numExecutions > 0) && (iExecution > 0) || (this->numExecutions == 0))
	{
		this->metric->timingsAnalysis->Add(this->problem->analysisTime);
		this->metric->timingsSolve->Add(this->problem->solveTime);
		this->metric->solved->Add(this->problem->solved);
		this->metric->matched->Add(this->problem->matched);
		//this->metric->solutionSpaceSizes = this->problem->solutionSpaceSizes;
		this->metric->ComputeOverallPerformance();
		this->metric->Display();
		this->metrics->Add(metric);
	}
}

void PMIProcessorV3::ExecuteLiK(Int32 iExecution)
{
	this->problemLiK->Solve();
}

void PMIProcessorV3::ExecuteS0L(Int32 iExecution)
{
	this->problemS0L->Solve();
}

void PMIProcessorV3::ExecuteS0LM(Int32 iExecution)
{
	this->problemS0LM->Solve();
}

void PMIProcessorV3::ExecuteD2L(Int32 iExecution)
{
		this->problemD2L->Solve();
}

void PMIProcessorV3::ExecuteHD0L(Int32 iExecution)
{
	this->problemHD0L->Solve();
}

void PMIProcessorV3::ExecuteD0LEDC(Int32 iExecution)
{
	switch (this->lsitType)
	{
	case PMIProcessorV3::LSITType::Direct:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Length:
		//this->problem = gcnew LSIProblemV3(this->model, LSIProblemV3::ProblemType::Master);
		this->problemEDC->Solve();
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoL_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v2:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Lex:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::ML:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::M:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	default:
		break;
	}

	// Ignore the first one because the timings are off due to code optimization
	if ((this->numExecutions > 0) && (iExecution > 0) || (this->numExecutions == 0))
	{
		this->metric->timingsAnalysis->Add(this->problem->analysisTime);
		this->metric->timingsSolve->Add(this->problem->solveTime);
		this->metric->solved->Add(this->problem->solved);
		this->metric->matched->Add(this->problem->matched);
		//this->metric->solutionSpaceSizes = this->problem->solutionSpaceSizes;
		this->metric->ComputeOverallPerformance();
		this->metric->Display();
		this->metrics->Add(metric);
	}
}

void PMIProcessorV3::ExecuteWolframs(Int32 iExecution)
{
	switch (this->lsitType)
	{
	case PMIProcessorV3::LSITType::Direct:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Length:
		//this->problem = gcnew LSIProblemV3(this->model, LSIProblemV3::ProblemType::Master);
		this->problemWolfram->Solve();
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoL_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v1:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::PoG_v2:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::Lex:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::ML:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	case PMIProcessorV3::LSITType::M:
		//this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		//this->problem->fitnessFunction = this->fitnessFunction;
		//this->problem->Solve();
		break;
	default:
		break;
	}

	// Ignore the first one because the timings are off due to code optimization
	//if ((this->numExecutions > 0) && (iExecution > 0) || (this->numExecutions == 0))
	//{
		this->metric->timingsAnalysis->Add(this->problemWolfram->analysisTime);
		this->metric->timingsSolve->Add(this->problemWolfram->solveTime);
		//this->metric->solved->Add(this->problem->solved);
		//this->metric->matched->Add(this->problem->matched);
		//this->metric->solutionSpaceSizes = this->problem->solutionSpaceSizes;
		this->metric->ComputeOverallPerformance();
		this->metric->Display();
		this->metrics->Add(metric);
	//}
}


void PMIProcessorV3::Generate()
{
	// Need to switch this to load the statistics
	GeneratorStatistics^ genStats = gcnew GeneratorStatistics();
	genStats->Load("PDcombined_style_stats2.csv");

	PMIModelGeneratorV3^ generator = gcnew PMIModelGeneratorV3(genStats);

	this->model = generator->BuildD0L();
}

void PMIProcessorV3::GenerateStrings()
{
	this->Load();

	//for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	//{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		//array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");
		array<String^>^ files = Directory::GetFiles(folder, "03*_problemV3.txt");

		if (files->Length > 0)
		{
			for (size_t iFile = 11; iFile <= 11; iFile++)
			{
				Console::WriteLine("Processing file " + files[iFile]);
				this->modelFile = files[iFile];
				this->model = gcnew ModelV3(this->modelFile, false);
				String^ outputFileName = files[iFile]->Replace("problemv3", "strings");
				if (outputFileName != files[iFile])
				{

					StreamWriter^ sw = gcnew StreamWriter(outputFileName);
					for (size_t iGen = 0; iGen <= this->model->evidence->GetUpperBound(0); iGen++)
					{
						Console::WriteLine("Writing line " + iGen);
						sw->WriteLine(this->model->evidence[iGen]->symbols);
					}

					sw->Close();
				}
			}
		}
	//}
}


void PMIProcessorV3::Load()
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./lsit.txt");

	array<Char>^ seperatorComma = gcnew array<Char>{','};
	array<Char>^ seperatorDot = gcnew array<Char>{'.'};
	array<Char>^ seperatorSpace = gcnew array<Char>{' '};
	ConfigFileLine lineNum = PMIProcessorV3::ConfigFileLine::Algorithm;

	int valueI;
	float valueF;
	bool valueB;
	Int32 ruleNum = 0;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();

		if (line != "")
		{
			array<String^, 1>^ words;

			//Console::WriteLine("Processing Config Line # " + ((int)lineNum).ToString() + " : " + line);

			words = line->Split(seperatorComma, System::StringSplitOptions::RemoveEmptyEntries);

			if ((int)lineNum == (int)PMIProcessorV3::ConfigFileLine::Algorithm)
			{
				int::TryParse(words[1], valueI);
				switch (valueI)
				{
				case 0:
					this->lsitType = PMIProcessorV3::LSITType::Direct;
					break;
				case 1:
					this->lsitType = PMIProcessorV3::LSITType::Length;
					break;
				case 2:
					this->lsitType = PMIProcessorV3::LSITType::PoL_v1;
					break;
				case 3:
					this->lsitType = PMIProcessorV3::LSITType::PoG_v1;
					break;
				case 4:
					this->lsitType = PMIProcessorV3::LSITType::PoG_v2;
					break;
				case 5:
					this->lsitType = PMIProcessorV3::LSITType::Lex;
					break;
				case 6:
					this->lsitType = PMIProcessorV3::LSITType::ML;
					break;
				case 7:
					this->lsitType = PMIProcessorV3::LSITType::M;
					break;
				default:
					break;
				}
			}
			else if ((int)lineNum == (int)PMIProcessorV3::ConfigFileLine::Problem)
			{
				if (words[1]->Contains("."))
				{// run a sequence of problems
					array<String^, 1>^ range;
					range = words[1]->Split(seperatorDot, System::StringSplitOptions::RemoveEmptyEntries);

					Int32 start, end;

					int::TryParse(range[0], start);
					int::TryParse(range[1], end);

					for (size_t iProblem = start; iProblem <= end; iProblem++)
					{
						this->problems->Add(iProblem);
					}
				}
				else
				{
					array<String^, 1>^ problems;
					problems = words[1]->Split(seperatorSpace, System::StringSplitOptions::RemoveEmptyEntries);
					for (size_t iProblem = 0; iProblem <= problems->GetUpperBound(0); iProblem++)
					{
						int::TryParse(problems[iProblem], valueI);
						this->problems->Add(valueI);
					}
				}
			}
			else if ((int)lineNum == (int)PMIProcessorV3::ConfigFileLine::Executions)
			{
				int::TryParse(words[1], valueI);
				this->numExecutions = valueI;
			}
			else if ((int)lineNum == (int)PMIProcessorV3::ConfigFileLine::Output)
			{
				bool::TryParse(words[1], valueB);
				this->output = valueB;
			}
		}
		lineNum++;
	}
}

void PMIProcessorV3::Process()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);
			this->problem = gcnew LSIProblemV3(this->model, LSIProblemV3::ProblemType::Master);

			if (!this->problem->isAmbiguous)
			{
				for (size_t iExecution = 0; iExecution < this->numExecutions; iExecution++)
				{
					this->Execute(iExecution);
				}
			}
		}

	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessLiK()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);

			this->problemLiK = gcnew LiKProblem(this->model);
			//this->problemLiK->Generate();

			for (size_t iExecution = 0; iExecution < this->numExecutions; iExecution++)
			{
				this->ExecuteLiK(iExecution);
			}
		}

	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessS0L()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			for (size_t iExecution = 0; iExecution <= this->numExecutions; iExecution++)
			{
				this->modelFile = files[0];
				this->model = gcnew ModelV3(this->modelFile);


				do
				{
					bool valid = false;
					while (!valid)
					{
						//this->model->GenerateS0LPrefix();
						this->model->CreateEvidenceS0L();
						this->model->ComputeOddsProduced();
						this->model->contextFree = true;
						this->problemS0L = gcnew S0LProblemV3(this->model, LSIProblemV3::ProblemType::Master);
						valid = (this->problemS0L->MAO->factsList->Count == this->model->alphabet->numNonTurtles);

						for (size_t iRule = 0; iRule < this->model->productions[0]->rules->Count; iRule++)
						{
							for (size_t iSuccessor = 0; iSuccessor < this->model->productions[0]->rules[iRule]->successor->Count; iSuccessor++)
							{
								valid = valid && (this->model->productions[0]->rules[iRule]->count[iSuccessor] > 0);
							}
						}

						if (!valid)
						{
							Console::WriteLine("Invalid S0L-System or Productions ... Re-generating");
						}
					}
					this->ExecuteS0L(iExecution);
				} while (!this->problemS0L->success);

				if (iExecution > 0)
				{
					if (this->problemS0L->success)
					{
						this->wtp1 += this->problemS0L->wtp1;
						this->wtp2 += this->problemS0L->wtp2;
						this->wfp += this->problemS0L->wfp;
						this->wfn += this->problemS0L->wfn;
						this->mtts += this->problemS0L->solveTime;
						this->errorAvg += this->problemS0L->errorAvg;
						this->errorTotal += this->problemS0L->errorTotal;
						if (this->problemS0L->successorCountMM)
						{
							this->successorCountMM++;
						}

						if (this->problemS0L->successorCountDiff > this->successorCountDiffMax)
						{
							this->successorCountDiffMax = this->problemS0L->successorCountDiff;
						}

						this->sr++;
					}
					else
					{
						Console::WriteLine("Paused");
						Console::ReadLine();
					}

					if (iExecution >= 1)
					{
						Console::WriteLine("** RESULTS SO FAR **");
						Console::WriteLine("weighted true positive (M2C) = " + (this->wtp1 / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted true positive (C2M) = " + (this->wtp2 / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted false positive      = " + (this->wfp / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted false negative      = " + (this->wfn / (iExecution)).ToString(L"F4"));
						Console::WriteLine("error (avg)                  = " + (this->errorAvg / (iExecution)).ToString(L"F4"));
						Console::WriteLine("error (total)                = " + (this->errorTotal / (iExecution)).ToString(L"F4"));
						Console::WriteLine("successor count mm max       = " + (this->successorCountDiffMax));
						Console::WriteLine("successor count mm %         = " + ((float)this->successorCountMM / (iExecution)).ToString(L"F4"));
						Console::WriteLine("mtts                         = " + (this->mtts / (iExecution)).ToString(L"F4"));
						Console::WriteLine("sr                           = " + (this->sr / (iExecution)).ToString(L"F4"));
						Console::WriteLine();
						//Console::WriteLine("Paused");
						//Console::ReadLine();
					}
				}
			}

			if (this->numExecutions >= 1)
			{
				this->wtp1 = this->wtp1 / (this->numExecutions);
				this->wtp2 = this->wtp2 / (this->numExecutions);
				this->wfp = this->wfp / (this->numExecutions);
				this->wfn = this->wfn / (this->numExecutions);
				this->mtts = this->mtts / (this->numExecutions);
				this->errorAvg = this->errorAvg / (this->numExecutions);
				this->errorTotal = this->errorTotal / (this->numExecutions);
				this->successorCountMM = this->successorCountMM / (this->numExecutions);
				this->sr = this->sr / (this->numExecutions);
			}
		}

		Console::WriteLine("**** FINAL RESULTS ****");
		Console::WriteLine("template file = " + files[0]);
		Console::WriteLine("weighted true positive (M2C) = " + (this->wtp1).ToString(L"F4"));
		Console::WriteLine("weighted true positive (C2M) = " + (this->wtp2).ToString(L"F4"));
		Console::WriteLine("weighted false positive      = " + (this->wfp).ToString(L"F4"));
		Console::WriteLine("weighted false negative      = " + (this->wfn).ToString(L"F4"));
		Console::WriteLine("error (avg)                  = " + (this->errorAvg).ToString(L"F4"));
		Console::WriteLine("error (total)                = " + (this->errorTotal).ToString(L"F4"));
		Console::WriteLine("successor count mm max       = " + (this->successorCountDiffMax));
		Console::WriteLine("successor count mm %         = " + ((float)this->successorCountMM).ToString(L"F4"));
		Console::WriteLine("mtts                         = " + (this->mtts).ToString(L"F4"));
		Console::WriteLine("sr                           = " + (this->sr).ToString(L"F4"));
		Console::WriteLine("Paused");
		Console::ReadLine();
	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessS0LM()
{
	Int32 M = 5;
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*s0l_problemv3.txt");

		if (files->Length > 0)
		{
			for (size_t iExecution = 0; iExecution <= this->numExecutions; iExecution++)
			{
				this->modelFile = files[0];
				this->model = gcnew ModelV3(this->modelFile);

				do
				{
					bool valid = false;
					while (!valid)
					{
						//this->model->GenerateS0L();
						this->model->CreateEvidenceS0L();
						this->model->ComputeOddsProduced();
						this->model->contextFree = true;
						this->problemS0LM = gcnew S0LMProblemV3(this->model, LSIProblemV3::ProblemType::Master);
						this->problemS0LM->lengthBasedFlag = true;
						this->problemS0LM->secondaryModels = gcnew List<ModelV3^>;

						valid = (this->problemS0LM->MAO->factsList->Count == this->model->alphabet->numNonTurtles);

						for (size_t iRule = 0; iRule < this->model->productions[0]->rules->Count; iRule++)
						{
							for (size_t iSuccessor = 0; iSuccessor < this->model->productions[0]->rules[iRule]->successor->Count; iSuccessor++)
							{
								valid = valid && (this->model->productions[0]->rules[iRule]->count[iSuccessor] > 0);
							}
						}

						if (valid)
						{
							for (size_t iModel = 0; iModel < M; iModel++)
							{
								bool match = true;

								do
								{
									match = true;
									this->problemS0LM->secondaryModels->Add(gcnew ModelV3(this->modelFile));
									this->problemS0LM->secondaryModels[iModel]->productions = this->model->productions;
									this->problemS0LM->secondaryModels[iModel]->alphabet = this->model->alphabet;
									this->problemS0LM->secondaryModels[iModel]->alphabetShadow = this->model->alphabetShadow;
									this->problemS0LM->secondaryModels[iModel]->CreateEvidenceS0L();
									// check to make sure evidence is not exactly the same as the evidence in main model or any secondary model

									for (size_t iGen = 0; iGen <= this->problemS0LM->model->evidence->GetUpperBound(0); iGen++)
									{
										match = match && (this->problemS0LM->model->evidence[iGen]->symbols == this->problemS0LM->secondaryModels[iModel]->evidence[iGen]->symbols);
									}

									Int32 jModel = 0;
									while ((!match) && (jModel < this->problemS0LM->secondaryModels->Count))
									{
										if (jModel != iModel)
										{
											for (size_t iGen = 0; iGen <= this->problemS0LM->model->evidence->GetUpperBound(0); iGen++)
											{
												match = match && (this->problemS0LM->secondaryModels[jModel]->evidence[iGen]->symbols == this->problemS0LM->secondaryModels[iModel]->evidence[iGen]->symbols);
											}
										}
										jModel++;
									}
								} while (match);
							}
						}
						else 
						{
							Console::WriteLine("Invalid S0L-System or Productions ... Re-generating");
						}
					}
					this->ExecuteS0LM(iExecution);
				} while (!this->problemS0LM->success);

				if (iExecution > 0)
				{
					if (this->problemS0LM->success)
					{
						this->wtp1 += this->problemS0LM->wtp1;
						this->wtp2 += this->problemS0LM->wtp2;
						this->wfp += this->problemS0LM->wfp;
						this->wfn += this->problemS0LM->wfn;
						this->mtts += this->problemS0LM->solveTime;
						this->errorAvg += this->problemS0LM->errorAvg;
						this->errorTotal += this->problemS0LM->errorTotal;
						if (this->problemS0LM->successorCountMM)
						{
							this->successorCountMM++;
						}
					
						if (this->problemS0LM->successorCountDiff > this->successorCountDiffMax)
						{
							this->successorCountDiffMax = this->problemS0LM->successorCountDiff;
						}

						this->sr++;
					}
					else
					{
						Console::WriteLine("Paused");
						Console::ReadLine();
					}

					if (iExecution >= 1)
					{
						Console::WriteLine("** RESULTS SO FAR **");
						Console::WriteLine("weighted true positive (M2C) = " + (this->wtp1 / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted true positive (C2M) = " + (this->wtp2 / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted false positive      = " + (this->wfp / (iExecution)).ToString(L"F4"));
						Console::WriteLine("weighted false negative      = " + (this->wfn / (iExecution)).ToString(L"F4"));
						Console::WriteLine("error (avg)                  = " + (this->errorAvg / (iExecution)).ToString(L"F4"));
						Console::WriteLine("error (total)                = " + (this->errorTotal / (iExecution)).ToString(L"F4"));
						Console::WriteLine("successor count mm max       = " + (this->successorCountDiffMax).ToString(L"F4"));
						Console::WriteLine("successor count mm %         = " + (this->successorCountMM / (iExecution)).ToString(L"F4"));
						Console::WriteLine("mtts                         = " + (this->mtts / (iExecution)).ToString(L"F4"));
						Console::WriteLine("sr                           = " + (this->sr / (iExecution)).ToString(L"F4"));
						Console::WriteLine();
						//Console::WriteLine("Paused");
						//Console::ReadLine();
					}
				}
			}

			if (this->numExecutions >= 1)
			{
				this->wtp1 = this->wtp1 / (this->numExecutions);
				this->wtp2 = this->wtp2 / (this->numExecutions);
				this->wfp = this->wfp / (this->numExecutions);
				this->wfn = this->wfn / (this->numExecutions);
				this->mtts = this->mtts / (this->numExecutions);
				this->errorAvg = this->errorAvg / (this->numExecutions);
				this->errorTotal = this->errorTotal / (this->numExecutions);
				this->successorCountMM = this->successorCountMM / (this->numExecutions);
				this->sr = this->sr / (this->numExecutions);
			}
		}

		Console::WriteLine("**** FINAL RESULTS ****");
		Console::WriteLine("template file = " + files[0]);
		Console::WriteLine("weighted true positive (M2C) = " + (this->wtp1).ToString(L"F4"));
		Console::WriteLine("weighted true positive (C2M) = " + (this->wtp2).ToString(L"F4"));
		Console::WriteLine("weighted false positive      = " + (this->wfp).ToString(L"F4"));
		Console::WriteLine("weighted false negative      = " + (this->wfn).ToString(L"F4"));
		Console::WriteLine("error (avg)                  = " + (this->errorAvg).ToString(L"F4"));
		Console::WriteLine("error (total)                = " + (this->errorTotal).ToString(L"F4"));
		Console::WriteLine("successor count mm max       = " + (this->successorCountDiffMax));
		Console::WriteLine("successor count mm %         = " + (this->successorCountMM).ToString(L"F4"));
		Console::WriteLine("mtts                         = " + (this->mtts).ToString(L"F4"));
		Console::WriteLine("sr                           = " + (this->sr).ToString(L"F4"));
		Console::WriteLine("Paused");
		Console::ReadLine();
	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessD2L()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			for (size_t iExecution = 0; iExecution <= this->numExecutions; iExecution++)
			{
				this->modelFile = files[0];
				this->model = gcnew ModelV3(this->modelFile);

				bool valid = false;

				while (!valid)
				{
					//this->model->GenerateD2L();
					//this->model->CreateEvidence();
					this->problemD2L = gcnew D2LProblemV3(this->model, LSIProblemV3::ProblemType::Master);
					valid = true;
				}

				this->ExecuteD2L(iExecution);

				if (iExecution > 0)
				{
					this->mtts += this->problemD2L->solveTime;
					if (this->problemD2L->success)
					{
						this->sr++;
					}

					if (iExecution >= 1)
					{
						Console::WriteLine("mtts                         = " + (this->mtts / (iExecution)).ToString(L"F4"));
						Console::WriteLine("sr                           = " + (this->sr / (iExecution)).ToString(L"F4"));
						Console::WriteLine();
					}
				}
			}

			if (this->numExecutions >= 1)
			{
				this->mtts = this->mtts / (this->numExecutions);
				this->sr = this->sr / (this->numExecutions);
			}
		}

		Console::WriteLine("template file = " + files[0]);
		Console::WriteLine("mtts                         = " + (this->mtts).ToString(L"F4"));
		Console::WriteLine("sr                           = " + (this->sr).ToString(L"F4"));
		Console::WriteLine("Paused");
		Console::ReadLine();
	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessHD0L()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			for (size_t iExecution = 0; iExecution <= this->numExecutions; iExecution++)
			{
				this->modelFile = files[0];
				this->model = gcnew ModelV3();
				this->model->contextFree = true;
				List<String^>^ filters = gcnew List<String^>;
				this->model->Load(this->modelFile, filters);
				this->model->CreateEvidence();

				bool valid = false;

				while (!valid)
				{
					this->problemHD0L = gcnew HD0LProblemV3(this->model, LSIProblemV3::ProblemType::Master);
					this->problemHD0L->lengthBasedFlag = true;
					valid = true;
				}

				this->ExecuteHD0L(iExecution);

				if (iExecution > 0)
				{
					this->mtts += this->problemHD0L->solveTime;
					if (this->problemHD0L->success)
					{
						this->sr++;
					}

					if (iExecution >= 1)
					{
						Console::WriteLine("mtts                         = " + (this->mtts / (iExecution)).ToString(L"F4"));
						Console::WriteLine("sr                           = " + (this->sr / (iExecution)).ToString(L"F4"));
						Console::WriteLine();
					}
				}
			}

			if (this->numExecutions >= 1)
			{
				this->mtts = this->mtts / (this->numExecutions);
				this->sr = this->sr / (this->numExecutions);
			}
		}

		Console::WriteLine("template file = " + files[0]);
		Console::WriteLine("mtts                         = " + (this->mtts).ToString(L"F4"));
		Console::WriteLine("sr                           = " + (this->sr).ToString(L"F4"));
		Console::WriteLine("Paused");
		Console::ReadLine();
	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessD0LEDC()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);
			this->model->contextFree = true;
			this->problemEDC = gcnew EDCProblemV3(this->model, LSIProblemV3::ProblemType::Master);

			for (size_t iExecution = 0; iExecution < this->numExecutions; iExecution++)
			{
				this->ExecuteD0LEDC(iExecution);
			}
		}

	}

	if (output)
	{
		this->WriteResults();
	}
}

String^ PMIProcessorV3::ComputeWolframSuccessor(Byte B)
{
	if (B == 0)
	{
		return "A";
	}
	else if (B == 1)
	{
		return "B";
	}
	else if (B == 2)
	{
		return "C";
	}
	else if (B == 3)
	{
		return "D";
	}
	else if (B == 4)
	{
		return "E";
	}
	else if (B == 5)
	{
		return "G";
	}
	else if (B == 6)
	{
		return "H";
	}
	else if (B == 7)
	{
		return "I";
	}
}

void PMIProcessorV3::ProcessWolframs()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);

			array<Byte>^ values = gcnew array<Byte>(8);
			Int32 index = 0;

			for (size_t iWolfram = 0; iWolfram <= 255; iWolfram++)
			{
				// Adjust the model
				for (size_t iRule = 0; iRule < 15; iRule++)
				{
					switch (iRule)
					{
					case 0:
						this->model->productions[0]->rules[0]->successor[0] = this->ComputeWolframSuccessor(values[0]); // A < A > A
						break;
					case 1:
						this->model->productions[0]->rules[0]->successor[1] = this->ComputeWolframSuccessor(values[1]); // A < A > B
						break;
					case 2:
						this->model->productions[0]->rules[0]->successor[2] = this->ComputeWolframSuccessor(values[2]); // B < A > A
						break;
					case 3:
						this->model->productions[0]->rules[0]->successor[3] = this->ComputeWolframSuccessor(values[3]); // B < A > B
						break;
					case 4:
						this->model->productions[0]->rules[0]->successor[4] = "B" + this->ComputeWolframSuccessor(values[2]); // * < A > A
						break;
					case 5:
						this->model->productions[0]->rules[0]->successor[5] = "B" + this->ComputeWolframSuccessor(values[3]); // * < A > B
						break;
					case 6:
						this->model->productions[0]->rules[0]->successor[6] = this->ComputeWolframSuccessor(values[1]) + "B"; // A < A > *
						break;
					case 7:
						this->model->productions[0]->rules[0]->successor[7] = this->ComputeWolframSuccessor(values[3]) + "B"; // B < A > *
						break;
					case 8:
						this->model->productions[0]->rules[1]->successor[0] = this->ComputeWolframSuccessor(values[4]); // A < B > A
						break;
					case 9:
						this->model->productions[0]->rules[1]->successor[1] = this->ComputeWolframSuccessor(values[5]); // A < B > B
						break;
					case 10:
						this->model->productions[0]->rules[1]->successor[2] = this->ComputeWolframSuccessor(values[6]); // B < B > A
						break;
					case 11:
						this->model->productions[0]->rules[1]->successor[3] = this->ComputeWolframSuccessor(values[7]); // B < B > B
						break;
					case 12:
						this->model->productions[0]->rules[1]->successor[4] = "B" + this->ComputeWolframSuccessor(values[6]); // * < B > A
						break;
					case 13:
						this->model->productions[0]->rules[1]->successor[5] = "B" + this->ComputeWolframSuccessor(values[7]); // * < B > B
						break;
					case 14:
						this->model->productions[0]->rules[1]->successor[6] = this->ComputeWolframSuccessor(values[5]) + "B"; // A < B > *
						break;
					case 15:
						this->model->productions[0]->rules[1]->successor[7] = this->ComputeWolframSuccessor(values[7]) + "B"; // B < B > *
						break;
					default:
						break;
					}
				}

				this->model->CreateEvidence();
				this->problemWolfram = gcnew LSIProblemV3_Wolfram(this->model, LSIProblemV3::ProblemType::Master);
				this->problemWolfram->tracker = iWolfram + 1;

				for (size_t iExecution = 0; iExecution < this->numExecutions; iExecution++)
				{
					this->ExecuteWolframs(iExecution);
				}

				bool done = false;
				while (!done && index <= values->GetUpperBound(0))
				{
					values[index]++;
					if (values[index] > 1)
					{
						values[index] = 0;
						index++;
					}
					else
					{
						done = true;
						index = 0;
					}
				}
			
				for (size_t i = 0; i <= values->GetUpperBound(0); i++)
				{
					Console::Write(values[i]);
				}
				Console::WriteLine();
			}

		}

	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessWolframs_NSymbol(Int32 N)
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		this->metric = gcnew PMIPerformanceMetrics();

		String^ folder = "./";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);

			Int32 numValues = Math::Pow(N, 3) + 1;
			UInt64 numWolframs = Math::Pow(N, numValues);
			Int32 numRules1 = Math::Pow(N, 3);
			Int32 numRules2 = (N * (2 * N));

			array<Byte>^ values = gcnew array<Byte>(numValues);
			Int32 index = 0;

			Random^ generator = gcnew Random();
			UInt64 iWolfram = numWolframs * generator->NextDouble();

			// Convert the iWolfram into a set of values
			for (Int32 i = 0; i < numValues; i++)
			{
				values[i] = (UInt64) (iWolfram / (Math::Pow(N, i))) % N;
			}

			Int32 numSamples = 1000;
			for (UInt64 iWolfram = 0; iWolfram <= numSamples; iWolfram++)
			{
				// Adjust the model
				for (size_t iRule = 0; iRule < N; iRule++)
				{
					Int32 idx = 0;
					// Non-empty space successors
					for (size_t iSuccessor = 0; iSuccessor < Math::Pow(N, 2); iSuccessor++)
					{
						this->model->productions[0]->rules[iRule]->successor[iSuccessor] = this->ComputeWolframSuccessor(values[idx]);
						idx++;
					}
				}

				String^ emptySpace = this->ComputeWolframSuccessor(values[values->GetUpperBound(0)]);

				for (size_t iRule = 0; iRule < N; iRule++)
				{
					// Empty space successors
					for (size_t iSuccessor = 0; iSuccessor < 2*N; iSuccessor++)
					{
						if (this->model->productions[0]->rules[iRule]->contextLeft[Math::Pow(N, 2) + iSuccessor] == "*")
						{
							String^ production = "";
							Int32 jSuccessor = 0;

							while (production == "" && jSuccessor < this->model->productions[0]->rules[iRule]->successor->Count)
							{
								if (this->model->productions[0]->rules[iRule]->contextLeft[jSuccessor] == emptySpace && this->model->productions[0]->rules[iRule]->contextRight[jSuccessor] == this->model->productions[0]->rules[iRule]->contextRight[iSuccessor])
								{
									production = this->model->productions[0]->rules[iRule]->successor[jSuccessor];
								}
								jSuccessor++;
							}
							this->model->productions[0]->rules[iRule]->successor[Math::Pow(N, 2) + iSuccessor] = emptySpace + production;
						}
						else if (this->model->productions[0]->rules[iRule]->contextRight[Math::Pow(N, 2) + iSuccessor] == "*")
						{
							String^ production = "";
							Int32 jSuccessor = 0;

							while (production == "" && jSuccessor < this->model->productions[0]->rules[iRule]->successor->Count)
							{
								if (this->model->productions[0]->rules[iRule]->contextRight[jSuccessor] == emptySpace && this->model->productions[0]->rules[iRule]->contextLeft[jSuccessor] == this->model->productions[0]->rules[iRule]->contextLeft[iSuccessor])
								{
									production = this->model->productions[0]->rules[iRule]->successor[jSuccessor];
								}
								jSuccessor++;
							}
							this->model->productions[0]->rules[iRule]->successor[Math::Pow(N, 2) + iSuccessor] = production + emptySpace;
						}
					}
				}

				this->model->CreateEvidence();
				this->problemWolfram = gcnew LSIProblemV3_Wolfram(this->model, LSIProblemV3::ProblemType::Master);
				this->problemWolfram->tracker = iWolfram + 1;

				for (size_t iExecution = 0; iExecution < this->numExecutions; iExecution++)
				{
					this->ExecuteWolframs(iExecution);
				}

				bool done = false;
				while (!done && index <= values->GetUpperBound(0))
				{
					values[index]++;
					if (values[index] > 1)
					{
						values[index] = 0;
						index++;
					}
					else
					{
						done = true;
						index = 0;
					}
				}

				for (size_t i = 0; i <= values->GetUpperBound(0); i++)
				{
					Console::Write(values[i]);
				}
				Console::WriteLine();
			}

		}

	}

	if (output)
	{
		this->WriteResults();
	}
}

void PMIProcessorV3::ProcessEquations()
{
	this->Load();

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		String^ folder = "./";
		String^ output = "./equations";
		array<String^>^ files = Directory::GetFiles(folder, this->problems[iProblem].ToString(L"D4") + "*_problemV3.txt");

		if (files->Length > 0)
		{
			String^ outputFileName = files[0]->Replace("problemv3", "equation");
			StreamWriter^ sw = gcnew StreamWriter(output + outputFileName, false);
			this->modelFile = files[0];
			this->model = gcnew ModelV3(this->modelFile);
			this->problemD2L = gcnew D2LProblemV3(this->model, LSIProblemV3::ProblemType::Master);

			array<String^>^ equations = gcnew array<String^>(this->model->generations - 1);

			for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
			{
				String^ seperator = "";
				Int32 turtleCount = 0;
				FactCounter^ fc = this->problemD2L->ComputeFactCount(iGen, this->model->evidence[iGen]);
				for (size_t iCount = 0; iCount < fc->counter->Count; iCount++)
				{
					Int32 iFact = this->problemD2L->MAO->GetFactIndex(fc->counter[iCount]->fact);
					if (iFact >= 0)
					{
						char variable = 'A';
						String^ tmp = "";
						Int32 modifier = iFact % 26;

						if (iFact >= 26)
						{
							tmp = "_";
						}
						variable = variable + modifier;
						String^ s = gcnew String(variable, 1);
						s = fc->counter[iCount]->count + tmp + s;
						equations[iGen] = equations[iGen] + seperator + s;
						seperator = " + ";
					}
					else
					{
						turtleCount += fc->counter[iCount]->count;
					}
				}

				equations[iGen] = equations[iGen] + " = " + (this->model->evidence[iGen + 1]->Length()-turtleCount);
				Console::WriteLine(equations[iGen]);
				sw->WriteLine(equations[iGen]);
			}

			sw->Close();
		}
	}
}


void PMIProcessorV3::WriteResults()
{
	String^ seperator = ",";
	String^ srString = "SR:" + seperator;
	String^ mttsString = "MTTS:" + seperator;
	String^ mmrString = "MMR:" + seperator;
	String^ problemString = "" + seperator;

	for (size_t iProblem = 0; iProblem < this->problems->Count; iProblem++)
	{
		problemString += iProblem + seperator;
		srString += this->metrics[iProblem]->SR + seperator;
		mttsString += this->metrics[iProblem]->SR + seperator;
		mmrString += this->metrics[iProblem]->SR + seperator;
	}

	String^ dateString = DateTime::Now.ToString("d");
	String^ timeString = DateTime::Now.Hour.ToString() + DateTime::Now.Minute.ToString();
	String^ lsitType = "";

	switch (this->lsitType)
	{
	case PMIProcessorV3::LSITType::Direct:
		lsitType = "Direct";
		break;
	case PMIProcessorV3::LSITType::Length:
		lsitType = "Length";
		break;
	case PMIProcessorV3::LSITType::PoL_v1:
		lsitType = "PoLV1";
		break;
	case PMIProcessorV3::LSITType::PoG_v1:
		lsitType = "PoGV1";
		break;
	case PMIProcessorV3::LSITType::PoG_v2:
		lsitType = "PoGV2";
		break;
	case PMIProcessorV3::LSITType::Lex:
		lsitType = "Lex";
		break;
	case PMIProcessorV3::LSITType::ML:
		lsitType = "ML";
		break;
	case PMIProcessorV3::LSITType::M:
		lsitType = "M";
		break;
	default:
		break;
	}

	String^ path = "D://PMIT//results_phase3//" + lsitType + "_" + dateString + "_" + timeString + CommonGlobals::PoolInt(0, 999999).ToString() + ".txt";

	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(path, false);

	sw->WriteLine(problemString);
	sw->WriteLine(srString);
	sw->WriteLine(mttsString);
	sw->WriteLine(mmrString);

	sw->Close();
}