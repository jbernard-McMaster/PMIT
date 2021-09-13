#include "stdafx.h"
#include "ModelV3.h"


ModelV3::ModelV3()
{
	this->Initialize();
}

ModelV3::ModelV3(String^ FileName)
{
	this->Initialize();
	this->Load(FileName);
	this->CreateEvidence(true);
}

ModelV3::ModelV3(String^ FileName, bool WithOverlays)
{
	this->Initialize();
	this->Load(FileName);
	this->CreateEvidence(WithOverlays);
}

ModelV3::~ModelV3()
{
}

void ModelV3::Initialize()
{
	this->alphabet = gcnew AlphabetV3();
	this->alphabetShadow = gcnew AlphabetV3();
	this->axiom = gcnew WordXV3();
	this->productions = gcnew List<ProductionTableV3^>;
	this->parameters = gcnew List<String^>;
	this->successorLongest = 0;
	this->successorShortest = 999;
}

List<SAC^>^ ModelV3::ComputeSACsInWord(WordXV3^ W)
{
	List<SAC^>^ result = gcnew List<SAC^>;

	for (size_t iPos = 0; iPos < W->Length(); iPos++)
	{
		Int32 symbolIndex = W->GetSymbolIndex(iPos);

		if (!this->alphabet->IsTurtle(symbolIndex))
		{
			Int32 iLeft = W->GetLeftContext(iPos);
			Int32 iRight = W->GetRightContext(iPos);
			//TODO: extract the parameter values

			SAC^ s = gcnew SAC();
			s->iSymbol = symbolIndex;
			s->iLeft = iLeft;
			s->iRight = iRight;

			if (!this->IsSACInList(result, s))
			{
				result->Add(s);
			}
		}
	}

	return result;
}

WordXV3^ ModelV3::ComputeWordX(WordV3^ W)
{
	WordXV3^ w = gcnew WordXV3(W);
	array<Int32>^ symbolIndices1 = gcnew array<Int32>(W->Length());
	array<Int32>^ leftContext1 = gcnew array<Int32>(W->Length());
	array<Int32>^ rightContext1 = gcnew array<Int32>(W->Length());
	array<Int32>^ depth1 = gcnew array<Int32>(W->Length());

	Int32 d = 0;
	for (size_t iPos = 0; iPos < W->Length(); iPos++)
	{
		String^ s = W->GetSymbol(iPos);
		Int32 symbolIndex = this->alphabet->FindIndex(s);
		symbolIndices1[iPos] = symbolIndex;
		leftContext1[iPos] = this->alphabet->GetLeftContext(W, iPos);
		rightContext1[iPos] = this->alphabet->GetRightContext(W, iPos);
		if (s == this->alphabet->StartBranch)
		{
			d++;
		}
		else if (s == this->alphabet->EndBranch)
		{
			d--;
		}
		depth1[iPos] = d;
	}

	w->SetSymbolIndices(symbolIndices1);
	w->SetLeftContexts(leftContext1);
	w->SetRightContexts(rightContext1);
	w->SetDepth(depth1);
	this->alphabet->CreateParikhVector(w);

	return w;
}

ModelV3^ ModelV3::Adjust(Int32 NumSymbol)
{
	ModelV3^ result = gcnew ModelV3();

	// Adjust the alphabet
	for (size_t iSymbol = 0; iSymbol < NumSymbol; iSymbol++)
	{
		result->alphabet->AddSymbol(this->alphabet->GetSymbol(iSymbol), this->alphabet->homomorphisms[iSymbol], this->alphabet->successors[iSymbol], this->alphabet->IsModule(iSymbol));
	}

	// Copy the evidence
	result->evidence = gcnew array<WordXV3^>(this->evidence->GetUpperBound(0)+1);

	for (size_t iGen = 0; iGen <= this->evidence->GetUpperBound(0); iGen++)
	{
		result->evidence[iGen] = gcnew WordXV3(this->evidence[iGen]);
	}

	// Adjust the evidence
	for (size_t iSymbol = 0; iSymbol < this->alphabet->numSymbols; iSymbol++)
	{
		if (!result->alphabet->symbols->Contains(this->alphabet->symbols[iSymbol]))
		{
			for (size_t iGen = 0; iGen <= this->evidence->GetUpperBound(0); iGen++)
			{
				
			}
		}
	}


	return result;
}

void ModelV3::ComputeGenerations()
{
	// Create the evidence for this solution
	WordV3^ a = gcnew WordV3(this->axiom);
	WordXV3^ x = this->ComputeWordX(a);

	Int32 tableIndex = CommonGlobals::PoolInt(0, this->productions->Count - 1);
	Int32 iGen = 0;
	Int32 stopAt = this->generations;

	List<SAC^>^ global = gcnew List<SAC^>;

	do
	{
		x = EvidenceFactoryV3::Process(x, this->productions[tableIndex]->rules, iGen);
		x = this->ComputeWordX(x);

		// check to see if any new SACs have been added
		List<SAC^>^ local = this->ComputeSACsInWord(x);
		bool change = false;

		for (size_t iSAC = 0; iSAC < local->Count; iSAC++)
		{
			if (!this->IsSACInList(global, local[iSAC]))
			{
				Console::WriteLine("Found new SAC " + this->alphabet->SymbolCondition(local[iSAC]->iSymbol, local[iSAC]->iLeft, local[iSAC]->iRight) + " in Generation " + iGen);
				global->Add(local[iSAC]);
				change = true;
			}
		}

		if (change)
		{
			stopAt = Math::Min(50, Math::Max(this->generations, iGen * 2));
		}
		else
		{
			Console::WriteLine("No new SACs in generation " + iGen);
		}

		iGen++;
	} while (iGen <= stopAt);

	Console::WriteLine(global->Count + " unique SACs found");
}

void ModelV3::ComputeOddsProduced()
{
	this->oddsProduced = 1.0;

	for (size_t iRule = 0; iRule < this->productions[0]->rules->Count; iRule++)
	{
		Int32 total = 0;

		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iRule]->successor->Count; iSuccessor++)
		{
			total += this->productions[0]->rules[iRule]->count[iSuccessor];
		}

		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iRule]->successor->Count; iSuccessor++)
		{
			this->oddsProduced *= Math::Pow((double)this->productions[0]->rules[iRule]->successor->Count / total, this->productions[0]->rules[iRule]->successor->Count);
		}
	}
}


void ModelV3::CreateError(float Pnop, float Pr, float Pi, float Pd, bool ReplacementAllowed, bool InsertionAllowed, bool DeletionAllowed)
{
	for (size_t iGen = 1; iGen < this->generations; iGen++)
	{
		String^ replacement = "";
		for (size_t iPos = 0; iPos < this->words[iGen]->Length(); iPos++)
		{
			float roll = CommonGlobals::PoolFloat();
			String^ curr = this->words[iGen]->GetSymbol(iPos);

			if (roll < Pr)
			{
				//Console::WriteLine(roll + ": Replace");
				replacement = replacement + this->alphabet->ReplaceSymbol(curr);
			}
			else if (roll < Pr + Pi)
			{
				//Console::WriteLine(roll + ": Insert");
				replacement = replacement + this->alphabet->GetRandomSymbol_NonTurtle();
			}
			else if (roll < Pr + Pi + Pd)
			{
				//Console::WriteLine(roll + ": Delete");
			}
			else
			{
				//Console::WriteLine(roll + ": NOP");
				replacement = replacement + curr;
			}
		}
		this->words[iGen]->Display();
		Console::WriteLine(replacement);

		this->words[iGen]->SetSymbols(replacement);
	}
}

