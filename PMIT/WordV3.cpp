#include "stdafx.h"
#include "WordV3.h"


WordV3::WordV3()
{
	this->symbols = "";
	this->isCompressed = false;
}

WordV3::WordV3(WordV3^ W)
{
	this->symbols = W->symbols;
	this->isCompressed = W->isCompressed;
	if (W->isCompressed)
	{
		this->parikhVector = gcnew array<Int32>(W->parikhVector->GetUpperBound(0) + 1);

		for (size_t iSymbol = 0; iSymbol <= W->parikhVector->GetUpperBound(0); iSymbol++)
		{
			this->parikhVector[iSymbol] = W->parikhVector[iSymbol];
		}
	}
}


WordV3::~WordV3()
{
}

void WordV3::Display()
{
	Console::WriteLine(this->symbols);
}

WordXV3::WordXV3() : WordV3()
{
}

WordXV3::WordXV3(WordV3^ W) : WordV3(W)
{
	this->Initialize(W);
}

WordXV3::WordXV3(WordXV3^ W) : WordV3(W)
{
	this->Initialize(W);
}

WordXV3::~WordXV3()
{
}

void WordXV3::ReInitialize()
{
	this->symbolIndices = gcnew array<Int32>(this->Length());
	this->successorID = gcnew array<Int32>(this->Length());
	this->leftContext = gcnew array<Int32>(this->Length());
	this->rightContext = gcnew array<Int32>(this->Length());
	this->depth = gcnew array<Int32>(this->Length());
	this->wtw = gcnew array<Range^>(this->Length());
	this->btf = gcnew array<FragmentSigned^>(this->Length());

	Int32 d = 0;
	for (size_t i = 0; i < this->Length(); i++)
	{
		if (this->GetSymbol(i) == "[")
		{
			d++;
		}
		else if (this->GetSymbol(i) == "]")
		{
			d--;
		}

		this->wtw[i] = gcnew Range();
		this->wtw[i]->start = 0;
		this->wtw[i]->end = this->Length() - 1;

		this->btf[i] = gcnew FragmentSigned();

		this->depth[i] = d;
	}
}

void WordXV3::Initialize(WordV3^ W)
{
	this->symbolIndices = gcnew array<Int32>(W->Length());
	this->successorID = gcnew array<Int32>(W->Length());
	this->leftContext = gcnew array<Int32>(W->Length());
	this->rightContext = gcnew array<Int32>(W->Length());
	this->wtw = gcnew array<Range^>(W->Length());
	this->btf = gcnew array<FragmentSigned^>(W->Length());
	this->depth = gcnew array<Int32>(W->Length());

	Int32 d = 0;
	for (size_t i = 0; i < W->Length(); i++)
	{
		if (W->GetSymbol(i) == "[")
		{
			d++;
		}
		else if (W->GetSymbol(i) == "]")
		{
			d--;
		}

		this->wtw[i] = gcnew Range();
		this->wtw[i]->start = 0;
		this->wtw[i]->end = W->Length()-1;
		this->btf[i] = gcnew FragmentSigned();
		this->depth[i] = d;
	}

	for (size_t i = 0; i <= this->successorID->GetUpperBound(0); i++)
	{
		this->successorID[i] = -1;
	}
}

void WordXV3::Initialize(WordXV3^ W)
{
	this->Initialize((WordV3^)W);

	if (W->symbolIndices != nullptr)
	{
		for (size_t i = 0; i < W->Length(); i++)
		{
			this->symbolIndices[i] = W->symbolIndices[i];
		}
	}

	if (W->leftContext != nullptr)
	{
		for (size_t i = 0; i < W->Length(); i++)
		{
			this->leftContext[i] = W->leftContext[i];
		}
	}

	if (W->rightContext != nullptr)
	{
		for (size_t i = 0; i < W->Length(); i++)
		{
			this->rightContext[i] = W->rightContext[i];
		}
	}

	if (W->depth != nullptr)
	{
		for (size_t i = 0; i < W->Length(); i++)
		{
			this->depth[i] = W->depth[i];
		}
	}

}

List<String^>^ WordXV3::GetSubwords()
{
	List<String^>^ result = gcnew List<String^>;

	return result;
}

List<String^>^ WordXV3::GetSubwordsOfLength(Int32 Length)
{
	List<String^>^ result = gcnew List<String^>;

	for (size_t iStart = 0; iStart <= this->symbols->Length - Length; iStart++)
	{
		result->Add(this->symbols->Substring(iStart, Length));
	}

	return result;
}

void WordXV3::SetWTW(Int32 iPos, Range^ R)
{
	this->SetWTW(iPos, R->start, R->end);
}

void WordXV3::SetWTW(Int32 iPos, Int32 Start, Int32 End)
{
	if (this->wtw[iPos]->start < Start)
	{
		this->wtw[iPos]->start = Start;
	}

	if (this->wtw[iPos]->end > End)
	{
		this->wtw[iPos]->end = End;
	}
}