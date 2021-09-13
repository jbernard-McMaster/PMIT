#include "stdafx.h"
#include "ProductionRuleV3.h"


ProductionRuleV3::ProductionRuleV3()
{
	this->successor = gcnew List<String^>;
	this->odds = gcnew List<Int32>;
	this->contextLeft = gcnew List<String^>;
	this->contextRight = gcnew List<String^>;
	this->condition = gcnew List<String^>;
	this->count = gcnew List<Int32>;
	this->parameters = gcnew List<String^>;
	//this->timeStart = gcnew List<Int32>; // used for a special time condition when time is the only parameter since time is such a common parameter
	//this->timeEnd = gcnew List<Int32>; // used for a special time condition when time is the only parameter since time is such a common parameter
}

ProductionRuleV3::~ProductionRuleV3()
{
}

void ProductionRuleV3::Display()
{
	for (size_t iRule = 0; iRule < this->successor->Count; iRule++)
	{
		String^ leftStr = "";
		String^ rightStr = "";

		if (this->contextLeft[iRule] == "")
		{
			leftStr = "*";
		}
		else
		{
			leftStr = this->contextLeft[iRule];
		}

		if (this->contextRight[iRule] == "")
		{
			rightStr = "*";
		}
		else
		{
			rightStr = this->contextRight[iRule];
		}

		Console::WriteLine(leftStr + " < " + this->predecessor + " > " + rightStr + " -> " + this->successor[iRule]);
	}
}

bool ProductionRuleV3::IsMatchPredecessor(String^ P)
{
	return this->predecessor == P;
}

Int32 ProductionRuleV3::GetSuccessorID(String^ S)
{
	bool found = false;
	Int32 id = 0;

	while (!found && id < this->successor->Count)
	{
		if (S == this->successor[id])
		{
			found = true;
		}
		else
		{
			id++;
		}
	}

	return id;
}

String^ ProductionRuleV3::Produce(String^ L, String^ R, Int32 O, Int32 T, List<float>^ V)
{
	String^ result = this->predecessor;

	Int32 roll = O;
	Int32 iRule = 0;
	bool found = false;
	while ((!found) && (iRule < this->successor->Count))
	{
		if (this->ShouldFire(iRule, L, R, roll, T, V))
		{
			found = true;
			this->count[iRule]++;
		}
		else
		{
			roll = Math::Max(0, roll - this->odds[iRule]);
			iRule++;
		}
	}

	if (found)
	{
		result = this->successor[iRule];

		if (this->parameters->Count > 0)
		{
			String^ tmp1 = result;
			// TODO: this doesn't work. It needs to convert any equations in the successor with a value

			char expr[255] = "";
			MathParser^ prs;

			// replace variables with values
			for (size_t iParameter = 0; iParameter < this->parameters->Count; iParameter++)
			{
				tmp1 = tmp1->Replace(this->parameters[iParameter], V[iParameter].ToString());
			}

			array<Char>^ tmp2 = tmp1->ToCharArray();

			for (size_t i = 0; i <= tmp2->GetUpperBound(0); i++)
			{
				expr[i] = tmp2[i];
			}

			//double ans = prs->parse(expr);
		}
	}

	return result;
}

bool ProductionRuleV3::ShouldFire(Int32 RuleIndex, String^ L, String^ R, Int32 O, Int32 T, List<float>^ V)
{
	bool result = false;

	// Check odds
	if (O <= this->odds[RuleIndex])
	{
		// Check context
		if (((L == this->contextLeft[RuleIndex]) || (this->contextLeft[RuleIndex] == "*")) &&
			((R == this->contextRight[RuleIndex]) || (this->contextRight[RuleIndex] == "*")))
		{
			if (this->condition[RuleIndex] != "*")
			{
				// TODO: make this a global common parser
				MathParser^ prs = gcnew MathParser();
				double  ans = prs->parse(this->condition[RuleIndex]);

				if (ans == 1.0)
				{
					result = true;
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = true;
			}
		}
	}
	
	return result;
}

