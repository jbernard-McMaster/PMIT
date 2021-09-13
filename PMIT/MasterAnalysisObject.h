#pragma once

#include "defines.h"
#include "ModelV3.h"

using namespace System;
using namespace System::Collections::Generic;

public ref struct GrowthFact
{
	Int32 iGen;
	Int32 iSymbol;
	String^ predecessor;
	String^ successor;
	bool isMin;
	Int32 value;
};

public ref struct LengthFact
{
	Int32 iGen;
	Int32 iSymbol;
	String^ predecessor;
	bool isMin;
	Int32 value;
};

public ref struct FragmentFact
{
	String^ fragment;
	bool isHead;
	bool isTail;
	bool isMin;
	bool isComplete;
};

public ref struct SymbolFact
{
	bool isKnown;
	bool isProducer;
	bool isGraphical;
	bool isTerminal;
};

//public ref struct FragmentSigned {
//	Fragment^ fr;
//	String^ signature;
//};

public ref struct TotalLengthFact
{
	List<Fact^>^ facts;
	MinMax^ length;
};

public ref struct FactCount {
	Int32 iGen;
	Int32 iTable;
	Int32 iSymbol;
	Int32 iLeft;
	Int32 iRight;
	Int32 count;
	Fact^ fact;
};

public ref struct ValidatedFragment
{
	Fragment^ fr;
	bool isValid;
};

public ref struct ValidatedFragmentX
{
	ValidatedFragment^ vfr;
	Int32 start; // these are adjustments to the start and end of the original fragment
	Int32 end;
};

// indicates if the fragment was successfully revised and should be saved along with the new fragment
public ref struct RevisedFragment
{
	bool isRevised;
	Fragment^ fr;
	Int32 start;
	Int32 end;
};

// sorts from longest to shortest
public ref class FragmentLengthComparer : IComparer<Fragment^>
{
public:
	virtual int Compare(Fragment^ x, Fragment^ y)
	{
		if (x->fragment == nullptr)
		{
			if (y->fragment == nullptr)
			{
				// If x is null and y is null, they're
				// equal. 
				return 0;
			}
			else
			{
				// If x is null and y is not null, y
				// is greater. 
				return -1;
			}
		}
		else
		{
			// If x is not null...
			//
			if (y->fragment == nullptr)
				// ...and y is null, x is greater.
			{
				return 1;
			}
			else
			{
				// ...and y is not null, compare the 
				// lengths of the two strings.
				//
				int retval = -1 * x->fragment->Length.CompareTo(y->fragment->Length);

				if (retval != 0)
				{
					// If the strings are not of equal length,
					// the longer string is greater.
					//
					return retval;
				}
				else
				{
					// If the strings are of equal length,
					// sort them with ordinary string comparison.
					//
					return x->fragment->CompareTo(y->fragment);
				}
			}
		}
	}
};

// sorts from longest to shortest
public ref class BTFLengthComparer : IComparer<FragmentSigned^>
{
public:
	virtual int Compare(FragmentSigned^ x, FragmentSigned^ y)
	{
		if (x->fragment == nullptr)
		{
			if (y->fragment == nullptr)
			{
				// If x is null and y is null, they're
				// equal. 
				return 0;
			}
			else
			{
				// If x is null and y is not null, y
				// is greater. 
				return -1;
			}
		}
		else
		{
			// If x is not null...
			//
			if (y->fragment == nullptr)
				// ...and y is null, x is greater.
			{
				return 1;
			}
			else
			{
				// ...and y is not null, compare the 
				// lengths of the two strings.
				//
				int retval = -1 * x->fragment->Length.CompareTo(y->fragment->Length);

				if (retval != 0)
				{
					// If the strings are not of equal length,
					// the longer string is greater.
					//
					return retval;
				}
				else
				{
					// If the strings are of equal length,
					// sort them with ordinary string comparison.
					//
					return x->fragment->CompareTo(y->fragment);
				}
			}
		}
	}
};

public ref struct SuccessorCount
{
	String^ successor;
	Int32 count;
	double odds;
};

// sorts from longest to shortest
public ref class SuccessorOddsComparer : IComparer<SuccessorCount^>
{
public:
	virtual int Compare(SuccessorCount^ x, SuccessorCount^ y)
	{
		return -1 * x->odds.CompareTo(y->odds);
	}
};