void ModelV3::CreateOverlays()
{
	// Apply homomorphism and compute parikh vectors, and any overlays
	this->evidence = gcnew array<WordXV3^>(this->words->Length);
	for (size_t iGen = 0; iGen <= this->words->GetUpperBound(0); iGen++)
	{
		WordXV3^ w = gcnew WordXV3();
		array<Int32>^ symbolIndices1 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ symbolIndices2 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ leftContext1 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ rightContext1 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ depth1 = gcnew array<Int32>(this->words[iGen]->Length());

		Int32 d = 0;
		for (size_t iPos = 0; iPos < this->words[iGen]->Length(); iPos++)
		{
			String^ s = this->words[iGen]->GetSymbol(iPos);
			Int32 symbolIndex = this->alphabetShadow->FindIndex(s);
			symbolIndices1[iPos] = symbolIndex;
			leftContext1[iPos] = this->alphabetShadow->GetLeftContext(this->words[iGen], iPos);
			rightContext1[iPos] = this->alphabetShadow->GetRightContext(this->words[iGen], iPos);

			w->AppendSymbols(this->alphabetShadow->homomorphisms[symbolIndex]);
			symbolIndex = this->alphabet->FindIndex(this->alphabetShadow->homomorphisms[symbolIndex]);
			symbolIndices2[iPos] = symbolIndex;

			if (s == this->alphabet->StartBranch)
			{
				d++;
			}
			else if (s == this->alphabet->EndBranch)
			{
				d--;
			}

			depth1[iPos] = d;
		}

		this->words[iGen]->SetSymbolIndices(symbolIndices1);
		this->words[iGen]->SetLeftContexts(leftContext1);
		this->words[iGen]->SetRightContexts(rightContext1);
		this->words[iGen]->SetRightContexts(depth1);
		w->SetSymbolIndices(symbolIndices2);

		// Compute the contexts for the homomorphized word
		array<Int32>^ leftContext2 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ rightContext2 = gcnew array<Int32>(this->words[iGen]->Length());

		for (size_t iPos = 0; iPos < w->Length(); iPos++)
		{
			leftContext2[iPos] = this->alphabet->GetLeftContext(w, iPos);
			rightContext2[iPos] = this->alphabet->GetRightContext(w, iPos);
		}

		w->SetLeftContexts(leftContext2);
		w->SetRightContexts(rightContext2);
		w->SetDepth(depth1);

		this->alphabet->CreateParikhVector(this->words[iGen]);
		this->alphabet->CreateParikhVector(w);
		this->evidence[iGen] = w;
	}

	for (size_t iGen = 0; iGen <= this->words->GetUpperBound(0) - 1; iGen++)
	{
		array<Range^>^ btw1 = gcnew array<Range^>(this->words[iGen]->Length());
		array<FragmentSigned^>^ btf1 = gcnew array<FragmentSigned^>(this->words[iGen]->Length());

		for (size_t iPos = 0; iPos < this->words[iGen]->Length(); iPos++)
		{
			Range^ r = gcnew Range();
			r->start = 0;
			r->end = this->words[iGen + 1]->Length() - 1;
			btw1[iPos] = r;
		}
		this->words[iGen]->SetWTW_Direct(btw1);
		this->evidence[iGen]->SetWTW_Direct(btw1);
		this->words[iGen]->SetBTF_Direct(btf1);
		this->evidence[iGen]->SetBTF_Direct(btf1);
	}
}

void ModelV3::CreateEvidence()
{
	this->CreateEvidence(true);
}

void ModelV3::CreateEvidence(bool WithOverlays)
{
	if (this->generations >= 1)
	{
		// Create the evidence for this solution
		WordXV3^ a = gcnew WordXV3(this->axiom);

		Int32 tableIndex = CommonGlobals::PoolInt(0, this->productions->Count - 1);

		// Create the words
		this->words = EvidenceFactoryV3::Process(a, this->productions[tableIndex]->rules, 0, this->generations, false);
	}
	else
	{
		this->CreateEvidence_Dynamic();
	}

	//WordXV3^ x1 = gcnew WordXV3();
	//x1->SetSymbols("F-F-F");
	//x1->ReInitialize();

	//WordXV3^ x2 = gcnew WordXV3();
	//x2->SetSymbols("F-F-F+F+FF+FF-FFF-FF-F-F+F+FF+FF-FF-F-F+F+F-FF-F-F+F+FF+FF");
	//x2->ReInitialize();

	//WordXV3^ x3 = gcnew WordXV3();
	//x3->SetSymbols("F+F+F-FF-F-F+F+F-F-FF-F+F+FFF+F+F-F-F+F+F-F-F+F+FF+F-F-F+F+F-F-FF-F+F+F-F-F+F+F-FF-F-F+F+F-F-F+F+F-F-FFF-F-F+FF+F+F-F-F+F+FF+F-F-F+FF+F+F-F-F+F+F-FF-F-F+F+F-F-F+F+F-F-FFF-F-F+FF+F+F-F-F+F+FF+F-F-FF-F+F+F-FF-F-F+F+F-F-FF-F+F+FFF+F+F-F-F+F+F-F-F+F+FF+F-F-F+F+F-F-F");
	//x3->ReInitialize();
	//
	//this->words[0] = x1;
	//this->words[1] = x2;
	//this->words[2] = x3;

	if (WithOverlays)
	{
		this->CreateOverlays();
	}
	else
	{
		this->evidence = gcnew array<WordXV3^>(this->words->GetUpperBound(0)+1);
		for (size_t iGen = 0; iGen <= this->words->GetUpperBound(0); iGen++)
		{
			this->evidence[iGen] = this->words[iGen];
		}

	}
}

