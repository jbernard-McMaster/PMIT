#include "stdafx.h"
#include "Lexicon.h"

Lexicon::Lexicon(Int32 NumGenerations, Int32 MinRuleLength, Int32 MaxRuleLength, Vocabulary^ V, array<Int32>^ MinRuleLengths, array<Int32>^ MaxRuleLengths, array<Word^>^ RuleHead, array<Word^>^ RuleTail, array<Word^>^ RuleFragments, array<Int32>^ TotalPoG, array<Int32, 2>^ MinPoG, array<Int32, 2>^ MaxPoG)
{
	this->vocabulary = V;
	this->symbolSubwords = gcnew array<List<Word^>^>(V->numModules);
	this->subwordCounts = gcnew array<List<SubwordCountType^>^, 2>(NumGenerations, MaxRuleLength + 1);
	this->minRuleLengths = MinRuleLengths;
	this->maxRuleLengths = MaxRuleLengths;
	this->ruleHead = RuleHead;
	this->ruleTail = RuleTail;
	this->ruleFragments = RuleFragments;
	this->totalPoG = TotalPoG;
	this->minPoG = MinPoG;
	this->maxPoG = MaxPoG;

	for (size_t i = 0; i <= this->symbolSubwords->GetUpperBound(0); i++)
	{
		this->symbolSubwords[i] = gcnew List<Word^>;
	}

	for (size_t i = 0; i <= this->subwordCounts->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->subwordCounts->GetUpperBound(1); j++)
		{
			this->subwordCounts[i, j] = gcnew List<SubwordCountType^>;
		}
	}
}

Lexicon::Lexicon(Int32 NumGenerations, Vocabulary^ V, array<Int32>^ MinRuleLengths, array<Int32>^ MaxRuleLengths, array<Word^>^ RuleHead, array<Word^>^ RuleTail, array<Word^>^ RuleFragments, array<Int32>^ TotalPoG, array<Int32, 2>^ MinPoG, array<Int32, 2>^ MaxPoG)
{
	Int32 longest = 0;
	Int32 shortest = 99999999;

	for (size_t iRule = 0; iRule <= MaxRuleLengths->GetUpperBound(0); iRule++)
	{
		if (MinRuleLengths[iRule] < shortest)
		{
			shortest = MinRuleLengths[iRule];
		}

		if (MaxRuleLengths[iRule] > longest)
		{
			longest = MaxRuleLengths[iRule];
		}
	}

	this->vocabulary = V;
	this->symbolSubwords = gcnew array<List<Word^>^>(V->numModules);
	this->subwordCounts = gcnew array<List<SubwordCountType^>^,2>(NumGenerations, longest+1);
	this->minRuleLengths = MinRuleLengths;
	this->maxRuleLengths = MaxRuleLengths;
	this->ruleHead = RuleHead;
	this->ruleTail = RuleTail;
	this->ruleFragments = RuleFragments;
	this->totalPoG = TotalPoG;
	this->minPoG = MinPoG;
	this->maxPoG = MaxPoG;

	for (size_t i = 0; i <= this->symbolSubwords->GetUpperBound(0); i++)
	{
		this->symbolSubwords[i] = gcnew List<Word^>;
	}

	for (size_t i = 0; i <= this->subwordCounts->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->subwordCounts->GetUpperBound(1); j++)
		{
			this->subwordCounts[i, j] = gcnew List<SubwordCountType^>;
		}
	}
}

Lexicon::Lexicon(Vocabulary^ V, Int32 Length, array<Int32>^ TotalPoG)
{
	this->vocabulary = V;
	this->totalPoG = TotalPoG;
	this->subwordCounts = gcnew array<List<SubwordCountType^>^, 2>(1, Length + 1);

	for (size_t i = 0; i <= this->subwordCounts->GetUpperBound(0); i++)
	{
		for (size_t j = 0; j <= this->subwordCounts->GetUpperBound(1); j++)
		{
			this->subwordCounts[i, j] = gcnew List<SubwordCountType^>;
		}
	}
}

Lexicon::~Lexicon()
{
}

void Lexicon::Compress()
{
	delete this->subwordCounts;
}

