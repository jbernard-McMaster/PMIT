#pragma once

#include "Symbol.h"
#include "Word.h"

using namespace System::Collections::Generic;

public ref class Vocabulary
{
public:
	Vocabulary();
	Vocabulary(bool BranchingAllowed);
	virtual ~Vocabulary();

	virtual property List<Symbol^>^ Symbols;
	virtual property List<Symbol^>^ Abstracts;
	virtual property Symbol^ YieldsSymbol;
	virtual property Symbol^ SwapSymbol;

	Int32 numModules;
	Int32 numConstants;

	Int32 IndexModulesStart;
	Int32 IndexModulesEnd;
	Int32 IndexConstantsStart;
	Int32 IndexConstantsEnd;
	Int32 IndexSymbolsStart;
	Int32 IndexSymbolsEnd;
	Int32 IndexAlpha;

	//virtual property List<Symbol^>^ Instructions;
	//virtual property List<Symbol^>^ Operations;
	//virtual property List<Symbol^>^ Special;

	bool branchingAllowed;

	//void AddSymbol(String^ Value);
	void AddSymbol(String^ Value, bool AsModule);
	void AddSymbol(Symbol^ S, bool AsModule);

	List<Symbol^>^ AllModules();
	List<Symbol^>^ AllConstants();

	void ComputeIndices();
	void CompressSymbols(Word^ A);
	array<Int32>^ CompressSymbols(List<Symbol^>^ A);

	Int32 ConstantIndex(Symbol^ S);

	Word^ CreateWord(String^ S);

	Int32 CountConstants(Word^ A);

	Symbol^ FindSymbol(String^ V);
	Int32 FindIndex(Symbol^ S);
	Int32 FindIndex(String^ V);

	bool IsModule(Symbol^ S);
	bool IsConstant(Symbol^ S);
	bool Contains(Symbol^ S);

	void Load(String^ FileName);

	Int32 ParikhVectorLength(array<Int32>^ PV);

	Int32 RuleIndex(Symbol^ S);

	void SwitchSymbol(Symbol^ S);

	enum class ConfigFileLine
	{
		Modules,
		Operations,
		Instructions,
		Special,
		END
	};

	static String^ StopSymbol = "||";
	static String^ EndSymbol = "|";
	static String^ StartBranch = "[";
	static String^ EndBranch = "]";
	static String^ TurnLeft = "+";
	static String^ TurnRight = "-";

	static const Int32 SkipStopIndex = 1;
	static const Int32 SkipEndIndex = 2;
	Int32 RegularSymbolEnd;
	Int32 SkipBranchIndex;
	Int32 endBranchIndex;

	virtual Int32 NumSymbols() { return this->numModules + this->numConstants; };
protected:

private:
};

