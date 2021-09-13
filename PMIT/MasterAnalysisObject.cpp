#include "stdafx.h"
#include "MasterAnalysisObject.h"


MasterAnalysisObject::MasterAnalysisObject(ModelV3^ M)
{
	this->nextID = 1;
	this->model = M;
	this->turtleFacts = gcnew array<Fact^>(this->model->alphabet->Size());
	this->NoFact = gcnew Fact();
	this->NoFact->condition = "!";
	this->NoFact->ID = MasterAnalysisObject::NoFactID;

	// +1 on left/right to allow for the 'nothing' context
	this->facts = gcnew array<List<Fact^>^, 5>(this->model->generations-1, 1, this->model->alphabet->SizeNonTurtle(), this->model->alphabet->SizeNonTurtle()+1, this->model->alphabet->SizeNonTurtle()+1);
	this->factsList = gcnew List<Fact^>;
	//this->factsMaster = gcnew array<List<Fact^>^, 5>(NumGenerations, 1, AlphabetSize, AlphabetSize, AlphabetSize);
	this->nothingID = this->model->alphabet->SizeNonTurtle(); // this way I don't have to constantly re-compute the ID for the nothing slot

	this->knownSymbols = gcnew array<bool>(this->model->alphabet->SizeNonTurtle());

	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		this->knownSymbols[iSymbol] = false;
	}

	// Establish turtle facts
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		if (this->model->alphabet->IsTurtle(this->model->alphabet->symbols[iSymbol]))
		{
			Fact^ turtleFact = gcnew Fact();
			turtleFact->condition = "*";
			turtleFact->length = gcnew MinMax();
			turtleFact->length->min = 1;
			turtleFact->length->max = 1;
			turtleFact->growth = gcnew array<Int32, 2>(this->model->alphabet->Size(),2);
			for (size_t i = 0; i < this->model->alphabet->Size(); i++)
			{
				if (i == iSymbol)
				{
					turtleFact->growth[i, 0] = 1;
					turtleFact->growth[i, 1] = 1;
				}
				else
				{
					turtleFact->growth[i, 0] = 0;
					turtleFact->growth[i, 1] = 0;
				}
			}
			Fragment^ turtleFragment = gcnew Fragment();
			turtleFragment->fragment = this->model->alphabet->symbols[iSymbol];
			turtleFact->complete = turtleFragment;
			this->turtleFacts[iSymbol] = turtleFact;
		}
	}

	// For each generation
	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		//For each table
		for (size_t iTable = 0; iTable < 1; iTable++)
		{
			// For each non-turtle symbol
			for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
			{
				// For each symbol as a left context
				for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle()+1; iLeft++)
				{
					// For each symbol as a right context
					for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle()+1; iRight++)
					{
						this->facts[iGen, iTable, iSymbol, iLeft, iRight] = gcnew List<Fact^>;
					}
				}
			}
		}
	}

	this->symbolsByGeneration = gcnew array<bool, 2>(this->model->generations, this->model->alphabet->SizeNonTurtle());
	this->ComputeSymbolsByGeneration(this->model->evidence);

	this->sacsByGeneration = gcnew array<List<SACX^>^>(this->model->generations);
	this->sacs = gcnew List<SACX^>;
	this->ComputeSACsByGeneration(this->model->evidence);

	this->totalLengthSuccessors = gcnew array<MinMax^>(this->model->generations-1);

	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		MinMax^ mm1 = gcnew MinMax();
		mm1->min = 0;
		mm1->max = 999;
		this->totalLengthSuccessors[iGen] = mm1;
	}
}

MasterAnalysisObject::~MasterAnalysisObject()
{
}

void MasterAnalysisObject::UploadMAO(MasterAnalysisObject^ MAO)
{

}

void MasterAnalysisObject::Display()
{
#if _PHASE3_MAO_DISPLAY_ >= 1
	Console::WriteLine();
	Console::WriteLine("Displaying MAO");
	Console::WriteLine("==============");
	Console::WriteLine();

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Console::WriteLine("Total Min/Max Length All Successors: " + this->totalLengthSuccessors[iGen]->min + " / " + this->totalLengthSuccessors[iGen]->max);
		Console::WriteLine();
		Int32 iTable = 0;
	}

	for (size_t iFact = 0; iFact < this->factsList->Count; iFact++)
	{
		this->DisplayFact(this->factsList[iFact]);
	}
//		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
//		{
//			for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle()+1; iLeft++)
//			{
//				for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle()+1; iRight++)
//				{
//					Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft-1, iRight-1, nullptr);
//
//					if (!this->IsNoFact(f))
//					{
//						Console::Write(iGen + ", " + this->SymbolCondition(iSymbol, iLeft-1, iRight-1) + " " + f->condition + " :");
//						this->DisplayFact(f);
//					}
//#if PHASE3_MAO_DISPLAY_NOFACTS_ >= 1
//					else
//					{
//						this->DisplayFact(f);
//					}
//#endif
//				}
//			}
//		}
	//	Console::WriteLine();
	//}
#endif
}

void MasterAnalysisObject::Write()
{
#if _PHASE3_MAO_DISPLAY_ >= 1
	System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("./MAO.txt", false);
	sw->WriteLine();
	sw->WriteLine("Displaying MAO");
	sw->WriteLine("==============");
	sw->WriteLine();

	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		sw->WriteLine("Total Min/Max Length All Successors: " + this->totalLengthSuccessors[iGen]->min + " / " + this->totalLengthSuccessors[iGen]->max);
		sw->WriteLine();
		Int32 iTable = 0;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle() + 1; iLeft++)
			{
				for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle() + 1; iRight++)
				{
					Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft - 1, iRight - 1, nullptr);

					if (!this->IsNoFact(f))
					{
						sw->Write(iGen + ", " + this->SymbolCondition(iSymbol, iLeft - 1, iRight - 1) + " " + f->condition + " :");
						this->WriteFact(f, sw);
					}
#if PHASE3_MAO_DISPLAY_NOFACTS_ >= 1
					else
					{
						this->DisplayFact(f);
					}
#endif
				}
			}
		}
		sw->WriteLine();
	}

	sw->Close();
#endif
}

void MasterAnalysisObject::DisplayFact(Fact^ F)
{
	Console::WriteLine("### " + this->SymbolCondition(F->sac->iSymbol, F->sac->iLeft, F->sac->iRight) + " ###");
	Console::Write(F->length->min.ToString(L"D3") + " / " + F->length->max.ToString(L"D3") + " : ");
	String^ growthMin = "< ";
	String^ growthMax = "< ";
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		growthMin += F->growth[iSymbol, 0].ToString(L"D3") + this->model->alphabet->symbols[iSymbol] + " ";
		growthMax += F->growth[iSymbol, 1].ToString(L"D3") + this->model->alphabet->symbols[iSymbol] + " ";
	}
	growthMin += ">";
	growthMax += ">";
	Console::WriteLine(growthMin + " / " + growthMax);
	if (F->btf->Count > 0)
	{
		for (size_t iFragment = 0; iFragment < F->btf->Count; iFragment++)
		{
			if (F->btf[iFragment]->fragment != nullptr)
			{
				Console::Write("BTF " + iFragment.ToString(L"D3") + " : " + F->btf[iFragment]->fragment + " & " + F->btf[iFragment]->leftSignature + " / " + F->btf[iFragment]->rightSignature);
				//if (F->btf[iFragment]->isComplete)
				//{
				//	Console::Write(" (complete)");
				//}
				Console::WriteLine();
			}
		}
	}

	if (F->complete != nullptr)
	{
		Console::WriteLine("Complete : " + F->complete->fragment);
	}
	else
	{
		if (F->head->min->fragment != nullptr)
		{
			Console::WriteLine("Head: " + F->head->min->fragment + " / " + F->head->max->fragment);
		}
		if (F->tail->min->fragment != nullptr)
		{
			Console::WriteLine("Tail: " + F->tail->min->fragment + " / " + F->tail->max->fragment);
		}
		if (F->mid->fragment != nullptr)
		{
			Console::WriteLine("Mid : " + F->mid->fragment);
		}

		if (F->max->Count > 0)
		{
			for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
			{
				if (F->max[iFragment]->fragment != nullptr)
				{
					Console::Write("Max " + iFragment.ToString(L"D3") + " : " + F->max[iFragment]->fragment + " (" + F->max[iFragment]->isTrue.ToString() + ")");
					//if (F->max[iFragment]->isComplete)
					//{
					//	Console::Write(" (complete)");
					//}
					Console::WriteLine();
				}
			}
		}
	}
}

void MasterAnalysisObject::WriteFact(Fact^ F, System::IO::StreamWriter^ SW)
{
	SW->Write(F->length->min.ToString(L"D3") + " / " + F->length->max.ToString(L"D3") + " : ");
	String^ growthMin = "< ";
	String^ growthMax = "< ";
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		growthMin += F->growth[iSymbol, 0].ToString(L"D3") + this->model->alphabet->symbols[iSymbol] + " ";
		growthMax += F->growth[iSymbol, 1].ToString(L"D3") + this->model->alphabet->symbols[iSymbol] + " ";
	}
	growthMin += ">";
	growthMax += ">";
	SW->WriteLine(growthMin + " / " + growthMax);
	if (F->complete->fragment != nullptr)
	{
		SW->WriteLine("Complete: " + F->complete->fragment);
	}
	else
	{
		if (F->head->min->fragment != nullptr)
		{
			SW->WriteLine("Head: " + F->head->min->fragment + " / " + F->head->max->fragment);
		}
		if (F->tail->min->fragment != nullptr)
		{
			SW->WriteLine("Tail: " + F->tail->min->fragment + " / " + F->tail->max->fragment);
		}
		if (F->mid->fragment != nullptr)
		{
			SW->WriteLine("Mid : " + F->mid->fragment);
		}

		if (F->max->Count > 0)
		{
			for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
			{
				if (F->max[iFragment]->fragment != nullptr)
				{
					SW->Write("Max " + iFragment + " : " + F->max[iFragment]->fragment);
					//if (F->max[iFragment]->isComplete)
					//{
					//	Console::Write(" (complete)");
					//}
					SW->WriteLine();
				}
			}
		}
	}
}

void MasterAnalysisObject::ComputeSolutionSpaceSize()
{
	List<Int32>^ used = gcnew List<Int32>;

	this->ComputeSolutionSpaceSize_Absolute();
	this->ComputeSolutionSpaceSize_Length();
	this->ComputeSolutionSpaceSize_PoG();
	this->ComputeSolutionSpaceSize_PoL();

	array<Int32>^ largestPV = gcnew array<Int32>(this->model->alphabet->Size());
	Int32 longest = 0;
	for (size_t iGen = 0; iGen < this->model->generations - 1; iGen++)
	{
		Int32 iTable = 0;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle() + 1; iLeft++)
			{
				for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle() + 1; iRight++)
				{
					Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft - 1, iRight - 1, nullptr);

					if (!this->IsNoFact(f))
					{
						// compute the solution space
						// first ensure this fact has already been included
						if (!used->Contains(f->ID))
						{
							// Used for C variant
							if (1 + (f->length->max - f->length->min) > longest)
							{
								longest = 1 + (f->length->max - f->length->min);
							}
							this->solutionSpaceSize_Length *= 1 + (f->length->max - f->length->min);

							for (size_t i = 0; i < this->model->alphabet->Size(); i++)
							{
								// Used to compute the C variant
								if (1 + (f->growth[i, 1] - f->growth[i, 0]) > largestPV[i])
								{
									largestPV[i] = 1 + (f->growth[i, 1] - f->growth[i, 0]);
								}
								this->solutionSpaceSize_Growth *= 1 + (f->growth[i, 1] - f->growth[i, 0]);
							}
							//solutionSpaceSize_Growth /= largest;

							used->Add(f->ID);
						}
					}
				}
			}
		}
	}

	this->solutionSpaceSize_Length /= longest;
	for (size_t i = 0; i < this->model->alphabet->Size(); i++)
	{
		this->solutionSpaceSize_Growth /= largestPV[i];
	}
}

void MasterAnalysisObject::ComputeSolutionSpaceSize_Absolute()
{
	for (size_t iGen = 0; iGen <= this->localization->GetUpperBound(0); iGen++)
	{
		for (size_t i = 0; i <= this->localization[iGen]->GetUpperBound(0); i++)
		{
			Int32 sum = 0;
			for (size_t j = 0; j <= this->localization[iGen]->GetUpperBound(1); j++)
			{
				if (this->localization[iGen][i, j])
				{
					sum++;
				}
			}
			this->solutionSpaceSize_Absolute *= sum;
		}
	}
}

void MasterAnalysisObject::ComputeSolutionSpaceSize_Length()
{
	this->solutionSpaceSize_Length = 1;

	// Need a number of genes equal to the number of rules in the sub-problem
	for (size_t iFact = 0; iFact < this->factsList->Count; iFact++)
	{
		this->solutionSpaceSize_Length *= 1 + (this->factsList[iFact]->length->max - this->factsList[iFact]->length->min);
	}
}

