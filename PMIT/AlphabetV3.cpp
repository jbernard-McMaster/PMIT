#include "stdafx.h"
#include "AlphabetV3.h"


AlphabetV3::AlphabetV3()
{
	this->symbols = gcnew List<String^>;
	this->specials = gcnew List<String^>;
	this->homomorphisms = gcnew List<String^>;
	this->successors = gcnew List<Int32>;
}


AlphabetV3::~AlphabetV3()
{
}

void AlphabetV3::Add2DTurtleGraphics()
{
	this->AddSymbol("+", "+", AlphabetV3::Deterministic, false);
	this->AddSymbol("-", "-", AlphabetV3::Deterministic, false);
	this->AddSymbol("f", "f", AlphabetV3::Deterministic, false);
	this->AddSymbol("F", "F", AlphabetV3::Deterministic, false);
}

void AlphabetV3::Add3DTurtleGraphics()
{
	this->Add2DTurtleGraphics();
	this->AddSymbol("&", "&", AlphabetV3::Deterministic, false);
	this->AddSymbol("^", "^", AlphabetV3::Deterministic, false);
	this->AddSymbol("/", "/", AlphabetV3::Deterministic, false);
	this->AddSymbol("\\", "\\", AlphabetV3::Deterministic, false);
	this->AddSymbol("+", "+", AlphabetV3::Deterministic, false);
	this->AddSymbol("!", "!", AlphabetV3::Deterministic, false);
}

void AlphabetV3::AddBranching()
{
	this->AddSymbol("[", "[", AlphabetV3::Deterministic, false);
	this->AddSymbol("]", "]", AlphabetV3::Deterministic, false);
}

void AlphabetV3::AddSymbol(String^ S, String^ H, Int32 NumSuccessors, bool AsSymbol)
{
	// check to see if the symbol already exists
	bool found = false;
	Int32 iSymbol = 0;
	while ((!found) && (iSymbol < this->symbols->Count))
	{
		if (S == this->symbols[iSymbol])
		{
			found = true;
		}
		iSymbol++;
	}

	if (!found)
	{
		this->symbols->Add(S);
		this->homomorphisms->Add(H);

		if (!this->IsTurtle(S))
		{
			this->numNonTurtles++;
		}

		if (AsSymbol)
		{
			this->numSymbols++;
			this->successors->Add(NumSuccessors);
		}
		else
		{
			this->numSpecials++;
			this->successors->Add(AlphabetV3::Deterministic); // a special is always determinisitc
			this->specials->Add(S);
		}
	}
}

void AlphabetV3::ComputeIndices()
{
	this->IndexSymbolsStart = 0;
	this->IndexSymbolsEnd = this->numSymbols - 1;
	this->IndexSpecialsStart = this->IndexSymbolsEnd + 1;
	this->IndexSpecialsEnd = this->IndexSpecialsStart + (this->numSpecials - 1);
	this->IndexAlphabetStart = this->IndexSymbolsStart;
	this->IndexAlphabetEnd = this->IndexSpecialsEnd;
}

bool AlphabetV3::Contains(String^ S)
{
	return this->symbols->Contains(S);
}

void AlphabetV3::CreateParikhVector(WordV3^ W)
{
	array<Int32>^ result = gcnew array<Int32>(this->symbols->Count);
	W->parikhVector = this->CreateParikhVector(W->GetSymbolsNoCorrection(0, W->Length()));
	W->isCompressed = true;
}

array<Int32>^ AlphabetV3::CreateParikhVector(String^ S)
{
	array<Int32>^ result = gcnew array<Int32>(this->symbols->Count);

	for (size_t i = 0; i < this->symbols->Count; i++)
	{
		Int32 count = 0;
		for (size_t j = 0; j < S->Length; j++)
		{
			String^ s = S->Substring(j,1);
			if (s == this->symbols[i])
			{
				count++;
			}
		}

		result[i] = count;
	}

	return result;
}

