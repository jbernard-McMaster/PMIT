#include "stdafx.h"
#include "Fragment.h"

Fragment::Fragment()
{
}

Fragment::Fragment(String^ Frag, bool IsTrue)
{
	this->fragment = Frag;
	//this->isComplete = IsComplete;
	this->isTrue = IsTrue;
}

Fragment::~Fragment()
{
}


FragmentSigned::FragmentSigned() : Fragment()
{
	this->leftSignature = "";
	this->rightSignature = "";
	//this->associated = gcnew List<FragmentSigned^>;
	this->uncertain = gcnew List<FragmentSigned^>;
	this->sacCount = 0;
}

FragmentSigned::FragmentSigned(Fragment^ Fr, String^ Left, String^ Right) : Fragment(Fr->fragment, Fr->isTrue)
{
	this->leftSignature = Left;
	this->rightSignature = Right;
	//this->associated = gcnew List<FragmentSigned^>;
	this->uncertain = gcnew List<FragmentSigned^>;
	this->sacCount = 0;
}

FragmentSigned::~FragmentSigned()
{
}

void FragmentSigned::ComputeSACCount(String^ S)
{
	Int32 index;
	this->sacCount = 0;
	index = this->leftSignature->IndexOf(S);
	while (index != -1)
	{
		this->sacCount++;
		index = this->leftSignature->IndexOf(S, index);
	}

	index = this->rightSignature->IndexOf(S);
	while (index != -1)
	{
		this->sacCount++;
		index = this->rightSignature->IndexOf(S, index);
	}
}

void FragmentSigned::Update(String^ Frag, Fact^ F)
{
	this->fragment = Frag;
}