void MasterAnalysisObject::ComputeSolutionSpaceSize_PoG()
{
	array<Int32>^ largestPV = gcnew array<Int32>(this->model->alphabet->Size());
	for (size_t iFact = 0; iFact < this->factsList->Count; iFact++)
	{
		for (size_t i = 0; i < this->model->alphabet->Size(); i++)
		{
			// Used to compute the C variant
			if (1 + (this->factsList[iFact]->growth[i, 1] - this->factsList[iFact]->growth[i, 0]) > largestPV[i])
			{
				largestPV[i] = 1 + (this->factsList[iFact]->growth[i, 1] - this->factsList[iFact]->growth[i, 0]);
			}
			this->solutionSpaceSize_Growth *= 1 + (this->factsList[iFact]->growth[i, 1] - this->factsList[iFact]->growth[i, 0]);
		}
	}

	for (size_t i = 0; i < this->model->alphabet->Size(); i++)
	{
		this->solutionSpaceSize_Growth /= largestPV[i];
	}
}

void MasterAnalysisObject::ComputeSolutionSpaceSize_PoL()
{

	Int32 longest = 0;
	this->solutionSpaceSize_PoL = 1 + (this->totalLengthSuccessors[0]->max - this->totalLengthSuccessors[0]->min);

	for (size_t iFact = 0; iFact < this->factsList->Count; iFact++)
	{
		// Used for C variant
		if (1 + (this->factsList[iFact]->length->max - this->factsList[iFact]->length->min) > longest)
		{
			longest = 1 + (this->factsList[iFact]->length->max - this->factsList[iFact]->length->min);
		}
		this->solutionSpaceSize_PoL *= 1 + (this->factsList[iFact]->length->max - this->factsList[iFact]->length->min);
	}

	this->solutionSpaceSize_PoL /= longest;
}

void MasterAnalysisObject::DisplaySolutionSpaces()
{
	Console::WriteLine("Solution Space Sizes");
	Console::WriteLine("====================");
	Console::WriteLine("Absolute = " + solutionSpaceSize_Absolute);
	Console::WriteLine("Length   = " + solutionSpaceSize_Length);
	Console::WriteLine("PoL      = " + solutionSpaceSize_PoL);
	Console::WriteLine("PoG      = " + solutionSpaceSize_Growth);
	//Console::WriteLine("Direct   = " + solutionSpaceSize_Direct);
	Console::WriteLine();
}

void MasterAnalysisObject::AddFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
{
	if (!this->model->alphabet->IsTurtle(iSymbol))
	{
		if (iLeft == -1)
		{
			iLeft = this->nothingID;
		}

		if (iRight == -1)
		{
			iRight = this->nothingID;
		}

		// Check to see if the fact already exists
		bool found = false;
		bool foundInEarlierGeneration = false;
		Int32 iFact = 0;
		Int32 iGen2 = iGen;
		Fact^ toAdd = F;

		while ((!found) && (iGen2 >= 0))
		{
			while ((!found) && (iFact < this->facts[iGen2, iTable, iSymbol, iLeft, iRight]->Count))
			{
				if (F->condition == this->facts[iGen2, iTable, iSymbol, iLeft, iRight][iFact]->condition)
				{
					if (iGen == iGen2)
					{
						found = true;
					}
					else
					{
						found = true;
						foundInEarlierGeneration = true;
						toAdd = this->facts[iGen2, iTable, iSymbol, iLeft, iRight][iFact];
					}
				}
			}
			iGen2--;
		}

		if (!found)
		{
			F->ID = this->nextID;
			this->nextID++;
			this->facts[iGen, iTable, iSymbol, iLeft, iRight]->Add(F);
			this->factsList->Add(F);
		}
		else if (foundInEarlierGeneration)
		{
			this->facts[iGen, iTable, iSymbol, iLeft, iRight]->Add(toAdd);
		}
	}
}

void MasterAnalysisObject::CopyFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 fromLeft, Int32 fromRight, Int32 toLeft, Int32 toRight, Fact^ F)
{
	if (!this->model->alphabet->IsTurtle(iSymbol))
	{
		if (fromLeft == -1)
		{
			fromLeft = this->nothingID;
		}

		if (fromRight == -1)
		{
			fromRight = this->nothingID;
		}

		if (toLeft == -1)
		{
			toLeft = this->nothingID;
		}

		if (toRight == -1)
		{
			toRight = this->nothingID;
		}

		// Check to see if the fact already exists
		bool found = false;
		bool foundInEarlierGeneration = false;
		Int32 iFact = 0;
		Int32 iGen2 = iGen;
		Fact^ toAdd = F;

		while ((!found) && (iGen2 >= 0))
		{
			while ((!found) && (iFact < this->facts[iGen2, iTable, iSymbol, fromLeft, fromRight]->Count))
			{
				if (F->condition == this->facts[iGen2, iTable, iSymbol, fromLeft, fromRight][iFact]->condition)
				{
					if (iGen == iGen2)
					{
						if (this->model->contextFree)
						{
							foundInEarlierGeneration = true;
							toAdd = this->facts[iGen2, iTable, iSymbol, fromLeft, fromRight][iFact];
						}
						found = true;
					}
					else
					{
						found = true;
						foundInEarlierGeneration = true;
						toAdd = this->facts[iGen2, iTable, iSymbol, fromLeft, fromRight][iFact];
					}
				}
			}
			iGen2--;
		}

		if (!found)
		{
			F->ID = this->nextID;
			this->nextID++;
			this->facts[iGen, iTable, iSymbol, fromLeft, fromRight]->Add(F);
			this->facts[iGen, iTable, iSymbol, toLeft, toRight]->Add(F);
			this->factsList->Add(F);
		}
		else if (foundInEarlierGeneration)
		{
			this->facts[iGen, iTable, iSymbol, toLeft, toRight]->Add(toAdd);
		}
		else if (found && !this->facts[iGen, iTable, iSymbol, toLeft, toRight]->Contains(F))
		{
			this->facts[iGen, iTable, iSymbol, toLeft, toRight]->Add(toAdd);
		}

	}
}

void MasterAnalysisObject::ComputeSACsByGeneration(array<WordXV3^>^ E)
{
	for (size_t iGen = 0; iGen < E->GetUpperBound(0); iGen++)
	{	
		this->sacsByGeneration[iGen] = gcnew List<SACX^>;

		for (size_t iPos = 0; iPos < E[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = E[iGen]->GetSymbolIndex(iPos);

			if (!this->model->alphabet->IsTurtle(symbolIndex))
			{
				Int32 iLeft = E[iGen]->GetLeftContext(iPos);
				Int32 iRight = E[iGen]->GetRightContext(iPos);

				//TODO: extract the parameter values
				SACX^ s = gcnew SACX();
				s->iGen = iGen;
				s->iTable = 0;
				s->iSymbol = symbolIndex;
				s->iLeft = iLeft;
				s->iRight = iRight;

				bool found = false;
				Int32 iSac = 0;
				while ((!found) && (iSac < this->sacs->Count))
				{
					if ((this->sacs[iSac]->iGen == s->iGen) && (this->sacs[iSac]->iSymbol == s->iSymbol) && (this->sacs[iSac]->iLeft == s->iLeft) && (this->sacs[iSac]->iRight == s->iRight))
					{
						found = true;
					}
					iSac++;
				}

				if (!found)
				{
#if _PHASE3_VERBOSE_ >= 1
					Console::WriteLine("Adding SAC " + this->SymbolCondition(s->iSymbol, s->iLeft, s->iRight) + " to generation " + s->iGen);
#endif
					this->sacsByGeneration[iGen]->Add(s);
					this->sacs->Add(s);
				}
			}
		}
	}
}

void MasterAnalysisObject::ComputeSymbolsByGeneration(array<WordXV3^>^ E)
{
	for (size_t iGen = 0; iGen < E->GetUpperBound(0); iGen++)
	{
		for (size_t iPos = 0; iPos < E[iGen]->Length(); iPos++)
		{
			Int32 symbolIndex = E[iGen]->GetSymbolIndex(iPos);
			if (!this->model->alphabet->IsTurtle(symbolIndex))
			{
				this->symbolsByGeneration[iGen, symbolIndex] = true;
			}
		}
	}
}

List<SACX^>^ MasterAnalysisObject::GetSACs(Int32 iGen)
{
	return this->sacsByGeneration[iGen];
}

List<SACX^>^ MasterAnalysisObject::GetAppearingSACs(Int32 From, Int32 To)
{
	List<SACX^>^ result = gcnew List<SACX^>;
	
	if (From >= 0)
	{// if the from generation exists (i.e. is zero or higher)
		for (size_t iSAC1 = 0; iSAC1 < this->sacsByGeneration[To]->Count; iSAC1++)
		{
			bool found = false;
			Int32 iSAC2 = 0;
			SACX^ toSAC = this->sacsByGeneration[To][iSAC1];

			while ((!found) && (iSAC2 < this->sacsByGeneration[From]->Count))
			{
				SACX^ fromSAC = this->sacsByGeneration[From][iSAC2];

				if ((toSAC->iSymbol == fromSAC->iSymbol) && (toSAC->iLeft == fromSAC->iLeft) && (toSAC->iRight == fromSAC->iRight))
				{
					found = true;
				}

				iSAC2++;
			}

			if (!found)
			{
				result->Add(toSAC);
			}
		}
	}
	else
	{// return all the SACs in the first generation
		for (size_t iSAC = 0; iSAC < this->sacsByGeneration[0]->Count; iSAC++)
		{
			result->Add(this->sacsByGeneration[0][iSAC]);
		}
	}

	return result;
}

List<SACX^>^ MasterAnalysisObject::GetDisappearingSACs(Int32 From, Int32 To)
{
	List<SACX^>^ result = gcnew List<SACX^>;

	for (size_t iSAC1 = 0; iSAC1 < this->sacsByGeneration[From]->Count; iSAC1++)
	{
		bool found = false;
		Int32 iSAC2 = 0;
		SACX^ fromSAC = this->sacsByGeneration[From][iSAC1];

		while ((!found) && (iSAC2 < this->sacsByGeneration[To]->Count))
		{
			SACX^ toSAC = this->sacsByGeneration[To][iSAC2];

			if ((toSAC->iSymbol == fromSAC->iSymbol) && (toSAC->iLeft == fromSAC->iLeft) && (toSAC->iRight == fromSAC->iRight))
			{
				found = true;
			}

			iSAC2++;
		}

		if (!found)
		{
			result->Add(fromSAC);
		}
	}

	return result;
}

//void MasterAnalysisObject::AddFactMaster(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
//{
//	this->factsMaster[iGen, iTable, iSymbol, iLeft, iRight]->Add(F);
//}

void MasterAnalysisObject::SetFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
{
	if (!this->model->alphabet->IsTurtle(iSymbol))
	{
		if (iLeft == -1)
		{
			iLeft = this->nothingID;
		}

		if (iRight == -1)
		{
			iRight = this->nothingID;
		}

		// TODO: this needs to figure out which fact should be overriten
		this->facts[iGen, iTable, iSymbol, iLeft, iRight][0] = F;
	}
}

//void MasterAnalysisObject::SetFactMaster(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F)
//{
//	// TODO: this needs to figure out which fact should be overriten
//	this->factsMaster[iGen, iTable, iSymbol, iLeft, iRight][0] = F;
//}

array<Int32>^ MasterAnalysisObject::ConvertSymbolInWordToFactTable(Int32 iGen, WordXV3^ W, Int32 iPos)
{
	array<Int32>^ result = gcnew array<Int32>(5);
	Int32 iTable = 0;
	Int32 iSymbol = W->GetSymbolIndex(iPos);
	Int32 iLeft = W->GetLeftContext(iPos);
	Int32 iRight = W->GetRightContext(iPos);

	result[0] = iGen;
	result[1] = iTable;
	result[2] = iSymbol;
	result[3] = iLeft;
	result[4] = iRight;

	return result;
}

Int32 MasterAnalysisObject::ComputeError(String^ S1, String^ S2)
{
	Int32 result = Math::Abs(S1->Length - S2->Length);
	Int32 length = Math::Min(S1->Length, S2->Length);

	for (size_t iPos = 0; iPos < length; iPos++)
	{
		if (S1->Substring(iPos, 1) != S2->Substring(iPos, 1))
		{
			result++;
		}
	}

	return result;
}

SACX^ MasterAnalysisObject::GetSAC(Fact^ F)
{
	for (size_t iGen = 0; iGen < this->model->generations-1; iGen++)
	{
		Int32 iTable = 0;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle() + 1; iLeft++)
			{
				for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle() + 1; iRight++)
				{
					Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);

					if ((!this->IsNoFact(f)) && (F == f))
					{
						SACX^ s = gcnew SACX;
						s->iGen = iGen;
						s->iTable = iTable;
						s->iSymbol = iSymbol;
						s->iLeft = iLeft;
						s->iRight = iRight;
						return s;
					}
				}
			}
		}
	}
}

Fact^ MasterAnalysisObject::GetFact(Int32 iGen, WordXV3^ W, Int32 iPos)
{
	Int32 iTable = 0;
	Int32 iSymbol = W->GetSymbolIndex(iPos);
	Int32 iLeft = W->GetLeftContext(iPos);
	Int32 iRight = W->GetRightContext(iPos);

	return this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, nullptr);
}

Fact^ MasterAnalysisObject::GetFact(SACX^ S)
{
	return this->GetFact(S->iGen, S->iTable, S->iSymbol, S->iLeft, S->iRight, nullptr);
}