void ModelV3::CreateEvidenceS0L()
{
	if (this->generations >= 1)
	{
		// Create the evidence for this solution
		WordXV3^ a = gcnew WordXV3(this->axiom);

		Int32 tableIndex = CommonGlobals::PoolInt(0, this->productions->Count - 1);

		// Create the words
		this->words = EvidenceFactoryV3::Process(a, this->productions[tableIndex]->rules, 0, this->generations, false);
	}
	else
	{
		this->CreateEvidence_Dynamic();
	}

	// Apply homomorphism and compute parikh vectors, and any overlays
	this->evidence = gcnew array<WordXV3^>(this->words->Length);
	for (size_t iGen = 0; iGen <= this->words->GetUpperBound(0); iGen++)
	{
		WordXV3^ w = gcnew WordXV3();
		array<Int32>^ symbolIndices1 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ symbolIndices2 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ leftContext1 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ rightContext1 = gcnew array<Int32>(this->words[iGen]->Length());

		for (size_t iPos = 0; iPos < this->words[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = this->alphabet->FindIndex(this->words[iGen]->GetSymbol(iPos));
			symbolIndices1[iPos] = symbolIndex;
			leftContext1[iPos] = this->alphabet->GetLeftContext(this->words[iGen], iPos);
			rightContext1[iPos] = this->alphabet->GetRightContext(this->words[iGen], iPos);

			w->AppendSymbols(this->alphabet->homomorphisms[symbolIndex]);
			symbolIndex = this->alphabet->FindIndex(this->alphabet->homomorphisms[symbolIndex]);
			symbolIndices2[iPos] = symbolIndex;
		}

		this->words[iGen]->SetSymbolIndices(symbolIndices1);
		this->words[iGen]->SetLeftContexts(leftContext1);
		this->words[iGen]->SetRightContexts(rightContext1);
		w->SetSymbolIndices(symbolIndices2);

		// Compute the contexts for the homomorphized word
		array<Int32>^ leftContext2 = gcnew array<Int32>(this->words[iGen]->Length());
		array<Int32>^ rightContext2 = gcnew array<Int32>(this->words[iGen]->Length());

		for (size_t iPos = 0; iPos < w->Length(); iPos++)
		{
			leftContext2[iPos] = this->alphabet->GetLeftContext(w, iPos);
			rightContext2[iPos] = this->alphabet->GetRightContext(w, iPos);
		}

		w->SetLeftContexts(leftContext2);
		w->SetRightContexts(rightContext2);

		this->alphabet->CreateParikhVector(this->words[iGen]);
		this->alphabet->CreateParikhVector(w);
		this->evidence[iGen] = w;
	}

	for (size_t iGen = 0; iGen <= this->words->GetUpperBound(0) - 1; iGen++)
	{
		array<Range^>^ btw1 = gcnew array<Range^>(this->words[iGen]->Length());
		array<FragmentSigned^>^ btf1 = gcnew array<FragmentSigned^>(this->words[iGen]->Length());

		for (size_t iPos = 0; iPos < this->words[iGen]->Length(); iPos++)
		{
			Range^ r = gcnew Range();
			r->start = 0;
			r->end = this->words[iGen + 1]->Length() - 1;
			btw1[iPos] = r;
		}
		this->words[iGen]->SetWTW_Direct(btw1);
		this->evidence[iGen]->SetWTW_Direct(btw1);
		this->words[iGen]->SetBTF_Direct(btf1);
		this->evidence[iGen]->SetBTF_Direct(btf1);
	}
}

void ModelV3::CreateEvidence_Dynamic()
{
	// Create the evidence for this solution
	WordV3^ a = gcnew WordV3(this->axiom);
	WordXV3^ x = this->ComputeWordX(a);

	Int32 tableIndex = CommonGlobals::PoolInt(0, this->productions->Count - 1);
	Int32 iGen = 0;
	Int32 stopAt = this->alphabet->SizeNonTurtle()+1;
	Int32 last = 0;

	List<SAC^>^ global = gcnew List<SAC^>;

	do
	{
		x = EvidenceFactoryV3::Process(x, this->productions[tableIndex]->rules, iGen);
		x = this->ComputeWordX(x);
		Array::Resize(this->words, iGen+1);
		this->words[iGen] = x;

		// check to see if any new SACs have been added
		List<SAC^>^ local = this->ComputeSACsInWord(x);
		bool change = false;

		for (size_t iSAC = 0; iSAC < local->Count; iSAC++)
		{
			if (!this->IsSACInList(global, local[iSAC]))
			{
				Console::WriteLine("Found new SAC " + this->alphabet->SymbolCondition(local[iSAC]->iSymbol, local[iSAC]->iLeft, local[iSAC]->iRight) + " in Generation " + iGen);
				global->Add(local[iSAC]);
				change = true;
			}
		}

		if (change)
		{
			//stopAt = Math::Min(50, (iGen+1) * 2);
			stopAt = Math::Min(50, iGen+3);
			last = iGen;
		}
		else
		{
			Console::WriteLine("No new SACs in generation " + iGen);
		}

		iGen++;
	} while (iGen <= stopAt);

	Array::Resize(this->words, last+2); // +2 because we want the next generation and last is 0-indexed

	Console::WriteLine(global->Count + " unique SACs found");
}

void ModelV3::Filter()
{
	for (size_t iGen = 0; iGen < this->generations; iGen++)
	{
		String^ replacement = "";
	}
}

void ModelV3::Display()
{
	Console::WriteLine("Alphabet");
	Console::Write("Symbols     : ");
	for (size_t iSymbol = this->alphabet->IndexAlphabetStart; iSymbol <= this->alphabet->IndexAlphabetEnd; iSymbol++)
	{
		Console::Write(this->alphabet->symbols[iSymbol] + " ");
	}
	Console::WriteLine();
	Console::Write("Homomorphism: ");
	for (size_t iSymbol = this->alphabet->IndexSymbolsStart; iSymbol <= this->alphabet->IndexSymbolsEnd; iSymbol++)
	{
		Console::Write(this->alphabet->homomorphisms[iSymbol] + " ");
	}
	Console::WriteLine();

	for (size_t iTable = 0; iTable < this->productions->Count; iTable++)
	{
		Console::WriteLine("Table #" + iTable + " Rules");
		for (size_t iRule = 0; iRule < this->alphabet->numSymbols; iRule++)
		{
			this->productions[iTable]->rules[iRule]->Display();
		}
	}

	Console::WriteLine();
	Console::WriteLine("Productions");

	for (size_t iWord = 0; iWord < this->words->GetUpperBound(0); iWord++)
	{
		Console::Write(iWord.ToString(L"D2") + ": ");
		this->words[iWord]->Display();
	}

	Console::WriteLine();
	Console::WriteLine("Evidence");

	for (size_t iWord = 0; iWord < this->evidence->GetUpperBound(0); iWord++)
	{
		Console::Write(iWord.ToString(L"D2") + ": ");
		this->evidence[iWord]->Display();
	}
}

void ModelV3::Load(String^ FileName)
{
	this->Load(FileName, gcnew List<String^>);
}

void ModelV3::Load(String^ FileName, List<String^>^ Filters)
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + FileName);

	array<Char>^ seperatorComma = gcnew array<Char>{','};
	array<Char>^ seperatorSpace = gcnew array<Char>{' '};
	ConfigFileLine lineNum = ModelV3::ConfigFileLine::VocabularyFile;

	int valueI;
	float valueF;
	bool valueB;
	Int32 ruleNum = 0;
	//this->successorModules = gcnew List<Int32>;
	//this->successorConstants = gcnew List<Int32>;

	bool ignore = false;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();

		if ((line != "") && (!ignore))
		{
			array<String^, 1>^ words;

			if (line->Substring(0, 1) != "#" && line != "***")
			{
				if (((int)lineNum >= (int)ModelV3::ConfigFileLine::Rules) || ((int)lineNum == (int)ModelV3::ConfigFileLine::Axiom))

				{
					words = line->Split(seperatorSpace, System::StringSplitOptions::RemoveEmptyEntries);
				}
				else
				{
					words = line->Split(seperatorComma, System::StringSplitOptions::RemoveEmptyEntries);
				}

				if ((int)lineNum == (int)ModelV3::ConfigFileLine::VocabularyFile)
				{
					array<String^, 1>^ parts = words[1]->Split(gcnew array<Char>{'|'});
					array<String^, 1>^ symbols = parts[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
					array<String^, 1>^ specials = parts[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
					array<String^, 1>^ homomorphisms = parts[2]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
					array<String^, 1>^ successors = parts[3]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);

					this->numSymbols = symbols->Length;
					this->numSpecials = specials->Length;

					for (size_t i = 0; i < symbols->Length; i++)
					{
						if (!Filters->Contains(symbols[i]))
						{
							int::TryParse(successors[i], valueI);
							this->alphabetShadow->AddSymbol(symbols[i], homomorphisms[i], valueI, true);
						}
					}

					for (size_t i = 0; i < homomorphisms->Length; i++)
					{
						if (!Filters->Contains(symbols[i]))
						{
							int::TryParse(successors[i], valueI);
							this->alphabet->AddSymbol(homomorphisms[i], homomorphisms[i], valueI, true);
						}
					}

					for (size_t i = 0; i < specials->Length; i++)
					{
						if (!Filters->Contains(specials[i]))
						{
							this->alphabet->AddSymbol(specials[i], specials[i], AlphabetV3::Deterministic, false);
							this->alphabetShadow->AddSymbol(specials[i], specials[i], AlphabetV3::Deterministic, false);
						}
					}

					this->alphabet->ComputeIndices();
				}
				else if ((int)lineNum == (int)ModelV3::ConfigFileLine::Generations)
				{
					if (words[1] == "*")
					{
						//this->generations = (this->numSymbols) + 1;
						this->generations = -1;
					}
					else
					{
						int::TryParse(words[1], valueI);
						this->generations = valueI;
					}
				}
				else if ((int)lineNum == (int)ModelV3::ConfigFileLine::Axiom)
				{
					array<String^, 1>^ symbols = words[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
					
					for (size_t i = 0; i < symbols->Length; i++)
					{
						if (!Filters->Contains(symbols[i]))
						{
							bool found = false;
							Int32 iSymbol = 0;

							this->axiom->AppendSymbols(symbols[i]);
						}
					}

					this->alphabet->CreateParikhVector(this->axiom);
				}
				else if ((int)lineNum == (int)ModelV3::ConfigFileLine::Table)
				{
					array<String^, 1>^ odds = words[1]->Split(gcnew array<Char>{'|'}, System::StringSplitOptions::RemoveEmptyEntries);

					for (size_t iTable = 0; iTable <= odds->GetUpperBound(0); iTable++)
					{
						int::TryParse(odds[iTable], valueI);
						this->productions->Add(gcnew ProductionTableV3(valueI));
					}
				}
				else if ((int)lineNum == (int)ModelV3::ConfigFileLine::Parameters)
				{
					// TODO: need to add a capability to bound the parameter values
					if (words->Length > 1)
					{
						array<String^, 1>^ parameters = words[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);

						for (size_t iParameter = 0; iParameter <= parameters->GetUpperBound(0); iParameter++)
						{
							this->parameters->Add(parameters[iParameter]);
						}
					}
				}
				else if ((int)lineNum >= (int)ModelV3::ConfigFileLine::Rules)
				{
					this->LoadRule(words, this->parameters, Filters);
					ruleNum++;
				}
				lineNum++;
			}
			else if (line == "***")
			{
				ignore = true;
			}
		}
	}

	sr->Close();
}