void Lexicon::CompileLexicon(List<Word^>^ Evidence)
{
	this->evidence = Evidence;
	for (size_t iRule = 0; iRule <= this->symbolSubwords->GetUpperBound(0); iRule++)
	{
		this->CompileLexiconAction(iRule);
	}
}

void Lexicon::CompileLexiconAction(Int32 iRule)
{
	for (size_t iGen = 1; iGen <= this->subwordCounts->GetUpperBound(0); iGen++)
	{
		//Console::WriteLine(iRule + " " + iGen);
		for (size_t iLength = this->minRuleLengths[iRule]; iLength <= Math::Min(this->subwordCounts->GetUpperBound(1), this->maxRuleLengths[iRule]); iLength++)
		{
			for (size_t iWord = 0; iWord < this->subwordCounts[iGen, iLength]->Count; iWord++)
			{
				bool isCandidate = true;
				bool found = false;
				// #1 check to see if this is subword is the special case of a rule producing itself, this is not permitted
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
				if ((this->subwordCounts[iGen, iLength][iWord]->subword->symbols->Count > 1) || (this->subwordCounts[iGen, iLength][iWord]->subword->symbols[0] != this->vocabulary->Symbols[iRule]))
				{
#endif					// #2 check to see if it violates the max/min PoG
					bool isValid = true;
					Int32 iSymbol = 0;
					do
					{
						if ((this->subwordCounts[iGen, iLength][iWord]->subword->parikhVector[iSymbol] > this->maxPoG[iRule, iSymbol]) || (this->subwordCounts[iGen, iLength][iWord]->subword->parikhVector[iSymbol] < this->minPoG[iRule, iSymbol]))
						{
							isValid = false;
						}
						iSymbol++;
					} while ((isValid) && (iSymbol <= this->subwordCounts[iGen, iLength][iWord]->subword->parikhVector->GetUpperBound(0)));

					if (isValid)
					{
						// #3 check to see if the word matches the head and tail fragments
						if (this->subwordCounts[iGen, iLength][iWord]->subword->OverlapHeadTail(this->ruleHead[iRule], this->ruleTail[iRule]))
						{
							// #4 check to see if the subword already in the lexicon
							for (size_t i = 0; i < this->symbolSubwords[iRule]->Count; i++)
							{
								if (this->subwordCounts[iGen, iLength][iWord]->subword->IsMatch(this->symbolSubwords[iRule][i]))
								{
									found = true;
								}
							}

							if (!found)
							{
								// For this word check to see if count match for every generation
								for (size_t iGenInner = 1; iGenInner <= this->subwordCounts->GetUpperBound(0); iGenInner++)
								{
									for (size_t iWordInner = 0; iWordInner < this->subwordCounts[iGenInner, iLength]->Count; iWordInner++)
									{
										// Check to see if they are the same word
										if (this->subwordCounts[iGenInner, iLength][iWordInner]->subword->IsMatch(this->subwordCounts[iGen, iLength][iWord]->subword))
										{
											Int32 symbolCount = this->evidence[iGenInner - 1]->parikhVector[iRule];

											// Check to see if the symbols counts are the same (as this is quicker than seeing if the words match)
											if (symbolCount > this->subwordCounts[iGenInner, iLength][iWordInner]->count)
											{
												//Console::WriteLine("G: " + iGenInner + " A(" + this->vocabulary->Symbols[iRule]->ToString()  + "): " + symbolCount + " C:" + this->subwordCounts[iGenInner, iLength][iWordInner]->count + " W=" + this->subwordCounts[iGenInner, iLength][iWordInner]->subword->ToString() + " FALSE!");
												isCandidate = false;
											}
											//else
											//{
											//	Console::WriteLine("G: " + iGenInner + " A(" + this->vocabulary->Symbols[iRule]->ToString() + "): " + symbolCount + " C:" + this->subwordCounts[iGenInner, iLength][iWordInner]->count + " W=" + this->subwordCounts[iGenInner, iLength][iWordInner]->subword->ToString());
											//}
										}
									}
								}

								if (isCandidate)
								{
									//if (iRule == 3)
									//{
									//	Console::WriteLine("Added: " + this->subwordCounts[iGen, iLength][iWord]->subword->ToString() + " to rule " + iRule);
									//}
									this->symbolSubwords[iRule]->Add(this->subwordCounts[iGen, iLength][iWord]->subword);
								}
							}
						}
#if _PMI_PROBLEM_KNOWN_CONSTANTS_ > 0	
					}
#endif
				}
			}
		}
	}
}


