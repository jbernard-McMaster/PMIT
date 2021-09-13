/**
 * @file parser.h
 *
 * @brief
 * C++ Expression parser
 *
 * Features:
 *     Operators:
 *         & | << >>
 *         = <> < > <= >=
 *         + -
 *         * / % ||
 *         ^
 *         !
 *
 *     Functions:
 *         Abs, Exp, Sign, Sqrt, Log, Log10
 *         Sin, Cos, Tan, ASin, ACos, ATan
 *         Factorial
 *
 *     Variables:
 *         Pi, e
 *         you can define your own variables
 *
 *     Other:
 *        Scientific notation supported
 *         Error handling supported
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



#ifndef PARSER_H
#define PARSER_H

// declarations
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>

#include "constants.h"
//#include "error.h"
#include "functions.h"
#include "variablelist.h"

//using namespace std;

using namespace System;

public ref class MathParser
{
    // public functions
    public:
        MathParser();
        double parse(String^ expr);

    // enumerations
    private:

        enum class TOKENTYPE {NOTHING = -1, DELIMETER, NUMBER, VARIABLE, FUNCTION, UNKNOWN};

        enum class OPERATOR_ID {AND, OR, BITSHIFTLEFT, BITSHIFTRIGHT,                 // level 2
                       EQUAL, UNEQUAL, SMALLER, LARGER, SMALLEREQ, LARGEREQ,    // level 3
                       PLUS, MINUS,                     // level 4
                       MULTIPLY, DIVIDE, MODULUS, XOR,  // level 5
                       POW,                             // level 6
                       FACTORIAL};                      // level 7

    // data
    private:
        String^ expr;    // holds the expression
        char e;                      // points to a character in expr

		Int32 iSymbol;
		Int32 iToken;
		String^ token;   // holds the token
        TOKENTYPE token_type;         // type of the token

        double ans;                   // holds the result of the expression
        String^ ans_str;            // holds a string containing the result
                                      // of the expression

        Variablelist user_var;        // list with variables defined by user

    // private functions
    private:
        void getToken();

        double parse_level1();
        double parse_level2();
        double parse_level3();
        double parse_level4();
        double parse_level5();
        double parse_level6();
        double parse_level7();
        double parse_level8();
        double parse_level9();
        double parse_level10();
        double parse_number();

        int get_operator_id(String^ op_name);
        double eval_operator(const int op_id, const double &lhs, const double &rhs);
        double eval_function(String^ fn_name, const double &value);
        double eval_variable(String^ var_name);

        int row();
        //int col();
};

#endif
