#pragma once

using namespace System;
using namespace System::Collections::Generic;

public ref class Symbol
{
public:
	Symbol(String^ X);
	Symbol(String^ X, Symbol^ P);
	Symbol(String^ X, Symbol^ P, Int32 Abstraction);
	Symbol(String^ X, Int32 Abstraction, Int32 VocabularyIndex);
	Symbol(String^ X, Int32 Abstraction, Int32 VocabularyIndex, bool IsModule);
	virtual ~Symbol();

	void Output();
	bool isModule;
	bool isTerminal;

	bool IsMatch(Symbol^ S)
	{
		return this->value == S->value;
	}

	String^ ToString() override
	{
		return this->value;
	}

	Symbol^ Parent()
	{
		return this->parent;
	}

	String^ Value()
	{
		return this->value;
	}
	
	void operator=(Symbol^ B)
	{
		this->value = B->value;
	}

	bool operator==(Symbol^ B)
	{
		return this->value == B->value;
	};

	bool operator!=(Symbol^ B)
	{
		return this->value != B->value;
	};

	bool branchStart;
	bool branchEnd;
	Int32 abstraction;
	Int32 vocabularyIndex;

	List<Symbol^>^ parametersSymbols;
	List<Int32>^ parametersInteger;

protected:

private:
	String^ value;
	Symbol^ parent;
};

