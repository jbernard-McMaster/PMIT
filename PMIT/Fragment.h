#pragma once

using namespace System;
using namespace System::Collections::Generic;

ref class Fragment;
ref class FragmentSigned;

public ref struct MinMax {
	Int32 min;
	Int32 max;
};

public ref struct MinMaxFragment {
	Fragment^ min;
	Fragment^ max;
};

public ref struct SAC
{
	Int32 iSymbol;
	Int32 iLeft;
	Int32 iRight;
};

public ref struct SACX
{
	Int32 iGen;
	Int32 iTable;
	Int32 iSymbol;
	Int32 iLeft;
	Int32 iRight;
};

public ref struct Fact {
	SAC^ sac;
	Int32 ID;
	String^ condition;
	array<Int32, 2>^ growth;
	MinMax^ length;
	MinMaxFragment^ head;
	MinMaxFragment^ tail;
	Fragment^ mid;
	Fragment^ complete;
	List<FragmentSigned^>^ max;
	List<FragmentSigned^>^ btf;
	List<FragmentSigned^>^ cache;
	List<String^>^ tabu;
};

public ref class Fragment
{
public:
	Fragment();
	Fragment(String^ Frag, bool IsTrue);
	virtual ~Fragment();

	String^ fragment;
	bool isComplete;
	bool isTrue;
};

public ref class FragmentSigned :
	public Fragment
{
public:
	FragmentSigned();
	FragmentSigned(Fragment^ Fr, String^ Left, String^ Right);
	virtual ~FragmentSigned();

	void ComputeSACCount(String^ S);

	void Update(String^ Frag, Fact^ F);

	Int32 sacCount;
	//List<FragmentSigned^>^ associated;
	List<FragmentSigned^>^ uncertain;
	String^ leftSignature;
	String^ rightSignature;
};

