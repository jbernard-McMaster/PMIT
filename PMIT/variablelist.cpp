/**
 * @file VariableList.cpp
 *
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


#include "stdafx.h"
#include "variablelist.h"

/*
 * Returns true if the given name already exists in the variable list
 */
bool Variablelist::exist(String^ name)
{
    return (get_id(name) != -1);
}


/*
 * Add a name and value to the variable list
 */
bool Variablelist::add(String^ name, double value)
{
    VAR^ new_var = gcnew VAR();
	new_var->name = name;
    //strncpy(new_var.name, name, 30);
    new_var->value = value;

    int id = get_id(name);
    if (id == -1)
    {
        // variable does not yet exist
        var->Add(new_var);
    }
    else
    {
        // variable already exists. overwrite it
        var[id] = new_var;
    }
    return true;
}

/*
 * Delete given variablename from the variable list
 */
bool Variablelist::del(String^ name)
{
    int id = get_id(name);
    if (id != -1)
    {
		var->RemoveAt(id);
		return true;
    }
    return false;
}

/*
 * Get value of variable with given name
 */
bool Variablelist::get_value(String^ name, double* value)
{
    int id = get_id(name);
    if (id != -1)
    {
        *value = var[id]->value;
        return true;
    }
    return false;
}


/*
 * Get value of variable with given id
 */
bool Variablelist::get_value(const int id, double* value)
{
    if (id >= 0 && id < (int)var->Count)
    {
        *value = var[id]->value;
        return true;
    }
    return false;
}


bool Variablelist::set_value(String^ name, const double value)
{
    return add(name, value);
}

/*
 * Returns the id of the given name in the variable list. Returns -1 if name
 * is not present in the list. Name is case insensitive
 */
int Variablelist::get_id(String^ name)
{
    // first make the name uppercase
	String^ nameU = name->ToUpper();
    for (unsigned int i = 0; i < var->Count; i++)
    {
		if (nameU->ToUpper() == var[i]->name->ToUpper())
		{
			return i;
		}
    }
    return -1;
}


/*
 * str is copied to upper and made uppercase
 * upper is the returned string
 * str should be null-terminated
 */
void toupper(char upper[], const char str[])
{
    int i = -1;
    do
    {
        i++;
        upper[i] = std::toupper(str[i]);
    }
    while (str[i] != '\0');
}
