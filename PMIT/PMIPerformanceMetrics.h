#pragma once

using namespace System;
using namespace System::Collections::Generic;

public ref class PMIPerformanceMetrics
{
public:
	PMIPerformanceMetrics();
	virtual ~PMIPerformanceMetrics();

	size_t MTTS;
	size_t MTTS_Solve;
	size_t MTTS_Analysis;
	float SR;
	float MMR;

	List<size_t>^ timingsAnalysis;
	List<size_t>^ timingsSolve;
	List<bool>^ solved;
	List<bool>^ matched;
	List<UInt64>^ solutionSpaceSizes;

	void ComputeOverallPerformance();
	void Display();
	void Write(Int32 T, Int32 P);
};

