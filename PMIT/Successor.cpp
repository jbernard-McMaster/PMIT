#include "stdafx.h"
#include "Successor.h"


Successor::Successor()
{
	this->symbols = gcnew List<Symbol^>;
	this->chance = 1.00;
}

Successor::Successor(List<Symbol^>^ Symbols)
{
	this->symbols = Symbols;
}

Successor::Successor(List<Symbol^>^ Symbols, float Chance)
{
	this->symbols = Symbols;
	this->chance = Chance;
}


Successor::~Successor()
{
}
