#include "stdafx.h"
#include "PMIProcessor.h"

using namespace System::IO;

PMIProcessor::PMIProcessor()
{
	this->LoadConfiguration();
	this->errorRate = 0.0;
}

PMIProcessor::~PMIProcessor()
{
}

void PMIProcessor::EvidenceGenerator()
{
	this->model->CreateEvidence();

	Console::WriteLine("Model");
	Console::WriteLine(" Axiom = " + this->model->axiom->ToString());
	Console::WriteLine(" Rules: ");

	for each (ProductionRule^ r in this->model->rules)
	{
		Console::WriteLine("   " + r->ToString());
	}

	Console::WriteLine();
}

bool PMIProcessor::OptionsMenu()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : Executions = " + this->numExecutions);
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
		} while (((pick > 0) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			switch (pick)
			{
				// set the number of executions
			case 0:
				Console::WriteLine("Enter the desired number of executions");
				Console::WriteLine(">>>");
				choice = Console::ReadLine();

				try
				{
					Int32 value;
					value = System::Convert::ToInt32(choice);
					this->numExecutions = value;
				}
				catch (FormatException ^e)
				{
					Console::WriteLine("Not a number");
				}

				break;
			default:
				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu()
{
	String^ choice;

	do
	{
		Console::WriteLine("Problem Menu");
		Console::WriteLine("1: D0L systems - Known Constants");
		Console::WriteLine("2: D0L systems - Limited Constants");
		Console::WriteLine("3: Stochastic");
		Console::WriteLine("4: Contextual");
		Console::WriteLine("5: Parameterized");
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
				this->ProblemMenu_D0L();
				break;
			case 2:
				this->ProblemMenu_D0L_LimitedConstants();
				break;
			case 3:
				this->ProblemMenu_Stochastic();
				break;
			case 4:
				this->ProblemMenu_Contextual();
				break;
			case 5:
				this->ProblemMenu_Parameterized();
				break;
			default:
				break;
			}
		}
	} while (choice != "q");

	return true;
}

void PMIProcessor::Execute(Int32 iExecution)
{
	switch (this->pmitType)
	{
	case PMIProcessor::PMITType::PoG_v1:
		this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::PoL_v1:
		this->problem = gcnew PMIProblem_PoL(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Length_v1:
		this->problem = gcnew PMIProblem_LengthV1(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Length_v2:
		this->problem = gcnew PMIProblem_LengthV2(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv1:
		this->problem = gcnew PMIProblem_Direct(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv2:
		this->problem = gcnew PMIProblem_Directv2(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Directv3:
		this->problem = gcnew PMIProblem_Directv3(this->model, this->errorRate, this->problemFile);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Mathv1:
		this->problem = gcnew PMIProblem_Math(this->model);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::Mathv2:
		this->problem = gcnew PMIProblem_Mathv2(this->model);
		this->problem->Solve();
		break;
	case PMIProcessor::PMITType::ErrorCorrection:
		this->problem = gcnew PMIProblem_ECC(this->model);
		this->problem->Solve();
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

		metrics->ComputeOverallPerformance();
		metrics->Display();
	}
}

bool PMIProcessor::ProblemMenu_D0L()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "0*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0  : random");
		Console::WriteLine("99 : DFS special");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while ((((pick > file->Length) && (pick != 99)) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			if (pick == 99)
			{
				this->problemFile = "special";
			}
			else if (pick > 0)
			{
				this->problemFile = file[pick - 1];
			}
			else
			{
				this->problemFile = "random";
			}

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				if (this->problemFile == "special")
				{
					Int32 numRules = 0;
					Int32 minLength = 0;
					Int32 maxLength = 0;
					/*
					do
					{
						Console::Write("Enter a number of rules>>>");
						choice = Console::ReadLine();

						try
						{
							numRules = System::Convert::ToInt32(choice);
						}
						catch (FormatException ^e) {
							numRules = -1;
						}
					} while (numRules < 0);

					do
					{
						Console::Write("Enter a min length >>>");
						choice = Console::ReadLine();

						try
						{
							minLength = System::Convert::ToInt32(choice);
						}
						catch (FormatException ^e) {
							minLength = -1;
						}
					} while (minLength < 0);

					do
					{
						Console::Write("Enter a max length >>>");
						choice = Console::ReadLine();

						try
						{
							maxLength = System::Convert::ToInt32(choice);
						}
						catch (FormatException ^e) {
							maxLength = -1;
						}
					} while (maxLength < 0);
					*/

					minLength = 1;
					maxLength = 10;
					this->numExecutions = 1;

					for (size_t iRulesMax = 2; iRulesMax < 100; iRulesMax++)
					{
						for (size_t j = 0; j < 100; j++)
						{
							this->model = gcnew PlantModel();
							do
							{
								PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
								generator->Load();
								this->model = generator->BuildD0L_Special(iRulesMax, maxLength);
								this->model->CreateEvidence();
							} while (!this->model->Validate());
							this->model->Save();
						}
					}

					List<float>^ results = gcnew List<float>;
					Int32 maxRules = 111;

					for (size_t numRules = maxRules; numRules <= maxRules; numRules++)
					{
						this->metrics = gcnew PMIPerformanceMetrics();

						for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
						{
							do
							{
								if ((this->problemFile != "random") && (this->problemFile != "special"))
								{
									this->model = gcnew PlantModel();
									this->model->Load(this->problemFile);
								}
								else if (this->problemFile == "special")
								{
									PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
									generator->Load();
									this->model = generator->BuildD0L_Special(numRules, maxLength);
								}
								else
								{
									PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
									generator->Load();
									this->model = generator->BuildD0L();
								}

								this->model->CreateEvidence();
							} while (!this->model->Validate());

							this->Execute(iExecution);
							Console::WriteLine("#" + iExecution + ", |V| = " + (numRules)+", MTTS(Analysis) = " + this->metrics->MTTS_Analysis +", MTTS(Solve) = " + this->metrics->MTTS_Solve);
						}

						if (this->numExecutions > 0)
						{
							this->metrics->ComputeOverallPerformance();
							results->Add(this->metrics->MTTS_Analysis - this->metrics->MTTS_Solve);
							this->metrics->Display();
						}
						
						Console::WriteLine("|V| = " + (numRules)+", MTTS = " + results[numRules-2]);
						Console::WriteLine("Press enter to continue");
						Console::ReadLine();
					}
				}
				else
				{
					for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
					{
						if ((this->problemFile != "random") && (this->problemFile != "special"))
						{
							this->model = gcnew PlantModel();
							this->model->Load(this->problemFile);
						}
						else if (this->problemFile == "special")
						{
							if (iExecution == 0)
							{
								Int32 numRules = 0;
								Int32 maxLength = 0;

								do
								{
									Console::Write("Enter a number of rules>>>");
									choice = Console::ReadLine();

									try
									{
										numRules = System::Convert::ToInt32(choice);
									}
									catch (FormatException ^e) {
										numRules = -1;
									}
								} while (numRules < 0);

								do
								{
									Console::Write("Enter a max length >>>");
									choice = Console::ReadLine();

									try
									{
										maxLength = System::Convert::ToInt32(choice);
									}
									catch (FormatException ^e) {
										maxLength = -1;
									}
								} while (maxLength < 0);

								PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
								generator->Load();
								this->model = generator->BuildD0L_Special(numRules, maxLength);
							}
						}
						else
						{
							if (iExecution == 0)
							{
								PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
								generator->Load();
								this->model = generator->BuildD0L();
							}
						}
						this->model->CreateEvidence();
						this->Execute(iExecution);
					}

					if (this->numExecutions > 0)
					{
						this->metrics->ComputeOverallPerformance();
						this->metrics->Display();
					}
				}

				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu_D0L_LimitedConstants()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "1*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : random (not implemented)");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			if (pick > 0)
			{
				this->problemFile = file[pick - 1];
			}
			else
			{
				this->problemFile = "random";
			}

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
				{
					if (this->problemFile != "random")
					{
						this->model = gcnew PlantModel();
						this->model->Load(this->problemFile);
					}
					else
					{
						PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
						generator->Load();
						this->model = generator->BuildD0L();
					}

					this->model->CreateEvidence();
					this->Execute(iExecution);
				}

				if (this->numExecutions > 0)
				{
					this->metrics->ComputeOverallPerformance();
					this->metrics->Display();
				}

				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu_Stochastic()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "1*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : random");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			if (pick > 0)
			{
				this->problemFile = file[pick - 1];
			}
			else
			{
				this->problemFile = "random";
			}

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
				{
					if (this->problemFile != "random")
					{
						this->model = gcnew PlantModelStochastic();
						this->model->Load(this->problemFile);
					}
					else
					{
						PMIRandomModelGenerator^ generator = gcnew PMIRandomModelGenerator();
						generator->Load();
						this->model = generator->BuildD0L();
					}

					this->model->CreateEvidence();
					this->Execute(iExecution);
				}

				if (this->numExecutions > 0)
				{
					this->metrics->ComputeOverallPerformance();
					this->metrics->Display();
				}
				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu_ChangeOverTime()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "2*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : random (not implemented)");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			this->problemFile = file[pick - 1];

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
				{
					this->model = gcnew PlantModel();
					this->model->Load(this->problemFile);
					this->model->CreateEvidence();
					this->Execute(iExecution);
				}

				if (this->numExecutions > 0)
				{
					this->metrics->ComputeOverallPerformance();
					this->metrics->Display();
				}

				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu_Contextual()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "3*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0  : random ");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			this->problemFile = file[pick - 1];

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
				{
					this->model = gcnew PlantModel();
					this->model->Load(this->problemFile);
					this->model->CreateEvidence();
					this->Execute(iExecution);
				}

				if (this->numExecutions > 0)
				{
					this->metrics->ComputeOverallPerformance();
					this->metrics->Display();
				}

				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::ProblemMenu_Parameterized()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "4*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : random (not implemented)");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			this->problemFile = file[pick - 1];

			switch (this->algorithmChoice)
			{
			case PMIProcessor::AlgorithmChoices::Analyzer:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->WriteAnalysis();
				break;
			default:
				this->metrics = gcnew PMIPerformanceMetrics();

				for (size_t iExecution = 0; iExecution < this->numExecutions + 1; iExecution++)
				{
					this->model = gcnew PlantModel();
					this->model->Load(this->problemFile);
					this->model->CreateEvidence();
					this->Execute(iExecution);
				}

				if (this->numExecutions > 0)
				{
					this->metrics->ComputeOverallPerformance();
					this->metrics->Display();
				}

				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::AnalysisMenu()
{
	String^ choice;

	do
	{
		String^ folder = "./";
		bool result = false;

		array<String^>^ file = Directory::GetFiles(folder, "*problem*.txt");

		Console::WriteLine("Problem Menu");
		Console::WriteLine("0 : random (not implemented)");

		for (int i = 0; i < file->Length; i++)
		{
			file[i] = file[i]->Replace("./", "");

			Console::WriteLine((i + 1).ToString(L"G") + " : " + file[i]);
		}

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
		} while (((pick > file->Length) || (pick < 0)) && (choice != "q"));

		if (choice != "q")
		{
			this->problemFile = file[pick - 1];
			this->model = gcnew PlantModel();
			this->model->Load(this->problemFile);
			this->model->CreateEvidence();

			//switch (this->algorithmChoice)
			//{
			//	case PMIProcessor::AlgorithmChoices::EvidenceGenerator:
			//		this->EvidenceGenerator();
			//		break;
			//	case PMIProcessor::AlgorithmChoices::GeneticAlgorithm:
			//		this->problem = gcnew PMIProblem(this->model);
			//		this->problem->Solve();
			//		break;
			//	default:
			//		break;
			//}

			switch (this->pmitType)
			{
			case PMIProcessor::PMITType::PoG_v1:
				this->problem = gcnew PMIProblem_PoG(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::PoL_v1:
				this->problem = gcnew PMIProblem_PoL(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Length_v1:
				this->problem = gcnew PMIProblem_LengthV1(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Length_v2:
				this->problem = gcnew PMIProblem_LengthV2(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Directv1:
				this->problem = gcnew PMIProblem_Direct(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Directv2:
				this->problem = gcnew PMIProblem_Directv2(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Directv3:
				this->problem = gcnew PMIProblem_Directv3(this->model, this->errorRate, this->problemFile);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Mathv1:
				this->problem = gcnew PMIProblem_Math(this->model);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::Mathv2:
				this->problem = gcnew PMIProblem_Math(this->model);
				this->problem->Solve();
				break;
			case PMIProcessor::PMITType::ErrorCorrection:
				this->problem = gcnew PMIProblem_ECC(this->model);
				this->problem->Solve();
				break;
			default:
				break;
			}
		}
	} while (choice != "q");

	return true;
}

bool PMIProcessor::AlgorithmMenu()
{
	bool redisplay = true;

	try
	{
		String^ choice;

		do
		{
			if (redisplay)
			{
				Console::WriteLine("Algorithm Menu");

				Console::WriteLine("0 : Evidence Generator (not implemented)");
				Console::WriteLine("1 : PMIT - PoG");
				Console::WriteLine("2 : PMIT - PoL");
				Console::WriteLine("3 : PMIT - Length v1");
				Console::WriteLine("4 : PMIT - Length v2");
				Console::WriteLine("5 : PMIT - Direct Encoding v1");
				Console::WriteLine("6 : PMIT - Direct Encoding v2");
				Console::WriteLine("7 : PMIT - Direct Encoding v3");
				Console::WriteLine("8 : PMIT - Math v1");
				Console::WriteLine("9 : PMIT - Math v2");
				Console::WriteLine("10 : Error Correction - ECC (doens't work)");
				Console::WriteLine("a : Analyzer");
				Console::WriteLine("e : Error Rate:" + this->errorRate.ToString(L"F3"));
				Console::WriteLine("o : Options");
				Console::WriteLine("q : Quit");
				redisplay = false;
			}

			Console::Write(">>>");
			choice = Console::ReadLine();
			if (choice == "0")
			{
				this->algorithmChoice = PMIProcessor::AlgorithmChoices::EvidenceGenerator;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "1")
			{
				this->pmitType = PMIProcessor::PMITType::PoG_v1;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "2")
			{
				this->pmitType = PMIProcessor::PMITType::PoL_v1;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "3")
			{
				this->pmitType = PMIProcessor::PMITType::Length_v1;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "4")
			{
				this->pmitType = PMIProcessor::PMITType::Length_v2;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "5")
			{
				this->pmitType = PMIProcessor::PMITType::Directv1;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "6")
			{
				this->pmitType = PMIProcessor::PMITType::Directv2;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "7")
			{
				this->pmitType = PMIProcessor::PMITType::Directv3;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "8")
			{
				this->pmitType = PMIProcessor::PMITType::Mathv1;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "9")
			{
				this->pmitType = PMIProcessor::PMITType::Mathv2;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "10")
			{
				this->pmitType = PMIProcessor::PMITType::ErrorCorrection;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "a")
			{
				this->algorithmChoice = PMIProcessor::AlgorithmChoices::Analyzer;
				this->ProblemMenu();
				redisplay = true;
			}
			else if (choice == "e")
			{
				Console::WriteLine("Enter new error rate:");
				float::TryParse(Console::ReadLine(), this->errorRate);
				redisplay = true;
			}
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

//

void PMIProcessor::RecursiveFor(Int32 Level, Int32 Limit, List<Int32>^ Values, Int32 NumSymbols, System::IO::StreamWriter^ sw)
{
	if (Level == Limit)
	{
		array<int, 1>^ last = gcnew array<int, 1>(NumSymbols + 1);
		array<int, 1>^ current = gcnew array<int, 1>(NumSymbols + 1);
		array<String^, 1>^ outputs = gcnew array<String^, 1>(5);
		bool axiomEmpty = true;
		bool productionFailed = true;

		for (size_t i = 0; i < NumSymbols+1; i++)
		{
			last[i] = Values[i];

			if (i < NumSymbols)
			{
				axiomEmpty = axiomEmpty && (Values[i] == 0);
			}
		}

#if _PMI_PROCESSOR_VERBOSE_ >= 2
		Console::Write("Axiom: ");
		for (size_t i = 0; i < NumSymbols + 1; i++)
		{
			Console::Write(Values[i] + " ");
		}

		Console::WriteLine();

		for (size_t iRule = 0; iRule < NumSymbols; iRule++)
		{
			Console::Write("Rule #" + iRule + ":  0 => ");
			for (size_t iSymbol = 0; iSymbol < NumSymbols + 1; iSymbol++)
			{
				Console::Write(Values[((iRule + 1) * (NumSymbols + 1)) + iSymbol] + " ");
			}
		}
	
		Console::WriteLine();
#endif
		if (!axiomEmpty)
		{
			for (size_t iGen = 0; iGen < 5; iGen++)
			{
				for (size_t iSymbol = 0; iSymbol < NumSymbols + 1; iSymbol++)
				{
					for (size_t iRule = 0; iRule < NumSymbols; iRule++)
					{
						if (iSymbol < NumSymbols)
						{
							current[iSymbol] = last[iRule] * Values[((iRule + 1) * (NumSymbols + 1)) + iSymbol];
						}
						else
						{
							current[iSymbol] = last[iSymbol] + (last[iRule] * Values[((iRule + 1) * (NumSymbols + 1)) + iSymbol]);
						}
					}
				}

				for (size_t i = 0; i < current->Length; i++)
				{
					last[i] = current[i];

					if (i < current->Length - 1)
					{
						productionFailed = productionFailed && (current[i] == 0);
					}
				}

#if _PMI_PROCESSOR_VERBOSE_ >= 1
				Console::Write("Current: Gen" + iGen);

				for (size_t i = 0; i < current->Length; i++)
				{
					Console::Write(" " + current[i]);
				}

				Console::WriteLine();
#endif

				if (productionFailed)
				{
					break;
				}
				else
				{
					for (size_t i = 0; i < this->maximumNumSymbols + 1; i++)
					{
						if (i < current->Length - 1)
						{
							outputs[iGen] += current[i] + ",";
						}
						else if (i == this->maximumNumSymbols)
						{
							outputs[iGen] += current[current->Length - 1] + ",";
						}
						else
						{
							outputs[iGen] += "0,";
						}
					}
				}
			}
		}

		if ((!productionFailed) && (!axiomEmpty))
		{
#if _PMI_PROCESSOR_VERBOSE_ >= 1
			Console::Write("Values: ");

			for (size_t i = 0; i < Values->Count; i++)
			{
				Console::Write(" " + Values[i]);
			}

			Console::WriteLine();
#endif

			// Write out the results
			for (size_t i = 0; i < this->maximumNumSymbols + 1; i++)
			{
				sw->Write(outputs[i]);
			}

			int tmp = 0;

			// Write out the axiom
			for (size_t i = 0; i < this->maximumNumSymbols + 1; i++)
			{
				if (i < current->Length-1)
				{
					sw->Write(Values[tmp].ToString() + ",");
					tmp++;
				}
				else if (i == this->maximumNumSymbols)
				{
					sw->Write(Values[tmp].ToString() + ",");
					tmp++;
				}
				else
				{
					sw->Write(L"0,");
				}
			}

			// Write out the rules
			for (size_t iRule = 0; iRule < NumSymbols; iRule++)
			{
				for (size_t iSymbol = 0; iSymbol < NumSymbols; iSymbol++)
				{
					sw->Write(Values[((iRule + 1) * (NumSymbols + 1)) + iSymbol] + ",");
				}

				for (size_t i = NumSymbols; i < this->maximumNumSymbols; i++)
				{
					sw->Write("0,");
				}

				sw->Write(Values[((iRule + 1) * (NumSymbols + 1)) + NumSymbols] + ",");
			}

			for (size_t iRule = NumSymbols; iRule < this->maximumNumSymbols; iRule++)
			{
				for (size_t i = 0; i < this->maximumNumSymbols+1; i++)
				{
					sw->Write("0,");
				}
			}
			
			sw->WriteLine();
		}
	}
	else
	{
		for (size_t i = 0; i < this->maximumSymbolOccurance; i++)
		{
			Values[Level] = i;
			RecursiveFor(Level + 1, Limit, Values, NumSymbols, sw);
		}
	}
}

void PMIProcessor::LoadConfiguration()
{
	// Load Configuration File
	System::IO::StreamReader^ sr = gcnew System::IO::StreamReader("./configuration.txt");

	array<Char>^ seperator = gcnew array<Char>{','};
	ConfigFileLine lineNum = PMIProcessor::ConfigFileLine::NumExecutions;

	int valueI;
	float valueF;
	bool valueB;
	Int32 ruleNum = 0;

	while (!sr->EndOfStream)
	{
		String^ line = sr->ReadLine();

		if (line != "")
		{
			array<String^, 1>^ words;

			words = line->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);

			if ((int)lineNum == (int)PMIProcessor::ConfigFileLine::NumExecutions)
			{
				int::TryParse(words[1], valueI);
				this->numExecutions = valueI;
			}
		}
	}
}

/*

for (size_t iNumSymbols = 1; iNumSymbols <= 4; iNumSymbols++)
{
Int32 axiomIndex = 0;
array<Int32, 1>^ axiom = gcnew array<Int32, 1>(iNumSymbols + 1);

for (size_t i = 0; i < axiom->Length; i++)
{
axiom[i] = 0;
}

bool done = false;

do
{
array<int, 2>^ rules = gcnew array<int,2>(iNumSymbols, iNumSymbols+1);

for (size_t i = 0; i < iNumSymbols; i++)
{
for (size_t j = 0; j < iNumSymbols + 1; j++)
{
rules[i, j] = 0;
}
}

this->ProduceRule_Recursive(0, iNumSymbols, rules);

if (axiomIndex > 0)
{
axiomIndex = 0;
}

Console::Write("Axiom: ");

for (size_t i = 0; i < axiom->Length - 1; i++)
{
Console::Write(" S" + (i + 1) + ": " + axiom[i]);
}

Console::WriteLine(" C: " + axiom[axiom->Length - 1]);

} while (!done);
}

Console::WriteLine();

bool doneInner1 = false;
do
{
	bool setInner1 = true;
	do
	{
		bool doneInner2 = false;
		do
		{
			bool setInner2 = true;
			do
			{
				rules[index1, index2]++;
				setInner2 = true;

				if (rules[index1, index2] > 4)
				{
					rules[index1, index2] = 0;
					index2++;
					setInner2 = false;

					if (index2 > rules->GetUpperBound(1))
					{
						doneInner2 = true;
					}
				}
			} while ((!setInner2) && (!doneInner2));

			if (index2 > 0)
			{
				index2 = 0;
			}
		} while (!doneInner2);


		rules[index1, index2] = 0;
		index1++;

		if (index1 > rules->GetUpperBound(0))
		{
			doneInner1 = true;
		}
	} while (!doneInner1);
} while (!doneInner1);

bool set = true;
do
{
	axiom[axiomIndex]++;
	set = true;

	if (axiom[axiomIndex] > 4)
	{
		axiom[axiomIndex] = 0;
		axiomIndex++;
		set = false;

		if (axiomIndex >= axiom->Length)
		{
			done = true;
		}
	}
} while ((!set) && (!done));
*/

//void PMIProcessor::TestACOO()
//{
//	this->problem->CreateEvidence();
//
//	LayeredAntColonyGraph^ solutionSpace = this->problem->CreateGraph();
//	//this->problem->terminationCondition = gcnew TerminationCondition_MaxGenerations<AntColonyState^>(10000);
//
//	AntColonySystemConfiguration^ configuration = gcnew AntColonySystemConfiguration();
//	//configuration->generationsMax = 10000;
//	configuration->preferHighFitness = false;
//	configuration->populationSize = 10;
//
//	AntColonySystemO<Int32>^ ai = gcnew AntColonySystemO<Int32>(configuration);
//	//pso->problem = cipherProblem;
//	//pso->solutionSpace = solutionSpace;
//	//VectorOperatorInt^ vectorOperator = gcnew VectorOperatorInt();
//	//vectorOperator->accelerationGlobalBest = 0.50;
//	//vectorOperator->accelerationIndividualBest = 0.25;
//	//vectorOperator->inertia = 0.25;
//
//	//ParticleFactoryInt^ factory = gcnew ParticleFactoryInt(solutionSpace);
//	//factory->solutionSpace = solutionSpace;
//	//pso->factory = factory;
//	//pso->vectorOperator = vectorOperator;
//
//	array<Int32>^ solution = ai->Solve();
//
//	OutputModel(this->problem, solution);
//}
//
//void PMIProcessor::TestACOP()
//{
//	this->problem->CreateEvidence();
//	LayeredAntColonyGraph^ solutionSpace = this->problem->CreateGraph();
//
//	AntColonySystemConfiguration^ configuration = gcnew AntColonySystemConfiguration();
//	//configuration->generationsMax = 10000;
//	configuration->preferHighFitness = false;
//	configuration->populationSize = 10;
//
//	AntColonySystemO<Int32>^ ai = gcnew AntColonySystemO<Int32>(configuration);
//	//pso->problem = cipherProblem;
//	//pso->solutionSpace = solutionSpace;
//	//VectorOperatorInt^ vectorOperator = gcnew VectorOperatorInt();
//	//vectorOperator->accelerationGlobalBest = 0.50;
//	//vectorOperator->accelerationIndividualBest = 0.25;
//	//vectorOperator->inertia = 0.25;
//
//	//ParticleFactoryInt^ factory = gcnew ParticleFactoryInt(solutionSpace);
//	//factory->solutionSpace = solutionSpace;
//	//pso->factory = factory;
//	//pso->vectorOperator = vectorOperator;
//
//	array<Int32>^ solution = ai->Solve();
//	OutputModel(this->problem, solution);
//}
//
//
//void PMIProcessor::TestGAO()
//{
//	this->problem->CreateEvidence();
//
//	GenomeConfiguration<Int32>^ genomeConfig = this->problem->CreateGenome();
//	GeneticAlgorithmConfiguration<Int32>^ configuration = gcnew GeneticAlgorithmConfiguration<Int32>();
//	configuration->populationSizeInit = 10;
//	configuration->populationSizeMax = 10;
//	configuration->crossoverWeight = 0.80;
//	configuration->mutationWeight = 0.01;
//	configuration->preferHighFitness = false;
//
//	GeneticAlgorithmO<Int32>^ ai = gcnew GeneticAlgorithmO<Int32>(configuration);
//	ai->problem = this->problem;
//	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GeneticAlgorithmState^>(1000));
//	ai->terminationConditions->Add(gcnew TerminationCondition_SufficientFitness<GeneticAlgorithmState^>(0.0));
//
//	GenomeFactoryInt^ factory = gcnew GenomeFactoryInt(genomeConfig);
//	ai->factory = factory;
//
//	array<Int32>^ solution = ai->Solve();
//	OutputModel(this->problem, solution);
//}
//
//void PMIProcessor::TestPSOO()
//{
//	this->problem->CreateEvidence();
//	HypershapeInt^ solutionSpace = this->problem->CreateHypershape();
//
//	ParticleSwarmOptimizationConfiguration^ configuration = gcnew ParticleSwarmOptimizationConfiguration();
//	//configuration->generationsMax = 10000;
//	configuration->preferHighFitness = false;
//	configuration->populationSize = 10;
//
//	ParticleSwarmOptimizationO<Int32>^ ai = gcnew ParticleSwarmOptimizationO<Int32>(configuration);
//	ai->problem = this->problem;
//	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<ParticleSwarmOptimizationState^>(10000));
//	ai->solutionSpace = solutionSpace;
//	VectorOperatorInt^ vectorOperator = gcnew VectorOperatorInt();
//	vectorOperator->accelerationGlobalBest = 0.50;
//	vectorOperator->accelerationIndividualBest = 0.25;
//	vectorOperator->inertia = 0.25;
//
//	ParticleFactoryInt^ factory = gcnew ParticleFactoryInt(solutionSpace);
//	factory->solutionSpace = solutionSpace;
//	ai->factory = factory;
//	ai->vectorOperator = vectorOperator;
//
//	array<Int32>^ solution = ai->Solve();
//	OutputModel(this->problem, solution);;
//}
//
//void PMIProcessor::TestPSOP()
//{
//	this->problem->CreateEvidence();
//	HypershapeInt^ solutionSpace = this->problem->CreateHypershape();
//
//	ParticleSwarmOptimizationConfiguration^ configuration = gcnew ParticleSwarmOptimizationConfiguration();
//	//configuration->generationsMax = 10000;
//	configuration->preferHighFitness = false;
//	configuration->populationSize = 10;
//
//	ParticleSwarmOptimizationP<Int32>^ ai = gcnew ParticleSwarmOptimizationP<Int32>(configuration);
//	ai->problem = this->problem;
//	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<ParticleSwarmOptimizationState^>(10000));
//	ai->solutionSpace = solutionSpace;
//	VectorOperatorInt^ vectorOperator = gcnew VectorOperatorInt();
//	vectorOperator->accelerationGlobalBest = 0.50;
//	vectorOperator->accelerationIndividualBest = 0.25;
//	vectorOperator->inertia = 0.25;
//
//	ParticleFactoryInt^ factory = gcnew ParticleFactoryInt(solutionSpace);
//	factory->solutionSpace = solutionSpace;
//	ai->factory = factory;
//	ai->vectorOperator = vectorOperator;
//
//	array<Int32>^ solution = ai->Solve();
//	OutputModel(this->problem, solution);;
//}
//
//void PMIProcessor::TestGS()
//{
//	this->problem->CreateEvidence();
//
//	Int32 segmentSize = 20;
//
//	GridSearchConfiguration<Int32>^ configuration = gcnew GridSearchConfiguration<Int32>(this->problem->CreateSearchSpace(segmentSize));
//	configuration->preferHighFitness = false;
//
//	GridSearch<Int32>^ ai = gcnew GridSearch<Int32>(configuration);
//	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<GridSearchState^>(10000));
//	ai->problem = this->problem;
//
//	array<Int32>^ solution = ai->Solve();
//	OutputModel(this->problem, solution);;
//}
//
//void PMIProcessor::TestFSSRS()
//{
//	//this->problem->CreateEvidence();
//
//	//Int32 segmentSize = 1;
//
//	//RandomSearchConfiguration<float>^ configuration = gcnew RandomSearchConfiguration<float>();
//	//configuration->preferHighFitness = false;
//	//configuration->radius = 5;
//	////configuration->generationsMax = 10000;
//
//	//RandomSearch^ ai = gcnew RandomSearch(configuration, this->problem->CreateHypershape());
//	//ai->problem = this->problem;
//	//ai->terminationCondition = gcnew TerminationCondition_MaxGenerations<RandomSearchState^>(10000);
//
//	//array<float>^ solution = ai->Solve();
//
//	//OutputModel(this->problem, solution);;
//}
//
//void PMIProcessor::TestFSSDRS()
//{
//	this->problem->CreateEvidence();
//
//	Int32 segmentSize = 1;
//
//	RandomSearchConfiguration<Int32>^ configuration = gcnew RandomSearchConfiguration<Int32>();
//	configuration->preferHighFitness = false;
//	configuration->radius = 5;
//	//configuration->generationsMax = 10000;
//
//	FixedStepSizeDiscreteRandomSearch<Int32>^ ai = gcnew FixedStepSizeDiscreteRandomSearch<Int32>(configuration, this->problem->CreateSearchSpace(segmentSize));
//	ai->problem = this->problem;
//	ai->terminationConditions->Add(gcnew TerminationCondition_MaxGenerations<RandomSearchState^>(10000));
//
//	std::chrono::time_point<std::chrono::system_clock> start, end;
//	std::chrono::duration<double> elapsed_seconds;
//
//	start = std::chrono::system_clock::now();
//	array<Int32>^ solution = ai->Solve();
//	end = std::chrono::system_clock::now();
//	elapsed_seconds = end - start;
//
//	Console::WriteLine("Fitness:" + ai->bestFitness.ToString(L"F2"));
//	Console::WriteLine("Elapsed Time:  " + elapsed_seconds.count());
//	OutputModel(this->problem, solution);
//}

//void PMIProcessor::OutputModelOLD(PMIProblem^ Problem, array<Int32>^ Solution)
//{
//	if (this->output)
//	{
//		Int32 index = 0;
//		Int32 local = 0;
//
//		// Generate an axiom
//		Axiom^ a = gcnew Axiom();
//		Int32 axiomConstantsCount = 0;
//		Int32 axiomModulesCount = 0;
//
//		//while ((local < Problem->MaxAxiomLength) && (Problem->vocabulary->Modules[Solution[index]]->Value() != Vocabulary::EndSymbol) && (Problem->vocabulary->Modules[Solution[index]]->Value() != Vocabulary::StopSymbol))
//		while (local < this->problem->MaxAxiomLength)
//		{
//			if ((axiomConstantsCount < this->problem->axiomConstants) && (axiomModulesCount < this->problem->axiomModules))
//			{
//				// Decode as any symbol
//				a->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//
//				if ((Solution[index] >= this->problem->vocabulary->IndexModulesStart) && (Solution[index] <= this->problem->vocabulary->IndexModulesEnd))
//				{
//					axiomModulesCount++;
//				}
//				else
//				{
//					axiomConstantsCount++;
//				}
//			}
//			else if (axiomConstantsCount == this->problem->axiomConstants)
//			{
//				// Decode as Module, no more constants permitted!
//				if ((Solution[index] >= this->problem->vocabulary->IndexModulesStart) && (Solution[index] <= this->problem->vocabulary->IndexModulesEnd))
//				{
//					a->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp -= this->problem->vocabulary->IndexConstantsStart;
//					} while (tmp > this->problem->vocabulary->IndexModulesEnd);
//
//					a->Add(this->problem->vocabulary->Abstracts[tmp]);
//				}
//
//				axiomModulesCount++;
//			}
//			else
//			{
//				if ((Solution[index] >= this->problem->vocabulary->IndexConstantsStart) && (Solution[index] <= this->problem->vocabulary->IndexConstantsEnd))
//				{
//					a->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//				}
//				else
//				{
//					Int32 tmp = Solution[index];
//
//					do
//					{
//						tmp += this->problem->vocabulary->IndexConstantsStart;
//					} while (tmp < this->problem->vocabulary->IndexModulesStart);
//
//					if (tmp > this->problem->vocabulary->IndexConstantsEnd)
//					{
//						tmp -= this->problem->vocabulary->IndexModulesEnd;
//					}
//
//					a->Add(this->problem->vocabulary->Abstracts[tmp]);
//				}
//				axiomConstantsCount++;
//			}
//
//			index++;
//			local++;
//		}
//		index = this->problem->MaxAxiomLength;
//		// Generate production rules
//		List<ProductionRule^>^ R = gcnew List<ProductionRule^>(0);
//
//		for (size_t iRule = 0; iRule < Problem->numRules; iRule++)
//		{
//			Symbol^ operation = Problem->vocabulary->YieldsSymbol;
//			//index++;
//
//			local = 0;
//			List<Symbol^>^ predecessor = gcnew List<Symbol^>(0);
//			predecessor->Add(Problem->vocabulary->Abstracts[iRule]);
//
//			local = 0;
//			List<Symbol^>^ successor = gcnew List<Symbol^>(0);
//			Int32 successorConstantsCount = 0;
//			Int32 successorModulesCount = 0;
//
//			while (local < Problem->sizeSuccessor[iRule])
//			{
//				if ((successorConstantsCount < this->problem->successorConstants[iRule]) && (successorModulesCount < this->problem->successorModules[iRule]))
//				{
//					// Decode as any symbol
//					successor->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//
//					if ((Solution[index] >= this->problem->vocabulary->IndexModulesStart) && (Solution[index] <= this->problem->vocabulary->IndexModulesEnd))
//					{
//						successorModulesCount++;
//					}
//					else
//					{
//						successorConstantsCount++;
//					}
//				}
//				else if (successorConstantsCount == this->problem->successorConstants[iRule])
//				{
//					// Decode as Module, no more constants permitted!
//					if ((Solution[index] >= this->problem->vocabulary->IndexModulesStart) && (Solution[index] <= this->problem->vocabulary->IndexModulesEnd))
//					{
//						successor->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//					}
//					else
//					{
//						Int32 tmp = Solution[index];
//
//						do
//						{
//							tmp -= this->problem->vocabulary->IndexConstantsStart;
//						} while (tmp > this->problem->vocabulary->IndexModulesEnd);
//
//						successor->Add(this->problem->vocabulary->Abstracts[tmp]);
//					}
//
//					successorModulesCount++;
//				}
//				else
//				{
//					if ((Solution[index] >= this->problem->vocabulary->IndexConstantsStart) && (Solution[index] <= this->problem->vocabulary->IndexConstantsEnd))
//					{
//						successor->Add(this->problem->vocabulary->Abstracts[Solution[index]]);
//					}
//					else
//					{
//						Int32 tmp = Solution[index];
//
//						do
//						{
//							tmp += this->problem->vocabulary->IndexConstantsStart;
//						} while (tmp < this->problem->vocabulary->IndexModulesStart);
//
//						if (tmp > this->problem->vocabulary->IndexConstantsEnd)
//						{
//							tmp -= this->problem->vocabulary->IndexModulesEnd;
//						}
//
//						successor->Add(this->problem->vocabulary->Abstracts[tmp]);
//					}
//
//					successorConstantsCount++;
//				}
//
//				index++;
//				local++;
//			}
//
//			//	successor->Add(Problem->vocabulary->Abstracts[Solution[index]]);
//			//	index++;
//			//	local++;
//			//}
//
//			index += (Problem->sizeSuccessor[iRule] - local);
//
//			ProductionRule^ R1 = gcnew ProductionRule(predecessor, operation, successor);
//
//			R->Add(R1);
//		}
//
//		Console::WriteLine("Problem");
//		Console::WriteLine(" Axiom = " + Problem->axiom->ToString());
//		Console::WriteLine(" Rules: ");
//		for each (ProductionRule^ r in Problem->rules)
//		{
//			Console::WriteLine("   " + r->ToString());
//		}
//		Console::WriteLine();
//		Console::WriteLine("Solution");
//		Console::WriteLine(" Axiom = " + a->ToString());
//		Console::WriteLine(" Rules: ");
//		for each (ProductionRule^ r in R)
//		{
//			Console::WriteLine("   " + r->ToString());
//		}
//
//		Console::WriteLine();
//
//
//		for (size_t i = 0; i < Problem->generations; i++)
//		{
//			List<Symbol^>^ x = gcnew List<Symbol^>;
//
//			for each (Symbol^ s in a->symbols)
//			{
//				int index = 0;
//				bool found = false;
//
//				do
//				{
//					found = R[index]->IsMatch(s);
//					if (!found)
//					{
//						index++;
//					}
//				} while ((index < R->Count) && (!found));
//
//				if (found)
//				{
//					x->AddRange(R[index]->Produce(s));
//				}
//				else
//				{
//					x->Add(s);
//				}
//			}
//
//			a->symbols = x;
//
//#if _PMI_PROCESSOR_VERBOSE_ >= 2
//			Console::WriteLine(" Evidence #" + i.ToString(L"G") + ": " + Problem->evidence[i]->ToString());
//			Console::WriteLine(" Solution #" + i.ToString(L"G") + ": " + a->ToString());
//#endif
//		}
//
//	}
//
//#if _PMI_PROCESSOR_VERBOSE_ >= 2
//	Console::WriteLine("Press enter to continue");
//	Console::ReadLine();
//#endif
//}

//Axiom^ PMIProcessor::ChangeAxiomMenu()
//{
//	Console::WriteLine("Enter new axiom:");
//	String^ newAxiom = Console::ReadLine();
//	array<String^,1>^ words;
//	array<Char>^ seperator = gcnew array<Char>{' '};
//	Axiom^ result = gcnew Axiom();
//
//	words = newAxiom->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);
//
//	for (size_t i = 0; i < words->Length; i++)
//	{
//		bool found = false;
//		Int32 iSymbol = 0;
//
//		while ((iSymbol < this->problem->vocabulary->Symbols->Count) && (!found))
//		{
//			if (words[i] == this->problem->vocabulary->Symbols[iSymbol]->Value())
//			{
//				result->Add(this->problem->vocabulary->Symbols[iSymbol]);
//				found = true;
//			}
//			iSymbol++;
//		}
//
//		if (!found)
//		{
//			Console::WriteLine("Symbol " + words[i] + " not found in vocabulary");
//			result = this->problem->axiom;
//		}
//	}
//
//	return result;
//}
//
//List<ProductionRule^>^ PMIProcessor::ChangeRulesMenu()
//{
//	Console::WriteLine("Enter rule number and then new successor in the following syntax (#|S):");
//	String^ newRule = Console::ReadLine();
//	int ruleNum;
//
//	array<String^, 1>^ words;
//	array<Char>^ seperator = gcnew array<Char>{'|'};
//	array<String^, 1>^ symbols;
//	words = newRule->Split(seperator, System::StringSplitOptions::RemoveEmptyEntries);
//	int::TryParse(words[0], ruleNum);
//
//	List<Symbol^>^ predecessor = gcnew List<Symbol^>(0);
//	predecessor = this->problem->rules[ruleNum]->predecessor;
//
//	//array<String^, 1>^ symbols = words[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
//	//for (size_t i = 0; i < symbols->Length; i++)
//	//{
//	//	bool found = false;
//	//	Int32 iSymbol = 0;
//
//	//	while ((iSymbol < this->problem->vocabulary->Symbols->Count) && (!found))
//	//	{
//	//		if (symbols[i] == this->problem->vocabulary->Symbols[iSymbol]->Value())
//	//		{
//	//			predecessor->Add(this->problem->vocabulary->Symbols[iSymbol]);
//	//			found = true;
//	//		}
//	//		iSymbol++;
//	//	}
//
//	//	if (!found)
//	//	{
//	//		return this->problem->rules;
//	//	}
//	//}
//
//	Symbol^ operation = this->problem->vocabulary->YieldsSymbol;
//
//	List<Symbol^>^ successor = gcnew List<Symbol^>(0);
//	symbols = words[1]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
//	for (size_t i = 0; i < symbols->Length; i++)
//	{
//		bool found = false;
//		Int32 iSymbol = 0;
//
//		while ((iSymbol < this->problem->vocabulary->Symbols->Count) && (!found))
//		{
//			if (symbols[i] == this->problem->vocabulary->Symbols[iSymbol]->Value())
//			{
//				successor->Add(this->problem->vocabulary->Symbols[iSymbol]);
//				found = true;
//			}
//			iSymbol++;
//		}
//
//		if (!found)
//		{
//			return this->problem->rules;
//		}
//	}
//	
//	ProductionRule^ r = gcnew ProductionRule(predecessor, operation, successor);
//
//
//	this->problem->rules[ruleNum] = r;
//	
//	return this->problem->rules;
//}