Fact^ MasterAnalysisObject::GetFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V)
{
	if (!this->model->alphabet->IsTurtle(iSymbol))
	{
		if (iLeft == -1)
		{
			iLeft = this->nothingID;
		}

		if (iRight == -1)
		{
			iRight = this->nothingID;
		}

		List<Fact^>^ candidates = this->facts[iGen, iTable, iSymbol, iLeft, iRight];
		return this->FindFact(candidates, V);
	}
	else
	{
		return this->turtleFacts[iSymbol];
	}
}

//Fact^ MasterAnalysisObject::GetFactMaster(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V)
//{
//	if (!this->model->alphabet->IsTurtle(iSymbol))
//	{
//		List<Fact^>^ candidates = this->factsMaster[iGen, iTable, iSymbol, iLeft, iRight];
//		return this->FindFact(candidates, V);
//	}
//	else
//	{
//		return this->turtleFacts[iSymbol];
//	}
//}

Fact^ MasterAnalysisObject::FindFact(List<Fact^>^ F, List<float>^ V)
{
	if (F->Count > 0)
	{
		return F[0];
	}
	else
	{
		return this->NoFact;
	}
}

bool MasterAnalysisObject::IsSignatureCompatible(String^ L1, String^ R1, String^ L2, String^ R2)
{
	bool result = true;
	String^ shorterLeft;
	String^ longerLeft;
	String^ shorterRight;
	String^ longerRight;

	if (L1->Length <= L2->Length)
	{ 
		shorterLeft = L1;
		longerLeft = L2;
	}
	else
	{
		shorterLeft = L2;
		longerLeft = L1;
	}

	if (R1->Length <= R2->Length)
	{
		shorterRight = R1;
		longerRight = R2;
	}
	else
	{
		shorterRight = R2;
		longerRight = R1;
	}

	String^ overlapLeft = longerLeft->Substring(longerLeft->Length - shorterLeft->Length, shorterLeft->Length);
	String^ overlapRight = longerRight->Substring(0, shorterRight->Length);

	result = result && (overlapLeft == shorterLeft) && (overlapRight == shorterRight);

	return result;
}

ValidatedFragment^ MasterAnalysisObject::CreateVFR(Fragment^ Fr)
{
	ValidatedFragment^ result = gcnew ValidatedFragment();
	result->fr = gcnew Fragment();
	result->fr = Fr;
	result->isValid = true;

	return result;
}

ValidatedFragment^ MasterAnalysisObject::CreateVFR(String^ Frag)
{
	Fragment^ fr = gcnew Fragment();
	fr->fragment = Frag;
	fr->isComplete = false;
	fr->isTrue = false;
	ValidatedFragment^ result = this->CreateVFR(fr);

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::CreateVFRX(Fragment^ Fr)
{
	ValidatedFragmentX^ result = gcnew ValidatedFragmentX();
	result->vfr = this->CreateVFR(Fr);
	result->start = 0;
	result->end = Fr->fragment->Length;

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::CreateVFRX(String^ Frag)
{
	Fragment^ fr = gcnew Fragment();
	fr->fragment = Frag;
	fr->isComplete = false;
	fr->isTrue = false;
	ValidatedFragmentX^ result = this->CreateVFRX(fr);
		
	return result;
}

/* BRANCHING */

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_BranchingMinFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	ValidatedFragmentX^ result = gcnew ValidatedFragmentX();
	result->vfr->fr = Fr;
	result->start = 0;
	result->end = Fr->fragment->Length;

	result = this->ReviseFragment_BranchingMinFragmentsOnly(result, F);

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_BranchingMinFragmentsOnly(ValidatedFragmentX^ VFR, Fact^ F)
{
	ValidatedFragmentX^ result = VFR;

	Int32 branchCount = 0;
	Int32 branchStartPos = 0;

	// find all [ and ] symbols, keeping a running count, [ = ++ and ] = --. I.e. if balanced should be 0 at the end
	// also, track where a branch starts, if it doesn't end then this will be the start of the problem
	for (size_t iPos = 0; iPos < result->vfr->fr->fragment->Length; iPos++)
	{
		String^ s = result->vfr->fr->fragment->Substring(iPos, 1);

		if (s == this->model->alphabet->StartBranch)
		{
			if (branchCount == 0)
			{
				branchStartPos = iPos;
			}
			branchCount++;
		}
		else if (s == this->model->alphabet->EndBranch)
		{
			if (branchCount == 0)
			{
				branchStartPos = iPos;
			}
			branchCount--;
		}
	}

	if (branchCount != 0)
	{
		result->start = 0;
		result->end = branchStartPos - 1;
		result->vfr->fr->fragment = result->vfr->fr->fragment->Substring(result->start, result->end);
	}

	return result;
}

List<ValidatedFragment^>^ MasterAnalysisObject::ReviseFragment_BranchingMaxFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment();
	result->fr = Fr;
	result->isValid = true;

	return this->ReviseFragment_BranchingMaxFragmentsOnly(result, F);
}

// This version of the function must split the fragments rather than trimming them
// An unbalanced branching symbol and the beginning or end can be trimmed
// Example #1:  ABC[DEF => ABC or [DEF => ABC or DEF
// Example #2:  ABC[DEF]ABC]XYX => ABC[DEF]ABC or ]XYX => ABC[DEF]ABC or XYX

// Algorithm:
// Keep a running count of the number of branching symbols
// If count becomes negative, trim off everything so far as a fragment candidate, pass the rest into the function recursively
// Every time a [ is hit, push a cut point to stack
// Every time a ] is hit, pop a cut point from stack
List<ValidatedFragment^>^ MasterAnalysisObject::ReviseFragment_BranchingMaxFragmentsOnly(ValidatedFragment^ VFR, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;
	Stack<Int32>^ cutpoint = gcnew Stack<Int32>;
	cutpoint->Push(-1);

	Int32 branchCount = 0;
	Int32 branchStartPos = 0;

	// find all [ and ] symbols, keeping a running count, [ = ++ and ] = --. I.e. if balanced should be 0 at the end
	// also, track where a branch starts, if it doesn't end then this will be the start of the problem
	Int32 iPos = 0;
	bool stop = false;
	while ((!stop) && (iPos < VFR->fr->fragment->Length))
	{ 
		String^ s = VFR->fr->fragment->Substring(iPos, 1);

		if (s == this->model->alphabet->StartBranch)
		{
			branchCount++;
			cutpoint->Push(iPos);
		}
		else if (s == this->model->alphabet->EndBranch)
		{
			branchCount--;
			if (branchCount < 0)
			{
				// recursive call
				ValidatedFragment^ x = gcnew ValidatedFragment();
				x->fr = gcnew Fragment();
				x->fr->fragment = VFR->fr->fragment->Substring(0, iPos);
				x->fr->isComplete = false;
				x->isValid = true;
				result->Add(x);

				ValidatedFragment^ y = gcnew ValidatedFragment();
				y->fr = gcnew Fragment();
				y->fr->fragment = VFR->fr->fragment->Substring(iPos+1, VFR->fr->fragment->Length-(iPos+1));
				y->fr->isComplete = false;
				y->isValid = true;

				// make the recursive call and then add whatever comes back
				List<ValidatedFragment^>^ z = this->ReviseFragment_BranchingMaxFragmentsOnly(y, F);
				for (size_t i = 0; i < z->Count; i++)
				{
					if (z[i]->fr->fragment != "")
					{
						result->Add(z[i]);
					}
				}
				stop = true; // stop processing, everything further on has been completed
			}
			else
			{
				cutpoint->Pop();
			}
		}
		iPos++;
	}

	if (!stop)
	{
		Int32 end = VFR->fr->fragment->Length;
		Int32 count = cutpoint->Count;
		for (size_t iCut = 0; iCut < count; iCut++)
		{
			Int32 start = cutpoint->Pop() + 1;
			ValidatedFragment^ tmp = gcnew ValidatedFragment();
			tmp->fr = gcnew Fragment();
			tmp->fr->fragment = VFR->fr->fragment->Substring(start, end - start);
			tmp->isValid = true;
			if (tmp->fr->fragment != "")
			{
				result->Add(tmp);
			}
			end = start - 1;
		}
	}

	return result;
}

/* TURTLES */

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_Turtles(Fragment^ Fr, Fact^ F)
{
	ValidatedFragmentX^ result = this->CreateVFRX(Fr);
	result = this->ReviseFragment_Turtles(result, F);
	return result;
}

// This procedure assumes that the branching symbols are already balanced
ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_Turtles(ValidatedFragmentX^ VFR, Fact^ F)
{
	ValidatedFragmentX^ result = VFR;

	bool valid = true;
	bool stop = false;
	Int32 iPos = 0;
	List<String^>^ turtles = gcnew List<String^>;
	Int32 branchLevel = 0;

	do
	{
		String^ s = result->vfr->fr->fragment->Substring(iPos, 1);
		Int32 symbolIndex = this->model->alphabet->FindIndex(s);

		if (s == this->model->alphabet->StartBranch)
		{
			branchLevel++;
			iPos++;
		}
		else if (s == this->model->alphabet->EndBranch)
		{
			branchLevel--;
			iPos++;
			if (branchLevel == 0)
			{
				turtles->Clear();
			}
		}
		else if ((!this->model->alphabet->IsTurtle(symbolIndex) || s == this->model->alphabet->LineDown || s == this->model->alphabet->LineUp) && (branchLevel == 0))
		{
			turtles->Clear();
			iPos++;
		}
		else if (this->model->alphabet->IsTurtle(symbolIndex) && (branchLevel == 0))
		{
			if (!turtles->Contains(s))
			{
				turtles->Add(s);
				valid = this->model->alphabet->IsInvalidTurtleSet(turtles);
			}
			iPos++;
		}
		else
		{
			iPos++;
		}
	} while ((!stop) && (valid) && (iPos < result->vfr->fr->fragment->Length));

	if (!valid)
	{
		Console::WriteLine("Invalid turtle sequence " + result->vfr->fr->fragment->Substring(0, iPos));
		result->vfr->fr->fragment = result->vfr->fr->fragment->Substring(0, iPos);
	}

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_TurtlesHeadTailFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	ValidatedFragmentX^ result = this->CreateVFRX(Fr);
	result = this->ReviseFragment_TurtlesHeadTailFragmentsOnly(result, F);
	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_TurtlesHeadTailFragmentsOnly(ValidatedFragmentX^ VFR, Fact^ F)
{
	ValidatedFragmentX^ result = VFR;

	result = this->ReviseFragment_Turtles(VFR, F);

	//// Find invalid turtle sequences in the middle of the fragment
	//// In this case, split the sequence and keep the head (even for tail fragments, as they come in reversed)
	//List<ValidatedFragment^>^ tmp = this->ReviseFragment_TurtlesMaxFragmentsOnly(result->vfr, F);
	//result->vfr = tmp[0];

	return result;
}

List<ValidatedFragment^>^ MasterAnalysisObject::ReviseFragment_TurtlesMaxFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;
	result = this->ReviseFragment_TurtlesMaxFragmentsOnly(this->CreateVFR(Fr), F);
	return result;
}

List<ValidatedFragment^>^ MasterAnalysisObject::ReviseFragment_TurtlesMaxFragmentsOnly(ValidatedFragment^ VFR, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;

	Int32 iPos = 0;
	List<String^>^ turtles = gcnew List<String^>;
	List<Int32>^ cutpoint = gcnew List<Int32>;

	while (iPos < VFR->fr->fragment->Length)
	{
		String^ s = VFR->fr->fragment->Substring(iPos, 1);
		Int32 symbolIndex = this->model->alphabet->FindIndex(s);
		if (this->model->alphabet->IsTurtle(symbolIndex))
		{
			if (!turtles->Contains(s))
			{
				turtles->Add(s);
				if (!this->model->alphabet->IsInvalidTurtleSet(turtles))
				{
					cutpoint->Add(iPos);
					turtles->Clear();
					turtles->Add(s);
				}
			}
		}
		else
		{// if a non-turtle is reached then clear the turtles
			turtles->Clear();
		}
		iPos++;
	}

	if (cutpoint->Count > 0)
	{
		Int32 start = 0;
		for (size_t iCut = 0; iCut < cutpoint->Count; iCut++)
		{
			Int32 end = cutpoint[iCut];
			String^ tmp = VFR->fr->fragment->Substring(start, end-start); // no + 1 because end is an indexed position
			start = end; // no +1 because this is an indexed position indicating where to cut, e.g. ++-F-Y.... the first cut will be a iPos = 3 so that's the start of the next subword
			ValidatedFragment^ x = gcnew ValidatedFragment();
			x->fr = gcnew Fragment();
			x->fr->fragment = tmp;
			x->fr->isComplete = false;
			x->isValid = true;
			result->Add(x);
		}

		// need to add the point from the last cutpoint to the end of the string
		String^ tmp = VFR->fr->fragment->Substring(start, VFR->fr->fragment->Length-start);
		ValidatedFragment^ x = gcnew ValidatedFragment();
		x->fr = gcnew Fragment();
		x->fr->fragment = tmp;
		x->fr->isComplete = false;
		x->isValid = true;
		result->Add(x);
	}
	else
	{
		result->Add(VFR);
	}

	return result;
}

/* HEAD FRAGMENT */

ValidatedFragment^ MasterAnalysisObject::ReviseFragment_HeadFragmentMaxFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment;
	//result->vfr->fr = Fr;
	//result->start = 0;
	//result->end = Fr->fragment->Length;

	//result = this->ReviseFragment_Turtles(result, F);

	return result;
}

