#include "stdafx.h"
#include "Vocabulary.h"


Vocabulary::Vocabulary()
{
	this->Symbols = gcnew List<Symbol^>(0);
	this->Abstracts = gcnew List<Symbol^>(0);
	this->YieldsSymbol = gcnew Symbol("=>");
	this->SwapSymbol = gcnew Symbol("SWP");

	//this->Operations = gcnew List<Symbol^>(0);
	//this->Special = gcnew List<Symbol^>(0);

	//Symbol^ swap = gcnew Symbol(Vocabulary::SwapSymbol); // used to temporarily indicate a symbol is being swapped
	//this->Special->Add(swap);

	//this->Operations->Add(gcnew Symbol("=>"));

	//this->Special->Add(gcnew Symbol("*"));

	this->branchingAllowed = false;
}

Vocabulary::Vocabulary(bool BranchingAllowed) : Vocabulary::Vocabulary()
{
	this->branchingAllowed = BranchingAllowed;
}

Vocabulary::~Vocabulary()
{
}

//void Vocabulary::AddSymbol(String^ Value)
//{
//	this->Symbols->Add(gcnew Symbol(Value, 0, this->Symbols->Count));
//	this->Abstracts->Add(gcnew Symbol((this->Abstracts->Count).ToString(), 0, this->Abstracts->Count));
//}

void Vocabulary::AddSymbol(String^ Value, bool AsModule)
{
	this->Symbols->Add(gcnew Symbol(Value, 0, this->Symbols->Count, AsModule));
	this->Abstracts->Add(gcnew Symbol((this->Abstracts->Count).ToString(), 0, this->Abstracts->Count, AsModule));

	if (AsModule)
	{
		this->numModules++;
	}
	else
	{
		this->numConstants++;
	}
}

void Vocabulary::AddSymbol(Symbol^ S, bool AsModule)
{

	if (AsModule)
	{
		S->isModule = true;
		this->numModules++;
	}
	else
	{
		S->isModule = false;
		this->numConstants++;
	}

	this->Symbols->Add(S);
}

// Returns a list of all constants
List<Symbol^>^ Vocabulary::AllConstants()
{
	List<Symbol^>^ result = gcnew List<Symbol^>;

	for (size_t i = this->IndexConstantsStart; i <= this->IndexConstantsEnd; i++)
	{
		result->Add(this->Symbols[i]);
	}

	return result;
}

// Returns a list of all modules
List<Symbol^>^ Vocabulary::AllModules()
{
	List<Symbol^>^ result = gcnew List<Symbol^>;

	for (size_t i = this->IndexModulesStart; i <= this->IndexModulesEnd; i++)
	{
		result->Add(this->Symbols[i]);
	}

	return result;
}

void Vocabulary::ComputeIndices()
{
	this->IndexModulesStart = 0;
	this->IndexModulesEnd = this->numModules - 1;
	this->IndexConstantsStart = this->IndexModulesEnd + 1;
	this->IndexConstantsEnd = this->IndexConstantsStart + (this->numConstants - 1);
	this->IndexSymbolsStart = this->IndexModulesStart;
	this->IndexSymbolsEnd = this->IndexConstantsEnd;
	this->IndexAlpha = this->IndexConstantsEnd + 1;
}

void Vocabulary::CompressSymbols(Word^ A)
{
	A->parikhVector = gcnew array<Int32>(this->Symbols->Count);

	for (size_t i = 0; i < this->Symbols->Count; i++)
	{
		Int32 count = 0;
		for (size_t j = 0; j < A->symbols->Count; j++)
		{
			if (A->symbols[j] == this->Symbols[i])
			{
				count++;
			}
		}

		A->parikhVector[i] = count;
	}

	A->compressed = true;
}

array<Int32>^ Vocabulary::CompressSymbols(List<Symbol^>^ A)
{
	array<Int32>^ parikhVector = gcnew array<Int32>(this->Symbols->Count);

	for (size_t i = 0; i < this->Symbols->Count; i++)
	{
		Int32 count = 0;
		for (size_t j = 0; j < A->Count; j++)
		{
			if (A[j] == this->Symbols[i])
			{
				count++;
			}
		}

		parikhVector[i] = count;
	}

	return parikhVector;
}

Word^ Vocabulary::CreateWord(String^ S)
{
	Word^ result = gcnew Word();

	for (size_t i = 0; i < S->Length; i++)
	{
		String^ c = S->Substring(i, 1);
		result->Add(this->FindSymbol(c));
	}

	return result;
}

