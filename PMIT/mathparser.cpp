/**
 * @file parser.h
 *
 * @brief
 * C++ Expression parser. See the header file for more detailed explanation
 *
 * @license
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * Copyright (C) 2007-2011 Jos de Jong, http://www.speqmath.com
 *
 * The parser is based on the example "A mini C++ Interpreter" from the
 * book "The art of C++" by Herbert Schildt.
 *
 * @author 	Jos de Jong, <wjosdejong@gmail.com>
 * @date	2007-12-22, updated 2012-02-06
 */

// declarations
#include "stdafx.h"
#include "mathparser.h"


using namespace std;



/*
 * constructor.
 * Initializes all data with zeros and empty strings
 */
MathParser::MathParser()
{
	expr = "";
	e = '\0';
    //expr[0] = '\0';
    //e = NULL;

	token = "";
    //token[0] = '\0';
    token_type = TOKENTYPE::NOTHING;
}


/**
 * parses and evaluates the given expression
 * On error, an error of type Error is thrown
 */
double MathParser::parse(String^ new_expr)
{
    //try
    //{
        // check the length of expr
        if (new_expr->Length > EXPR_LEN_MAX)
        {
			throw gcnew Exception("Expression too long");
			// throw Error(row(), col(), 200);
        }

        // initialize all variables
        //strncpy(expr, new_expr, EXPR_LEN_MAX - 1);  // copy the given expression to expr
		iToken = 0;
		iSymbol = 0;
		expr = new_expr; // copy the given expression to expr
        e = expr[0];                                  // let e point to the start of the expression
        ans = 0;

        getToken();
        if (token_type == TOKENTYPE::DELIMETER && token == "\0")
        {
			throw gcnew Exception("Nope");
            //throw Error(row(), col(), 4);
        }

        ans = parse_level1();

        // check for garbage at the end of the expression
        // an expression ends with a character '\0' and token_type = TOKENTYPE::DELIMETER
        if (token_type != TOKENTYPE::DELIMETER || token != "\0")
        {
            if (token_type == TOKENTYPE::DELIMETER)
            {
				throw gcnew Exception("Bad operator");
				// user entered a not existing operator like "//"
                //throw Error(row(), col(), 101, token);
            }
            else
            {
				throw gcnew Exception("Nope");
				//throw Error(row(), col(), 5, token);
            }
        }

        // add the answer to memory as variable "Ans"
        user_var.add("Ans", ans);

        //snprintf(ans_str, sizeof(ans_str), "Ans = %g", ans);
		//snprintf(ans_str, sizeof(ans_str), "%g", ans);
	//}
    //catch (Error err)
    //{
    //    if (err.get_row() == -1)
    //    {
    //        snprintf(ans_str, sizeof(ans_str), "Error: %s (col %i)", err.get_msg(), err.get_col());
    //    }
    //    else
    //    {
    //        snprintf(ans_str, sizeof(ans_str), "Error: %s (ln %i, col %i)", err.get_msg(), err.get_row(), err.get_col());
    //    }
    //}

    return ans;
}


/*
 * checks if the given char c is a minus
 */
bool isMinus(const char c)
{
    if (c == 0) return 0;
    return c == '-';
}

/*
 * checks if the given char c is whitespace
 * whitespace when space chr(32) or tab chr(9)
 */
bool isWhiteSpace(const char c)
{
    if (c == 0) return 0;
    return c == 32 || c == 9;  // space or tab
}

/*
 * checks if the given char c is a TOKENTYPE::DELIMETER
 * minus is checked apart, can be unary minus
 */
bool isDelimeter(char c)
{
    if (c == 0) return 0;
    return strchr("&|<>=+/*%^!", c) != 0;
}

/*
 * checks if the given char c is NO TOKENTYPE::DELIMETER
 */
bool isNotDelimeter(const char c)
{
    if (c == 0) return 0;
    return strchr("&|<>=+-/*%^!()", c) != 0;
}

/*
 * checks if the given char c is a letter or undersquare
 */
bool isAlpha(const char c)
{
    if (c == 0) return 0;
    return strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ_", toupper(c)) != 0;
}