void Lexicon::CompileLexiconT(List<Word^>^ Evidence)
{
	this->evidence = Evidence;
	System::Threading::Tasks::ParallelOptions^ options = gcnew System::Threading::Tasks::ParallelOptions();
	options->MaxDegreeOfParallelism = 6;

	System::Threading::Tasks::Parallel::For(0, this->symbolSubwords->GetUpperBound(0) + 1, options, gcnew Action<int>(this, &Lexicon::CompileLexiconAction));

	// Order the lexicon from shortest to longest word
	// Required so other sections of code will work
	for (size_t iRule = 0; iRule <= this->symbolSubwords->GetUpperBound(0); iRule++)
	{
		this->symbolSubwords[iRule]->Sort(gcnew LengthComparer());
	}
}

void Lexicon::Display()
{
	for (size_t iRule = 0; iRule <= this->symbolSubwords->GetUpperBound(0); iRule++)
	{
		for (size_t iWord = 0; iWord < this->symbolSubwords[iRule]->Count; iWord++)
		{
			Console::WriteLine(this->vocabulary->Symbols[iRule]->ToString() + " " + this->symbolSubwords[iRule][iWord]->ToString());
		}
	}
}

void Lexicon::DisplayCounts()
{
	for (size_t iGen = 0; iGen <= this->subwordCounts->GetUpperBound(0); iGen++)
	{
		for (size_t iLength = 0; iLength <= this->subwordCounts->GetUpperBound(1); iLength++)
		{
			for (size_t iWord = 0; iWord < this->subwordCounts[iGen, iLength]->Count; iWord++)
			{
				Console::WriteLine(this->subwordCounts[iGen, iLength][iWord]->subword->ToString() + " " + this->subwordCounts[iGen, iLength][iWord]->count);
			}
		}
	}
}

void Lexicon::Filter(Lexicon^ L)
{
	this->processToRemove = gcnew List<Symbol^>;
	this->processL = L;

	for (size_t iSymbol = 0; iSymbol < this->processL->vocabulary->Symbols->Count; iSymbol++)
	{
		Int32 idx = 0;

		bool found = true;

		do
		{
			found = (this->vocabulary->Symbols[idx] == this->processL->vocabulary->Symbols[iSymbol]);
			idx++;
		} while ((!found) && (idx < this->vocabulary->Symbols->Count));

		if (!found)
		{
			this->processToRemove->Add(this->processL->vocabulary->Symbols[iSymbol]);
		}
	}

	for (size_t iRule = 0; iRule <= this->processL->symbolSubwords->GetUpperBound(0); iRule++)
	{
		this->FilterProcess(iRule);
	}

	//this->Display();
}

void Lexicon::FilterMT(Lexicon^ L)
{
	this->processToRemove = gcnew List<Symbol^>;
	this->processL = L;

	for (size_t iSymbol = 0; iSymbol < this->processL->vocabulary->Symbols->Count; iSymbol++)
	{
		Int32 idx = 0;

		bool found = true;

		do
		{
			found = (this->vocabulary->Symbols[idx] == this->processL->vocabulary->Symbols[iSymbol]);
			idx++;
		} while ((!found) && (idx < this->vocabulary->Symbols->Count));

		if (!found)
		{
			this->processToRemove->Add(this->processL->vocabulary->Symbols[iSymbol]);
		}
	}

	System::Threading::Tasks::ParallelOptions^ options = gcnew System::Threading::Tasks::ParallelOptions();
	options->MaxDegreeOfParallelism = 8;

	System::Threading::Tasks::Parallel::For(0, L->symbolSubwords->GetUpperBound(0) + 1, options, gcnew Action<int>(this, &Lexicon::FilterProcess));
	//this->Display();
}


