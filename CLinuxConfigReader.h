/*
   Copyright [2010] [Richard Bross]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* 
 * File:   CLinuxConfigReader.h
 * Author: rbross
 *
 * Created on April 20, 2012, 12:47 PM
 */

#ifndef CLINUXCONFIGREADER_H
#define	CLINUXCONFIGREADER_H

#include <map>
#include <string>

using namespace std;

typedef map<string, string> MAP_CONFIG;
typedef MAP_CONFIG::iterator MAP_CONFIG_ITER;

//! A simple class to read a text configuration file and insert values into a map
class CLinuxConfigReader
{
public:
    CLinuxConfigReader();
    CLinuxConfigReader(const CLinuxConfigReader& orig);
    virtual ~CLinuxConfigReader();

    //! Read a configuration value
    string &ReadConfigValue(const char *pKey, const char *pDefault = NULL);
    //! Read a configuration file
    bool ReadConfig(const char *pPath);

public:
    MAP_CONFIG  mConfig;
private:
static string sValue;

};

#endif	/* CLINUXCONFIGREADER_H */

