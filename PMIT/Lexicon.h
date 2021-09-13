#pragma once

#include "defines.h"
#include "Word.h"
#include "Vocabulary.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;

public ref struct SubwordCountType
{
	Word^ subword;
	Int32 count;
};

public ref class LengthComparer : IComparer<Word^>
{
public:
	virtual int Compare(Word^ X, Word^ Y)
	{
		if (X->Count >= Y->Count)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
};

public ref class Lexicon
{
public:
	Lexicon(Int32 NumGenerations, Int32 MinRuleLength, Int32 MaxRuleLength, Vocabulary^ V, array<Int32>^ MinRuleLengths, array<Int32>^ MaxRuleLengths, array<Word^>^ RuleHead, array<Word^>^ RuleTail, array<Word^>^ RuleFragments, array<Int32>^ TotalPoG, array<Int32, 2>^ MinPoG, array<Int32, 2>^ MaxPoG);
	Lexicon(Int32 NumGenerations, Vocabulary^ V, array<Int32>^ MinRuleLengths, array<Int32>^ MaxRuleLengths, array<Word^>^ RuleHead, array<Word^>^ RuleTail, array<Word^>^ RuleFragments, array<Int32>^ TotalPoG, array<Int32, 2>^ MinPoG, array<Int32, 2>^ MaxPoG);
	Lexicon(Vocabulary^ V, Int32 Length, array<Int32>^ TotalPoG);
	virtual ~Lexicon();

	Vocabulary^ vocabulary;

	// All possible subwords for each symbol
	array<List<Word^>^>^ symbolSubwords;
	List<Word^>^ evidence;
	array<Int32>^ minRuleLengths;
	array<Int32>^ maxRuleLengths;
	array<Word^>^ ruleHead;
	array<Word^>^ ruleTail;
	array<Word^>^ ruleFragments;
	array<Int32>^ totalPoG;
	array<Int32, 2>^ maxPoG;
	array<Int32, 2>^ minPoG;

	// Counts of each subword at each generation
	array<List<SubwordCountType^>^,2>^ subwordCounts;

	void Compress();

	void CompileLexicon(List<Word^>^ Evidence);
	void CompileLexiconT(List<Word^>^ Evidence);
	void CompileLexiconAction(Int32 iRule);

	void Display();
	void DisplayCounts();

	void Filter(Lexicon^ L);
	void FilterMT(Lexicon^ L);
	void FilterProcess(Int32 iRule);
	Int32 FindSubword(Word^ W);
	Int32 FindSubwordCount(Word^ W);

	Int32 FoundSubword(Word^ W, Int32 iGen);

	bool IsSubwordOverlap(Word^ W);

	bool Load(String^ FileName);

	void Save(String^ FileName);

	void UpdateLexicon_Length(Int32 iRule, Int32 MinLength, Int32 MaxLength);

	bool VerifyFragment(Int32 iRule, Word^ W);
	bool VerifyRules(array<Int32, 2>^ R, array<bool, 2>^ SymbolSubset, Int32 SymbolSubsetIndex, Int32 SymbolIndexEnd);

protected:
	Lexicon^ processL;
	List<Symbol^>^ processToRemove;
};