void ModelV3::LoadRule(array<String^, 1>^ Words, List<String^>^ Parameters, List<String^>^ Filters)
{
	// Parts Legend
	// 0 : rule name, does nothing
	// 1 : table index
	// 2 : odds of firing
	// 3 : special condition for time, should only be used when time is the only parameter of interest
	// 4 : predecessor including context
	// 5 : condition
	// 6 : homomorphism
	// 7 : successor

	Int32 tableIndex;
	Int32 odds;
	Int32 timeStart;
	Int32 timeEnd;

	String^ ruleName = Words[ModelV3::cRuleName];
	int::TryParse(Words[ModelV3::cTableIndex], tableIndex);
	int::TryParse(Words[ModelV3::cOdds], odds);
	//int::TryParse(Words[ModelV3::cTimeConditionStart], timeStart);
	//int::TryParse(Words[ModelV3::cTimeConditionEnd], timeEnd);
	String^ predecessor = Words[ModelV3::cPredecessor];
	String^ leftContext = Words[ModelV3::cLeftContext];
	String^ rightContext = Words[ModelV3::cRightContext];
	String^ condition = Words[ModelV3::cCondition];
	String^ successor = Words[ModelV3::cSuccessor];

	if (!Filters->Contains(predecessor))
	{
		// Apply filters
		if (Filters->Contains(leftContext))
		{
			leftContext = "*";
		}

		if (Filters->Contains(rightContext))
		{
			rightContext = "*";
		}

		for (size_t iFilter = 0; iFilter < Filters->Count; iFilter++)
		{
			successor = successor->Replace(Filters[iFilter], "");
		}

		// Check to see if a rule with this predecessor exists in the production table
		ProductionRuleV3^ r;
		Int32 iRule = 0;
		bool found = false;
		while ((!found) && (iRule < this->productions[tableIndex]->rules->Count))
		{
			if (this->productions[tableIndex]->rules[iRule]->predecessor == predecessor)
			{
				found = true;
				r = this->productions[tableIndex]->rules[iRule];
			}
			iRule++;
		}

		// If not create the rule set
		if (!found)
		{
			r = gcnew ProductionRuleV3();
			r->predecessor = predecessor;
			this->productions[tableIndex]->rules->Add(r);
		}

		// Add the successor
		r->successor->Add(successor);

		if (successor->Length > this->successorLongest)
		{
			this->successorLongest = successor->Length;
		}

		if (successor->Length < this->successorShortest)
		{
			this->successorShortest = successor->Length;
		}

		// Add context
		r->contextLeft->Add(leftContext);
		r->contextRight->Add(rightContext);

		// Set chance of firing
		r->odds->Add(odds);

		// Add special timing conditions
		//r->timeStart->Add(timeStart);
		//r->timeEnd->Add(timeEnd);

		// Add regular condition
		r->condition->Add(condition);

		// Add parameters
		r->parameters = Parameters;

		r->count->Add(0);
	}
}

void ModelV3::Save()
{
	//String^ dateString = DateTime::Now.ToString("d");
	//String^ timeString = DateTime::Now.Hour.ToString() + DateTime::Now.Minute.ToString();

	//String^ path = "D://PMIT//random_models//random_" + dateString + "_" + timeString + CommonGlobals::PoolInt(0, 999999).ToString() + ".txt";

	//System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter(path, false);

	//sw->WriteLine("Vocabulary");
	//sw->Write("Modules:   ");
	//for (size_t iSymbol = this->vocabulary->IndexModulesStart; iSymbol <= this->vocabulary->IndexModulesEnd; iSymbol++)
	//{
	//	sw->Write(this->vocabulary->Symbols[iSymbol] + ",");
	//}
	//sw->WriteLine();

	//sw->Write("Constants: ");
	//for (size_t iSymbol = this->vocabulary->IndexConstantsStart; iSymbol <= this->vocabulary->IndexConstantsEnd; iSymbol++)
	//{
	//	sw->Write(this->vocabulary->Symbols[iSymbol] + ",");
	//}
	//sw->WriteLine();

	//sw->WriteLine("Rules");
	//for (size_t iRule = 0; iRule < this->vocabulary->numModules; iRule++)
	//{
	//	sw->Write(this->rules[iRule]->predecessor->Value() + " => ");
	//	for (size_t iSymbol = 0; iSymbol < this->rules[iRule]->successor->Count; iSymbol++)
	//	{
	//		sw->Write(this->rules[iRule]->successor[iSymbol]);
	//	}
	//	sw->WriteLine();
	//}

	//sw->Close();
}

