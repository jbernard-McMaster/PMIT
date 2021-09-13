#pragma once
#include "ModelV3.h"
#include "PMIPerformanceMetrics.h"
#include "D2LProblemV3.h"
#include "HD0LProblemV3.h"
#include "LSIProblemV3.h"
#include "LSIProblemV3_Wolfram.h"
#include "S0LProblemV3.h"
#include "S0LMProblemV3.h"
#include "EDCProblemV3.h"
#include "LiKProblem.h"
#include "PMIModelGeneratorV3.h"

// Only needed for analyzer, should convert analyzer to Phase 3 models when all the problems have been converted over
#include "PlantModel.h"

#include <string>
#include <iostream>
#include <filesystem>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;

public ref class PMIProcessorV3
{
public:
	PMIProcessorV3();
	virtual ~PMIProcessorV3();

	enum class LSITType
	{
		Direct, // v4
		Length, // v2
		PoL_v1, // v1b
		PoG_v1, // v1b
		PoG_v2,
		Lex, // v1b
		ML,
		M,
	};

	LSITType lsitType;
	Int32 numExecutions;
	List<Int32>^ problems;
	bool output;
	
	String^ modelFile;
	String^ configFile;
	ModelV3^ model;

	LSIProblemV3^ problem;
	LSIProblemV3_Wolfram^ problemWolfram;
	S0LProblemV3^ problemS0L;
	S0LMProblemV3^ problemS0LM;
	D2LProblemV3^ problemD2L;
	HD0LProblemV3^ problemHD0L;
	EDCProblemV3^ problemEDC;
	LiKProblem^ problemLiK;

	PMIPerformanceMetrics^ metric;
	List<PMIPerformanceMetrics^>^ metrics;

	virtual GeneratorStatistics^ Analyze();
	virtual GeneratorStatistics^ CompileStats(List<ModelStatistics^>^ Stats, String^ FileName);
	void Execute(Int32 iExecution);
	void Generate();
	void GenerateStrings();
	void Load();
	void Process();
	void WriteResults();

	void ProcessLiK();
	void ExecuteLiK(Int32 iExecution);

	void ProcessS0L();
	void ExecuteS0L(Int32 iExecution);

	void ProcessS0LM();
	void ExecuteS0LM(Int32 iExecution);

	void ProcessD2L();
	void ExecuteD2L(Int32 iExecution);

	void ProcessHD0L();
	void ExecuteHD0L(Int32 iExecution);

	void ProcessD0LEDC();
	void ExecuteD0LEDC(Int32 iExecution);

	String^ ComputeWolframSuccessor(Byte B);
	void ProcessWolframs();
	void ExecuteWolframs(Int32 iExecution);

	void ProcessWolframs_NSymbol(Int32 N);

	void ProcessEquations();

	enum class ConfigFileLine
	{
		Algorithm,
		Problem,
		Executions,
		Output
	};

	float wtp1;
	float wtp2;
	float wfp;
	float wfn;
	float sr;
	std::size_t mtts;
	float errorAvg;
	float errorTotal;
	Int32 successorCountDiffMax;
	float successorCountMM;
};