void Lexicon::FilterProcess(Int32 iRule)
{
	for (size_t iWord = 0; iWord < this->processL->symbolSubwords[iRule]->Count; iWord++)
	{
		Word^ w = gcnew Word(this->processL->symbolSubwords[iRule][iWord]);
		w->FilterOutSymbol(this->processToRemove);

		bool found = false;
		//for (size_t i = 0; i < this->symbolSubwords[iRule]->Count; i++)
		//{
		//	if (w->IsMatch(this->symbolSubwords[iRule][i]))
		//	{
		//		found = true;
		//	}
		//}

		if (!found)
		{
			this->vocabulary->CompressSymbols(w);
			this->symbolSubwords[iRule]->Add(w);
		}
	}

	this->symbolSubwords[iRule]->Sort(gcnew LengthComparer());
	//this->Display();
}

Int32 Lexicon::FindSubword(Word^ W)
{
	return -1;
}

// TODO: this assumes only looking in generation 0, i.e. it is intended for markers
Int32 Lexicon::FindSubwordCount(Word^ W)
{
	Int32 result = -1;
	
	// if the target word is too big than it cannot be in the lexicon
	if (W->Count <= this->subwordCounts->GetUpperBound(1))
	{
		Int32 iWord = 0;
		bool found = false;

		while ((!found) && (iWord < this->subwordCounts[0, W->Count]->Count))
		{
			if ((W->IsMatch(this->subwordCounts[0, W->Count][iWord]->subword)))
			{
				found = true;
				result = iWord;
			}
			iWord++;
		}
	}

	return result;
}

Int32 Lexicon::FoundSubword(Word^ W, Int32 iGen)
{
	Int32 result = 0;


	// Check #1 - Check to make sure the word doesn't value the total PoG values
	this->vocabulary->CompressSymbols(W);

	//Console::WriteLine(iGen + ": " + W->ToString());

	bool isValid = true;
	Int32 iSymbol = 0;
	do
	{
		if (W->parikhVector[iSymbol] > this->totalPoG[iSymbol])
		{
			isValid = false;
		}
		iSymbol++;
	} while ((isValid) && (iSymbol <= W->parikhVector->GetUpperBound(0)));

	if (isValid)
	{
		Int32 iWord = 0;
		bool found = false;
		List<SubwordCountType^>^ wordList = this->subwordCounts[iGen, W->symbols->Count];

		// check to see if the word is already in the list
		while ((iWord < wordList->Count) && (!found))
		{
			// check to see if this word is a match
			if (W->IsMatch(wordList[iWord]->subword))
			{
				// if so increase the count
				wordList[iWord]->count++;
				result = wordList[iWord]->count;
				found = true;
			}
			iWord++;
		}

		if (!found)
		{
			SubwordCountType^ x = gcnew SubwordCountType();
			x->count = 1;
			x->subword = gcnew Word(W);
			wordList->Add(x);
			result = 1;
		}
	}
	
	return result;
}

bool Lexicon::IsSubwordOverlap(Word^ W)
{
	bool result = false;

	// if the target word is too big than it cannot be in the lexicon
	if (W->Count <= this->subwordCounts->GetUpperBound(1))
	{
		// Check to see if current symbol is the 1st symbol in any marker
		Int32 iLength = W->Count;

		while ((!result) && (iLength <= this->subwordCounts->GetUpperBound(1)))
		{
			Int32 iWord = 0;
			while ((!result) && (iWord < this->subwordCounts[0, iLength]->Count))
			{
				Int32 iSymbol = 0;
				bool isMatch = true;

				while ((isMatch) && (iSymbol < W->Count))
				{
					if (W[iSymbol] != this->subwordCounts[0, iLength][iWord]->subword[iSymbol])
					{
						isMatch = false;
					}
					iSymbol++;
				}

				if (isMatch)
				{
					result = true;
				}

				iWord++;
			}
			iLength++;
		}
	}

	return result;
}

bool Lexicon::Load(String^ FileName)
{
	bool result = false;

	if (System::IO::File::Exists("./" + FileName))
	{
		// Load Configuration File
		System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./" + FileName);

		array<Char>^ seperator = gcnew array<Char>{','};

		while (!sr->EndOfStream)
		{
			String^ line = sr->ReadLine();
			array<String^, 1>^ words;
			words = line->Split(seperator);

			Symbol^ S = this->vocabulary->FindSymbol(words[0]);
			Int32 iRule = this->vocabulary->RuleIndex(S);
			Word^ W = this->vocabulary->CreateWord(words[1]);
			this->vocabulary->CompressSymbols(W);

			this->symbolSubwords[iRule]->Add(W);
		}

		sr->Close();
		result = true;
	}

	return result;
}