void ModelV3::GenerateD2L()
{
	this->lengthDistribution = gcnew array<float>(10);
	this->symbolDistribution = gcnew array<float>(6);

	this->lengthDistribution[0] = 0.04; // 0.04
	this->lengthDistribution[1] = 0.20; // 0.16
	this->lengthDistribution[2] = 0.40; // 0.20
	this->lengthDistribution[3] = 0.60; // 0.20
	this->lengthDistribution[4] = 0.80; // 0.20
	this->lengthDistribution[5] = 0.93; // 0.13
	this->lengthDistribution[6] = 0.97; // 0.04
	this->lengthDistribution[7] = 0.98; // 0.1
	this->lengthDistribution[8] = 0.99; // 0.1
	this->lengthDistribution[9] = 1.00; // 0.1

	this->symbolDistribution[0] = 0.20; // 0.20
	this->symbolDistribution[1] = 0.60; // 0.50
	this->symbolDistribution[2] = 0.90; // 0.20
	this->symbolDistribution[3] = 0.95; // 0.05
	this->symbolDistribution[4] = 0.98; // 0.03
	this->symbolDistribution[5] = 1.00; // 0.02

	Int32 total = 0;

	for (size_t iSymbol = 0; iSymbol < this->productions[0]->rules->Count; iSymbol++)
	{
		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			bool valid = false;
			String^ successor = "";

			while (!valid)
			{
				successor = "";
				Int32 length = 0;
				Int32 numSymbols = 0;

				float roll = CommonGlobals::PoolFloat();
				Int32 idx = 0;

				while (roll > this->lengthDistribution[idx])
				{
					idx++;
				}
				length = idx + 1;

				idx = 0;
				roll = CommonGlobals::PoolFloat();
				while (roll > this->symbolDistribution[idx])
				{
					idx++;
				}
				numSymbols = Math::Min(this->alphabet->numNonTurtles, idx + 1);
				total += numSymbols;

				if (total > iSymbol * 6)
				{
					numSymbols = CommonGlobals::PoolInt(2, 4);
				}

				List<Int32>^ symbols = gcnew List<Int32>(numSymbols);
				if (iSymbol < this->productions[0]->rules->Count-1)
				{
					symbols->Add(iSymbol + 1);
				}
				else
				{
					symbols->Add(0);
				}

				for (size_t j = 1; j < numSymbols; j++)
				{
					Int32 tmp;
					Int32 count = 0;
					do
					{
						tmp = CommonGlobals::PoolInt(0, this->alphabet->numNonTurtles - 1);
						count++;
					} while (symbols->Contains(tmp) && count < 10);

					symbols->Add(tmp);
				}

				bool flag1 = false;
				for (size_t j = 0; j < length; j++)
				{
					float roll = CommonGlobals::PoolFloat();

					if ((roll <= ((float)j / length)) && (!flag1))
					{
						flag1 = true;
						successor += this->alphabet->GetSymbol(symbols[0]);
					}
					else
					{
						successor += this->alphabet->GetSymbol(symbols[CommonGlobals::PoolInt(0, symbols->Count - 1)]);
					}
				}

				valid = true;
				//for (size_t jSuccessor = 0; jSuccessor < iSuccessor; jSuccessor++)
				//{
				//	Int32 tmp;
				//	if (this->productions[0]->rules[iSymbol]->successor[jSuccessor]->Length < successor->Length)
				//	{
				//		tmp = successor->IndexOf(this->productions[0]->rules[iSymbol]->successor[jSuccessor]);
				//	}
				//	else
				//	{
				//		tmp = this->productions[0]->rules[iSymbol]->successor[jSuccessor]->IndexOf(successor);
				//	}
				//	valid = valid && (tmp != 0);
				//}
			}

			Console::WriteLine("Generated successor for SAC " + this->productions[0]->rules[iSymbol]->contextLeft[iSuccessor] + " < " + this->alphabet->GetSymbol(iSymbol) + " > " + this->productions[0]->rules[iSymbol]->contextRight[iSuccessor] +" -> " + successor);
			this->productions[0]->rules[iSymbol]->successor[iSuccessor] = successor;
		}
	}
	this->productions[0]->rules = this->SortProductionRules(this->productions[0]->rules);
}

