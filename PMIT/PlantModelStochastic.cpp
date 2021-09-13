#include "stdafx.h"
#include "PlantModelStochastic.h"


PlantModelStochastic::PlantModelStochastic()
{
	this->rules = gcnew List<ProductionRuleStochastic^>;
}


PlantModelStochastic::~PlantModelStochastic()
{
}

void PlantModelStochastic::CreateEvidence()
{

#if _PMI_PLANT_MODEL_VERBOSE_ >= 1
	Console::WriteLine("Problem");
	Console::WriteLine(" Axiom = " + this->axiom->ToString());
	Console::WriteLine(" Rules: ");
	for each (ProductionRule^ r in this->rules)
	{
		Console::WriteLine("   " + r->ToString());
	}

	Console::WriteLine();
#endif

	// Create the solution
	this->evidence = gcnew List<Word^>;

	// Create the evidence for this solution
	Word^ a = gcnew Word();
	for each (Symbol^ s in this->axiom->symbols)
	{
		a->Add(s);
	}

	this->evidence = EvidenceFactory::Process(a, this->rules, 0, this->generations, false);

	for each (Word^ a in this->evidence)
	{
		this->vocabulary->CompressSymbols(a);
	}

	List<Symbol^>^ constants = this->vocabulary->AllConstants();

	// Make a copy of all the evidence while removing the constants
	this->evidenceModuleOnly = gcnew List<Word^>;
	for (size_t iGen = 0; iGen < this->evidence->Count; iGen++)
	{
		Word^ a = gcnew Word(this->evidence[iGen]);
		a->FilterOutSymbol(constants);
		this->vocabulary->CompressSymbols(a);
		this->evidenceModuleOnly->Add(a);
	}

#if _PMI_PLANT_MODEL_VERBOSE_ >= 2
	for (size_t i = 0; i < this->generations; i++)
	{
		Console::WriteLine("Evidence #" + i.ToString(L"G") + " : " + this->evidence[i]->ToString());
	}
	Console::ReadLine();
#endif
}

bool PlantModelStochastic::IsMatch(PlantModel^ Model)
{
	return false;
}

void PlantModelStochastic::LoadRule(array<String^, 1>^ Words)
{
	List<IProductionRuleConstraint^>^ constraints = gcnew List<IProductionRuleConstraint^>;
	array<String^, 1>^ parts = Words[PlantModel::cRuleSymbols]->Split(gcnew array<Char>{'|'}, System::StringSplitOptions::RemoveEmptyEntries);

	array<String^, 1>^ symbols = parts[0]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
	String^ p = symbols[0]; // the activating symbol of the predecessor is always in the 1st spot
	Symbol^ predecessor = this->vocabulary->FindSymbol(p);

	// Create the successor
	List<Symbol^>^ successor = gcnew List<Symbol^>(0);
	symbols = parts[2]->Split(gcnew array<Char>{' '}, System::StringSplitOptions::RemoveEmptyEntries);
	for (size_t i = 0; i < symbols->Length; i++)
	{
		successor->Add(this->vocabulary->FindSymbol(symbols[i]));
	}

	float chance;
	float::TryParse(Words[3], chance);

	Successor^ S = gcnew Successor(successor, chance);

	// Check to see if a rule already exists with this predecessor
	for (size_t iRule = 0; iRule < this->rules->Count; iRule++)
	{
		if (this->rules[iRule]->predecessor == predecessor)
		{
			// if so, then simply add the successor to the rule
			this->rules[iRule]->successors->Add(S);
		}
	}

	// Otherwise continue with the rest of the rule

	// There is a constraint or parameter
	if (symbols->Length > 1)
	{
		// If the second symbol is a ( then it is a parameter list
		// If the second symbol is < or > then it is a context constraint
		if (symbols[1] == "(")
		{

		}
		else if ((symbols[1] == "<") || (symbols[1] == ">"))
		{
			String^ c = symbols[2];
			Symbol^ contextSymbol = this->vocabulary->FindSymbol(c);
			if (symbols[1] == "<")
			{
				ProductionRuleConstraint_ContextLeft^ contextConstraint = gcnew ProductionRuleConstraint_ContextLeft(contextSymbol);
				constraints->Add(contextConstraint);
			}
			else if (symbols[1] == ">")
			{
				ProductionRuleConstraint_ContextRight^ contextConstraint = gcnew ProductionRuleConstraint_ContextRight(contextSymbol);
				constraints->Add(contextConstraint);
			}
		}
	}

	// Create the operation. Currently only Yields operations are used.
	Symbol^ operation = this->vocabulary->YieldsSymbol;

	// Create a time constraint if there is any
	array<String^, 1>^ genConstraint = Words[PlantModel::cRuleGenerationConstraint]->Split(gcnew array<Char>{'|'}, System::StringSplitOptions::RemoveEmptyEntries);;

	Int32 start = 0;
	Int32 end = ProductionRule::cEndOfTime;

	if ((genConstraint[0] != "*") && (genConstraint[1] == "*"))
	{
		int::TryParse(genConstraint[0], start);
		end = ProductionRule::cEndOfTime;
	}
	else if ((genConstraint[0] != "*") && (genConstraint[1] != "*"))
	{
		int::TryParse(genConstraint[0], start);
		int::TryParse(genConstraint[1], end);
	}

	ProductionRuleConstraint_Time^ constraint = gcnew ProductionRuleConstraint_Time(start, end);
	constraints->Add(constraint);

	// Create the production
	ProductionRuleStochastic^ r = gcnew ProductionRuleStochastic(predecessor, operation, S, constraints);

	// Add it to the rule set
	this->rules->Add(r);
}