void AlphabetV3::CreateSymbolIndexArray(WordXV3^ W)
{
	array<Int32>^ i = gcnew array<Int32>(W->Length());

	for (size_t iSymbol = 0; iSymbol < W->Length(); iSymbol++)
	{
		i[iSymbol] = this->FindIndex(W->GetSymbol(iSymbol));
	}

	W->SetSymbolIndices(i);
}

Int32 AlphabetV3::FindIndex(String^ S)
{
	Int32 result = -1;
	Int32 iSymbol = 0;

	if (S == AlphabetV3::Nothing)
	{
		return -1;
	}
	else
	{
		while ((result < 0) && (iSymbol < this->symbols->Count))
		{
			if (this->symbols[iSymbol] == S)
			{
				result = iSymbol;
			}
			iSymbol++;
		}
	}

	return result;
}

String^ AlphabetV3::GetSymbol(Int32 iSymbol)
{
	if (iSymbol == -1)
	{
		return "";
	}
	else
	{
		return this->symbols[iSymbol];
	}
}

Int32 AlphabetV3::GetLeftContext(WordV3^ W, Int32 iPos)
{
	String^ result = "";

	String^ S = W->GetSymbol(iPos);
	Int32 iSymbol = this->FindIndex(S);

	//Console::WriteLine("GetLeftContext: W = " + W->symbols + ", iPos = " + iPos + ", S = " + S + ", iSymbol = " + iSymbol);

	if ((iPos == 0) || (this->IsTurtle(iSymbol)))
	{
		result = AlphabetV3::Nothing;
	}
	else
	{
		Int32 iScan = 1;
		Int32 branchCount = 0;
		bool inBranch = false;
		bool found = false;
		String^ left = "";

		do
		{
			 left = W->GetSymbol(iPos - iScan);

			 // if it is a turtle, ignore it but...
			 if (this->IsTurtle(left))
			 {
				 // if it is a branch symbol, increment or decrement the branch counter accordingly
				 if (left == AlphabetV3::EndBranch)
				 {
					 branchCount++;
				 }
				 else if (left == AlphabetV3::StartBranch)
				 {
					 branchCount--;
				 }

				 if (branchCount < 0)
				 {
					 inBranch = true;
				 }
			 }
			 else if (branchCount == 0)
			 {// if it is a non-turtle, and not currently in a branch, then this is left context
				 found = true;
				 result = left;
			 }
			 else if (inBranch)
			 {// if it is a non-turtle, and the original symbol is in a branch, then this is the left context
				 found = true;
				 result = left;
			 }

			 iScan++;
		} while ((!found) && (iPos - iScan >= 0));
	}

	return this->FindIndex(result);
}

Int32 AlphabetV3::GetRightContext(WordV3^ W, Int32 iPos)
{
	String^ result = "";

	if ((iPos >= W->Length() - 1) || (this->IsTurtle(this->FindIndex(W->GetSymbol(iPos)))))
	{
		result = AlphabetV3::Nothing;
	}
	else
	{
		Int32 iScan = 1;
		Int32 branchCount = 0;
		bool inBranch = false;
		bool found = false;
		String^ right = "";

		do
		{
			right = W->GetSymbol(iPos + iScan);

			// if it is a turtle, ignore it but...
			if (this->IsTurtle(right))
			{
				// if it is a branch symbol, increment or decrement the branch counter accordingly
				// also raise a flag to grab the next non-turtle as the branch right context
				if (right == AlphabetV3::StartBranch)
				{
					branchCount++;
				}
				else if (right == AlphabetV3::EndBranch)
				{
					branchCount--;
				}

				if (branchCount < 0)
				{// symbols inside a branch do not have a right context unless it is the same branch as them
					inBranch = true;
					found = true;
				}
			}
			else if (branchCount == 0)
			{// if it is a non-turtle, and not currently in a branch then, this is right context
				found = true;
				result = right;
			}

			iScan++;
		} while ((!found) && (iPos + iScan < W->Length()));
	}

	return this->FindIndex(result);
}

String^ AlphabetV3::ReplaceSymbol(String^ S)
{
	return this->GetRandomSymbol_NonTurtle(S);
}

