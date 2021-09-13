#include "stdafx.h"
#include "Symbol.h"


Symbol::Symbol(String^ X)
{
	this->value = X;
	this->parent = this;
}

Symbol::Symbol(String^ X, Symbol^ P) : Symbol(X)
{
	this->parent = P;
}

Symbol::Symbol(String^ X, Symbol^ P, Int32 Abstraction) : Symbol(X,P)
{
	this->abstraction = Abstraction;
}

Symbol::Symbol(String^ X, Int32 Abstraction, Int32 VocabularyIndex) : Symbol(X)
{	
	this->abstraction = Abstraction;
	this->vocabularyIndex = VocabularyIndex;
}

Symbol::Symbol(String^ X, Int32 Abstraction, Int32 VocabularyIndex, bool IsModule) : Symbol(X, Abstraction, VocabularyIndex)
{
	this->abstraction = Abstraction;
	this->vocabularyIndex = VocabularyIndex;
	this->isModule = IsModule;
}


Symbol::~Symbol()
{
}

void Symbol::Output()
{
	Console::Write(this->value);
}
