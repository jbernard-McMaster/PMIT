#include "stdafx.h"
#include "Word.h"

using namespace System::Collections::Generic;


Word::Word()
{
	this->symbols = gcnew List<Symbol^>;
	this->compressed = false;
}

Word::Word(List<Symbol^>^ Symbols) : Word()
{
	this->symbols = gcnew List<Symbol^>;
	for (size_t i = 0; i < Symbols->Count; i++)
	{
		this->symbols->Add(Symbols[i]);
	}
	this->compressed = false;
}

Word::Word(Word^ A)
{
	this->symbols = gcnew List<Symbol^>;
	for (size_t iSymbol = 0; iSymbol < A->symbols->Count; iSymbol++)
	{
		this->symbols->Add(A->symbols[iSymbol]);
	}

	if (A->compressed)
	{
		this->parikhVector = gcnew array<Int32>(A->parikhVector->GetUpperBound(0) + 1);
		for (size_t iSymbol = 0; iSymbol <= A->parikhVector->GetUpperBound(0); iSymbol++)
		{
			this->parikhVector[iSymbol] = A->parikhVector[iSymbol];
		}
		this->compressed = A->compressed;
	}

}

Word::~Word()
{
}

void Word::Add(Symbol^ S)
{
	this->symbols->Add(S);
	this->compressed = false;
}

void Word::FilterOutSymbol(List<Symbol^>^ S)
{
	for (size_t iSymbol = 0; iSymbol < S->Count; iSymbol++)
	{
		this->symbols->RemoveAll(gcnew Predicate<Symbol^>(S[iSymbol], &Symbol::IsMatch));
	}
	this->compressed = false;
}

bool Word::IsMatch(Word^ W)
{
	bool result = true;

	if (this->symbols->Count == W->symbols->Count)
	{
		Int32 iSymbol = 0;
		while ((result) && (iSymbol < this->symbols->Count))
		{
			if (this->symbols[iSymbol] != W->symbols[iSymbol])
			{
				result = false;
			}
			iSymbol++;
		}
	}
	else
	{
		result = false;
	}

	return result;
}

void Word::Output()
{
	for each (Symbol^ s in this->symbols)
	{
		s->Output();
	}
	Console::WriteLine();
}

bool Word::Overlap(Word^ W)
{
	return false;
}

bool Word::OverlapHead(Word^ W)
{
	bool result = true;
	if (W->Count <= this->Count)
	{
		result = true;
		Int32 iSymbol = 0;

		while ((iSymbol < W->Count) && (result))
		{
			if (W[iSymbol] != this[iSymbol])
			{
				result = false;
			}
			iSymbol++;
		}
	}
	else 
	{
		result = false;
	}

	return result;
}

bool Word::OverlapHeadTail(Word^ Head, Word^ Tail)
{
	return this->OverlapHead(Head) && this->OverlapTail(Tail);
}

bool Word::OverlapTail(Word^ W)
{
	bool result = true;
	if (W->Count <= this->Count)
	{
		result = true;
		Int32 iSymbol = 0;

		while ((iSymbol < W->Count) && (result))
		{
			if (W[iSymbol] != this[this->Count - W->Count + iSymbol])
			{
				result = false;
			}
			iSymbol++;
		} 
	}
	else
	{
		result = false;
	}

	return result;
}

bool Word::Produce(ProductionRule^ Rule)
{
	List<Symbol^>^ result = gcnew List<Symbol^>(0);
	
	for (int index = 0; index < this->symbols->Count; index++)
	{
		Symbol^ s = this->symbols[index];

		if (Rule->IsMatch(s))
		{
			result->AddRange(Rule->successor);
		}
		else
		{
			result->Add(this->symbols[index]);
		}
	}
	
	this->symbols = result;
	return false;
}

String^ Word::ToString()
{
	String^ result = "";

	for each (Symbol^ s in this->symbols)
	{
		result = result + s->ToString();
	}

	return result;
}

String^ Word::ToStringAbstract()
{
	String^ result = "";

	for each (Symbol^ s in this->symbols)
	{
		result = result + s->abstraction.ToString(L"G");
	}

	return result;
}