/*
 * checks if the given char c is a digit or dot
 */
bool isDigitDot(const char c)
{
    if (c == 0) return 0;
    return strchr("0123456789.", c) != 0;
}

/*
 * checks if the given char c is a digit
 */
bool isDigit(const char c)
{
    if (c == 0) return 0;
    return strchr("0123456789", c) != 0;
}


/**
 * Get next token in the current string expr.
 * Uses the Parser data expr, e, token, t, token_type and err
 */
void MathParser::getToken()
{
    token_type = TOKENTYPE::NOTHING;
    char t;           // points to a character in token
	iToken = 0;
    t = token[0];         // let t point to the first character in token
    //*t = '\0';         // set token empty

    //printf("\tgetToken e:{%c}, ascii=%i, col=%i\n", *e, *e, e-expr);

    // skip over whitespaces
    while (e == ' ' || e == '\t')     // space or tab
    {
        iSymbol++;
    }

    // check for end of expression
    if (iSymbol > expr->Length)
    {
        // token is still empty
        token_type = TOKENTYPE::DELIMETER;
        return;
    }

    // check for minus
    if (e == '-')
    {
        token_type = TOKENTYPE::DELIMETER;
        t = e;
        iSymbol++;
        iToken++;
        //*t = '\0';  // add a null character at the end of token
        return;
    }

    // check for parentheses
    if (e == '(' || e == ')')
    {
        token_type = TOKENTYPE::DELIMETER;
        t = e;
        iSymbol++;
        iToken++;
        //*t = '\0';
        return;
    }

    // check for operators (TOKENTYPE::DELIMETERs)
    if (isDelimeter(e))
    {
        token_type = TOKENTYPE::DELIMETER;
        while (isDelimeter(e))
        {
            //*t = *e;
            //e++;
            //t++;
        }
        //*t = '\0';  // add a null character at the end of token
        return;
    }

    // check for a value
    if (isDigitDot(e))
    {
        token_type = TOKENTYPE::NUMBER;
        while (isDigitDot(e))
        {
            //*t = *e;
            //e++;
            //t++;
        }

        //// check for scientific notation like "2.3e-4" or "1.23e50"
        //if (toupper(*e) == 'E')
        //{
        //    *t = *e;
        //    e++;
        //    t++;

        //    if (*e == '+' || *e == '-')
        //    {
        //        *t = *e;
        //        e++;
        //        t++;
        //    }

        //    while (isDigit(*e))
        //    {
        //        *t = *e;
        //        e++;
        //        t++;
        //    }
        //}

        //*t = '\0';
        return;
    }

    // check for variables or functions
    if (isAlpha(e))
    {
        while (isAlpha(e) || isDigit(e))
        {
            //*t = *e;
            //e++;
            //t++;
        }
        //*t = '\0';  // add a null character at the end of token

        // check if this is a variable or a function.
        // a function has a parentesis '(' open after the name
        //char* e2 = NULL;
        //e2 = e;

        // skip whitespaces
        //while (*e2 == ' ' || *e2 == '\t')     // space or tab
        //{
        //    e2++;
        //}

        //if (*e2 == '(')
        //{
        //    token_type = FUNCTION;
        //}
        //else
        //{
        //    token_type = VARIABLE;
        //}
        return;
    }

    // something unknown is found, wrong characters -> a syntax error
    token_type = TOKENTYPE::UNKNOWN;
    //while (*e != '\0')
    //{
    //    *t = *e;
    //    e++;
    //    t++;
    //}
    //*t = '\0';
    //throw Error(row(), col(), 1, token);

    return;
}


/*
 * assignment of variable or function
 */
double MathParser::parse_level1()
{
    if (token_type == TOKENTYPE::VARIABLE)
    {
        // copy current token
        char e_now = e;
        TOKENTYPE token_type_now = token_type;
		String^ token_now = token;
        //strcpy(token_now, token);

        getToken();
        if (token == "=")
        {
            // assignment
            double ans;
            getToken();
            ans = parse_level2();
            //if (user_var.add(token_now, ans) == false)
            //{
            //    throw Error(row(), col(), 300);
            //}
            return ans;
        }
        else
        {
            // go back to previous token
            e = e_now;
            token_type = token_type_now;
			token = token_now;
            //strcpy(token, token_now);
        }
    }

    return parse_level2();
}


