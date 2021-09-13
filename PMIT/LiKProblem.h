#pragma once

#include "GeneticAlgorithmPMIT.h"
#include "ModelV3.h"
#include "LSIProblemV3.h"

public ref class LiKProblem :
	public LSIProblemV3
{
public:
	LiKProblem(ModelV3^ M);
	virtual ~LiKProblem();
	virtual void Initialize() override;
	
	virtual float Assess(array<Int32>^ Solution) override;
	virtual float Assess_D(array<Int32>^ Solution);
	virtual float Assess_G(array<Int32>^ Solution);
	virtual float Assess_L(array<Int32>^ Solution);
	virtual float Assess_PoG(array<Int32>^ Solution);
	virtual float Assess_PoL(array<Int32>^ Solution);
	virtual float Assess_Lex(array<Int32>^ Solution);

	virtual float AssessProductionRulesParikh(array<Int32, 2>^ Rules);

	array<Int32>^ ConvertSolutionToRuleLengths(array<Int32>^ Solution);

	virtual void ComputeSymbolLexicons() override;
	Int32 ChooseSymbolFromLexicon(Int32 iSymbol, Int32 Roll, bool BlankAllowed);

	virtual GenomeConfiguration<Int32>^ CreateGenome() override;
	virtual List<ProductionRuleV3^>^ CreateRulesByScanning(array<Int32>^ RuleLengths) override;

	virtual void Generate();

	virtual void Solve() override;	
	virtual SolutionType^ SolveProblemGA(System::IO::StreamWriter^ SW, float Target);
	virtual SolutionType^ SolveProblemGA2(System::IO::StreamWriter^ SW, float Target, SolutionType^ Solution);

	Int32 totalLengthMin;
	Int32 totalLengthMax;
	array<Int32>^ lengthMin;
	array<Int32>^ lengthMax;
	array<Int32, 2>^ growthMin;
	array<Int32, 2>^ growthMax;
	array<Int32, 2>^ growth;
	array<Int32>^ lastGeneration;
	array<Int32, 2>^ uaMaster;

	array<Int32, 2>^ symbolLexicon;
	array<float, 2>^ symbolOdds;
	array<float, 2>^ symbolOddsWithBlank;
	array<Int32>^ symbolOptions;

	Int32 maxValue1 = 0;
	Int32 maxValue2 = 0;

	Int32 wheelSize = 100;

	Int32 lengthMinError = 3;
	Int32 lengthMaxError = 3;
	Int32 totalLengthMinError = 2;
	Int32 totalLengthMaxError = 2;
	Int32 growthMinError = 3;
	Int32 growthMaxError = 3;

	array<float>^ lengthDistribution;
	array<float>^ symbolDistribution;

	enum class LiKProblemType
	{
		D,
		L,
		PoL,
		G,
		PoG,
		Lex
	};

private:
	LiKProblemType problemType;

};