Int32 Vocabulary::CountConstants(Word^ A)
{
	Int32 result = 0;

	for (size_t j = 0; j < A->symbols->Count; j++)
	{
		if (this->IsConstant(A->symbols[j]))
		{
			result++;
		}
	}

	return result;
}

Symbol^ Vocabulary::FindSymbol(String^ V)
{
	Symbol^ result;
	Int32 idx = 0;
	bool found = false;

	do
	{
		if (this->Symbols[idx]->Value() == V)
		{
			result = this->Symbols[idx];
			found = true;
		}
		idx++;
	} while ((!found) && (idx < this->Symbols->Count));

	return result;
}

void Vocabulary::Load(String^ FileName)
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + FileName);

	array<Char>^ seperator = gcnew array<Char>{','};
	ConfigFileLine lineNum = Vocabulary::ConfigFileLine::Modules;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();
		array<String^, 1>^ words;

		words = line->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);

		switch ((int)lineNum)
		{
		case (int)Vocabulary::ConfigFileLine::Modules:
			if (words[1] != "*")
			{
				for (int i = 1; i < words->Length; i++)
				{
					Symbol^ s = gcnew Symbol(words[i]);
					s->abstraction = i;
					s->vocabularyIndex = i;
					this->Symbols->Add(s);
				}
			}
			break;
		case (int)Vocabulary::ConfigFileLine::Operations:
			break;
		case (int)Vocabulary::ConfigFileLine::Instructions:
			break;
		case (int)Vocabulary::ConfigFileLine::Special:
			break;
		}

		lineNum++;
	}

	sr->Close();

	this->RegularSymbolEnd = this->Symbols->Count;
}

Int32 Vocabulary::FindIndex(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = -1;

	while ((!found) && (iSymbol < this->Symbols->Count-1))
	{
		iSymbol++;
		if (this->Symbols[iSymbol] == S)
		{
			found = true;
		}
	}

	return iSymbol;
}

Int32 Vocabulary::FindIndex(String^ V)
{
	bool found = false;
	Int32 iSymbol = this->IndexModulesStart;

	while ((!found) && (iSymbol < this->Symbols->Count))
	{
		if (this->Symbols[iSymbol]->Value() == V)
		{
			found = true;
		}
		else
		{
			iSymbol++;
		}
	}

	return iSymbol;
}

Int32 Vocabulary::RuleIndex(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexModulesStart;

	while ((!found) && (iSymbol <= this->IndexModulesEnd))
	{
		if (this->Symbols[iSymbol] == S)
		{
			found = true;
		}
		else
		{
			iSymbol++;
		}
	}

#if _PMI_PROBLEM_ERROR_CHECKING_ > 0
	if (!found)
	{
		throw gcnew Exception("Symbol " + S->ToString() + " not in vocabulary");
	}
#endif

	return iSymbol;
}

Int32 Vocabulary::ConstantIndex(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexConstantsStart;

	while ((!found) && (iSymbol <= this->IndexConstantsEnd))
	{
		if (this->Symbols[iSymbol] == S)
		{
			found = true;
		}
		else
		{
			iSymbol++;
		}
	}

	return iSymbol;
}

bool Vocabulary::IsModule(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexModulesStart;

	while ((!found) && (iSymbol <= this->IndexModulesEnd))
	{
		if (this->Symbols[iSymbol] == S)
		{
			found = true;
		}
		iSymbol++;
	}

	return found;
}

bool Vocabulary::IsConstant(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = this->IndexConstantsStart;

	while ((!found) && (iSymbol <= this->IndexConstantsEnd))
	{
		if (this->Symbols[iSymbol] == S)
		{
			found = true;
		}
		iSymbol++;
	}

	return found;
}

bool Vocabulary::Contains(Symbol^ S)
{
	bool found = false;
	Int32 iSymbol = 0;

	while ((!found) && (iSymbol <= this->IndexConstantsEnd))
	{
		if (this->Symbols[iSymbol]->Value() == S->Value())
		{
			found = true;
		}
		iSymbol++;
	}

	return found;
}

Int32 Vocabulary::ParikhVectorLength(array<Int32>^ PV)
{
	Int32 result = 0;

	for (size_t i = 0; i <= PV->GetUpperBound(0); i++)
	{
		result += PV[i];
	}

	return result;
}

void Vocabulary::SwitchSymbol(Symbol^ S)
{
	Int32 ruleIndex = this->FindIndex(S);
	this->Symbols->RemoveAt(ruleIndex);
	this->AddSymbol(S, false);
	this->numModules--;
	this->ComputeIndices();
}