/*
 * conditional operators and bitshift
 */
double MathParser::parse_level2()
{
    int op_id;
    double ans;
    ans = parse_level3();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::AND || op_id == (int)OPERATOR_ID::OR || op_id == (int)OPERATOR_ID::BITSHIFTLEFT || op_id == (int)OPERATOR_ID::BITSHIFTRIGHT)
    {
        getToken();
        ans = eval_operator(op_id, ans, parse_level3());
        op_id = get_operator_id(token);
    }

    return ans;
}

/*
 * conditional operators
 */
double MathParser::parse_level3()
{
    int op_id;
    double ans;
    ans = parse_level4();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::EQUAL || op_id == (int)OPERATOR_ID::UNEQUAL || op_id == (int)OPERATOR_ID::SMALLER || op_id == (int)OPERATOR_ID::LARGER || op_id == (int)OPERATOR_ID::SMALLEREQ || op_id == (int)OPERATOR_ID::LARGEREQ)
    {
        getToken();
        ans = eval_operator(op_id, ans, parse_level4());
        op_id = get_operator_id(token);
    }

    return ans;
}

/*
 * add or subtract
 */
double MathParser::parse_level4()
{
    int op_id;
    double ans;
    ans = parse_level5();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::PLUS || op_id == (int)OPERATOR_ID::MINUS)
    {
        getToken();
        ans = eval_operator(op_id, ans, parse_level5());
        op_id = get_operator_id(token);
    }

    return ans;
}


/*
 * multiply, divide, modulus, xor
 */
double MathParser::parse_level5()
{
    int op_id;
    double ans;
    ans = parse_level6();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::MULTIPLY || op_id == (int)OPERATOR_ID::DIVIDE || op_id == (int)OPERATOR_ID::MODULUS || op_id == (int)OPERATOR_ID::XOR)
    {
        getToken();
        ans = eval_operator(op_id, ans, parse_level6());
        op_id = get_operator_id(token);
    }

    return ans;
}


/*
 * power
 */
double MathParser::parse_level6()
{
    int op_id;
    double ans;
    ans = parse_level7();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::POW)
    {
        getToken();
        ans = eval_operator(op_id, ans, parse_level7());
        op_id = get_operator_id(token);
    }

    return ans;
}

/*
 * Factorial
 */
double MathParser::parse_level7()
{
    int op_id;
    double ans;
    ans = parse_level8();

    op_id = get_operator_id(token);
    while (op_id == (int)OPERATOR_ID::FACTORIAL)
    {
        getToken();
        // factorial does not need a value right from the
        // operator, so zero is filled in.
        ans = eval_operator(op_id, ans, 0.0);
        op_id = get_operator_id(token);
    }

    return ans;
}

/*
 * Unary minus
 */
double MathParser::parse_level8()
{
    double ans;

    int op_id = get_operator_id(token);
    if (op_id == (int)OPERATOR_ID::MINUS)
    {
        getToken();
        ans = parse_level9();
        ans = -ans;
    }
    else
    {
        ans = parse_level9();
    }

    return ans;
}


/*
 * functions
 */
double MathParser::parse_level9()
{
    String^ fn_name;
    double ans;

    if (token_type == TOKENTYPE::FUNCTION)
    {
		fn_name = token;
        //strcpy(fn_name, token);
        getToken();
        ans = eval_function(fn_name, parse_level10());
    }
    else
    {
        ans = parse_level10();
    }

    return ans;
}


/*
 * parenthesized expression or value
 */
