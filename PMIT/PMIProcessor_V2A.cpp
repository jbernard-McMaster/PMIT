#include "stdafx.h"
#include "PMIProcessor_V2A.h"


PMIProcessor_V2A::PMIProcessor_V2A() : PMIProcessor()
{
}


PMIProcessor_V2A::~PMIProcessor_V2A()
{
}

bool PMIProcessor_V2A::AlgorithmMenu()
{
	bool redisplay = true;

	try
	{
		String^ choice;

		do
		{
			if (redisplay)
			{
				Console::WriteLine("Algorithm Menu - Phase 2A");

				//Console::WriteLine("0 : Evidence Generator (not implemented)");
				Console::WriteLine("1 : PMIT - PoG v1");
				Console::WriteLine("1b : PMIT - PoG v1b");
				Console::WriteLine("2 : PMIT - PoG v2");
				Console::WriteLine("3 : PMIT - PoL");
				Console::WriteLine("3b : PMIT - PoL v1b");
				Console::WriteLine("4 : PMIT - Length v2");
				Console::WriteLine("5 : PMIT - Lexicon v1");
				Console::WriteLine("5b : PMIT - Lexicon v1b");
				Console::WriteLine("6 : PMIT - Direct Encoding v4");
				Console::WriteLine("7 : PMIT - M+L");
				Console::WriteLine("8a : PMIT - Direct Encoding v1");
				Console::WriteLine("8b : PMIT - Direct Encoding v2");
				Console::WriteLine("8c : PMIT - Direct Encoding v3");
				//Console::WriteLine("3 : PMIT - Length v1");
				//Console::WriteLine("5 : PMIT - Direct Encoding v1");
				//Console::WriteLine("6 : PMIT - Direct Encoding v2");
				//Console::WriteLine("8 : PMIT - Math v1");
				//Console::WriteLine("9 : PMIT - Math v2");
				//Console::WriteLine("10 : Error Correction - ECC (doens't work)");
				Console::WriteLine("a : Analyzer");
				//Console::WriteLine("e : Error Rate:" + this->errorRate.ToString(L"F3"));
				Console::WriteLine("o : Options");
				Console::WriteLine("q : Quit");
				redisplay = false;
			}

			Console::Write(">>>");
			choice = Console::ReadLine();
			if (choice == "0")
			{
				this->algorithmChoice = PMIProcessor::AlgorithmChoices::EvidenceGenerator;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "1")
			{
				this->pmitType = PMIProcessor::PMITType::PoG_v1;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "1b")
			{
				this->pmitType = PMIProcessor::PMITType::PoG_v1b;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "2")
			{
				this->pmitType = PMIProcessor::PMITType::PoG_v2;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "3")
			{
				this->pmitType = PMIProcessor::PMITType::PoL_v1;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "3b")
			{
				this->pmitType = PMIProcessor::PMITType::PoL_v1b;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "4")
			{
				this->pmitType = PMIProcessor::PMITType::Length_v2;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "5")
			{
				this->pmitType = PMIProcessor::PMITType::Lex_v1;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "5b")
			{
				this->pmitType = PMIProcessor::PMITType::Lex_v1b;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "6")
			{
				this->pmitType = PMIProcessor::PMITType::Directv4;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "7")
			{
				this->pmitType = PMIProcessor::PMITType::ML;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "8a")
			{
				this->pmitType = PMIProcessor::PMITType::Directv1;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "8b")
			{
				this->pmitType = PMIProcessor::PMITType::Directv2;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			else if (choice == "8c")
			{
				this->pmitType = PMIProcessor::PMITType::Directv3;
				this->FitnessFunctionMenu();
				redisplay = true;
			}
			//else if (choice == "8")
			//{
			//	this->pmitType = PMIProcessor::PMITType::Mathv1;
			//	this->FitnessFunctionMenu();
			//	redisplay = true;
			//}
			//else if (choice == "9")
			//{
			//	this->pmitType = PMIProcessor::PMITType::Mathv2;
			//	this->FitnessFunctionMenu();
			//	redisplay = true;
			//}
			//else if (choice == "10")
			//{
			//	this->pmitType = PMIProcessor::PMITType::ErrorCorrection;
			//	this->FitnessFunctionMenu();
			//	redisplay = true;
			//}
			else if (choice == "a")
			{
				this->pmitType = PMIProcessor::PMITType::Analyzer;
				this->ProblemMenu();
				redisplay = true;
			}
			//else if (choice == "e")
			//{
			//	Console::WriteLine("Enter new error rate:");
			//	float::TryParse(Console::ReadLine(), this->errorRate);
			//	redisplay = true;
			//}
			else if (choice == "o")
			{
				this->OptionsMenu();
				redisplay = true;
			}
			else if (choice == "b")
			{
				//Console::WriteLine("*** WARNING ***");
				//Console::WriteLine("This may take a long time and will overwrite the existing table.");
				//Console::WriteLine("Do you wish to continue? (y/n)");
				//choice = Console::ReadLine();

				//if (choice == "y")
				//{
				//	this->BuildAnalyticsTable();
				//}
			}
		} while (choice != "q");

		return true;
	}
	catch (ProductionFailedException^ e)
	{
		Console::WriteLine(e->Message);
	}
}

bool PMIProcessor_V2A::FitnessFunctionMenu()
{
	String^ choice;

	do
	{
		Console::WriteLine("Fitness Function Menu");
		Console::WriteLine("1: Parikh");
		Console::WriteLine("2: Positional");
		Console::WriteLine("3: Rule Consistency");
		Console::WriteLine("4: Rule/Symbol Consistency");
		Console::WriteLine("5: Assembled (Stochastic Problems Only?)");
		Console::WriteLine("q : back");

		Int32 pick = 0;

		do
		{
			Console::Write(">>>");
			choice = Console::ReadLine();

			if (choice != "q")
			{
				try
				{
					pick = System::Convert::ToInt32(choice);
				}
				catch (FormatException ^e) {
					pick = -1;
				}
			}
		} while (((pick > 5) || (pick < 1)) && (choice != "q"));

		if (choice != "q")
		{
			switch (pick)
			{
			case 1:
				this->fitnessFunctionChoice = PMIProcessor_V2A::FitnessFunctionChoices::Parikh;
				this->fitnessFunction = gcnew PMIFitnessFunction_Parikh();
				break;
			case 2:
				this->fitnessFunctionChoice = PMIProcessor_V2A::FitnessFunctionChoices::Positional;
				this->fitnessFunction = gcnew PMIFitnessFunction_Positional();
				break;
			case 3:
				this->fitnessFunctionChoice = PMIProcessor_V2A::FitnessFunctionChoices::RuleConsistency;
				break;
			case 4:
				this->fitnessFunctionChoice = PMIProcessor_V2A::FitnessFunctionChoices::RuleSymbolConsistency;
				break;
			case 5:
				this->fitnessFunctionChoice = PMIProcessor_V2A::FitnessFunctionChoices::Assembled;
				break;
			default:
				break;
			}

			this->ProblemMenu();
		}
	} while (choice != "q");

	return true;
}

void PMIProcessor_V2A::Execute(Int32 iExecution)
{
	switch (this->pmitType)
	{
	case PMIProcessor::PMITType::PoG_v1:
		this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::PoG_v1b:
		this->problem = gcnew PMIProblemV2A_PoG_v1b(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::PoG_v2:
		this->problem = gcnew PMIProblemV2A_PoG_v2(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::PoL_v1:
		this->problem = gcnew PMIProblemV2A_PoL_v1(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::PoL_v1b:
		this->problem = gcnew PMIProblemV2A_PoL_v1(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Length_v2:
		this->problem = gcnew PMIProblemV2A_L_v2(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Lex_v1:
		this->problem = gcnew PMIProblemV2A_Lex_v1(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Lex_v1b:
		this->problem = gcnew PMIProblemV2A_Lex_v1b(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv1:
		this->problem = gcnew PMIProblemV1E_D_v1(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv2:
		this->problem = gcnew PMIProblemV1E_D_v2(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv3:
		this->problem = gcnew PMIProblemV2A_D_v3(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv4:
		this->problem = gcnew PMIProblemV2A_D_v4(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::ML:
		this->problem = gcnew PMIProblemV2A_ML(this->model, this->errorRate, this->problemFile);
		this->problem->fitnessFunction = this->fitnessFunction;
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Analyzer:
		this->problem = gcnew PMIProblemV2A_PoG_v1(this->model, this->errorRate, this->problemFile);
		this->problem->WriteAnalysis();
		break;
	default:
		break;
	}

	// Ignore the first one because the timings are off due to code optimization
	if (iExecution > 0)
	{
		metrics->timingsAnalysis->Add(this->problem->analysisTime);
		metrics->timingsSolve->Add(this->problem->solveTime);
		metrics->solved->Add(this->problem->solved);
		metrics->matched->Add(this->problem->matched);
		metrics->solutionSpaceSizes = this->problem->solutionSpaceSizes;

		metrics->ComputeOverallPerformance();
		metrics->Display();
	}
}
