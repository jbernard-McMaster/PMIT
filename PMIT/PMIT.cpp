// PMIT.cpp : main project file.

#include "stdafx.h"
#include "PMIProcessor.h"
#include "PMIProcessor_V2A.h"
#include "PMIProcessorV3.h"

using namespace System;
using namespace System::Collections::Generic;

int main(array<System::String ^> ^args)
{
	Console::OutputEncoding = System::Text::Encoding::UTF8;
	Console::SetWindowSize(160, 48);

	CommonGlobals::Pool = gcnew RandomPool(1000000, 100);
	System::Threading::Thread^ poolThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(CommonGlobals::Pool, &RandomPool::Start));
	poolThread->Start();

	do
	{
		System::Threading::Thread::Sleep(5);
	} while (!CommonGlobals::Pool->Live());

	//Console::WriteLine("Processor Menu");
	//Console::WriteLine("1. PMI Processor Phase 1");
	//Console::WriteLine("2. PMI Processor Phase 2");
	//Console::WriteLine("3. PMI Processor Phase 3");

	int pick = 2;

	PMIProcessor^ processor;
	PMIProcessorV3^ processor3 = gcnew PMIProcessorV3;
	//PMIProcessor_V2A^ processorV2A = gcnew PMIProcessor_V2A;
	bool quitting = false;

	switch (pick)
	{
	case 1:
		processor = gcnew PMIProcessor();
		break;
	case 2:
		processor = gcnew PMIProcessor_V2A();
		break;
	case 3:
		/*
		Console::WriteLine("Main Menu");
		Console::WriteLine("=========");
		Console::WriteLine("1: PMIT-S0L v4.0");
		Console::WriteLine("2: PMIT-S0L v4.0 - M Sequence");
		Console::WriteLine("3: PMIT-D2L v1.0");
		Console::WriteLine("4: PMIT-D2L v2.0");
		Console::WriteLine("5: PMIT-HD0L (non-erasing)");
		Console::WriteLine("6: PMIT-D2L v1.0 (D0L ED&C experiment)");
		Console::WriteLine("7: PMIT-M+L (LiK experiment)");
		Console::WriteLine("8: Analyze");
		Console::WriteLine("9: Generate Strings");

		String^ choice;
		Console::Write(">>>");
		choice = Console::ReadLine();
		if (choice == "1")
		{
			processor3->ProcessS0L();
		}
		else if (choice == "2")
		{
			processor3->ProcessS0LM();
		}
		else if (choice == "3")
		{
			processor3->ProcessD2L();
		}
		else if (choice == "4")
		{
			processor3->Process();
		}
		else if (choice == "5")
		{
			processor3->ProcessHD0L();
		}
		else if (choice == "6")
		{
			processor3->ProcessD0LEDC();
		}
		else if (choice == "7")
		{
			processor3->ProcessLiK();
		}
		else if (choice == "8")
		{
			processor3->ProcessEquations();
		}
		else if (choice == "9")
		{
			processor3->GenerateStrings();
		}

		//else if (choice == "4")
		//{
		//	processor3->ProcessWolframs_NSymbol(3);
		//}
		//else if (choice == "5")
		//{
		//	processor3->ProcessWolframs_NSymbol(4);
		//}
		//else if (choice == "6")
		//{
		//	processor3->ProcessWolframs_NSymbol(5);
		//}

		quitting = true;
		*/
		break;
	default:
		break;
	}

	while (!quitting)
	{
		quitting = processor->AlgorithmMenu();
	}


	CommonGlobals::Pool->Stop();
	do
	{
		System::Threading::Thread::Sleep(CommonGlobals::Pool->sleepTime);
	} while (CommonGlobals::Pool->Live());

	//Console::WriteLine("Press enter to exit");
	//Console::ReadLine();

    return 0;
}
