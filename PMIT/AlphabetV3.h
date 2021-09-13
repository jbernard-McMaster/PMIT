#pragma once
#include "WordV3.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class AlphabetV3
{
public:
	AlphabetV3();
	virtual ~AlphabetV3();

	List<String^>^ symbols;
	List<String^>^ specials;
	List<String^>^ homomorphisms;
	List<Int32>^ successors;

	Int32 numSymbols;
	Int32 numSpecials;
	Int32 numNonTurtles;

	Int32 IndexSymbolsStart;
	Int32 IndexSymbolsEnd;
	Int32 IndexSpecialsStart;
	Int32 IndexSpecialsEnd;
	Int32 IndexAlphabetStart;
	Int32 IndexAlphabetEnd;

	virtual void Add2DTurtleGraphics();
	virtual void Add3DTurtleGraphics();
	virtual void AddBranching();

	virtual void AddSymbol(String^ S, String^ H, Int32 NumSuccessors, bool AsSymbol);

	void ComputeIndices();
	bool Contains(String^ S);

	void CreateParikhVector(WordV3^ W);
	array<Int32>^ CreateParikhVector(String^ S);
	void CreateSymbolIndexArray(WordXV3^ W);
	
	virtual Int32 FindIndex(String^ S);

	virtual String^ GetSymbol(Int32 iSymbol);
	virtual Int32 GetLeftContext(WordV3^ W, Int32 iPos);
	virtual Int32 GetRightContext(WordV3^ W, Int32 iPos);

	virtual String^ ReplaceSymbol(String^ S);
	virtual String^ GetRandomSymbol_NonTurtle();
	virtual String^ GetRandomSymbol_NonTurtle(String^ S);
	virtual String^ GetRandomSymbol_Turtle(bool Only2D);

	bool IsModule(String^ S);
	bool IsConstant(String^ S);
	bool IsTurtle(String^ S);
	virtual bool IsInvalidTurtleSet(List<String^>^ T);

	bool IsModule(Int32 I);
	bool IsConstant(Int32 I);
	bool IsTurtle(Int32 I);
	
	bool IsLeftContextMatch(WordV3^ W, Int32 iPos, String^ Context);
	bool IsRightContextMatch(WordV3^ W, Int32 iPos, String^ Context);

	virtual Int32 Size() { return this->symbols->Count; };
	virtual Int32 SizeNonTurtle() { return this->numNonTurtles; };

	String^ SymbolCondition(Int32 iSymbol, Int32 iLeft, Int32 iRight) {
		String^ result = "";

		if ((iLeft == this->SizeNonTurtle()) || (iLeft == -1))
		{
			result = "* < ";
		}
		else
		{
			result = this->symbols[iLeft] + " < ";
		}

		result += this->symbols[iSymbol];

		if ((iRight == this->SizeNonTurtle()) || (iRight == -1))
		{
			result += " > *";
		}
		else
		{
			result += " > " + this->symbols[iRight];
		}

		return result;
	}


	// 3D Turtle Alphabet
	String^ TurnLeft  = "+";
	String^ TurnRight = "-";
	String^ Turn180 = "!";
	String^ StartBranch = "[";
	String^ EndBranch = "]";
	String^ LineUp = "f";
	String^ LineDown = "F";
	String^ RollLeft = "/";
	String^ RollRight = "\\";
	String^ PitchUp = "^";
	String^ PitchDown = "&";

	static String^ Unknown = "?";
	static String^ Nothing = "?";

	static const Int32 Deterministic = 0;
	static const Int32 UnknownSuccessor = -1;
};

