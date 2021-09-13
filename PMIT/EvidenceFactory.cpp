#include "stdafx.h"
#include "EvidenceFactory.h"


EvidenceFactory::EvidenceFactory()
{
}


EvidenceFactory::~EvidenceFactory()
{
}

array<Int32, 2>^ EvidenceFactory::Process(array<Int32>^ A, array<Int32, 2>^ R, Int32 Generations, Int32 NumSymbols, Int32 NumModules)
{
	array<Int32, 2>^ result;
	Int32 idx = 0;

	idx = 1;
	result = gcnew array<Int32, 2>(Generations+1, NumSymbols);// +1 for the axiom
	for (size_t iSymbol = 0; iSymbol <= result->GetUpperBound(1); iSymbol++)
	{
		result[0, iSymbol] = A[iSymbol];
	}

	for (size_t iGen = 0; iGen < Generations; iGen++)
	{
		for (size_t iSymbol = 0; iSymbol <= result->GetUpperBound(1); iSymbol++)
		{
			Int32 count = 0;
			// Move the already produced constants up to the current generation
			if (iSymbol+1 > NumModules)
			{
				result[iGen + 1, iSymbol] = result[iGen, iSymbol];
			}

			// Produce the symbols for each rule
			for (size_t iRule = 0; iRule <= R->GetUpperBound(0); iRule++)
			{
				result[iGen+1, iSymbol] += result[iGen, iRule] * R[iRule, iSymbol];
			}
		}
	}

	return result;
}

List<Word^>^ EvidenceFactory::Process(Word^ A, List<ProductionRule^>^ R, Int32 Start, Int32 End, bool AxiomFirst)
{
	List<Word^>^ result = gcnew List<Word^>;

	if (AxiomFirst)
	{
		result->Add(gcnew Word(A->symbols));
	}

	for (size_t iGen = Start; iGen < End; iGen++)
	{
		List<Symbol^>^ x = gcnew List<Symbol^>;

		for each (Symbol^ s in A->symbols)
		{
			int index = 0;
			bool found = false;
			bool fired = false;

			do
			{
				found = R[index]->IsMatch(s);

				if (!found)
				{
					index++;
				}
				else
				{
					Symbol^ Left = nullptr;
					Symbol^ Right = nullptr;

					List<Symbol^>^ toAdd = R[index]->Produce(iGen, Left, Right);

					if (toAdd != nullptr)
					{
						x->AddRange(toAdd);
						fired = true;
					}
					else
					{
						index++; // try the next rule
					}
				}

			} while ((index < R->Count) && (!fired));

			// If no rule has fired then treat as a constant
			if (!fired)
			{
				x->Add(s);
			}

			// This is intended to keep the string of evidence from getting too long
			if (x->Count > ProductionRule::cMaxLength)
			{
				Console::WriteLine("Evidence Limit Exceeded");
				break;
			}
		}

		A->symbols = x;
		result->Add(gcnew Word(x));
	}

	return result;
}


List<Word^>^ EvidenceFactory::Process(Word^ A, List<ProductionRuleStochastic^>^ R, Int32 Start, Int32 End, bool AxiomFirst)
{
	List<Word^>^ result = gcnew List<Word^>;

	if (AxiomFirst)
	{
		result->Add(gcnew Word(A->symbols));
	}

	for (size_t iGen = Start; iGen < End; iGen++)
	{
		List<Symbol^>^ x = gcnew List<Symbol^>;

		for each (Symbol^ s in A->symbols)
		{
			int index = 0;
			bool found = false;
			bool fired = false;

			do
			{
				found = R[index]->IsMatch(s);

				if (!found)
				{
					index++;
				}
				else
				{
					Symbol^ Left = nullptr;
					Symbol^ Right = nullptr;

					List<Symbol^>^ toAdd = R[index]->Produce(iGen, Left, Right);

					if (toAdd != nullptr)
					{
						x->AddRange(toAdd);
						fired = true;
					}
					else
					{
						index++; // try the next rule
					}
				}

			} while ((index < R->Count) && (!fired));

			// If no rule has fired then treat as a constant
			if (!fired)
			{
				x->Add(s);
			}

			// This is intended to keep the string of evidence from getting too long
			if (x->Count > ProductionRule::cMaxLength)
			{
				break;
			}
		}

		A->symbols = x;
		result->Add(gcnew Word(x));
	}

	return result;
}

Word^ EvidenceFactory::Process(Word^ A, List<ProductionRule^>^ R)
{
	Word^ result = gcnew Word();

	List<Symbol^>^ x = gcnew List<Symbol^>;

	for each (Symbol^ s in A->symbols)
	{
		int index = 0;
		bool found = false;
		bool fired = false;

		do
		{
			found = R[index]->IsMatch(s);

			if (!found)
			{
				index++;
			}
			else
			{
				Symbol^ Left = nullptr;
				Symbol^ Right = nullptr;

				List<Symbol^>^ toAdd = R[index]->Produce(0, Left, Right);

				if (toAdd != nullptr)
				{
					x->AddRange(toAdd);
					fired = true;
				}
				else
				{
					index++; // try the next rule
				}
			}

		} while ((index < R->Count) && (!fired));

		// If no rule has fired then treat as a constant
		if (!fired)
		{
			x->Add(s);
		}

		// This is intended to keep the string of evidence from getting too long
		if (x->Count > ProductionRule::cMaxLength)
		{
			Console::WriteLine("Evidence Limit Exceeded");
			break;
		}
	}

	result->symbols = x;

	return result;
}

//List<Axiom^>^ EvidenceFactory::ProcessAbstract(Axiom^ A, List<ProductionRule^>^ R, Int32 Start, Int32 End, bool AxiomFirst)
//{
//	List<Axiom^>^ result = gcnew List<Axiom^>;
//
//	if (AxiomFirst)
//	{
//		result->Add(gcnew Axiom(A->symbols));
//	}
//
//	for (size_t iGen = Start; iGen < End; iGen++)
//	{
//		List<Symbol^>^ x = gcnew List<Symbol^>;
//
//		for each (Symbol^ s in A->symbols)
//		{
//			int index = 0;
//			bool found = false;
//			bool fired = false;
//
//			do
//			{
//				found = R[index]->IsMatch(s);
//
//				if (!found)
//				{
//					index++;
//				}
//				else
//				{
//					Symbol^ Left = nullptr;
//					Symbol^ Right = nullptr;
//
//					List<Symbol^>^ toAdd = R[index]->ProduceAbstract(iGen, Left, Right);
//
//					if (toAdd != nullptr)
//					{
//						x->AddRange(toAdd);
//						fired = true;
//					}
//					else
//					{
//						index++; // try the next rule
//					}
//				}
//
//			} while ((index < R->Count) && (!fired));
//
//			// If no rule has fired then treat as a constant
//			if (!fired)
//			{
//				x->Add(s);
//			}
//
//			// This is intended to keep the string of evidence from getting too long
//			if (x->Count > EvidenceFactory::cMaxLength)
//			{
//				break;
//			}
//		}
//
//		A->symbols = x;
//		result->Add(gcnew Axiom(x));
//	}
//
//	return result;
//}