String^ AlphabetV3::GetRandomSymbol_NonTurtle()
{
	Int32 choice = CommonGlobals::PoolInt(0, this->SizeNonTurtle() - 1);
	return this->symbols[choice];
}

String^ AlphabetV3::GetRandomSymbol_NonTurtle(String^ S)
{
	bool found = false;
	Int32 choice;
	while (!found)
	{
		 choice = CommonGlobals::PoolInt(0, this->SizeNonTurtle() - 1);
		 if (this->symbols[choice] != S)
		 {
			 found = true;
		 }
	}

	return this->symbols[choice];
}

String^ AlphabetV3::GetRandomSymbol_Turtle(bool Only2D)
{
	Int32 choice;

	if (Only2D)
	{
		choice = CommonGlobals::PoolInt(1, 9);
	}
	else
	{
		choice = CommonGlobals::PoolInt(1, 4);
	}

	switch (choice)
	{
	case 1:
		return AlphabetV3::TurnLeft;
		break;
	case 2:
		return AlphabetV3::TurnRight;
		break;
	case 3:
		return AlphabetV3::LineDown;
		break;
	case 4:
		return AlphabetV3::LineUp;
		break;
	case 5:
		return AlphabetV3::Turn180;
		break;
	case 6:
		return AlphabetV3::RollLeft;
		break;
	case 7:
		return AlphabetV3::RollRight;
		break;
	case 8:
		return AlphabetV3::PitchDown;
		break;
	case 9:
		return AlphabetV3::PitchUp;
		break;
	default:
		break;
	}


}


bool AlphabetV3::IsLeftContextMatch(WordV3^ W, Int32 iSymbol, String^ Context)
{
	return false;
}

bool AlphabetV3::IsRightContextMatch(WordV3^ W, Int32 iSymbol, String^ Context)
{
	return false;
}


bool AlphabetV3::IsModule(String^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexSymbolsStart;

	while ((!found) && (iSymbol <= this->IndexSymbolsEnd))
	{
		if (this->symbols[iSymbol] == S)
		{
			found = true;
		}
		iSymbol++;
	}

	return found;
}

bool AlphabetV3::IsModule(Int32 I)
{
	return this->IsModule(this->symbols[I]);
}

bool AlphabetV3::IsConstant(String^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexSpecialsStart;

	while ((!found) && (iSymbol <= this->IndexSpecialsEnd))
	{
		if (this->symbols[iSymbol] == S)
		{
			found = true;
		}
		iSymbol++;
	}

	return found;
}

bool AlphabetV3::IsConstant(Int32 I)
{
	return this->IsConstant(this->symbols[I]);
}

bool AlphabetV3::IsTurtle(String^ S)
{
	if (this->specials->Contains(S) || S == AlphabetV3::TurnLeft || S == AlphabetV3::TurnRight || S == AlphabetV3::Turn180 || S == AlphabetV3::StartBranch || S == AlphabetV3::EndBranch || S == AlphabetV3::PitchUp || S == AlphabetV3::PitchDown || S == AlphabetV3::RollLeft || S == AlphabetV3::RollRight || S == AlphabetV3::LineDown || S == AlphabetV3::LineUp)
	{
		return true;
	}

	return false;
}

bool AlphabetV3::IsTurtle(Int32 I)
{
	return this->IsTurtle(this->symbols[I]);
}

// No set of turtle commands cannot have contradictory commands such as turn left and turn right
bool AlphabetV3::IsInvalidTurtleSet(List<String^>^ T)
{
	bool result = true;

	result = result && !(T->Contains(AlphabetV3::TurnLeft) && T->Contains(AlphabetV3::TurnRight));
	result = result && !(T->Contains(AlphabetV3::TurnLeft) && T->Contains(AlphabetV3::Turn180));
	result = result && !(T->Contains(AlphabetV3::TurnRight) && T->Contains(AlphabetV3::Turn180));

	result = result && !(T->Contains(AlphabetV3::RollLeft) && T->Contains(AlphabetV3::RollRight));

	result = result && !(T->Contains(AlphabetV3::PitchUp) && T->Contains(AlphabetV3::PitchDown));

	return result;
}