void ModelV3::GenerateS0L()
{
	if (this->productions[0]->rules->Count >= 3)
	{
		this->productions[0]->rules->RemoveRange(2, this->productions[0]->rules->Count - 2);
	}

	if (this->alphabet->numNonTurtles > 2)
	{
		this->alphabet->symbols->RemoveRange(2, this->alphabet->symbols->Count - 2);
		this->alphabet->homomorphisms->RemoveRange(2, this->alphabet->homomorphisms->Count - 2);
		this->alphabet->successors->RemoveRange(2, this->alphabet->successors->Count - 2);
		this->alphabet->numNonTurtles = 2;
	}

	for (size_t iRule = 0; iRule < this->productions[0]->rules->Count; iRule++)
	{
		this->productions[0]->rules[iRule]->successor->RemoveRange(1, this->productions[0]->rules[iRule]->successor->Count - 1);
		this->productions[0]->rules[iRule]->count->RemoveRange(1, this->productions[0]->rules[iRule]->count->Count - 1);
		this->productions[0]->rules[iRule]->contextLeft->RemoveRange(1, this->productions[0]->rules[iRule]->contextLeft->Count - 1);
		this->productions[0]->rules[iRule]->contextRight->RemoveRange(1, this->productions[0]->rules[iRule]->contextRight->Count - 1);
		this->productions[0]->rules[iRule]->condition->RemoveRange(1, this->productions[0]->rules[iRule]->condition->Count - 1);
		this->productions[0]->rules[iRule]->odds->RemoveRange(1, this->productions[0]->rules[iRule]->odds->Count - 1);
	}

	this->lengthDistribution = gcnew array<float>(10);
	this->symbolDistribution = gcnew array<float>(6);

	this->lengthDistribution[0] = 0.04; // 0.04
	this->lengthDistribution[1] = 0.20; // 0.16
	this->lengthDistribution[2] = 0.40; // 0.20
	this->lengthDistribution[3] = 0.60; // 0.20
	this->lengthDistribution[4] = 0.80; // 0.20
	this->lengthDistribution[5] = 0.93; // 0.13
	this->lengthDistribution[6] = 0.97; // 0.04
	this->lengthDistribution[7] = 0.98; // 0.1
	this->lengthDistribution[8] = 0.99; // 0.1
	this->lengthDistribution[9] = 1.00; // 0.1

	this->symbolDistribution[0] = 0.20; // 0.20
	this->symbolDistribution[1] = 0.60; // 0.50
	this->symbolDistribution[2] = 0.90; // 0.20
	this->symbolDistribution[3] = 0.95; // 0.05
	this->symbolDistribution[4] = 0.98; // 0.03
	this->symbolDistribution[5] = 1.00; // 0.02

	Int32 total = 0;

	Int32 numSuccessorsTotal = CommonGlobals::PoolInt(4, 7);
	Int32 numSymbols = CommonGlobals::PoolInt(2, numSuccessorsTotal - 1);
	Int32 absMin = 1;
	Int32 absMax = 3;
	Int32 reserve = numSymbols - 1;
	Int32 totalSuccessors = 0;

	for (size_t iSymbol = 0; iSymbol < numSymbols; iSymbol++)
	{
		// determine number of successors for this symbol
		Int32 min = Math::Max(absMin, (int)Math::Floor(((float)(numSuccessorsTotal - totalSuccessors) / (reserve + 1))));
		Int32 max = Math::Min(absMax, numSuccessorsTotal - totalSuccessors - reserve);

		Int32 numSuccessors = CommonGlobals::PoolInt(min, max);
		totalSuccessors += numSuccessors;
		reserve--;

		if (iSymbol > 1)
		{
			switch (iSymbol)
			{
			case 2:
				this->alphabet->AddSymbol("C", "C", -1, true);
				break;
			case 3:
				this->alphabet->AddSymbol("D", "D", -1, true);
				break;
			case 4:
				this->alphabet->AddSymbol("E", "E", -1, true);
				break;
			case 5:
				this->alphabet->AddSymbol("G", "G", -1, true);
				break;
			case 6:
				this->alphabet->AddSymbol("H", "H", -1, true);
				break;
			case 7:
				this->alphabet->AddSymbol("I", "I", -1, true);
				break;
			case 8:
				this->alphabet->AddSymbol("J", "J", -1, true);
				break;
			case 9:
				this->alphabet->AddSymbol("K", "K", -1, true);
				break;
			case 10:
				this->alphabet->AddSymbol("L", "L", -1, true);
				break;
			default:
				break;
			}
		}

		if (this->productions[0]->rules->Count < this->alphabet->numNonTurtles)
		{
			this->productions[0]->rules->Add(gcnew ProductionRuleV3());
			this->productions[0]->rules[iSymbol]->predecessor = this->alphabet->GetSymbol(this->alphabet->numNonTurtles - 1);
		}

		for (size_t iSuccessor = 0; iSuccessor < numSuccessors; iSuccessor++)
		{
			if ((iSymbol > 1) || (iSymbol <= 1 && iSuccessor > 0))
			{
				this->productions[0]->rules[iSymbol]->successor->Add("A");
				this->productions[0]->rules[iSymbol]->condition->Add("*");
				this->productions[0]->rules[iSymbol]->contextLeft->Add("*");
				this->productions[0]->rules[iSymbol]->contextRight->Add("*");
				this->productions[0]->rules[iSymbol]->odds->Add(100);
				this->productions[0]->rules[iSymbol]->count->Add(0);
			}
			else
			{
				this->productions[0]->rules[iSymbol]->count[0] = 0;
			}
		}
	}

	bool firstMultiSuccessor = true;
	for (size_t iSymbol = 0; iSymbol < this->productions[0]->rules->Count; iSymbol++)
	{
		Int32 totalOdds = 0;
		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			String^ successor = "";
			Int32 odds = CommonGlobals::PoolInt(1, 100);
			totalOdds += odds;
			this->productions[0]->rules[iSymbol]->odds[iSuccessor] = odds;
			float roll;

			bool valid = false;

			while (!valid)
			{
				successor = "";
				Int32 length = 0;
				Int32 numSymbols = 0;

				roll = CommonGlobals::PoolFloat();
				Int32 idx = 0;

				while (roll > this->lengthDistribution[idx])
				{
					idx++;
				}
				length = idx + 1;

				if (iSuccessor == 0)
				{
					length = Math::Max(2, length);
				}

				idx = 0;
				roll = CommonGlobals::PoolFloat();
				while (roll > this->symbolDistribution[idx])
				{
					idx++;
				}
				numSymbols = Math::Min(this->alphabet->numNonTurtles, idx + 1);
				total += numSymbols;

				if (total > iSymbol * 6)
				{
					numSymbols = CommonGlobals::PoolInt(2, 4);
				}

				List<Int32>^ symbols = gcnew List<Int32>(numSymbols);
				if (iSymbol < this->productions[0]->rules->Count - 1)
				{
					symbols->Add(iSymbol + 1);
				}
				else
				{
					symbols->Add(0);
				}

				for (size_t j = 1; j < numSymbols; j++)
				{
					Int32 tmp;
					Int32 count = 0;
					do
					{
						tmp = CommonGlobals::PoolInt(0, this->alphabet->numNonTurtles - 1);
						count++;
					} while (symbols->Contains(tmp) && count < 10);

					symbols->Add(tmp);
				}

				bool flag1 = false;
				for (size_t j = 0; j < length; j++)
				{
					float roll = CommonGlobals::PoolFloat();

					if ((roll <= ((float)j / length)) && (!flag1))
					{
						flag1 = true;
						successor += this->alphabet->GetSymbol(symbols[0]);
					}
					else
					{
						successor += this->alphabet->GetSymbol(symbols[CommonGlobals::PoolInt(0, symbols->Count - 1)]);
					}
				}

				valid = true;
				for (size_t jRule = 0; jRule < this->productions[0]->rules->Count; jRule++)
				{
					for (size_t jSuccessor = 0; jSuccessor < this->productions[0]->rules[jRule]->successor->Count; jSuccessor++)
					{
						valid = valid && (successor != this->productions[0]->rules[jRule]->successor[jSuccessor]);
					}
				}
			}

			this->productions[0]->rules[iSymbol]->successor[iSuccessor] = successor;
		}

		// Normalize odds
		Int32 newTotal = 0;
		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			Int32 normalizedOdds = 100 * ((float)this->productions[0]->rules[iSymbol]->odds[iSuccessor] / totalOdds);
			this->productions[0]->rules[iSymbol]->odds[iSuccessor] = normalizedOdds;
			newTotal += normalizedOdds;
		}

		if (newTotal < 100)
		{
			this->productions[0]->rules[iSymbol]->odds[0] += 100 - newTotal;
		}

		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			Console::WriteLine("Generated successor for SAC " + this->alphabet->GetSymbol(iSymbol) + " : " + this->productions[0]->rules[iSymbol]->successor[iSuccessor] + ", p = " + this->productions[0]->rules[iSymbol]->odds[iSuccessor]);
		}
	}
}

