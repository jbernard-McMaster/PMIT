#pragma once
#include "ConsistencyItem.h"

using namespace System;
using namespace System::Collections::Generic;

public ref class ConsistencyTable
{
public:
	ConsistencyTable(Int32 Size);
	virtual ~ConsistencyTable();

	array<List<ConsistencyItem^>^>^ items;

	void Add(Int32 Index, ConsistencyItem^ I);

	void Display();
};