double MathParser::parse_level10()
{
    // check if it is a parenthesized expression
    if (token_type == TOKENTYPE::DELIMETER)
    {
        if (token[0] == '(' && token[1] == '\0')
        {
            getToken();
            double ans = parse_level2();
            //if (token_type != TOKENTYPE::DELIMETER || token[0] != ')' || token[1] || '\0')
            //{
            //    throw Error(row(), col(), 3);
            //}
            getToken();
            return ans;
        }
    }

    // if not parenthesized then the expression is a value
    return parse_number();
}


double MathParser::parse_number()
{
double ans = 0;

    switch (token_type)
    {
	case TOKENTYPE::NUMBER:
            // this is a number
			double::TryParse(token, ans);
            //ans = strtod(token, NULL);
            getToken();
            break;

        case TOKENTYPE::VARIABLE:
            // this is a variable
            ans = eval_variable(token);
            getToken();
            break;

        default:
            // syntax error or unexpected end of expression
            //if (token[0] == '\0')
            //{
            //    throw Error(row(), col(), 6);
            //}
            //else
            //{
            //    throw Error(row(), col(), 7);
            //}
            break;
    }

    return ans;
}


/*
 * returns the id of the given operator
 * returns -1 if the operator is not recognized
 */
int MathParser::get_operator_id(String^ op_name)
{
    // level 2
    if (op_name == "&") {return (int)OPERATOR_ID::AND;}
    if (op_name == "|") {return (int)OPERATOR_ID::OR;}
    if (op_name == "<<") {return (int)OPERATOR_ID::BITSHIFTLEFT;}
    if (op_name == ">>") {return (int)OPERATOR_ID::BITSHIFTRIGHT;}

    // level 3
    if (op_name == "=") {return (int)OPERATOR_ID::EQUAL;}
    if (op_name == "<>") {return (int)OPERATOR_ID::UNEQUAL;}
    if (op_name == "<") {return (int)OPERATOR_ID::SMALLER;}
    if (op_name == ">") {return (int)OPERATOR_ID::LARGER;}
    if (op_name == "<=") {return (int)OPERATOR_ID::SMALLEREQ;}
    if (op_name == ">=") {return (int)OPERATOR_ID::LARGEREQ;}

    // level 4
    if (op_name == "+") {return (int)OPERATOR_ID::PLUS;}
    if (op_name == "-") {return (int)OPERATOR_ID::MINUS;}

    // level 5
    if (op_name == "*") {return (int)OPERATOR_ID::MULTIPLY;}
    if (op_name == "/") {return (int)OPERATOR_ID::DIVIDE;}
    if (op_name == "%") {return (int)OPERATOR_ID::MODULUS;}
    if (op_name == "||") {return (int)OPERATOR_ID::XOR;}

    // level 6
    if (op_name == "^") {return (int)OPERATOR_ID::POW;}

    // level 7
    if (op_name == "!") {return (int)OPERATOR_ID::FACTORIAL;}

    return -1;
}


/*
 * evaluate an operator for given valuess
 */
double MathParser::eval_operator(const int op_id, const double &lhs, const double &rhs)
{
    switch (op_id)
    {
        // level 2
		case (int)OPERATOR_ID::AND:           return static_cast<int>(lhs) & static_cast<int>(rhs);
        case (int)OPERATOR_ID::OR:            return static_cast<int>(lhs) | static_cast<int>(rhs);
        case (int)OPERATOR_ID::BITSHIFTLEFT:  return static_cast<int>(lhs) << static_cast<int>(rhs);
        case (int)OPERATOR_ID::BITSHIFTRIGHT: return static_cast<int>(lhs) >> static_cast<int>(rhs);

        // level 3
        case (int)OPERATOR_ID::EQUAL:     return lhs == rhs;
        case (int)OPERATOR_ID::UNEQUAL:   return lhs != rhs;
        case (int)OPERATOR_ID::SMALLER:   return lhs < rhs;
        case (int)OPERATOR_ID::LARGER:    return lhs > rhs;
        case (int)OPERATOR_ID::SMALLEREQ: return lhs <= rhs;
        case (int)OPERATOR_ID::LARGEREQ:  return lhs >= rhs;

        // level 4
        case (int)OPERATOR_ID::PLUS:      return lhs + rhs;
        case (int)OPERATOR_ID::MINUS:     return lhs - rhs;

        // level 5
        case (int)OPERATOR_ID::MULTIPLY:  return lhs * rhs;
        case (int)OPERATOR_ID::DIVIDE:    return lhs / rhs;
        case (int)OPERATOR_ID::MODULUS:   return static_cast<int>(lhs) % static_cast<int>(rhs); // todo: give a warning if the values are not integer?
        case (int)OPERATOR_ID::XOR:       return static_cast<int>(lhs) ^ static_cast<int>(rhs);

        // level 6
        case (int)OPERATOR_ID::POW:       return pow(lhs, rhs);

        // level 7
        case (int)OPERATOR_ID::FACTORIAL: return factorial(lhs);
    }

    //throw Error(row(), col(), 104, op_id);
    return 0;
}