void ModelV3::GenerateS0LPrefix()
{
	if (this->productions[0]->rules->Count >= 3)
	{
		this->productions[0]->rules->RemoveRange(2, this->productions[0]->rules->Count - 2);
	}

	if (this->alphabet->numNonTurtles > 2)
	{
		this->alphabet->symbols->RemoveRange(2, this->alphabet->symbols->Count - 2);
		this->alphabet->homomorphisms->RemoveRange(2, this->alphabet->homomorphisms->Count - 2);
		this->alphabet->successors->RemoveRange(2, this->alphabet->successors->Count - 2);
		this->alphabet->numNonTurtles = 2;
	}

	for (size_t iRule = 0; iRule < this->productions[0]->rules->Count; iRule++)
	{
		this->productions[0]->rules[iRule]->successor->RemoveRange(1, this->productions[0]->rules[iRule]->successor->Count - 1);
		this->productions[0]->rules[iRule]->count->RemoveRange(1, this->productions[0]->rules[iRule]->count->Count - 1);
		this->productions[0]->rules[iRule]->contextLeft->RemoveRange(1, this->productions[0]->rules[iRule]->contextLeft->Count - 1);
		this->productions[0]->rules[iRule]->contextRight->RemoveRange(1, this->productions[0]->rules[iRule]->contextRight->Count - 1);
		this->productions[0]->rules[iRule]->condition->RemoveRange(1, this->productions[0]->rules[iRule]->condition->Count - 1);
		this->productions[0]->rules[iRule]->odds->RemoveRange(1, this->productions[0]->rules[iRule]->odds->Count - 1);
	}

	this->lengthDistribution = gcnew array<float>(10);
	this->symbolDistribution = gcnew array<float>(6);

	this->lengthDistribution[0] = 0.04; // 0.04
	this->lengthDistribution[1] = 0.20; // 0.16
	this->lengthDistribution[2] = 0.40; // 0.20
	this->lengthDistribution[3] = 0.60; // 0.20
	this->lengthDistribution[4] = 0.80; // 0.20
	this->lengthDistribution[5] = 0.93; // 0.13
	this->lengthDistribution[6] = 0.97; // 0.04
	this->lengthDistribution[7] = 0.98; // 0.1
	this->lengthDistribution[8] = 0.99; // 0.1
	this->lengthDistribution[9] = 1.00; // 0.1

	this->symbolDistribution[0] = 0.20; // 0.20
	this->symbolDistribution[1] = 0.60; // 0.50
	this->symbolDistribution[2] = 0.90; // 0.20
	this->symbolDistribution[3] = 0.95; // 0.05
	this->symbolDistribution[4] = 0.98; // 0.03
	this->symbolDistribution[5] = 1.00; // 0.02

	Int32 total = 0;

	Int32 numSuccessorsTotal = CommonGlobals::PoolInt(4,7);
	Int32 numSymbols = CommonGlobals::PoolInt(2, numSuccessorsTotal - 1);
	Int32 absMin = 1;
	Int32 absMax = 3;
	Int32 reserve = numSymbols - 1;
	Int32 totalSuccessors = 0;

	for (size_t iSymbol = 0; iSymbol < numSymbols; iSymbol++)
	{
		// determine number of successors for this symbol
		Int32 min = Math::Max(absMin, (int)Math::Floor(((float)(numSuccessorsTotal - totalSuccessors) / (reserve+1))));
		Int32 max = Math::Min(absMax, numSuccessorsTotal - totalSuccessors - reserve);

		Int32 numSuccessors = CommonGlobals::PoolInt(min, max);
		totalSuccessors += numSuccessors;
		reserve--;

		if (iSymbol > 1)
		{
			switch (iSymbol)
			{
			case 2:
				this->alphabet->AddSymbol("C", "C", -1, true);
				break;
			case 3:
				this->alphabet->AddSymbol("D", "D", -1, true);
				break;
			case 4:
				this->alphabet->AddSymbol("E", "E", -1, true);
				break;
			case 5:
				this->alphabet->AddSymbol("G", "G", -1, true);
				break;
			case 6:
				this->alphabet->AddSymbol("H", "H", -1, true);
				break;
			case 7:
				this->alphabet->AddSymbol("I", "I", -1, true);
				break;
			case 8:
				this->alphabet->AddSymbol("J", "J", -1, true);
				break;
			case 9:
				this->alphabet->AddSymbol("K", "K", -1, true);
				break;
			case 10:
				this->alphabet->AddSymbol("L", "L", -1, true);
				break;
			default:
				break;
			}
		}

		if (this->productions[0]->rules->Count < this->alphabet->numNonTurtles)
		{
			this->productions[0]->rules->Add(gcnew ProductionRuleV3());
			this->productions[0]->rules[iSymbol]->predecessor = this->alphabet->GetSymbol(this->alphabet->numNonTurtles-1);
		}

		for (size_t iSuccessor = 0; iSuccessor < numSuccessors; iSuccessor++)
		{
			if ((iSymbol > 1) || (iSymbol <= 1 && iSuccessor > 0))
			{
				this->productions[0]->rules[iSymbol]->successor->Add("A");
				this->productions[0]->rules[iSymbol]->condition->Add("*");
				this->productions[0]->rules[iSymbol]->contextLeft->Add("*");	
				this->productions[0]->rules[iSymbol]->contextRight->Add("*");
				this->productions[0]->rules[iSymbol]->odds->Add(100);
				this->productions[0]->rules[iSymbol]->count->Add(0);
			}
			else
			{
				this->productions[0]->rules[iSymbol]->count[0] = 0;
			}
		}
	}

	bool firstMultiSuccessor = true;
	for (size_t iSymbol = 0; iSymbol < this->productions[0]->rules->Count; iSymbol++)
	{
		Int32 totalOdds = 0;
		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			String^ successor = "";
			Int32 odds = CommonGlobals::PoolInt(1, 100);
			totalOdds += odds;
			this->productions[0]->rules[iSymbol]->odds[iSuccessor] = odds;
			float roll = CommonGlobals::PoolFloat();

			if ((iSuccessor == 1 && firstMultiSuccessor) || (iSuccessor == 1 && roll <= 0.20))
			{
				firstMultiSuccessor = false;
				Int32 newLength = CommonGlobals::PoolInt(1, this->productions[0]->rules[iSymbol]->successor[0]->Length - 1);
				successor = this->productions[0]->rules[iSymbol]->successor[0]->Substring(0, newLength);
			}
			else 
			{
				bool valid = false;

				while (!valid)
				{
					successor = "";
					Int32 length = 0;
					Int32 numSymbols = 0;

					roll = CommonGlobals::PoolFloat();
					Int32 idx = 0;

					while (roll > this->lengthDistribution[idx])
					{
						idx++;
					}
					length = idx + 1;

					if (iSuccessor == 0)
					{
						length = Math::Max(2, length);
					}

					idx = 0;
					roll = CommonGlobals::PoolFloat();
					while (roll > this->symbolDistribution[idx])
					{
						idx++;
					}
					numSymbols = Math::Min(this->alphabet->numNonTurtles, idx + 1);
					total += numSymbols;

					if (total > iSymbol * 6)
					{
						numSymbols = CommonGlobals::PoolInt(2, 4);
					}

					List<Int32>^ symbols = gcnew List<Int32>(numSymbols);
					if (iSymbol < this->productions[0]->rules->Count - 1)
					{
						symbols->Add(iSymbol + 1);
					}
					else
					{
						symbols->Add(0);
					}

					for (size_t j = 1; j < numSymbols; j++)
					{
						Int32 tmp;
						Int32 count = 0;
						do
						{
							tmp = CommonGlobals::PoolInt(0, this->alphabet->numNonTurtles - 1);
							count++;
						} while (symbols->Contains(tmp) && count < 10);

						symbols->Add(tmp);
					}

					bool flag1 = false;
					for (size_t j = 0; j < length; j++)
					{
						float roll = CommonGlobals::PoolFloat();

						if ((roll <= ((float)j / length)) && (!flag1))
						{
							flag1 = true;
							successor += this->alphabet->GetSymbol(symbols[0]);
						}
						else
						{
							successor += this->alphabet->GetSymbol(symbols[CommonGlobals::PoolInt(0, symbols->Count - 1)]);
						}
					}

					valid = true;
					for (size_t jRule = 0; jRule < this->productions[0]->rules->Count; jRule++)
					{
						for (size_t jSuccessor = 0; jSuccessor < this->productions[0]->rules[jRule]->successor->Count; jSuccessor++)
						{
							valid = valid && (successor != this->productions[0]->rules[jRule]->successor[jSuccessor]);
						}
					}
				}
			}

			this->productions[0]->rules[iSymbol]->successor[iSuccessor] = successor;
		}

		// Normalize odds
		Int32 newTotal = 0;
		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			Int32 normalizedOdds = 100 * ((float)this->productions[0]->rules[iSymbol]->odds[iSuccessor] / totalOdds);
			this->productions[0]->rules[iSymbol]->odds[iSuccessor] = normalizedOdds;
			newTotal += normalizedOdds;
		}

		if (newTotal < 100)
		{
			this->productions[0]->rules[iSymbol]->odds[0] += 100 - newTotal;
		}

		for (size_t iSuccessor = 0; iSuccessor < this->productions[0]->rules[iSymbol]->successor->Count; iSuccessor++)
		{
			Console::WriteLine("Generated successor for SAC " + this->alphabet->GetSymbol(iSymbol) + " : " + this->productions[0]->rules[iSymbol]->successor[iSuccessor] + ", p = " + this->productions[0]->rules[iSymbol]->odds[iSuccessor]);
		}
	}
}

