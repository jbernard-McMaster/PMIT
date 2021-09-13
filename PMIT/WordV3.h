#pragma once

#include "Fragment.h"

using namespace System;
using namespace System::Collections::Generic;

public ref struct Range
{
	Int32 start;
	Int32 end;
};

public ref class WordV3
{
public:
	WordV3();
	WordV3(WordV3^ W);
	virtual ~WordV3();

	bool isCompressed;

	array<Int32>^ parikhVector;

	virtual void Display();

	String^ GetSymbol(Int32 iPos) {
		return this->symbols->Substring(iPos, 1);
	}

	String^ GetSymbols() {
		return this->symbols;
	}

	String^ GetSymbols(Int32 iPos, Int32 Count) {
		if (iPos + Count < this->symbols->Length)
		{
			return this->symbols->Substring(iPos, Count);
		}
		else
		{
			return this->symbols->Substring(iPos, this->symbols->Length - iPos);
		}
	}

	String^ GetSymbolsNoCorrection(Int32 iPos, Int32 Count) {
		return this->symbols->Substring(iPos, Count);
	}

	String^ GetSymbolsFromPos(Int32 iPos1, Int32 iPos2) {
		Int32 length = Math::Min(1 + iPos2 - iPos1, this->symbols->Length - iPos1);
		return this->symbols->Substring(iPos1, length);
	}

	String^ GetLeftSignature(Int32 iPos, Int32 Length) {
		if (iPos != 0)
		{
			Int32 start = Math::Max(0, iPos - 1 - Length);
			Int32 end = Math::Max(0, iPos - 1);
			return this->symbols->Substring(start, 1 + end - start);
		}
		else
		{
			return "";
		}
	}

	String^ GetRightSignature(Int32 iPos, Int32 Length) {
		if (iPos != this->Length() - 1)
		{
			Int32 start = Math::Min(this->Length() - 1, iPos + 1);
			Int32 end = Math::Min(this->Length() - 1, iPos + 1 + Length);
			return this->symbols->Substring(start, 1 + end - start);
		}
		else
		{
			return "";
		}
	}

	void AppendSymbols(String^ S) {
		this->symbols += S;
	}

	void SetSymbols(String^ S)
	{
		this->symbols = S;
	}

	Int32 Length() { return this->symbols->Length; }
	String^ symbols;

protected:

};

public ref class WordXV3 :
	public WordV3
{
public:
	WordXV3();
	WordXV3(WordV3^ W);
	WordXV3(WordXV3^ W);
	virtual ~WordXV3();
	void ReInitialize();
	void Initialize(WordV3^ W);
	void Initialize(WordXV3^ W);

	List<String^>^ GetSubwords();
	List<String^>^ GetSubwordsOfLength(Int32 Length);

	Int32 GetSymbolIndex(Int32 iPos) {
		return this->symbolIndices[iPos];
	}

	Int32 GetDepth(Int32 iPos) {
		return this->depth[iPos];
	}

	void SetSymbolIndex(Int32 iPos, Int32 I)
	{
		this->symbolIndices[iPos] = I;
	}

	void SetSymbolIndices(array<Int32>^ Indices)
	{
		this->symbolIndices = Indices;
	}

	void SetDepth(array<Int32>^ Depth)
	{
		this->depth = Depth;
	}

	Int32 GetLeftContext(Int32 iPos) {
		return this->leftContext[iPos];
	}

	void SetLeftContext(Int32 iPos, Int32 I)
	{
		this->leftContext[iPos] = I;
	}

	void SetLeftContexts(array<Int32>^ Indices)
	{
		this->leftContext = Indices;
	}

	Int32 GetRightContext(Int32 iPos) {
		return this->rightContext[iPos];
	}

	void SetRightContext(Int32 iPos, Int32 I)
	{
		this->rightContext[iPos] = I;
	}

	void SetRightContexts(array<Int32>^ Indices)
	{
		this->rightContext = Indices;
	}

	array<Int32>^ GetContext(Int32 iPos) {
		array<Int32>^ result = gcnew array<Int32>(2);
		result[0] = this->GetLeftContext(iPos);
		result[1] = this->GetRightContext(iPos);
		return result;
	}

	Int32 Length() { return this->symbols->Length; }

	void SetWTW(Int32 iPos, Range^ R);
	void SetWTW(Int32 iPos, Int32 Start, Int32 End);
	void SetWTW_Direct(array<Range^>^ R) { this->wtw = R; }

	void SetBTF(Int32 iPos, FragmentSigned^ Fr) {	this->btf[iPos] = Fr; }
	void SetBTF_Direct(array<FragmentSigned^>^ L) {this->btf = L; }

	Range^ GetWTW(Int32 iPos) { return this->wtw[iPos]; }
	FragmentSigned^ GetBTF(Int32 iPos) { return this->btf[iPos]; }

	array<Int32>^ successorID;
	array<Int32>^ symbolIndices;
	array<Int32>^ leftContext;
	array<Int32>^ rightContext;
	array<Int32>^ depth;
	array<Range^>^ wtw;
	array<FragmentSigned^>^ btf;

private:
};