public ref class MasterAnalysisObject
{
public:
	MasterAnalysisObject(ModelV3^ M);
	virtual ~MasterAnalysisObject();

	Int32 nextID;
	Int32 nothingID;

	array<bool, 2>^ symbolsByGeneration; // keeping this public because it doesn't change once set, so no real need to put it behind a getter/setter
	array<List<SACX^>^>^ sacsByGeneration; // same as above
	List<TotalLengthFact^>^ partialTotalLengthFacts;
	List<SACX^>^ sacs; // same as above
	array<array<Byte, 2>^>^ localization;

	// this function will take a MAO and adjust the internal MAO accordingly
	// the idea is this, a symbol LSI problem will return a solution and a MAO
	// the next symbol problem will upload the MAO so that the facts can be moved on
	// however the generation LSI problem will not be modified, it represents the master MAO for that subproblem
	// when a generation LSI problem returns, it will also return a solution and a MAO
	// the master LSI problem will upload the MAO because it is guaranteed correct
	// then when it creates the next generation problem it will be based on the master problem's MAO
	void UploadMAO(MasterAnalysisObject^ MAO); 
	void Display();
	void Write();
	void DisplayFact(Fact^ F);
	void WriteFact(Fact^ F, System::IO::StreamWriter^ SW);
	void DisplaySolutionSpaces();

	String^ Reverse(String^ S) {
		array<wchar_t>^ tmp = S->ToCharArray();
		Array::Reverse(tmp);
		return gcnew String(tmp);
	}

	/* FACTS METHODS */

	array<Int32>^ ConvertSymbolInWordToFactTable(Int32 iGen, WordXV3^ W, Int32 iPos);
	void ComputeSACsByGeneration(array<WordXV3^>^ E);
	void ComputeSymbolsByGeneration(array<WordXV3^>^ E);
	//Fragment^ CreateFragment(String^ S, Int32 Start, Int32 End, bool IsComplete) { Fragment^ fr = gcnew Fragment(); fr->fragment = S; fr->start = Start; fr->end = End; fr->isComplete = IsComplete; return fr; }
	//Fragment^ CreateFragment(String^ S, bool IsComplete, bool IsTrue) { Fragment^ fr = gcnew Fragment(); fr->fragment = S; fr->isComplete = IsComplete; fr->isTrue = IsTrue;  return fr; }
	Fragment^ CreateFragment(String^ S, bool IsComplete, bool IsTrue) { Fragment^ fr = gcnew Fragment(); fr->isComplete = IsComplete;  fr->fragment = S; fr->isTrue = IsTrue;  return fr; }
	FragmentSigned^ CreateSignedFragment(String^ S, bool IsComplete, bool IsTrue, String^ Left, String^ Right) { FragmentSigned^ fr = gcnew FragmentSigned(); fr->fragment = S; fr->isComplete = IsComplete;  fr->isTrue = IsTrue;  fr->leftSignature = Left; fr->rightSignature = Right; return fr; }
	FragmentSigned^ CreateBTF(Fragment^ Fr, String^ Left, String^ Right) { FragmentSigned^ result = gcnew FragmentSigned(); result->fragment = Fr->fragment; result->isTrue = Fr->isTrue; result->leftSignature = Left; result->rightSignature = Right;  return result; }

	bool IsFragmentComplete(String^ Min, String^ Max) { return (Min == Max); }
	bool IsFragmentRevisable(Fragment^ Fr) { return (Fr->fragment != nullptr); }
	bool IsFragmentTabu(Fragment^ Fr, Fact^ F) { return F->tabu->Contains(Fr->fragment); }
	bool IsFactHasMaxFragment(FragmentSigned^ Fr, Fact^ F) { return this->IsSignedFragmentInList(Fr, F->max);}
	bool IsFragmentInList(Fragment^ Fr, List<Fragment^>^ L) {
		Int32 idx = 0; bool found = false;
		while ((!found) && (idx < L->Count))
		{
			if (L[idx]->fragment == Fr->fragment)
			{
				found = true;
			}
			idx++;
		}
		return found;
	}
	bool IsSignedFragmentInList(FragmentSigned^ Fr, List<FragmentSigned^>^ L) {
		Int32 idx = 0; bool found = false;
		while ((!found) && (idx < L->Count))
		{
			if (L[idx]->fragment == Fr->fragment && L[idx]->leftSignature == Fr->leftSignature && L[idx]->rightSignature == Fr->rightSignature)
			{
				found = true;
			}
			idx++;
		}
		return found;
	}

	bool IsBTFInList(FragmentSigned^ Fr, List<FragmentSigned^>^ L) {
		Int32 idx = 0; bool found = false;
		while ((!found) && (idx < L->Count))
		{
			if (L[idx]->fragment == Fr->fragment && L[idx]->leftSignature == Fr->leftSignature && L[idx]->rightSignature == Fr->rightSignature)
			{
				found = true;
			}
			idx++;
		}
		return found;
	}
	bool IsBTFInList(Fragment^ Fr, List<FragmentSigned^>^ L) {
		Int32 idx = 0; bool found = false;
		while ((!found) && (idx < L->Count))
		{
			if (L[idx]->fragment == Fr->fragment)
			{
				found = true;
			}
			idx++;
		}
		return found;
	}
	bool IsSignatureCompatible(String^ L1, String^ R1, String^ L2, String^ R2);


	void ComputeSolutionSpaceSize();
	void ComputeSolutionSpaceSize_Absolute();
	void ComputeSolutionSpaceSize_Length();
	void ComputeSolutionSpaceSize_PoL();
	void ComputeSolutionSpaceSize_PoG();

	UInt64 solutionSpaceSize_Absolute = 1;
	UInt64 solutionSpaceSize_Length = 1;
	UInt64 solutionSpaceSize_PoL = 1;
	UInt64 solutionSpaceSize_Growth = 1;
	UInt64 solutionSpaceSize_Direct = 0;

	ValidatedFragmentX^ CreateVFRX(Fragment^ Fr);
	ValidatedFragmentX^ CreateVFRX(String^ Frag);
	ValidatedFragment^ CreateVFR(Fragment^ Fr);
	ValidatedFragment^ CreateVFR(String^ Frag);

	List<ValidatedFragment^>^ ReviseMaxFragment(Fragment^ Frag, Fact^ F);
	List<ValidatedFragment^>^ ReviseMaxFragment_Correct(Fragment^ Frag, Fact^ F);
	List<ValidatedFragment^>^ ReviseMaxFragmentWithCorrections(Fragment^ Frag, Fact^ F);
	ValidatedFragment^ ValidateFragment(Fragment^ Frag, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_HeadTailFragmentOnly(String^ Frag, Fact^ F);

	ValidatedFragmentX^ ReviseFragment_BranchingMinFragmentsOnly(Fragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_BranchingMinFragmentsOnly(ValidatedFragmentX^ Vfr, Fact^ F);
	List<ValidatedFragment^>^ ReviseFragment_BranchingMaxFragmentsOnly(Fragment^ Fr, Fact^ F);
	List<ValidatedFragment^>^ ReviseFragment_BranchingMaxFragmentsOnly(ValidatedFragment^ Vfr, Fact^ F);

	ValidatedFragmentX^ ReviseFragment_Turtles(Fragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_Turtles(ValidatedFragmentX^ Vfr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_TurtlesHeadTailFragmentsOnly(Fragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_TurtlesHeadTailFragmentsOnly(ValidatedFragmentX^ Vfr, Fact^ F);
	List<ValidatedFragment^>^ ReviseFragment_TurtlesMaxFragmentsOnly(Fragment^ Fr, Fact^ F);
	List<ValidatedFragment^>^ ReviseFragment_TurtlesMaxFragmentsOnly(ValidatedFragment^ Vfr, Fact^ F);

	ValidatedFragment^ ReviseFragment_HeadFragmentMaxFragmentsOnly(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ReviseFragment_HeadFragmentMaxFragmentsOnly(ValidatedFragment^ Vfr, Fact^ F);

	ValidatedFragment^ ReviseFragment_TailFragmentMaxFragmentsOnly(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ReviseFragment_TailFragmentMaxFragmentsOnly(ValidatedFragment^ Vfr, Fact^ F);
	
	bool ValidateFragment_MaxLength(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ValidateFragment_MaxLength(ValidatedFragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_MaxLength(Fragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_MaxLength(ValidatedFragmentX^ Vfr, Fact^ F);

	bool ValidateFragment_MinLength(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ValidateFragment_MinLength(ValidatedFragment^ Fr, Fact^ F);

	bool ValidateFragment_MaxGrowth(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ValidateFragment_MaxGrowth(ValidatedFragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_MaxGrowth(Fragment^ Fr, Fact^ F);
	ValidatedFragmentX^ ReviseFragment_MaxGrowth(ValidatedFragmentX^ Vfr, Fact^ F);

	bool ValidateFragment_MinGrowth(Fragment^ Fr, Fact^ F);
	ValidatedFragment^ ValidateFragment_MinGrowth(ValidatedFragment^ Fr, Fact^ F);

	bool ValidateFragment_MaxHead(Fragment^ Fr, Fact^ F);
	bool ValidateFragment_MinHead(Fragment^ Fr, Fact^ F);

	bool ValidateFragment_MaxTail(Fragment^ Fr, Fact^ F);
	bool ValidateFragment_MinTail(Fragment^ Fr, Fact^ F);


	bool IsNoFact(Fact^ F) { return (F->condition == this->NoFact->condition && F->ID == this->NoFact->ID); }
	
	//// TODO: modify this so it is only added if unique
	//void AddFragmentFact(FragmentFact^ F) { this->fragmentFacts->Add(F); }
	//void AddGrowthFact(GrowthFact^ F) { this->growthFacts->Add(F); }
	//void AddLengthFact(LengthFact^ F) { this->lengthFacts->Add(F); }
	//void SetSymbolFact(Int32 iSymbol, SymbolFact^ F) { this->symbolFacts[iSymbol] = F; }

	void AddFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F);
	void CopyFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 fromLeft, Int32 fromRight, Int32 toLeft, Int32 toRight, Fact^ F);

	List<String^>^ GetSubwords(String^ S);
	List<String^>^ GetSubwordsOfLength(WordXV3^ W, Int32 Length);
	List<String^>^ GetSubwordsOfLength(String^ S, Int32 Length);

	bool IsFullyCovered(Int32 Length, List<Range^>^ Covered);
	bool IsSubword(String^ Fr1, String^ Fr2);
	bool IsCommonSubword(Fragment^ Fr1, List<FragmentSigned^>^ L);
	String^ FindLongestSubword(String^ Fr1, String^ Fr2);
	List<Fragment^>^ Compute_MaxFragmentsFromPairedFragments(Fragment^ Fr1, Fragment^ Fr2, Fact^ F);

	void SetFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F);

	void UpdateFact_MinFragment(Fact^ F, Fragment^ Fr);
	void UpdateFact_MaxFragment(Fact^ F, Fragment^ Fr);
	void UpdateFact_BTF(Fact^ F, Fragment^ Fr);
	void UpdateFact_MaxFragments(Fact^ F);
	void UpdateFact_CompleteFragment(Fact^ F, Fragment^ Fr);

	void SetFact_CompleteFragmentOnly(Fact^ F, Fragment^ Fr);
	void SetFact_CompleteFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_CompleteFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MinHeadFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MinHeadFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MaxHeadFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MaxHeadFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MinTailFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MinTailFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MaxTailFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MaxTailFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MidFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MidFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MaxFragmentOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_MaxFragmentOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	void SetFact_MaxFragmentOnly(Fact^ F, Fragment^ Fr);
	void SetFact_MaxFragmentOnly_Direct(Fact^ F, FragmentSigned^ Fr);
	//void SetFact_BTFOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, Fragment^ Fr);
	void SetFact_BTFOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, FragmentSigned^ Fr);
	//void SetFact_BTFOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Fragment^ Fr);
	void SetFact_BTFOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, FragmentSigned^ Fr);
	void SetFact_BTFOnly(Fact^ F, FragmentSigned^ Fr);
	void SetFact_BTFOnly_Direct(Fact^ F, FragmentSigned^ Fr);
	void SetFact_BTFOnly_UncertainOnly(Fact^ F, Int32 iBTF, FragmentSigned^ Fr);
	void SetFact_BTFOnly_Update(Fact^ F, FragmentSigned^ Fr);
	void SetFact_BTFOnly_Update(FragmentSigned^ Fr1, FragmentSigned^ Fr2, Fact^ F);
	void SetFact_CacheOnly(Int32 iGen, WordXV3^ W1, Int32 iPos, FragmentSigned^ Fr, WordXV3^ W2, Int32 Start, Int32 End);
	void SetFact_CacheOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, FragmentSigned^ Fr, WordXV3^ W, Int32 Start, Int32 End);
	String^ Compute_PrefixOverlap(String^ Frag1, String^ Frag2);
	String^ Compute_SuffixOverlap(String^ Frag1, String^ Frag2);

	void RemoveFact_ResetCache(Fact^ F);
	void RemoveFact_MaxFragmentOnly(Fact^ F, List<Fragment^>^ Fr, bool IsTabu);
	void RemoveFact_MaxFragmentOnly(Fact^ F, Fragment^ Fr, bool IsTabu);

	void SetFact_MinGrowthOnly(Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Fact^ F, array<Int32>^ PV);
	void SetFact_MaxGrowthOnly(Fact^ F, array<Int32>^ PV);
	void SetFact_MinGrowthOnly(Fact^ F, Int32 ForSymbol, Int32 Max);
	void SetFact_MaxGrowthOnly(Fact^ F, Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Int32 iGen, Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 iGen, Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Int32 ForSymbol, Int32 Max);
	void SetFact_MinGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 ForSymbol, Int32 Min);
	void SetFact_MaxGrowthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 ForSymbol, Int32 Max);
	
	void SetFact_MinLengthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 Min);
	void SetFact_MaxLengthOnly(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V, Int32 Max);
	void SetFact_MinLengthOnly(Fact^ F, Int32 Min);
	void SetFact_MaxLengthOnly(Fact^ F, Int32 Max);

	static const Byte cLocalization_Locked = 6;
	static const Byte cLocalization_Strong = 5;
	static const Byte cLocalization_StrongBTF = 4;
	static const Byte cLocalization_Weak = 3;
	static const Byte cLocalization_WeakBTF = 2;
	static const Byte cLocalization_Unset = 1;
	static const Byte cLocalization_Never = 0;

	void SetLocalization(Int32 iGen, Int32 iPos1, Int32 iPos2, Byte V);
	void SetLocalization_Direct(Int32 iGen, Int32 iPos1, Int32 iPos2, Byte V);

	// these functions convert one fragment into another, e.g. max into a mid
	RevisedFragment^ ConvertMax2MidFragment_MinGrowth(Fragment^ Fr, Fact^ F);

	// these functions revise existing fragments
	RevisedFragment^ ReviseFragment_UnderMinGrowth(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End);
	RevisedFragment^ ReviseFragment_UnderMinGrowth_HeadFragmentOnly(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End);
	RevisedFragment^ ReviseFragment_UnderMinGrowth_TailFragmentOnly(Fragment^ Fr, Fact^ F, WordXV3^ W, Int32 Start, Int32 End);

	static Int32 ComputeError(String^ S1, String^ S2);

	//void SetFactMaster(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, Fact^ F);

	SACX^ GetSAC(Fact^ F);
	List<SACX^>^ GetSACs(Int32 iGen);
	List<SACX^>^ GetAppearingSACs(Int32 From, Int32 To);
	List<SACX^>^ GetDisappearingSACs(Int32 From, Int32 To);

	Fact^ GetFact(Int32 iGen, WordXV3^ W, Int32 iPos);
	Fact^ GetFact(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V);
	Fact^ GetFact(SACX^ S);
	Int32 GetFactIndex(Fact^ F) { 
		Int32 iFact = 0;  
		while (iFact < this->factsList->Count)
		{
			if (F == this->factsList[iFact])
			{
				return iFact;
			}
			iFact++;
		}
		return -1;
	}
	MinMax^ GetTotalLengthAllSuccessors(Int32 iGen) { return this->totalLengthSuccessors[iGen];	}
	//MinMax^ GetTotalLengthLongestSuccessors(Int32 iGen) { return this->totalLengthLongestSuccessors[iGen]; };
	//MinMax^ GetTotalLengthShortestSuccessors(Int32 iGen) { return this->totalLengthShortestSuccessors[iGen]; };

	//Fact^ GetFactMaster(Int32 iGen, Int32 iTable, Int32 iSymbol, Int32 iLeft, Int32 iRight, List<float>^ V);

	//List<FragmentFact^>^ GetFragmentFacts() { return this->fragmentFacts; }
	//List<GrowthFact^>^ GetGrowthFacts() { return this->growthFacts; }
	//List<LengthFact^>^ GetLengthFacts() { return this->lengthFacts; }
	//SymbolFact^ GetSymbolFact(Int32 iSymbol) { return this->symbolFacts[iSymbol]; }

	/* TOTAL LENGTH METHODS */

	void SetTotalLengthSuccessors(Int32 iGen, MinMax^ MM) {
		this->SetTotalLengthSuccessors_MinOnly(iGen, MM->min);
		this->SetTotalLengthSuccessors_MaxOnly(iGen, MM->max);
	};

	void SetTotalLengthSuccessors_MinOnly(Int32 iGen, Int32 V) {
		if (V > this->totalLengthSuccessors[iGen]->min)
		{
			this->totalLengthSuccessors[iGen]->min = V;
		}
	};

	void SetTotalLengthSuccessors_MaxOnly(Int32 iGen, Int32 V) {
		if (V < this->totalLengthSuccessors[iGen]->max)
		{
			this->totalLengthSuccessors[iGen]->max = V;
		}
	};

	//void SetTotalLengthLongestSuccessors(Int32 iGen, MinMax^ MM) {
	//	this->SetTotalLengthLongestSuccessors_MinOnly(iGen, MM->min);
	//	this->SetTotalLengthLongestSuccessors_MaxOnly(iGen, MM->max);
	//};

	//void SetTotalLengthLongestSuccessors_MinOnly(Int32 iGen, Int32 V) {
	//	if (V > this->totalLengthLongestSuccessors[iGen]->min)
	//	{
	//		this->totalLengthLongestSuccessors[iGen]->min = V;
	//	}
	//};

	//void SetTotalLengthLongestSuccessors_MaxOnly(Int32 iGen, Int32 V) {
	//	if (V < this->totalLengthLongestSuccessors[iGen]->max)
	//	{
	//		this->totalLengthLongestSuccessors[iGen]->max = V;
	//	}
	//};

	//void SetTotalLengthShortestSuccessors(Int32 iGen, MinMax^ MM) {
	//	this->SetTotalLengthShortestSuccessors_MinOnly(iGen, MM->min);
	//	this->SetTotalLengthShortestSuccessors_MaxOnly(iGen, MM->max);
	//};

	//void SetTotalLengthShortestSuccessors_MinOnly(Int32 iGen, Int32 V) {
	//	if (V > this->totalLengthShortestSuccessors[iGen]->min)
	//	{
	//		this->totalLengthShortestSuccessors[iGen]->min = V;
	//	}
	//};

	//void SetTotalLengthShortestSuccessors_MaxOnly(Int32 iGen, Int32 V) {
	//	if (V < this->totalLengthShortestSuccessors[iGen]->max)
	//	{
	//		this->totalLengthShortestSuccessors[iGen]->max = V;
	//	}
	//};

	/* SYMBOL LEXICON METHODS */

	Int32 GetSymbolRightLexicon(Int32 iGen, Int32 iSymbol, Int32 iNeighbour) {
		return this->symbolRightLexicon[iGen, iSymbol, iNeighbour];
	}

	Int32 GetDoubleSymbolRightLexicon(Int32 iGen, Int32 iSymbol1, Int32 iSymbol2, Int32 iNeighbour) {
		return this->symbolDoubleRightLexicon[iGen, iSymbol1, iSymbol2, iNeighbour];
	}

	Int32 GetSymbolLeftLexicon(Int32 iGen, Int32 iSymbol, Int32 iNeighbour) {
		return this->symbolLeftLexicon[iGen, iSymbol, iNeighbour];
	}

	Int32 GetDoubleSymbolLeftLexicon(Int32 iGen, Int32 iSymbol1, Int32 iSymbol2, Int32 iNeighbour) {
		return this->symbolDoubleLeftLexicon[iGen, iSymbol1, iSymbol2, iNeighbour];
	}

	Int32 GetSymbolLeftRightLexicon(Int32 iGen, Int32 iSymbol, Int32 iLeft, Int32 iRight) {
		return this->symbolLeftRightLexicon[iGen, iSymbol, iLeft, iRight];
	}

	void SetSymbolLexicons(array<Int32, 3>^ R, array<Int32, 4>^ DR, array<Int32, 3>^ L, array<Int32, 4>^ DL, array<Int32, 4>^ LR)
	{
		this->symbolRightLexicon = R;
		this->symbolDoubleRightLexicon = DR;
		this->symbolLeftLexicon = L;
		this->symbolDoubleLeftLexicon = DL;
		this->symbolLeftRightLexicon = LR;
	}

	String^ SymbolCondition(Int32 iSymbol, Int32 iLeft, Int32 iRight) {
		String^ result = "";

		if ((iLeft == this->nothingID) || (iLeft == -1))
		{
			result = "* < ";
		}
		else
		{
			result = this->model->alphabet->symbols[iLeft] + " < ";
		}

		result += this->model->alphabet->symbols[iSymbol];

		if ((iRight == this->nothingID) || (iRight == -1))
		{
			result += " > *";
		}
		else
		{
			result += " > "  + this->model->alphabet->symbols[iRight];
		}

		return result;
	}

	String^ SymbolCondition(Int32 iGen, WordXV3^ W, Int32 iPos) {
		String^ result = "";
		Int32 iTable = 0;
		Int32 iSymbol = W->GetSymbolIndex(iPos);
		Int32 iLeft = W->GetLeftContext(iPos);
		Int32 iRight = W->GetRightContext(iPos);

		if ((iLeft == this->nothingID) || (iLeft == -1))
		{
			result = "* < ";
		}
		else
		{
			result = this->model->alphabet->symbols[iLeft] + " < ";
		}

		result += this->model->alphabet->symbols[iSymbol];

		if ((iRight == this->nothingID) || (iRight == -1))
		{
			result += " > *";
		}
		else
		{
			result += " > " + this->model->alphabet->symbols[iRight];
		}

		return result;
	}


	void ResetChangeFlag()
	{
		this->change = false;
	}

	void ResetBTFChangeFlag()
	{
		this->changeBTF = false;
	}

	bool HasChangeOccured()
	{
		return this->change;
	}

	bool HasBTFChangeOccured()
	{
		return this->changeBTF;
	}

	const Int32 cUnknown = -1;

	const Int32 cDim_Gen = 0;
	const Int32 cDim_Table = 1;
	const Int32 cDim_Symbol = 2;
	const Int32 cDim_Left = 3;
	const Int32 cDim_Right = 4;

	static const Int32 NoFactID = -1;

	List<Fact^>^ factsList;

private:
	ModelV3^ model;
	array<Fact^>^ turtleFacts;
	Fact^ NoFact;
	bool change;
	bool changeBTF;

	array<bool>^ knownSymbols;

	array<MinMax^>^ totalLengthSuccessors;

	array<Int32, 3>^ symbolRightLexicon;
	array<Int32, 4>^ symbolDoubleRightLexicon;
	array<Int32, 3>^ symbolLeftLexicon;
	array<Int32, 4>^ symbolDoubleLeftLexicon;
	array<Int32, 4>^ symbolLeftRightLexicon;

	//// These are the raw facts about the system made during analysis
	//// These are compressed down into the fact matrix above
	//List<GrowthFact^>^ growthFacts;
	//List<LengthFact^>^ lengthFacts;
	//List<FragmentFact^>^ fragmentFacts;
	//array<SymbolFact^>^ symbolFacts;

	// These are the compressed fact matrices
	// Growth/Length/Fragment Facts indicies: Generation, Table, Symbol, Left Context, Right Context. Condition is embedded in the Fact structure.
	array<List<Fact^>^, 5>^ facts;
	//array<List<Fact^>^, 5>^ factsMaster;

	Fact^ FindFact(List<Fact^>^ F, List<float>^ V);
};