ValidatedFragment^ MasterAnalysisObject::ReviseFragment_HeadFragmentMaxFragmentsOnly(ValidatedFragment^ VFR, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment();
	result->fr = gcnew Fragment();
	result->fr->fragment = VFR->fr->fragment;
	result->isValid = true;

	String^ longest = "";

	if (F->head->max->fragment != nullptr)
	{
		if (result->fr->fragment->Length >= F->head->max->fragment->Length)
		{
			longest = this->FindLongestSubword(result->fr->fragment, F->head->max->fragment);
		}
		else if (result->fr->fragment->Length < F->head->max->fragment->Length)
		{
			longest = this->FindLongestSubword(F->head->max->fragment, result->fr->fragment);
		}
		result->fr->fragment = longest;
	}

	return result;
}

/* TAIL FRAGMENT */

ValidatedFragment^ MasterAnalysisObject::ReviseFragment_TailFragmentMaxFragmentsOnly(Fragment^ Fr, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment;
	//result->vfr->fr = Fr;
	//result->start = 0;
	//result->end = Fr->fragment->Length;

	//result = this->ReviseFragment_Turtles(result, F);

	return result;
}

ValidatedFragment^ MasterAnalysisObject::ReviseFragment_TailFragmentMaxFragmentsOnly(ValidatedFragment^ VFR, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment();
	result->fr = gcnew Fragment();
	result->fr->fragment = VFR->fr->fragment;
	result->isValid = true;

	String^ longest = "";

	if (F->tail->max->fragment != nullptr)
	{
		if (result->fr->fragment->Length >= F->tail->max->fragment->Length)
		{
			longest = this->FindLongestSubword(result->fr->fragment, F->tail->max->fragment);
		}
		else if (result->fr->fragment->Length < F->tail->max->fragment->Length)
		{
			longest = this->FindLongestSubword(F->tail->max->fragment, result->fr->fragment);
		}
		result->fr->fragment = longest;
	}

	return result;
}

/* MAX LENGTH */

bool MasterAnalysisObject::ValidateFragment_MaxLength(Fragment^ Fr, Fact^ F)
{
	if (Fr->fragment->Length > F->length->max)
	{
		return false;
	}
	else
	{
		return true;
	}
}



ValidatedFragment^ MasterAnalysisObject::ValidateFragment_MaxLength(ValidatedFragment^ VFR, Fact^ F)
{
	VFR->isValid = VFR->isValid && this->ValidateFragment_MaxLength(VFR->fr, F);
	return VFR;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_MaxLength(Fragment^ Fr, Fact^ F)
{
	ValidatedFragmentX^ result = gcnew ValidatedFragmentX();
	result->vfr->fr = Fr;
	result->start = 0;
	result->end = Fr->fragment->Length;

	result = this->ReviseFragment_MaxLength(result, F);

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_MaxLength(ValidatedFragmentX^ VFRX, Fact^ F)
{
	VFRX->vfr = this->ValidateFragment_MaxLength(VFRX->vfr, F);

	if (!VFRX->vfr->isValid)
	{
		VFRX->start = 0;
		VFRX->end = F->length->max;
		VFRX->vfr->fr->fragment = VFRX->vfr->fr->fragment->Substring(VFRX->start, VFRX->end);
		VFRX->vfr->isValid = true;
	}

	return VFRX;
}

/* MIN LENGTH */

bool MasterAnalysisObject::ValidateFragment_MinLength(Fragment^ Fr, Fact^ F)
{
	if (Fr->fragment->Length < F->length->min)
	{
		return false;
	}
	else
	{
		return true;
	}
}



ValidatedFragment^ MasterAnalysisObject::ValidateFragment_MinLength(ValidatedFragment^ VFR, Fact^ F)
{
	VFR->isValid = VFR->isValid && this->ValidateFragment_MinLength(VFR->fr, F);
	return VFR;
}

/* MAX GROWTH */

bool MasterAnalysisObject::ValidateFragment_MaxGrowth(Fragment^ Fr, Fact^ F)
{
	bool result = true;
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	Int32 iSymbol = 0;

	while ((iSymbol < this->model->alphabet->Size()) && (result))
	{
		if (PV[iSymbol] > F->growth[iSymbol, 1])
		{
			result = false;
		}
		iSymbol++;
	}

	return result;
}

ValidatedFragment^ MasterAnalysisObject::ValidateFragment_MaxGrowth(ValidatedFragment^ VFR, Fact^ F)
{
	VFR->isValid = VFR->isValid && this->ValidateFragment_MaxGrowth(VFR->fr, F);
	return VFR;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_MaxGrowth(Fragment^ Fr, Fact^ F)
{
	ValidatedFragmentX^ result = gcnew ValidatedFragmentX();
	result->vfr->fr = Fr;
	result->start = 0;
	result->end = Fr->fragment->Length;

	result = this->ReviseFragment_MaxGrowth(result, F);

	return result;
}

ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_MaxGrowth(ValidatedFragmentX^ VFRX, Fact^ F)
{
	ValidatedFragmentX^ result = VFRX;
	array<Int32>^ growth = gcnew array<Int32>(this->model->alphabet->Size());
	Int32 iPos = 0;
	bool stop = false;

	while ((!stop) && (iPos < VFRX->vfr->fr->fragment->Length))
	{
		// keep a running tally of how much growth in the fragment
		Int32 symbolIndex = this->model->alphabet->FindIndex(result->vfr->fr->fragment->Substring(iPos, 1));
		growth[symbolIndex]++;
		// if the amount of growth exceed the maximum for a symbol eliminate the rest of the fragment
		if (growth[symbolIndex] > F->growth[symbolIndex, 1])
		{
			result->start = 0;
			result->end = iPos;
			result->vfr->fr->fragment = result->vfr->fr->fragment->Substring(result->start, result->end); // note NOT iPos - 1, because this is a length and not a position in the string and it is zero indexed
			result->vfr->isValid = true;
		}
		iPos++;
	}

	return result;
}

/* MIN GROWTH */

bool MasterAnalysisObject::ValidateFragment_MinGrowth(Fragment^ Fr, Fact^ F)
{
	bool result = true;
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	Int32 iSymbol = 0;

	while ((iSymbol < this->model->alphabet->Size()) && (result))
	{
		if (PV[iSymbol] < F->growth[iSymbol, 0])
		{
			result = false;
		}
		iSymbol++;
	}

	return result;
}

ValidatedFragment^ MasterAnalysisObject::ValidateFragment_MinGrowth(ValidatedFragment^ VFR, Fact^ F)
{
	VFR->isValid = VFR->isValid && this->ValidateFragment_MaxGrowth(VFR->fr, F);
	return VFR;
}

bool MasterAnalysisObject::ValidateFragment_MaxHead(Fragment^ Fr, Fact^ F)
{
	if (F->head->max->fragment != nullptr && F->head->max->fragment->IndexOf(Fr->fragment) != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool MasterAnalysisObject::ValidateFragment_MinHead(Fragment^ Fr, Fact^ F)
{
	if (F->head->min->fragment != nullptr && Fr->fragment->IndexOf(F->head->min->fragment) != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool MasterAnalysisObject::ValidateFragment_MaxTail(Fragment^ Fr, Fact^ F)
{
	if (F->tail->max->fragment != nullptr && F->tail->max->fragment->LastIndexOf(Fr->fragment) + Fr->fragment->Length != F->tail->max->fragment->Length)
	{
		Int32 index = F->tail->max->fragment->LastIndexOf(Fr->fragment);
		return false;
	}
	else
	{
		return true;
	}
}

bool MasterAnalysisObject::ValidateFragment_MinTail(Fragment^ Fr, Fact^ F)
{
	if (F->tail->min->fragment != nullptr && Fr->fragment->LastIndexOf(F->tail->min->fragment) != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}


// This revises the fragments by considering the growth
// 1. If the fragment does not violate the min growth fact, then it can be trimmed to a mid fragment
// 2. If the fragment does violate the min growth, then trim left>right, then right>left, if these produce the same fragment it is a mid fragment
// NOTE: Point 2. encompasses Point 1. So although there's some efficiency to be gained when Point 1. is true, it is maybe easier to just do Point 2.
RevisedFragment^ MasterAnalysisObject::ConvertMax2MidFragment_MinGrowth(Fragment^ Fr, Fact^ F)
{
#if _PHASE3_REVISE_FRAGMENTS_GROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Revising Fragment " + Fr->fragment + " from Min Growth");
#endif

	RevisedFragment^ result = gcnew RevisedFragment();
	result->isRevised = false;

	if (!Fr->isComplete)
	{

		String^ trimmed1 = Fr->fragment;
		String^ trimmed2 = Fr->fragment;

		Int32 tmp = 0;
		// Check to see if there is at least two symbols in the min growth of the fact, cannot bookend a fragment if there are not at least 2 symbols
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
		{
			tmp += F->growth[iSymbol, 0];
		}

		if (tmp >= 2)
		{
			// Compute the PV of the fragment
			array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);

			// PART 1A. Trim left first, then right
			bool stop = false;
			Int32 iPos = 0;
			array<Int32>^ working = gcnew array<Int32>(this->model->alphabet->Size());
			Array::Copy(PV, working, PV->Length); // since we're going to be changing the PV, make a copy first so we still have the original for later

			do
			{
				Int32 symbolIndex = this->model->alphabet->FindIndex(trimmed1->Substring(iPos, 1));

				if (working[symbolIndex] > F->growth[symbolIndex, 0])
				{
					working[symbolIndex]--;
					iPos++;
				}
				else
				{
					stop = true;
				}
			} while (!stop);

			// Do the actual trim
			trimmed1 = trimmed1->Substring(iPos, trimmed1->Length - iPos);

			// Trim from the right until a symbol is reached that cannot be trimmed as it violates the min growth
			// PART 1A. Trim left first, then right
			stop = false;
			iPos = trimmed1->Length - 1;

			do
			{
				Int32 symbolIndex = this->model->alphabet->FindIndex(trimmed1->Substring(iPos, 1));

				if (working[symbolIndex] > F->growth[symbolIndex, 0])
				{
					working[symbolIndex]--;
					iPos--;
				}
				else
				{
					stop = true;
				}
			} while (!stop);

			// Do the actual trim
			trimmed1 = trimmed1->Substring(0, trimmed1->Length - iPos);

			// 2nd pass
			// Trim right first then left
			// Trim from the right until a symbol is reached that cannot be trimmed as it violates the min growth
			// PART 1B. Trim left first, then right
			stop = false;
			iPos = trimmed2->Length - 1;
			working = gcnew array<Int32>(this->model->alphabet->Size());
			Array::Copy(PV, working, PV->Length); // since we're going to be changing the PV, make a copy first so we still have the original for later

			do
			{
				Int32 symbolIndex = this->model->alphabet->FindIndex(trimmed2->Substring(iPos, 1));

				if (working[symbolIndex] > F->growth[symbolIndex, 0])
				{
					working[symbolIndex]--;
					iPos--;
				}
				else
				{
					stop = true;
				}
			} while (!stop);

			// Do the actual trim
			result->end = iPos + 1; // +1 because it is non-inclusive of the last character
			trimmed2 = trimmed2->Substring(0, iPos + 1);

			stop = false;
			iPos = 0;
			do
			{
				Int32 symbolIndex = this->model->alphabet->FindIndex(trimmed2->Substring(iPos, 1));

				if (working[symbolIndex] > F->growth[symbolIndex, 0])
				{
					working[symbolIndex]--;
					iPos++;
				}
				else
				{
					stop = true;
				}
			} while (!stop);

			// Do the actual trim
			result->start = iPos;
			trimmed2 = trimmed2->Substring(iPos, trimmed2->Length - iPos);

#if _PHASE3_REVISE_FRAGMENTS_GROWTH_ >= 1
			Console::WriteLine("Trimming Fragment " + Fr->fragment + " to min growth produces " + trimmed1 + " & " + trimmed2);
#endif
			if (trimmed1 == trimmed2)
			{
				result->fr = gcnew Fragment();
				result->fr->fragment = trimmed1;
				result->fr->isComplete = false;
				result->isRevised = true;
			}
		}
	}

	return result;
}

// TODO: Put Point 3. in its own function
// 3. If a fragment is less than the min growth, grow left>right, then right>left, if these produce the same fragment it is a mid fragment
// 4. A head/tail fragment is less than the min growth, can be grown, left or right, until it reaches the min growth
RevisedFragment^ MasterAnalysisObject::ReviseFragment_UnderMinGrowth(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End)
{
#if _PHASE3_REVISE_FRAGMENTS_UNDERGROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Revising Fragment " + Fr->fragment + " if Under Min Growth");
#endif

	RevisedFragment^ result = gcnew RevisedFragment();
	result->isRevised = false;

	Console::WriteLine(Fr->fragment + " " + Start + "::" + End);
	W->Display();

	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);

	bool isUnderGrowth = false;
	bool stop = false;
	Int32 iSymbol = 0;

	do
	{
		if (PV[iSymbol] < F->growth[iSymbol, 0])
		{
			isUnderGrowth = true;
			stop = true;
		}
		iSymbol++;
	} while ((!stop) && (iSymbol < this->model->alphabet->Size()) && (!isUnderGrowth));

	if (isUnderGrowth)
	{
		// Grow right then left until minimum met
		Int32 iPos1 = End + 1;
		Int32 iPos2 = Start; // will subtract 1 later if needed
		array<Int32>^ working = gcnew array<Int32>(this->model->alphabet->Size());
		Array::Copy(PV, working, PV->Length);
		Int32 length = Fr->fragment->Length;
		bool stop = false;

		while ((!stop) && (iPos1 < W->Length()) && (length < F->length->max))
		{
			Int32 symbolIndex = W->GetSymbolIndex(iPos1);
			if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
			{
				working[symbolIndex]++;
				iPos1++;
				length++;
			}
			else
			{
				stop = true;
			}
		}

		bool minimumMet = false;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			minimumMet = minimumMet && (working[iSymbol] >= F->growth[iSymbol, 0]);
		}

		if (!minimumMet)
		{
			stop = false;
			iPos2--;
			while ((!stop) && (iPos2 > 0) && (length < F->length->max))
			{
				Int32 symbolIndex = W->GetSymbolIndex(iPos2);
				if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
				{
					working[symbolIndex]++;
					iPos2--;
					length++;
				}
				else
				{
					stop = true;
				}
			}
		}

		String^ grown1 = W->GetSymbolsFromPos(iPos2, iPos1);

		// Grow left then right until the minimum is met
		iPos1 = End;
		iPos2 = Start - 1;
		working = gcnew array<Int32>(this->model->alphabet->Size());
		Array::Copy(PV, working, PV->Length);
		length = Fr->fragment->Length;
		stop = false;

		while ((!stop) && (iPos2 > 0) && (length < F->length->max))
		{
			Int32 symbolIndex = W->GetSymbolIndex(iPos2);
			if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
			{
				working[symbolIndex]++;
				iPos2--;
				length++;
			}
			else
			{
				stop = true;
			}
		}

		minimumMet = false;
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
		{
			minimumMet = minimumMet && (working[iSymbol] >= F->growth[iSymbol, 0]);
		}

		if (!minimumMet)
		{
			stop = false;
			iPos1++;
			while ((!stop) && (iPos1 < W->Length()) && (length < F->length->max))
			{
				Int32 symbolIndex = W->GetSymbolIndex(iPos1);
				if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
				{
					working[symbolIndex]++;
					iPos1++;
					length++;
				}
				else
				{
					stop = true;
				}
			}
		}

		String^ grown2 = W->GetSymbolsFromPos(iPos2, iPos1);

		// If the two results are the same then this is a unique amount of growth

		Console::WriteLine("Growing Fragment " + Fr->fragment + " to min growth gives " + grown1 + " and " + grown2);
		if (grown1 == grown2)
		{
			result->fr = gcnew Fragment();
			result->fr->fragment = grown1;
			result->fr->isComplete = false;
			result->isRevised = true;
		}
	}

	return result;
}

// 3. If a fragment is less than the min growth, grow left>right, then right>left, if these produce the same fragment it is a mid fragment
RevisedFragment^ MasterAnalysisObject::ReviseFragment_UnderMinGrowth_HeadFragmentOnly(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End)
{
#if _PHASE3_REVISE_FRAGMENTS_UNDERGROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Check if Head Fragment " + Fr->fragment + " is Under Min Growth");
#endif

	RevisedFragment^ result = gcnew RevisedFragment();
	result->isRevised = false;

	//Console::WriteLine(Fr->fragment + " " + Start + "::" + End);
	//W->Display();

	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);

	bool isUnderGrowth = false;
	bool stop = false;
	Int32 iSymbol = 0;

	do
	{
		if (PV[iSymbol] < F->growth[iSymbol, 0])
		{
			isUnderGrowth = true;
			stop = true;
		}
		iSymbol++;
	} while ((!stop) && (iSymbol < this->model->alphabet->Size()) && (!isUnderGrowth));

	if (isUnderGrowth)
	{
		Console::WriteLine("Revising Head Fragment " + Fr->fragment + " as it is under Min Growth");
		// Grow right until minimum met
		Int32 iPos1 = End + 1;
		array<Int32>^ working = gcnew array<Int32>(this->model->alphabet->Size());
		Array::Copy(PV, working, PV->Length);
		Int32 length = Fr->fragment->Length;
		bool stop = false;

		while ((!stop) && (iPos1 < W->Length()) && (length < F->length->max))
		{
			Int32 symbolIndex = W->GetSymbolIndex(iPos1);
			if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
			{
				working[symbolIndex]++;
				iPos1++;
				length++;
			}
			else
			{
				stop = true;
			}
		}

		String^ grown1 = W->GetSymbolsFromPos(Start, iPos1);

		Console::WriteLine("Growing Head Fragment " + Fr->fragment + " to min growth gives " + grown1);
		result->fr = gcnew Fragment();
		result->fr->fragment = grown1;
		result->fr->isComplete = false;
		result->isRevised = true;
	}

	return result;
}

// 3. If a fragment is less than the min growth, grow left>right, then right>left, if these produce the same fragment it is a mid fragment
RevisedFragment^ MasterAnalysisObject::ReviseFragment_UnderMinGrowth_TailFragmentOnly(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End)
{
#if _PHASE3_REVISE_FRAGMENTS_UNDERGROWTH_ >= 1
	Console::WriteLine();
	Console::WriteLine("Check if Tail Fragment " + Fr->fragment + " is Under Min Growth");
#endif

	RevisedFragment^ result = gcnew RevisedFragment();
	result->isRevised = false;

	//Console::WriteLine(Fr->fragment + " " + Start + "::" + End);
	//W->Display();

	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);

	bool isUnderGrowth = false;
	bool stop = false;
	Int32 iSymbol = 0;

	do
	{
		if (PV[iSymbol] < F->growth[iSymbol, 0])
		{
			isUnderGrowth = true;
			stop = true;
		}
		iSymbol++;
	} while ((!stop) && (iSymbol < this->model->alphabet->Size()) && (!isUnderGrowth));

	if (isUnderGrowth)
	{
		Console::WriteLine("Revising Tail Fragment " + Fr->fragment + " as it is under Min Growth");
		// Grow left until minimum met
		Int32 iPos2 = Start-1;
		array<Int32>^ working = gcnew array<Int32>(this->model->alphabet->Size());
		Array::Copy(PV, working, PV->Length);
		Int32 length = Fr->fragment->Length;
		bool stop = false;

		while ((!stop) && (iPos2 >= 0) && (length < F->length->max))
		{
			Int32 symbolIndex = W->GetSymbolIndex(iPos2);
			if ((working[symbolIndex] < F->growth[symbolIndex, 0]) && (working[symbolIndex] < F->growth[symbolIndex, 1]))
			{
				working[symbolIndex]++;
				iPos2--;
				length++;
			}
			else
			{
				stop = true;
			}
		}

		String^ grown1 = W->GetSymbolsFromPos(iPos2+1, End); // +1 to iPos2 because the loop above stops at the symbol that should NOT be included

		Console::WriteLine("Growing Tail Fragment " + Fr->fragment + " to min growth gives " + grown1);
		result->fr = gcnew Fragment();
		result->fr->fragment = grown1;
		result->fr->isComplete = false;
		result->isRevised = true;
	}

	return result;
}

// A. Update minimum length
// B. Update minimum growth
// C. Update other fragments
// I. for head fragments
// i. If min head fragment is not a subword of a max fragment, the max fragment is false
// ii. if min head fragment is exactly equal to a max fragment, then it is a complete fragment
// iii. Find the first index of the overlap, remove everything before that from max fragment

// II. for tail fragments
// i. If min tail fragment is not a subword of a max fragment, the max fragment is false
// ii. if min tail  fragment is exactly equal to a max fragment, then it is a complete fragment
// iii. Find the last index of the overlap, remove everything after that from max fragment
void MasterAnalysisObject::UpdateFact_MinFragment(Fact^ F, Fragment^ Fr)
{
	this->SetFact_MinLengthOnly(F, Fr->fragment->Length);
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		this->SetFact_MinGrowthOnly(F, iSymbol, PV[iSymbol]);
	}
}

// A. Update maximum length
// B. Update maximum growth
// C. Update other fragments
// I. for head fragments
// i. If max head fragment is subword of a max fragment, replace that fragment with the max head fragment
// ii. Find the longest overlap of the two fragments, replace the max fragment with the longest overlap
// iii. If there is no overlap, the max fragment is false, tabu it

// II. for tail fragments * note these are the same as head fragment
// i. If max tail fragment is subword of a max fragment, replace that fragment with the max head fragment
// ii. Find the longest overlap of the two fragments, replace the max fragment with the longest overlap
// iii. If there is no overlap, the max fragment is false, tabu it

void MasterAnalysisObject::UpdateFact_MaxFragment(Fact^ F, Fragment^ Fr)
{
	this->SetFact_MaxLengthOnly(F, Fr->fragment->Length);
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		this->SetFact_MaxGrowthOnly(F, iSymbol, PV[iSymbol]);
	}
}

// III. For BTF:
// Update length and growth
// Check existing head/tail fragments
void MasterAnalysisObject::UpdateFact_BTF(Fact^ F, Fragment^ Fr)
{
	this->SetFact_MaxLengthOnly(F, Fr->fragment->Length);
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		this->SetFact_MaxGrowthOnly(F, iSymbol, PV[iSymbol]);
	}

	if (F->head->max->fragment != nullptr)
	{
		F->head->max->fragment = Fr->fragment;
	}

	if (F->tail->max->fragment != nullptr)
	{
		F->tail->max->fragment = Fr->fragment;
	}
}