/*
 * evaluate a function
 */
double MathParser::eval_function(String^ fn_name, const double &value)
{
    //try
    //{
        // first make the function name upper case
		String^ fnU = fn_name->ToUpper();
        //char fnU[NAME_LEN_MAX+1];
        //toupper(fnU, fn_name);

        // arithmetic
		if (fnU == "ABS") { return abs(value); }
		if (fnU == "EXP") { return exp(value); }
		if (fnU == "SIGN") { return sign(value); }
		if (fnU == "SQRT") { return sqrt(value); }
		if (fnU == "LOG") { return log(value); }
		if (fnU == "LOG10") { return log10(value); }
		//if (!strcmp(fnU, "ABS")) {return abs(value);}
  //      if (!strcmp(fnU, "EXP")) {return exp(value);}
  //      if (!strcmp(fnU, "SIGN")) {return sign(value);}
  //      if (!strcmp(fnU, "SQRT")) {return sqrt(value);}
  //      if (!strcmp(fnU, "LOG")) {return log(value);}
  //      if (!strcmp(fnU, "LOG10")) {return log10(value);}

        // trigonometric
        if (fnU == "SIN") {return sin(value);}
        if (fnU == "COS") {return cos(value);}
        if (fnU == "TAN") {return tan(value);}
        if (fnU == "ASIN") {return asin(value);}
        if (fnU == "ACOS") {return acos(value);}
        if (fnU == "ATAN") {return atan(value);}
		//if (!strcmp(fnU, "SIN")) { return sin(value); }
		//if (!strcmp(fnU, "COS")) { return cos(value); }
		//if (!strcmp(fnU, "TAN")) { return tan(value); }
		//if (!strcmp(fnU, "ASIN")) { return asin(value); }
		//if (!strcmp(fnU, "ACOS")) { return acos(value); }
		//if (!strcmp(fnU, "ATAN")) { return atan(value); }

        // probability
        //if (!strcmp(fnU, "FACTORIAL")) {return factorial(value);}
    //}
    //catch (Error err)
    //{
    //    // retrow error, add information about column and row of occurance
    //    // TODO: doesnt work yet
    //    throw Error(col(), row(), err.get_id(), err.get_msg());
    //}

    // unknown function
    //throw Error(row(), col(), 102, fn_name);
    return 0;
}

/*
 * evaluate a variable
 */
double MathParser::eval_variable(String^ var_name)
{
    // first make the variable name uppercase
	String^ varU = var_name->ToUpper();
    //char varU[NAME_LEN_MAX+1];
    //toupper(varU, var_name);

    // check for built-in variables
    if (varU == "E") {return 2.7182818284590452353602874713527;}
    if (varU == "PI") {return 3.1415926535897932384626433832795;}

    // check for user defined variables
    double ans;
    if (user_var.get_value(var_name, &ans))
    {
        return ans;
    }

    // unknown variable
    //throw Error(row(), col(), 103, var_name);
    return 0;
}



/*
 * Shortcut for getting the current row value (one based)
 * Returns the line of the currently handled expression
 */
int MathParser::row()
{
    return -1;
}

/*
 * Shortcut for getting the current col value (one based)
 * Returns the column (position) where the last token starts
 */
//int MathParser::col()
//{
//    return e-expr-strlen(token)+1;
//}
