#include "stdafx.h"
#include "PMIPerformanceMetrics.h"


PMIPerformanceMetrics::PMIPerformanceMetrics()
{
	this->timingsAnalysis = gcnew List<size_t>;
	this->timingsSolve = gcnew List<size_t>;
	this->solved = gcnew List<bool>;
	this->matched = gcnew List<bool>;
	this->solutionSpaceSizes = gcnew List<UInt64>;
}


PMIPerformanceMetrics::~PMIPerformanceMetrics()
{
}

void PMIPerformanceMetrics::ComputeOverallPerformance()
{
	size_t totalTimeAnalysis = 0;
	size_t totalTimeSolve = 0;
	Int32 totalSolved = 0;
	Int32 totalMatched = 0;

	for (size_t iResult = 0; iResult < this->timingsSolve->Count; iResult++)
	{
		totalTimeAnalysis += this->timingsAnalysis[iResult];
		totalTimeSolve += this->timingsSolve[iResult];
		
		//if (this->solved[iResult])
		//{
		//	totalSolved++;
		//}

		//if (this->matched[iResult])
		//{
		//	totalMatched++;
		//}
	}

	this->MTTS_Analysis = totalTimeAnalysis / this->timingsAnalysis->Count;
	this->MTTS_Solve = totalTimeAnalysis / this->timingsAnalysis->Count;
	this->MTTS = (totalTimeAnalysis + totalTimeSolve) / this->timingsAnalysis->Count;
	//this->SR = (float)totalSolved / this->timingsAnalysis->Count;
	//this->MMR = (float)totalMatched / this->timingsAnalysis->Count;
}

void PMIPerformanceMetrics::Display()
{
	Console::WriteLine("MTTS (Analysis): " + this->MTTS_Analysis);
	Console::WriteLine("MTTS (Solve)   : " + this->MTTS_Solve);
	Console::WriteLine("MTTS (Overall) : " + this->MTTS);
	//Console::WriteLine("Success Rate   : " + this->SR.ToString(L"F4"));
	//Console::WriteLine("Match Rate     : " + this->MMR.ToString(L"F4"));
	//for (size_t iSize = 0; iSize < this->solutionSpaceSizes->Count; iSize++)
	//{
	//	Console::WriteLine("Solution Space #" + iSize + " : " + this->solutionSpaceSizes[iSize]);
	//}
	Console::WriteLine();
}

void PMIPerformanceMetrics::Write(Int32 T, Int32 P)
{
	String^ dateString = DateTime::Now.ToString("d");
	String^ timeString = DateTime::Now.Hour.ToString() + DateTime::Now.Minute.ToString();
	String^ lsitType = "";

	switch (T)
	{
	case 0:
		lsitType = "Direct";
		break;
	case 1:
		lsitType = "Length";
		break;
	case 2:
		lsitType = "PoLV1";
		break;
	case 3:
		lsitType = "PoGV1";
		break;
	case 4:
		lsitType = "PoGV2";
		break;
	case 5:
		lsitType = "Lex";
		break;
	case 6:
		lsitType = "ML";
		break;
	case 7:
		lsitType = "M";
		break;
	default:
		break;
	}

	String^ path = "D://PMIT//results_phase3//" + lsitType + "_" + P.ToString() + "_" + dateString + "_" + timeString + CommonGlobals::PoolInt(0, 999999).ToString() + ".csv";

	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(path, false);

	sw->WriteLine("Success Rate   : " + this->SR.ToString(L"F4"));
	sw->WriteLine("MTTS (Overall) : " + this->MTTS);
	sw->WriteLine("Match Rate     : " + this->MMR.ToString(L"F4"));
	for (size_t iSize = 0; iSize < this->solutionSpaceSizes->Count; iSize++)
	{
		sw->WriteLine("Solution Space #" + iSize + " : " + this->solutionSpaceSizes[iSize]);
	}
	sw->WriteLine();

	sw->Close();
}