// this sets the fact based on the max fragments being updated
void MasterAnalysisObject::UpdateFact_MaxFragments(Fact^ F)
{
	Int32 longest = 0;
	array<Int32>^ largest = gcnew array<Int32>(this->model->alphabet->Size());

	for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
	{
		if (F->max[iFragment]->fragment->Length > longest)
		{
			longest = F->max[iFragment]->fragment->Length;
		}

		array<Int32>^ PV = this->model->alphabet->CreateParikhVector(F->max[iFragment]->fragment);
		for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
		{
			if (PV[iSymbol] > largest[iSymbol])
			{
				largest[iSymbol] = PV[iSymbol];
			}
		}
	}

	this->SetFact_MaxLengthOnly(F, longest);
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		this->SetFact_MaxGrowthOnly(F, iSymbol, largest[iSymbol]);
	}
}

// A. Update minimum length
// B. Update minimum growth
// C. Update other fragments. Set all other fragments to the complete fragment just to ensure a bad fragment doesn't get taken anywhere else in the code
void MasterAnalysisObject::UpdateFact_CompleteFragment(Fact^ F, Fragment^ Fr)
{
	this->SetFact_MinLengthOnly(F, Fr->fragment->Length);
	this->SetFact_MaxLengthOnly(F, Fr->fragment->Length);
	array<Int32>^ PV = this->model->alphabet->CreateParikhVector(Fr->fragment);
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->Size(); iSymbol++)
	{
		this->SetFact_MinGrowthOnly(F, iSymbol, PV[iSymbol]);
		this->SetFact_MaxGrowthOnly(F, iSymbol, PV[iSymbol]);
	}

	// Update other fragments
	F->head->min = Fr;
	F->head->max = Fr;
	F->tail->min = Fr;
	F->tail->max = Fr;
	F->mid = Fr;
	if (F->btf->Count >= 1)
	{
		F->btf[0]->Update(Fr->fragment, F);
	}
}

void MasterAnalysisObject::SetFact_CompleteFragmentOnly(Fact^ F, Fragment^ Fr)
{
	if (!this->IsNoFact(F))
	{
		F->complete = Fr;
		this->UpdateFact_CompleteFragment(F, Fr);
	}
}

void MasterAnalysisObject::SetFact_CompleteFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_CompleteFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MinHeadFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MinHeadFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MaxHeadFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MaxHeadFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MinTailFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MinTailFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MaxTailFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MaxTailFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MidFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MidFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_MaxFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_MaxFragmentOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

//void MasterAnalysisObject::SetFact_BTFOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr)
//{
//	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
//	this->SetFact_BTFOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr);
//}

void MasterAnalysisObject::SetFact_BTFOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, FragmentSigned^ Fr)
{
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_BTFOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr);
}

//void MasterAnalysisObject::SetFact_BTFOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr)
//{
//	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
//	this->ComputeBTF_L
//	String^ left = "";
//	String^ right = "";
//	this->SetFact_BTFOnly(f, this->CreateBTF(Fr, left, right));
//}

void MasterAnalysisObject::SetFact_BTFOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, FragmentSigned^ Fr)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
	this->SetFact_BTFOnly(f, Fr);
}

