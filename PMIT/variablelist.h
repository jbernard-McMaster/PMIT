/**
 * @file VariableList.h
 * @brief The class VariableList can manages a list with variables.
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
 * @author 	Jos de Jong, <wjosdejong@gmail.com>
 * @date	2007, updated 2012-02-06
 */


#ifndef USER_VARIABLES_H
#define USER_VARIABLES_H

#include <cstdio>
#include <cctype>
#include <cstring>
#include <vector>

#include "constants.h"

//using namespace std;
using namespace System;
using namespace System::Collections::Generic;

void toupper(char upper[], const char str[]);

public ref struct VAR
{
	String^ name;
	double value;
};

public ref class Variablelist 
{
public:
        bool exist(String^ name);
        bool add(String^ name, double value);
        bool del(String^ name);

        bool get_value(String^ name, double* value);
        bool get_value(const int id, double* value);
        int  get_id(String^ name);
        bool set_value(String^ name, const double value);

private:

        List<VAR^>^ var;
};


#endif
