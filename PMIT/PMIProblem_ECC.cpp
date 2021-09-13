#include "stdafx.h"
#include "PMIProblem_ECC.h"


PMIProblem_ECC::PMIProblem_ECC(PlantModel^ Model) : PMIProblem(Model)
{
}


PMIProblem_ECC::~PMIProblem_ECC()
{
}

void PMIProblem_ECC::Solve()
{

#ifdef _PMI_PROBLEM_VERBOSE >= 3
	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}
#endif

	//for (size_t iInterleave = 2; iInterleave < 16; iInterleave++)
	//{
	//	Console::WriteLine("Interleave: " + iInterleave);
	//	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	//	{
	//		Int32 iSymbol = 0;

	//		while (iSymbol < this->model->evidence[iGen]->symbols->Count)
	//		{
	//			Console::Write(this->model->evidence[iGen]->symbols[iSymbol]->ToString());
	//			iSymbol += iInterleave;
	//		}
	//		Console::WriteLine();
	//	}
	//}	

	for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
	{
		Console::WriteLine(this->model->evidence[iGen]->ToString());
	}

	for (size_t iVocab = 0; iVocab < this->vocabulary->Symbols->Count; iVocab++)
	{
		for (size_t iGen = 0; iGen < this->model->evidence->Count; iGen++)
		{
			Console::Write(this->vocabulary->Symbols[iVocab]->ToString() + "(" + iGen + "): ");
			Int32 count = 0;
			for (size_t iSymbol = 0; iSymbol < this->model->evidence[iGen]->symbols->Count; iSymbol++)
			{
				if (this->model->evidence[iGen]->symbols[iSymbol] == this->vocabulary->Symbols[iVocab])
				{
					Console::Write(count + ",");
				}
				count++;
			}
			Console::WriteLine();
		}
	}

}