void MasterAnalysisObject::SetFact_BTFOnly(Fact^ F, FragmentSigned^ Fr)
{
	if (!this->IsNoFact(F))
	{

		if (F->btf->Count == 0)
		{
			F->btf->Add(Fr);
			this->UpdateFact_BTF(F, Fr);
		}
		else
		{
			bool added = false;

			for (size_t iFragment = 0; iFragment < F->btf->Count; iFragment++)
			{
				// if the new BTF is compatible with an existing BTF, then update the existing BTF (do not add/remove them because we want them to stay in sync with the SAC instance ones)
				if (this->IsSubword(F->btf[iFragment]->fragment, Fr->fragment) && this->IsSignatureCompatible(F->btf[iFragment]->leftSignature, F->btf[iFragment]->rightSignature, Fr->leftSignature, Fr->rightSignature))
				{
					if (F->btf[iFragment]->fragment != Fr->fragment)
					{
						F->btf[iFragment]->fragment = Fr->fragment;
						this->changeBTF = true;
					}
					added = true;
				}
			}

			if (!added)
			{
				// TODO: need to enhance this, check to see if the fragment is equal to another BTF, if so then check to see if the signatures can be made compatible
				// if so immediately adjust the signature of the matching BTF
				if (!this->IsBTFInList(Fr, F->btf))
				{
					// A. If two BTFs have the same fragment, then their signatures can be compressed (and one of them can be removed)
					bool shouldAdd = true;
					for (size_t iBTF = 0; iBTF < F->btf->Count; iBTF++)
					{
						if (F->btf[iBTF]->fragment == Fr->fragment)
						{
							Console::WriteLine("For SAC " + this->SymbolCondition(F->sac->iSymbol, F->sac->iLeft, F->sac->iRight) + " BTF " + F->btf[iBTF]->fragment + " is equal to " + Fr->fragment);
							String^ right = this->Compute_PrefixOverlap(F->btf[iBTF]->rightSignature, Fr->rightSignature);
							String^ left = this->Compute_SuffixOverlap(F->btf[iBTF]->leftSignature, Fr->leftSignature);
							Console::WriteLine("For SAC " + this->SymbolCondition(F->sac->iSymbol, F->sac->iLeft, F->sac->iRight) + " new signatures for BTF " + F->btf[iBTF]->fragment + " & " + left + " / " + right);
							F->btf[iBTF]->leftSignature = left;
							F->btf[iBTF]->rightSignature = right;
							shouldAdd = false;
						}
					}

					if (shouldAdd)
					{
						F->btf->Add(Fr);
						this->changeBTF = true;
					}
				}
			}

			this->UpdateFact_BTF(F, Fr);
		}
	}

	// it is critical that the list remain sorted as we always want to use the longest BTF possible during localization
	F->btf->Sort(gcnew BTFLengthComparer());
}

void MasterAnalysisObject::SetFact_BTFOnly_Direct(Fact^ F, FragmentSigned^ Fr)
{
	if (!this->IsBTFInList(Fr, F->btf))
	{
		F->btf->Add(Fr);
		F->btf->Sort(gcnew BTFLengthComparer());
		this->UpdateFact_BTF(F, Fr);
	}
}

void MasterAnalysisObject::SetFact_BTFOnly_UncertainOnly(Fact^ F, Int32 iBTF, FragmentSigned^ Fr)
{
	List<ValidatedFragment^>^ toAdd = this->ReviseMaxFragment(Fr, F);

	for (size_t i = 0; i < toAdd->Count; i++)
	{
		if (toAdd[i]->isValid)
		{
			F->btf[iBTF]->uncertain->Add(this->CreateBTF(toAdd[i]->fr, Fr->leftSignature, Fr->rightSignature));
		}
	}
}

void MasterAnalysisObject::SetFact_BTFOnly_Update(Fact^ F, FragmentSigned^ Fr)
{
}

void MasterAnalysisObject::SetFact_BTFOnly_Update(FragmentSigned^ Fr1, FragmentSigned^ Fr2, Fact^ F)
{
	if (Fr1->fragment != Fr2->fragment)
	{
		Fr1->fragment = Fr2->fragment;
		this->UpdateFact_BTF(F, Fr1);
 		this->changeBTF = true;
	}
}

void MasterAnalysisObject::SetFact_CacheOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, FragmentSigned^ Fr, WordXV3^ W2, Int32 Start, Int32 End)
{
	//String^ tmp = W2->GetSymbolsFromPos(Start, End);
	//Console::WriteLine(tmp + " = " + Fr->fragment);
	array<Int32>^ coords = this->ConvertSymbolInWordToFactTable(iGen, W1, iPos);
	this->SetFact_CacheOnly(coords[0], coords[1], coords[2], coords[3], coords[4], nullptr, Fr, W2, Start, End);
}

void MasterAnalysisObject::SetFact_CacheOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, FragmentSigned^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (!this->IsBTFInList(Fr, f->cache))
		{
			
			f->cache->Add(Fr);
		}
	}
}

void MasterAnalysisObject::SetFact_CompleteFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		f->complete = Fr;
		this->UpdateFact_CompleteFragment(f, Fr);
	}
}

void MasterAnalysisObject::SetFact_MinHeadFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (Fr->isComplete)
		{
			this->SetFact_CompleteFragmentOnly(f, Fr);
		}

		if ((f->head->min->fragment == nullptr) || (Fr->fragment->Length > f->head->min->fragment->Length))
		{
			if (this->IsFragmentRevisable(Fr))
			{
				RevisedFragment^ candidate = this->ReviseFragment_UnderMinGrowth_HeadFragmentOnly(Fr, f, W, Start, End);
				if (candidate->isRevised)
				{
					Fr = candidate->fr;
				}
			}

			f->head->min = Fr;
			this->change = true;
			this->UpdateFact_MinFragment(f, Fr);
		}
	}
}

void MasterAnalysisObject::SetFact_MaxHeadFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (Fr->isComplete)
		{
			this->SetFact_CompleteFragmentOnly(f, Fr);
		}

		if ((f->head->max->fragment == nullptr) || (Fr->fragment->Length < f->head->max->fragment->Length))
		{
			f->head->max = Fr;
			this->change = true;
			this->SetFact_MaxFragmentOnly(iGen, iTable, iSymbol, iLeft, iRight, V, f->head->max, W, Start, End);
			this->UpdateFact_MaxFragment(f, f->head->max);
		}
	}
}

void MasterAnalysisObject::SetFact_MinTailFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (Fr->isComplete)
		{
			this->SetFact_CompleteFragmentOnly(f, Fr);
		}

		if ((f->tail->min->fragment == nullptr) || (Fr->fragment->Length > f->tail->min->fragment->Length))
		{
			if (this->IsFragmentRevisable(Fr))
			{
				RevisedFragment^ candidate = this->ReviseFragment_UnderMinGrowth_TailFragmentOnly(Fr, f, W, Start, End);
				if (candidate->isRevised)
				{
					Fr = candidate->fr;
				}
			}

			f->tail->min = Fr;
			this->change = true;
			this->UpdateFact_MinFragment(f, Fr);
		}
	}
}

void MasterAnalysisObject::SetFact_MaxTailFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (Fr->isComplete)
		{
			this->SetFact_CompleteFragmentOnly(f, Fr);
		}

		if ((f->tail->max->fragment == nullptr) || (Fr->fragment->Length < f->tail->max->fragment->Length))
		{
			f->tail->max = Fr;
			this->SetFact_MaxFragmentOnly(iGen, iTable, iSymbol, iLeft, iRight, V, f->tail->max, W, Start, End);
			this->UpdateFact_MaxFragment(f, f->tail->max);
			this->change = true;
		}
	}
}

void MasterAnalysisObject::SetFact_MidFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);

	if (!this->IsNoFact(f))
	{
		if (Fr->isComplete)
		{
			this->SetFact_CompleteFragmentOnly(f, Fr);
		}

		if ((f->mid->fragment == nullptr) || (Fr->fragment->Length > f->mid->fragment->Length))
		{
			if (this->IsFragmentRevisable(Fr))
			{
				RevisedFragment^ candidate = this->ReviseFragment_UnderMinGrowth(Fr, f, W, Start, End);
				if (candidate->isRevised)
				{
					Fr = candidate->fr;
				}
			}

			f->mid = Fr;
			this->UpdateFact_MinFragment(f, Fr);
			this->change = true;
		}
	}
}

bool MasterAnalysisObject::IsSubword(String^ Str1, String^ Str2)
{
	return Str1->Length >= Str2->Length && Str1->Contains(Str2);
}

bool MasterAnalysisObject::IsCommonSubword(Fragment^ Fr, List<FragmentSigned^>^ L)
{
	bool result = true;
	Int32 idx = 0;
	while (result && idx < L->Count)
	{
		result = result && this->IsSubword(L[idx]->fragment, Fr->fragment);
		idx++;
	}

	return result;
}

String^ MasterAnalysisObject::FindLongestSubword(String^ Fr1, String^ Fr2)
{
	if (Fr2->Length == 0)
	{
		return "";
	}
	if (Fr1->Contains(Fr2))
	{
		//Console::WriteLine(Fr2 + " is subword of " + Fr1);
		return Fr2;
	}
	else
	{
		String^ result = FindLongestSubword(Fr1, Fr2->Substring(0, Fr2->Length - 1));
		if ((result == nullptr) || (result == ""))
		{
			result = FindLongestSubword(Fr1, Fr2->Substring(1, Fr2->Length - 1));
		}

		return result;
	}
}

List<String^>^ MasterAnalysisObject::GetSubwords(String^ S)
{
	List<String^>^ result = gcnew List<String^>;

	return result;
}

List<String^>^ MasterAnalysisObject::GetSubwordsOfLength(WordXV3^ W, Int32 Length)
{
	return this->GetSubwordsOfLength(W->GetSymbols(0, W->Length()),Length);
}

List<String^>^ MasterAnalysisObject::GetSubwordsOfLength(String^ S, Int32 Length)
{
	List<String^>^ result = gcnew List<String^>;

	for (size_t iStart = 0; iStart <= S->Length - Length; iStart++)
	{
		String^ tmp = S->Substring(iStart, Length);
		if (!result->Contains(tmp))
		{
			result->Add(tmp);
		}
	}

	return result;
}

bool MasterAnalysisObject::IsFullyCovered(Int32 Length, List<Range^>^ Covered)
{
	bool result = true;
	array<bool>^ completed = gcnew array<bool>(Length);

	for (size_t i = 0; i < Covered->Count; i++)
	{
		for (size_t j = Covered[i]->start; j <= Covered[i]->end; j++)
		{
			completed[j] = true;
		}
	}

	Int32 idx = 0;

	while ((result) && (idx < Length))
	{
		result = result && completed[idx];
		idx++;
	}

	return result;
}

// This function takes two fragments and finds the ways in which that can overlap
// Fr1 is the longer of the two fragments
// Terminates when Fr1 is fully covered or no more subwords are found
// If at termination F1 is not fully covered then: 
// A. If both fragments are true, the uncovered sections are rejected
// B. If both Fr2 is true, the uncovered sections are returned as subwords
// C. If both fragments are false, return Fr1
List<Fragment^>^ MasterAnalysisObject::Compute_MaxFragmentsFromPairedFragments(Fragment^ Fr1, Fragment^ Fr2, Fact^ F)
{
	List<Fragment^>^ result = gcnew List<Fragment^>;
	bool done = false;
	Int32 iLength = Fr2->fragment->Length - 1;
	array<bool>^ completed = gcnew array<bool>(Fr1->fragment->Length);

	while ((!done) && (iLength >= 1))
	{
		List<String^>^ subwords = this->GetSubwordsOfLength(Fr2->fragment, iLength);

		for (size_t iWord = 0; iWord < subwords->Count; iWord++)
		{
			//Console::WriteLine(subwords[iWord]);
			Int32 start = 0;
			Int32 index = -1;
			do
			{
				index = Fr1->fragment->IndexOf(subwords[iWord], start);
				if (index >= 0)
				{
					//Console::WriteLine("Fragment " + subwords[iWord] + " found @ " + index);
					Int32 end = index + subwords[iWord]->Length - 1;

					bool isCovered = true;
					Int32 iCovered = index;

					while ((iCovered < end) && (isCovered))
					{
						isCovered = isCovered && completed[iCovered];
						iCovered++;
					}

					if (!isCovered)
					{
						// check to see if the word has already been added
						bool isDuplicate = false;
						bool shouldCover = true;
						Int32 iFragment = 0;

						while ((!isDuplicate) && (iFragment < result->Count))
						{
							isDuplicate = (result[iFragment]->fragment == subwords[iWord]);
							iFragment++;
						}

						if (!isDuplicate)
						{
							Fragment^ fr = this->CreateFragment(subwords[iWord], false, false); // initial set the subword to false, but can be changed to true if there's only one

																								// check to see if the fragment is tabu, do not capture tabu fragments and do not mark the area covered
							if (!this->IsFragmentTabu(fr, F))
							{
								result->Add(fr);
							}
							else
							{
								shouldCover = false; // do not cover a tabu subword
							}
						}

						// set the range of the subword to be completed
						if (shouldCover)
						{
							for (size_t i = index; i <= end; i++)
							{
								completed[i] = true;
							}
						}

						// check to see if the fragment if fully completed
						Int32 idx = 0;
						bool allTrue = true;
						while ((allTrue) && (idx < Fr1->fragment->Length))
						{
							allTrue = allTrue && completed[idx];
							idx++;
						}

						done = allTrue;
					}

					start = index + 2; // +2 to convert from a 0-indexed value to the position after the index
				}
			} while ((index >= 0) && (start < Fr1->fragment->Length));
		}

		iLength--;
	}

	// If at termination F1 is not fully covered then: 
	// A. If both fragments are true, the uncovered sections are rejected
	// B. If Fr2 is true, the uncovered sections are returned as subwords
	// C. If both fragments are false, return Fr1
	if (!done)
	{
		if (!Fr1->isTrue && !Fr2->isTrue)
		{// scenario C
			result->Insert(0, Fr1);
		}
		else if (!Fr1->isTrue && Fr2->isTrue)
		{// scenario B
			Console::WriteLine("Find uncovered sections");
		}
	}

	// if there is only one subword in the result then change it to be a true superstring if Fr1 and Fr2 are true
	if (Fr1->isTrue && Fr2->isTrue && result->Count == 1)
	{
		result[0]->isTrue = true;
	}

	return result;
}

