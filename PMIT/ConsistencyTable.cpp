#include "stdafx.h"
#include "ConsistencyTable.h"


ConsistencyTable::ConsistencyTable(Int32 Size)
{
	this->items = gcnew array<List<ConsistencyItem^>^>(Size);
	for (size_t iList = 0; iList <= this->items->GetUpperBound(0); iList++)
	{
		this->items[iList] = gcnew List<ConsistencyItem^>;
	}
}

ConsistencyTable::~ConsistencyTable()
{
}

void ConsistencyTable::Add(Int32 Index, ConsistencyItem^ I)
{
	// check to see if the item already exists
	Int32 iItem = 0;
	bool found = false;
	List<ConsistencyItem^>^ itemList = this->items[Index];

	// check to see if the item already exists
	// if it does increment the count
	while ((iItem < this->items[Index]->Count) && (!found))
	{
		// check to see if predecessor matches
		if (itemList[iItem]->rule->predecessor == I->rule->predecessor)
		{
			bool match = true;
			// check to see if the successor matches
			for (size_t iSymbol = 0; iSymbol < I->rule->successor->Count; iSymbol++)
			{
				match = match && (I->rule->successor[iSymbol] == itemList[iItem]->rule->successor[iSymbol]);
			}

			if (match)
			{
				found = true;
				itemList[iItem]->count++;
			}
		}
		iItem++;
	};

	// Otherwise add it to the list
	if (!found)
	{
		itemList->Add(I);
	}
}

void ConsistencyTable::Display()
{
	for (size_t iList = 0; iList <= this->items->GetUpperBound(0); iList++)
	{
		List<ConsistencyItem^>^ itemList = this->items[iList];

		for (size_t iItem = 0; iItem < itemList->Count; iItem++)
		{
			Console::WriteLine(itemList[iItem]->rule->ToString() + ": " + itemList[iItem]->count);
		}
	}
}