bool ModelV3::IsMatch(ModelV3^ M)
{
	bool result = true;

	Int32 iRule1 = 0;

	for (size_t iTable = 0; iTable < M->productions->Count; iTable++)
	{
		do
		{
			for (size_t iRule2 = 0; iRule2 < M->productions[iTable]->rules->Count; iRule2++)
			{
				// if the predecessor are the same and the lengths match
				if (this->productions[iTable]->rules[iRule1]->predecessor == M->productions[iTable]->rules[iRule2]->predecessor)
				{
					if (this->productions[iTable]->rules[iRule1]->successor->Count == M->productions[iTable]->rules[iRule2]->successor->Count)
					{
						for (size_t iSymbol = 0; iSymbol < this->productions[iTable]->rules[iRule1]->successor->Count; iSymbol++)
						{
							if (this->productions[iTable]->rules[iRule1]->successor[iSymbol] != M->productions[iTable]->rules[iRule2]->successor[iSymbol])
							{
								result = false;
							}
						}
					}
					else
					{
						result = false;
					}
				}
			}
		iRule1++;
		} while ((iRule1 < this->productions[iTable]->rules->Count) && (result));
	}

	return result;
} 

bool ModelV3::IsSACInList(List<SACX^>^ L, SACX^ S)
{
	bool found = false;
	Int32 iSAC = 0;

	while ((!found) && (iSAC < L->Count))
	{
		if ((L[iSAC]->iGen == S->iGen) && (L[iSAC]->iLeft == S->iLeft) && (L[iSAC]->iRight == S->iRight) && (L[iSAC]->iSymbol == S->iSymbol))
		{
			found = true;
		}
		iSAC++;
	}

	return found;
}

bool ModelV3::IsSACInList(List<SAC^>^ L, SAC^ S)
{
	bool found = false;
	Int32 iSAC = 0;

	while ((!found) && (iSAC < L->Count))
	{
		if ((L[iSAC]->iLeft == S->iLeft) && (L[iSAC]->iRight == S->iRight) && (L[iSAC]->iSymbol == S->iSymbol))
		{
			found = true;
		}
		iSAC++;
	}

	return found;
}

List<ProductionRuleV3^>^ ModelV3::SortProductionRules(List<ProductionRuleV3^>^ Rules)
{
	List<ProductionRuleV3^>^ result = Rules;

	for (size_t iRule = 0; iRule < Rules->Count; iRule++)
	{
		ProductionRuleV3^ r = Rules[iRule];
		List<Int32>^ toRemove = gcnew List<Int32>;

		List<String^>^ contextLeft = gcnew List<String^>;
		List<String^>^ contextRight = gcnew List<String^>;
		List<Int32>^ odds = gcnew List<Int32>;
		List<String^>^ successor = gcnew List<String^>;
		List<String^>^ condition = gcnew List<String^>;

		bool contextFreeFound = false;
		String^ contextLeftFinal;
		String^ contextRightFinal;
		Int32 oddsFinal;
		String^ conditionFinal;
		String^ successorFinal;

		for (size_t jRule = 0; jRule < r->successor->Count; jRule++)
		{
			if (r->contextLeft[jRule] == "*" && r->contextRight[jRule] == "*")
			{
				contextFreeFound = true;
				contextLeftFinal = r->contextLeft[jRule];
				contextRightFinal = r->contextRight[jRule];
				oddsFinal = r->odds[jRule];
				conditionFinal = r->condition[jRule];
				successorFinal = r->successor[jRule];
				toRemove->Add(jRule);
			}
			else if (r->contextLeft[jRule] == "*" || r->contextRight[jRule] == "*")
			{
				contextLeft->Add(r->contextLeft[jRule]);
				contextRight->Add(r->contextRight[jRule]);
				odds->Add(r->odds[jRule]);
				condition->Add(r->condition[jRule]);
				successor->Add(r->successor[jRule]);
				toRemove->Add(jRule);
			}
		}

		toRemove->Sort();

		for (Int32 i = toRemove->Count - 1; i >= 0; --i)
		{
			r->contextLeft->RemoveAt(toRemove[i]);
			r->contextRight->RemoveAt(toRemove[i]);
			r->odds->RemoveAt(toRemove[i]);
			r->condition->RemoveAt(toRemove[i]);
			r->successor->RemoveAt(toRemove[i]);
		}

		for (size_t i = 0; i < contextLeft->Count; i++)
		{
			r->contextLeft->Add(contextLeft[i]);
			r->contextRight->Add(contextRight[i]);
			r->odds->Add(odds[i]);
			r->condition->Add(condition[i]);
			r->successor->Add(successor[i]);
		}

		if (contextFreeFound)
		{
			r->contextLeft->Add(contextLeftFinal);
			r->contextRight->Add(contextRightFinal);
			r->odds->Add(oddsFinal);
			r->condition->Add(conditionFinal);
			r->successor->Add(successorFinal);
		}
	}

	return result;
}