// IT IS CRITICAL THAT ALL MAX FRAGMENTS ARE TRUE WHEN THE ARE PASSED IN HERE!
void MasterAnalysisObject::SetFact_MaxFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End)
{
	if (Fr->isTrue)
	{
		Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
		//this->SetFact_BTFOnly(f, Fr);
		this->SetFact_MaxFragmentOnly(f, Fr);
	}
	else
	{
		Console::WriteLine("Attempted to add a max fragment " + Fr->fragment + " but it is not a true fragment");
	}
}

// this function can add a false fragment
// minimal checks are done before adding it
void MasterAnalysisObject::SetFact_MaxFragmentOnly_Direct(Fact^ F, FragmentSigned^ Fr)
{
	List<ValidatedFragment^>^ toAdd = this->ReviseMaxFragmentWithCorrections(Fr, F);

	//for (size_t i = 0; i < toAdd->Count; i++)
	//{
	//	if ((toAdd[i]->isValid) && (!this->IsSignedFragmentInList(toAdd[i]->fr, F->max)))
	//	{
	//		F->max->Add(toAdd[i]->fr);
	//	}
	//}

	F->max->Sort(gcnew BTFLengthComparer());
	this->UpdateFact_MaxFragments(F);
}

void MasterAnalysisObject::SetFact_MaxFragmentOnly(Fact^ F, Fragment^ Fr)
{
	//if (!this->IsNoFact(F))
	//{
	//	// A complete fragment does not need to be processed, just add it
	//	if (Fr->isComplete)
	//	{
	//		this->SetFact_CompleteFragmentOnly(F, Fr);
	//	}
	//	else if (F->max->Count == 0)
	//	{// if there's no existing fragment, revise it and then add the result
	//		List<ValidatedFragment^>^ toAdd = this->ReviseMaxFragmentWithCorrections(Fr, F);

	//		for (size_t i = 0; i < toAdd->Count; i++)
	//		{
	//			if (toAdd[i]->isValid)
	//			{
	//				F->max->Add(toAdd[i]->fr);
	//			}
	//		}

	//		if (F->max->Count > 0)
	//		{
	//			F->max->Sort(gcnew FragmentLengthComparer());
	//			this->UpdateFact_MaxFragments(F);
	//			this->change = true;
	//		}
	//		else
	//		{
	//			Console::WriteLine("Attempted to add a max fragment " + Fr->fragment + " but none were valid after corrections");
	//		}

	//	}
	//	// if the incoming fragment is very short, simply replace the fragment list with the new fragment as it is guaranteed true
	//	// unless there's exactly one fragment that is already shorter
	//	else if (((Fr->fragment->Length <= 3) && (F->max->Count > 1)) || ((F->max->Count == 1) && (Fr->fragment->Length < F->max[0]->fragment->Length)))
	//	{
	//		F->max->Clear();
	//		F->max->Add(Fr);
	//		this->UpdateFact_MaxFragments(F);
	//	}
	//	else
	//	{
	//		// A. Check to make sure the fragment isn't already in the list
	//		if (!this->IsFactHasMaxFragment(Fr, F))
	//		{
	//			// B. For every fragment, check to see the lengths of the new and existing fragment
	//			// If the new fragment is shorter, and is a subword of the old fragment (which really means just a subword), replace the old fragment with the new one
	//			// If the new fragment is shorter, and is not a subword of the old fragment, then revise the fragment
	//			// If the old fragment is a subword, null operation
	//			// If the old fragment is not a subword, then review the fragments
	//			// Note, at this point the incoming max fragment must be true
	//			// DO NOT DO ANY PROCESS TO IT PRIOR TO THIS POINT THAT COULD MAKE IT FALSE
	//			// SERIOUSLY, I LOOKED INTO THIS A LOT AND IF THE FRAGMENT IS POTENTIALLY FALSE YOU CAN DESTROY A TRUE FRAGMENT!!
	//			List<Fragment^>^ toAdd = gcnew List<Fragment^>;
	//			List<Fragment^>^ toRemove = gcnew List<Fragment^>;
	//			List<Fragment^>^ toTabu = gcnew List<Fragment^>;
	//			bool doNotAddOriginal = false;
	//			for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
	//			{
	//				// If the new fragment is shorter, and is a subword of the old fragment (which really means just a subword), replace the old fragment with the new one
	//				if (this->IsSubword(F->max[iFragment]->fragment, Fr->fragment))
	//				{
	//					toTabu->Add(F->max[iFragment]); // this fragment is now tabu
	//					toAdd->Add(Fr);
	//				}
	//				// If the new fragment is shorter, and is not a subword of the old fragment, then revise the fragment
	//				else if (Fr->fragment->Length <= F->max[iFragment]->fragment->Length)
	//				{
	//					toRemove->Add(F->max[iFragment]); // this fragment is NOT tabu
	//					List<Fragment^>^ fragments = this->Compute_MaxFragmentsFromPairedFragments(F->max[iFragment], Fr, F);
	//					// If the reivision fully covers the existing fragment, then do not add the new fragment
	//					if (fragments[0] != F->max[iFragment])
	//					{// if these are not equal then the entire fragment was covered
	//						doNotAddOriginal = true; // this forces the original fragment not to be added as it has been revised here
	//					}
	//					// otherwise add the new fragment
	//					for (size_t iAdd = 0; iAdd < fragments->Count; iAdd++)
	//					{
	//						toAdd->Add(fragments[iAdd]);
	//					}
	//				}
	//				// If the old fragment is a subword, null operation
	//				// If the old fragment is not a subword, then revise the fragments
	//				else if (!this->IsSubword(Fr->fragment, F->max[iFragment]->fragment))
	//				{
	//					toTabu->Add(F->max[iFragment]); // this fragment is now tabu
	//					List<Fragment^>^ fragments = this->Compute_MaxFragmentsFromPairedFragments(Fr, F->max[iFragment], F);
	//					for (size_t iAdd = 0; iAdd < fragments->Count; iAdd++)
	//					{
	//						toAdd->Add(fragments[iAdd]);
	//					}
	//				}
	//			}

	//			// remove all the fragments that need removing
	//			for (size_t iRemove = 0; iRemove < toRemove->Count; iRemove++)
	//			{
	//				this->RemoveFact_MaxFragmentOnly(F, toRemove[iRemove], false);
	//			}

	//			// remove all the fragments that need removing
	//			for (size_t iRemove = 0; iRemove < toTabu->Count; iRemove++)
	//			{
	//				this->RemoveFact_MaxFragmentOnly(F, toTabu[iRemove], true);
	//			}

	//			// add the fragments that need adding, do not add duplicates (Fragment revision will validate fragments)
	//			for (size_t iAdd = 0; iAdd < toAdd->Count; iAdd++)
	//			{
	//				if (!((doNotAddOriginal) && (Fr->fragment == toAdd[iAdd]->fragment)))
	//				{
	//					// C. Revise the fragment into sub-fragments
	//					List<ValidatedFragment^>^ revised = this->ReviseMaxFragment(toAdd[iAdd], F);
	//					for (size_t iRevised = 0; iRevised < revised->Count; iRevised++)
	//					{
	//						if (!revised[iRevised]->isValid)
	//						{
	//							//List<ValidatedFragment^>^ corrected = this->ReviseMaxFragment_Correct(toAdd[iAdd], f);
	//							//for (size_t iCorrected = 0; iCorrected < corrected->Count; iCorrected++)
	//							//{
	//							//	F->max->Add(corrected[iCorrected]->fr);
	//							//}
	//						}
	//						else if (!this->IsFactHasMaxFragment(revised[iRevised]->fr, F))
	//						{
	//							F->max->Add(revised[iRevised]->fr);
	//						}
	//					}
	//				}
	//			}

	//			if (F->max->Count == 0)
	//			{
	//				List<ValidatedFragment^>^ corrected = this->ReviseMaxFragment_Correct(Fr, F);
	//				for (size_t iCorrected = 0; iCorrected < corrected->Count; iCorrected++)
	//				{
	//					F->max->Add(corrected[iCorrected]->fr);
	//				}
	//			}

	//			// D. Update other facts based on the max fragments
	//			// The max length is the longest of any fragment
	//			// The max growth of a symbol is the largest in any fragment
	//			F->max->Sort(gcnew FragmentLengthComparer());
	//			this->UpdateFact_MaxFragments(F);
	//		}
	//	}
	//}
}

void MasterAnalysisObject::RemoveFact_MaxFragmentOnly(Fact^ F, List<Fragment^>^ Fr, bool IsTabu)
{
	for (size_t iFragment = 0; iFragment < Fr->Count; iFragment++)
	{
		this->RemoveFact_MaxFragmentOnly(F, Fr[iFragment], IsTabu);
	}
}

void MasterAnalysisObject::RemoveFact_MaxFragmentOnly(Fact^ F, Fragment^ Fr, bool IsTabu)
{
	if (IsTabu)
	{
		if (!F->tabu->Contains(Fr->fragment))
		{
			F->tabu->Add(Fr->fragment);
		}
	}

	Int32 iFragment = 0;
	bool found = false;
	while ((!found) && (iFragment < F->max->Count))
	{
		if (Fr->fragment == F->max[iFragment]->fragment)
		{
			found = true;
		}
		else
		{
			iFragment++;
		}
	}

	if (found)
	{
		F->max->RemoveAt(iFragment);
	}
}

String^ MasterAnalysisObject::Compute_PrefixOverlap(String^ Frag1, String^ Frag2)
{
	String^ result = "";

	if (Frag1 != "" && Frag2 != "")
	{
		String^ shortest;
		String^ longest;

		if (Frag1->Length <= Frag2->Length)
		{
			shortest = Frag1;
			longest = Frag2;
		}
		else
		{
			shortest = Frag2;
			longest = Frag1;
		}

		Int32 index = 0;
		Int32 length = 1;
		do
		{
			index = longest->IndexOf(shortest->Substring(0, length));
			length++;
		} while (index == 0 && length <= shortest->Length);
		
		result = shortest->Substring(0, length - 2);
	}

	return result;
}

String^ MasterAnalysisObject::Compute_SuffixOverlap(String^ Frag1, String^ Frag2)
{
	String^ result = "";
	
	if (Frag1 != "" && Frag2 != "")
	{
		String^ shortest;
		String^ longest;

		if (Frag1->Length <= Frag2->Length)
		{
			shortest = Frag1;
			longest = Frag2;
		}
		else
		{
			shortest = Frag2;
			longest = Frag1;
		}

		Int32 index = 0;
		Int32 length = 1;
		do
		{
			index = longest->LastIndexOf(shortest->Substring(shortest->Length - length, length));
			length++;
		} while (index + (length - 1) == longest->Length && length <= shortest->Length);

		result = shortest->Substring(shortest->Length - (length - 1), length - 1);
	}

	return result;
}

void MasterAnalysisObject::RemoveFact_ResetCache(Fact^ F)
{
	F->cache->Clear();
}

// A. Check to ensure [ ] symbols are balanced
// B. Check to for invalid turtle commands are the head and tail, trim these
// C. Check to for invalid turtle commands mid fragment, split these
// D. Check fragment to head / tail max. Can eliminate symbols at the head and tail based on overlap.
List<ValidatedFragment^>^ MasterAnalysisObject::ReviseMaxFragment(Fragment^ Fr, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;

	// A. Check to ensure [ ] symbols are balanced, split these
	 List<ValidatedFragment^>^ tmp = this->ReviseFragment_BranchingMaxFragmentsOnly(Fr, F);

	// B. Check to for invalid turtle commands at the head and tail, trim these
	for (size_t iFragment = 0; iFragment < tmp->Count; iFragment++)
	{
		tmp[iFragment] = this->ReviseFragment_Turtles(Fr, F)->vfr;
	}

	// C. Check to for invalid turtle commands in the middle, split these
	List<ValidatedFragment^>^ tmp2;
	List<ValidatedFragment^>^ tmp3 = gcnew List<ValidatedFragment^>;
	for (size_t iFragment = 0; iFragment < tmp->Count; iFragment++)
	{
		tmp2 = this->ReviseFragment_TurtlesMaxFragmentsOnly(tmp[iFragment], F);
		for (size_t i = 0; i < tmp2->Count; i++)
		{
			tmp3->Add(tmp2[i]);
		}
	}

	// D. Check fragment to head / tail max. Can eliminate symbols at the head and tail based on overlap.
	for (size_t iFragment = 0; iFragment < tmp3->Count; iFragment++)
	{
		tmp3[iFragment] = this->ReviseFragment_HeadFragmentMaxFragmentsOnly(tmp3[iFragment], F);
		tmp3[iFragment] = this->ReviseFragment_TailFragmentMaxFragmentsOnly(tmp3[iFragment], F);
	}

	for (size_t i = 0; i < tmp3->Count; i++)
	{
		result->Add(this->ValidateFragment(tmp3[i]->fr, F));
	}

	return result;
}