void Lexicon::Save(String^ FileName)
{
	// Load Configuration File
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("./" + FileName, false);

	for (size_t iRule = 0; iRule <= this->symbolSubwords->GetUpperBound(0); iRule++)
	{
		Symbol^ S = this->vocabulary->Symbols[iRule];
		for (size_t iWord = 0; iWord < this->symbolSubwords[iRule]->Count; iWord++)
		{
			sw->WriteLine(S->Value() + "," + this->symbolSubwords[iRule][iWord]->ToString());
		}
	}

	sw->Close();
}

void Lexicon::UpdateLexicon_Length(Int32 iRule, Int32 MinLength, Int32 MaxLength)
{
	Int32 lowerBound = 0;
	Int32 lowerCount = 0;
	bool stop = false;

	while ((lowerBound < this->symbolSubwords[iRule]->Count) && (!stop))
	{
		if (this->symbolSubwords[iRule][lowerBound]->symbols->Count < MinLength)
		{
			lowerCount++;
		}
		else
		{
			stop = true;
		}
		lowerBound++;
	}

	stop = false;
	Int32 upperBound = this->symbolSubwords[iRule]->Count-1;
	Int32 upperCount = 0;

	while ((upperBound > 0) && (!stop))
	{
		if (this->symbolSubwords[iRule][upperBound]->symbols->Count > MaxLength)
		{
			upperCount++;
		}
		else
		{
			stop = true;
		}
		upperBound--;
	}

	this->symbolSubwords[iRule]->RemoveRange(0, lowerCount);
	this->symbolSubwords[iRule]->RemoveRange(this->symbolSubwords[iRule]->Count - upperCount, upperCount);

	if (this->symbolSubwords[iRule]->Count == 0)
	{
		throw gcnew Exception("Lexicon has zero words for rule #" + iRule);
	}
}

bool Lexicon::VerifyFragment(Int32 iRule, Word^ W)
{
	bool result = false;
	Int32 iWord = 0;

	do
	{
		Int32 iSymbol = 0;
		bool wordMatch = true;

		if (W->symbols->Count != this->symbolSubwords[iRule][iWord]->symbols->Count)
		{
			wordMatch = false;
		}
		else
		{
			do
			{
				wordMatch = (W[iSymbol] == this->symbolSubwords[iRule][iWord]->symbols[iSymbol]);
				iSymbol++;
			} while ((wordMatch) && (iSymbol < this->symbolSubwords[iRule][iWord]->Count));
		}
		result = wordMatch;
		iWord++;
	} while ((!result) && (iWord < this->symbolSubwords[iRule]->Count));

	return result;
}

bool Lexicon::VerifyRules(array<Int32, 2>^ R, array<bool, 2>^ SymbolSubset, Int32 SymbolSubsetIndex, Int32 SymbolIndexEnd)
{
	bool result = true;
	Int32 iRule = 0;

	do
	{
		// Check only if the symbol is being solved for in the current problem
		if (SymbolSubset[SymbolSubsetIndex, iRule])
		{
			Int32 iWord = 0;
			bool wordRuleMatch = true;
			do
			{
				//if (SymbolSubsetIndex == 1 && SymbolIndexEnd == 16)
				//{
				//	Console::WriteLine(this->symbolSubwords[iRule][iWord]->ToString());
				//}
				Int32 iSymbol = 0;
				do
				{
					//if (SymbolSubsetIndex == 1 && SymbolIndexEnd == 16)
					//{
					//	Console::WriteLine("R = " + iRule + ", S=" + iSymbol + ", W=" + iWord + " : " + R[iRule, iSymbol] + " : " + this->symbolSubwords[iRule][iWord]->parikhVector[iSymbol]);
					//}
					wordRuleMatch = (R[iRule, iSymbol] == this->symbolSubwords[iRule][iWord]->parikhVector[iSymbol]);
					iSymbol++;
				} while ((iSymbol < SymbolIndexEnd) && (wordRuleMatch));

				iWord++;
			} while ((!wordRuleMatch) && (iWord < this->symbolSubwords[iRule]->Count));

			result = wordRuleMatch;
		}
		iRule++;
	} while ((result) && (iRule <= R->GetUpperBound(0)));

	return result;
}