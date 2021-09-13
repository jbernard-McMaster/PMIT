#pragma once
#include "Symbol.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class Successor
{
public:
	Successor();
	Successor(List<Symbol^>^ Symbols);
	Successor(List<Symbol^>^ Symbols, float Chance);
	virtual ~Successor();

	List<Symbol^>^ symbols;
	float chance;
};

