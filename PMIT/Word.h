#pragma once

#include "Symbol.h"
#include "ProductionRule.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class Word
{
public:
	Word();
	Word(List<Symbol^>^ Symbols);
	Word(Word^ A);
	virtual ~Word();

	virtual property Int32 Count
	{
		Int32 get() { return this->symbols->Count; }
	};

	Symbol^ operator [](Int32 i)
	{
		return this->symbols[i];
	}

	void Add(Symbol^ S);

	void FilterOutSymbol(List<Symbol^>^ S);

	bool IsMatch(Word^ W);

	void Output();
	bool Overlap(Word^ W);
	bool OverlapHead(Word^ W);
	bool OverlapHeadTail(Word^ Head, Word^ Tail);
	bool OverlapTail(Word^ W);

	bool Produce(ProductionRule^ Rule);

	List<Symbol^>^ symbols;
	array<Int32>^ parikhVector;

	bool compressed;

	String^ ToString() override;
	String^ ToStringAbstract();

	Int32 sizeLimit = 50;

protected:

private:
};