// this is a recursive function that corrects a max fragment, but then ensures that the result is itself valid
List<ValidatedFragment^>^ MasterAnalysisObject::ReviseMaxFragment_Correct(Fragment^ Fr, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;

	// A. if the fragment is too long then replace with all the subwords of the max length
	if (Fr->fragment->Length > F->length->max)
	{
		List<String^>^ s = this->GetSubwordsOfLength(Fr->fragment, F->length->max);
		for (size_t i = 0; i < s->Count; i++)
		{
			List<ValidatedFragment^>^ tmp = this->ReviseMaxFragment_Correct(this->CreateFragment(s[i], false, false), F);
			for (size_t i = 0; i < tmp->Count; i++)
			{
				result->Add(tmp[i]);
			}
		}
	}
	// B. if the fragment is on the tabu list then replace with all the subwords of the max length - 1
	else if (F->tabu->Contains(Fr->fragment))
	{
		List<String^>^ s = this->GetSubwordsOfLength(Fr->fragment, Fr->fragment->Length - 1);
		for (size_t i = 0; i < s->Count; i++)
		{
			List<ValidatedFragment^>^ tmp = this->ReviseMaxFragment_Correct(this->CreateFragment(s[i], false, false), F);
			for (size_t i = 0; i < tmp->Count; i++)
			{
				result->Add(tmp[i]);
			}
		}
	}
	else
	{
		result->Add(this->CreateVFR(Fr));
	}

	return result;
}

List<ValidatedFragment^>^ MasterAnalysisObject::ReviseMaxFragmentWithCorrections(Fragment^ Fr, Fact^ F)
{
	List<ValidatedFragment^>^ result = gcnew List<ValidatedFragment^>;
	List<ValidatedFragment^>^ tmp1 = this->ReviseMaxFragment(Fr, F);

	// check the fragments, if any are invalid, then correct them
	// Allowed corrections:
	// A. if the fragment is too long then replace with all the subwords of the max length
	// B. if the fragment is on the tabu list then replace with all the subwords of the max length - 1
	for (size_t i = 0; i < tmp1->Count; i++)
	{
		bool done = false;

			if (!tmp1[i]->isValid)
			{
				List<ValidatedFragment^>^ tmp2 = this->ReviseMaxFragment_Correct(tmp1[i]->fr, F);
				for (size_t i = 0; i < tmp2->Count; i++)
				{
					result->Add(tmp2[i]);
				}
			}
			else
			{
				result->Add(tmp1[i]);
			}
	}

	return result;
}


// If the fragment violates any other rule it is rejected
// Only call this if the fragment is certain to be true
// I.e. do not call this with split max fragments since they are not guaranteed to be true
// A. Cannot violate maximum length. 
// B. Cannot violate minimum length. 
// C. Cannot violate minimum growth
// D. Cannot violate maximum growth
// E. Cannot be in the tabu list
// F. Must be a subword of all BTFs
// G. Must properly overlap head/tail fragment
ValidatedFragment^ MasterAnalysisObject::ValidateFragment(Fragment^ Fr, Fact^ F)
{
	ValidatedFragment^ result = gcnew ValidatedFragment();
	result->fr = Fr;
	result->isValid = true;

	if (this->IsFragmentTabu(Fr, F))
	{
		result->isValid = false;
	}
	else
	{
		// A. & B. Cannot violate min/max length
		result = this->ValidateFragment_MaxLength(result, F);
		result = this->ValidateFragment_MinLength(result, F);

		// C. & D. Cannot violate min/max growth
		result = this->ValidateFragment_MaxGrowth(result, F);
		result = this->ValidateFragment_MinGrowth(result, F);

		// F. Must be a subword of all BTFs
		result->isValid = result->isValid && this->IsCommonSubword(Fr, F->btf);

		// G. Must properly overlap head/tail fragment
		result->isValid = result->isValid && this->ValidateFragment_MaxHead(Fr, F);
		result->isValid = result->isValid && this->ValidateFragment_MaxTail(Fr, F);

	}

	return result;
}

// A. Cannot violate maximum length
// B. Check to ensure [ ] symbols are balanced
// C. Check for impossible turtle commands, e.g. ++- cannot exist in a single successor
// D. Check for impossible turtle commands mid fragment, split the fragment, keep the head (note a tail fragment is reversed so still keep the 'head')
// E. Cannot violate maximum growth
// TODO: F. A head/tail fragment is less than the max growth (always will be), can be grown, left or right, until it reaches the max growth
ValidatedFragmentX^ MasterAnalysisObject::ReviseFragment_HeadTailFragmentOnly(String^ Frag, Fact^ F)
{
	ValidatedFragmentX^ result = gcnew ValidatedFragmentX();
	result->vfr = gcnew ValidatedFragment();
	result->vfr->fr = gcnew Fragment();
	result->vfr->fr->fragment = Frag;
	result->vfr->fr->isComplete = false;
	result->vfr->isValid = true;
	result->start = 0;
	result->end = Frag->Length;

	// A. Cannot violate maximum length
	result = this->ReviseFragment_MaxLength(result, F);

	// B. Check to ensure [ ] symbols are balanced
	result = this->ReviseFragment_BranchingMinFragmentsOnly(result, F);

	// C. Check for impossible turtle commands, e.g. ++- cannot exist in a single successor
	// D. Check for impossible turtle commands mid fragment, split the fragment, keep the head (note a tail fragment is reversed so still keep the 'head')
	result = this->ReviseFragment_TurtlesHeadTailFragmentsOnly(result, F);

	// E. Cannot violate maximum growth
	result = this->ReviseFragment_MaxGrowth(result, F);

	return result;
}


void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 ForSymbol, Int32 Min)
{
	for (size_t iGen = 0; iGen < this->model->generations; iGen++)
	{
		this->SetFact_MinGrowthOnly(iGen, ForSymbol, Min);
	}
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 iGen, Int32 ForSymbol, Int32 Min)
{
	Int32 iTable = 0;
	this->SetFact_MinGrowthOnly(iGen, iTable, ForSymbol, Min);
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 ForSymbol, Int32 Min)
{
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		this->SetFact_MinGrowthOnly(iGen, iTable, iSymbol, ForSymbol, Min);
	}
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 Min)
{
	for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle(); iLeft++)
	{
		for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle(); iRight++)
		{
			this->SetFact_MinGrowthOnly(iGen, iTable, iSymbol, iLeft, iRight, ForSymbol, Min);
		}
	}
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 iLeft, Int32 iRight, Int32 Min)
{
	this->SetFact_MinGrowthOnly(iGen, iTable, iSymbol, iLeft, iRight, nullptr, ForSymbol, Min);
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 ForSymbol, Int32 Min)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
	this->SetFact_MinGrowthOnly(f, ForSymbol, Min);
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 ForSymbol, Int32 Max)
{
	for (size_t iGen = 0; iGen < this->model->generations; iGen++)
	{
		this->SetFact_MaxGrowthOnly(iGen, ForSymbol, Max);
	}
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 iGen, Int32 ForSymbol, Int32 Max)
{
	Int32 iTable = 0;
	this->SetFact_MaxGrowthOnly(iGen, iTable, ForSymbol, Max);
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 ForSymbol, Int32 Max)
{
	for (size_t iSymbol = 0; iSymbol < this->model->alphabet->SizeNonTurtle(); iSymbol++)
	{
		this->SetFact_MaxGrowthOnly(iGen, iTable, iSymbol, ForSymbol, Max);
	}
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 Max)
{
	for (size_t iLeft = 0; iLeft < this->model->alphabet->SizeNonTurtle(); iLeft++)
	{
		for (size_t iRight = 0; iRight < this->model->alphabet->SizeNonTurtle(); iRight++)
		{
			this->SetFact_MaxGrowthOnly(iGen, iTable, iSymbol, iLeft, iRight, ForSymbol, Max);
		}
	}
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 iLeft, Int32 iRight, Int32 Max)
{
	this->SetFact_MaxGrowthOnly(iGen, iTable, iSymbol, iLeft, iRight, nullptr, ForSymbol, Max);
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 ForSymbol, Int32 Max)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
	this->SetFact_MaxGrowthOnly(f, ForSymbol, Max);
}

void MasterAnalysisObject::SetFact_MinGrowthOnly(Fact^ F, array<Int32>^ PV)
{
	for (size_t i = 0; i < PV->GetUpperBound(0); i++)
	{
		this->SetFact_MinGrowthOnly(F, i, PV[i]);
	}
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Fact^ F, array<Int32>^ PV)
{
	for (size_t i = 0; i < PV->GetUpperBound(0); i++)
	{
		this->SetFact_MaxGrowthOnly(F, i, PV[i]);
	}
}


void MasterAnalysisObject::SetFact_MinGrowthOnly(Fact^ F, Int32 ForSymbol, Int32 Min)
{
	if (!this->IsNoFact(F))
	{
		if (Min > F->growth[ForSymbol, 0])
		{
			F->growth[ForSymbol, 0] = Min;
			this->change = true;
		}

		if (Min > F->growth[ForSymbol, 1])
		{
			Console::WriteLine("Min value > Max value");
			F->growth[ForSymbol, 1] = Min;
			this->change = true;
		}
	}
}

void MasterAnalysisObject::SetFact_MaxGrowthOnly(Fact^ F, Int32 ForSymbol, Int32 Max)
{
	if (!this->IsNoFact(F))
	{
		if (Max < F->growth[ForSymbol, 1])
		{
			F->growth[ForSymbol, 1] = Max;
			this->change = true;
		}

		if (Max < F->growth[ForSymbol, 0])
		{
			Console::WriteLine("Max value < Min value");
			F->growth[ForSymbol, 0] = Max;
			this->change = true;
		}
	}
}

void MasterAnalysisObject::SetFact_MinLengthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 Min)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
	this->SetFact_MinLengthOnly(f, Min);
}

void MasterAnalysisObject::SetFact_MaxLengthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 Max)
{
	Fact^ f = this->GetFact(iGen, iTable, iSymbol, iLeft, iRight, V);
	this->SetFact_MaxLengthOnly(f, Max);
}

void MasterAnalysisObject::SetFact_MinLengthOnly(Fact^ F, Int32 Min)
{
	if (Min > F->length->min)
	{
		F->length->min = Min;
		this->change = true;

		// i. Update max fragments. Remove any fragment that is shorter than the min length
		List<Fragment^>^ toTabu = gcnew List<Fragment^>;
		for (size_t iFragment = 0; iFragment < F->max->Count; iFragment++)
		{
			if (F->max[iFragment]->fragment->Length < F->length->min)
			{
				toTabu->Add(F->max[iFragment]);
			}
		}
	}

	if (Min > F->length->max)
	{
		Console::WriteLine("Min value > Max value");
		F->length->max = Min;
		this->change = true;
	}
}

void MasterAnalysisObject::SetFact_MaxLengthOnly(Fact^ F, Int32 Max)
{
	if (Max < F->length->max)
	{
		F->length->max = Max;
		this->change = true;

		// i. Update head/tail fragments
		if ((F->head->max->fragment != nullptr) && (F->head->max->fragment->Length > F->length->max))
		{
			F->head->max->fragment = F->head->max->fragment->Substring(0, F->length->max);
		}

		if ((F->tail->max->fragment != nullptr) && (F->tail->max->fragment->Length > F->length->max))
		{
			F->tail->max->fragment = F->tail->max->fragment->Substring(F->tail->max->fragment->Length - F->length->max, F->length->max);
		}
	}

	if (Max < F->length->min)
	{
		Console::WriteLine("Max value < Min value");
		F->length->min = Max;
		this->change = true;
	}
}

void MasterAnalysisObject::SetLocalization(Int32 iGen, Int32 iPos1, Int32 iPos2, Byte V)
{
	if (V == cLocalization_Never)
	{
		if (this->localization[iGen][iPos1, iPos2] != cLocalization_Locked)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
		else
		{
			Console::WriteLine("Trying to never a localization marked as locked");
		}
	}
	else if (V == cLocalization_Unset)
	{
		if ((this->localization[iGen][iPos1, iPos2] != cLocalization_Locked) && (this->localization[iGen][iPos1, iPos2] != cLocalization_Never))
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
		else
		{
			Console::WriteLine("Trying to unset a localization marked as locked or never");
		}
	}
	else if (V == cLocalization_WeakBTF)
	{
		if (this->localization[iGen][iPos1, iPos2] == cLocalization_Unset)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
	}
	else if (V == cLocalization_Weak)
	{
		if (this->localization[iGen][iPos1,iPos2] == cLocalization_StrongBTF)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
	}
	else if (V == cLocalization_StrongBTF)
	{
		if (this->localization[iGen][iPos1, iPos2] == cLocalization_WeakBTF)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
	}
	else if (V == cLocalization_Strong)
	{
		if (this->localization[iGen][iPos1, iPos2] == cLocalization_Weak)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
	}
	else if (V == cLocalization_Locked)
	{
		if (this->localization[iGen][iPos1, iPos2] != cLocalization_Never)
		{
			this->localization[iGen][iPos1, iPos2] = V;
		}
		else
		{
			Console::WriteLine("Trying to lock a localization that is marked never");
		}
	}
}

void MasterAnalysisObject::SetLocalization_Direct(Int32 iGen, Int32 iPos1, Int32 iPos2, Byte V)
{
	if (this->localization[iGen][iPos1, iPos2] != cLocalization_Never)
	{
		this->localization[iGen][iPos1, iPos2] = V;
	}
}